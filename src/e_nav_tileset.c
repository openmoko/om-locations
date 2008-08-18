/* e_nav_tileset.c -
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

#include "e_nav.h"
#include "e_nav_tileset.h"
#include <e_dbus_proxy.h>
#include <math.h>

#include <Ecore_File.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <Eet.h>

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Nav_Tile_Job E_Nav_Tile_Job;

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
   Evas_Object *underlay;
   Evas_Coord x, y, w, h;

   Evas_Object *nav;
   E_DBus_Proxy *proxy;

   Ecore_Hash *jobs;

   E_Nav_Tileset_Format format;
   char *dir;
   char *src;
   char *suffix;
   int size;
   int min_level, max_level;

   Evas_List *maps;
   Ecore_Hash *mons;

   int min_span, max_span;
   double max_lon, max_lat;

   double px, py;
   int span;
   int level;

   struct {
      double tilesize;
      Evas_Coord offset_x, offset_y;
      int olevel, ospan;
      int ox, oy, ow, oh;
      E_Nav_Tile_Job **jobs;
      int num_jobs;
   } tiles;
};

struct _E_Nav_Tile_Job
{
   Evas_Object *nt;
   Evas_Object *obj;
   int level;
   int x, y;
   unsigned int id;
   double timestamp;
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

static int job_submit(E_Nav_Tile_Job *job, int force);
static void job_cancel(E_Nav_Tile_Job *job);
static void job_reset(E_Nav_Tile_Job *job, int x, int y);
static void job_load(E_Nav_Tile_Job *job);

static void _e_nav_tileset_tile_completed_cb(Evas_Object *obj, DBusMessage *message);
static int _e_nav_tileset_prepare(Evas_Object *obj);
static void _e_nav_tileset_rearrange(Evas_Object *obj, int x, int y, int w, int h);
static int _e_nav_tileset_realloc(Evas_Object *obj, int num_jobs);
static void _e_nav_tileset_free(Evas_Object *obj);
static E_Nav_Tile_Job *_e_nav_tileset_tile_add(Evas_Object *obj, int i, int j);
static Evas_Object *_e_nav_tileset_tile_get(Evas_Object *obj, int i, int j);
static void _e_nav_tileset_update(Evas_Object *obj);

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_nav_tileset")) return ret

#define TILE_VALID_NUM(lv, num) ((num) >= 0 && (num) < (1 << lv))
#define TILE_VALID(lv, x, y) (TILE_VALID_NUM(lv, x) && TILE_VALID_NUM(lv, y))

Evas_Object *
e_nav_tileset_add(Evas_Object *nav, E_Nav_Tileset_Format format, const char *dir)
{
   Evas_Object *obj;
   E_Smart_Data *sd;

   _e_nav_tileset_smart_init();
   obj = evas_object_smart_add(evas_object_evas_get(nav), _e_smart);
   if (!obj) return NULL;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     {
	evas_object_del(obj);
	return NULL;
     }

   sd->nav = nav;

   sd->format = format;
   sd->dir = strdup(dir);
   sd->src = strdup("osm");
   sd->suffix = strdup("png");
   sd->size = 256;
   sd->min_level = 0;
   sd->max_level = 17;

   sd->min_span = (1 << sd->min_level) * sd->size;
   sd->max_span = (1 << sd->max_level) * sd->size * 2;
   sd->max_lon = 180.0;
   mercator_project_inv(0.0, 0.0, NULL, &sd->max_lat);

   e_nav_world_tileset_set(nav, obj);

   e_nav_tileset_level_set(obj, sd->min_level);
   e_nav_tileset_center_set(obj, 0.0, 0.0);

   return obj;
}

void
e_nav_tileset_update(Evas_Object *obj)
{
   _e_nav_tileset_update(obj);
}

static void
proxy_level_set(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd || !sd->proxy) return;

   e_dbus_proxy_simple_call(sd->proxy, "SetLevel",
			    NULL,
			    DBUS_TYPE_INT32, &sd->level,
			    DBUS_TYPE_INVALID,
			    DBUS_TYPE_INVALID);
}

int
e_nav_tileset_span_set(Evas_Object *obj, int span)
{
   E_Smart_Data *sd;
   int level;
   
   SMART_CHECK(obj, 0;);

   if (span < sd->min_span || span > sd->max_span)
     return 0;

   if (sd->span == span)
     return 1;

   sd->px *= (double) span / sd->span;
   sd->py *= (double) span / sd->span;
   sd->span = span;

   level = (int) (log((double) span / sd->size) / M_LOG2);

   if (level < sd->min_level)
     level = sd->min_level;
   else if (level > sd->max_level)
     level = sd->max_level;

   if (sd->level != level)
     {
	sd->level = level;
	proxy_level_set(obj);
     }

   return 1;
}

int
e_nav_tileset_span_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0;);

   return sd->span;
}

void
e_nav_tileset_pos_set(Evas_Object *obj, double px, double py, int scaled)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (!scaled)
     {
	px *= sd->span;
	py *= sd->span;
     }

   sd->px = px;
   sd->py = py;
}

void
e_nav_tileset_pos_get(Evas_Object *obj, double *px, double *py, int scaled)
{
   E_Smart_Data *sd;
   int scale = 1;
   
   SMART_CHECK(obj, ;);

   if (!scaled)
     scale = sd->span;

   if (px)
     *px = sd->px / scale;

   if (py)
     *py = sd->py / scale;
}

int
e_nav_tileset_to_pos(Evas_Object *obj, double lon, double lat, double *px, double *py, int scaled)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0;);

   if (lon < -sd->max_lon || lon > sd->max_lon ||
       lat < -sd->max_lat || lat > sd->max_lat)
     return 0;

   mercator_project(lon, lat, px, py);

   if (scaled)
     {
	if (px)
	  *px *= sd->span;

	if (py)
	  *py *= sd->span;
     }

   return 1;
}

int
e_nav_tileset_from_pos(Evas_Object *obj, double px, double py, double *lon, double *lat, int scaled)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0;);

   if (scaled)
     {
	px /= sd->span;
	py /= sd->span;
     }

   if (px < 0.0 || px > 1.0 || py < 0.0 || py > 1.0)
     return 0;

   mercator_project_inv(px, py, lon, lat);

   return 1;
}

int
e_nav_tileset_level_set(Evas_Object *obj, int level)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0;);

   if (level < sd->min_level || level > sd->max_level)
     return 0;

   e_nav_tileset_span_set(obj, (1 << level) * sd->size);

   return 1;
}

int
e_nav_tileset_level_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0.0;);

   return sd->level;
}

void
e_nav_tileset_levels_list(Evas_Object *obj, int *max_level, int *min_level)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (max_level)
     *max_level = sd->max_level;
   if (min_level)
     *min_level = sd->min_level;
}

int
e_nav_tileset_center_set(Evas_Object *obj, double lon, double lat)
{
   double px, py;
   
   if (!e_nav_tileset_to_pos(obj, lon, lat, &px, &py, 1))
     return 0;

   e_nav_tileset_pos_set(obj, px, py, 1);

   return 1;
}

int
e_nav_tileset_center_get(Evas_Object *obj, double *lon, double *lat)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0;);

   if (!e_nav_tileset_from_pos(obj, sd->px, sd->py, lon, lat, 1))
     return 0;

   return 1;
}

int
e_nav_tileset_to_offsets(Evas_Object *obj, double lon, double lat, double *x, double *y)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, 0;);

   if (!e_nav_tileset_to_pos(obj, lon, lat, x, y, 1))
     return 0;

   if (x)
     *x -= sd->px;

   if (y)
     *y -= sd->py;

   return 1;
}

int
e_nav_tileset_from_offsets(Evas_Object *obj, double x, double y, double *lon, double *lat)
{
   E_Smart_Data *sd;
   double olon, olat;

   SMART_CHECK(obj, 0;);

   if (!e_nav_tileset_from_pos(obj, sd->px + x, sd->py + y, lon, lat, 1))
     return 0;

   if (!e_nav_tileset_center_get(obj, &olon, &olat))
     return 0;

   if (lon)
     *lon -= olon;
   if (lat)
     *lat -= olat;

   return 1;
}

void
e_nav_tileset_proxy_set(Evas_Object *obj, E_DBus_Proxy *proxy)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (sd->proxy == proxy)
     return;

   if (sd->proxy)
     e_dbus_proxy_disconnect_signal(sd->proxy, "TileCompleted",
	   (E_DBus_Signal_Cb) _e_nav_tileset_tile_completed_cb, obj);

   sd->proxy = proxy;
   e_dbus_proxy_connect_signal(sd->proxy, "TileCompleted",
	 (E_DBus_Signal_Cb) _e_nav_tileset_tile_completed_cb, obj);

   e_dbus_proxy_simple_call(sd->proxy, "SetSource",
			    NULL,
			    DBUS_TYPE_STRING, &sd->src,
			    DBUS_TYPE_INVALID,
			    DBUS_TYPE_INVALID);
}

E_DBus_Proxy *
e_nav_tileset_proxy_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);

   return sd->proxy;
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
	map->desc->source = NULL;
	map->desc->min_level = 0;
	map->desc->max_level = 20;
	map->desc->lon = -180.0;
	map->desc->lat = -90.0;
	map->desc->width = 360.0;
	map->desc->height = 180.0;
     }

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

static void
on_path_changed(void *obj, Ecore_File_Monitor *ecore_file_monitor, Ecore_File_Event event, const char *path)
{
   E_Smart_Data *sd;
   struct stat st;
   const char *p;
   Evas_List *l;

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
	       sd->maps = evas_list_prepend(sd->maps, map);
	  }
     }
   else
     {
	if (l)
	  {
	     //printf("del %s\n", (char *) map->path);

	     map_free(l->data);
	     sd->maps = evas_list_remove_list(sd->maps, l);
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
   Evas_List *l;

   SMART_CHECK(obj, ;);

   if (!ecore_hash_remove(sd->mons, dn))
     return;

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
	     sd->maps = evas_list_remove_list(sd->maps, l->next);
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
	 "e_nav_tileset",
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

   sd->underlay = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->underlay, obj);
   evas_object_color_set(sd->underlay, 180, 180, 180, 255);
   evas_object_clip_set(sd->underlay, sd->clip);
   evas_object_lower(sd->underlay);
   evas_object_show(sd->underlay);

   sd->jobs = ecore_hash_new(ecore_direct_hash, ecore_direct_compare);

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

	sd->maps = evas_list_remove_list(sd->maps, sd->maps);
     }

   if (sd->dir) free(sd->dir);
   if (sd->src) free(sd->src);
   if (sd->suffix) free(sd->suffix);

   evas_object_del(sd->underlay);
   evas_object_del(sd->clip);

   if (sd->proxy)
     e_dbus_proxy_disconnect_signal(sd->proxy, "TileCompleted",
	   (E_DBus_Signal_Cb) _e_nav_tileset_tile_completed_cb, obj);

   _e_nav_tileset_free(obj);

   ecore_hash_destroy(sd->jobs);

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
   evas_object_move(sd->underlay, sd->x, sd->y);
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
   evas_object_resize(sd->underlay, sd->w, sd->h);
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

static int
job_submit(E_Nav_Tile_Job *job, int force)
{
   E_Smart_Data *sd;
   unsigned int id;

   sd = evas_object_smart_data_get(job->nt);
   if (!sd || !sd->proxy) return 0;

   //printf("submit job for tile (%d,%d)@%d\n", job->x, job->y, sd->level);

   if (job->id)
     return 1;

   if (!e_dbus_proxy_simple_call(sd->proxy, "SubmitTile",
				 NULL,
				 DBUS_TYPE_INT32, &job->x,
				 DBUS_TYPE_INT32, &job->y,
				 DBUS_TYPE_BOOLEAN, &force,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_UINT32, &id,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to get tile\n");

	return 0;
     }

   job->id = id;
   job->timestamp = ecore_time_get();

   ecore_hash_set(sd->jobs, (void *) id, job);

   return 1;
}

static void
job_cancel(E_Nav_Tile_Job *job)
{
   E_Smart_Data *sd;

   if (!job->id) return;

   sd = evas_object_smart_data_get(job->nt);
   if (!sd || !sd->proxy) return;

   e_dbus_proxy_simple_call(sd->proxy, "CancelTile",
			    NULL,
			    DBUS_TYPE_UINT32, &job->id,
			    DBUS_TYPE_INVALID,
			    DBUS_TYPE_INVALID);

   //printf("job %u cancelled\n", job->id);

   ecore_hash_remove(sd->jobs, (void *) job->id);
   job->id = 0;
}

static void
job_reset(E_Nav_Tile_Job *job, int x, int y)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(job->nt);
   if (!sd) return;

   if (job->id)
     job_cancel(job);

   job->level = sd->level;
   job->x = x;
   job->y = y;

   evas_object_image_file_set(job->obj, NULL, NULL);
   evas_object_hide(job->obj);
}

static int
job_load_from_map(E_Nav_Tile_Job *job, E_Nav_Map *map)
{
   E_Smart_Data *sd;
   int err;
   char key[64];

   sd = evas_object_smart_data_get(job->nt);
   if (!sd) return EVAS_LOAD_ERROR_DOES_NOT_EXIST;

   if (job->level < map->desc->min_level ||
       job->level > map->desc->max_level)
     return EVAS_LOAD_ERROR_DOES_NOT_EXIST;

   if (job->x < map->min_x[job->level] || job->x > map->max_x[job->level] ||
       job->y < map->min_y[job->level] || job->y > map->max_y[job->level])
     return EVAS_LOAD_ERROR_DOES_NOT_EXIST;

   snprintf(key, sizeof(key), "%s/%d/%d/%d",
	 sd->src, job->level, job->x, job->y);

   evas_object_image_file_set(job->obj, map->path, key);
   err = evas_object_image_load_error_get(job->obj);

   return err;
}

static void
job_load(E_Nav_Tile_Job *job)
{
   E_Smart_Data *sd;
   Evas_List *l;
   char buf[PATH_MAX];
   int err;

   sd = evas_object_smart_data_get(job->nt);
   if (!sd) return;

   snprintf(buf, sizeof(buf), "%s/%s/%d/%d/%d.%s",
	 sd->dir, sd->src,
	 job->level, job->x, job->y, sd->suffix);

   evas_object_image_file_set(job->obj, buf, NULL);
   err = evas_object_image_load_error_get(job->obj);

   if (err == EVAS_LOAD_ERROR_NONE)
     {
	evas_object_show(job->obj);

	return;
     }

   for (l = sd->maps; l; l = l->next)
     {
	err = job_load_from_map(job, l->data);
	if (err == EVAS_LOAD_ERROR_NONE)
	  break;
     }
   
   if (err == EVAS_LOAD_ERROR_NONE)
     {
	evas_object_show(job->obj);
     }
   else
     {
	evas_object_hide(job->obj);

	/* `buf' is reloaded when job is done */
	evas_object_image_file_set(job->obj, buf, NULL);

	job_submit(job, 1);
     }
}

