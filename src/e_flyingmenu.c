/* e_flyingmenu.c -
 *
 * Copyright 2008 Openmoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
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
#include "e_nav_theme.h"
#include "e_flyingmenu.h"
#include "e_nav_misc.h"
#include "widgets/e_nav_button_bar.h"

typedef struct _E_Smart_Data E_Smart_Data;

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *src_obj;
   Evas_Object     *event;
   
   Evas_Object     *clip;

   E_Nav_Drop_Data *drop;
   
   Evas_Object     *bbar;
   Evas_Coord       button_min_size;

   unsigned char autodelete : 1;
};

static void _e_flyingmenu_smart_init(void);
static void _e_flyingmenu_smart_add(Evas_Object *obj);
static void _e_flyingmenu_smart_del(Evas_Object *obj);
static void _e_flyingmenu_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_flyingmenu_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_flyingmenu_smart_show(Evas_Object *obj);
static void _e_flyingmenu_smart_hide(Evas_Object *obj);
static void _e_flyingmenu_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_flyingmenu_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_flyingmenu_smart_clip_unset(Evas_Object *obj);

static void _e_flyingmenu_move_and_resize(Evas_Object *obj);
static void _e_flyingmenu_cb_src_obj_del(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_flyingmenu_cb_src_obj_move(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_flyingmenu_cb_src_obj_resize(void *data, Evas *evas, Evas_Object *obj, void *event);

#define SMART_NAME "e_flyingmenu"
static Evas_Smart *_e_smart = NULL;

Evas_Object *
e_flyingmenu_add(Evas *e)
{
   _e_flyingmenu_smart_init();
   return evas_object_smart_add(e, _e_smart);
}

void
e_flyingmenu_theme_source_set(Evas_Object *obj, const char *custom_dir)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   
   sd->bbar = e_nav_button_bar_add(evas_object_evas_get(obj));
   e_nav_button_bar_embed_set(sd->bbar, obj, "modules/diversity_nav/flying_menu");

   evas_object_smart_member_add(sd->bbar, obj);
   evas_object_clip_set(sd->bbar, sd->clip);
   evas_object_show(sd->bbar);
}

void
e_flyingmenu_source_object_set(Evas_Object *obj, Evas_Object *src_obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   if (sd->src_obj)
     {
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_DEL,
				       _e_flyingmenu_cb_src_obj_del);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_MOVE,
				       _e_flyingmenu_cb_src_obj_move);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_RESIZE,
				       _e_flyingmenu_cb_src_obj_resize);
     }
   sd->src_obj = src_obj;
   if (sd->src_obj)
     {
	evas_object_event_callback_add(sd->src_obj, EVAS_CALLBACK_DEL,
				       _e_flyingmenu_cb_src_obj_del, obj);
	evas_object_event_callback_add(sd->src_obj, EVAS_CALLBACK_MOVE,
				       _e_flyingmenu_cb_src_obj_move, obj);
	evas_object_event_callback_add(sd->src_obj, EVAS_CALLBACK_RESIZE,
				       _e_flyingmenu_cb_src_obj_resize, obj);
     }

   _e_flyingmenu_move_and_resize(sd->obj);
}

Evas_Object *
e_flyingmenu_source_object_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
   return sd->src_obj;
}
  
void
e_flyingmenu_autodelete_set(Evas_Object *obj, Evas_Bool autodelete)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   sd->autodelete = autodelete;
}

Evas_Bool
e_flyingmenu_autodelete_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0;);
   return sd->autodelete;
}

void
e_flyingmenu_item_size_min_set(Evas_Object *obj, Evas_Coord size)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   sd->button_min_size = size;
}

Evas_Coord
e_flyingmenu_item_size_min_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   return sd->button_min_size;
}

void
e_flyingmenu_item_add(Evas_Object *obj, const char *label, void (*func) (void *data, Evas_Object *obj), void *data)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   e_nav_button_bar_button_add(sd->bbar, label, func, data);

   if (e_nav_button_bar_num_buttons_get(sd->bbar) == 2)
     e_nav_button_bar_paddings_set(sd->bbar, 10, 1, 9);
   else
     e_nav_button_bar_paddings_set(sd->bbar, 10, 0, 10);
}

static void
_e_flyingmenu_move_and_resize(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_Coord x, y, w, h;
   Evas_Coord menu_w, menu_h, menu_gap;
   
   SMART_CHECK(obj, ;);

   if (sd->bbar)
     {
	e_nav_button_bar_button_size_request(sd->bbar,
	      sd->button_min_size, 0);

	menu_w = e_nav_button_bar_width_min_calc(sd->bbar);
	menu_h = e_nav_button_bar_height_min_calc(sd->bbar);
	menu_gap = 2;
     }
   else
     {
	menu_w = 0;
	menu_h = 0;
	menu_gap = 0;
     }

   if (sd->src_obj)
     {
	evas_object_geometry_get(sd->src_obj, &x, &y, &w, &h);

	x = x + (w - menu_w) / 2;
	y -= menu_h + menu_gap;
     }
   else
     {
	x = sd->x;
	y = sd->y;
     }

   if (sd->drop)
     {
	e_nav_drop_apply(sd->drop, sd->obj, x, y, menu_w, menu_h);
     }
   else
     {
	evas_object_move(sd->obj, x, y);
	evas_object_resize(sd->obj, menu_w, menu_h);
     }
}

static void
on_drop_done(void *data, Evas_Object *obj)
{
   E_Smart_Data *sd = data;

   if (sd->drop)
     {
	e_nav_drop_destroy(sd->drop);
	sd->drop = NULL;
     }
}

void
e_flyingmenu_activate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (sd->drop)
     return;

   evas_object_show(sd->event);
   sd->drop = e_nav_drop_new(1.0, on_drop_done, sd);
   _e_flyingmenu_move_and_resize(sd->obj);
}

/* internal calls */
static void
_e_flyingmenu_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
	       EVAS_SMART_CLASS_VERSION,
	       _e_flyingmenu_smart_add,
	       _e_flyingmenu_smart_del,
	       _e_flyingmenu_smart_move,
	       _e_flyingmenu_smart_resize,
	       _e_flyingmenu_smart_show,
	       _e_flyingmenu_smart_hide,
	       _e_flyingmenu_smart_color_set,
	       _e_flyingmenu_smart_clip_set,
	       _e_flyingmenu_smart_clip_unset,
	       
	       NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_flyingmenu_cb_event_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   evas_object_del(data);
}

