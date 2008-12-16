/* e_nav_tileset.c -
 *
 * Copyright 2008 Openmoko, Inc.
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

#include "e_nav.h"
#include "e_nav_tileset.h"
#include "tileman.h"
#include <e_dbus_proxy.h>
#include <math.h>

#include <Ecore_File.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <Eet.h>

#define RADIANS(d) ((d) * M_PI / 180.0)
#define DEGREES(d) ((d) * 180.0 / M_PI)
#define M_LOG2		(0.6931471805)

typedef struct _E_Smart_Data E_Smart_Data;

typedef struct _E_Nav_Map E_Nav_Map;
typedef struct _E_Nav_Map_Desc E_Nav_Map_Desc;

struct _E_Nav_Map_Desc
{
   unsigned char format;
   int version;
   char *source;
   int min_level, max_level;
   double lon, lat;
   double width, height;
};

struct _E_Nav_Map
{
   char *path;
   int min_x[18];
   int max_x[18];
   int min_y[18];
   int max_y[18];
   E_Nav_Map_Desc *desc;
};

struct _E_Smart_Data
{
   Evas_Object *obj;
   Evas_Object *clip;
   Evas_Coord x, y, w, h;

   Eina_List *maps;
   Ecore_Hash *mons;

   Tileman *tman;
   int tilesize;

   int min_level, max_level;
   int min_span, max_span;
   double abs_lon, abs_lat;

   double px, py;
   int span;
   int level;

   struct {
      double tilesize;
      Evas_Coord offset_x, offset_y;
      int olevel, ospan;
      int ox, oy, ow, oh;
      Evas_Object **jobs;
      int num_jobs;
   } tiles;
};

static void _e_nav_tileset_smart_init(void);
static void _e_nav_tileset_smart_add(Evas_Object *obj);
static void _e_nav_tileset_smart_del(Evas_Object *obj);
static void _e_nav_tileset_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_nav_tileset_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_nav_tileset_smart_show(Evas_Object *obj);
static void _e_nav_tileset_smart_hide(Evas_Object *obj);
static void _e_nav_tileset_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_nav_tileset_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_nav_tileset_smart_clip_unset(Evas_Object *obj);

inline static void mercator_project(double lon, double lat, double *x, double *y);
inline static void mercator_project_inv(double x, double y, double *lon, double *lat);

static int _e_nav_tileset_prepare(Evas_Object *obj);
static void _e_nav_tileset_rearrange(Evas_Object *obj, int x, int y, int w, int h);
static int _e_nav_tileset_realloc(Evas_Object *obj, int num_jobs);
static void _e_nav_tileset_free(Evas_Object *obj);
static Evas_Object *_e_nav_tileset_tile_get(Evas_Object *obj, int i, int j);
static void _e_nav_tileset_update(Evas_Object *obj);

#define SMART_NAME "e_nav_tileset"
static Evas_Smart *_e_smart = NULL;

Evas_Object *
e_nav_tileset_add(Evas *e, E_Nav_Tileset_Format format, const char *dir)
{
   Evas_Object *obj;
   E_Smart_Data *sd;

   _e_nav_tileset_smart_init();
   obj = evas_object_smart_add(e, _e_smart);
   if (!obj) return NULL;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     {
	evas_object_del(obj);
	return NULL;
     }

   sd->tman = tileman_new(e, format, dir);
   if (!sd->tman)
     {
	evas_object_del(obj);

	return NULL;
     }

   sd->tilesize = tileman_tile_size_get(sd->tman);
   tileman_levels_list(sd->tman, &sd->max_level, &sd->min_level);

   sd->min_span = (1 << sd->min_level) * sd->tilesize;
   sd->max_span = (1 << sd->max_level) * sd->tilesize * 2;
   sd->abs_lon = 180.0;
   mercator_project_inv(0.0, 0.0, NULL, &sd->abs_lat);

   e_nav_tileset_span_set(obj, sd->min_span);
   e_nav_tileset_pos_set(obj, 0.5, 0.5);

   return obj;
}

void
e_nav_tileset_update(Evas_Object *obj)
{
   _e_nav_tileset_update(obj);
}

void
e_nav_tileset_pos_set(Evas_Object *obj, double px, double py)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->px = px;
   sd->py = py;
}

void
e_nav_tileset_pos_get(Evas_Object *obj, double *px, double *py)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (px)
     *px = sd->px;

   if (py)
     *py = sd->py;
}

static void
proxy_level_set(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   tileman_proxy_level_set(sd->tman, sd->level);
}

void
e_nav_tileset_span_set(Evas_Object *obj, int span)
{
   E_Smart_Data *sd;
   int level;

   SMART_CHECK(obj, ;);

   if (span < sd->min_span)
     span = sd->min_span;
   else if (span > sd->max_span)
     span = sd->max_span;

   if (sd->span == span)
     return;

   sd->span = span;

   level = e_nav_tileset_level_from_span(obj, span);

   if (sd->level != level)
     {
	sd->level = level;
	proxy_level_set(obj);
     }

   return;
}

int
e_nav_tileset_span_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, 0;);

   return sd->span;
}

void
e_nav_tileset_span_range(Evas_Object *obj, int *min_span, int *max_span)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (min_span)
     *min_span = sd->min_span;

   if (max_span)
     *max_span = sd->max_span;
}

void
e_nav_tileset_coord_to_pos(Evas_Object *obj, double lon, double lat, double *px, double *py)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (lon < -sd->abs_lon)
     lon = -sd->abs_lon;
   else if (lon > sd->abs_lon)
     lon = sd->abs_lon;

   if (lat < -sd->abs_lat)
     lat = -sd->abs_lat;
   else if (lat > sd->abs_lat)
     lat = sd->abs_lat;

   mercator_project(lon, lat, px, py);

   return;
}

void
e_nav_tileset_coord_from_pos(Evas_Object *obj, double px, double py, double *lon, double *lat)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (px < 0.0)
     px = 0.0;
   else if (px > 1.0)
     px = 1.0;

   if (py < 0.0)
     py = 0.0;
   else if (py > 1.0)
     py = 1.0;

   mercator_project_inv(px, py, lon, lat);

   return;
}

void
e_nav_tileset_coord_range(Evas_Object *obj, double *abs_lon, double *abs_lat)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (abs_lon)
     *abs_lon = sd->abs_lon;

   if (abs_lat)
     *abs_lat = sd->abs_lat;
}

int
e_nav_tileset_level_to_span(Evas_Object *obj, int level)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (level < sd->min_level)
     level = sd->min_level;
   else if (level > sd->max_level)
     level = sd->max_level;

   return (1 << level) * sd->tilesize;
}

int
e_nav_tileset_level_from_span(Evas_Object *obj, int span)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, 0;);

   if (span < sd->min_span)
     span = sd->min_span;
   else if (span > sd->max_span)
     span = sd->max_span;

   return (int) (log((double) span / sd->tilesize) / M_LOG2);
}

void
e_nav_tileset_level_range(Evas_Object *obj, int *min_level, int *max_level)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (min_level)
     *min_level = sd->min_level;

   if (max_level)
     *max_level = sd->max_level;
}

void
e_nav_tileset_proxy_set(Evas_Object *obj, E_DBus_Proxy *proxy)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   tileman_proxy_set(sd->tman, proxy);
}

E_DBus_Proxy *
e_nav_tileset_proxy_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);

   return tileman_proxy_get(sd->tman);
}

static Eet_Data_Descriptor *
map_describe(void)
{
   Eet_Data_Descriptor *edd;

   edd = eet_data_descriptor_new(
	 "E_Nav_Map_Desc", sizeof(E_Nav_Map_Desc),
	 (void *) eina_list_next,
	 (void *) eina_list_append,
	 (void *) eina_list_data_get,
	 (void *) eina_list_free,
	 (void *) eina_hash_foreach,
	 (void *) eina_hash_add,
	 (void *) eina_hash_free);

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

static int
map_read(E_Nav_Map *map)
{
   Eet_File *ef;
   Eet_Data_Descriptor *edd;

   ef = eet_open(map->path, EET_FILE_MODE_READ);
   if (!ef)
     return 0;

   edd = map_describe();
   if (!edd)
     {
	eet_close(ef);

	return 1;
     }

   map->desc = eet_data_read(ef, edd, "description");
   if (map->desc)
     {
	if (map->desc->lon < -180.0)
	  map->desc->lon = -180.0;
	else if (map->desc->lon > 180.0)
	  map->desc->lon = 180.0;

	if (map->desc->lat < -90.0)
	  map->desc->lat = -90.0;
	else if (map->desc->lat > 90.0)
	  map->desc->lat = 90.0;

	if (map->desc->width < 0.0)
	  map->desc->width = 0.0;
	else if (map->desc->lon + map->desc->width > 180.0)
	  map->desc->width = 180.0 - map->desc->lon;

	if (map->desc->height < 0.0)
	  map->desc->height = 0.0;
	else if (map->desc->lat + map->desc->height > 90.0)
	  map->desc->height = 90.0 - map->desc->lat;

	if (map->desc->min_level < 0)
	  map->desc->min_level = 0;
	if (map->desc->max_level < 0)
	  map->desc->max_level = 0;
     }

   eet_data_descriptor_free(edd);
   eet_close(ef);

   return 1;
}

static E_Nav_Map *
map_new(const char *path)
{
   E_Nav_Map *map;
   int z;

   map = malloc(sizeof(*map));
   if (!map)
     return NULL;

   map->path = strdup(path);
   map->desc = NULL;

   if (!map_read(map))
     {
	free(map->path);
	free(map);

	return NULL;
     }

   if (!map->desc)
     {
	map->desc = malloc(sizeof(*map->desc));
	if (!map->desc)
	  {
	     free(map);

	     return NULL;
	  }

	map->desc->format = 0;
	map->desc->version = 0;
	map->desc->source = strdup("osm");
	map->desc->min_level = 0;
	map->desc->max_level = 20;
	map->desc->lon = -180.0;
	map->desc->lat = -90.0;
	map->desc->width = 360.0;
	map->desc->height = 180.0;
     }

   printf("%s: %d %d %s %d %d %f %f %f %f\n",
	 map->path,
	 map->desc->format,
	 map->desc->version,
	 map->desc->source,
	 map->desc->min_level,
	 map->desc->max_level,
	 map->desc->lon,
	 map->desc->lat,
	 map->desc->width,
	 map->desc->height);

   for (z = 0; z < 18; z++)
     {
	double x, y;

	if (z < map->desc->min_level || z > map->desc->max_level)
	  {
	     map->min_x[z] = 1 << z;
	     map->max_x[z] = 0;
	     map->min_y[z] = 1 << z;
	     map->max_y[z] = 0;

	     continue;
	  }

	mercator_project(map->desc->lon, map->desc->lat, &x, &y);
	x *= 1 << z;
	y *= 1 << z;

	map->min_x[z] = (int) x;
	map->max_y[z] = (int) y;

	mercator_project(map->desc->lon + map->desc->width,
	      map->desc->lat + map->desc->height, &x, &y);
	x *= 1 << z;
	y *= 1 << z;

	map->max_x[z] = (int) x;
	map->min_y[z] = (int) y;

	if (map->max_x[z] - map->min_x[z] < 5)
	  {
	     map->min_x[z] -= 2;
	     map->max_x[z] += 2;
	  }

	if (map->max_y[z] - map->min_y[z] < 5)
	  {
	     map->min_y[z] -= 2;
	     map->max_y[z] += 2;
	  }

	//printf("%d: %d %d %d %d\n", z, map->min_x[z], map->max_x[z],
	//      map->min_y[z], map->max_y[z]);
     }

   return map;
}

static void
map_free(E_Nav_Map *map)
{
   if (map->desc->source)
     free(map->desc->source);

   free(map->desc);
   free(map->path);
   free(map);
}

static int
map_get_key(E_Nav_Map *map, char *buf, int size, int z, int x, int y)
{
   if (z < map->desc->min_level ||
       z > map->desc->max_level)
     return 0;

   if (x < map->min_x[z] || x > map->max_x[z] ||
       y < map->min_y[z] || y > map->max_y[z])
     return 0;

   snprintf(buf, size, "%s/%d/%d/%d",
	 map->desc->source, z, x, y);

   return 1;
}

static void
on_path_changed(void *obj, Ecore_File_Monitor *ecore_file_monitor, Ecore_File_Event event, const char *path)
{
   E_Smart_Data *sd;
   struct stat st;
   const char *p;
   Eina_List *l;

   SMART_CHECK(obj, ;);

   switch (event)
     {
      case ECORE_FILE_EVENT_CREATED_FILE:
      case ECORE_FILE_EVENT_DELETED_FILE:
	 p = strrchr(path, '/');
	 if (!p || p[1] == '.')
	   return;

	 p = strrchr(p, '.');
	 if (!p || strcmp(p + 1, "eet") != 0)
	   return;
	 break;
      default:
	 return;
	 break;
     }

   for (l = sd->maps; l; l = l->next)
     {
	E_Nav_Map *map = l->data;

	if (strcmp(map->path, path) == 0)
	  break;
     }

   if (event == ECORE_FILE_EVENT_CREATED_FILE)
     {
	if (l)
	  return;

	if (access(path, R_OK) != 0)
	  return;

	stat(path, &st);
	if (S_ISREG(st.st_mode))
	  {
	     E_Nav_Map *map;
	    
	     //printf("add %s\n", path);
	     map = map_new(path);

	     if (map)
	       sd->maps = eina_list_prepend(sd->maps, map);
	  }
     }
   else
     {
	if (l)
	  {
	     //printf("del %s\n", (char *) map->path);

	     map_free(l->data);
	     sd->maps = eina_list_remove_list(sd->maps, l);
	  }
     }
}

void
e_nav_tileset_monitor_add(Evas_Object *obj, const char *dn)
{
   E_Smart_Data *sd;
   Ecore_File_Monitor *mon;
   DIR *dir;
   struct dirent *d;
   char path[PATH_MAX + 1], *p;
   int len;

   SMART_CHECK(obj, ;);

   if (ecore_hash_get(sd->mons, dn))
     return;

   len = strlen(dn);
   if (len + 6 > PATH_MAX)
     return;

   dir = opendir(dn);
   if (!dir) return;

   strcpy(path, dn);
   p = path + len;
   *p++ = '/';

   while ((d = readdir(dir)))
     {
	if (len + 1 + strlen(d->d_name) > PATH_MAX)
	  continue;

	strcpy(p, d->d_name);
	on_path_changed(obj, NULL, ECORE_FILE_EVENT_CREATED_FILE, path);
     }

   closedir(dir);

   mon = ecore_file_monitor_add(dn, on_path_changed, obj);
   ecore_hash_set(sd->mons, strdup(dn), mon);
}

void
e_nav_tileset_monitor_del(Evas_Object *obj, const char *dn)
{
   E_Smart_Data *sd;
   Ecore_File_Monitor *mon;
   Eina_List *l;

   SMART_CHECK(obj, ;);

   mon = ecore_hash_remove(sd->mons, dn);
   if (!mon)
     return;

   ecore_file_monitor_del(mon);

   //printf("del mon %s\n", dn);

   l = sd->maps;
   for (l = sd->maps; l; l = l->next)
     {
	E_Nav_Map *map = l->data;
	int len = strlen(dn);

	if (strncmp(map->path, dn, len) == 0 && map->path[len] == '/' &&
	    !strchr(map->path + len + 1, '/'))
	  {
	     printf("del %s\n", map->path);
	     map_free(map);

	     l = l->prev;
	     sd->maps = eina_list_remove_list(sd->maps, l->next);
	  }
     }
}

/* internal calls */
static void
_e_nav_tileset_smart_init(void)
{
   if (_e_smart) return;

   {
      static const Evas_Smart_Class sc =
      {
	 SMART_NAME,
	 EVAS_SMART_CLASS_VERSION,
	 _e_nav_tileset_smart_add,
	 _e_nav_tileset_smart_del,
	 _e_nav_tileset_smart_move,
	 _e_nav_tileset_smart_resize,
	 _e_nav_tileset_smart_show,
	 _e_nav_tileset_smart_hide,
	 _e_nav_tileset_smart_color_set,
	 _e_nav_tileset_smart_clip_set,
	 _e_nav_tileset_smart_clip_unset,

	 NULL /* data */
      };
      _e_smart = evas_smart_class_new(&sc);
   }
}

