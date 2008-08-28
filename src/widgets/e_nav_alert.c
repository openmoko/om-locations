/* e_nav_alert.c -
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

#include "../e_nav.h"
#include "e_nav_alert.h"
#include "../e_nav_theme.h"
#include "../e_nav_misc.h"

/* navigator object */
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Button_Item E_Button_Item;

struct _E_Button_Item
{
   Evas_Object *obj;
   Evas_Object *item_obj;
   Evas_Coord sz;
   void (*func) (void *data, Evas_Object *obj);
   void *data;
};

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *bg_object;
   Evas_Object     *title_object;
   int              title_color_r;
   int              title_color_g;
   int              title_color_b;
   int              title_color_a;
   Evas_Object     *event;
   
   Evas_Object     *clip;

   Evas_List      *buttons;

   E_Nav_Drop_Data *drop;

   /* directory to find theme .edj files from the module - if there is one */
   const char      *dir;
};

static void _e_alert_smart_init(void);
static void _e_alert_smart_add(Evas_Object *obj);
static void _e_alert_smart_del(Evas_Object *obj);
static void _e_alert_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_alert_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_alert_smart_show(Evas_Object *obj);
static void _e_alert_smart_hide(Evas_Object *obj);
static void _e_alert_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_alert_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_alert_smart_clip_unset(Evas_Object *obj);

static void _e_alert_update(Evas_Object *obj);

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_alert")) return ret

Evas_Object *
e_alert_add(Evas *e)
{
   _e_alert_smart_init();
   return evas_object_smart_add(e, _e_smart);
}

void
e_alert_theme_source_set(Evas_Object *obj, const char *custom_dir)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   
   sd->dir = custom_dir;
   sd->bg_object = evas_object_rectangle_add(evas_object_evas_get(obj)); 
   evas_object_smart_member_add(sd->bg_object, obj);
   evas_object_move(sd->bg_object, sd->x, sd->y);
   evas_object_resize(sd->bg_object, sd->w, sd->h);
   evas_object_color_set(sd->bg_object, 0, 0, 0, 200);
   evas_object_clip_set(sd->bg_object, sd->clip);
   evas_object_repeat_events_set(sd->bg_object, 1);
   evas_object_show(sd->bg_object);
}

void
e_alert_activate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   evas_object_show(sd->event);

   if (e_nav_drop_active_get(sd->drop))
     return;

   e_nav_drop_apply(sd->drop, obj, 0, 0, 0, 0);
   _e_alert_update(obj);
}

void
e_alert_deactivate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   e_nav_drop_stop(sd->drop, FALSE);

   evas_object_del(obj);
}

/* internal calls */
static void
_e_alert_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     "e_alert",
	       EVAS_SMART_CLASS_VERSION,
	       _e_alert_smart_add,
	       _e_alert_smart_del,
	       _e_alert_smart_move,
	       _e_alert_smart_resize,
	       _e_alert_smart_show,
	       _e_alert_smart_hide,
	       _e_alert_smart_color_set,
	       _e_alert_smart_clip_set,
	       _e_alert_smart_clip_unset,
	       
	       NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_alert_smart_add(Evas_Object *obj)
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
   
   sd->title_color_r = 255;
   sd->title_color_g = 255;
   sd->title_color_b = 255;
   sd->title_color_a = 255;

   sd->drop = e_nav_drop_new(1.0, NULL, NULL);

   evas_object_smart_data_set(obj, sd);
}

static void
_e_alert_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   e_nav_drop_destroy(sd->drop);

   while (sd->buttons)   
     {
	E_Button_Item *bi;
	
	bi = sd->buttons->data;
	sd->buttons = evas_list_remove_list(sd->buttons, sd->buttons);
	evas_object_del(bi->item_obj);
	free(bi);
     }
   if(sd->bg_object) evas_object_del(sd->bg_object);
   if(sd->title_object) evas_object_del(sd->title_object);
   evas_object_del(sd->clip);
   evas_object_del(sd->event);
}
                    
