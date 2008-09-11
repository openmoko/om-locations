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
#include "e_nav_entry.h"
#include "e_nav_button_bar.h"
#include "../e_nav_theme.h"
#include "../e_nav_misc.h"

/* navigator object */
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Button_Item E_Button_Item;
typedef struct _E_TextBlock_Item E_TextBlock_Item;

struct _E_TextBlock_Item
{
   Evas_Object *obj;
   Evas_Coord sz;
   char *label;
   char *input;
   Evas_Object *label_obj;
   Evas_Object *item_obj;
   size_t length_limit;
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

   int              type;
   const char      *group_base;

   /* the parent the dialog is transient for */
   Evas_Object     *parent;
   Evas_Object     *parent_mask;
   
   Evas_Object     *clip;

   Evas_List      *textblocks;
   Evas_Object    *bbar;

   E_Nav_Drop_Data *drop;
};

static void _e_nav_dialog_smart_init(void);
static void _e_nav_dialog_smart_add(Evas_Object *obj);
static void _e_nav_dialog_smart_del(Evas_Object *obj);
static void _e_nav_dialog_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_nav_dialog_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_nav_dialog_smart_show(Evas_Object *obj);
static void _e_nav_dialog_smart_hide(Evas_Object *obj);
static void _e_nav_dialog_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_nav_dialog_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_nav_dialog_smart_clip_unset(Evas_Object *obj);

static void _e_nav_dialog_update(Evas_Object *obj);
static void _e_nav_alert_update(Evas_Object *obj);

#define SMART_NAME "e_nav_dialog"
static Evas_Smart *_e_smart = NULL;

Evas_Object *
e_nav_dialog_add(Evas *e)
{
   _e_nav_dialog_smart_init();
   return evas_object_smart_add(e, _e_smart);
}

void
e_nav_dialog_type_set(Evas_Object *obj, int type)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   switch (type)
     {
      case E_NAV_DIALOG_TYPE_NORMAL:
	 sd->group_base = "modules/diversity_nav/dialog";
	 break;
      case E_NAV_DIALOG_TYPE_ALERT:
	 sd->group_base = "modules/diversity_nav/alert";
	 break;
      default:
	 return;
	 break;
     }

   sd->type = type;
}

static void
_e_nav_dialog_move_and_resize(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_Coord x, y, w, h;
   
   SMART_CHECK(obj, ;);

   if (sd->parent)
     {
	if (sd->type == E_NAV_DIALOG_TYPE_NORMAL)
	  {
	     evas_object_geometry_get(sd->parent, &x, &y, &w, &h);

	     evas_object_move(sd->parent_mask, x, y);
	     evas_object_resize(sd->parent_mask, w, h);

	     y = h / 8.0;

	     if (sd->textblocks)
	       h *= 5.0 / 8.0;
	     else
	       h /= 3.0;
	  }
	else
	  {
	     evas_object_geometry_get(sd->parent, &x, &y, &w, &h);

	     evas_object_move(sd->parent_mask, x, y);
	     evas_object_resize(sd->parent_mask, w, h);

	     y = h / 3.0;
	     h = h / 3.0;
	  }
     }
   else
     {
	x = sd->x;
	y = sd->y;
	w = sd->w;
	h = sd->h;
     }

   if (sd->drop)
     {
	e_nav_drop_apply(sd->drop, obj, x, y, w, h);
     }
   else if (sd->parent)
     {
	evas_object_move(obj, x, y);
	evas_object_resize(obj, w, h);
     }
}

static void
on_parent_del(void *data, Evas *evas, Evas_Object *parent, void *event)
{
   e_nav_dialog_transient_for_set(data, NULL);
}

