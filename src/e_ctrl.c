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

#include "e_nav_taglist.h"
#include "e_nav.h"    
#include "e_nav_theme.h"
#include "e_ctrl.h"
#include "e_nav_tileset.h"
#include "e_nav_item_location.h"
#include "e_nav_item_neo_me.h"
#include "e_spreadmenu.h"

#define E_NEW(s, n) (s *)calloc(n, sizeof(s))

static Evas_Object *ctrl = NULL;
static Evas_Object *neo_me = NULL;
static Ecore_Hash *bardRoster = NULL;
static Ecore_Hash *objectStore = NULL;

typedef struct _E_Smart_Data E_Smart_Data;
static void _e_ctrl_cb_signal_drag(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_ctrl_cb_signal_drag_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_ctrl_cb_signal_drag_stop(void *data, Evas_Object *obj, const char *emission, const char *source);

struct _E_Smart_Data
{
   Evas_Object *obj;      
   Evas_Object *clip;
   Evas_Object *map_overlay;   
   Evas_Object *nav;
   Tag_List *listview;   
   Evas_Object *panel_buttons;
   Evas_Object *message;

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
   _e_ctrl_smart_init();
   if(ctrl) return ctrl;
   ctrl = evas_object_smart_add(e, _e_smart);
   objectStore = ecore_hash_new(ecore_str_hash, ecore_str_compare);

   if(!objectStore)
     return NULL; 
   ecore_hash_free_key_cb_set(objectStore, free);

   return ctrl;
}

static void
_e_nav_tag_sel(void *data, void *data2)
{
   Evas_Object *object;
   Diversity_Tag *tag;
   E_Smart_Data *sd; 
   int unread;

   sd = evas_object_smart_data_get(data);
   if(!sd) 
     return;
    
   object = (Evas_Object *)data2;
   tag = e_nav_world_item_location_tag_get(object);

   double lon = e_nav_world_item_location_lon_get(data2);
   double lat = e_nav_world_item_location_lat_get(data2);
   unread = e_nav_world_item_location_unread_get(object); 
   int val = 0;
   if(unread && tag)
     {
        unread = 0;
        val = diversity_dbus_property_set((Diversity_DBus *) tag,
	      DIVERSITY_DBUS_IFACE_TAG, "Unread", DBUS_TYPE_BOOLEAN, &unread);
        if(val)
          e_nav_world_item_location_unread_set(object, 0); 
     }

   e_nav_taglist_deactivate(sd->listview);
   sd->follow = 0;
   e_nav_coord_set(sd->nav, lon, lat, 0.0);
   evas_object_show(sd->nav);
   e_nav_world_item_location_title_show(object);
   evas_object_show(sd->map_overlay);
   if(evas_object_visible_get(sd->message))
     evas_object_raise(sd->message);
   evas_object_raise(object);
   evas_object_show(sd->panel_buttons);
   edje_object_signal_emit(sd->panel_buttons, "JUMP_TO_MAP", "");
}

void
e_ctrl_taglist_tag_set(const char *name, const char *note, void *object)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   e_nav_taglist_tag_update(sd->listview, name, note, object);
}

void
e_ctrl_taglist_tag_add(const char *name, const char *note, time_t timestamp, void *loc_object)
{
   E_Smart_Data *sd;
   Tag_List_Item *item;

   sd = evas_object_smart_data_get(ctrl);

   item = E_NEW(Tag_List_Item, 1);

   if (name)
     item->name = strdup(name); 
   else
     item->name = strdup("");

   if (note)
     item->description = strdup(note);

   item->timestamp = timestamp;
   item->func = _e_nav_tag_sel;
   item->data = ctrl;
   item->data2 = loc_object;

   e_nav_taglist_tag_add(sd->listview, item);
}

void
e_ctrl_taglist_tag_delete(void *loc_object)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   e_nav_taglist_tag_remove(sd->listview, loc_object);
}

int
e_ctrl_contact_add(const char *id, Neo_Other_Data *data)
{
   if(!id) return FALSE;
   return ecore_hash_set(bardRoster, (void *)strdup(id), (void *)data);
}

int
e_ctrl_contact_remove(const char *id)
{
   Neo_Other_Data *neod = ecore_hash_remove(bardRoster, (void *)id);
   if (!neod) return FALSE;
   if (neod->bard) diversity_bard_destroy(neod->bard);
   free(neod);
   return TRUE;
}

