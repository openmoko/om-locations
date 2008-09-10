/* tileman.c -
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

#include <e_dbus_proxy.h>
#include <assert.h>
#include "tileman.h"
#include "e_nav_theme.h"

typedef struct _E_Smart_Data E_Smart_Data;

struct _Tileman {
     Evas *e;

     Tileman_Format format;
     char *src;
     char *suffix;
     int tilesize;
     int min_level, max_level;

     char *dir;

     E_DBus_Proxy *proxy;
     int proxy_level;
     Ecore_Hash *jobs;
};

struct _E_Smart_Data
{
   Evas_Object *obj;
   Evas_Coord x, y, w, h;

   Evas_Object *clip;

   Evas_Object *img;
   Evas_Object *fallback;

   Tileman *tman;

   int tz, tx, ty;

   /* these members are used by job_ */
   E_DBus_Proxy_Call *pending;
   unsigned int id;
};

static void _tileman_tile_smart_init(void);
static void _tileman_tile_smart_add(Evas_Object *obj);
static void _tileman_tile_smart_del(Evas_Object *obj);
static void _tileman_tile_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _tileman_tile_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _tileman_tile_smart_show(Evas_Object *obj);
static void _tileman_tile_smart_hide(Evas_Object *obj);
static void _tileman_tile_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _tileman_tile_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _tileman_tile_smart_clip_unset(Evas_Object *obj);

static void on_job_completed(Tileman *tman, DBusMessage *message);
static int job_busy(Tileman *tman, E_Smart_Data *sd);
static int job_submit(Tileman *tman, E_Smart_Data *sd, int force);
static void job_cancel(Tileman *tman, E_Smart_Data *sd);

#define SMART_NAME "tileman_tile"
static Evas_Smart *_e_smart = NULL;

#define TILE_VALID_NUM(lv, num) ((num) >= 0 && (num) < (1 << lv))
#define TILE_VALID(lv, x, y) (TILE_VALID_NUM(lv, x) && TILE_VALID_NUM(lv, y))

Tileman *
tileman_new(Evas *e, Tileman_Format format, const char *dir)
{
   Tileman *tman;

   if (!e || !dir)
     return NULL;

   tman = calloc(1, sizeof(*tman));
   if (!tman)
     return NULL;

   tman->e = e;

   tman->format = format;
   tman->src = strdup("osm");
   tman->suffix = strdup("png");
   tman->tilesize = 256;
   tman->min_level = 0;
   tman->max_level = 17;

   tman->dir = strdup(dir);

   tman->jobs = ecore_hash_new(ecore_direct_hash, ecore_direct_compare);

   _tileman_tile_smart_init();

   return tman;
}

void
tileman_destroy(Tileman *tman)
{
   tileman_proxy_set(tman, NULL);

   ecore_hash_destroy(tman->jobs);

   if (tman->dir)
     free(tman->dir);

   if (tman->suffix)
     free(tman->suffix);

   if (tman->src)
     free(tman->src);

   free(tman);
}

void
tileman_levels_list(Tileman *tman, int *max_level, int *min_level)
{
   if (max_level)
     *max_level = tman->max_level;

   if (min_level)
     *min_level = tman->min_level;
}

int
tileman_tile_size_get(Tileman *tman)
{
   return tman->tilesize;
}