static void
_e_nav_tileset_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   sd->obj = obj;
   
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);

   ecore_file_init();
   sd->mons = ecore_hash_new(ecore_str_hash, ecore_str_compare);
   ecore_hash_free_key_cb_set(sd->mons, free);
   ecore_hash_free_value_cb_set(sd->mons,
	 (Ecore_Free_Cb) ecore_file_monitor_del);

   evas_object_smart_data_set(obj, sd);
}

static void
_e_nav_tileset_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   ecore_hash_destroy(sd->mons);
   ecore_file_shutdown();

   while (sd->maps)
     {
	map_free(sd->maps->data);

	sd->maps = eina_list_remove_list(sd->maps, sd->maps);
     }

   evas_object_del(sd->clip);

   _e_nav_tileset_free(obj);

   tileman_destroy(sd->tman);

   free(sd);
}

static void
_e_nav_tileset_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;
   evas_object_move(sd->clip, sd->x, sd->y);
   _e_nav_tileset_update(obj);
}

static void
_e_nav_tileset_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->clip, sd->w, sd->h);
   _e_nav_tileset_update(obj);
}

static void
_e_nav_tileset_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_nav_tileset_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_nav_tileset_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_nav_tileset_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_nav_tileset_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
} 

inline static void
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

inline static void
mercator_project_inv(double x, double y, double *lon, double *lat)
{
   if (lon)
     *lon = -180.0 + x * 360.0;

   if (lat)
      *lat = DEGREES(atan(sinh((1.0 - y * 2.0) * M_PI)));
}

