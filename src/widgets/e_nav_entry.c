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
#include "../e_nav.h"
#include "../e_nav_theme.h"
#include "../e_nav_misc.h"

typedef struct _E_Smart_Data E_Smart_Data;

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   char            *dir;
   Evas_Object     *obj;
   Evas_Object     *clip;

   Evas_Object     *frame;

   Etk_Widget      *embed;
   Etk_Widget      *entry;

   /* button 0, 1, 2 corresponds to button on the left, right,
    * and bottom respectively */
   struct {
	void      (*cb)(void *data, Evas_Object *entry);
	void       *data;
   } buttons[3];
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

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_nav_entry")) return ret

Evas_Object *
e_nav_entry_add(Evas *e)
{
   _e_nav_entry_smart_init();

   return evas_object_smart_add(e, _e_smart);
}

static void
on_button_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd = data;
   char *p;
   int i;

   p = strchr(source, '.');
   if (!p)
     return;
   p++;

   switch (*p)
     {
      case 'l': /* left */
	 i = 0;
	 break;
      case 'r': /* right */
	 i = 1;
	 break;
      case 'b': /* bottom */
	 i = 2;
	 break;
      default:
	 printf("unknown button %s clicked\n", p);
	 
	 return;
	 break;
     }

   if (sd->buttons[i].cb)
     sd->buttons[i].cb(sd->buttons[i].data, sd->obj);
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
e_nav_entry_theme_source_set(Evas_Object *entry, const char *custom_dir)
{
   E_Smart_Data *sd;

   SMART_CHECK(entry, ;);

   evas_object_event_callback_add(entry, EVAS_CALLBACK_HIDE, on_frame_hide, sd);

   if (custom_dir)
     sd->dir = strdup(custom_dir);

   sd->frame = e_nav_theme_object_new(evas_object_evas_get(entry),
	 custom_dir, "modules/diversity_nav/entry");
   evas_object_move(sd->frame, sd->x, sd->y);
   evas_object_resize(sd->frame, sd->w, sd->h);
   evas_object_smart_member_add(sd->frame, sd->obj);
   evas_object_clip_set(sd->frame, sd->clip);
   evas_object_show(sd->frame);

   edje_object_signal_callback_add(sd->frame, "mouse,clicked,*", "button.*", on_button_clicked, sd);

   sd->entry = etk_entry_new();
   etk_signal_connect_by_code(ETK_WIDGET_FOCUSED_SIGNAL, ETK_OBJECT(sd->entry), ETK_CALLBACK(on_entry_focused), sd);
   etk_signal_connect_by_code(ETK_WIDGET_UNFOCUSED_SIGNAL, ETK_OBJECT(sd->entry), ETK_CALLBACK(on_entry_unfocused), sd);

   sd->embed  = etk_embed_new(evas_object_evas_get(entry));
   etk_container_add(ETK_CONTAINER(sd->embed), sd->entry);
   edje_object_part_swallow(sd->frame, "swallow", etk_embed_object_get(ETK_EMBED(sd->embed)));
   etk_widget_show_all(ETK_WIDGET(sd->embed));
}

void
e_nav_entry_button_add(Evas_Object *entry, const char *label, void (*func)(void *data, Evas_Object *entry), void *data)
{
   E_Smart_Data *sd;
   int i;

   SMART_CHECK(entry, ;);

   /* XXX do we want to have a real button bar? */
   for (i = 0; i < 3; i++)
     {
	if (!sd->buttons[i].cb)
	  {
	     const char *part;

	     sd->buttons[i].cb = func;
	     sd->buttons[i].data = data;

	     switch (i)
	       {
		case 0:
		   part = "button.left";
		   break;
		case 1:
		   part = "button.right";
		   break;
		case 2:
		   edje_object_signal_emit(sd->frame, "e,state,active", "e");
		   part = "button.bottom";
		   break;
	       }

	     edje_object_part_text_set(sd->frame, part, label);

	     break;
	  }
     }
}

void
e_nav_entry_button_remove(Evas_Object *entry, void (*func)(void *data, Evas_Object *entry), void *data)
{
   E_Smart_Data *sd;
   int i;

   SMART_CHECK(entry, ;);

   for (i = 0; i < 3; i++)
     {
	if (sd->buttons[i].cb == func && sd->buttons[i].data == data)
	  {
	     sd->buttons[i].cb = NULL;
	     sd->buttons[i].data = NULL;

	     if (i == 2)
	       edje_object_signal_emit(sd->frame, "e,state,passive", "e");

	     break;
	  }
     }
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
	     "e_nav_entry",
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

   evas_object_smart_data_set(obj, sd);
}

static void
_e_nav_entry_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (sd->dir)
     free(sd->dir);

   evas_object_del(sd->clip);
   evas_object_del(sd->frame);

   etk_object_destroy(ETK_OBJECT(sd->embed));

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