static void
_e_nav_tileset_tile_completed_cb(Evas_Object *obj, DBusMessage *message)
{
   E_Smart_Data *sd;
   E_Nav_Tile_Job *job;
   unsigned int id;
   int status;

   if (!message)
     return;

   sd = evas_object_smart_data_get(obj);
   if (!sd || !sd->proxy) return;

   if (!dbus_message_get_args(message, NULL,
			      DBUS_TYPE_UINT32, &id,
			      DBUS_TYPE_INT32, &status,
			      DBUS_TYPE_INVALID))
     return;

   job = ecore_hash_get(sd->jobs, (void *) id);
   if (!job)
     return;

   //printf("job %u completed with status %d\n", id, status);
   if (status == 0 && job->obj)
     {
	evas_object_image_reload(job->obj);
	if (evas_object_image_load_error_get(job->obj) == EVAS_LOAD_ERROR_NONE)
	  evas_object_show(job->obj);
     }

   job->id = 0;
   ecore_hash_remove(sd->jobs, (void *) id);
}

static E_Nav_Tile_Job *
_e_nav_tileset_tile_add(Evas_Object *obj, int i, int j)
{
   E_Smart_Data *sd;
   E_Nav_Tile_Job *job;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;

   job = calloc(1, sizeof(E_Nav_Tile_Job));
   if (!job) return NULL;

   job->nt = obj;
   job->obj = evas_object_image_add(evas_object_evas_get(sd->obj));

   evas_object_smart_member_add(job->obj, sd->obj);
   evas_object_clip_set(job->obj, sd->clip);
   evas_object_pass_events_set(job->obj, 1);
   evas_object_stack_below(job->obj, sd->clip);
   evas_object_image_smooth_scale_set(job->obj, 0);

   sd->tiles.jobs[(j * sd->tiles.ow) + i] = job;

   job->level = sd->level;
   job->x = i + sd->tiles.ox;
   job->y = j + sd->tiles.oy;

   return job;
}
 