int
e_ctrl_contact_update(const char *id, Neo_Other_Data *data)
{
   if(!id || !data) return FALSE;
   if(!e_ctrl_contact_remove(id)) return FALSE;
   return e_ctrl_contact_add(id, data);
}

Neo_Other_Data *
e_ctrl_contact_get(const char *id)
{
   return (Neo_Other_Data *)ecore_hash_get(bardRoster, (void *)id);
}

Neo_Other_Data *
e_ctrl_contact_get_by_name(const char *name)
{
   Ecore_List *cl;
   int lstcount;
   int n;
   Neo_Other_Data *neod;

   cl = e_ctrl_contacts_get();
   lstcount = ecore_list_count(cl);
   for(n=0; n<lstcount; n++)
     {
        neod =  ecore_list_index_goto(cl, n);
        if(neod && !strcmp(neod->name, name))
          {
             ecore_list_destroy(cl);
             return neod;
          }
     }
   ecore_list_destroy(cl);
   return NULL;
}

Neo_Other_Data *
e_ctrl_contact_get_by_number(const char *number)
{
   Ecore_List *cl;
   int lstcount;
   int n;
   Neo_Other_Data *neod;

   cl = e_ctrl_contacts_get();
   lstcount = ecore_list_count(cl);
   for(n=0; n<lstcount; n++)
     {
        neod =  ecore_list_index_goto(cl, n);
        if(neod && !strcmp(neod->phone, number))
          {
             ecore_list_destroy(cl);
             return neod;
          }
     }
   ecore_list_destroy(cl);
   return NULL;
}

Ecore_List *
e_ctrl_contacts_get(void)
{
   Ecore_List *values;
   Ecore_List *keys = ecore_hash_keys(bardRoster);
   int count = ecore_list_count(keys);
   int n;

   values = ecore_list_new();
   Neo_Other_Data *neod;
   for(n=0; n<count; n++)
     {
        char *key = ecore_list_index_goto(keys, n);
        neod = (Neo_Other_Data *)ecore_hash_get(bardRoster, (void *)key);
        if(neod)
          {
             ecore_list_append(values, (void *)neod);
          }
     }
   return values;
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
   evas_object_hide(sd->nav);
   evas_object_hide(sd->map_overlay);
   e_nav_taglist_activate(sd->listview);
   evas_object_raise(sd->panel_buttons);
   evas_object_lower(sd->message);
}

static void   
_e_nav_refresh_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   double lon, lat, w, h;
   E_Smart_Data *sd; 
   sd = evas_object_smart_data_get(data);
   if(!sd) {
       return;
   }

   if (!neo_me) return;

   e_nav_world_item_geometry_get(neo_me, &lon, &lat, &w, &h);
   sd->follow = 1;
   e_nav_coord_set(sd->nav, lon, lat, 0.0);

   e_nav_taglist_deactivate(sd->listview);
   evas_object_show(sd->nav);
   evas_object_show(sd->map_overlay);
   if(evas_object_visible_get(sd->message))
     evas_object_raise(sd->message);
   evas_object_show(sd->panel_buttons);
   evas_object_raise(neo_me);
}

static void
_e_nav_map_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(data);
   e_nav_taglist_deactivate(sd->listview);
   evas_object_show(sd->nav);
   evas_object_show(sd->map_overlay);
   if(evas_object_visible_get(sd->message))
     evas_object_raise(sd->message);
   evas_object_show(sd->panel_buttons);
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

   sd->listview = e_nav_taglist_new(obj, THEMEDIR);

   sd->panel_buttons = e_nav_theme_object_new(evas_object_evas_get(obj), sd->dir,
				      "modules/diversity_nav/panel");
   edje_object_part_text_set(sd->panel_buttons, "refresh_text", _("REFRESH"));
   edje_object_part_text_set(sd->panel_buttons, "map_text", _("MAP"));
   edje_object_part_text_set(sd->panel_buttons, "list_text", _("LIST"));
   evas_object_move(sd->panel_buttons, sd->x, sd->y);
   evas_object_resize(sd->panel_buttons, sd->w, sd->h);

   star = edje_object_part_object_get(sd->panel_buttons, "star_button"); 
   map = edje_object_part_object_get(sd->panel_buttons, "map_button"); 
   refresh = edje_object_part_object_get(sd->panel_buttons, "refresh_button"); 
   list = edje_object_part_object_get(sd->panel_buttons, "list_button"); 

   evas_object_event_callback_add(map, EVAS_CALLBACK_MOUSE_UP,
				  _e_nav_map_button_cb_mouse_down, ctrl);
   evas_object_event_callback_add(refresh, EVAS_CALLBACK_MOUSE_UP,
				  _e_nav_refresh_button_cb_mouse_down, ctrl);
   evas_object_event_callback_add(list, EVAS_CALLBACK_MOUSE_UP,
				  _e_nav_list_button_cb_mouse_down, ctrl);
   evas_object_show(sd->panel_buttons);

   sd->message = e_nav_theme_object_new(evas_object_evas_get(obj), sd->dir,
				      "modules/diversity_nav/message");
   edje_object_part_text_set(sd->message, "message.text", _("Searching for your location"));

   evas_object_move(sd->message, sd->x, sd->y);
   evas_object_resize(sd->message, sd->w, sd->h);
}

