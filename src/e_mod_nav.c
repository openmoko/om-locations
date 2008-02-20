/* e_mod_nav.c -
 *
 * Copyright 2007-2008 OpenMoko, Inc.
 * Authored by Carsten Haitzler <raster@openmoko.org>
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
#include "e_mod_nav.h"
#include "e_spiralmenu.h"
#include "e_nav_item_ap.h"
#include "e_nav_item_neo_me.h"
#include "e_nav_item_neo_other.h"
#include "e_nav_item_link.h"
#include "e_nav_item_location.h"
#include "e_nav_dbus.h"
#include "e_nav_tileset.h"
#include "e_ctrl.h"

/* FIXME: need objects:
 * 
 * link object
 * signal node object
 * 
 * ? user map object
 * ? map note object
 * ? user icon object
 * 
 */

/* create (and destroy) a nav object on the desktop bg */
/* setup and teardown */
static Evas_Object *ctrl = NULL;
static Evas_Object *nav = NULL;
static E_Nav_World *world = NULL;
static E_Nav_Bard *self = NULL;

static void add_city(Evas* evas, double lon, double lat, const char* cityname);
static void test_map(Evas* evas);

static Evas_Object *
theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;
   
   o = edje_object_add(e);
   if (!e_nav_edje_object_set(o, "default", group))
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

static void
city_menu_cb_1(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb1\n");
   e_spiralmenu_deactivate(obj);
}

static void
city_menu_cb_2(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb2\n");
   e_spiralmenu_deactivate(obj);
}

static void
city_menu_cb_3(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb3\n");
   e_spiralmenu_deactivate(obj);
}

static void
city_menu_cb_4(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb4\n");
   e_spiralmenu_deactivate(obj);
}

static void
city_menu_cb_5(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb5\n");
   e_spiralmenu_deactivate(obj);
}

static void
city_menu_cb_6(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb6\n");
   e_spiralmenu_deactivate(obj);
}

static void
city_mouse_cb_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *om;
   
   om = e_spiralmenu_add(evas);
   e_spiralmenu_theme_source_set(om, data);
   e_spiralmenu_autodelete_set(om, 1);
   e_spiralmenu_deacdelete_set(om, 1);
   e_spiralmenu_source_object_set(om, obj);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Silly", city_menu_cb_1, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Fish", city_menu_cb_2, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Onion", city_menu_cb_3, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Eggplant", city_menu_cb_4, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Potatoe", city_menu_cb_5, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Plum", city_menu_cb_6, NULL);
   evas_object_show(om);
   e_spiralmenu_activate(om);
   printf("menu activate for %p = %p\n", obj, om);
}

static void
map_resize(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Coord w, h;
   
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   evas_object_image_fill_set(obj, 0, 0, w, h);
}

static void on_geometry_changed(void *data, DBusMessage *message)
{
   double x, y, w, h;

   if (!dbus_message_get_args(message, NULL,
			      DBUS_TYPE_DOUBLE, &x,
			      DBUS_TYPE_DOUBLE, &y,
			      DBUS_TYPE_DOUBLE, &w,
			      DBUS_TYPE_DOUBLE, &h,
			      DBUS_TYPE_INVALID))
     return;

   printf("geom: %f, %f\n", x, y);
   e_nav_coord_set(nav, x, -y, 0.0);
}

static Evas_Object *
osm_tileset_add(Evas_Object *nav)
{
   E_DBus_Proxy *proxy = NULL;
   Evas_Object *nt = NULL;

   if (world)
     {
	self = e_nav_world_get_self(world);
	if (self)
	  proxy = e_nav_bard_equipment_get(self,
		"osm", "org.openmoko.Diversity.Atlas");
     }


   if (proxy)
     {
	char *path;

	if (e_dbus_proxy_simple_call(proxy, "GetPath",
				     NULL,
				     DBUS_TYPE_INVALID,
				     DBUS_TYPE_STRING, &path,
				     DBUS_TYPE_INVALID))
	  {
	     nt = e_nav_tileset_add(nav,
		   E_NAV_TILESET_FORMAT_OSM, path);
	     e_nav_tileset_proxy_set(nt, proxy);
	     free(path);
	  }
	else
	  e_dbus_proxy_destroy(proxy);
     }

   return nt;
}

