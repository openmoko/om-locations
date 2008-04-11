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
#include "e_nav_theme.h"

/* navigator object */
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Button_Item E_Button_Item;

struct _E_Button_Item
{
   Evas_Object *obj;
   Evas_Object *item_obj;
   Evas_Coord sz;
   void (*func) (void *data, Evas_Object *obj, Evas_Object *src_obj);
   void *data;
};

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *src_obj;
   Evas_Object     *bg_object;
   Evas_Object     *title_object;
   int              title_color_r;
   int              title_color_g;
   int              title_color_b;
   int              title_color_a;
   Evas_Object     *event;
   
   Evas_Object     *clip;

   Evas_List      *buttons;
   double           activate_time;
   int              activate_deactivate;
   Ecore_Animator  *animator;
   
   /* directory to find theme .edj files from the module - if there is one */
   const char      *dir;
   unsigned char active : 1;
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

static int  _e_alert_cb_animator(void *data);
static void _e_alert_update(Evas_Object *obj);
static void _e_alert_cb_src_obj_del(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_alert_cb_src_obj_move(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_alert_cb_src_obj_resize(void *data, Evas *evas, Evas_Object *obj, void *event);

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
e_alert_source_object_set(Evas_Object *obj, Evas_Object *src_obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   if (sd->src_obj)
     {
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_DEL,
				       _e_alert_cb_src_obj_del);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_MOVE,
				       _e_alert_cb_src_obj_move);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_RESIZE,
				       _e_alert_cb_src_obj_resize);
     }
   sd->src_obj = src_obj;
   if (sd->src_obj)
     {
	evas_object_event_callback_add(sd->src_obj, EVAS_CALLBACK_DEL,
				       _e_alert_cb_src_obj_del, obj);
	evas_object_event_callback_add(sd->src_obj, EVAS_CALLBACK_MOVE,
				       _e_alert_cb_src_obj_move, obj);
	evas_object_event_callback_add(sd->src_obj, EVAS_CALLBACK_RESIZE,
				       _e_alert_cb_src_obj_resize, obj);
     }
}

Evas_Object *
e_alert_source_object_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
   return sd->src_obj;
}

void
e_alert_activate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   evas_object_show(sd->event);
   if (sd->active) return;
   sd->activate_deactivate = 1;
   sd->active = 1;
   sd->activate_time = ecore_time_get();
   _e_alert_update(obj);

   if (sd->animator) return;
   sd->animator = ecore_animator_add(_e_alert_cb_animator, obj);
}

void
e_alert_deactivate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   if (!sd->active) return;
   sd->activate_deactivate = -1;

   evas_object_hide(sd->event);
   _e_alert_smart_hide(obj);
   _e_alert_smart_del(obj);    
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
   
   evas_object_smart_data_set(obj, sd);
}

static void
_e_alert_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->src_obj)
     {
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_DEL,
				       _e_alert_cb_src_obj_del);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_MOVE,
				       _e_alert_cb_src_obj_move);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_RESIZE,
				       _e_alert_cb_src_obj_resize);
     }
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
   if (!sd->src_obj) return;
   if (bi->func) bi->func(bi->data, bi->obj, sd->src_obj);
}

void
e_alert_button_add(Evas_Object *obj, const char *label, void (*func) (void *data, Evas_Object *obj, Evas_Object *src_obj), void *data)
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
        sd->title_color_r = 255;
        sd->title_color_g = 255;
        sd->title_color_b = 255;
        sd->title_color_a = 255;
     }
}

void
e_alert_title_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   Evas_Object *o;
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

   if (sd->activate_deactivate == -1)
     {
	sd->activate_deactivate = 0;
	sd->active = 0;
	evas_object_hide(sd->event);
        return;
     }

   int screen_x, screen_y, screen_w, screen_h;
   evas_output_viewport_get(evas_object_evas_get(obj), &screen_x, &screen_y, &screen_w, &screen_h);
   int alert_x = 0;
   int alert_y = 0;
   int alert_w = screen_w;
   int alert_h = 0;

   double t;
   if (sd->activate_deactivate == 0)
     {
        alert_y = screen_h* (1.0/3);
        alert_h = screen_h * (1.0/3);
     }
   else if (sd->activate_deactivate == 1)
     {
	t = ecore_time_get() - sd->activate_time;
	t = t / 1.0; /* anim time */
	if (t >= 1.0) t = 1.0;
	t = 1.0 - ((1.0 - t) * (1.0 - t)); /* decelerate */
	if (t >= 1.0) sd->activate_deactivate = 0;
        alert_y = ((-screen_h) * (1.0/3)) + (t * screen_h * (1.0/3) * 2);
        alert_h = screen_h * (1.0/3);
     }

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
        tmp_x = tmp_x + button_w + 1;
     }
}

static int
_e_alert_cb_animator(void *data)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return 0;
 
   _e_alert_update(sd->obj);
   if (sd->activate_deactivate == 0)
     {
	sd->animator = NULL;
	if (!sd->active) evas_object_del(sd->obj);
	return 0;
     }
   return 1;
}

static void
_e_alert_cb_src_obj_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
}

static void
_e_alert_cb_src_obj_move(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   _e_alert_update(sd->obj);
}

static void
_e_alert_cb_src_obj_resize(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   _e_alert_update(sd->obj);
}