void
e_nav_dialog_transient_for_set(Evas_Object *obj, Evas_Object *parent)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (sd->parent)
     {
	evas_object_event_callback_del(sd->parent, EVAS_CALLBACK_DEL,
	      on_parent_del);
	evas_object_event_callback_del(sd->parent, EVAS_CALLBACK_MOVE,
	      (void *) _e_nav_dialog_move_and_resize);
	evas_object_event_callback_del(sd->parent, EVAS_CALLBACK_RESIZE,
	      (void *) _e_nav_dialog_move_and_resize);
     }

   sd->parent = parent;
   if (sd->parent)
     {
	evas_object_event_callback_add(sd->parent, EVAS_CALLBACK_DEL,
	      on_parent_del, obj);
	evas_object_event_callback_add(sd->parent, EVAS_CALLBACK_MOVE,
	      (void *) _e_nav_dialog_move_and_resize, obj);
	evas_object_event_callback_add(sd->parent, EVAS_CALLBACK_RESIZE,
	      (void *) _e_nav_dialog_move_and_resize, obj);

	if (!sd->parent_mask)
	  {
	     sd->parent_mask = evas_object_rectangle_add(evas_object_evas_get(obj));

	     evas_object_smart_member_add(sd->parent_mask, sd->obj);
	     evas_object_clip_set(sd->parent_mask, sd->clip);
	     evas_object_lower(sd->parent_mask);
	     evas_object_color_set(sd->parent_mask, 0, 0, 0, 0);
	     evas_object_show(sd->parent_mask);
	  }
     }
   else if (sd->parent_mask)
     {
	evas_object_del(sd->parent_mask);
	sd->parent_mask = NULL;
     }

   _e_nav_dialog_move_and_resize(obj);
}

Evas_Object *
e_nav_dialog_transient_for_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, NULL;);

   return sd->parent;
}

static void
on_drop_done(void *data, Evas_Object *obj)
{
   E_Smart_Data *sd = data;

   evas_object_color_set(sd->bg_object, 0, 0, 0, 200);

   e_nav_drop_destroy(sd->drop);
   sd->drop = NULL;
}

void
e_nav_dialog_activate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (sd->drop)
     return;

   sd->drop = e_nav_drop_new(1.0, on_drop_done, sd);
   _e_nav_dialog_move_and_resize(obj);
}

void
e_nav_dialog_deactivate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (sd->drop)
     e_nav_drop_stop(sd->drop, FALSE);

   evas_object_del(obj);
}

/* internal calls */
static void
_e_nav_dialog_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
	       EVAS_SMART_CLASS_VERSION,
	       _e_nav_dialog_smart_add,
	       _e_nav_dialog_smart_del,
	       _e_nav_dialog_smart_move,
	       _e_nav_dialog_smart_resize,
	       _e_nav_dialog_smart_show,
	       _e_nav_dialog_smart_hide,
	       _e_nav_dialog_smart_color_set,
	       _e_nav_dialog_smart_clip_set,
	       _e_nav_dialog_smart_clip_unset,
	       
	       NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_theme_source_set(E_Smart_Data *sd)
{
   sd->bg_object = evas_object_rectangle_add(evas_object_evas_get(sd->obj)); 
   evas_object_smart_member_add(sd->bg_object, sd->obj);
   evas_object_move(sd->bg_object, sd->x, sd->y);
   evas_object_resize(sd->bg_object, sd->w, sd->h);
   evas_object_color_set(sd->bg_object, 0, 0, 0, 255);
   evas_object_clip_set(sd->bg_object, sd->clip);
   evas_object_repeat_events_set(sd->bg_object, 1);
   evas_object_show(sd->bg_object);
}

static void
_e_nav_dialog_smart_add(Evas_Object *obj)
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

   sd->title_color_r = 255;
   sd->title_color_g = 255;
   sd->title_color_b = 255;
   sd->title_color_a = 255;

   _theme_source_set(sd);

   evas_object_smart_data_set(obj, sd);

   e_nav_dialog_type_set(obj, E_NAV_DIALOG_TYPE_NORMAL);
}

