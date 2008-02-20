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

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Nav_Tile_Job E_Nav_Tile_Job;

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
   char *map;
   char *suffix;
   int size;
   int min_level, max_level;

   double lon, lat;
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
   sd->map = strdup("tah");
   sd->suffix = strdup("png");
   sd->size = 256;
   sd->min_level = 0;
   sd->max_level = 17;

   e_nav_world_tileset_add(nav, obj);

   e_nav_tileset_center_set(obj, 0.0, 0.0);
   e_nav_tileset_level_set(obj, sd->min_level);

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

void
e_nav_tileset_level_set(Evas_Object *obj, int level)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (level > sd->max_level)
     level = sd->max_level;
   else if (level < sd->min_level)
     level = sd->min_level;

   if (sd->level != level)
     {
	sd->level = level;
	proxy_level_set(obj);
     }
   sd->span = sd->size << level;
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

void
e_nav_tileset_span_set(Evas_Object *obj, int span)
{
   E_Smart_Data *sd;
   int level;
   
   SMART_CHECK(obj, ;);

   if (span < 0)
     span = 0;

   level = (int) ((log((double) span / sd->size) / M_LOG2) + 0.5);

   sd->span = span;

   if (level > sd->max_level)
     level = sd->max_level;
   else if (level < sd->min_level)
     level = sd->min_level;

   if (sd->level != level)
     {
	sd->level = level;
	proxy_level_set(obj);
     }
}

int
e_nav_tileset_span_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0;);

   return sd->span;
}

void
e_nav_tileset_center_set(Evas_Object *obj, double lon, double lat)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   while (lon > 180.0) lon -= 360.0;
   while (lon < -180.0) lon += 360.0;
   if (lat > 90.0) lon = 90.0;
   if (lat < -90.0) lon = -90.0;

   sd->lon = lon;
   sd->lat = lat;
}

void
e_nav_tileset_center_get(Evas_Object *obj, double *lon, double *lat)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (lon)
      *lon = sd->lon;
   if (lat)
      *lat = sd->lat;
}

void
e_nav_tileset_to_offsets(Evas_Object *obj, double lon, double lat, double *x, double *y)
{
   E_Smart_Data *sd;
   double ox, oy;

   SMART_CHECK(obj, ;);

   mercator_project(sd->lon, sd->lat, &ox, &oy);
   mercator_project(lon, lat, x, y);

   if (x)
     *x = (*x - ox) * sd->span;

   if (y)
     *y = (*y - oy) * sd->span;
}

void
e_nav_tileset_from_offsets(Evas_Object *obj, double x, double y, double *lon, double *lat)
{
   E_Smart_Data *sd;
   double ox, oy;
   int span;

   SMART_CHECK(obj, ;);

   mercator_project(sd->lon, sd->lat, &ox, &oy);

   span = (sd->span) ? sd->span : 1;
   ox += x / span;
   oy += y / span;

   if (ox < 0.0)
     ox = 0.0;
   else if (ox > 1.0)
     ox = 1.0;

   mercator_project_inv(ox, oy, lon, lat);

   if (lon)
     *lon -= sd->lon;
   if (lat)
     *lat -= sd->lat;
}

void
e_nav_tileset_scale_set(Evas_Object *obj, double scale)
{
   E_Smart_Data *sd;
   double perimeter;
   
   SMART_CHECK(obj, ;);

   perimeter = cos(RADIANS(sd->lat)) * M_EARTH_RADIUS * M_PI * 2;

   /* perimeter <= 2^26; limit scale to avoid overflow */
   if (scale < 0.0001)
     scale = 0.0001;

   e_nav_tileset_span_set(obj, perimeter / scale);
}

double
e_nav_tileset_scale_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   double perimeter;
   
   SMART_CHECK(obj, 0.0;);

   perimeter = cos(RADIANS(sd->lat)) * M_EARTH_RADIUS * M_PI * 2;

   return perimeter / sd->span;
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
}

E_DBus_Proxy *
e_nav_tileset_proxy_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);

   return sd->proxy;
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

   evas_object_smart_data_set(obj, sd);
}

static void
_e_nav_tileset_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   if (sd->dir) free(sd->dir);
   if (sd->map) free(sd->map);
   if (sd->suffix) free(sd->suffix);

   evas_object_del(sd->underlay);
   evas_object_del(sd->clip);

   if (sd->proxy)
     e_dbus_proxy_disconnect_signal(sd->proxy, "TileCompleted",
	   (E_DBus_Signal_Cb) _e_nav_tileset_tile_completed_cb, obj);
   ecore_hash_destroy(sd->jobs);

   _e_nav_tileset_free(obj);

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

   printf("submit job for tile (%d,%d)@%d\n", job->x, job->y, sd->level);

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

   printf("job %u submitted\n", id);

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

   printf("job %u cancelled\n", job->id);

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

static void
job_load(E_Nav_Tile_Job *job)
{
   E_Smart_Data *sd;
   char buf[PATH_MAX];
   int err;

   sd = evas_object_smart_data_get(job->nt);
   if (!sd) return;

   snprintf(buf, sizeof(buf), "%s/%s/%d/%d/%d.%s",
	 sd->dir, sd->map,
	 job->level, job->x, job->y, sd->suffix);

   evas_object_image_file_set(job->obj, buf, NULL);
   err = evas_object_image_load_error_get(job->obj);

   if (err == EVAS_LOAD_ERROR_NONE)
     evas_object_show(job->obj);
   else
     {
	printf("%p load %s failed\n", job->obj, buf);
	evas_object_hide(job->obj);

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

   printf("job %u completed with status %d\n", id, status);
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

   tilesize = sd->span / num_tiles;
   if (tilesize < 1.0 || tilesize / sd->size < 0.1)
     return MODE_FAIL;

   tiles_ow = 1 + (((double)sd->w + tilesize) / tilesize);
   tiles_oh = 1 + (((double)sd->h + tilesize) / tilesize);

   if (!_e_nav_tileset_realloc(obj, tiles_ow * tiles_oh))
     return MODE_FAIL;

   mode = MODE_RELOAD;

   mercator_project(sd->lon, sd->lat, &tpx, &tpy);

   /* starting from left-top corner */
   tpx = (tpx * num_tiles) - (sd->w / 2.0) / tilesize;
   tpy = (tpy * num_tiles) - (sd->h / 2.0) / tilesize;

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
