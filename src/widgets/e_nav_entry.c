/* e_nav_entry.c -
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

#include <Etk.h>
#include "e_nav_entry.h"
#include "e_nav_button_bar.h"
#include "../e_nav.h"
#include "../e_nav_theme.h"
#include "../e_nav_misc.h"

typedef struct _E_Smart_Data E_Smart_Data;

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *clip;

   Evas_Object     *frame;

   Etk_Widget      *embed;
   Etk_Widget      *entry;

   Evas_Object     *top_bbar;
   Evas_Object     *bottom_bbar;
};

static void _e_nav_entry_smart_init(void);
static void _e_nav_entry_smart_add(Evas_Object *obj);
static void _e_nav_entry_smart_del(Evas_Object *obj);
static void _e_nav_entry_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_nav_entry_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_nav_entry_smart_show(Evas_Object *obj);
static void _e_nav_entry_smart_hide(Evas_Object *obj);
static void _e_nav_entry_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_nav_entry_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_nav_entry_smart_clip_unset(Evas_Object *obj);

#define SMART_NAME "e_nav_entry"
static Evas_Smart *_e_smart = NULL;

Evas_Object *
e_nav_entry_add(Evas *e)
{
   _e_nav_entry_smart_init();

   return evas_object_smart_add(e, _e_smart);
}

static Etk_Bool
on_entry_focused(Etk_Object *entry, void *data)
{
   e_misc_keyboard_launch();

   return TRUE;
}

static Etk_Bool
on_entry_unfocused(Etk_Object *entry, void *data)
{
   e_misc_keyboard_hide();

   return TRUE;
}

static void
on_frame_hide(void *data, Evas *e, Evas_Object *obj, void *event)
{
   e_misc_keyboard_hide();
}

void
e_nav_entry_button_add(Evas_Object *entry, const char *label, void (*func)(void *data, Evas_Object *entry), void *data)
{
   E_Smart_Data *sd;
   int num_buttons;;

   SMART_CHECK(entry, ;);

   /* this is weird... */
   num_buttons = e_nav_button_bar_num_buttons_get(sd->top_bbar);
   switch (num_buttons)
     {
      case 0:
	 e_nav_button_bar_button_add(sd->top_bbar, label, func, data);
	 break;
      case 1:
	 e_nav_button_bar_button_add_back(sd->top_bbar, label, func, data);
	 break;
      default:
	 e_nav_button_bar_button_add(sd->bottom_bbar, label, func, data);
	 break;
     }

   if (e_nav_button_bar_num_buttons_get(sd->bottom_bbar))
     edje_object_signal_emit(sd->frame, "e,state,active", "e");
}

void
e_nav_entry_button_remove(Evas_Object *entry, void (*func)(void *data, Evas_Object *entry), void *data)
{
   E_Smart_Data *sd;

   SMART_CHECK(entry, ;);

   /* try both */
   e_nav_button_bar_button_remove(sd->top_bbar, func, data);
   e_nav_button_bar_button_remove(sd->bottom_bbar, func, data);

   if (!e_nav_button_bar_num_buttons_get(sd->bottom_bbar))
     edje_object_signal_emit(sd->frame, "e,state,passive", "e");
}

void
e_nav_entry_title_set(Evas_Object *entry, const char *title)
{
   E_Smart_Data *sd;

   SMART_CHECK(entry, ;);

   edje_object_part_text_set(sd->frame, "title", title);
}

const char *
e_nav_entry_title_get(Evas_Object *entry)
{
   E_Smart_Data *sd;

   SMART_CHECK(entry, NULL;);

   return edje_object_part_text_get(sd->frame, "title");
}

void
e_nav_entry_text_set(Evas_Object *entry, const char *text)
{
   E_Smart_Data *sd;

   SMART_CHECK(entry, ;);

   if (!text)
     text = "";

   etk_entry_text_set(ETK_ENTRY(sd->entry), text);
}

const char *
e_nav_entry_text_get(Evas_Object *entry)
{
   E_Smart_Data *sd;

   SMART_CHECK(entry, NULL;);

   return etk_entry_text_get(ETK_ENTRY(sd->entry));
}

void
e_nav_entry_text_limit_set(Evas_Object *entry, size_t limit)
{
   E_Smart_Data *sd;

   SMART_CHECK(entry, ;);

   etk_entry_text_limit_set(ETK_ENTRY(sd->entry), limit);
}

size_t
e_nav_entry_text_limit_get(Evas_Object *entry)
{
   E_Smart_Data *sd;

   SMART_CHECK(entry, 0;);

   return etk_entry_text_limit_get(ETK_ENTRY(sd->entry));
}