static void
_e_nav_dialog_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   if (sd->parent)
     {
	evas_object_event_callback_del(sd->parent, EVAS_CALLBACK_DEL,
	      on_parent_del);
	evas_object_event_callback_del(sd->parent, EVAS_CALLBACK_MOVE,
	      (void *) _e_nav_dialog_move_and_resize);
	evas_object_event_callback_del(sd->parent, EVAS_CALLBACK_RESIZE,
	      (void *) _e_nav_dialog_move_and_resize);
     }

   if (sd->parent_mask)
     evas_object_del(sd->parent_mask);

   if (sd->drop)
     e_nav_drop_destroy(sd->drop);

   while (sd->textblocks)   
     {
	E_TextBlock_Item *tbi;
	
	tbi = sd->textblocks->data;
	sd->textblocks = evas_list_remove_list(sd->textblocks, sd->textblocks);
	evas_object_del(tbi->label_obj);
	evas_object_del(tbi->item_obj);
	if (tbi->label)
	  free(tbi->label);
	if (tbi->input)
	  free(tbi->input);

	free(tbi);
     }

   if (sd->bbar)
     evas_object_del(sd->bbar);

   if(sd->bg_object) evas_object_del(sd->bg_object);
   if(sd->title_object) evas_object_del(sd->title_object);

   evas_object_del(sd->clip);

   free(sd);
}
                    
static void
_e_nav_dialog_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;

   if (sd->type == E_NAV_DIALOG_TYPE_NORMAL)
     _e_nav_dialog_update(obj);
   else
     _e_nav_alert_update(obj);
}

static void
_e_nav_dialog_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;

   if (sd->type == E_NAV_DIALOG_TYPE_NORMAL)
     _e_nav_dialog_update(obj);
   else
     _e_nav_alert_update(obj);
}

static void
_e_nav_dialog_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_nav_dialog_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_nav_dialog_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_nav_dialog_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_nav_dialog_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
} 

void
e_nav_dialog_button_add(Evas_Object *obj, const char *label, void (*func) (void *data, Evas_Object *obj), void *data)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (!sd->bbar)
     {
	char group[256];

	snprintf(group, sizeof(group), "%s/%s", sd->group_base, "button_bar");

	sd->bbar = e_nav_button_bar_add(evas_object_evas_get(sd->obj));
	e_nav_button_bar_embed_set(sd->bbar, sd->obj, group);

	evas_object_smart_member_add(sd->bbar, sd->obj);
	evas_object_clip_set(sd->bbar, sd->clip);
	evas_object_show(sd->bbar);
     }

   e_nav_button_bar_button_add(sd->bbar, label, func, data);
}

void 
e_nav_dialog_title_set(Evas_Object *obj, const char *title, const char *message)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   if(!sd) return;
   if(!sd->title_object) 
     {
	Evas_Object *o;
	char group[256];

	snprintf(group, sizeof(group), "%s/%s", sd->group_base, "text");

	o = e_nav_theme_object_new(evas_object_evas_get(obj),
	      NULL, group);
	sd->title_object = o;
	edje_object_part_text_set(sd->title_object, "title", title);
	edje_object_part_text_set(sd->title_object, "message", message);

	evas_object_smart_member_add(sd->title_object, obj);
	evas_object_clip_set(sd->title_object, sd->clip);
	evas_object_show(sd->title_object);
     }
}

void
e_nav_dialog_title_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->title_color_r = r;
   sd->title_color_g = g;
   sd->title_color_b = b;
   sd->title_color_a = a;
}

static void
e_nav_dialog_textblock_text_set(void *obj, const char *input)
{
   E_TextBlock_Item *tbi = (E_TextBlock_Item*)obj;
   if(tbi->input) free((void*)tbi->input);
   if(input)
     tbi->input = strdup(input);
   else
     tbi->input = strdup("");
   edje_object_part_text_set(tbi->item_obj, "e.textblock.text", input);
}

