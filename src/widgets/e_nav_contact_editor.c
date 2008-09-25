/* e_nav_contact_editor.c -
 *
 * Copyright 2008 Openmoko, Inc.
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

#include "e_nav_contact_editor.h"
#include "e_nav_list.h"
#include "e_nav_entry.h"
#include "../e_nav_item_neo_other.h"
#include "../e_nav.h"
#include "../e_nav_theme.h"
#include "../e_nav_misc.h"

typedef struct _E_Smart_Data E_Smart_Data;

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *clip;

   Evas_Object     *entry;
   Evas_Object     *contact_list;
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

#define SMART_NAME "e_contact_editor"
static Evas_Smart *_e_smart = NULL;

Evas_Object *
e_contact_editor_add(Evas *e)
{
   _e_contact_editor_smart_init();
   return evas_object_smart_add(e, _e_smart);
}

static void
_e_nav_contact_cancel(void *data, Evas_Object *li)
{
   E_Smart_Data *sd = data;

   evas_object_hide(sd->contact_list);
   e_nav_entry_focus(sd->entry);
}

static void
_e_nav_contact_sel(void *data, Evas_Object *li, E_Nav_List_Item *card)
{
   E_Smart_Data *sd = data;
   const char *name;

   name = e_nav_card_name_get((E_Nav_Card *) card);
   e_nav_entry_text_set(sd->entry, name);

   evas_object_hide(sd->contact_list);
   e_nav_entry_focus(sd->entry);
}

static int
_e_nav_contact_sort(void *data, E_Nav_List_Item *card1, E_Nav_List_Item *card2)
{
   const char *p1, *p2;

   if (!card1)
     return -1;
   else if (!card2)
     return 1;

   p1 = e_nav_card_name_get((E_Nav_Card *) card1);
   if (!p1)
     return -1;

   p2 = e_nav_card_name_get((E_Nav_Card *) card2);
   if (!p2)
     return 1;

   return strcmp(p1, p2);
}

static void
on_add_contact(void *data, Evas_Object *entry)
{
   E_Smart_Data *sd = data;

   e_nav_entry_unfocus(sd->entry);
   evas_object_show(sd->contact_list);
}

void
e_contact_editor_callbacks_set(Evas_Object *obj, void (*positive_func)(void *data, Evas_Object *obj), void *data1, void (*negative_func)(void *data, Evas_Object *obj), void *data2)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   e_nav_entry_button_add(sd->entry, _("Send"), positive_func, data1);
   e_nav_entry_button_add(sd->entry, _("Cancel"), negative_func, data2);
   e_nav_entry_button_add(sd->entry, _("Add Contact"), on_add_contact, sd);

   sd->contact_list = e_nav_list_add(evas_object_evas_get(obj),
	 E_NAV_LIST_TYPE_CARD);
   e_nav_list_title_set(sd->contact_list, _("Select a contact"));
   e_nav_list_sort_set(sd->contact_list, _e_nav_contact_sort, NULL);
   e_nav_list_button_add(sd->contact_list, _("Cancel"), _e_nav_contact_cancel, sd);
   e_nav_list_callback_add(sd->contact_list, _e_nav_contact_sel, sd);

   evas_object_smart_member_add(sd->contact_list, obj);
   evas_object_clip_set(sd->contact_list, sd->clip);
}

void
e_contact_editor_input_length_limit_set(Evas_Object *obj, size_t length_limit)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   e_nav_entry_text_limit_set(sd->entry, length_limit);
}

size_t
e_contact_editor_input_length_limit_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, 0;);

   return e_nav_entry_text_limit_get(sd->entry);
}

void
e_contact_editor_input_set(Evas_Object *obj, const char *name, const char *input)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (name)
     e_nav_entry_title_set(sd->entry, name);

   e_nav_entry_text_set(sd->entry, input);
}

const char *
e_contact_editor_input_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, NULL;);

   return e_nav_entry_text_get(sd->entry);
}

void
e_contact_editor_contacts_set(Evas_Object *obj, Evas_List *contacts)
{
   E_Smart_Data *sd;
   Evas_List *l;

   SMART_CHECK(obj, ;);

   e_nav_list_freeze(sd->contact_list);

   e_nav_list_clear(sd->contact_list);

   for (l = contacts; l; l = l->next)
     {
	E_Nav_List_Item *bard = l->data;

	e_nav_list_item_add(sd->contact_list, bard);
     }

   e_nav_list_thaw(sd->contact_list);
}

/* internal calls */
static void
_e_contact_editor_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
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

   sd->entry = e_nav_entry_add(evas_object_evas_get(obj));
   evas_object_move(sd->entry, sd->x, sd->y);
   evas_object_resize(sd->entry, sd->w, sd->h);
   evas_object_smart_member_add(sd->entry, sd->obj);
   evas_object_clip_set(sd->entry, sd->clip);
   evas_object_show(sd->entry);

   evas_object_smart_data_set(obj, sd);
}

static void
_e_contact_editor_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_del(sd->clip);
   evas_object_del(sd->entry);

   if (sd->contact_list)
     evas_object_del(sd->contact_list);

   free(sd);
}


static void
_e_contact_editor_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->x = x;
   sd->y = y;

   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_move(sd->entry, sd->x, sd->y);
   if (sd->contact_list)
     evas_object_move(sd->contact_list, sd->x, sd->y);
}

static void
_e_contact_editor_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->w = w;
   sd->h = h;

   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_resize(sd->entry, sd->w, sd->h);
   if (sd->contact_list)
     evas_object_resize(sd->contact_list, sd->w, sd->h);
}

static void
_e_contact_editor_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_show(sd->clip);
   e_nav_entry_focus(sd->entry);
}

static void
_e_contact_editor_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_hide(sd->clip);
   e_nav_entry_unfocus(sd->entry);
}

static void
_e_contact_editor_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_contact_editor_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_clip_set(sd->clip, clip);
}

static void
_e_contact_editor_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_clip_unset(sd->clip);
}
