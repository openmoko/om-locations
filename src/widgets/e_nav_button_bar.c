/* e_nav_button_bar.c -
 *
 * Copyright 2008 Openmoko, Inc.
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
#include <assert.h>
#include "e_nav_button_bar.h"
#include "../e_nav.h"
#include "../e_nav_theme.h"

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _Button_Data Button_Data;

enum {
     E_NAV_BUTTON_BAR_BUTTON_TYPE_FRONT,
     E_NAV_BUTTON_BAR_BUTTON_TYPE_BACK,
     N_E_NAV_BUTTON_BAR_BUTTON_TYPES,
};

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
   Evas_Coord req_button_width;
   Evas_Coord req_button_height;

   Eina_List *buttons;
   Evas_Coord button_width;
   Evas_Coord button_height;

   char recalc_width : 1;
   char recalc_height : 1;
   Evas_Coord min_width;
   Evas_Coord min_height;
};

struct _Button_Data {
     unsigned int id;
     int type;
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

	sd->bg = e_nav_theme_component_new(evas_object_evas_get(bbar),
	      sd->group_base, "bg", 1);
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

   sd->recalc_width = 1;
   sd->recalc_height = 1;
   _e_nav_button_bar_update(bbar);
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

	sd->recalc_width = 1;
	_e_nav_button_bar_update(bbar);
     }
}

static void
e_nav_button_bar_button_size_calc(Evas_Object *bbar)
{
   E_Smart_Data *sd;
   Eina_List *l;
   Evas_Coord w, h;

   SMART_CHECK(bbar, ;);

   w = 0;
   h = 0;
   for (l = sd->buttons; l; l = l->next)
     {
	Button_Data *bdata = l->data;
	Evas_Coord tmp_w, tmp_h;

	if (bdata->obj)
	  {
	     edje_object_size_min_calc(bdata->obj, &tmp_w, &tmp_h);
	     if (tmp_w > w)
	       w = tmp_w;
	     if (tmp_h > h)
	       h = tmp_h;
	  }
     }

   /* fit request */
   if (w < sd->req_button_width)
     w = sd->req_button_width;
   if (h < sd->req_button_height)
     h = sd->req_button_height;

   sd->button_width = w;
   sd->button_height = h;
}

void
e_nav_button_bar_button_size_request(Evas_Object *bbar, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   int changed = 0, recalc = 0;

   SMART_CHECK(bbar, ;);

   if (sd->req_button_width < w)
     {
	sd->req_button_width = w;

	if (sd->button_width < w)
	  {
	     changed = 1;
	     sd->button_width = w;
	  }
     }
   else if (sd->req_button_width > w)
     {
	if (sd->button_width == sd->req_button_width)
	  recalc = 1;

	sd->req_button_width = w;
     }

   if (sd->req_button_height < h)
     {
	sd->req_button_height = h;

	if (sd->button_height < h)
	  {
	     changed = 1;
	     sd->button_height = h;
	  }
     }
   else if (sd->req_button_height > h)
     {
	if (sd->button_height == sd->req_button_height)
	  recalc = 1;

	sd->req_button_height = h;
     }

   if (recalc)
     e_nav_button_bar_button_size_calc(bbar);

   if (changed || recalc)
     {
	sd->recalc_width = 1;
	sd->recalc_height = 1;
	_e_nav_button_bar_update(bbar);
     }
}

int
e_nav_button_bar_num_buttons_get(Evas_Object *bbar)
{
   E_Smart_Data *sd;

   SMART_CHECK(bbar, 0;);

   return eina_list_count(sd->buttons);
}

Evas_Coord
e_nav_button_bar_width_min_calc(Evas_Object *bbar)
{
   E_Smart_Data *sd;
   Evas_Coord w;
   int count;

   SMART_CHECK(bbar, 0;);

   if (!sd->recalc_width)
     return sd->min_width;

   w = sd->pad_front + sd->pad_back;

   count = eina_list_count(sd->buttons);
   if (count)
     w += sd->button_width * count + sd->pad_inter * (count - 1);

   sd->recalc_width = 0;
   sd->min_width = w;

   return w;
}