void
_e_mod_nav_init(Evas *evas)
{
   Evas_Object *nwi, *nt;

   if (nav) return;

   nav = e_nav_add(evas);
   e_nav_theme_source_set(nav, THEME_PATH);

   e_nav_dbus_init();
   world = e_nav_world_new();

   nt = osm_tileset_add(nav);
   evas_object_show(nt);

     {
	E_DBus_Proxy *proxy;

	proxy = e_nav_bard_object_get(self);
	e_dbus_proxy_connect_signal(proxy, "GeometryChanged", on_geometry_changed, NULL);
     }

   ctrl = e_ctrl_add(evas);
   e_ctrl_theme_source_set(ctrl, THEME_PATH);
   e_ctrl_nav_set(nav);
   evas_object_show(ctrl);

   /* testing items */
   test_map(evas); 
   nwi = theme_obj_new(evas, THEME_PATH, "modules/diversity_nav/city");
   evas_object_event_callback_add(nwi, EVAS_CALLBACK_MOUSE_DOWN,
				  city_mouse_cb_down,
				  THEME_PATH);
   edje_object_part_text_set(nwi, "e.text.name", "Sydney");
   e_nav_world_item_add(nav, nwi);
   e_nav_world_item_type_set(nwi, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(nwi,
				 151.205907, 33.875938,
				 32, 32);
   e_nav_world_item_scale_set(nwi, 0);
   e_nav_world_item_update(nwi);
   evas_object_show(nwi);
   
   nwi = theme_obj_new(evas, THEME_PATH,
		       "modules/diversity_nav/city");
   evas_object_event_callback_add(nwi, EVAS_CALLBACK_MOUSE_DOWN,
				  city_mouse_cb_down,
				  THEME_PATH);
   edje_object_part_text_set(nwi, "e.text.name", "Taipei");
   e_nav_world_item_add(nav, nwi);
   e_nav_world_item_type_set(nwi, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(nwi,
				 121.549644, -25.046414,
				 32, 32);
   e_nav_world_item_scale_set(nwi, 0);
   e_nav_world_item_update(nwi);
   evas_object_show(nwi);
   
   nwi = theme_obj_new(evas, THEME_PATH,
		       "modules/diversity_nav/city");
   evas_object_event_callback_add(nwi, EVAS_CALLBACK_MOUSE_DOWN,
				  city_mouse_cb_down,
				  THEME_PATH);
   edje_object_part_text_set(nwi, "e.text.name", "Kings Cross");
   e_nav_world_item_add(nav, nwi);
   e_nav_world_item_type_set(nwi, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(nwi, 
				 151.223588, 33.873622,
				 32, 32);
   e_nav_world_item_scale_set(nwi, 0);
   e_nav_world_item_update(nwi);
   evas_object_show(nwi);
   
   nwi = evas_object_image_add(evas);
   evas_object_image_file_set(nwi, "/tmp/sydney_city_map.png", NULL);
   evas_object_image_smooth_scale_set(nwi, 0);
   evas_object_event_callback_add(nwi, EVAS_CALLBACK_RESIZE, map_resize, NULL);
   e_nav_world_item_add(nav, nwi);
   e_nav_world_item_type_set(nwi, E_NAV_WORLD_ITEM_TYPE_WALLPAPER);
   e_nav_world_item_geometry_set(nwi, 
				 151.205907, 33.875938, 
				 2 * (151.232171 - 151.205907), 
				 (928 * 2 * (151.232171 - 151.205907)) / 1606);
   e_nav_world_item_scale_set(nwi, 1);
   e_nav_world_item_update(nwi);
   evas_object_show(nwi);

   nwi = e_nav_world_item_ap_add(nav, THEME_PATH, NULL,
				 151.220000, 33.874000);
   e_nav_world_item_ap_essid_set(nwi, "OpenMoko");
   e_nav_world_item_ap_key_type_set(nwi, E_NAV_ITEM_AP_KEY_TYPE_NONE);
   e_nav_world_item_ap_range_set(nwi, 100 NAV_UNIT_M);
    
   nwi = e_nav_world_item_neo_other_add(nav, THEME_PATH, NULL,
				     151.215000, 33.871000);
   e_nav_world_item_neo_other_name_set(nwi, "Sean");
   nwi = e_nav_world_item_neo_other_add(nav, THEME_PATH, NULL,
				     151.213000, 33.874000);
   e_nav_world_item_neo_other_name_set(nwi, "Olv");
   
   /* test NEO ME object */
   nwi = e_nav_world_item_neo_me_add(nav, THEME_PATH,
				     151.210000, 33.870000);
   e_nav_world_item_neo_me_name_set(nwi, "Me");

   /* test location object */
   nwi = e_nav_world_item_location_add(nav, THEME_PATH,
				     151.110000, 33.770000);
   e_nav_world_item_location_name_set(nwi, "New Office");
   e_nav_world_item_location_description_set(nwi, "Our new office will be opening soon. Can't wait\n to move in and throw a big party!");

   /* start off at a zoom level and location instantly */
   e_nav_zoom_set(nav, 5, 0.0);
   e_nav_coord_set(nav, 151.205907, 33.875938, 0.0);
            
   _e_mod_nav_update(evas);
   evas_object_show(nav);
   evas_object_show(ctrl);
}

void
_e_mod_nav_update(Evas *evas)
{
   int w, h;
   
   if (!nav) return;

   evas_object_move(nav, 0, 0);
   evas_output_size_get(evas, &w, &h);
   evas_object_resize(nav, w, h);
   evas_object_resize(ctrl, w, h);
   evas_object_show(nav);
   evas_object_show(ctrl);
}

void
_e_mod_nav_shutdown(void)
{
   if (!nav) return;

   if (world)
     {
	if (self)
	  {
	     e_nav_bard_destroy(self);
	     self = NULL;
	  }
	e_nav_world_destroy(world);
	world = NULL;
     }

   e_nav_dbus_shutdown();

   evas_object_del(nav);
   nav = NULL;
}

// for test
static void test_map(Evas* evas)
{
    add_city(evas, -0.087891, -51.495065, "London");
    add_city(evas, 33.178711, -68.989925, "Mypmahck");
    add_city(evas, 116.28, -39.54, "Beijing");
    add_city(evas, -122.32974, -47.6035, "Seattle");
}

static void add_city(Evas* evas, double lon, double lat, const char* cityname)
{
  Evas_Object*  nwi = theme_obj_new(evas, THEME_PATH, "modules/diversity_nav/city");
   evas_object_event_callback_add(nwi, EVAS_CALLBACK_MOUSE_DOWN,
                                  city_mouse_cb_down,
                                  THEME_PATH);
   edje_object_part_text_set(nwi, "e.text.name", cityname);
   e_nav_world_item_add(nav, nwi);
   e_nav_world_item_type_set(nwi, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(nwi,
                                 lon, lat,
                                 32, 32);
   e_nav_world_item_scale_set(nwi, 0);
   e_nav_world_item_update(nwi);
    evas_object_show(nwi);
}
