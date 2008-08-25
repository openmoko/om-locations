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
   Evas_Object *message;
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

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_ctrl")) return ret


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
_e_nav_tag_sel(void *data, Evas_Object *tl, Evas_Object *loc)
{
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
	e_nav_list_object_update(sd->listview, loc);
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
_e_nav_tag_sort(void *data, Evas_Object *tag1, Evas_Object *tag2)
{
   time_t t1, t2;

   t1 = e_nav_world_item_location_timestamp_get(tag1);
   t2 = e_nav_world_item_location_timestamp_get(tag2);

   return (t2 - t1);
}

void
e_ctrl_taglist_tag_set(Evas_Object *obj, Evas_Object *loc)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   e_nav_list_object_update(sd->listview, loc);
}

void
e_ctrl_taglist_tag_add(Evas_Object *obj, Evas_Object *loc)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   e_nav_list_object_add(sd->listview, loc);
}

void
e_ctrl_taglist_tag_delete(Evas_Object *obj, Evas_Object *loc)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   e_nav_list_object_remove(sd->listview, loc);
}

static void   
_e_nav_list_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd; 
   sd = evas_object_smart_data_get(data);
   if(!sd) {
       printf("sd is NULL\n");
       return;
   }
   evas_object_show(sd->listview);
   evas_object_hide(sd->map_overlay);

   evas_object_hide(sd->nav);
}

static void   
_e_nav_refresh_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd; 
   Evas_Object *neo_me;

   sd = evas_object_smart_data_get(data);
   if (!sd)
     return;

   sd->follow = 1;

   neo_me = e_nav_world_neo_me_get(sd->nav);
   if (neo_me)
     e_nav_world_item_focus(neo_me);

   evas_object_show(sd->nav);

   evas_object_hide(sd->listview);
   evas_object_show(sd->map_overlay);
}

static void
_e_nav_map_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(data);

   evas_object_show(sd->nav);

   evas_object_hide(sd->listview);
   evas_object_show(sd->map_overlay);
}

static void
_e_nav_level_up(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *ctrl_obj = (Evas_Object *)data; 
   E_Smart_Data *sd;
   SMART_CHECK(ctrl_obj, ;);
   e_nav_level_up(sd->nav);
}

static void
_e_nav_level_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *ctrl_obj = (Evas_Object *)data; 
   E_Smart_Data *sd;
   SMART_CHECK(ctrl_obj, ;);
   e_nav_level_down(sd->nav);
}

static void
_e_nav_view_up(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *ctrl_obj = (Evas_Object *)data; 
   E_Smart_Data *sd;
   SMART_CHECK(ctrl_obj, ;);
   e_nav_move_up(sd->nav);
}

static void
_e_nav_view_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *ctrl_obj = (Evas_Object *)data; 
   E_Smart_Data *sd;
   SMART_CHECK(ctrl_obj, ;);
   e_nav_move_down(sd->nav);
}

static void
_e_nav_view_left(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *ctrl_obj = (Evas_Object *)data; 
   E_Smart_Data *sd;
   SMART_CHECK(ctrl_obj, ;);
   e_nav_move_left(sd->nav);
}

static void
_e_nav_view_right(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *ctrl_obj = (Evas_Object *)data; 
   E_Smart_Data *sd;
   SMART_CHECK(ctrl_obj, ;);
   e_nav_move_right(sd->nav);
}