Evas_Coord
e_nav_button_bar_height_min_calc(Evas_Object *bbar)
{
   E_Smart_Data *sd;
   Evas_Coord h;

   SMART_CHECK(bbar, 0;);

   if (!sd->recalc_height)
     return sd->min_height;

   h = 0;

   if (sd->bg)
     {
	Evas_Coord tmp;

	edje_object_size_min_calc(sd->bg, NULL, &tmp);
	if (h < tmp)
	  h = tmp;
     }

   /* check pad? */

   if (h < sd->button_height)
     h = sd->button_height;

   sd->recalc_height = 0;
   sd->min_height = h;

   return h;
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

static Button_Data *
button_new(Evas_Object *bbar, const char *label, void *cb, void *cb_data, int type)
{
   E_Smart_Data *sd;
   Button_Data *bdata;

   SMART_CHECK(bbar, NULL;);

   bdata = calloc(1, sizeof(*bdata));
   if (!bdata)
     return NULL;

   bdata->id = ++sd->serial_number;
   bdata->type = type;

   bdata->bbar = bbar;
   bdata->cb = cb;
   bdata->cb_data = cb_data;

   bdata->obj = e_nav_theme_component_new(evas_object_evas_get(bbar),
	 sd->group_base, "button", 1);
   if (bdata->obj)
     {
	evas_object_smart_member_add(bdata->obj, sd->obj);
	evas_object_clip_set(bdata->obj, sd->clip);
	evas_object_show(bdata->obj);

	edje_object_part_text_set(bdata->obj, "button.text", label);

	edje_object_signal_callback_add(bdata->obj,
	      "mouse,clicked,*", "button.*", on_button_clicked, bdata);

	if (type == E_NAV_BUTTON_BAR_BUTTON_TYPE_FRONT)
	  edje_object_signal_emit(bdata->obj, "nav,state,front", "nav");
	else
	  edje_object_signal_emit(bdata->obj, "nav,state,back", "nav");
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
e_nav_button_bar_bdata_add(Evas_Object *bbar, Button_Data *bdata)
{
   E_Smart_Data *sd;
   Button_Data *prev = NULL;
   Eina_List *l;

   SMART_CHECK(bbar, ;);

   for (l = sd->buttons; l; l = l->next)
     {
	Button_Data *tmp = l->data;

	if (tmp->type == bdata->type)
	  {
	     prev = tmp;

	     break;
	  }
     }

   if (prev)
     {
	assert(!prev->pad);

	prev->pad = e_nav_theme_component_new(evas_object_evas_get(bbar),
	      sd->group_base, "pad", 1);
	if (prev->pad)
	  {
	     evas_object_smart_member_add(prev->pad, sd->obj);
	     evas_object_clip_set(prev->pad, sd->clip);
	     evas_object_show(prev->pad);
	  }
     }

   sd->buttons = eina_list_prepend(sd->buttons, bdata);
   e_nav_button_bar_button_size_calc(bbar);
}

static void
e_nav_button_bar_bdata_remove(Evas_Object *bbar, Button_Data *bdata)
{
   E_Smart_Data *sd;
   Button_Data *prev = NULL;
   Eina_List *l, *tmp_l;

   SMART_CHECK(bbar, ;);

   l = eina_list_data_find(sd->buttons, bdata);
   if (!l)
     return;

   for (tmp_l = l->next; tmp_l; tmp_l = tmp_l->next)
     {
	Button_Data *tmp = tmp_l->data;

	if (tmp->type == bdata->type)
	  {
	     prev = tmp;

	     break;
	  }
     }

   if (prev && prev->pad)
     {
	evas_object_del(prev->pad);
	prev->pad = NULL;
     }

   sd->buttons = eina_list_remove_list(sd->buttons, l);
   e_nav_button_bar_button_size_calc(bbar);
}

void
e_nav_button_bar_button_add(Evas_Object *bbar, const char *label, void (*func)(void *data, Evas_Object *bbar), void *data)
{
   E_Smart_Data *sd;
   Button_Data *bdata;

   SMART_CHECK(bbar, ;);

   bdata = button_new(bbar, label, func, data, E_NAV_BUTTON_BAR_BUTTON_TYPE_FRONT);
   if (!bdata)
     return;

   e_nav_button_bar_bdata_add(bbar, bdata);

   sd->recalc_width = 1;
   sd->recalc_height = 1;
   _e_nav_button_bar_update(bbar);
}

void
e_nav_button_bar_button_add_back(Evas_Object *bbar, const char *label, void (*func)(void *data, Evas_Object *bbar), void *data)
{
   E_Smart_Data *sd;
   Button_Data *bdata;

   SMART_CHECK(bbar, ;);

   bdata = button_new(bbar, label, func, data, E_NAV_BUTTON_BAR_BUTTON_TYPE_BACK);
   if (!bdata)
     return;

   e_nav_button_bar_bdata_add(bbar, bdata);

   sd->recalc_width = 1;
   sd->recalc_height = 1;
   _e_nav_button_bar_update(bbar);
}

void
e_nav_button_bar_button_remove(Evas_Object *bbar, void (*func)(void *data, Evas_Object *bbar), void *data)
{
   E_Smart_Data *sd;
   Button_Data *bdata;
   Eina_List *l;

   SMART_CHECK(bbar, ;);

   for (l = sd->buttons; l; l = l->next)
     {
	bdata = l->data;

	if (bdata->cb == func && bdata->cb_data == data)
	  break;
     }
   if (!l)
     return;

   e_nav_button_bar_bdata_remove(bbar, bdata);
   button_destroy(bdata);

   sd->recalc_width = 1;
   sd->recalc_height = 1;
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
	   eina_list_remove_list(sd->buttons, sd->buttons);
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
   Eina_List *l;
   Evas_Coord x, y, w, h;
   int count;

   SMART_CHECK(bbar, ;);

   count = eina_list_count(sd->buttons);
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
	 w = sd->button_width;
	 break;
      case E_NAV_BUTTON_BAR_STYLE_RIGHT_ALIGNED:
	 w = e_nav_button_bar_width_min_calc(sd->obj);
	 x += sd->w - w;

	 w = sd->button_width;
	 break;
      case E_NAV_BUTTON_BAR_STYLE_CENTERED:
	 w = e_nav_button_bar_width_min_calc(sd->obj);
	 x += (sd->w - w) / 2;

	 w = sd->button_width;
	 break;
      case E_NAV_BUTTON_BAR_STYLE_JUSTIFIED:
	 w = sd->button_width;
	 break;
     }

   /* layout buttons in the front */
   for (l = eina_list_last(sd->buttons); l; l = l->prev)
     {
	Button_Data *bdata = l->data;

	if (bdata->type != E_NAV_BUTTON_BAR_BUTTON_TYPE_FRONT)
	  continue;

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
	count--;
     }

   /* no button added to the back */
   if (!count)
     return;

   if (sd->style == E_NAV_BUTTON_BAR_STYLE_JUSTIFIED)
     {
	w = sd->pad_back + count * sd->button_width +
	   (count - 1) * sd->pad_inter;

	x = sd->x + sd->w - w;
	w = sd->button_width;
     }

   /* layout buttons in the back */
   for (l = sd->buttons; l; l = l->next)
     {
	Button_Data *bdata = l->data;

	if (bdata->type == E_NAV_BUTTON_BAR_BUTTON_TYPE_FRONT)
	  continue;

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
