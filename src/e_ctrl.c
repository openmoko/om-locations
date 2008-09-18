/* e_ctrl.c -
 *
 * Copyright 2008 OpenMoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
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

#include "widgets/e_nav_list.h"
#include "e_nav.h"    
#include "e_nav_theme.h"
#include "e_ctrl.h"
#include "e_nav_tileset.h"
#include "e_nav_item_location.h"
#include "msgboard.h"

#define E_NEW(s, n) (s *)calloc(n, sizeof(s))

typedef struct _E_Smart_Data E_Smart_Data;
static void _e_ctrl_cb_signal_drag(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_ctrl_cb_signal_drag_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_ctrl_cb_signal_drag_stop(void *data, Evas_Object *obj, const char *emission, const char *source);

struct _E_Smart_Data
{
   Evas_Object *nav;

   Evas_List  *contacts;
   Ecore_Hash *objectStore;

   Evas_Object *obj;      

   /* sorted by stack order */
   Evas_Object *panel_buttons;
   Evas_Object *listview;   
   Evas_Object *msgboard;
   Evas_Object *map_overlay;   
   Evas_Object *clip;

   int follow;
   Evas_Coord x, y, w, h;
   const char      *dir;

#define NUM_DRAG_VALUES 18
   double drag_values[NUM_DRAG_VALUES];
   double drag_tolerance;
};

static void _e_ctrl_smart_init(void);
static void _e_ctrl_smart_add(Evas_Object *obj);
static void _e_ctrl_smart_del(Evas_Object *obj);
static void _e_ctrl_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_ctrl_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_ctrl_smart_show(Evas_Object *obj);
static void _e_ctrl_smart_hide(Evas_Object *obj);
static void _e_ctrl_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_ctrl_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_ctrl_smart_clip_unset(Evas_Object *obj);

#define SMART_NAME "e_ctrl"
static Evas_Smart *_e_smart = NULL;


Evas_Object *
e_ctrl_add(Evas *e)
{
   Evas_Object *obj;
   E_Smart_Data *sd;

   _e_ctrl_smart_init();
   obj = evas_object_smart_add(e, _e_smart);
   if (!obj) return NULL;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     {
	evas_object_del(obj);
	return NULL;
     }

   sd->objectStore = ecore_hash_new(ecore_str_hash, ecore_str_compare);

   if(!sd->objectStore)
     {
	evas_object_del(obj);

	return NULL; 
     }

   ecore_hash_free_key_cb_set(sd->objectStore, free);

   return obj;
}

static void
_e_nav_tag_sel(void *data, Evas_Object *tl, E_Nav_List_Item *item)
{
   Evas_Object *loc = (Evas_Object *) item;
   E_Smart_Data *sd; 
   int unread;

   sd = evas_object_smart_data_get(data);
   if (!sd) 
     return;
    
   unread = e_nav_world_item_location_unread_get(loc); 

   if (unread)
     {
	Diversity_Tag *tag;

	unread = 0;

	tag = e_nav_world_item_location_tag_get(loc);
	if (tag)
	  diversity_dbus_property_set((Diversity_DBus *) tag,
		DIVERSITY_DBUS_IFACE_TAG,
		"Unread", DBUS_TYPE_BOOLEAN, &unread);

	e_nav_world_item_location_unread_set(loc, 0); 
	e_nav_list_item_update(sd->listview, item);
     }

   e_nav_world_item_location_details_set(loc, 1);
   e_nav_world_item_focus(loc);

   evas_object_show(sd->nav);

   sd->follow = 0;

   evas_object_hide(sd->listview);
   evas_object_show(sd->map_overlay);

   edje_object_signal_emit(sd->panel_buttons, "JUMP_TO_MAP", "");
}

static int
_e_nav_tag_sort(void *data, E_Nav_List_Item *tag1, E_Nav_List_Item *tag2)
{
   time_t t1 = 0, t2 = 0;

   if (tag1)
     t1 = e_nav_world_item_location_timestamp_get((Evas_Object *) tag1);
   if (tag2)
     t2 = e_nav_world_item_location_timestamp_get((Evas_Object *) tag2);

   return (t2 - t1);
}

void
e_ctrl_taglist_tag_set(Evas_Object *obj, Evas_Object *loc)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   e_nav_list_item_update(sd->listview, (E_Nav_List_Item *) loc);
}

