/* om-locations-map-pack.c -
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
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include <Ecore.h>
#include <Ecore_Evas.h>

#include <Evas.h>
#include <Eet.h>

#include <Ecore_File.h>
#include "tileiter.h"

Evas *evas;
char *basedir;
int verbose;

#define MAP_FORMAT 1

typedef struct _E_Nav_Map_Desc E_Nav_Map_Desc;

struct _E_Nav_Map_Desc {
	unsigned char format;
	int version;
	char *source;
	int min_level, max_level;
	double lon, lat;
	double width, height;
};

void get_key(const char *path, char *buf)
{
	char *p;

	strcpy(buf, path + strlen(basedir) + 1);

	p = strrchr(buf, '.');
	if (p)
		*p = '\0';
}

static void _eet_pack(Eet_File *ef, const char *path)
{
	Evas_Object *im;
	void *im_data;
	int  im_w, im_h;
	int  im_alpha;
	char key[PATH_MAX];

	get_key(path, key);
	if (verbose)
		printf("packing %s\n", key);

	im = evas_object_image_add(evas);
	if (!im)
		goto fail;

	evas_object_image_file_set(im, path, NULL);
	if (evas_object_image_load_error_get(im) != EVAS_LOAD_ERROR_NONE)
		goto fail;

	evas_object_image_size_get(im, &im_w, &im_h);
	im_alpha = evas_object_image_alpha_get(im);
	im_data = evas_object_image_data_get(im, 0);

	if ((im_data) && (im_w > 0) && (im_h > 0)) {
		int bytes, qual = 80;

		bytes = eet_data_image_write(ef, key,
				im_data, im_w, im_h,
				im_alpha,
				0, qual, 1);

		if (bytes <= 0)
			goto fail;
	}

	evas_object_del(im);

	return;

fail:
	if (im)
		evas_object_del(im);
	printf("failed to pack %s\n", key);

	return;
}

static void _eet_traverse(Eet_File *ef, const char *base)
{
	DIR *dir;
	struct dirent *d;
	struct stat s;
	char path[PATH_MAX];

	dir = opendir(base);
	if (!dir) {
		printf("failed to open %s: %s\n", base, strerror(errno));

		return;
	}


	while ((d = readdir(dir))) {
		if (d->d_name[0] == '.')
			continue;

		snprintf(path, sizeof(path), "%s/%s", base, d->d_name);

		if (stat(path, &s) < 0) {
			printf("failed to stat() %s\n", path);

			continue;
		}

		if (S_ISDIR(s.st_mode))
			_eet_traverse(ef, path);
		else if (S_ISREG(s.st_mode))
			_eet_pack(ef, path);
	}

	closedir(dir);
}

static Eet_Data_Descriptor *
map_describe(void)
{
   Eet_Data_Descriptor *edd;

   edd = eet_data_descriptor_new(
	 "E_Nav_Map_Desc", sizeof(E_Nav_Map_Desc),
	 (void *) evas_list_next,
	 (void *) evas_list_append,
	 (void *) evas_list_data,
	 (void *) evas_list_free,
	 (void *) evas_hash_foreach,
	 (void *) evas_hash_add,
	 (void *) evas_hash_free);

   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Nav_Map_Desc,
	 "format", format, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Nav_Map_Desc,
	 "version", version, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Nav_Map_Desc,
	 "source", source, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Nav_Map_Desc,
	 "min_level", min_level, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Nav_Map_Desc,
	 "max_level", max_level, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Nav_Map_Desc,
	 "lon", lon, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Nav_Map_Desc,
	 "lat", lat, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Nav_Map_Desc,
	 "width", width, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, E_Nav_Map_Desc,
	 "height", height, EET_T_DOUBLE);

   return edd;
}

static int _eet_describe(Eet_File *ef, E_Nav_Map_Desc *md)
{
	Eet_Data_Descriptor *edd;
	void *data;
	int s;
	int ret;

	edd = map_describe();

	//ret = eet_data_write(ef, edd, "description", md, 0);
	data = eet_data_descriptor_encode(edd, md, &s);
	ret = eet_write(ef, "description", data, s, 0);
	free(data);

	eet_data_descriptor_free(edd);

	return ret;
}

static int parse_desc(const char *desc, E_Nav_Map_Desc *md)
{
	const char *p;

	p = desc;

	md->format = MAP_FORMAT;

	md->version = strtol(p, (char **) &p, 10);
	if (!p || *p != ',')
		return 0;
	p++;

	md->source = (char *) p;
	p = strchr(p, ',');
	if (!p || *p != ',')
		return 0;
	{
		char *src;

		src = malloc(p - md->source + 1);
		memcpy(src, md->source, p - md->source);
		src[p - md->source] = '\0';

		md->source = src;
	}

	p++;

	md->min_level = strtol(p, (char **) &p, 10);
	if (!p || *p != ',')
		return 0;
	p++;

	md->max_level = strtol(p, (char **) &p, 10);
	if (!p || *p != ',')
		return 0;
	p++;

	md->lon = strtod(p, (char **) &p);
	if (!p || *p != ',')
		return 0;
	p++;

	md->lat = strtod(p, (char **) &p);
	if (!p || *p != ',')
		return 0;
	p++;

	md->width = strtod(p, (char **) &p);
	if (!p || *p != ',')
		return 0;
	p++;

	md->height = strtod(p, NULL);

	if (verbose) {
		printf("description:\n"
		       "\tformat %d\n"
		       "\tversion %d\n"
		       "\tsource %s\n"
		       "\tmin_level %d\n"
		       "\tmax_level %d\n"
		       "\tlon %f\n"
		       "\tlat %f\n"
		       "\twidth %f\n"
		       "\theight %f\n",
		       md->format,
		       md->version,
		       md->source,
		       md->min_level,
		       md->max_level,
		       md->lon,
		       md->lat,
		       md->width,
		       md->height);
	}

	if (md->min_level < 0 || md->max_level < md->min_level)
		return 0;
	if (md->width <= 0.0 || md->height <= 0.0)
		return 0;
	if (md->lon < -180.0 || md->lon + md->width > 360.0 ||
	    md->lat < -90.0  || md->lat + md->height > 90.0)
		return 0;

	return 1;
}

static int fetch_sched(TileIter *iter);

static void fetch_completion(void *data, const char *file, int status)
{
	TileIter *iter = data;

	if (status) {
		printf("failed to fetch %s\n", file);
		ecore_main_loop_quit();

		return;
	}

	if (verbose && file)
		printf("saved to %s\n", file);

	if (tile_iter_next(iter)) {
		if (!fetch_sched(iter)) {
			printf("failed to schedule fetch\n");
			ecore_main_loop_quit();
		}
	} else {
		ecore_main_loop_quit();
	}
}

static int
fetch_skip_idler(void *data)
{
	fetch_completion(data, NULL, 0);

	return 0;
}

static int fetch_sched(TileIter *iter)
{
	const char *url;
	char dst[PATH_MAX];
	int skip = 0;

	snprintf(dst, sizeof(dst), "%s/%s/%d/%d",
			(const char *) iter->data, "osm",
			iter->z, iter->x);

	if (!ecore_file_is_dir(dst) && !ecore_file_mkpath(dst))
		return 0;

	snprintf(dst, sizeof(dst), "%s/%s/%d/%d/%d.png",
			(const char *) iter->data, "osm",
			iter->z, iter->x, iter->y);

	if (ecore_file_exists(dst)) {
		if (ecore_file_size(dst) > 0)
			skip = 1;
		else
			ecore_file_unlink(dst);
	}

	url = tile_iter_url(iter);
	printf("%s [%d/%d]: %s\n",
			(skip) ? "skip" : "fetch",
			tile_iter_cur(iter) + 1,
			tile_iter_count(iter), url);

	if (skip) {
		ecore_idler_add(fetch_skip_idler, iter);

		return 1;
	}

	return ecore_file_download(url, dst,
			fetch_completion, NULL, iter);
}

static int fetch_tiles(E_Nav_Map_Desc *md, const char *dst)
{
	TileIter *iter;
	int success = 1;

	ecore_file_init();
	iter = tile_iter_new(TILE_ITER_FORMAT_OSM,
			md->lon, md->lat, md->width, md->height,
			md->min_level, md->max_level);
	iter->data = dst;

	if (tile_iter_next(iter)) {
		if (fetch_sched(iter))
			ecore_main_loop_begin();
		else
			printf("failed to schedule fetch\n");
	}

	if (tile_iter_cur(iter) + 1 != tile_iter_count(iter)) {
		printf("fetch failed: %d/%d\n",
				tile_iter_cur(iter) + 1,
				tile_iter_count(iter));
		success = 0;
	}

	tile_iter_destroy(iter);
	ecore_file_shutdown();

	return success;
}

void _eet_merge(Eet_File *ef, Eet_File *base)
{
	char **keys;
	int num_keys, i;

	keys = eet_list(base, "*", &num_keys);

	for (i = 0; i < num_keys; i++) {
		char *k = keys[i];
		const void *data;
		int size;

		data = eet_read_direct(base, k, &size);
		if (!data) {
			printf("failed to merge %s\n", k);

			continue;
		}
		eet_write(ef, k, data, size, 0);
	}

	free(keys);
}

void usage(const char *prog)
{
	printf("%s [-b base] [-d version,source,min_level,max_level,lon,lat,width,height] [-k] [-v] <cache-dir> [<output>]\n", prog);
}

int main(int argc, char **argv)
{
	Ecore_Evas *ee;
	E_Nav_Map_Desc md;
	Eet_File *ef = NULL;
	char *map = NULL, *basemap = NULL, *desc = NULL;
	int skip_fetch = 0;
	int opt;

	while ((opt = getopt(argc, argv, "b:d:kv")) != -1) {
		switch (opt) {
		case 'b':
			basemap = optarg;
			break;
		case 'd':
			desc = optarg;
			break;
		case 'k':
			skip_fetch = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage(argv[0]);
			return 1;
			break;
		}
	}

	if (optind < argc)
		basedir = argv[optind++];
	if (optind < argc)
		map = argv[optind++];

	if (!basedir || optind != argc) {
		usage(argv[0]);

		return 1;
	}

	if (!ecore_init() || !ecore_evas_init()) {
		printf("failed to init\n");

		return 1;
	}

	ee = ecore_evas_buffer_new(1, 1);
	if (!ee) {
		printf("failed to open ecore evas\n");

		return 1;
	}
	evas = ecore_evas_get(ee);

	if (map) {
		ef = eet_open(map, EET_FILE_MODE_WRITE);
		if (!ef) {
			printf("failed to open eet\n");

			return 1;
		}
	}

	if (desc) {
		if (!parse_desc(desc, &md)) {
			printf("failed to parse description\n");

			return 1;
		}

		if (!skip_fetch && !fetch_tiles(&md, basedir)) {
			printf("failed to fetch tiles\n");

			return 1;
		}
	}

	if (ef) {
		if (basemap) {
			Eet_File *bef;

			bef = eet_open(basemap, EET_FILE_MODE_READ);
			if (!bef) {
				printf("failed to open base eet\n");

				return 1;
			}

			_eet_merge(ef, bef);

			eet_close(bef);
		}

		if (desc && !_eet_describe(ef, &md)) {
			printf("failed to describe eet\n");

			return 1;
		}

		_eet_traverse(ef, basedir);

		eet_close(ef);
	}

	ecore_evas_free(ee);
	ecore_evas_shutdown();
	ecore_shutdown();

	return 0;
}