const char *
e_nav_dialog_textblock_text_get(Evas_Object *obj, const char *label)
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
on_entry_ok(void *data, Evas_Object *entry)
{
   E_TextBlock_Item *tbi = data;
   E_Smart_Data *sd;
   const char *text;

   SMART_CHECK(tbi->obj, ;);

   text = e_nav_entry_text_get(entry);
   e_nav_dialog_textblock_text_set(tbi, text);

   evas_object_del(entry);
}

static void
on_entry_cancel(void *data, Evas_Object *entry)
{
   evas_object_del(entry);
}

static void
_e_textblock_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_TextBlock_Item *tbi = (E_TextBlock_Item*)data;
   Evas_Object *entry;
   Evas_Coord x, y, w, h;
  
   entry = e_nav_entry_add(evas);
   e_nav_entry_title_set(entry, edje_object_part_text_get(tbi->label_obj, "dialog.label.text"));
   e_nav_entry_text_set(entry, tbi->input);
   e_nav_entry_text_limit_set(entry, tbi->length_limit);
   
   e_nav_entry_button_add(entry, _("OK"), on_entry_ok, tbi);
   e_nav_entry_button_add(entry, _("Cancel"), on_entry_cancel, tbi);

   evas_output_viewport_get(evas, &x, &y, &w, &h);
   evas_object_move(entry, x, y);
   evas_object_resize(entry, w, h);

   e_nav_entry_focus(entry);

   evas_object_show(entry);
}

void 
e_nav_dialog_textblock_add(Evas_Object *obj, const char *label, const char*input, Evas_Coord size, size_t length_limit, void *data)
{
   E_Smart_Data *sd;
   Evas_Object *text_object;
   E_TextBlock_Item *tbi;
   char group[256];
   
   SMART_CHECK(obj, ;);
   tbi = calloc(1, sizeof(E_TextBlock_Item));
   tbi->obj = obj;
   if (label)
     tbi->label = strdup(label);
   else 
     tbi->label = strdup("");

   snprintf(group, sizeof(group), "%s/%s", sd->group_base, "label");

   text_object = e_nav_theme_object_new(evas_object_evas_get(obj),
	 NULL, group);
   edje_object_part_text_set(text_object, "dialog.label.text", label);

   tbi->label_obj = text_object;
   evas_object_smart_member_add(tbi->label_obj, obj);
   evas_object_clip_set(tbi->label_obj, sd->clip);
   evas_object_show(tbi->label_obj);

   if(length_limit > 0)
     tbi->length_limit = length_limit;

   if (input)
     tbi->input = strdup(input);
   else
     tbi->input = strdup("");
   if(size < 30) size=30;
   if(size > 150) size=150;
   tbi->sz = size;
   tbi->data = data;

   tbi->item_obj = e_nav_theme_object_new(evas_object_evas_get(obj), THEMEDIR,
                                          "e/widgets/textblock");
   evas_object_smart_member_add(tbi->item_obj, obj);
   evas_object_clip_set(tbi->item_obj, sd->clip);
   evas_object_show(tbi->item_obj);
   edje_object_part_text_set(tbi->item_obj, "e.textblock.text", input);
   evas_object_event_callback_add(tbi->item_obj, EVAS_CALLBACK_MOUSE_UP, 
                                  _e_textblock_cb_mouse_up, tbi);
   
   sd->textblocks = evas_list_append(sd->textblocks, tbi);
}