void
e_ctrl_taglist_tag_add(Evas_Object *obj, Evas_Object *loc)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   e_nav_list_item_add(sd->listview, (E_Nav_List_Item *) loc);
}

void
e_ctrl_taglist_tag_delete(Evas_Object *obj, Evas_Object *loc)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   e_nav_list_item_remove(sd->listview, (E_Nav_List_Item *) loc);
}

void
e_ctrl_taglist_freeze(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   e_nav_list_freeze(sd->listview);
}

void
e_ctrl_taglist_thaw(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   e_nav_list_thaw(sd->listview);
}

static void
_e_nav_panel_cb_mouse_down(void *data, Evas_Object *panel, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   Evas_Object *neo_me;

   sd = evas_object_smart_data_get(data);

   switch (source[0])
     {
      case 's': /* star */
	 break;
      case 'r': /* refresh */
	 sd->follow = 1;

	 neo_me = e_nav_world_neo_me_get(sd->nav);
	 if (neo_me)
	   e_nav_world_item_focus(neo_me);

	 /* fall through */
      case 'm': /* map */
	 evas_object_show(sd->nav);

	 evas_object_hide(sd->listview);
	 evas_object_show(sd->map_overlay);

	 break;
      case 'l': /* list */
	 evas_object_show(sd->listview);
	 evas_object_hide(sd->map_overlay);

	 evas_object_hide(sd->nav);

	 break;
      default:
	 printf("%s %s\n", emission, source);

	 break;
     }
}

void
e_ctrl_theme_source_set(Evas_Object *obj, const char *custom_dir)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   
   sd->dir = custom_dir;
   sd->follow = 1;
   sd->map_overlay = e_nav_theme_object_new(evas_object_evas_get(obj), sd->dir,
				      "modules/diversity_nav/main");
   evas_object_smart_member_add(sd->map_overlay, obj);
   evas_object_move(sd->map_overlay, sd->x, sd->y);
   evas_object_resize(sd->map_overlay, sd->w, sd->h);
   evas_object_clip_set(sd->map_overlay, sd->clip);

   evas_object_show(sd->map_overlay);
   edje_object_signal_callback_add(sd->map_overlay, "drag", "*", _e_ctrl_cb_signal_drag, sd);
   edje_object_signal_callback_add(sd->map_overlay, "drag,start", "*", _e_ctrl_cb_signal_drag_start, sd);
   edje_object_signal_callback_add(sd->map_overlay, "drag,stop", "*", _e_ctrl_cb_signal_drag_stop, sd);
   edje_object_signal_callback_add(sd->map_overlay, "drag,step", "*", _e_ctrl_cb_signal_drag_stop, sd);
   edje_object_signal_callback_add(sd->map_overlay, "drag,set", "*", _e_ctrl_cb_signal_drag_stop, sd);

   sd->msgboard = msgboard_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->msgboard, obj);
   evas_object_move(sd->msgboard, sd->x, sd->y);
   evas_object_resize(sd->msgboard, sd->w, sd->h);
   evas_object_clip_set(sd->msgboard, sd->clip);
   evas_object_show(sd->msgboard);

   sd->listview = e_nav_list_add(evas_object_evas_get(obj), E_NAV_LIST_TYPE_TAG);
   e_nav_list_title_set(sd->listview, _("View Tags"));
   e_nav_list_sort_set(sd->listview, _e_nav_tag_sort, obj);
   e_nav_list_callback_add(sd->listview, _e_nav_tag_sel, obj);
   evas_object_smart_member_add(sd->listview, obj);
   evas_object_move(sd->listview, sd->x, sd->y);
   evas_object_resize(sd->listview, sd->w, sd->h);
   evas_object_clip_set(sd->listview, sd->clip);

   sd->panel_buttons = e_nav_theme_object_new(evas_object_evas_get(obj), sd->dir,
				      "modules/diversity_nav/panel");
   edje_object_part_text_set(sd->panel_buttons, "refresh_text", _("REFRESH"));
   edje_object_part_text_set(sd->panel_buttons, "map_text", _("MAP"));
   edje_object_part_text_set(sd->panel_buttons, "list_text", _("LIST"));

   edje_object_signal_callback_add(sd->panel_buttons,
	 "mouse,up,*", "*_button",
	 _e_nav_panel_cb_mouse_down, sd->obj);

   evas_object_smart_member_add(sd->panel_buttons, obj);
   evas_object_move(sd->panel_buttons, sd->x, sd->y);
   evas_object_resize(sd->panel_buttons, sd->w, sd->h);
   evas_object_clip_set(sd->panel_buttons, sd->clip);
   evas_object_show(sd->panel_buttons);
}

