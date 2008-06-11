/* e_nav_contact_editor.c -
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
#include "e_nav_contact_editor.h"
#include "e_nav_contact_list.h"
#include "e_nav_dialog.h"
#include "../e_nav_theme.h"
#include "../e_nav_misc.h"
#include "../e_ctrl.h"
#include <etk/Etk.h>

#define E_NEW(s, n) (s *)calloc(n, sizeof(s))
typedef struct _E_Button_Item E_Button_Item;
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _Embed_Canvas Embed_Canvas;

typedef enum _E_Nav_Movengine_Action
{
   E_NAV_MOVEENGINE_START,
   E_NAV_MOVEENGINE_STOP,
   E_NAV_MOVEENGINE_GO
} E_Nav_Movengine_Action;

struct mouse_status {
   int pressed;
   int s_x, s_y;
   int x, y;
   int pre_offset_x;
   int pre_offset_y;
};

struct _E_Button_Item
{
   Evas_Object     *obj;
   Evas_Object     *item_obj;
   Evas_Coord       sz;
   void (*func) (void *data, Evas_Object *obj, Evas_Object *src_obj);
   void            *data;
};

struct _Embed_Canvas
{
   Evas_Object     *frame;
   Etk_Widget      *embed;
   Etk_Widget      *vbox;
   Etk_Widget      *entry;
};

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   void            *src_obj;          
   Evas_Object     *bg_object;
   Evas_Object     *left_button;
   Evas_Object     *right_button;

   Evas_Object     *event;
   Evas_Object     *clip;

   Embed_Canvas    *embed;
   Contact_List    *contact_list;

   /* directory to find theme .edj files from the module - if there is one */
   const char      *dir;
};

static void _e_contact_editor_smart_init(void);
static void _e_contact_editor_smart_add(Evas_Object *obj);
static void _e_contact_editor_smart_del(Evas_Object *obj);
static void _e_contact_editor_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_contact_editor_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_contact_editor_smart_show(Evas_Object *obj);
static void _e_contact_editor_smart_hide(Evas_Object *obj);
static void _e_contact_editor_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_contact_editor_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_contact_editor_smart_clip_unset(Evas_Object *obj);

static void _e_contact_editor_update(Evas_Object *obj);

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_contact_editor")) return ret

Evas_Object *
e_contact_editor_add(Evas *e)
{
   _e_contact_editor_smart_init();
   return evas_object_smart_add(e, _e_smart);
}

static void
contact_editor_exit(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   e_contact_editor_deactivate(obj);
}

static void
contact_editor_save(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *object = (Evas_Object*)obj;
   E_Smart_Data *sd;
   
   SMART_CHECK(object, ;);

   const char *text = etk_entry_text_get(ETK_ENTRY(sd->embed->entry));
   e_dialog_textblock_text_set(sd->src_obj, text);
   e_contact_editor_deactivate(obj);
}

static void
_e_nav_contact_sel(void *data, void *data2)
{
   E_Smart_Data *sd; 
   Evas_Object *obj;
   char *contact_name;

   obj = data;
   if(!obj) return;

   sd = evas_object_smart_data_get(obj);
   if(!sd) 
     return;

   contact_name = (char *)(data2);
   etk_entry_text_set(ETK_ENTRY(sd->embed->entry), contact_name);
   etk_widget_focus(sd->embed->entry);

   e_nav_contact_list_deactivate(sd->contact_list);
   _e_contact_editor_update(obj);
}

static void
_e_nav_contact_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(data);
   
   if(!sd->contact_list) return;
   e_misc_keyboard_hide();
   e_nav_contact_list_activate(sd->contact_list);
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

static Etk_Bool 
_editor_focused_cb(Etk_Object *object, Etk_Event_Key_Down *event, void *data)
{
   e_misc_keyboard_launch();
   return TRUE;
}

static Etk_Bool 
_editor_unfocused_cb(Etk_Object *object, Etk_Event_Key_Down *event, void *data)
{
   e_misc_keyboard_hide();
   return TRUE;
}