static Evas_Object *
_e_nav_tileset_tile_add(Evas_Object *obj, int i, int j)
{
   E_Smart_Data *sd;
   Evas_Object *tile;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;

   tile = tileman_tile_add(sd->tman);
   evas_object_smart_member_add(tile, sd->obj);
   evas_object_clip_set(tile, sd->clip);
   evas_object_pass_events_set(tile, 1);
   evas_object_stack_below(tile, sd->clip);

   sd->tiles.jobs[(j * sd->tiles.ow) + i] = tile;

   return tile;
}
 
static Evas_Object *
_e_nav_tileset_tile_get(Evas_Object *obj, int i, int j)
{
   E_Smart_Data *sd;
   Evas_Object *tile;
   int x, y;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;

   tile = sd->tiles.jobs[(j * sd->tiles.ow) + i];
   if (!tile)
     tile = _e_nav_tileset_tile_add(obj, i, j);

   x = i + sd->tiles.ox;
   y = j + sd->tiles.oy;

   if (!tileman_tile_load(tile, sd->level, x, y))
     {
	Eina_List *l;
	int err = 1;

	for (l = sd->maps; l; l = l->next)
	  {
	     E_Nav_Map *map = l->data;
	     char key[64];

	     if (map_get_key(map, key, sizeof(key), sd->level, x, y) &&
		 tileman_tile_image_set(tile, map->path, key))
	       {
		  err = EVAS_LOAD_ERROR_NONE;

		  break;
	       }
	  }
   
	if (err != EVAS_LOAD_ERROR_NONE)
	  tileman_tile_download(tile);
     }

   return tile;
}

