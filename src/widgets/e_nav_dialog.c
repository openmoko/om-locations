/* e_nav_dialog.c -
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
#include "e_nav_dialog.h"
#include "e_widget_textblock.h"
#include "e_nav_textedit.h"
#include "e_nav_theme.h"

/* navigator object */
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Button_Item E_Button_Item;
typedef struct _E_TextBlock_Item E_TextBlock_Item;

struct _E_TextBlock_Item
{
   Evas_Object *obj;
   Evas_Coord sz;
   const char *label;
   const char *input;
   Evas_Object *label_obj;
   Evas_Object *item_obj;
   void *data;
};

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
   Evas_Object     *text_object;
   Evas_Object     *event;
   
   Evas_Object     *clip;

   Evas_List      *textblocks;
   Evas_List      *buttons;
   double           activate_time;
   int              activate_deactivate;
   Ecore_Animator  *animator;
   
   /* directory to find theme .edj files from the module - if there is one */
   const char      *dir;
   unsigned char active : 1;
};

static void _e_dialog_smart_init(void);
static void _e_dialog_smart_add(Evas_Object *obj);
static void _e_dialog_smart_del(Evas_Object *obj);
static void _e_dialog_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_dialog_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_dialog_smart_show(Evas_Object *obj);
static void _e_dialog_smart_hide(Evas_Object *obj);
static void _e_dialog_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_dialog_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_dialog_smart_clip_unset(Evas_Object *obj);

static int  _e_dialog_cb_animator(void *data);
static void _e_dialog_update(Evas_Object *obj);
static void _e_dialog_cb_src_obj_del(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_dialog_cb_src_obj_move(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_dialog_cb_src_obj_resize(void *data, Evas *evas, Evas_Object *obj, void *event);

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_dialog")) return ret

Evas_Object *
e_dialog_add(Evas *e)
{
   _e_dialog_smart_init();
   return evas_object_smart_add(e, _e_smart);
}

void
e_dialog_theme_source_set(Evas_Object *obj, const char *custom_dir)
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
e_dialog_source_object_set(Evas_Object *obj, Evas_Object *src_obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   if (sd->src_obj)
     {
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_DEL,
				       _e_dialog_cb_src_obj_del);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_MOVE,
				       _e_dialog_cb_src_obj_move);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_RESIZE,
				       _e_dialog_cb_src_obj_resize);
     }
   sd->src_obj = src_obj;
   if (sd->src_obj)
     {
	evas_object_event_callback_add(sd->src_obj, EVAS_CALLBACK_DEL,
				       _e_dialog_cb_src_obj_del, obj);
	evas_object_event_callback_add(sd->src_obj, EVAS_CALLBACK_MOVE,
				       _e_dialog_cb_src_obj_move, obj);
	evas_object_event_callback_add(sd->src_obj, EVAS_CALLBACK_RESIZE,
				       _e_dialog_cb_src_obj_resize, obj);
     }
}

Evas_Object *
e_dialog_source_object_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
   return sd->src_obj;
}

void
e_dialog_activate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   evas_object_show(sd->event);
   if (sd->active) return;
   sd->activate_deactivate = 1;
   sd->active = 1;
   sd->activate_time = ecore_time_get();
   _e_dialog_update(obj);

   if (sd->animator) return;
   sd->animator = ecore_animator_add(_e_dialog_cb_animator, obj);
}

void
e_dialog_deactivate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   if (!sd->active) return;
   sd->activate_deactivate = -1;

   evas_object_hide(sd->event);
   _e_dialog_smart_hide(obj);
   _e_dialog_smart_del(obj);    
}

/* internal calls */
static void
_e_dialog_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     "e_dialog",
	       EVAS_SMART_CLASS_VERSION,
	       _e_dialog_smart_add,
	       _e_dialog_smart_del,
	       _e_dialog_smart_move,
	       _e_dialog_smart_resize,
	       _e_dialog_smart_show,
	       _e_dialog_smart_hide,
	       _e_dialog_smart_color_set,
	       _e_dialog_smart_clip_set,
	       _e_dialog_smart_clip_unset,
	       
	       NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_dialog_smart_add(Evas_Object *obj)
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
_e_dialog_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->src_obj)
     {
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_DEL,
				       _e_dialog_cb_src_obj_del);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_MOVE,
				       _e_dialog_cb_src_obj_move);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_RESIZE,
				       _e_dialog_cb_src_obj_resize);
     }
   while (sd->textblocks)   
     {
	E_TextBlock_Item *tbi;
	
	tbi = sd->textblocks->data;
	sd->textblocks = evas_list_remove_list(sd->textblocks, sd->textblocks);
	evas_object_del(tbi->label_obj);
	evas_object_del(tbi->item_obj);
	free(tbi);
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
   if(sd->text_object) evas_object_del(sd->text_object);
   evas_object_del(sd->clip);
   evas_object_del(sd->event);
}
                    
