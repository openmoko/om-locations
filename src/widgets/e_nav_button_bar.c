/* e_nav_button_bar.c -
 *
 * Copyright 2008 OpenMoko, Inc.
 * Authored by Chia-I Wu <olv@openmoko.com>
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

#include <Ecore.h>
#include <Evas.h>
#include <Edje.h>
#include <stdlib.h>
#include <string.h>
#include "e_nav_button_bar.h"
#include "../e_nav.h"
#include "../e_nav_theme.h"

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _Button_Data Button_Data;

struct _E_Smart_Data
{
   Evas_Object *obj;
   Evas_Coord x, y, w, h;

   Evas_Object *clip;

   unsigned int serial_number;

   Evas_Object *embedding;

   char *group_base;
   Evas_Object *bg;

   int style;

   Evas_Coord pad_front;
   Evas_Coord pad_inter;
   Evas_Coord pad_back;

   Evas_List *buttons;
};

struct _Button_Data {
     unsigned int id;
     Evas_Object *bbar;

     Evas_Object *obj;
     Evas_Object *pad;

     void (*cb)(void *data, Evas_Object *bbar);
     void  *cb_data;
};

static void _e_nav_button_bar_smart_init(void);
static void _e_nav_button_bar_smart_add(Evas_Object *obj);
static void _e_nav_button_bar_smart_del(Evas_Object *obj);
static void _e_nav_button_bar_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_nav_button_bar_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_nav_button_bar_smart_show(Evas_Object *obj);
static void _e_nav_button_bar_smart_hide(Evas_Object *obj);
static void _e_nav_button_bar_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_nav_button_bar_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_nav_button_bar_smart_clip_unset(Evas_Object *obj);

static void _e_nav_button_bar_update(Evas_Object *bbar);

#define SMART_NAME "e_nav_button_bar"
static Evas_Smart *_e_smart = NULL;

Evas_Object *e_nav_button_bar_add(Evas *e)
{
   _e_nav_button_bar_smart_init();

   return evas_object_smart_add(e, _e_smart);
}

static Evas_Object *
obj_new(Evas *e, const char *base, const char *group)
{
   Evas_Object *obj;
   char buf[256];

   snprintf(buf, sizeof(buf), "%s/%s", base, group);
   if (!e_nav_theme_group_exist(NULL, buf))
     return NULL;

   obj = e_nav_theme_object_new(e, NULL, buf);

   return obj;
}

void
e_nav_button_bar_embed_set(Evas_Object *bbar, Evas_Object *embedding, const char *group_base)
{
   E_Smart_Data *sd;

   SMART_CHECK(bbar, 0;);

   sd->embedding = embedding;

   if (sd->group_base)
     {
	free(sd->group_base);

	if (sd->bg)
	  evas_object_del(sd->bg);
     }

   if (group_base)
     {
	sd->group_base = strdup(group_base);

	sd->bg = obj_new(evas_object_evas_get(bbar), sd->group_base, "bg");
	if (sd->bg)
	  {
	     evas_object_smart_member_add(sd->bg, sd->obj);
	     evas_object_clip_set(sd->bg, sd->clip);
	     evas_object_move(sd->bg, sd->x, sd->y);
	     evas_object_resize(sd->bg, sd->w, sd->h);
	     evas_object_show(sd->bg);
	  }
     }
   else
     {
	sd->group_base = NULL;
	sd->bg = NULL;
     }
}

void
e_nav_button_bar_style_set(Evas_Object *bbar, int style)
{
   E_Smart_Data *sd;

   SMART_CHECK(bbar, ;);

   if (style < 0 || style >= N_E_NAV_BUTTON_BAR_STYLES)
     return;

   if (sd->style != style)
     {
	sd->style = style;

	_e_nav_button_bar_update(bbar);
     }
}

void
e_nav_button_bar_paddings_set(Evas_Object *bbar, Evas_Coord front, Evas_Coord inter, Evas_Coord back)
{
   E_Smart_Data *sd;

   SMART_CHECK(bbar, ;);

   if (sd->pad_front != front ||
       sd->pad_inter != inter ||
       sd->pad_back != back)
     {
	sd->pad_front = front;
	sd->pad_inter = inter;
	sd->pad_back = back;

	_e_nav_button_bar_update(bbar);
     }
}

int
e_nav_button_bar_num_buttons_get(Evas_Object *bbar)
{
   E_Smart_Data *sd;

   SMART_CHECK(bbar, 0;);

   return evas_list_count(sd->buttons);
}

static Button_Data *
button_new(Evas_Object *bbar, const char *label, void *cb, void *cb_data)
{
   E_Smart_Data *sd;
   Button_Data *bdata;

   SMART_CHECK(bbar, NULL;);

   bdata = calloc(1, sizeof(*bdata));
   if (!bdata)
     return NULL;

   bdata->id = ++sd->serial_number;

   bdata->bbar = bbar;
   bdata->cb = cb;
   bdata->cb_data = cb_data;

   bdata->obj = obj_new(evas_object_evas_get(bbar), sd->group_base, "button");
   if (bdata->obj)
     {
	evas_object_smart_member_add(bdata->obj, sd->obj);
	evas_object_clip_set(bdata->obj, sd->clip);
	evas_object_show(bdata->obj);

	edje_object_part_text_set(bdata->obj, "button.text", label);
     }

   return bdata;
}

static void
button_destroy(Button_Data *bdata)
{
   if (bdata->pad)
     evas_object_del(bdata->pad);

   if (bdata->obj)
     evas_object_del(bdata->obj);

   free(bdata);
}

static void
on_button_clicked(void *data, Evas_Object *button, const char *emission, const char *source)
{
   Button_Data *bdata = data;
   E_Smart_Data *sd;

   SMART_CHECK(bdata->bbar, ;);

   if (bdata->cb)
     bdata->cb(bdata->cb_data, (sd->embedding) ? sd->embedding : bdata->bbar);
}

void
e_nav_button_bar_button_add(Evas_Object *bbar, const char *label, void (*func)(void *data, Evas_Object *bbar), void *data)
{
   E_Smart_Data *sd;
   Button_Data *bdata;

   SMART_CHECK(bbar, ;);

   bdata = button_new(bbar, label, func, data);
   if (!bdata)
     return;

   edje_object_signal_callback_add(bdata->obj,
        "mouse,clicked,*", "button.*", on_button_clicked, bdata);

   if (sd->buttons)
     {
	Button_Data *prev = sd->buttons->data;

	prev->pad = obj_new(evas_object_evas_get(bbar),
	      sd->group_base, "pad");
	if (prev->pad)
	  {
	     evas_object_smart_member_add(prev->pad, sd->obj);
	     evas_object_clip_set(prev->pad, sd->clip);
	     evas_object_show(prev->pad);
	  }
     }

   sd->buttons = evas_list_prepend(sd->buttons, bdata);

   _e_nav_button_bar_update(bbar);
}

void
e_nav_button_bar_button_remove(Evas_Object *bbar, void (*func)(void *data, Evas_Object *bbar), void *data)
{
   E_Smart_Data *sd;
   Button_Data *bdata;
   Evas_List *l;
   int last;

   SMART_CHECK(bbar, ;);

   for (l = sd->buttons; l; l = l->next)
     {
	bdata = l->data;

	if (bdata->cb == func && bdata->cb_data == data)
	  break;
     }
   if (!l)
     return;

   last = (sd->buttons == l);

   sd->buttons = evas_list_remove_list(sd->buttons, l);

   button_destroy(bdata);

   if (last)
     {
	bdata = sd->buttons->data;

	if (bdata->pad)
	  {
	     evas_object_del(bdata->pad);
	     bdata->pad = NULL;
	  }
     }

   _e_nav_button_bar_update(bbar);
}

/* internal calls */
static void
_e_nav_button_bar_smart_init(void)
{
   if (!_e_smart)
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
	     EVAS_SMART_CLASS_VERSION,
	     _e_nav_button_bar_smart_add,
	     _e_nav_button_bar_smart_del,
	     _e_nav_button_bar_smart_move,
	     _e_nav_button_bar_smart_resize,
	     _e_nav_button_bar_smart_show,
	     _e_nav_button_bar_smart_hide,
	     _e_nav_button_bar_smart_color_set,
	     _e_nav_button_bar_smart_clip_set,
	     _e_nav_button_bar_smart_clip_unset,

	     NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_nav_button_bar_smart_add(Evas_Object *obj)
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
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_resize(sd->clip, sd->w, sd->h);

   evas_object_smart_data_set(obj, sd);
}