void
e_ctrl_theme_source_set(Evas_Object *obj, const char *custom_dir)
{
   E_Smart_Data *sd;
   Evas_Object *star, *map, *refresh, *list; 
   Evas_Object *zoomin, *zoomout, *up, *down, *left, *right; 
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

   zoomin = edje_object_part_object_get(sd->map_overlay, "zoom_in"); 
   zoomout = edje_object_part_object_get(sd->map_overlay, "zoom_out"); 
   up = edje_object_part_object_get(sd->map_overlay, "up"); 
   down = edje_object_part_object_get(sd->map_overlay, "down"); 
   left = edje_object_part_object_get(sd->map_overlay, "left"); 
   right = edje_object_part_object_get(sd->map_overlay, "right"); 
   evas_object_event_callback_add(zoomin, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_level_up,
				  obj);
   evas_object_event_callback_add(zoomout, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_level_down,
				  obj);
   evas_object_event_callback_add(up, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_view_up,
				  obj);
   evas_object_event_callback_add(down, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_view_down,
				  obj);
   evas_object_event_callback_add(left, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_view_left,
				  obj);
   evas_object_event_callback_add(right, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_view_right,
				  obj);

   sd->message = e_nav_theme_object_new(evas_object_evas_get(obj), sd->dir,
				      "modules/diversity_nav/message");
   edje_object_part_text_set(sd->message, "message.text", _("Searching for your location"));

   evas_object_smart_member_add(sd->message, obj);
   evas_object_move(sd->message, sd->x, sd->y);
   evas_object_resize(sd->message, sd->w, sd->h);
   evas_object_clip_set(sd->message, sd->clip);

   sd->listview = e_nav_list_add(evas_object_evas_get(obj), E_NAV_LIST_TYPE_TAG, THEMEDIR);
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
   evas_object_smart_member_add(sd->panel_buttons, obj);
   evas_object_move(sd->panel_buttons, sd->x, sd->y);
   evas_object_resize(sd->panel_buttons, sd->w, sd->h);
   evas_object_clip_set(sd->panel_buttons, sd->clip);

   star = edje_object_part_object_get(sd->panel_buttons, "star_button"); 
   map = edje_object_part_object_get(sd->panel_buttons, "map_button"); 
   refresh = edje_object_part_object_get(sd->panel_buttons, "refresh_button"); 
   list = edje_object_part_object_get(sd->panel_buttons, "list_button"); 

   evas_object_event_callback_add(map, EVAS_CALLBACK_MOUSE_UP,
				  _e_nav_map_button_cb_mouse_down, obj);
   evas_object_event_callback_add(refresh, EVAS_CALLBACK_MOUSE_UP,
				  _e_nav_refresh_button_cb_mouse_down, obj);
   evas_object_event_callback_add(list, EVAS_CALLBACK_MOUSE_UP,
				  _e_nav_list_button_cb_mouse_down, obj);
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
	 "e_ctrl",
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
   evas_object_del(sd->message);
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
   evas_object_move(sd->message, sd->x, sd->y);
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
   evas_object_resize(sd->message, sd->w, sd->h);
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

void
e_ctrl_message_text_set(Evas_Object *obj, const char *msg)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   if(!sd) return;
   edje_object_part_text_set(sd->message, "message.text", msg);
}

void
e_ctrl_message_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   if(!sd) return;
   evas_object_hide(sd->message);
}

void
e_ctrl_message_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   if(!sd) return;
   evas_object_show(sd->message);
}

void
e_ctrl_contact_add(Evas_Object *obj, Evas_Object *bard)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->contacts = evas_list_prepend(sd->contacts, bard);
}

void
e_ctrl_contact_delete(Evas_Object *obj, Evas_Object *bard)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->contacts = evas_list_remove(sd->contacts, bard);
}

Evas_Object *
e_ctrl_contact_get_by_name(Evas_Object *obj, const char *name)
{
   E_Smart_Data *sd;
   Evas_List *l;

   SMART_CHECK(obj, NULL;);

   for (l = sd->contacts; l; l = l->next)
     {
	Evas_Object *bard = l->data;
	const char *n = e_nav_world_item_neo_other_name_get(bard);

	if (n && name && n[0] == name[0] && strcmp(n, name) == 0)
	  return bard;
     }

   return NULL;
}

Evas_Object *
e_ctrl_contact_get_by_number(Evas_Object *obj, const char *number)
{
   E_Smart_Data *sd;
   Evas_List *l;

   SMART_CHECK(obj, NULL;);

   for (l = sd->contacts; l; l = l->next)
     {
	Evas_Object *bard = l->data;
	const char *p = e_nav_world_item_neo_other_phone_get(bard);

	if (p && number && p[0] == number[0] && strcmp(p, number) == 0)
	  return bard;
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
e_ctrl_object_store_item_add(Evas_Object *obj, void *path, void *item)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   ecore_hash_set(sd->objectStore, strdup(path), item);
}

Evas_Object *
e_ctrl_object_store_item_get(Evas_Object *obj, const char *obj_path)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, NULL;);

   return (Evas_Object *)ecore_hash_get(sd->objectStore, obj_path);
}

void
e_ctrl_object_store_item_remove(Evas_Object *obj, const char *obj_path)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   ecore_hash_remove(sd->objectStore, obj_path);
}