enum {
   MODE_FAIL,
   MODE_NONE,
   MODE_RELOAD,
   MODE_RESIZE,
   MODE_MOVE
};

static void
_e_nav_tileset_rearrange(Evas_Object *obj, int x, int y, int w, int h)
{
   E_Smart_Data *sd;
   int i, j;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   if (sd->tiles.ox >= x + w ||
       sd->tiles.oy >= y + h ||
       sd->tiles.ox + sd->tiles.ow <= x ||
       sd->tiles.oy + sd->tiles.oh <= y)
     return;

   for (j = y; j < y + h; j++)
     {
	Evas_Object **src, **dst;

	if (j < sd->tiles.oy ||
	    j >= sd->tiles.oy + sd->tiles.oh)
	  continue;

	src = sd->tiles.jobs + (j - sd->tiles.oy) * sd->tiles.ow;
	dst = sd->tiles.jobs + (j - y) * w;

	for (i = x; i < x + w; i++)
	  {
	     Evas_Object *job;

	     if (i < sd->tiles.ox ||
		 i >= sd->tiles.ox + sd->tiles.ow)
	       continue;

	     job = dst[i - x];
	     dst[i - x] = src[i - sd->tiles.ox];
	     src[i - sd->tiles.ox] = job;
	  }
     }
}