void
e_ctrl_nav_set(Evas_Object *obj, Evas_Object *nav)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->nav = nav;
}

#define DRAG_SENSITIVITY 18

static int
to_span(double v)
{
   v = 1.0 - v;
   v = (pow(2.0, v * DRAG_SENSITIVITY) - 1.0)
      / (1 << DRAG_SENSITIVITY);

   return E_NAV_SPAN_MIN + ((E_NAV_SPAN_MAX - E_NAV_SPAN_MIN) * v);
}

static double
from_span(int span)
{
   double v;

   v = (double) (span - E_NAV_SPAN_MIN) / (E_NAV_SPAN_MAX - E_NAV_SPAN_MIN);
   if (v < 0.0)
     v = 0.0;
   else if (v > 1.0)
     v = 1.0;

#define M_LOG2		(0.6931471805)
   v = log((v * (1 << DRAG_SENSITIVITY)) + 1)
      / M_LOG2 / DRAG_SENSITIVITY;

   return 1.0 - v;
}

/* internal calls */
static void
_e_ctrl_smart_init(void)
{
   if (_e_smart) return;

   {
      static const Evas_Smart_Class sc =
      {
	 SMART_NAME,
	 EVAS_SMART_CLASS_VERSION,
	 _e_ctrl_smart_add,
	 _e_ctrl_smart_del,
	 _e_ctrl_smart_move,
	 _e_ctrl_smart_resize,
	 _e_ctrl_smart_show,
	 _e_ctrl_smart_hide,
	 _e_ctrl_smart_color_set,
	 _e_ctrl_smart_clip_set,
	 _e_ctrl_smart_clip_unset,

	 NULL /* data */
      };
      _e_smart = evas_smart_class_new(&sc);
   }
}

static void
_e_ctrl_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   int i;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   sd->obj = obj;
   
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_smart_data_set(obj, sd);

   for (i = 0; i < NUM_DRAG_VALUES; i++)
     {
	int span = 256 * (1 << i); /* XXX 256? */
	double v;

	v = from_span(span);

	sd->drag_values[i] = v;
     }
   sd->drag_tolerance = 0.012;
}

static void
_e_ctrl_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   if(sd->objectStore)
     {
        ecore_hash_destroy(sd->objectStore);
     }

   while (sd->contacts)
     sd->contacts = evas_list_remove_list(sd->contacts, sd->contacts);

   evas_object_del(sd->clip);
   evas_object_del(sd->map_overlay);
   evas_object_del(sd->panel_buttons);
   evas_object_del(sd->listview);
   evas_object_del(sd->msgboard);
   free(sd);
}

static void
_e_ctrl_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;

   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_move(sd->map_overlay, sd->x, sd->y);
   evas_object_move(sd->panel_buttons, sd->x, sd->y);
   evas_object_move(sd->msgboard, sd->x, sd->y);
   evas_object_move(sd->listview, sd->x, sd->y);
}

static void
_e_ctrl_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_resize(sd->map_overlay, sd->w, sd->h);
   evas_object_resize(sd->panel_buttons, sd->w, sd->h);
   evas_object_resize(sd->msgboard, sd->w, sd->h);
   evas_object_resize(sd->listview, sd->w, sd->h);
}

static void
_e_ctrl_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_ctrl_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_ctrl_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_ctrl_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_ctrl_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}

void e_ctrl_span_drag_value_set(Evas_Object *obj, int span)
{
   E_Smart_Data *sd;
   double v;

   SMART_CHECK(obj, ;);

   v = from_span(span);
   edje_object_part_drag_value_set(sd->map_overlay, "e.dragable.zoom", 0.0, v);
}

void e_ctrl_span_text_value_set(Evas_Object *obj, const char* buf)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   edje_object_part_text_set(sd->map_overlay, "e.text.zoom", buf);
}

void e_ctrl_longitude_set(Evas_Object *obj, const char* buf)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   edje_object_part_text_set(sd->map_overlay, "e.text.longitude", buf);
}