void
tileman_proxy_set(Tileman *tman, E_DBus_Proxy *proxy)
{
   if (tman->proxy == proxy)
     return;

   /* TODO cancel jobs */
   if (tman->proxy)
     e_dbus_proxy_disconnect_signal(tman->proxy, "TileCompleted",
	   (E_DBus_Signal_Cb) on_job_completed, tman);

   tman->proxy = proxy;
   if (tman->proxy)
     {
	e_dbus_proxy_connect_signal(tman->proxy, "TileCompleted",
	      (E_DBus_Signal_Cb) on_job_completed, tman);

	e_dbus_proxy_simple_call(tman->proxy, "SetPath",
	      NULL,
	      DBUS_TYPE_STRING, &tman->dir,
	      DBUS_TYPE_INVALID,
	      DBUS_TYPE_INVALID);

	e_dbus_proxy_simple_call(tman->proxy, "SetSource",
	      NULL,
	      DBUS_TYPE_STRING, &tman->src,
	      DBUS_TYPE_INVALID,
	      DBUS_TYPE_INVALID);

	e_dbus_proxy_simple_call(tman->proxy, "SetLevel",
	      NULL,
	      DBUS_TYPE_INT32, &tman->proxy_level,
	      DBUS_TYPE_INVALID,
	      DBUS_TYPE_INVALID);
     }
}

E_DBus_Proxy *
tileman_proxy_get(Tileman *tman)
{
   return tman->proxy;
}

void
tileman_proxy_level_set(Tileman *tman, int level)
{
   if (tman->proxy_level == level)
     return;

   tman->proxy_level = level;
   if (tman->proxy)
     e_dbus_proxy_simple_call(tman->proxy, "SetLevel",
	   NULL,
	   DBUS_TYPE_INT32, &tman->proxy_level,
	   DBUS_TYPE_INVALID,
	   DBUS_TYPE_INVALID);
}

int
tileman_proxy_level_get(Tileman *tman)
{
   return tman->proxy_level;
}

Evas_Object *tileman_tile_add(Tileman *tman)
{
   Evas_Object *tile;
   E_Smart_Data *sd;

   tile = evas_object_smart_add(tman->e, _e_smart);
   sd = evas_object_smart_data_get(tile);

   sd->fallback = e_nav_theme_object_new(tman->e, NULL,
	 "modules/diversity_nav/tile");
   evas_object_smart_member_add(sd->fallback, sd->obj);
   evas_object_clip_set(sd->fallback, sd->clip);
   evas_object_move(sd->fallback, sd->x, sd->y);
   evas_object_resize(sd->fallback, sd->w, sd->h);
   evas_object_stack_below(sd->fallback, sd->img);
   evas_object_show(sd->fallback);

   sd->tman = tman;

   return tile;
}

static void
tileman_tile_fail(Evas_Object *tile, const char *status)
{
   E_Smart_Data *sd;

   SMART_CHECK(tile, ;);

   evas_object_hide(sd->img);
   if (sd->fallback)
     edje_object_part_text_set(sd->fallback, "status", status);
}

static void
tileman_tile_reset(Evas_Object *tile, int z, int x, int y)
{
   E_Smart_Data *sd;

   SMART_CHECK(tile, ;);

   if (job_busy(sd->tman, sd))
     job_cancel(sd->tman, sd);

   sd->tz = z;
   sd->tx = x;
   sd->ty = y;

   if (sd->fallback)
     edje_object_part_text_set(sd->fallback, "status", NULL);
}

int
tileman_tile_load(Evas_Object *tile, int z, int x, int y)
{
   E_Smart_Data *sd;
   int err = 0;

   SMART_CHECK(tile, 0;);

   if (sd->tz == z && sd->tx == x && sd->ty == y)
     return 1;

   tileman_tile_reset(tile, z, x, y);

   if (TILE_VALID(z, x, y))
     {
	char buf[PATH_MAX];

	snprintf(buf, sizeof(buf), "%s/%s/%d/%d/%d.%s",
	      sd->tman->dir, sd->tman->src,
	      sd->tz, sd->tx, sd->ty, sd->tman->suffix);

	evas_object_image_file_set(sd->img, buf, NULL);
	err = evas_object_image_load_error_get(sd->img);
	if (!err)
	  evas_object_show(sd->img);
	else
	  tileman_tile_fail(tile, _("failed to load"));
     }
   else
     {
	tileman_tile_fail(tile, _("another world"));
	err = 0;
     }

   return (!err);
}

