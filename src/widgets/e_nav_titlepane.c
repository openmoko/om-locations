/* e_nav_tilepane.c -
 *
 * Copyright 2008 OpenMoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
 *
 * This work is based on e17 project.  See also COPYING.e17.
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
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "e_nav_titlepane.h"
#include "../e_nav_theme.h"

typedef struct _E_Smart_Data E_Smart_Data;

struct _E_Smart_Data
{
   Evas_Object *clip;
   Evas_Object *overlay;
   Evas_Object *obj;
   Evas_Object *src_obj;
   Evas_Coord x, y, w, h;
   const char      *dir;
};

static void _e_nav_titlepane_smart_init(void);
static void _e_nav_titlepane_smart_add(Evas_Object *obj);
static void _e_nav_titlepane_smart_del(Evas_Object *obj);
static void _e_nav_titlepane_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_nav_titlepane_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_nav_titlepane_smart_show(Evas_Object *obj);
static void _e_nav_titlepane_smart_hide(Evas_Object *obj);
static void _e_nav_titlepane_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_nav_titlepane_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_nav_titlepane_smart_clip_unset(Evas_Object *obj);

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_nav_titlepane")) return ret

Evas_Object *
e_nav_titlepane_add(Evas *e)
{
   _e_nav_titlepane_smart_init();
   return evas_object_smart_add(e, _e_smart);
}

void
e_nav_titlepane_set_message(Evas_Object *obj, const char *message)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   edje_object_part_text_set(sd->overlay, "titlepane.message", message);
}

void
e_nav_titlepane_set_left_button(Evas_Object *obj, const char *text, CallbackFunc cb, Evas_Object *data)  
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   edje_object_part_text_set(sd->overlay, "button.label.left", text);
   Evas_Object* o = edje_object_part_object_get(sd->overlay, "button.base.left");
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, cb, data);
}

void
e_nav_titlepane_set_right_button(Evas_Object *obj, const char *text, CallbackFunc cb, Evas_Object *data)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   edje_object_part_text_set(sd->overlay, "button.label.right", text);
   Evas_Object* o = edje_object_part_object_get(sd->overlay, "button.base.right");
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, cb, data);
}

void
e_nav_titlepane_hide_buttons(Evas_Object *obj)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   Evas_Object *left_button = edje_object_part_object_get(sd->overlay, "button.base.left");
   Evas_Object *right_button = edje_object_part_object_get(sd->overlay, "button.base.right");
   evas_object_hide(left_button);
   evas_object_hide(right_button);
}

void
e_nav_titlepane_theme_source_set(Evas_Object *obj, const char *custom_dir)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   
   sd->dir = custom_dir;
   sd->overlay = e_nav_theme_object_new(evas_object_evas_get(obj), sd->dir,
				      "modules/diversity_nav/titlepane");
   evas_object_smart_member_add(sd->overlay, obj);
   evas_object_move(sd->overlay, sd->x, sd->y);
   evas_object_resize(sd->overlay, sd->w, sd->h);
   evas_object_clip_set(sd->overlay, sd->clip);

   evas_object_show(sd->overlay);
}

void
e_nav_titlepane_source_object_set(Evas_Object *obj, void *src_obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   sd->src_obj = src_obj;
}

Evas_Object *
e_nav_titlepane_source_object_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
   return sd->src_obj;
}

/* internal calls */
static void
_e_nav_titlepane_smart_init(void)
{
   if (_e_smart) return;

   {
      static const Evas_Smart_Class sc =
      {
	 "e_nav_titlepane",
	 EVAS_SMART_CLASS_VERSION,
	 _e_nav_titlepane_smart_add,
	 _e_nav_titlepane_smart_del,
	 _e_nav_titlepane_smart_move,
	 _e_nav_titlepane_smart_resize,
	 _e_nav_titlepane_smart_show,
	 _e_nav_titlepane_smart_hide,
	 _e_nav_titlepane_smart_color_set,
	 _e_nav_titlepane_smart_clip_set,
	 _e_nav_titlepane_smart_clip_unset,

	 NULL /* data */
      };
      _e_smart = evas_smart_class_new(&sc);
   }
}

static void
_e_nav_titlepane_smart_add(Evas_Object *obj)
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
   evas_object_smart_data_set(obj, sd);
}

static void
_e_nav_titlepane_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_del(sd->clip);
   evas_object_del(sd->overlay);
   free(sd);
}

static void
_e_nav_titlepane_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;
   evas_object_move(sd->clip, sd->x, sd->y);
}

static void
_e_nav_titlepane_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->clip, sd->w, sd->h);
}

static void
_e_nav_titlepane_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_nav_titlepane_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_nav_titlepane_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_nav_titlepane_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_nav_titlepane_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}