static void
textblocks_update(E_Smart_Data *sd, Evas_Coord tb_start, Evas_Coord tb_end)
{
   Evas_List *l;
   Evas_Coord gap_big, gap_small;
   Evas_Coord label_pad, label_indent, label_height;
   Evas_Coord item_indent, item_height;
   Evas_Coord y;

   if (!sd->textblocks)
     return;

   gap_big = 25;
   gap_small = 10;

   label_pad = 7;
   label_indent = 19;
   label_height = 25;

   item_indent = 10;
   item_height = 0;

   /* adjust layout */
   while (1)
     {
	Evas_Coord h, kill_me;

	h = gap_big;

	for (l = sd->textblocks; l; l = l->next)
	  {
	     E_TextBlock_Item *tbi = l->data;
	     Evas_Coord tbh = (item_height) ? item_height : tbi->sz;

	     h += label_height + tbh + gap_small;
	  }

	kill_me = tb_start + h - tb_end;
	if (kill_me <= 0)
	  break;

	if (gap_big > 15)
	  {
	     gap_big -= kill_me;
	     if (gap_big < 15)
	       gap_big = 15;
	  }
	else if (gap_small > 3)
	  {
	     gap_small -= kill_me;
	     if (gap_small < 3)
	       gap_small = 3;
	  }
	else if (!item_height)
	  {
	     item_height = 30;
	  }
	else /* give up */
	  {
	     item_height = 10;

	     break;
	  }
     }

   y = tb_start + gap_big;

   for (l = sd->textblocks; l; l = l->next)
     {
	E_TextBlock_Item *tbi = l->data;
	Evas_Coord tbh = (item_height) ? item_height : tbi->sz;

        evas_object_move(tbi->label_obj, sd->x + label_indent, y + label_pad);
        evas_object_show(tbi->label_obj);

	y += label_height;

        evas_object_move(tbi->item_obj, sd->x + item_indent, y);
	evas_object_resize(tbi->item_obj, sd->w - (item_indent * 2), tbh);
	evas_object_show(tbi->item_obj);

	y += tbh + gap_small;
     }
}

static void
_e_nav_dialog_update(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_Coord title_height, bbar_height, bottom_border;
   Evas_Coord tb_start, tb_end;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   evas_object_move(sd->bg_object, sd->x, sd->y);
   evas_object_resize(sd->bg_object, sd->w, sd->h);

   title_height = sd->h / 6;
   bottom_border = 10;
   bbar_height = 60;

   tb_start = sd->y + title_height;
   tb_end = sd->y + sd->h - bbar_height - bottom_border;

   if (sd->title_object)
     {
	evas_object_move(sd->title_object, sd->x, sd->y);
	evas_object_resize(sd->title_object, sd->w, title_height);
     }
  
   if (sd->textblocks)
     textblocks_update(sd, tb_start, tb_end);

   if (sd->bbar)
     {
	Evas_Coord button_w;
	Evas_Coord inter;
	int num_buttons;

	button_w = 158;
	num_buttons = e_nav_button_bar_num_buttons_get(sd->bbar);

	if (num_buttons == 2)
	  {
	     inter = sd->w - (button_w + 50) * 2;
	     e_nav_button_bar_paddings_set(sd->bbar, 50, inter, 50);
	  }
	else if (num_buttons > 1)
	  {
	     inter = (sd->w - (button_w * num_buttons)) / (num_buttons - 1);
	     e_nav_button_bar_paddings_set(sd->bbar, 0, inter, 0);
	  }

	evas_object_move(sd->bbar, sd->x, tb_end);
	evas_object_resize(sd->bbar, sd->w, bbar_height);
     }
}

static void
_e_nav_alert_update(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_Coord title_height, bbar_height;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   evas_object_move(sd->bg_object, sd->x, sd->y );
   evas_object_resize(sd->bg_object, sd->w, sd->h );

   title_height = sd->h * 3 / 7;
   bbar_height = sd->h * 2 / 7;

   if (sd->title_object)
     {
        Evas_Object *o;

	evas_object_move(sd->title_object, sd->x, sd->y);
	evas_object_resize(sd->title_object, sd->w, title_height);

	o = edje_object_part_object_get(sd->title_object, "title");
        evas_object_color_set(o,
	      sd->title_color_r,
	      sd->title_color_g,
	      sd->title_color_b,
	      sd->title_color_a);
     }

   if (sd->bbar)
     {
	e_nav_button_bar_paddings_set(sd->bbar, 20, 3, 20);

	evas_object_move(sd->bbar, sd->x, sd->y + title_height);
	evas_object_resize(sd->bbar, sd->w, bbar_height);
     }
}
