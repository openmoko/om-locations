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
#include "e_nav_theme.h"
#include <ewl/Ewl.h>

typedef struct _E_Button_Item E_Button_Item;
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _Embed_Canvas Embed_Canvas;
typedef struct _Candidate_List Candidate_List;
typedef struct _Candidate_List_Item Candidate_List_Item;

static char *prefix_filter=NULL;

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
   Ewl_Widget      *embed;
   Evas_Object     *embed_eo;

   Ewl_Widget      *vbox;
   Ewl_Widget      *entry;
   unsigned int     candidate_mode;
   Candidate_List  *candidate_list;
};

struct _Candidate_List
{
   Ewl_Widget      *scrollpane;
   Ewl_Widget      *list;  
   //char            *prefix_filter;
};

struct _Candidate_List_Item 
{
   char            *name;
   char            *description;
   void (*func) (void *data, void *data2);
   void            *data;
   void            *data2;
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

static Ewl_Widget *list_view_cb_widget_fetch(void *data, unsigned int row, unsigned int column);
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
   e_textedit_deactivate(obj);
}

static void
textedit_save(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *object = (Evas_Object*)obj;
   E_Smart_Data *sd;
   
   SMART_CHECK(object, ;);

   const char *text = ewl_text_text_get(EWL_TEXT(sd->embed->entry));
   e_dialog_textblock_text_set(sd->src_obj, text);
   e_textedit_deactivate(obj);
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
   if (bi->func) bi->func(bi->data, bi->obj, sd->src_obj);   // src_obj is location item object
}

static void
_e_nav_textedit_entry_cb_key_down(Ewl_Widget *w, void *ev, void *data) 	
{
   E_Smart_Data *sd;
   Evas_Object *obj = (Evas_Object *)data;
   SMART_CHECK(obj, ;);
   const char *filter = ewl_text_text_get(EWL_TEXT(w));
   if(filter==NULL) return;
   prefix_filter = strdup(filter);
}


static Ewl_Widget *
list_view_cb_widget_fetch(void *data, unsigned int row, unsigned int column)
{
   Textedit_List_Item *tli;
   Ewl_Widget *w = NULL;
   w = ewl_label_new();
   tli = (Textedit_List_Item *)data;
   ewl_label_text_set(EWL_LABEL(w), tli->name);
   return w;
#if 0
   Ewl_Widget *w = NULL;
   int prefix_size = 0;
   if(prefix_filter) prefix_size = strlen(prefix_filter);
   switch (column) {
      case 0:
         if(prefix_filter==NULL || 
            prefix_size==0 || 
            !strncmp((const char *)data, prefix_filter, prefix_size))
           {
              w = ewl_label_new();
              ewl_object_custom_h_set(EWL_OBJECT(w), 30);
              ewl_label_text_set(EWL_LABEL(w), (char *)data);
              ewl_widget_show(w);
           }

         break;
   }
   return w;
#endif
}


static void
list_cb_value_changed(Ewl_Widget *w, void *ev, void *data)
{
   Ewl_List *list;
   Ecore_List *el;
   Ewl_Selection_Idx *idx;
   E_Smart_Data *sd;
   Evas_Object *obj;
   Textedit_List_Item *tli;
        
   obj = (Evas_Object *)data;
   SMART_CHECK(obj, ;);

   list = EWL_LIST(w);
   el = ewl_mvc_data_get(EWL_MVC(list));
   idx = ewl_mvc_selected_get(EWL_MVC(list));

   ecore_list_index_goto(el, idx->row);
   tli = (Textedit_List_Item *) ecore_list_current(el);
   ewl_text_text_set(EWL_TEXT(sd->embed->entry), tli->name);
}

