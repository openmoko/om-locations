/* tileman.c -
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
#include "msgboard.h"
#include "e_nav.h"
#include "e_nav_theme.h"

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _Message_Data Message_Data;

struct _E_Smart_Data
{
   Evas_Object *obj;
   Evas_Coord x, y, w, h;

   Evas_Object *clip;
   Evas_Object *bg;

   unsigned int serial_number;

   Evas_List *messages;
};

struct _Message_Data {
     unsigned int msg_id;

     Evas_Object *mb;
     Evas_Object *obj;

     Ecore_Timer *timeout;
};

static void _msgboard_smart_init(void);
static void _msgboard_smart_add(Evas_Object *obj);
static void _msgboard_smart_del(Evas_Object *obj);
static void _msgboard_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _msgboard_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _msgboard_smart_show(Evas_Object *obj);
static void _msgboard_smart_hide(Evas_Object *obj);
static void _msgboard_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _msgboard_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _msgboard_smart_clip_unset(Evas_Object *obj);

static void _msgboard_update(Evas_Object *mb);

#define SMART_NAME "msgboard"
static Evas_Smart *_e_smart = NULL;

static int
msg_timeout(void *data)
{
   Message_Data *md = data;

   msgboard_message_del(md->mb, md->msg_id);

   return 0;
}

static Message_Data *
msg_new(Evas_Object *mb)
{
   E_Smart_Data *sd;
   Message_Data *md;

   SMART_CHECK(mb, NULL;);

   md = calloc(1, sizeof(*md));
   if (!md)
     return NULL;

   md->msg_id = ++sd->serial_number;
   md->mb = mb;

   md->obj = e_nav_theme_object_new(evas_object_evas_get(mb), NULL,
	 "modules/diversity_nav/message");
   evas_object_smart_member_add(md->obj, sd->obj);
   evas_object_clip_set(md->obj, sd->clip);
   evas_object_show(md->obj);

   return md;
}

static void
msg_edit(Message_Data *md, const char *msg, double timeout)
{
   edje_object_part_text_set(md->obj, "message.text", msg);

   if (timeout > 0.0)
     {
	if (md->timeout)
	  ecore_timer_interval_set(md->timeout, timeout);
	else
	  md->timeout = ecore_timer_add(timeout, msg_timeout, md);
     }
   else if (timeout < 0.0 && md->timeout)
     {
	ecore_timer_del(md->timeout);
	md->timeout = NULL;
     }
}

static void
msg_destroy(Message_Data *md)
{
   if (md->timeout)
     ecore_timer_del(md->timeout);

   evas_object_del(md->obj);

   free(md);
}

Evas_Object *msgboard_add(Evas *e)
{
   _msgboard_smart_init();

   return evas_object_smart_add(e, _e_smart);
}

unsigned int
msgboard_message_add(Evas_Object *mb, const char *msg, double timeout)
{
   E_Smart_Data *sd;
   Message_Data *md;

   SMART_CHECK(mb, 0;);

   md = msg_new(mb);
   if (!md)
     return 0;

   sd->messages = evas_list_prepend(sd->messages, md);

   msg_edit(md, msg, timeout);

   _msgboard_update(mb);

   return md->msg_id;
}

void
msgboard_message_edit(Evas_Object *mb, unsigned int msg_id, const char *msg, double timeout)
{
   E_Smart_Data *sd;
   Message_Data *md;
   Evas_List *l;

   SMART_CHECK(mb, ;);

   for (l = sd->messages; l; l = l->next)
     {
	md = l->data;
	if (md->msg_id == msg_id)
	  break;
     }
   if (!l)
     return;

   msg_edit(md, msg, timeout);
}

void
msgboard_message_del(Evas_Object *mb, unsigned int msg_id)
{
   E_Smart_Data *sd;
   Message_Data *md;
   Evas_List *l;

   SMART_CHECK(mb, ;);

   for (l = sd->messages; l; l = l->next)
     {
	md = l->data;
	if (md->msg_id == msg_id)
	  break;
     }
   if (!l)
     return;

   sd->messages = evas_list_remove_list(sd->messages, l);

   msg_destroy(md);

   _msgboard_update(mb);
}

/* internal calls */
static void
_msgboard_smart_init(void)
{
   if (!_e_smart)
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
	     EVAS_SMART_CLASS_VERSION,
	     _msgboard_smart_add,
	     _msgboard_smart_del,
	     _msgboard_smart_move,
	     _msgboard_smart_resize,
	     _msgboard_smart_show,
	     _msgboard_smart_hide,
	     _msgboard_smart_color_set,
	     _msgboard_smart_clip_set,
	     _msgboard_smart_clip_unset,

	     NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_msgboard_smart_add(Evas_Object *obj)
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

   sd->bg = e_nav_theme_object_new(evas_object_evas_get(obj), NULL,
	 "modules/diversity_nav/msgboard");
   evas_object_smart_member_add(sd->bg, sd->obj);
   evas_object_clip_set(sd->bg, sd->clip);
   evas_object_move(sd->bg, sd->x, sd->y);
   evas_object_resize(sd->bg, sd->w, sd->h);
   evas_object_show(sd->bg);

   evas_object_smart_data_set(obj, sd);
}

static void
_msgboard_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   while (sd->messages)
     {
	msg_destroy(sd->messages->data);
	sd->messages = 
	   evas_list_remove_list(sd->messages, sd->messages);
     }

   evas_object_del(sd->bg);
   evas_object_del(sd->clip);

   free(sd);
}

static void
_msgboard_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   sd->x = x;
   sd->y = y;

   evas_object_move(sd->bg, sd->x, sd->y);
   evas_object_move(sd->clip, sd->x, sd->y);

   _msgboard_update(obj);
}

static void
_msgboard_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   sd->w = w;
   sd->h = h;

   evas_object_resize(sd->bg, sd->w, sd->h);
   evas_object_resize(sd->clip, sd->w, sd->h);

   _msgboard_update(obj);
}

static void
_msgboard_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_show(sd->clip);
}

static void
_msgboard_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_hide(sd->clip);
}

static void
_msgboard_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_msgboard_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_clip_set(sd->clip, clip);
}

static void
_msgboard_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   evas_object_clip_unset(sd->clip);
} 

static void
_msgboard_update(Evas_Object *mb)
{
   E_Smart_Data *sd;
   Evas_List *l;
   int count;

   SMART_CHECK(mb, ;);

   for (l = evas_list_last(sd->messages), count = 0; l; l = l->prev, count++)
     {
	Message_Data *md = l->data;
	Evas_Coord x, y, w, h;

	edje_object_size_min_calc(md->obj, &w, &h);
	if (w >= sd->w)
	  w = sd->w;

	x = sd->x;
	y = sd->y + count * h;

	evas_object_move(md->obj, x, y);
	evas_object_resize(md->obj, w, h);
     }
}