static void
_e_nav_button_bar_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   while (sd->buttons)
     {
	button_destroy(sd->buttons->data);
	sd->buttons = 
	   evas_list_remove_list(sd->buttons, sd->buttons);
     }

   if (sd->bg)
     evas_object_del(sd->bg);

   if (sd->group_base)
     free(sd->group_base);

   evas_object_del(sd->clip);

   free(sd);
}

static void
_e_nav_button_bar_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   sd->x = x;
   sd->y = y;

   if (sd->bg)
     evas_object_move(sd->bg, sd->x, sd->y);

   evas_object_move(sd->clip, sd->x, sd->y);

   _e_nav_button_bar_update(obj);
}

static void
_e_nav_button_bar_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   sd->w = w;
   sd->h = h;

   if (sd->bg)
     evas_object_resize(sd->bg, sd->w, sd->h);

   evas_object_resize(sd->clip, sd->w, sd->h);

   _e_nav_button_bar_update(obj);
}

static void
_e_nav_button_bar_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_show(sd->clip);
}

static void
_e_nav_button_bar_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_hide(sd->clip);
}

static void
_e_nav_button_bar_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_nav_button_bar_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_clip_set(sd->clip, clip);
}

static void
_e_nav_button_bar_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_clip_unset(sd->clip);
} 

