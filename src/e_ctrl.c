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

#include "e_nav.h"    
#include "e_ctrl.h"
#include "e_nav_tileset.h"
#include "e_nav_item_location.h"
#include "e_nav_taglist.h"
#include "e_spreadmenu.h"

static Evas_Object *ctrl = NULL;
static Diversity_Bard *self = NULL;
static Ecore_Hash *bardRoster = NULL;

typedef struct _E_Smart_Data E_Smart_Data;
static Evas_Object * _e_ctrl_theme_obj_new(Evas *e, const char *custom_dir, const char *group);
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

   Evas_Coord x, y, w, h;
   const char      *dir;
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
   return ctrl;
}

static void
_e_nav_tag_sel(void *data, void *data2)
{
   E_Smart_Data *sd; 
   sd = evas_object_smart_data_get(data);
   if(!sd) {
       printf("sd is NULL\n");
       return;
   }
    
   double lon = e_nav_world_item_location_lon_get(data2);
   double lat = e_nav_world_item_location_lat_get(data2);
   e_nav_taglist_deactivate(sd->listview);
   e_nav_coord_set(sd->nav, lon, lat, 0.0);
   evas_object_show(sd->nav);
   evas_object_show(sd->map_overlay);
   evas_object_show(sd->panel_buttons);
}

void
e_ctrl_taglist_tag_set(const char *name, const char *note, void *object)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   e_nav_taglist_tag_update(sd->listview, name, note, object);
}

void
e_ctrl_taglist_tag_add(const char *name, const char *note, void *loc_object)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   e_nav_taglist_tag_add(sd->listview, name, note, _e_nav_tag_sel, ctrl, loc_object);
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
   return ecore_hash_set(bardRoster, (void *)id, (void *)data);
}

Neo_Other_Data *
e_ctrl_contact_get(const char *id)
{
   return (Neo_Other_Data *)ecore_hash_get(bardRoster, (void *)id);
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
          ecore_list_append(values, (void *)neod->name);
     }
   return values;
}

void
e_ctrl_contact_remove(const char *id)
{
   Neo_Other_Data *neod = ecore_hash_remove(bardRoster, (void *)id);
   if (!neod) return;
   if (neod->bard) diversity_bard_destroy(neod->bard);
   free(neod);
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
}

static void   
_e_nav_refresh_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   double lon, lat, w, h;
   int accuracy;
   E_Smart_Data *sd; 
   sd = evas_object_smart_data_get(data);
   if(!sd) {
       return;
   }

   if (!self) return;
   diversity_dbus_property_get(((Diversity_DBus *)self), DIVERSITY_DBUS_IFACE_OBJECT, "Accuracy",  &accuracy);
   /* if not fixed yet, no action */
   if(accuracy == DIVERSITY_OBJECT_ACCURACY_NONE) return;   
   diversity_object_geometry_get((Diversity_Object *)self, &lon, &lat, &w, &h);
   lat = -lat;
   // ToDo:  set Follow me flag
   e_nav_coord_set(sd->nav, lon, lat, 0.0);
   e_nav_taglist_deactivate(sd->listview);
   evas_object_show(sd->nav);
   evas_object_show(sd->map_overlay);
   evas_object_show(sd->panel_buttons);
}

static void
_e_nav_map_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(data);
   e_nav_taglist_deactivate(sd->listview);
   evas_object_show(sd->nav);
   evas_object_show(sd->map_overlay);
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
   sd->map_overlay = _e_ctrl_theme_obj_new(evas_object_evas_get(obj), sd->dir,
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

   sd->listview = e_nav_taglist_new(obj, THEME_PATH);

   sd->panel_buttons = _e_ctrl_theme_obj_new(evas_object_evas_get(obj), sd->dir,
				      "modules/diversity_nav/panel");
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
}

static Evas_Object *
_e_ctrl_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;
   
   o = edje_object_add(e);
   if (!e_ctrl_edje_object_set(o, "default", group))
     {
	if (custom_dir)
	  {
	     char buf[PATH_MAX];
	     
	     snprintf(buf, sizeof(buf), "%s/default.edj", custom_dir);
	     edje_object_file_set(o, buf, group);
	  }
     }
   return o;
}

int
e_ctrl_edje_object_set(Evas_Object *o, const char *category, const char *group)
{
   char buf[PATH_MAX];
   int ok;
   
   snprintf(buf, sizeof(buf), "%s/%s.edj", THEME_PATH, category);
   ok = edje_object_file_set(o, buf, group);
   return ok;
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
e_ctrl_self_set(void* obj)
{
   self = (Diversity_Bard *)obj;
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
}

static void
_e_ctrl_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_del(sd->clip);
   evas_object_del(sd->map_overlay);
   evas_object_del(sd->panel_buttons);
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

void e_ctrl_zoom_drag_value_set(double y) 
{
   if(!ctrl) return;
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   edje_object_part_drag_value_set(sd->map_overlay, "e.dragable.zoom", 0.0, y);
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
	
	edje_object_part_drag_value_get(sd->map_overlay, "e.dragable.zoom", &x, &y);

	y = (pow(2.0, y * E_NAV_ZOOM_SENSITIVITY) - 1.0)
		/ (1 << E_NAV_ZOOM_SENSITIVITY);

	z = E_NAV_ZOOM_MIN + ((E_NAV_ZOOM_MAX - E_NAV_ZOOM_MIN) * y);
	e_nav_zoom_set(sd->nav, z, 0.2);
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