static int
_e_nav_tileset_realloc(Evas_Object *obj, int num_jobs)
{
   E_Smart_Data *sd;
   int i;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return 0;

   if (sd->tiles.num_jobs < num_jobs)
     {
	void *p;

	p = realloc(sd->tiles.jobs, num_jobs * sizeof(Evas_Object *));
	if (!p)
	  return 0;

	sd->tiles.jobs = p;

	num_jobs -= sd->tiles.num_jobs;
	memset(sd->tiles.jobs + sd->tiles.num_jobs, 0,
	      num_jobs * sizeof(Evas_Object *));

	sd->tiles.num_jobs += num_jobs;
     }
   else
     {
	for (i = num_jobs; i < sd->tiles.num_jobs; i++)
	  {
	     Evas_Object *job;

	     job = sd->tiles.jobs[i];
	     if (job)
	       evas_object_hide(job);
	  }
     }

   return 1;
}

static void _e_nav_tileset_free(Evas_Object *obj)
{
   E_Smart_Data *sd;
   int i;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->tiles.jobs) return;

   for (i = 0; i < sd->tiles.num_jobs; i++)
     {
	Evas_Object *job;

	job = sd->tiles.jobs[i];
	evas_object_del(job);
     }
   free(sd->tiles.jobs);

   sd->tiles.jobs = NULL;
   sd->tiles.ow = 0;
   sd->tiles.oh = 0;
}