void
e_textedit_theme_source_set(Evas_Object *obj, const char *custom_dir, void (*positive_func)(void *data, Evas_Object *obj, Evas_Object *src_obj), void *data1, void (*negative_func)(void *data, Evas_Object *obj, Evas_Object *src_obj), void *data2)
{
   Ecore_Evas *ee;
   Evas *evas;
   Ewl_Model  *model;
   Ewl_View   *view;
   Ecore_List *data;
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

   Embed_Canvas * ec = (Embed_Canvas *)malloc (sizeof(Embed_Canvas));
   memset(ec, 0, sizeof(Embed_Canvas)); 

   ec->frame = e_nav_theme_object_new(evas_object_evas_get(obj), custom_dir, "modules/diversity_nav/textedit"); 
   evas_object_smart_member_add(ec->frame, obj);
   evas_object_clip_set(ec->frame, sd->clip);

   /*
    * Setup the ewl embed
    */
   ec->embed = ewl_embed_new();
   ewl_object_fill_policy_set(EWL_OBJECT(ec->embed), EWL_FLAG_FILL_ALL);
   evas = evas_object_evas_get(obj);
   ee = ecore_evas_ecore_evas_get(evas);
   ec->embed_eo = ewl_embed_canvas_set(EWL_EMBED(ec->embed), evas,
                     (void *) ecore_evas_software_x11_window_get(ee));   
   evas_object_smart_member_add(ec->embed_eo, obj);
   evas_object_clip_set(ec->embed_eo, sd->clip);

   /*
    * swallow it into the edje
    */
   edje_object_part_swallow(ec->frame, "swallow", ec->embed_eo);

   ec->vbox = ewl_vbox_new();
   ewl_container_child_append(EWL_CONTAINER(ec->embed), ec->vbox);

   ec->entry = ewl_entry_new();
   ewl_entry_editable_set(EWL_ENTRY(ec->entry), TRUE);  
   ewl_container_child_append(EWL_CONTAINER(ec->vbox), ec->entry);

   /*
    * fill it with content
    */
   Candidate_List *cl = (Candidate_List *)malloc (sizeof(Candidate_List));
   memset(cl, 0, sizeof(Candidate_List)); 
   cl->scrollpane = ewl_scrollpane_new();
   ewl_scrollpane_kinetic_scrolling_set(EWL_SCROLLPANE(cl->scrollpane), EWL_KINETIC_SCROLL_EMBEDDED);
   ewl_scrollpane_hscrollbar_flag_set(EWL_SCROLLPANE(cl->scrollpane),
                                        EWL_SCROLLPANE_FLAG_ALWAYS_HIDDEN);
   ewl_scrollpane_vscrollbar_flag_set(EWL_SCROLLPANE(cl->scrollpane),
                                        EWL_SCROLLPANE_FLAG_ALWAYS_HIDDEN);
   ewl_container_child_append(EWL_CONTAINER(ec->vbox), cl->scrollpane);

   data = ecore_list_new();

   model = ewl_model_ecore_list_get();
   view = ewl_label_view_get();
   ewl_view_widget_fetch_set(view, list_view_cb_widget_fetch);

   cl->list = ewl_list_new();
   ewl_container_child_append(EWL_CONTAINER(cl->scrollpane), cl->list);
   ewl_box_orientation_set(EWL_BOX(cl->list), EWL_ORIENTATION_HORIZONTAL);
   ewl_mvc_model_set(EWL_MVC(cl->list), model);
   ewl_mvc_view_set(EWL_MVC(cl->list), view);
   ewl_mvc_data_set(EWL_MVC(cl->list), data);
   ewl_object_alignment_set(EWL_OBJECT(cl->list),
                            EWL_FLAG_ALIGN_BOTTOM);
   ewl_callback_append(cl->list, EWL_CALLBACK_VALUE_CHANGED,
                                   list_cb_value_changed, obj);

   sd->embed = ec;
   sd->embed->candidate_list = cl;
   sd->embed->candidate_mode = TEXTEDIT_CANDIDATE_MODE_FALSE;
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

   evas_object_del(sd->clip);
   evas_object_del(sd->event);

   if(sd->embed->candidate_list)
     {
        ewl_widget_destroy(sd->embed->candidate_list->list);
        ewl_widget_destroy(sd->embed->candidate_list->scrollpane);
        ewl_widget_destroy(sd->embed->entry);
        ewl_widget_destroy(sd->embed->vbox);
        ewl_widget_destroy(sd->embed->embed);
        evas_object_del(sd->embed->embed_eo);
        evas_object_del(sd->embed->frame);
     }
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

   evas_object_hide(sd->embed->frame);
   evas_object_hide(sd->embed->embed_eo);
   ewl_widget_hide(sd->embed->embed);
   ewl_widget_hide(sd->embed->vbox);
   ewl_widget_hide(sd->embed->entry);
   ewl_widget_hide(sd->embed->candidate_list->scrollpane);
   ewl_widget_hide(sd->embed->candidate_list->list);
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

   if(sd->embed)
     {
        edje_object_part_text_set(sd->embed->frame, "title", name);
        if(!input || !strcmp(input, "")) return;
        ewl_text_text_set(EWL_TEXT(sd->embed->entry), input);
     }
}

