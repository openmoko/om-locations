/* e_nav_textedit.c -
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
#include "e_nav_textedit.h"
#include "e_nav_dialog.h"
#include "e_entry.h"
#include "e_nav_theme.h"

typedef struct _E_Button_Item E_Button_Item;
typedef struct _E_Smart_Data E_Smart_Data;

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
   void            *src_obj;          
   Evas_Object     *bg_object;
   Evas_Object     *left_button;
   Evas_Object     *right_button;
   Evas_Object     *title_object;
   Evas_Object     *entry_object;
   const char      *input_text;
   Evas_Object     *event;
   Evas_Object     *clip;
   
   /* directory to find theme .edj files from the module - if there is one */
   const char      *dir;
   
};

static void _e_textedit_smart_init(void);
static void _e_textedit_smart_add(Evas_Object *obj);
static void _e_textedit_smart_del(Evas_Object *obj);
static void _e_textedit_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_textedit_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_textedit_smart_show(Evas_Object *obj);
static void _e_textedit_smart_hide(Evas_Object *obj);
static void _e_textedit_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_textedit_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_textedit_smart_clip_unset(Evas_Object *obj);

static void _e_textedit_update(Evas_Object *obj);

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_textedit")) return ret

Evas_Object *
e_textedit_add(Evas *e)
{
   _e_textedit_smart_init();
   return evas_object_smart_add(e, _e_smart);
}

static void
textedit_exit(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("textedit exit\n");
   e_textedit_deactivate(obj);
}

static void
textedit_save(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("textedit save\n");
   Evas_Object *object = (Evas_Object*)obj;
   E_Smart_Data *sd;
   
   SMART_CHECK(object, ;);

   const char *text = e_entry_text_get(sd->entry_object);
   e_dialog_textblock_text_set(sd->src_obj, text);
   e_textedit_deactivate(obj);
}

static void
_e_button_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event)
{     // data is location_data, bi->obj is textedit object,  
   E_Button_Item *bi;
   E_Smart_Data *sd;
     
   bi = data;
   if (!bi) return;
   sd = evas_object_smart_data_get(bi->obj);
   if (!sd) return;
   if (!sd->src_obj) return;
   if (bi->func) bi->func(bi->data, bi->obj, sd->src_obj);   // src_obj is location item object
}

void
e_textedit_theme_source_set(Evas_Object *obj, const char *custom_dir, void (*positive_func)(void *data, Evas_Object *obj, Evas_Object *src_obj), void *data1, void (*negative_func)(void *data, Evas_Object *obj, Evas_Object *src_obj), void *data2)
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
   evas_object_show(sd->bg_object);

   E_Button_Item *bi, *bi2;
   bi = calloc(1, sizeof(E_Button_Item));
   bi->obj = obj;
   if(!positive_func) bi->func = textedit_save;
   else bi->func = positive_func;
   bi->data = data1;
   bi->item_obj = e_nav_theme_object_new( evas_object_evas_get(obj), sd->dir, "modules/diversity_nav/button");
   evas_object_smart_member_add(bi->item_obj, obj);
   evas_object_clip_set(bi->item_obj, sd->clip);
   evas_object_event_callback_add(bi->item_obj, EVAS_CALLBACK_MOUSE_UP,
                                  _e_button_cb_mouse_up, bi);
   edje_object_part_text_set(bi->item_obj, "text", "OK");
   sd->left_button = bi->item_obj;

   bi2 = calloc(1, sizeof(E_Button_Item));
   bi2->obj = obj;
   if(!negative_func) bi2->func = textedit_exit;
   else bi2->func = negative_func;
   bi2->data = data2;
   bi2->item_obj = e_nav_theme_object_new( evas_object_evas_get(obj), sd->dir, "modules/diversity_nav/button");
   evas_object_smart_member_add(bi2->item_obj, obj);
   evas_object_clip_set(bi2->item_obj, sd->clip);
   evas_object_event_callback_add(bi2->item_obj, EVAS_CALLBACK_MOUSE_UP,
                                  _e_button_cb_mouse_up, bi2);
   edje_object_part_text_set(bi2->item_obj, "text", "Cancel");
   sd->right_button = bi2->item_obj;
}

void
e_textedit_source_object_set(Evas_Object *obj, void *src_obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   sd->src_obj = src_obj;
}

Evas_Object *
e_textedit_source_object_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
   return sd->src_obj;
}