void
e_contact_editor_theme_source_set(Evas_Object *obj, const char *custom_dir, void (*positive_func)(void *data, Evas_Object *obj, Evas_Object *src_obj), void *data1, void (*negative_func)(void *data, Evas_Object *obj, Evas_Object *src_obj), void *data2)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   char theme_file[PATH_MAX];

   snprintf(theme_file, PATH_MAX, "%s/%s.edj", THEME_PATH, e_nav_theme_name_get());
   
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
   if(!positive_func) bi->func = contact_editor_save;
   else bi->func = positive_func;
   bi->data = data1;
   bi->item_obj = e_nav_theme_object_new( evas_object_evas_get(obj), sd->dir, "modules/diversity_nav/button_28");
   evas_object_smart_member_add(bi->item_obj, obj);
   evas_object_clip_set(bi->item_obj, sd->clip);
   evas_object_event_callback_add(bi->item_obj, EVAS_CALLBACK_MOUSE_UP,
                                  _e_button_cb_mouse_up, bi);
   edje_object_part_text_set(bi->item_obj, "text", "OK");
   sd->left_button = bi->item_obj;

   bi2 = calloc(1, sizeof(E_Button_Item));
   bi2->obj = obj;
   if(!negative_func) bi2->func = contact_editor_exit;
   else bi2->func = negative_func;
   bi2->data = data2;
   bi2->item_obj = e_nav_theme_object_new( evas_object_evas_get(obj), sd->dir, "modules/diversity_nav/button_28");
   evas_object_smart_member_add(bi2->item_obj, obj);
   evas_object_clip_set(bi2->item_obj, sd->clip);
   evas_object_event_callback_add(bi2->item_obj, EVAS_CALLBACK_MOUSE_UP,
                                  _e_button_cb_mouse_up, bi2);
   edje_object_part_text_set(bi2->item_obj, "text", "Cancel");
   sd->right_button = bi2->item_obj;

   Embed_Canvas * ec = (Embed_Canvas *)malloc (sizeof(Embed_Canvas));
   memset(ec, 0, sizeof(Embed_Canvas)); 

   ec->frame = e_nav_theme_object_new(evas_object_evas_get(obj), custom_dir, "modules/diversity_nav/contact_editor"); 
   evas_object_smart_member_add(ec->frame, obj);
   evas_object_clip_set(ec->frame, sd->clip);

   /*
    * Setup the etk embed
    */
   ec->embed  = etk_embed_new(evas_object_evas_get(obj));
   edje_object_part_swallow(ec->frame, "swallow", etk_embed_object_get(ETK_EMBED(ec->embed)));

   /*
    * Create Box
    */
   ec->vbox = etk_vbox_new(ETK_FALSE, 0);
   etk_container_add(ETK_CONTAINER(ec->embed), ec->vbox);

   /*
    * Create Entry 
    */
   ec->entry = etk_entry_new();
   etk_box_append(ETK_BOX(ec->vbox), ec->entry, ETK_BOX_START, ETK_BOX_NONE, 0); // padding is 0
   etk_box_spacing_set(ETK_BOX(ec->vbox), 20);
   etk_signal_connect_by_code(ETK_WIDGET_FOCUSED_SIGNAL, ETK_OBJECT(ec->entry), ETK_CALLBACK(_editor_focused_cb), NULL);
   etk_signal_connect_by_code(ETK_WIDGET_UNFOCUSED_SIGNAL, ETK_OBJECT(ec->entry), ETK_CALLBACK(_editor_unfocused_cb), NULL);

   sd->embed = ec;
  
   Evas_Object *contact_button; 
   contact_button = edje_object_part_object_get(ec->frame, "button"); 

   evas_object_event_callback_add(contact_button, EVAS_CALLBACK_MOUSE_UP,
				  _e_nav_contact_button_cb_mouse_down, obj);

   sd->contact_list = e_nav_contact_list_new(obj, THEME_PATH);
}

void
e_contact_editor_source_object_set(Evas_Object *obj, void *src_obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   sd->src_obj = src_obj;
}

Evas_Object *
e_contact_editor_source_object_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
   return sd->src_obj;
}

void
e_contact_editor_activate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   if(!sd) return;
   evas_object_show(sd->event);
   _e_contact_editor_update(obj);
}

void
e_contact_editor_deactivate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   evas_object_hide(sd->event);

   e_misc_keyboard_hide();

   _e_contact_editor_smart_hide(obj);
   _e_contact_editor_smart_del(obj);
}