void
e_ctrl_nav_set(Evas_Object* obj)
{
   if(!ctrl) return;
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   sd->nav = obj;
}

void
e_ctrl_neo_me_set(Evas_Object *obj)
{
  neo_me = obj;
}

void *
e_ctrl_neo_me_get(void)
{
   return neo_me;
}

Diversity_Equipment *
e_ctrl_self_equipment_get(const char *eqp_name)
{
   Diversity_Bard *self;
   Diversity_Equipment *eqp = NULL;

   self = e_nav_world_item_neo_me_bard_get(neo_me); 
   if (self)
     eqp = diversity_bard_equipment_get(self, eqp_name);

   return eqp;
}

static double
to_zoom(double v)
{
   v = (pow(2.0, v * E_NAV_ZOOM_SENSITIVITY) - 1.0)
      / (1 << E_NAV_ZOOM_SENSITIVITY);

   return E_NAV_ZOOM_MIN + ((E_NAV_ZOOM_MAX - E_NAV_ZOOM_MIN) * v);
}

static double
from_zoom(double z)
{
   z = (z - E_NAV_ZOOM_MIN) / (E_NAV_ZOOM_MAX - E_NAV_ZOOM_MIN);
   if (z < 0.0)
     z = 0.0;
   else if (z > 1.0)
     z = 1.0;

   return log((z * (1 << E_NAV_ZOOM_SENSITIVITY)) + 1)
		/ M_LOG2 / E_NAV_ZOOM_SENSITIVITY;
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

   bardRoster = ecore_hash_new(ecore_str_hash, ecore_str_compare);
   if(!bardRoster) return;
   ecore_hash_free_key_cb_set(bardRoster, free);

   for (i = 0; i < NUM_DRAG_VALUES; i++)
     {
	int span = 256 * (1 << i); /* XXX 256? */
	double zoom = M_EARTH_RADIUS * M_PI * 2 / span;
	double v;

	v = from_zoom(zoom);

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

   if(objectStore)
     {
        ecore_hash_destroy(objectStore);
     }

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

void e_ctrl_zoom_drag_value_set(double z) 
{
   if(!ctrl) return;
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);

   z = from_zoom(z);
   edje_object_part_drag_value_set(sd->map_overlay, "e.dragable.zoom", 0.0, z);
}

void e_ctrl_zoom_text_value_set(const char* buf)
{
   if(!ctrl) return;
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   edje_object_part_text_set(sd->map_overlay, "e.text.zoom", buf);
}

void e_ctrl_longitude_set(const char* buf)
{
   if(!ctrl) return;
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   edje_object_part_text_set(sd->map_overlay, "e.text.longitude", buf);
}

void e_ctrl_latitude_set(const char* buf)
{
   if(!ctrl) return;
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   if(!ctrl) return;
   edje_object_part_text_set(sd->map_overlay, "e.text.latitude", buf);
}

static void
_e_ctrl_cb_signal_drag(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   
   sd = data;
     {
	double x = 0, y = 0, z;
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

	z = to_zoom(y);
	e_nav_zoom_set(sd->nav, z, 0.0);
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
e_ctrl_follow_set(int follow)
{
   E_Smart_Data *sd;
   if(!ctrl) return;
   sd = evas_object_smart_data_get(ctrl);
   if(!sd) return;
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
e_ctrl_object_store_item_add(void *path, void *item)
{
   ecore_hash_set(objectStore, strdup(path), item);
}

Evas_Object *
e_ctrl_object_store_item_get(const char *obj_path)
{
   return (Evas_Object *)ecore_hash_get(objectStore, obj_path);
}

void
e_ctrl_object_store_item_remove(const char *obj_path)
{
   ecore_hash_remove(objectStore, obj_path);
}