int
tileman_tile_image_set(Evas_Object *tile, const char *path, const char *key)
{
   E_Smart_Data *sd;
   const char *old_path, *old_key;
   int err;

   SMART_CHECK(tile, 0;);

   if (job_busy(sd->tman, sd))
     evas_object_image_file_get(sd->img, &old_path, &old_key);

   evas_object_image_file_set(sd->img, path, key);
   err = evas_object_image_load_error_get(sd->img);
   if (err)
     {
	if (job_busy(sd->tman, sd))
	  evas_object_image_file_set(sd->img, old_path, old_key);
     }
   else
     {
	if (job_busy(sd->tman, sd))
	  job_cancel(sd->tman, sd);

	evas_object_show(sd->img);
     }

   return (!err);
}

int
tileman_tile_download(Evas_Object *tile, int z, int x, int y)
{
   E_Smart_Data *sd;
   int ret = 0;

   SMART_CHECK(tile, 0;);

   if (sd->tz == z && sd->tx == x && sd->ty == y)
     {
	if (job_busy(sd->tman, sd))
	  return 1;
     }
   else
     {
	tileman_tile_reset(tile, z, x, y);
     }

   if (TILE_VALID(z, x, y) && sd->tman->proxy_level == z)
     ret = job_submit(sd->tman, sd, 1);

   if (ret)
     tileman_tile_fail(tile, _("downloading..."));
   else
     tileman_tile_fail(tile, _("failed to submit"));

   return ret;
}

void
tileman_tile_cancel(Evas_Object *tile)
{
   E_Smart_Data *sd;

   SMART_CHECK(tile, ;);

   if (job_busy(sd->tman, sd))
     {
	job_cancel(sd->tman, sd);
	tileman_tile_fail(tile, _("cancelled"));
     }
}

static void
on_job_completed(Tileman *tman, DBusMessage *message)
{
   Evas_Object *tile;
   E_Smart_Data *sd;
   unsigned int id;
   int status;

   if (!message)
     return;

   if (!tman->proxy)
     return;

   if (!dbus_message_get_args(message, NULL,
			      DBUS_TYPE_UINT32, &id,
			      DBUS_TYPE_INT32, &status,
			      DBUS_TYPE_INVALID))
     return;

   tile = ecore_hash_remove(tman->jobs, (void *) id);
   if (!tile)
     return;

   SMART_CHECK(tile, ;);

   //printf("job %u completed with status %d\n", id, status);
   if (status == 0)
     {
	evas_object_image_reload(sd->img);
	status = evas_object_image_load_error_get(sd->img);
     }

   if (status == 0)
     evas_object_show(sd->img);
   else
     tileman_tile_fail(tile, _("failed to download"));

   sd->id = 0;
}

static void
on_submitted(void *user_data, E_DBus_Proxy *proxy, E_DBus_Proxy_Call *call_id)
{
   E_Smart_Data *sd = user_data;
   DBusMessage *reply;
   unsigned int id;

   assert(sd->pending == call_id);
   assert(!sd->id);

   sd->pending = NULL;

   e_dbus_proxy_end_call(proxy, call_id, &reply);
   if (reply && dbus_message_get_args(reply, NULL,
	    DBUS_TYPE_UINT32, &id,
	    DBUS_TYPE_INVALID))
     sd->id = id;

   if (id)
     ecore_hash_set(sd->tman->jobs, (void *) id, sd->obj);
   else
     tileman_tile_fail(sd->obj, _("failed to queue"));
}

static int
job_busy(Tileman *tman, E_Smart_Data *sd)
{
   return (sd->id || sd->pending);
}