static void
_e_flyingmenu_smart_add(Evas_Object *obj)
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
   evas_object_move(sd->clip, -10000, -10000);
   evas_object_resize(sd->clip, 30000, 30000);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_show(sd->clip);

   sd->event = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->event, obj);
   evas_object_move(sd->event, -10000, -10000);
   evas_object_resize(sd->event, 30000, 30000);
   evas_object_color_set(sd->event, 255, 255, 255, 0);
   evas_object_clip_set(sd->event, sd->clip);
   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_flyingmenu_cb_event_down, obj);

   evas_object_smart_data_set(obj, sd);
}

static void
_e_flyingmenu_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->src_obj)
     {
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_DEL,
				       _e_flyingmenu_cb_src_obj_del);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_MOVE,
				       _e_flyingmenu_cb_src_obj_move);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_RESIZE,
				       _e_flyingmenu_cb_src_obj_resize);
     }

   if (sd->drop)
     e_nav_drop_destroy(sd->drop);

   if (sd->bbar)
     evas_object_del(sd->bbar);

   evas_object_del(sd->clip);
   evas_object_del(sd->event);
   free(sd);
}
                    
static void
_e_flyingmenu_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;

   if (sd->bbar)
     evas_object_move(sd->bbar, sd->x, sd->y);
}

static void
_e_flyingmenu_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;

   if (sd->bbar)
     evas_object_resize(sd->bbar, sd->w, sd->h);
}

static void
_e_flyingmenu_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_flyingmenu_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_flyingmenu_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_flyingmenu_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_flyingmenu_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
} 

static void
_e_flyingmenu_cb_src_obj_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   if (sd->autodelete)
     evas_object_del(sd->obj);
}

static void
_e_flyingmenu_cb_src_obj_move(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return;

   _e_flyingmenu_move_and_resize(sd->obj);
}

static void
_e_flyingmenu_cb_src_obj_resize(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return;

   _e_flyingmenu_move_and_resize(sd->obj);
}