static void
_e_dialog_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;
   _e_dialog_update(obj);
}

static void
_e_dialog_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   _e_dialog_update(obj);
}

static void
_e_dialog_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_dialog_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_dialog_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_dialog_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_dialog_smart_clip_unset(Evas_Object *obj)
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
e_dialog_button_add(Evas_Object *obj, const char *label, void (*func) (void *data, Evas_Object *obj, Evas_Object *src_obj), void *data)
{
   E_Smart_Data *sd;
   E_Button_Item *bi;
   
   SMART_CHECK(obj, ;);
   bi = calloc(1, sizeof(E_Button_Item));
   bi->obj = obj;
   bi->func = func;
   bi->data = data;
   bi->item_obj = e_nav_theme_object_new( evas_object_evas_get(obj), sd->dir, "modules/diversity_nav/button");
   evas_object_smart_member_add(bi->item_obj, obj);
   evas_object_clip_set(bi->item_obj, sd->clip);
   evas_object_event_callback_add(bi->item_obj, EVAS_CALLBACK_MOUSE_UP,
				  _e_button_cb_mouse_up, bi);
   
   edje_object_part_text_set(bi->item_obj, "text", label);
   sd->buttons = evas_list_append(sd->buttons, bi);
}

void 
e_dialog_title_set(Evas_Object *obj, const char *title, const char *message)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   if(!sd) return;
   if(!sd->title_object) 
     {
        Evas_Object *o;
        o = e_nav_theme_object_new( evas_object_evas_get(obj), sd->dir, "modules/diversity_nav/dialog/text");
        sd->title_object = o;
        edje_object_part_text_set(sd->title_object, "title", title);
        edje_object_part_text_set(sd->title_object, "message", message);
     }
}

void 
e_dialog_textblock_text_set(void *obj, const char *input)
{
   E_TextBlock_Item *tbi = (E_TextBlock_Item*)obj;
   if(tbi->input) free((void*)tbi->input);
   if(input)
     tbi->input = strdup(input);
   else
     tbi->input = strdup("");
   e_widget_textblock_plain_set(tbi->item_obj, tbi->input);
   _e_dialog_update(tbi->obj);
}

const char *
e_dialog_textblock_text_get(Evas_Object *obj, const char *label)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;
   E_TextBlock_Item *tbi;
   Evas_List *l;
   for (l = sd->textblocks; l; l = l->next)
     {
        tbi = l->data;
        if(!strcmp(tbi->label, label))
          {
             return tbi->input; 
          }
     }
   return NULL;
}

static void
_e_textblock_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_TextBlock_Item *tbi = (E_TextBlock_Item*)data;
   Evas_Object *teo = e_textedit_add(evas);
   e_textedit_theme_source_set(teo, THEME_PATH, NULL, NULL, NULL, NULL);  
   e_textedit_source_object_set(teo, data); // data is tbi ( TextBlock_Item)
   e_textedit_input_set(teo, evas_object_text_text_get(tbi->label_obj), tbi->input);
   
   evas_object_show(teo);
   e_textedit_activate(teo);
}

void 
e_dialog_textblock_add(Evas_Object *obj, const char *label, const char*input, Evas_Coord size, void *data)
{
   E_Smart_Data *sd;
   E_TextBlock_Item *tbi;
   
   SMART_CHECK(obj, ;);
   tbi = calloc(1, sizeof(E_TextBlock_Item));
   tbi->obj = obj;
   if (label)
     tbi->label = strdup(label);
   else 
     tbi->label = strdup("");
   Evas_Object *text_object;
   text_object = evas_object_text_add( evas_object_evas_get(obj) ); 
   evas_object_text_text_set(text_object, label);
   evas_object_text_font_set(text_object, "Sans:style=Bold,Edje-Vera-Bold", 20);
   evas_object_text_glow_color_set(text_object, 255, 255, 255, 255);
   tbi->label_obj = text_object;
   evas_object_smart_member_add(tbi->label_obj, obj);
   evas_object_clip_set(tbi->label_obj, sd->clip);

   if (input)
     tbi->input = strdup(input);
   else
     tbi->input = strdup("");
   if(size < 30) size=30;
   if(size > 150) size=150;
   tbi->sz = size;
   tbi->data = data;
   tbi->item_obj = e_widget_textblock_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(tbi->item_obj, obj);
   evas_object_clip_set(tbi->item_obj, sd->clip);
   e_widget_textblock_plain_set(tbi->item_obj, input);
   e_widget_textblock_event_callback_add(tbi->item_obj, EVAS_CALLBACK_MOUSE_UP,
     				  _e_textblock_cb_mouse_up, tbi);
   
   sd->textblocks = evas_list_append(sd->textblocks, tbi);

}

