/* e_nav_taglist.c -
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

#include "e_nav.h"
#include "e_nav_taglist.h"
#include "widgets/e_nav_titlepane.h"
#include "widgets/e_ilist.h"
#include "widgets/e_nav_theme.h"

typedef struct _E_Smart_Data E_Smart_Data;

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *bg_object;
   Evas_Object     *title_object;
   Evas_Object     *list_object;

   Evas_Object     *clip;
   /* directory to find theme .edj files from the module - if there is one */
   const char      *dir;
   
};

static void _e_taglist_smart_init(void);
static void _e_taglist_smart_add(Evas_Object *obj);
static void _e_taglist_smart_del(Evas_Object *obj);
static void _e_taglist_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_taglist_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_taglist_smart_show(Evas_Object *obj);
static void _e_taglist_smart_hide(Evas_Object *obj);
static void _e_taglist_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_taglist_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_taglist_smart_clip_unset(Evas_Object *obj);

static void _e_taglist_update(Evas_Object *obj);

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_taglist")) return ret

Evas_Object *
e_nav_taglist_add(Evas *e)
{
   _e_taglist_smart_init();
   return evas_object_smart_add(e, _e_smart);
}

void
e_nav_taglist_theme_source_set(Evas_Object *obj, const char *custom_dir)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   
   sd->dir = custom_dir;
   sd->bg_object = evas_object_rectangle_add(evas_object_evas_get(obj)); 
   evas_object_smart_member_add(sd->bg_object, obj);
   evas_object_move(sd->bg_object, sd->x, sd->y);
   evas_object_resize(sd->bg_object, sd->w, sd->h);
   evas_object_color_set(sd->bg_object, 0, 0, 0, 255);
   evas_object_clip_set(sd->bg_object, sd->clip);
   evas_object_repeat_events_set(sd->bg_object, 1);

   sd->title_object = e_nav_titlepane_add(evas_object_evas_get(obj));
   e_nav_titlepane_theme_source_set(sd->title_object, THEME_PATH);
   e_nav_titlepane_set_message(sd->title_object, "View Tags");
   e_nav_titlepane_hide_buttons(sd->title_object);
   evas_object_smart_member_add(sd->title_object, obj);   
   evas_object_clip_set(sd->title_object, sd->clip);

   sd->list_object = e_ilist_add( evas_object_evas_get(obj) );
   evas_object_smart_member_add(sd->list_object, obj);
   evas_object_clip_set( sd->list_object, sd->clip);   
}

void
e_nav_taglist_tag_update(Evas_Object *obj, const char *name, const char *description, void *object)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   Evas_Object *location_obj = NULL;
   int n;
   int count = e_ilist_count(sd->list_object); 
   for(n=0; n<count; n++) 
     {
        e_ilist_selected_set(sd->list_object, n);
        location_obj = (Evas_Object*) e_ilist_selected_data2_get(sd->list_object);
        if(location_obj == object)
          break;
     }
   if(location_obj) 
     e_ilist_nth_label_set(sd->list_object, n, name);
}

void
e_nav_taglist_tag_add(Evas_Object *obj, const char *name, const char *description, void (*func) (void *data, void *data2), void *data1, void *data2)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   e_ilist_append(sd->list_object, NULL, name, 20, func, NULL, data1, data2);
}

void
e_nav_taglist_activate(Evas_Object *obj)
{
   _e_taglist_update(obj);
}

void
e_nav_taglist_deactivate(Evas_Object *obj)
{
   _e_taglist_smart_hide(obj);
}

static void
_e_taglist_update(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   int screen_x, screen_y, screen_w, screen_h;
   evas_output_viewport_get(evas_object_evas_get(obj), &screen_x, &screen_y, &screen_w, &screen_h);
   evas_object_move(sd->bg_object, screen_x, screen_y);
   evas_object_resize(sd->bg_object, screen_w, screen_h);
   evas_object_show(sd->bg_object);

   if(sd->title_object)
     {
        evas_object_resize(sd->title_object, screen_w, screen_h*(1.0/10) );
        evas_object_move(sd->title_object, screen_x, screen_y );
        evas_object_show(sd->title_object);
     }
   if(sd->list_object)
     {
        int count = e_ilist_count(sd->list_object);
        evas_object_move(sd->list_object, 0, screen_h*(1.0/10));
        evas_object_resize(sd->list_object, screen_w, count * 70);   
        evas_object_show(sd->list_object);
     }
}

/* internal calls */
static void
_e_taglist_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     "e_taglist",
	       EVAS_SMART_CLASS_VERSION,
	       _e_taglist_smart_add,
	       _e_taglist_smart_del,
	       _e_taglist_smart_move,
	       _e_taglist_smart_resize,
	       _e_taglist_smart_show,
	       _e_taglist_smart_hide,
	       _e_taglist_smart_color_set,
	       _e_taglist_smart_clip_set,
	       _e_taglist_smart_clip_unset,
	       
	       NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_taglist_smart_add(Evas_Object *obj)
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
   evas_object_smart_data_set(obj, sd);
}

static void
_e_taglist_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if(sd->bg_object) evas_object_del(sd->bg_object);
   if(sd->title_object) evas_object_del(sd->title_object);
   if(sd->list_object) evas_object_del(sd->list_object);
   evas_object_del(sd->clip);
}
                    
static void
_e_taglist_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;
   _e_taglist_update(obj);
}

static void
_e_taglist_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   _e_taglist_update(obj);
}

static void
_e_taglist_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_taglist_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_taglist_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_taglist_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_taglist_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
} 


