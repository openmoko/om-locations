/* tileiter.h -
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

#ifndef _TILEITER_H_
#define _TILEITER_H_

enum {
	TILE_ITER_FORMAT_OSM,
};

typedef struct _TileIter {
	/* private */
	int format;
	double lon, lat, width, height;
	int min_z, max_z;
	char url[1024];
	double left, right, top, bottom;
	int min_x, max_x;
	int min_y, max_y;
	int cur, count;

	/* public */
	int z, x, y;
	const void *data;
} TileIter;

TileIter *tile_iter_new(int format, double lon, double lat, double width, double height, int min_z, int max_z);
void tile_iter_destroy(TileIter *iter);
void tile_iter_reset(TileIter *iter);
int tile_iter_next(TileIter *iter);
const char *tile_iter_url(TileIter *iter);

int tile_iter_cur(TileIter *iter);
int tile_iter_count(TileIter *iter);

#endif /* _TILEITER_H_ */