static void
_e_dialog_update(Evas_Object *obj)
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
   int dialog_x = 0;
   int dialog_y = 0;
   int dialog_w = screen_w;
   int dialog_h = 0;

   double t;
   int tbc = evas_list_count(sd->textblocks);
   if (sd->activate_deactivate == 0)
     {
        if(tbc==0) 
          {
             dialog_y = screen_h* (1.0/3);
             dialog_h = screen_h * (1.0/3);
          }
        else 
          {
             dialog_y = screen_h* (1.0/6);
             dialog_h = screen_h * (2.0/3);
          }
     }
   else if (sd->activate_deactivate == 1)
     {
	t = ecore_time_get() - sd->activate_time;
	t = t / 1.0; /* anim time */
	if (t >= 1.0) t = 1.0;
	t = 1.0 - ((1.0 - t) * (1.0 - t)); /* decelerate */
	if (t >= 1.0) sd->activate_deactivate = 0;
        if(tbc==0) 
          {
             dialog_y = ((-screen_h) * (1.0/3)) + (t * screen_h * (1.0/3) * 2);
             dialog_h = screen_h * (1.0/3);
          }
        else 
          {
             dialog_y = ((-screen_h) * (1.0/6)) + (t * screen_h* (1.0/6) * 2);
             dialog_h = screen_h * (2.0/3);
          }
     }
   evas_object_move(sd->bg_object, dialog_x, dialog_y );
   evas_object_resize(sd->bg_object, dialog_w, dialog_h );
   evas_object_show(sd->bg_object);

   if(sd->title_object)
     {
        evas_object_resize(sd->title_object, dialog_w, dialog_h*(1.0/6));
        evas_object_move(sd->title_object, dialog_x, dialog_y );
        evas_object_show(sd->title_object);
     }

   int tmp_x, tmp_y;
   Evas_List *l;
   E_TextBlock_Item *tbi;
   int indent = 10;
   tmp_y =  dialog_y + dialog_h*(1.0/6) + (indent*2.5);
   
   for (l = sd->textblocks; l; l = l->next)
     {
        tbi = l->data;
        evas_object_move(tbi->label_obj, indent, tmp_y);
        evas_object_show(tbi->label_obj);
        tmp_y = tmp_y + (indent*2.5);
        e_widget_textblock_move(tbi->item_obj, indent, tmp_y);
        e_widget_textblock_resize(tbi->item_obj, (dialog_w-(indent*2)), tbi->sz);
        e_widget_textblock_show(tbi->item_obj);
        tmp_y = tmp_y + tbi->sz + indent;
     }
   tmp_y = tmp_y + (indent*2.5);
 
   int button_w, button_h; 
   int bc = evas_list_count(sd->buttons);
   if(bc==0) return;
   E_Button_Item *bi;
   tmp_x = dialog_x;
   button_w = (int)(dialog_w/bc);
   if(tbc==0) 
     {
        button_h = dialog_h*(1.0/2);
     }
   else 
     {
        button_h = dialog_h*(1.0/8);
     }
   //button_h = dialog_y + dialog_h - tmp_y;
   for (l = sd->buttons; l; l = l->next)
     {
        bi = l->data;
        if(tbc==0) 
          {
             evas_object_move(bi->item_obj, tmp_x, (dialog_y+dialog_h*(1.0/2)) );
          }
        else 
          {
             evas_object_move(bi->item_obj, tmp_x, (dialog_y+dialog_h*(7.0/8)) );
          }
        // evas_object_move(bi->item_obj, tmp_x, tmp_y );
        evas_object_resize(bi->item_obj, button_w, button_h);
        evas_object_show(bi->item_obj);
        tmp_x = tmp_x + button_w;
     }
}

static int
_e_dialog_cb_animator(void *data)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return 0;
 
   _e_dialog_update(sd->obj);
   if (sd->activate_deactivate == 0)
     {
	sd->animator = NULL;
	if (!sd->active) evas_object_del(sd->obj);
	return 0;
     }
   return 1;
}

static void
_e_dialog_cb_src_obj_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
}

static void
_e_dialog_cb_src_obj_move(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   _e_dialog_update(sd->obj);
}

static void
_e_dialog_cb_src_obj_resize(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   _e_dialog_update(sd->obj);
}