/* internal calls */
static void
_e_contact_editor_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     "e_contact_editor",
	       EVAS_SMART_CLASS_VERSION,
	       _e_contact_editor_smart_add,
	       _e_contact_editor_smart_del,
	       _e_contact_editor_smart_move,
	       _e_contact_editor_smart_resize,
	       _e_contact_editor_smart_show,
	       _e_contact_editor_smart_hide,
	       _e_contact_editor_smart_color_set,
	       _e_contact_editor_smart_clip_set,
	       _e_contact_editor_smart_clip_unset,
	       
	       NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_contact_editor_smart_add(Evas_Object *obj)
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
_e_contact_editor_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if(sd->bg_object) evas_object_del(sd->bg_object);
   if(sd->left_button)  evas_object_del(sd->left_button);
   if(sd->right_button) evas_object_del(sd->right_button);

   evas_object_del(sd->clip);
   evas_object_del(sd->event);
}

                    
static void
_e_contact_editor_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;
   _e_contact_editor_update(obj);
}

static void
_e_contact_editor_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   _e_contact_editor_update(obj);
}

static void
_e_contact_editor_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_contact_editor_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);

   evas_object_hide(sd->embed->frame);
   etk_widget_hide(sd->embed->vbox);
   etk_widget_hide(sd->embed->entry);
}

static void
_e_contact_editor_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_contact_editor_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_contact_editor_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
} 

void
e_contact_editor_input_length_limit_set(Evas_Object *obj, size_t length_limit)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);

   if(!sd) return;
   if(length_limit > 0) etk_entry_text_limit_set(ETK_ENTRY(sd->embed->entry), length_limit);
}

size_t
e_contact_editor_input_length_limit_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, 0;);

   if(!sd) return 0;
   return etk_entry_text_limit_get(ETK_ENTRY(sd->embed->entry));
}

void 
e_contact_editor_input_set(Evas_Object *obj, const char *name, const char *input)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);

   if(!sd) return;
   if(sd->embed)
     {
        edje_object_part_text_set(sd->embed->frame, "title", name);
        if(!input || !strcmp(input, "")) return;

        etk_entry_text_set(ETK_ENTRY(sd->embed->entry), input);
     }
}

const char * 
e_contact_editor_input_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
   if(!sd) return NULL;
   return etk_entry_text_get(ETK_ENTRY(sd->embed->entry));
}

void 
e_contact_editor_contacts_set(Evas_Object *obj, Ecore_List *list)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   Contact_List_Item *item;
   Neo_Other_Data *neod;

   ecore_list_first_goto(list);
   while ((neod = ecore_list_current(list)))
     {
        if(!neod) continue;
        item = E_NEW(Contact_List_Item, 1);
        if (neod->name)
          item->name = strdup(neod->name); 
        if (neod->phone)
          item->description = strdup(neod->phone); 
        item->func = _e_nav_contact_sel;
        item->data = obj;
        item->data2 = item->name;
        e_nav_contact_list_item_add(sd->contact_list, item);
        ecore_list_next(list);
     }
}

static void
_e_contact_editor_update(Evas_Object *obj)
{
   Evas_Coord screen_x, screen_y, screen_w, screen_h;
   E_Smart_Data *sd;
   int indent=10;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   evas_output_viewport_get(evas_object_evas_get(obj), &screen_x, &screen_y, &screen_w, &screen_h);
   evas_object_move(sd->bg_object, screen_x, screen_y);
   evas_object_resize(sd->bg_object, screen_w, 10000);

   e_nav_contact_list_coord_set(sd->contact_list, screen_x, screen_y, screen_w, screen_h);

   evas_object_show(sd->bg_object);
   if(sd->left_button)
     {
        evas_object_resize(sd->left_button, screen_w*(1.0/3), (screen_h*(1.0/8)) );
        evas_object_move(sd->left_button, screen_x+indent, screen_y+indent);
        evas_object_show(sd->left_button);
     }
   if(sd->right_button)
     {
        evas_object_resize(sd->right_button, screen_w*(1.0/3), (screen_h*(1.0/8)) );
        evas_object_move(sd->right_button, screen_w-indent-(screen_w*(1.0/3)), screen_y+indent);
        evas_object_show(sd->right_button);
     }

   evas_object_move(sd->embed->frame, indent, (screen_h*(1.0/8)) + indent*2);
   evas_object_resize(sd->embed->frame, screen_w - (indent * 2), screen_h - ((screen_h*(1.0/8)) + indent*2));
   evas_object_show(sd->embed->frame);
   etk_widget_show_all(ETK_WIDGET(sd->embed->embed));
   etk_widget_show(sd->embed->vbox);
   etk_widget_show(sd->embed->entry);
   etk_widget_focus(sd->embed->entry);
}