static Evas_Object *
_e_nav_tileset_tile_get(Evas_Object *obj, int i, int j)
{
   E_Smart_Data *sd;
   E_Nav_Tile_Job *job;
   int x, y;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;

   x = i + sd->tiles.ox;
   y = j + sd->tiles.oy;

   job = sd->tiles.jobs[(j * sd->tiles.ow) + i];
   if (job)
     {
	if (job->level == sd->level && job->x == x && job->y == y)
	  {
	     if (!job->id)
	       evas_object_show(job->obj);

	     return job->obj;
	  }

	job_reset(job, x, y);
     }
   else
     job = _e_nav_tileset_tile_add(obj, i, j);


   if (TILE_VALID(sd->level, x, y))
     job_load(job);

   return job->obj;
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
	E_Nav_Tile_Job **src, **dst;

	if (j < sd->tiles.oy ||
	    j >= sd->tiles.oy + sd->tiles.oh)
	  continue;

	src = sd->tiles.jobs + (j - sd->tiles.oy) * sd->tiles.ow;
	dst = sd->tiles.jobs + (j - y) * w;

	for (i = x; i < x + w; i++)
	  {
	     E_Nav_Tile_Job *job;

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

	p = realloc(sd->tiles.jobs, num_jobs * sizeof(E_Nav_Tile_Job *));
	if (!p)
	  return 0;

	sd->tiles.jobs = p;

	num_jobs -= sd->tiles.num_jobs;
	memset(sd->tiles.jobs + sd->tiles.num_jobs, 0,
	      num_jobs * sizeof(E_Nav_Tile_Job *));

	sd->tiles.num_jobs += num_jobs;
     }
   else
     {
	for (i = num_jobs; i < sd->tiles.num_jobs; i++)
	  {
	     E_Nav_Tile_Job *job;

	     job = sd->tiles.jobs[i];
	     if (job)
	       evas_object_hide(job->obj);
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
	E_Nav_Tile_Job *job;

	job = sd->tiles.jobs[i];
	if (job)
	  {
	     if (job->id)
	       job_cancel(job);
	     evas_object_del(job->obj);
	     free(job);
	  }
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
   if (tilesize < 1.0 || tilesize / sd->size < 0.1)
     return MODE_FAIL;

   tiles_ow = 1 + (((double)sd->w + tilesize) / tilesize);
   tiles_oh = 1 + (((double)sd->h + tilesize) / tilesize);

   if (!_e_nav_tileset_realloc(obj, tiles_ow * tiles_oh))
     return MODE_FAIL;

   mode = MODE_RELOAD;

   tpx = (sd->px - (sd->w / 2.0)) / tilesize;
   tpy = (sd->py - (sd->h / 2.0)) / tilesize;

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
	       {
		  evas_object_resize(o, xx - x, yy - y);
		  evas_object_image_fill_set(o, 0, 0, xx - x, yy - y);
	       }
	  }
     }
}