const char * 
e_textedit_input_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
   if(!sd) return NULL;
   return ewl_text_text_get(EWL_TEXT(sd->embed->entry));
}

void
e_textedit_candidate_mode_set(Evas_Object *obj, unsigned int candidate_mode)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   switch(candidate_mode)
     {
        case TEXTEDIT_CANDIDATE_MODE_NONE:
        case TEXTEDIT_CANDIDATE_MODE_FALSE:
        case TEXTEDIT_CANDIDATE_MODE_TRUE:
          sd->embed->candidate_mode = candidate_mode;
          ewl_callback_append(EWL_WIDGET(sd->embed->entry), EWL_CALLBACK_KEY_DOWN,
                              _e_nav_textedit_entry_cb_key_down, obj);
        default:
          return;
     }
}

unsigned int
e_textedit_candidate_mode_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, TEXTEDIT_CANDIDATE_MODE_NONE;);
   return sd->embed->candidate_mode;
}

void 
e_textedit_candidate_list_set(Evas_Object *obj, Ecore_List *list)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   if(!sd->embed->candidate_mode == TEXTEDIT_CANDIDATE_MODE_TRUE) return;
   Ecore_List *cl = ewl_mvc_data_get(EWL_MVC(sd->embed->candidate_list->list));   
   ecore_list_append_list(cl, list);
}

Ecore_List *
e_textedit_candidate_list_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, NULL;);
   if(!sd->embed->candidate_mode == TEXTEDIT_CANDIDATE_MODE_TRUE) 
     return NULL;
   return ewl_mvc_data_get(EWL_MVC(sd->embed->candidate_list->list));   
}

void *
e_textedit_list_selected_get(Evas_Object *obj)
{
   Ecore_List *cl;
   E_Smart_Data *sd;
   SMART_CHECK(obj, NULL;);

   if(!sd->embed->candidate_mode == TEXTEDIT_CANDIDATE_MODE_TRUE) 
     return NULL;

   cl = ewl_mvc_data_get(EWL_MVC(sd->embed->candidate_list->list));   
   return ecore_list_current(cl);   
}

void *
e_textedit_list_item_get_by_name(Evas_Object *obj, const char *name)
{
   Ecore_List *cl;
   int lstcount;
   int n;
   Textedit_List_Item *item;
   E_Smart_Data *sd;
   SMART_CHECK(obj, NULL;);

   if(!sd->embed->candidate_mode == TEXTEDIT_CANDIDATE_MODE_TRUE) 
     return NULL;

   cl = ewl_mvc_data_get(EWL_MVC(sd->embed->candidate_list->list));   
   lstcount = ecore_list_count(cl);
   for(n=0; n<lstcount; n++)
     {
        item =  ecore_list_index_goto(cl, n); 
        if(item && !strcmp(item->name, name))
          return item;
     }
   return NULL;   
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

   evas_object_move(sd->embed->frame, indent, (screen_h*(1.0/6)) + indent*2);
   evas_object_show(sd->embed->frame);
   evas_object_move(sd->embed->embed_eo, indent, (screen_h*(1.0/6)) + indent*2 + 30 + indent*2);
   evas_object_resize(sd->embed->embed_eo, screen_w-(indent*2), screen_h*(1.0/3));
   evas_object_show(sd->embed->embed_eo);
   ewl_widget_show(sd->embed->embed);
   ewl_widget_show(sd->embed->vbox);
   ewl_widget_show(sd->embed->entry);
   if(sd->embed->candidate_mode == TEXTEDIT_CANDIDATE_MODE_TRUE)
     {
        ewl_widget_show(sd->embed->candidate_list->scrollpane);
        ewl_widget_show(sd->embed->candidate_list->list);
     }
   ewl_widget_focus_send(EWL_WIDGET(sd->embed->entry));
}

