/* tileiter.c -
 *
 * Copyright 2008 OpenMoko, Inc.
 * Authored by Chia-I Wu <olv@openmoko.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "tileiter.h"

#define RADIANS(d) ((d) * M_PI / 180.0)

static void
mercator_project(double lon, double lat, double *x, double *y)
{
	double tmp;

	if (x)
		*x = (lon + 180.0) / 360.0;

	if (!y)
		return;

	/* avoid NaN */
	if (lat > 89.99)
		lat = 89.99;
	else if (lat < -89.99)
		lat = -89.99;

	tmp = RADIANS(lat);
	tmp = log(tan(tmp) + 1.0 / cos(tmp));
	*y = (1.0 - tmp / M_PI) / 2.0;
}

static int
rule_check_format(const char *url_format, char *arg_types, int num_args)
{
	const char *p;
	int i = 0;

	p = url_format;
	for (i = 0; i < num_args + 1; i++)
	{
		while (1) {
			p = strchr(p, '%');
			if (!p || p[1] != '%')
				break;

			p += 2;
		}

		if (i == num_args && p) {
			i++;

			break;
		}

		if (!p || p[1] != arg_types[i])
			break;

		p++;
	}

	return (i == num_args);
}

static int
rule_check(int rule, int rule_data, const char *url_format, double lon, double lat, double width, double height, int min_z, int max_z)
{
	char arg_types[16];
	int num_args;

	memset(arg_types, 'd', 3);
	num_args = 3;

	switch (rule) {
	case TILE_ITER_RULE_NORMAL:
		break;
	case TILE_ITER_RULE_REVERSE_Z:
		if (rule_data < max_z)
			return 0;
		break;
	default:
		return 0;
		break;
	}

	return rule_check_format(url_format, arg_types, num_args);
}

TileIter *tile_iter_new(int rule, int rule_data, const char *url_format, double lon, double lat, double width, double height, int min_z, int max_z)
{
	TileIter *iter;

	if (!rule_check(rule, rule_data, url_format, lon, lat, width, height, min_z, max_z))
		return NULL;

	iter = malloc(sizeof(TileIter));
	if (!iter)
		return NULL;

	iter->rule = rule;
	iter->rule_data = rule_data;
	iter->url_format = strdup(url_format);
	iter->lon = lon;
	iter->lat = lat;
	iter->width = width;
	iter->height = height;
	iter->min_z = min_z;
	iter->max_z = max_z;

	mercator_project(iter->lon, iter->lat,
			&iter->left, &iter->bottom);
	mercator_project(iter->lon + iter->width,
			iter->lat + iter->height,
			&iter->right, &iter->top);
#define FIX_RANGE(v)			\
	do {				\
		if (v < 0.0)		\
			v = 0.0;	\
		else if (v > 1.0)	\
			v = 1.0;	\
	} while (0)

	FIX_RANGE(iter->left);
	FIX_RANGE(iter->bottom);
	FIX_RANGE(iter->right);
	FIX_RANGE(iter->top);

	if (iter->right < iter->left)
		iter->right = iter->left;
	if (iter->bottom < iter->top)
		iter->bottom = iter->top;

	iter->url[0] = '\0';
	iter->cur = -1;
	iter->count = 0; /* let's be lazy */

	tile_iter_reset(iter);

	return iter;
}

void tile_iter_destroy(TileIter *iter)
{
	free(iter->url_format);
	free(iter);
}

static void tile_iter_range_fix(TileIter *iter, int hori)
{
	int ntiles = 1 << iter->z;
	int *min, *max;

	if (hori) {
		min = &iter->min_x;
		max = &iter->max_x;
	} else {
		min = &iter->min_y;
		max = &iter->max_y;
	}

	if (*min < 0)
		*min = 0;
	if (*max >= ntiles)
		*max = ntiles - 1;

	if (*max - *min < 5) {
		*min -= 2;
		*max += 2;

		if (*min < 0)
			*min = 0;
		if (*max >= ntiles)
			*max = ntiles - 1;
	}
}

static void tile_iter_set_z(TileIter *iter, int z)
{
	int ntiles;

	iter->z = z;
	if (z < 0) {
		iter->min_x = iter->max_x = -1;
		iter->min_y = iter->max_y = -1;

		return;
	}

	ntiles = 1 << z;

	iter->min_x = (int) (iter->left * ntiles);
	iter->max_x = (int) (iter->right * ntiles) + 1;
	tile_iter_range_fix(iter, 1);

	iter->min_y = (int) (iter->top * ntiles);
	iter->max_y = (int) (iter->bottom * ntiles) + 1;
	tile_iter_range_fix(iter, 0);
}

void tile_iter_reset(TileIter *iter)
{
	tile_iter_set_z(iter, iter->min_z - 1);

	iter->cur = -1;

	iter->x = iter->max_x;
	iter->y = iter->max_y;
}

int tile_iter_next(TileIter *iter)
{
	int newlevel = 0;

	if (iter->z > iter->max_z)
		return 0;

	iter->y += 1;
	if (iter->y > iter->max_y) {
		iter->y = iter->min_y;
		iter->x += 1;
	}

	if (iter->x > iter->max_x) {
		iter->z += 1;
		newlevel = 1;
	}

	if (iter->z > iter->max_z)
		return 0;

	if (newlevel) {
		tile_iter_set_z(iter, iter->z);

		iter->x = iter->min_x;
		iter->y = iter->min_y;
	}

	iter->cur++;

	return 1;
}

const char *tile_iter_url(TileIter *iter)
{
	if (iter->z < iter->min_z || iter->z > iter->max_z)
		return NULL;

	switch (iter->rule)
	{
	case TILE_ITER_RULE_NORMAL:
	default:
		snprintf(iter->url, sizeof(iter->url),
				iter->url_format,
				iter->z, iter->x, iter->y);
		break;
	case TILE_ITER_RULE_REVERSE_Z:
		snprintf(iter->url, sizeof(iter->url),
				iter->url_format,
				iter->rule_data - iter->z, iter->x, iter->y);
		break;
	}


	return iter->url;
}

int tile_iter_cur(TileIter *iter)
{
	return iter->cur;
}

int tile_iter_count(TileIter *iter)
{
	if (!iter->count) {
		int z, old_z = iter->z;
		int count = 0;

		for (z = iter->min_z; z <= iter->max_z; z++) {
			tile_iter_set_z(iter, z);

			count += (iter->max_x - iter->min_x + 1) *
				(iter->max_y - iter->min_y + 1);
		}

		tile_iter_set_z(iter, old_z);

		iter->count = count;
	}

	return iter->count;
}
