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

Evas *evas;
Eet_File *ef;
char *basedir;

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

void pack(const char *path)
{
	Evas_Object *im;
	void *im_data;
	int  im_w, im_h;
	int  im_alpha;
	char key[PATH_MAX];

	get_key(path, key);

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

void traverse(const char *base)
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
			traverse(path);
		else if (S_ISREG(s.st_mode))
			pack(path);
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

int describe(const char *desc)
{
	Eet_Data_Descriptor *edd;
	E_Nav_Map_Desc md;
	const char *p;
	int ret;

	p = desc;

	md.format = MAP_FORMAT;

	md.version = strtol(p, (char **) &p, 10);
	if (!p || *p != ',')
		return 0;
	p++;

	md.source = (char *) p;
	p = strchr(p, ',');
	if (!p || *p != ',')
		return 0;
	{
		char *src;

		src = malloc(p - md.source + 1);
		memcpy(src, md.source, p - md.source);
		src[p - md.source] = '\0';

		md.source = src;
	}

	p++;

	md.min_level = strtol(p, (char **) &p, 10);
	if (!p || *p != ',')
		return 0;
	p++;

	md.max_level = strtol(p, (char **) &p, 10);
	if (!p || *p != ',')
		return 0;
	p++;

	md.lon = strtod(p, (char **) &p);
	if (!p || *p != ',')
		return 0;
	p++;

	md.lat = strtod(p, (char **) &p);
	if (!p || *p != ',')
		return 0;
	p++;

	md.width = strtod(p, (char **) &p);
	if (!p || *p != ',')
		return 0;
	p++;

	md.height = strtod(p, NULL);

	if (md.width <= 0.0 || md.height <= 0.0)
		return 0;

	edd = map_describe();

	if (1) {
		void *data;
		int s;

		data = eet_data_descriptor_encode(edd, &md, &s);
		ret = eet_write(ef, "description", data, s, 0);
		free(data);
	} else {
		ret = eet_data_write(ef, edd, "description", &md, 0);
	}

	eet_data_descriptor_free(edd);

	return ret;
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
	printf("%s [-b base] [-d version,source,min_level,max_level,lon,lat,width,height] <cache-dir> <output>\n", prog);
}

int main(int argc, char **argv)
{
	Ecore_Evas *ee;
	char *map, *basemap = NULL, *desc = NULL;
	int opt;

	while ((opt = getopt(argc, argv, "b:d:")) != -1) {
		switch (opt) {
		case 'b':
			basemap = optarg;
			break;
		case 'd':
			desc = optarg;
			break;
		default:
			usage(argv[0]);
			return 1;
			break;
		}
	}

	if (optind + 2 != argc) {
		usage(argv[0]);

		return 1;
	}

	basedir = argv[optind];
	map = argv[optind + 1];

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

	ef = eet_open(map, EET_FILE_MODE_WRITE);
	if (!ef) {
		printf("failed to open eet\n");

		return 1;
	}

	if (desc && !describe(desc)) {
		printf("failed to describe eet\n");

		return 1;
	}

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

	traverse(basedir);

	eet_close(ef);

	ecore_evas_free(ee);
	ecore_evas_shutdown();
	ecore_shutdown();

	return 0;
}