void
e_nav_entry_focus(Evas_Object *entry)
{
   E_Smart_Data *sd;

   SMART_CHECK(entry, ;);

   if (!etk_widget_is_focused(sd->entry))
     etk_widget_focus(sd->entry);
   else
     e_misc_keyboard_launch();
}

void
e_nav_entry_unfocus(Evas_Object *entry)
{
   E_Smart_Data *sd;

   SMART_CHECK(entry, ;);

   if (etk_widget_is_focused(sd->entry))
     etk_widget_unfocus(sd->entry);
   else
     e_misc_keyboard_hide();
}

/* internal calls */
static void
_e_nav_entry_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
	       EVAS_SMART_CLASS_VERSION,
	       _e_nav_entry_smart_add,
	       _e_nav_entry_smart_del,
	       _e_nav_entry_smart_move,
	       _e_nav_entry_smart_resize,
	       _e_nav_entry_smart_show,
	       _e_nav_entry_smart_hide,
	       _e_nav_entry_smart_color_set,
	       _e_nav_entry_smart_clip_set,
	       _e_nav_entry_smart_clip_unset,
	
	       NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_theme_source_set(E_Smart_Data *sd)
{
   evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_HIDE, on_frame_hide, sd);

   sd->frame = e_nav_theme_object_new(evas_object_evas_get(sd->obj),
	 NULL, "modules/diversity_nav/entry");
   evas_object_move(sd->frame, sd->x, sd->y);
   evas_object_resize(sd->frame, sd->w, sd->h);
   evas_object_smart_member_add(sd->frame, sd->obj);
   evas_object_clip_set(sd->frame, sd->clip);
   evas_object_show(sd->frame);

   sd->top_bbar = e_nav_button_bar_add(evas_object_evas_get(sd->obj));
   e_nav_button_bar_embed_set(sd->top_bbar, sd->obj,
	 "modules/diversity_nav/entry/button_bar");
   e_nav_button_bar_style_set(sd->top_bbar, E_NAV_BUTTON_BAR_STYLE_JUSTIFIED);
   e_nav_button_bar_button_size_request(sd->top_bbar, 160, 0);
   edje_object_part_swallow(sd->frame, "button_bar.top", sd->top_bbar);

   sd->bottom_bbar = e_nav_button_bar_add(evas_object_evas_get(sd->obj));
   e_nav_button_bar_embed_set(sd->bottom_bbar, sd->obj,
	 "modules/diversity_nav/entry/button_bar_bottom");
   e_nav_button_bar_style_set(sd->bottom_bbar, E_NAV_BUTTON_BAR_STYLE_CENTERED);
   edje_object_part_swallow(sd->frame, "button_bar.bottom", sd->bottom_bbar);

   sd->entry = etk_entry_new();
   etk_signal_connect_by_code(ETK_WIDGET_FOCUSED_SIGNAL, ETK_OBJECT(sd->entry), ETK_CALLBACK(on_entry_focused), sd);
   etk_signal_connect_by_code(ETK_WIDGET_UNFOCUSED_SIGNAL, ETK_OBJECT(sd->entry), ETK_CALLBACK(on_entry_unfocused), sd);

   sd->embed  = etk_embed_new(evas_object_evas_get(sd->obj));
   etk_container_add(ETK_CONTAINER(sd->embed), sd->entry);
   edje_object_part_swallow(sd->frame, "swallow", etk_embed_object_get(ETK_EMBED(sd->embed)));
   etk_widget_show_all(ETK_WIDGET(sd->embed));
}

static void
_e_nav_entry_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd)
     return;

   sd->obj = obj;

   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;

   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);

   _theme_source_set(sd);

   evas_object_smart_data_set(obj, sd);
}

static void
_e_nav_entry_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_del(sd->top_bbar);
   evas_object_del(sd->bottom_bbar);
   evas_object_del(sd->frame);

   etk_object_destroy(ETK_OBJECT(sd->embed));

   evas_object_del(sd->clip);

   free(sd);
}


static void
_e_nav_entry_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->x = x;
   sd->y = y;

   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_move(sd->frame, sd->x, sd->y);
}

static void
_e_nav_entry_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->w = w;
   sd->h = h;

   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_resize(sd->frame, sd->w, sd->h);
}

static void
_e_nav_entry_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_show(sd->clip);
}

static void
_e_nav_entry_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_hide(sd->clip);
}

static void
_e_nav_entry_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_nav_entry_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_clip_set(sd->clip, clip);
}

static void
_e_nav_entry_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_clip_unset(sd->clip);
}