static void
_e_nav_button_bar_update(Evas_Object *bbar)
{
   E_Smart_Data *sd;
   Evas_List *l;
   Evas_Coord x, y, w, h;
   int count;

   SMART_CHECK(bbar, ;);

   count = evas_list_count(sd->buttons);
   if (!count)
     return;

   x = sd->x + sd->pad_front;
   y = sd->y;
   h = sd->h;

   switch (sd->style)
     {
      case E_NAV_BUTTON_BAR_STYLE_SPREAD:
      default:
	 w = sd->w - sd->pad_front - sd->pad_back -
	    (count - 1) * sd->pad_inter;
	 if (w < 0)
	   w = 0;
	 w /= count;
	 break;
      case E_NAV_BUTTON_BAR_STYLE_LEFT_ALIGNED:
      case E_NAV_BUTTON_BAR_STYLE_RIGHT_ALIGNED:
	 w = 0;

	 for (l = evas_list_last(sd->buttons); l; l = l->prev)
	   {
	      Button_Data *bdata = l->data;
	      Evas_Coord tmp;

	      if (bdata->obj)
		{
		   edje_object_size_min_calc(bdata->obj, &tmp, NULL);
		   if (tmp > w)
		     w = tmp;
		}
	   }

	 if (sd->style == E_NAV_BUTTON_BAR_STYLE_RIGHT_ALIGNED)
	   {
	      x = count * w + (count - 1) * sd->pad_inter;
	      x = sd->x + sd->w - sd->pad_back - x;
	   }
	 break;
     }

   for (l = evas_list_last(sd->buttons); l; l = l->prev)
     {
	Button_Data *bdata = l->data;

	if (bdata->obj)
	  {
	     evas_object_move(bdata->obj, x, y);
	     evas_object_resize(bdata->obj, w, h);
	  }

	x += w;

	if (bdata->pad)
	  {
	     evas_object_move(bdata->pad, x, y);
	     evas_object_resize(bdata->pad, sd->pad_inter, h);
	  }

	x += sd->pad_inter;
     }
}
