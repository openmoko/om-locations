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
static Evas_Object *ctrl  = NULL;
static Evas_Object *nav   = NULL;
static Diversity_World *world = NULL;
static Diversity_Bard  *self  = NULL;
static Diversity_Viewport *worldview = NULL;

static Evas_Object *
osm_tileset_add(Evas_Object *nav)
{
   Diversity_Equipment *eqp = NULL;
   E_DBus_Proxy *proxy = NULL;
   Evas_Object *nt = NULL;

   if (world)
     {
	self = diversity_world_get_self(world);
	if (self)
	  eqp = diversity_bard_equipment_get(self, "osm");
     }


   if (eqp)
     proxy = diversity_dbus_proxy_get((Diversity_DBus *) eqp, DIVERSITY_DBUS_IFACE_ATLAS);

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

static void
viewport_object_added(void *data, DBusMessage *msg)
{
   const char *obj_path;
   DBusError error;
   dbus_error_init(&error);
   if (!dbus_message_get_args(msg, &error,
			      DBUS_TYPE_OBJECT_PATH, &obj_path,
			      DBUS_TYPE_INVALID))
     {
        printf("object added parse error: %s\n", error.message);
	dbus_error_free(&error);
        return;
     }
   else 
     {
        Diversity_Object *obj;
        double lon, lat, width, height;
	int type;

        printf("object added in the viewport, path:%s\n", obj_path);  
	obj = diversity_object_new(obj_path);
	if (!obj)
	  return;

        diversity_object_geometry_get(obj, &lon, &lat, &width, &height);
        printf("location geo get lon:%f lat:%f\n", lon, lat);
       	type = diversity_object_type_get(obj);  
        if(type==DIVERSITY_OBJECT_TYPE_TAG) 
          {
             Evas_Object *loc_obj = e_nav_world_item_location_add(nav, THEME_PATH,
				     lon, lat, obj);
             char *name = NULL;
             char *description = NULL;
             diversity_tag_prop_get((Diversity_Tag *) obj, "description", &description); 
             name = strsep(&description, "\n");
             e_nav_world_item_location_name_set(loc_obj, name);
             e_nav_world_item_location_note_set(loc_obj, description);
             e_ctrl_taglist_tag_add(name, description, loc_obj); 
          }
        else
          printf("other kind of object added\n");
     }
}

void
_e_mod_nav_init(Evas *evas)
{
   Evas_Object *nwi, *nt;

   if (nav) return;

   e_nav_dbus_init();
   world = diversity_world_new();

   nav = e_nav_add(evas, world);
   e_nav_theme_source_set(nav, THEME_PATH);

   nt = osm_tileset_add(nav);
   evas_object_show(nt);

   ctrl = e_ctrl_add(evas);
   e_ctrl_theme_source_set(ctrl, THEME_PATH);
   e_ctrl_nav_set(nav);
   evas_object_show(ctrl);

   if(world) 
     {
	worldview = diversity_world_viewport_add(world, -180, 90, 180, -90); // whole world viewport
	diversity_dbus_signal_connect((Diversity_DBus *) worldview, 
                                      DIVERSITY_DBUS_IFACE_VIEWPORT, 
                                      "ObjectAdded", 
                                      viewport_object_added,
                                      NULL); 
	printf("Create viewport for whole world\n");
	diversity_viewport_start(worldview);
     }

   /* test NEO ME object */
   nwi = e_nav_world_item_neo_me_add(nav, THEME_PATH,
				     151.210000, 33.870000);
   e_nav_world_item_neo_me_name_set(nwi, "Me");
   show_welcome_message(nwi);

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
	     diversity_bard_destroy(self);
	     self = NULL;
	  }
	diversity_world_destroy(world);
	world = NULL;
     }

   e_nav_dbus_shutdown();

   evas_object_del(nav);
   nav = NULL;
}