static int
job_submit(Tileman *tman, E_Smart_Data *sd, int force)
{
   DBusMessage *message;

   //printf("submit job for tile (%d,%d)@%d\n", job->x, job->y, sd->level);

   if (!tman->proxy)
     return 0;

   if (sd->id || sd->pending)
     return 1;

   message = e_dbus_proxy_new_method_call(tman->proxy, "SubmitTile");
   if (message)
     {
	if (!dbus_message_append_args(message,
		 DBUS_TYPE_INT32, &sd->tx,
		 DBUS_TYPE_INT32, &sd->ty,
		 DBUS_TYPE_BOOLEAN, &force,
		 DBUS_TYPE_INVALID))
	  {
	     dbus_message_unref(message);
	     message = NULL;
	  }
     }

   if (message)
     {
	sd->pending = e_dbus_proxy_begin_call(tman->proxy, message,
	      on_submitted, sd, NULL);
	dbus_message_unref(message);
     }

   return (sd->pending != NULL);
}

static void
job_cancel(Tileman *tman, E_Smart_Data *sd)
{
   if (sd->pending && tman->proxy)
     {
	e_dbus_proxy_cancel_call(tman->proxy, sd->pending);
	sd->pending = NULL;

	assert(!sd->id);

	return;
     }

   if (!sd->id)
     return;

   if (tman->proxy)
     {
	DBusMessage *message;

	message = e_dbus_proxy_new_method_call(tman->proxy, "CancelTile");
	if (message)
	  {
	     if (!dbus_message_append_args(message,
		      DBUS_TYPE_UINT32, &sd->id,
		      DBUS_TYPE_INVALID))
	       {
		  dbus_message_unref(message);
		  message = NULL;
	       }
	  }

	if (message)
	  {
	     e_dbus_proxy_call_no_reply(tman->proxy, message);
	     dbus_message_unref(message);
	  }
     }

   //printf("job %u cancelled\n", sd->id);

   ecore_hash_remove(tman->jobs, (void *) sd->id);
   sd->id = 0;
}

/* internal calls */
static void
_tileman_tile_smart_init(void)
{
   if (!_e_smart)
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
	     EVAS_SMART_CLASS_VERSION,
	     _tileman_tile_smart_add,
	     _tileman_tile_smart_del,
	     _tileman_tile_smart_move,
	     _tileman_tile_smart_resize,
	     _tileman_tile_smart_show,
	     _tileman_tile_smart_hide,
	     _tileman_tile_smart_color_set,
	     _tileman_tile_smart_clip_set,
	     _tileman_tile_smart_clip_unset,

	     NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_tileman_tile_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd)
     return;

   sd->obj = obj;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_resize(sd->clip, sd->w, sd->h);

   sd->img = evas_object_image_add(evas_object_evas_get(sd->obj));
   evas_object_image_smooth_scale_set(sd->img, 0);
   evas_object_smart_member_add(sd->img, sd->obj);
   evas_object_clip_set(sd->img, sd->clip);
   evas_object_move(sd->img, sd->x, sd->y);
   evas_object_resize(sd->img, sd->w, sd->h);
   evas_object_show(sd->img);

   evas_object_smart_data_set(obj, sd);
}

static void
_tileman_tile_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   tileman_tile_cancel(obj);

   if (sd->img)
     evas_object_del(sd->img);

   if (sd->fallback)
     evas_object_del(sd->fallback);

   evas_object_del(sd->clip);

   free(sd);
}

static void
_tileman_tile_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   sd->x = x;
   sd->y = y;

   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_move(sd->img, sd->x, sd->y);
   if (sd->fallback)
     evas_object_move(sd->fallback, sd->x, sd->y);
}

static void
_tileman_tile_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   sd->w = w;
   sd->h = h;

   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_resize(sd->img, sd->w, sd->h);
   evas_object_image_fill_set(sd->img, 0, 0, sd->w, sd->h);
   if (sd->fallback)
     evas_object_resize(sd->fallback, sd->w, sd->h);
}

static void
_tileman_tile_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_show(sd->clip);
}

static void
_tileman_tile_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_hide(sd->clip);
}

static void
_tileman_tile_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_tileman_tile_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_clip_set(sd->clip, clip);
}

static void
_tileman_tile_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_clip_unset(sd->clip);
} 