void e_ctrl_latitude_set(Evas_Object *obj, const char* buf)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   edje_object_part_text_set(sd->map_overlay, "e.text.latitude", buf);
}

static void
_e_ctrl_cb_signal_drag(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   
   sd = data;
     {
	double x = 0, y = 0;
	int span;
	int i;
	
	edje_object_part_drag_value_get(sd->map_overlay, "e.dragable.zoom", &x, &y);

	/* drag values are revert sorted */
	for (i = NUM_DRAG_VALUES - 1; i >= 0; i--)
	  {
	     if (sd->drag_values[i] < y)
	       continue;

	     if (fabs(y - sd->drag_values[i]) < sd->drag_tolerance)
	       {
		  y = sd->drag_values[i];
	       }
	     else if (i < NUM_DRAG_VALUES - 1)
	       {
		  if (fabs(y - sd->drag_values[i + 1]) < sd->drag_tolerance)
		    y = sd->drag_values[i + 1];
	       }

	     break;
	  }

	span = to_span(y);
	e_nav_span_set(sd->nav, span, 0.0);
     }
}

static void
_e_ctrl_cb_signal_drag_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   
   sd = data;
     {
	double x = 0, y = 0;
	
	edje_object_part_drag_value_get(sd->map_overlay, "e.dragable.zoom", &x, &y);
     }
}

static void
_e_ctrl_cb_signal_drag_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   
   sd = data;
     {
	double x = 0, y = 0;
	
	edje_object_part_drag_value_get(sd->map_overlay, "e.dragable.zoom", &x, &y);
     }
}

int
e_ctrl_follow_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, 0;);
   return sd->follow;
}

void
e_ctrl_follow_set(Evas_Object *obj, int follow)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->follow = follow;
}

unsigned int
e_ctrl_message_text_add(Evas_Object *obj, const char *msg, double timeout)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, 0;);

   return msgboard_message_add(sd->msgboard, msg, timeout);
}

void
e_ctrl_message_text_edit(Evas_Object *obj, unsigned int msg_id, const char *msg, double timeout)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   msgboard_message_edit(sd->msgboard, msg_id, msg, timeout);
}

void
e_ctrl_message_text_del(Evas_Object *obj, unsigned int msg_id)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   msgboard_message_del(sd->msgboard, msg_id);
}

void
e_ctrl_contact_add(Evas_Object *obj, E_Nav_Card *card)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->contacts = evas_list_prepend(sd->contacts, card);
}

void
e_ctrl_contact_delete(Evas_Object *obj, E_Nav_Card *card)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->contacts = evas_list_remove(sd->contacts, card);
}

E_Nav_Card *
e_ctrl_contact_get_by_name(Evas_Object *obj, const char *name)
{
   E_Smart_Data *sd;
   Evas_List *l;

   SMART_CHECK(obj, NULL;);

   for (l = sd->contacts; l; l = l->next)
     {
	E_Nav_Card *card = l->data;
	const char *n = e_nav_card_name_get(card);

	if (n && name && n[0] == name[0] && strcmp(n, name) == 0)
	  return card;
     }

   return NULL;
}

E_Nav_Card *
e_ctrl_contact_get_by_number(Evas_Object *obj, const char *number)
{
   E_Smart_Data *sd;
   Evas_List *l;

   SMART_CHECK(obj, NULL;);

   for (l = sd->contacts; l; l = l->next)
     {
	E_Nav_Card *card = l->data;
	const char *p = e_nav_card_phone_get(card);

	if (p && number && p[0] == number[0] && strcmp(p, number) == 0)
	  return card;
     }

   return NULL;
}

Evas_List *
e_ctrl_contact_list(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, NULL;);

   return sd->contacts;
}

void
e_ctrl_object_store_item_add(Evas_Object *obj, const char *obj_path, void *item)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   ecore_hash_set(sd->objectStore, strdup(obj_path), item);
}

void *
e_ctrl_object_store_item_get(Evas_Object *obj, const char *obj_path)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, NULL;);

   return ecore_hash_get(sd->objectStore, obj_path);
}

void *
e_ctrl_object_store_item_remove(Evas_Object *obj, const char *obj_path)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, NULL;);

   return ecore_hash_remove(sd->objectStore, obj_path);
}

Ecore_List *
e_ctrl_object_store_keys(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, NULL;);

   return ecore_hash_keys(sd->objectStore);
}