static int
_e_nav_tileset_prepare(Evas_Object *obj)
{
   E_Smart_Data *sd;
   int tiles_ow, tiles_oh, tiles_ox, tiles_oy;
   int offset_x, offset_y;
   int num_tiles;
   double tilesize;
   double tpx, tpy;
   int mode = MODE_NONE;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return MODE_FAIL;

   num_tiles = (1 << sd->level);

   tilesize = (double) sd->span / num_tiles;
   if (tilesize < 1.0 || tilesize / sd->tilesize < 0.1)
     return MODE_FAIL;

   tiles_ow = 1 + (((double)sd->w + tilesize) / tilesize);
   tiles_oh = 1 + (((double)sd->h + tilesize) / tilesize);

   if (!_e_nav_tileset_realloc(obj, tiles_ow * tiles_oh))
     return MODE_FAIL;

   mode = MODE_RELOAD;

   tpx = ((sd->px * sd->span) - (sd->w / 2.0)) / tilesize;
   tpy = ((sd->py * sd->span) - (sd->h / 2.0)) / tilesize;

   tiles_ox = (int)tpx;
   tiles_oy = (int)tpy;

   offset_x = -tilesize * (tpx - tiles_ox);
   offset_y = -tilesize * (tpy - tiles_oy);

   if (sd->tiles.ox != tiles_ox || sd->tiles.oy != tiles_oy ||
       sd->tiles.ow != tiles_ow || sd->tiles.oh != tiles_oh)
     {
	_e_nav_tileset_rearrange(obj, tiles_ox, tiles_oy, tiles_ow, tiles_oh);
	mode = MODE_RELOAD;
     }
   else if (sd->level != sd->tiles.olevel)
     mode = MODE_RELOAD;
   else if (tilesize != sd->tiles.tilesize)
     mode = MODE_RESIZE;
   else if ((offset_x != sd->tiles.offset_x) || (offset_y != sd->tiles.offset_y))
     mode = MODE_MOVE;
     
   if (mode > MODE_NONE)
     {
	sd->tiles.ospan = sd->span;
	sd->tiles.olevel = sd->level;
	sd->tiles.tilesize = tilesize;
	sd->tiles.offset_x = offset_x;
	sd->tiles.offset_y = offset_y;

	sd->tiles.ox = tiles_ox;
	sd->tiles.oy = tiles_oy;
	sd->tiles.ow = tiles_ow;
	sd->tiles.oh = tiles_oh;
     }

   return mode;
}

static void
_e_nav_tileset_update(Evas_Object *obj)
{
   E_Smart_Data *sd;
   int mode, i, j;
   Evas_Coord x, y, xx, yy;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   mode = _e_nav_tileset_prepare(obj);
   if (mode == MODE_FAIL || mode == MODE_NONE)
     return;

   for (j = 0; j < sd->tiles.oh; j++)
     {
	y = (j * sd->tiles.tilesize);
	yy = ((j + 1) * sd->tiles.tilesize);

	for (i = 0; i < sd->tiles.ow; i++)
	  {
	     Evas_Object *o;
	    
	     o = _e_nav_tileset_tile_get(obj, i, j);

	     x = (i * sd->tiles.tilesize);
	     xx = ((i + 1) * sd->tiles.tilesize);
	     evas_object_move(o, 
			      sd->x + sd->tiles.offset_x + x,
			      sd->y + sd->tiles.offset_y + y);

	     if ((mode == MODE_RELOAD) || (mode == MODE_RESIZE))
	       evas_object_resize(o, xx - x, yy - y);

	     evas_object_show(o);
	  }
     }
}