static void
_e_alert_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;
   _e_alert_update(obj);
}

static void
_e_alert_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   _e_alert_update(obj);
}

static void
_e_alert_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_alert_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_alert_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_alert_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_alert_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
} 

static void
_e_button_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Button_Item *bi;
   E_Smart_Data *sd;
     
   bi = data;
   if (!bi) return;
   sd = evas_object_smart_data_get(bi->obj);
   if (!sd) return;
   if (bi->func) bi->func(bi->data, bi->obj);
}

void
e_alert_button_add(Evas_Object *obj, const char *label, void (*func) (void *data, Evas_Object *obj), void *data)
{
   E_Smart_Data *sd;
   E_Button_Item *bi;
   
   SMART_CHECK(obj, ;);
   bi = calloc(1, sizeof(E_Button_Item));
   bi->obj = obj;
   bi->func = func;
   bi->data = data;
   bi->item_obj = e_nav_theme_object_new( evas_object_evas_get(obj), sd->dir, "modules/diversity_nav/button_48");
   evas_object_smart_member_add(bi->item_obj, obj);
   evas_object_clip_set(bi->item_obj, sd->clip);
   evas_object_event_callback_add(bi->item_obj, EVAS_CALLBACK_MOUSE_UP,
				  _e_button_cb_mouse_up, bi);
   
   edje_object_part_text_set(bi->item_obj, "text", label);
   sd->buttons = evas_list_append(sd->buttons, bi);
}

void 
e_alert_title_set(Evas_Object *obj, const char *title, const char *message)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   if(!sd) return;
   if(!sd->title_object) 
     {
        Evas_Object *o;
        o = e_nav_theme_object_new( evas_object_evas_get(obj), sd->dir, "modules/diversity_nav/alert/text");
        sd->title_object = o;
        edje_object_part_text_set(sd->title_object, "title", title);
        edje_object_part_text_set(sd->title_object, "message", message);
     }
}

void
e_alert_title_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   if(!sd) return;
   sd->title_color_r = r;
   sd->title_color_g = g;
   sd->title_color_b = b;
   sd->title_color_a = a;
}

static void
_e_alert_update(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   /* adjust dropping */
   if (e_nav_drop_active_get(sd->drop))
     {
	Evas_Coord x, y, w, h;

	evas_output_viewport_get(evas_object_evas_get(obj),
	      &x, &y, &w, &h);

	y = h / 3.0;
	h /= 3.0;

	if (sd->w != w || sd->h != h)
	  e_nav_drop_apply(sd->drop, obj, x, y, w, h);
     }

   int alert_x = sd->x;
   int alert_y = sd->y;
   int alert_w = sd->w;
   int alert_h = sd->h;

   evas_object_move(sd->bg_object, alert_x, alert_y );
   evas_object_resize(sd->bg_object, alert_w, alert_h );
   evas_object_show(sd->bg_object);

   if(sd->title_object)
     {
        evas_object_resize(sd->title_object, alert_w, alert_h*(3.0/7));
        evas_object_move(sd->title_object, alert_x, alert_y );
        Evas_Object *o = edje_object_part_object_get(sd->title_object, "title");
        evas_object_color_set(o, sd->title_color_r, sd->title_color_g, sd->title_color_b, sd->title_color_a);
        evas_object_show(sd->title_object);
     }

   int tmp_x;
   Evas_List *l;
   int indent = 20;
   int button_w, button_h; 
   int bc = evas_list_count(sd->buttons);
   E_Button_Item *bi;

   if(bc==0) return;
   tmp_x = alert_x + indent;
   button_w = (int)( ( alert_w - (indent * 2) ) / bc );
   button_h = alert_h*(2.0/7);

   for (l = sd->buttons; l; l = l->next)
     {
        bi = l->data;
        evas_object_move(bi->item_obj, tmp_x, (alert_y+alert_h*(3.0/7)) );
        evas_object_resize(bi->item_obj, button_w, button_h);
        evas_object_show(bi->item_obj);
        tmp_x = tmp_x + button_w + 3;
     }
}