void
e_textedit_activate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   evas_object_show(sd->event);
   _e_textedit_update(obj);
}

void
e_textedit_deactivate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   evas_object_hide(sd->event);

   _e_textedit_smart_hide(obj);
   _e_textedit_smart_del(obj);
}

/* internal calls */
static void
_e_textedit_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     "e_textedit",
	       EVAS_SMART_CLASS_VERSION,
	       _e_textedit_smart_add,
	       _e_textedit_smart_del,
	       _e_textedit_smart_move,
	       _e_textedit_smart_resize,
	       _e_textedit_smart_show,
	       _e_textedit_smart_hide,
	       _e_textedit_smart_color_set,
	       _e_textedit_smart_clip_set,
	       _e_textedit_smart_clip_unset,
	       
	       NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_textedit_smart_add(Evas_Object *obj)
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
_e_textedit_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if(sd->bg_object) evas_object_del(sd->bg_object);
   if(sd->left_button)  evas_object_del(sd->left_button);
   if(sd->right_button) evas_object_del(sd->right_button);
   if(sd->title_object) evas_object_del(sd->title_object);
   if(sd->entry_object) evas_object_del(sd->entry_object);
   if(sd->input_text) free((void*)sd->input_text);
   evas_object_del(sd->clip);
   evas_object_del(sd->event);
   free(sd);
}
                    
static void
_e_textedit_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;
   _e_textedit_update(obj);
}

static void
_e_textedit_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   _e_textedit_update(obj);
}

static void
_e_textedit_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_textedit_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_textedit_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_textedit_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_textedit_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
} 

void 
e_textedit_input_set(Evas_Object *obj, const char *name, const char *input)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   if(!sd) return;
   if(!sd->title_object) 
     {
        Evas_Object *o;
        o = evas_object_text_add( evas_object_evas_get(obj) ); 
        evas_object_text_text_set(o, name);
        evas_object_text_font_set(o, "Sans:style=Bold,Edje-Vera-Bold", 20);
        evas_object_text_glow_color_set(o, 255, 255, 255, 255);
        sd->title_object = o;
        evas_object_smart_member_add(sd->title_object, obj);
        evas_object_clip_set(sd->title_object, sd->clip);

        Evas_Object *entry;
        entry = e_entry_add( evas_object_evas_get(obj) );
	if(sd->input_text) printf("There are already text !!!!!!!!!!!!!!!!\n");
        sd->input_text = strdup(input);
        e_entry_text_set(entry, sd->input_text);
        sd->entry_object = entry;
        evas_object_smart_member_add(sd->entry_object, obj);
        evas_object_clip_set(sd->entry_object, sd->clip);
     }
}

const char * 
e_textedit_input_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
   if(!sd) return NULL;
   return e_entry_text_get(sd->entry_object);
}

static void
_e_textedit_update(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   int screen_x, screen_y, screen_w, screen_h;
   int indent=10;
   evas_output_viewport_get(evas_object_evas_get(obj), &screen_x, &screen_y, &screen_w, &screen_h);
   evas_object_move(sd->bg_object, screen_x, screen_y);
   evas_object_resize(sd->bg_object, screen_w, screen_h);
   evas_object_show(sd->bg_object);
   if(sd->left_button)
     {
        evas_object_resize(sd->left_button, screen_w*(1.0/6), (screen_h*(1.0/8)) );
        evas_object_move(sd->left_button, screen_x+indent, screen_y+indent);
        evas_object_show(sd->left_button);
     }
   if(sd->right_button)
     {
        evas_object_resize(sd->right_button, screen_w*(1.0/6), (screen_h*(1.0/8)) );
        evas_object_move(sd->right_button, screen_w-indent-(screen_w*(1.0/6)), screen_y+indent);
        evas_object_show(sd->right_button);
     }
   if(sd->title_object)
     {
        evas_object_resize(sd->title_object, screen_w, 30);
        evas_object_move(sd->title_object, indent, ((screen_h*(1.0/6))+indent) );
        evas_object_show(sd->title_object);
     }
   if(sd->entry_object)
     {
        e_entry_focus(sd->entry_object);
        e_entry_enable(sd->entry_object);
        evas_object_resize(sd->entry_object, screen_w-(indent*2), 40);   
        evas_object_move(sd->entry_object, indent,  (((screen_h*(1.0/6))+indent) + 30 + indent) );
        evas_object_show(sd->entry_object);
     }
}

