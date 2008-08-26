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

#include <Etk.h>
#include "e_nav_contact_editor.h"
#include "e_nav_list.h"
#include "e_nav_dialog.h"
#include "../e_nav.h"
#include "../e_nav_theme.h"
#include "../e_nav_misc.h"
#include "../e_ctrl.h"

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

   Evas_Object     *contact_list;

   void           (*button_left)(void *data, Evas_Object *obj);
   void            *button_left_data;

   void           (*button_right)(void *data, Evas_Object *obj);
   void            *button_right_data;
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
_e_nav_contact_cancel(void *data, Evas_Object *li)
{
   E_Smart_Data *sd = data;

   evas_object_hide(sd->contact_list);
   etk_widget_focus(sd->entry);
}

static void
_e_nav_contact_sel(void *data, Evas_Object *li, Evas_Object *bard)
{
   E_Smart_Data *sd = data;
   const char *name;

   name = e_nav_world_item_neo_other_name_get(bard);
   etk_entry_text_set(ETK_ENTRY(sd->entry), name);

   evas_object_hide(sd->contact_list);
   etk_widget_focus(sd->entry);
}

static int
_e_nav_contact_sort(void *data, Evas_Object *bard1, Evas_Object *bard2)
{
   const char *p1, *p2;

   p1 = e_nav_world_item_neo_other_name_get(bard1);
   if (!p1)
     return -1;

   p2 = e_nav_world_item_neo_other_name_get(bard2);
   if (!p2)
     return 1;

   return strcmp(p1, p2);
}

static void
_e_button_cb_mouse_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd = data;
   char *p;

   p = strchr(source, '.');
   if (!p)
     return;
   p++;

   etk_widget_unfocus(sd->entry);

   switch (*p)
     {
      case 'b':
	 if (sd->contact_list)
	   evas_object_show(sd->contact_list);
	 break;
      case 'l':
	 if (sd->button_left)
	   sd->button_left(sd->button_left_data, sd->obj);
	 break;
      case 'r':
      default:
	 if (sd->button_right)
	   sd->button_right(sd->button_right_data, sd->obj);
	 break;
     }
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
e_contact_editor_theme_source_set(Evas_Object *obj, const char *custom_dir, void (*positive_func)(void *data, Evas_Object *obj), void *data1, void (*negative_func)(void *data, Evas_Object *obj), void *data2)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (custom_dir)
     sd->dir = strdup(custom_dir);

   sd->frame = e_nav_theme_object_new(evas_object_evas_get(obj), custom_dir, "modules/diversity_nav/contact_editor");
   evas_object_move(sd->frame, sd->x, sd->y);
   evas_object_resize(sd->frame, sd->w, sd->h);
   evas_object_smart_member_add(sd->frame, sd->obj);
   evas_object_clip_set(sd->frame, sd->clip);
   evas_object_show(sd->frame);

   sd->button_left = positive_func;
   sd->button_left_data = data1;
   sd->button_right = negative_func;
   sd->button_right_data = data2;

   edje_object_part_text_set(sd->frame, "button.left", _("Send"));
   edje_object_part_text_set(sd->frame, "button.right", _("Cancel"));
   edje_object_part_text_set(sd->frame, "button.bottom", _("Add Contact"));

   edje_object_signal_callback_add(sd->frame, "mouse,clicked,*", "button.*", _e_button_cb_mouse_clicked, sd);

   sd->entry = etk_entry_new();
   etk_signal_connect_by_code(ETK_WIDGET_FOCUSED_SIGNAL, ETK_OBJECT(sd->entry), ETK_CALLBACK(_editor_focused_cb), NULL);
   etk_signal_connect_by_code(ETK_WIDGET_UNFOCUSED_SIGNAL, ETK_OBJECT(sd->entry), ETK_CALLBACK(_editor_unfocused_cb), NULL);

   sd->embed  = etk_embed_new(evas_object_evas_get(obj));
   etk_container_add(ETK_CONTAINER(sd->embed), sd->entry);
   edje_object_part_swallow(sd->frame, "swallow", etk_embed_object_get(ETK_EMBED(sd->embed)));
   etk_widget_show_all(ETK_WIDGET(sd->embed));

   sd->contact_list = e_nav_list_add(evas_object_evas_get(obj),
	 E_NAV_LIST_TYPE_BARD, THEMEDIR);
   e_nav_list_title_set(sd->contact_list, _("Select a contact"));
   e_nav_list_sort_set(sd->contact_list, _e_nav_contact_sort, NULL);
   e_nav_list_button_add(sd->contact_list, _("Cancel"), _e_nav_contact_cancel, sd);
   e_nav_list_callback_add(sd->contact_list, _e_nav_contact_sel, sd);

   evas_object_smart_member_add(sd->contact_list, obj);
   evas_object_clip_set(sd->contact_list, sd->clip);
}

void
e_contact_editor_activate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_Coord x, y, w, h;

   SMART_CHECK(obj, ;);

   evas_output_viewport_get(evas_object_evas_get(obj), &x, &y, &w, &h);
   evas_object_move(obj, x, y);
   evas_object_resize(obj, w, h);
   evas_object_show(obj);

   etk_widget_focus(sd->entry);
}

void
e_contact_editor_deactivate(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   etk_widget_unfocus(sd->entry);

   evas_object_del(obj);
}

void
e_contact_editor_input_length_limit_set(Evas_Object *obj, size_t length_limit)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (length_limit > 0)
     etk_entry_text_limit_set(ETK_ENTRY(sd->entry), length_limit);
}

size_t
e_contact_editor_input_length_limit_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, 0;);

   return etk_entry_text_limit_get(ETK_ENTRY(sd->entry));
}

void
e_contact_editor_input_set(Evas_Object *obj, const char *name, const char *input)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   edje_object_part_text_set(sd->frame, "title", name);

   if (!input)
     input = "";

   etk_entry_text_set(ETK_ENTRY(sd->entry), input);
}

const char *
e_contact_editor_input_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, NULL;);

   return etk_entry_text_get(ETK_ENTRY(sd->entry));
}

void
e_contact_editor_contacts_set(Evas_Object *obj, Evas_List *contacts)
{
   E_Smart_Data *sd;
   Evas_List *l;

   SMART_CHECK(obj, ;);

   e_nav_list_clear(sd->contact_list);

   for (l = contacts; l; l = l->next)
     {
	Evas_Object *bard = l->data;

	e_nav_list_object_add(sd->contact_list, bard);
     }
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
_e_contact_editor_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (sd->dir)
     free(sd->dir);

   evas_object_del(sd->clip);
   evas_object_del(sd->frame);

   etk_object_destroy(ETK_OBJECT(sd->embed));

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
   evas_object_move(sd->frame, sd->x, sd->y);
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
   evas_object_resize(sd->frame, sd->w, sd->h);
   if (sd->contact_list)
     evas_object_resize(sd->contact_list, sd->w, sd->h);
}

static void
_e_contact_editor_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_show(sd->clip);
}

static void
_e_contact_editor_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   evas_object_hide(sd->clip);
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
