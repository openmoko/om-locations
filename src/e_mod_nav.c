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
#include "e_mod_config.h"
#include "e_mod_nav.h"
#include "e_nav_theme.h"
#include "e_spiralmenu.h"
#include "e_nav_item_ap.h"
#include "e_nav_item_neo_me.h"
#include "e_nav_item_neo_other.h"
#include "e_nav_item_link.h"
#include "e_nav_item_location.h"
#include "e_nav_dbus.h"
#include "e_nav_tileset.h"
#include "e_ctrl.h"
#include "widgets/e_nav_alert.h"

typedef struct _E_Module_Data E_Module_Data;

static struct _E_Module_Data {
     Diversity_Nav_Config *cfg;

     Evas_Object          *ctrl;
     Evas_Object          *nav;
     Evas_Object          *tileset; /* owned by nav */
     Evas_Object          *neo_me;

     Diversity_World      *world;
     Diversity_Bard       *self;
     Diversity_Viewport   *worldview;

     Ecore_Timer          *fix_timer;
     unsigned int          fix_msg_id;
} mdata;


static void _e_mod_neo_me_init();
static void on_neo_other_geometry_changed(void *data, DBusMessage *msg);
static void on_neo_other_property_changed(void *data, DBusMessage *msg);
static void position_search_timer_start();
static void position_search_timer_stop();
static void alert_exit(void *data, Evas_Object *obj);
static void alert_gps_turn_on(void *data, Evas_Object *obj);
static void alert_gps_cancel(void *data, Evas_Object *obj);
static int check_gps_state();
static int turn_on_gps();
static int _e_nav_cb_timer_pos_search_pause(void *data);
static int handle_gps(void *data);

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
        Evas_Object *nwi = NULL;
        double lon, lat, width, height;
	int type;
        int secs = 0;
	time_t timep;

        if(e_ctrl_object_store_item_get(mdata.ctrl, obj_path))
          {
             printf("item  %s already existed. ignore\n", obj_path);
             return;
          }

	obj = diversity_object_new(obj_path);
	if (!obj)
	  return;

        diversity_object_geometry_get(obj, &lon, &lat, &width, &height);
	diversity_dbus_property_get((Diversity_DBus *) obj, DIVERSITY_DBUS_IFACE_OBJECT, "Timestamp",  &secs);

        timep = (time_t)secs;

       	type = diversity_object_type_get(obj);  
        if(type==DIVERSITY_OBJECT_TYPE_TAG) 
          {
             char *name = NULL;
             char *description = NULL;
             int unread;

             nwi = e_nav_world_item_location_add(mdata.nav, THEMEDIR,
				     lon, lat, obj);
             diversity_tag_prop_get((Diversity_Tag *) obj, "description", &description); 
	     diversity_dbus_property_get((Diversity_DBus *) obj,
		   DIVERSITY_DBUS_IFACE_TAG, "Unread", &unread);
             e_nav_world_item_location_unread_set(nwi, unread);

             name = strsep(&description, "\n");
             e_nav_world_item_location_name_set(nwi, name);
             e_nav_world_item_location_note_set(nwi, description);
             e_nav_world_item_location_timestamp_set(nwi, timep);
             e_ctrl_taglist_tag_add(mdata.ctrl, nwi);
             e_ctrl_object_store_item_add(mdata.ctrl, (void *)(obj_path), (void *)nwi);           
          }
        else if(type==DIVERSITY_OBJECT_TYPE_BARD) 
          {
             char *name = NULL;
             char *phone = NULL;
             char *alias = NULL;
             char *twitter = NULL;
             int accuracy = DIVERSITY_OBJECT_ACCURACY_NONE;

             diversity_bard_prop_get((Diversity_Bard *) obj, "fullname", &name); 
             diversity_bard_prop_get((Diversity_Bard *) obj, "phone", &phone); 
             diversity_bard_prop_get((Diversity_Bard *) obj, "alias", &alias); 
             diversity_bard_prop_get((Diversity_Bard *) obj, "twitter", &twitter); 

             diversity_dbus_property_get(((Diversity_DBus *)obj), DIVERSITY_DBUS_IFACE_OBJECT, "Accuracy",  &accuracy);

             nwi = e_nav_world_item_neo_other_add(mdata.nav, THEMEDIR, lon, lat, obj);
             e_nav_world_item_neo_other_name_set(nwi, name);
             e_nav_world_item_neo_other_phone_set(nwi, phone);
             e_nav_world_item_neo_other_alias_set(nwi, alias);
             e_nav_world_item_neo_other_twitter_set(nwi, twitter);
             e_ctrl_object_store_item_add(mdata.ctrl, (void *)obj_path, (void *)nwi);           

             printf("Add a bard contact: name:%s, phone:%s, alias:%s, twitter:%s, lon:%f, lat:%f\n", name, phone, alias, twitter, lon, lat);
             e_ctrl_contact_add(mdata.ctrl, nwi);

             diversity_dbus_signal_connect((Diversity_DBus *) obj,
                  DIVERSITY_DBUS_IFACE_OBJECT, "GeometryChanged", on_neo_other_geometry_changed, nwi);
             diversity_dbus_signal_connect((Diversity_DBus *) obj,
                  DIVERSITY_DBUS_IFACE_OBJECT, "PropertyChanged", on_neo_other_property_changed, nwi);            
          }
#if 0
        else if(type==DIVERSITY_OBJECT_TYPE_AP) 
	  {
	     char *ssid;
	     int flags, accuracy;

             diversity_dbus_property_get(((Diversity_DBus *) obj),
		   DIVERSITY_DBUS_IFACE_OBJECT, "Accuracy",  &accuracy);
	     /* XXX */
             if (accuracy == DIVERSITY_OBJECT_ACCURACY_NONE) return;  

	     lon += width / 2;
	     lat += height / 2;
             nwi = e_nav_world_item_ap_add(mdata.nav, THEMEDIR, lon, -lat);
	     e_nav_world_item_ap_range_set(nwi, width / 2);

	     diversity_dbus_property_get((Diversity_DBus *) obj,
		   DIVERSITY_DBUS_IFACE_AP, "Ssid", &ssid);
	     diversity_dbus_property_get((Diversity_DBus *) obj,
		   DIVERSITY_DBUS_IFACE_AP, "Flags", &flags);

	     e_nav_world_item_ap_essid_set(nwi, ssid);
	     if (flags)
	       e_nav_world_item_ap_key_type_set(nwi,
		     E_NAV_ITEM_AP_KEY_TYPE_WEP);

	     free(ssid);
	  }
#endif
        else
          printf("other kind of object added\n");

        if (mdata.neo_me)
	  e_nav_world_item_raise(mdata.neo_me);
     }
}

static void
viewport_object_removed(void *data, DBusMessage *msg)
{
   const char *obj_path;
   Evas_Object *world_item;
   DBusError error;
   dbus_error_init(&error);
   if (!dbus_message_get_args(msg, &error,
			      DBUS_TYPE_OBJECT_PATH, &obj_path,
			      DBUS_TYPE_INVALID))
     {
        printf("object removed parse error: %s\n", error.message);
	dbus_error_free(&error);
        return;
     }
   else 
     {
        printf("object deleted: %s \n", obj_path);
        world_item = e_ctrl_object_store_item_get(mdata.ctrl, obj_path);
        if(world_item) 
          {
	     /* item type? */
	     e_ctrl_contact_delete(mdata.ctrl, world_item);

             e_ctrl_object_store_item_remove(mdata.ctrl, obj_path);
             e_nav_world_item_delete(mdata.nav, world_item);
             evas_object_del(world_item);
          }
        else 
          printf("Can not find object %s in hash\n", obj_path);
     }
}


/* 
 * neo_other geometry changed cb function 
 */
static void
on_neo_other_geometry_changed(void *data, DBusMessage *msg)
{
   Evas_Object *nwi = data;
   DBusError err;
   double lon, lat;
   double dummy1, dummy2;
   Evas_Coord w, h; 

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_DOUBLE, &lon, DBUS_TYPE_DOUBLE, &lat,
       DBUS_TYPE_DOUBLE, &dummy1, DBUS_TYPE_DOUBLE, &dummy2, DBUS_TYPE_INVALID);
   if (dbus_error_is_set(&err)) {
      printf("Error: %s - %s\n", err.name, err.message);
      return;
   }

   evas_object_geometry_get(edje_object_part_object_get(nwi, "phone"), NULL, NULL, &w, &h);
   e_nav_world_item_geometry_set(nwi, lon, lat, w, h);
   e_nav_world_item_update(nwi);
}

/* 
 * neo_other property changed cb function 
 */
static void
on_neo_other_property_changed(void *data, DBusMessage *msg)
{
   Evas_Object *neo_other_obj;
   DBusMessageIter args;
   DBusMessageIter subargs;
   void *name;
   void *value;
   int type;

   /* Parse the dbus signal message, get property name and value */
   if (!dbus_message_iter_init(msg, &args))
     return;
   if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
     return;
   dbus_message_iter_get_basic(&args, &name);
   if(!dbus_message_iter_has_next(&args)) return;
   dbus_message_iter_next(&args);
   if(dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_VARIANT)
     return;
   dbus_message_iter_recurse(&args, &subargs);
   type = dbus_message_iter_get_arg_type(&subargs);
   if (!dbus_type_is_basic(type))
     return;
   dbus_message_iter_get_basic(&subargs, &value); 

   if(!data) return;
   neo_other_obj = data;

   if(!strcasecmp(name, "fullname"))
     e_nav_world_item_neo_other_name_set(neo_other_obj, value);
   if(!strcasecmp(name, "phone"))
     e_nav_world_item_neo_other_phone_set(neo_other_obj, value);
   if(!strcasecmp(name, "alias"))
     e_nav_world_item_neo_other_alias_set(neo_other_obj, value);
   if(!strcasecmp(name, "twitter"))
     e_nav_world_item_neo_other_twitter_set(neo_other_obj, value);
}

/* 
 * neo_me geometry changed cb function 
 */
static void
on_geometry_changed(void *data, DBusMessage *msg)
{
   Evas_Object *nwi;
   DBusError err;
   double lon, lat;
   double dummy1, dummy2;
   int follow;
   Evas_Coord w, h; 

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_DOUBLE, &lon, DBUS_TYPE_DOUBLE, &lat,
       DBUS_TYPE_DOUBLE, &dummy1, DBUS_TYPE_DOUBLE, &dummy2, DBUS_TYPE_INVALID);
   if (dbus_error_is_set(&err)) {
      printf("Error: %s - %s\n", err.name, err.message);
      return;
   }

   if (!mdata.neo_me)
     _e_mod_neo_me_init(); 

   nwi = mdata.neo_me;

   evas_object_geometry_get(edje_object_part_object_get(nwi, "phone"), NULL, NULL, &w, &h);
   e_nav_world_item_geometry_set(nwi, lon, lat, w, h);
   e_nav_world_item_update(nwi);

   follow = e_ctrl_follow_get(mdata.ctrl);
   if (follow) {
     e_nav_coord_set(mdata.nav, lon, lat, 0.0);
   }
}

/* 
 * neo_me property changed cb function 
 */
static void
on_property_changed(void *data, DBusMessage *msg)
{
   Evas_Object *nwi;
   int accuracy = 0;
   static int fixed = 0;
   double lon, lat;
   double dummy1, dummy2;

   diversity_dbus_property_get(((Diversity_DBus *) mdata.self),
	 DIVERSITY_DBUS_IFACE_OBJECT, "Accuracy",  &accuracy);

   if (!mdata.neo_me && accuracy != DIVERSITY_OBJECT_ACCURACY_NONE)
     {
        diversity_object_geometry_get((Diversity_Object *) mdata.self,
                                      &lon, &lat, &dummy1, &dummy2);
        _e_mod_neo_me_init();
     }

   if (!mdata.neo_me) return;

   nwi = mdata.neo_me;
     
   if( fixed && accuracy == DIVERSITY_OBJECT_ACCURACY_NONE )
     {
        e_nav_world_item_neo_me_fixed_set(nwi, 0);   
        fixed = 0;
     }
   else if ( !fixed && accuracy != DIVERSITY_OBJECT_ACCURACY_NONE )
     {
        e_nav_world_item_neo_me_fixed_set(nwi, 1);
        fixed = 1;
        position_search_timer_stop();
     }
}

static int 
handle_gps(void *data)
{
   int gps_state;
   gps_state = check_gps_state();
   if(!gps_state)
     {
        Evas_Object *alert_dialog;
    
        alert_dialog = e_alert_add(evas_object_evas_get(mdata.nav));
        e_alert_theme_source_set(alert_dialog, THEMEDIR);
	e_alert_transient_for_set(alert_dialog, mdata.nav);
        e_alert_title_set(alert_dialog, _("GPS is off"), _("Turn on GPS?"));
        e_alert_title_color_set(alert_dialog, 255, 0, 0, 255);
        e_alert_button_add(alert_dialog, _("Yes"), alert_gps_turn_on, alert_dialog);
        e_alert_button_add(alert_dialog, _("No"), alert_gps_cancel, alert_dialog);
        e_alert_activate(alert_dialog); 
        evas_object_show(alert_dialog);
     }
   else
     {
        if(!e_nav_world_item_neo_me_fixed_get(mdata.neo_me))
          position_search_timer_start();
     }
   return FALSE;
}

static void
alert_gps_turn_on(void *data, Evas_Object *obj)
{
   int ret;
   e_alert_deactivate(obj);
   ret = turn_on_gps();
   if(ret)
     position_search_timer_start();
}

static void
alert_gps_cancel(void *data, Evas_Object *obj)
{
   e_alert_deactivate(obj);
}

static void
alert_exit(void *data, Evas_Object *obj)
{
   e_alert_deactivate(obj);
   position_search_timer_stop();
}

#define GPS_DEVICE_NAME "/sys/bus/platform/drivers/neo1973-pm-gps/neo1973-pm-gps.0/pwron"

static int
check_gps_state()
{
   FILE *gpsd; 
   int ret;
   char data;
   gpsd = fopen(GPS_DEVICE_NAME, "r");
   if(!gpsd) 
     {
        printf("Open gps device %s failed.\n", GPS_DEVICE_NAME);
        return FALSE;
     }
   ret=fread(&data, sizeof(char), sizeof(&data), gpsd);
   fclose(gpsd);

   if(ret <=  0) 
     return FALSE;

   if(data == '1')
     return TRUE; 
   else
     return FALSE;
} 

static int
turn_on_gps()
{
   FILE *gpsd; 
   gpsd = fopen(GPS_DEVICE_NAME, "w");
   if(!gpsd) 
     {
        printf("Open gps device %s to write failed.\n", GPS_DEVICE_NAME);
        return FALSE;
     }
   char *on = "1\n";
   fwrite(on, sizeof(char), strlen(on), gpsd);
   fclose(gpsd);
   return TRUE;
}

static void
position_search_timer_start()
{
   mdata.fix_msg_id = e_ctrl_message_text_add(mdata.ctrl,
	 _("Searching for your location"), 0.0);

   mdata.fix_timer = ecore_timer_add(60.0,
                           _e_nav_cb_timer_pos_search_pause,
                           NULL);
}

static void
position_search_timer_stop()
{
   if (mdata.fix_timer)
     {
	ecore_timer_del(mdata.fix_timer);
	mdata.fix_timer = NULL;
     }

   if (mdata.fix_msg_id)
     {
	e_ctrl_message_text_del(mdata.ctrl, mdata.fix_msg_id);
	mdata.fix_msg_id = 0;
     }
}

static int
_e_nav_cb_timer_pos_search_pause(void *data)
{
   Evas_Object *alert_dialog;
   int fix_status;
    
   alert_dialog = e_alert_add(evas_object_evas_get(mdata.nav));
   e_alert_theme_source_set(alert_dialog, THEMEDIR);
   e_alert_transient_for_set(alert_dialog, mdata.nav);
   fix_status = e_nav_world_item_neo_me_fixed_get(mdata.neo_me);
   if(fix_status)
     {
        e_alert_title_set(alert_dialog, _("GPS FIX"), _("Your approximate location"));
        e_alert_title_color_set(alert_dialog, 0, 255, 0, 255);
        e_alert_button_add(alert_dialog, _("OK"), alert_exit, alert_dialog);
     }
   else
     {
        e_alert_title_set(alert_dialog, _("ERROR"), _("Unable to locate a fix"));
        e_alert_title_color_set(alert_dialog, 255, 0, 0, 255);
        e_alert_button_add(alert_dialog, _("OK"), alert_exit, alert_dialog);
     }
   e_alert_activate(alert_dialog); 
   evas_object_show(alert_dialog);

   mdata.fix_timer = NULL;

   return 0;
}

static E_DBus_Proxy *
_dbus_atlas_proxy_get(void)
{
   Diversity_Equipment *eqp;
   E_DBus_Proxy *proxy;

   eqp = diversity_bard_equipment_get(mdata.self, "osm");
   if (!eqp)
     return NULL;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) eqp, DIVERSITY_DBUS_IFACE_ATLAS);
   if (proxy)
     proxy = e_dbus_proxy_new_from_proxy(proxy, NULL, NULL);

   diversity_equipment_destroy(eqp);

   return proxy;
}

/* 
 * Create neo_me evas object on map 
 */
static void
_e_mod_neo_me_init()
{
   Evas_Object *nwi;
   double neo_me_lat, neo_me_lon;

   if (mdata.neo_me) return;

   neo_me_lat = dn_config_float_get(mdata.cfg, "neo_me_lat");
   neo_me_lon = dn_config_float_get(mdata.cfg, "neo_me_lon");
   nwi = e_nav_world_item_neo_me_add(mdata.nav, THEMEDIR,
				     neo_me_lon, neo_me_lat, mdata.self);

   if (mdata.self)
     {
	int accuracy;

	accuracy = DIVERSITY_OBJECT_ACCURACY_NONE;
	diversity_dbus_property_get(((Diversity_DBus *) mdata.self),
	      DIVERSITY_DBUS_IFACE_OBJECT, "Accuracy",  &accuracy);

	if (accuracy != DIVERSITY_OBJECT_ACCURACY_NONE)
	  e_nav_world_item_neo_me_fixed_set(nwi, 1);
     }

   e_nav_world_item_neo_me_name_set(nwi, _("Me"));
   e_nav_world_item_neo_me_activate(nwi);

   mdata.neo_me = nwi;
}

static void
_e_mod_nav_dbus_shutdown(void)
{
   if (mdata.worldview)
     {
	diversity_viewport_destroy(mdata.worldview);

	mdata.worldview = NULL;
     }
   if (mdata.self)
     {
	diversity_bard_destroy(mdata.self);

	mdata.self = NULL;
     }
   if (mdata.world)
     {
	diversity_world_snapshot(mdata.world);
	diversity_world_destroy(mdata.world);

	mdata.world = NULL;
     }

   e_nav_dbus_shutdown();
}

static int
_e_mod_nav_dbus_init(void)
{
   Diversity_Equipment *eqp;

   if (!e_nav_dbus_init())
     return 0;

   mdata.world = diversity_world_new();
   if (!mdata.world)
     goto fail;

   mdata.self = diversity_world_get_self(mdata.world);
   if (!mdata.self)
     goto fail;

   mdata.worldview =
      diversity_world_viewport_add(mdata.world, -180.0, -90.0, 180.0, 90.0); 
   if (!mdata.worldview)
     goto fail;

   diversity_dbus_signal_connect((Diversity_DBus *) mdata.worldview, 
	 DIVERSITY_DBUS_IFACE_VIEWPORT, 
	 "ObjectAdded", 
	 viewport_object_added,
	 NULL); 
   diversity_dbus_signal_connect((Diversity_DBus *) mdata.worldview, 
	 DIVERSITY_DBUS_IFACE_VIEWPORT, 
	 "ObjectRemoved", 
	 viewport_object_removed,
	 NULL); 

   eqp = diversity_bard_equipment_get(mdata.self, "nmea");
   if (eqp)
     {
	const char *dev;

	dev = "/dev/ttySAC1:9600";
	if (diversity_equipment_config_set(eqp, "device-path",
		 DBUS_TYPE_STRING, &dev))
	  {
#if 0
	     dev = "/tmp/nmea.log";
	     diversity_equipment_config_set(eqp, "log",
		   DBUS_TYPE_STRING, &dev);
#endif

	     diversity_dbus_signal_connect((Diversity_DBus *) mdata.self,
		   DIVERSITY_DBUS_IFACE_OBJECT,
		   "GeometryChanged", on_geometry_changed, NULL);
	     diversity_dbus_signal_connect((Diversity_DBus *) mdata.self,
		   DIVERSITY_DBUS_IFACE_OBJECT,
		   "PropertyChanged", on_property_changed, NULL);
	  }

	diversity_equipment_destroy(eqp);
     }

   diversity_viewport_start(mdata.worldview);

   return 1;

fail:
   _e_mod_nav_dbus_shutdown();

   return 0;
}

void
_e_mod_nav_init(Evas *evas, Diversity_Nav_Config *cfg)
{
   const char *theme, *tile_path;
   double lon, lat;
   int span;
   int standalone;

   if (mdata.nav)
     return;

   mdata.cfg = cfg;

   theme = dn_config_string_get(mdata.cfg, "theme");
   e_nav_theme_init(theme);

   mdata.nav = e_nav_add(evas);
   if (!mdata.nav)
     return;

   e_nav_theme_source_set(mdata.nav, THEMEDIR);

   mdata.ctrl = e_ctrl_add(evas);
   if (!mdata.ctrl)
     return;

   e_ctrl_theme_source_set(mdata.ctrl, THEMEDIR);

   e_ctrl_nav_set(mdata.ctrl, mdata.nav);
   e_nav_world_ctrl_set(mdata.nav, mdata.ctrl);

   lon = dn_config_float_get(mdata.cfg, "lon");
   lat = dn_config_float_get(mdata.cfg, "lat");

   span = dn_config_int_get(mdata.cfg, "span");
   if (span < E_NAV_SPAN_MIN)
     span = E_NAV_SPAN_MIN;

   tile_path = dn_config_string_get(mdata.cfg, "tile_path");
   mdata.tileset = e_nav_tileset_add(mdata.nav,
	 E_NAV_TILESET_FORMAT_OSM, tile_path);
   if (mdata.tileset)
     {
	e_nav_tileset_monitor_add(mdata.tileset, tile_path);
	e_nav_tileset_monitor_add(mdata.tileset, MAPSDIR);

	/* known places where maps are stored */
	e_nav_tileset_monitor_add(mdata.tileset, "/usr/share/om-maps");
	e_nav_tileset_monitor_add(mdata.tileset, "/usr/local/share/om-maps");
	e_nav_tileset_monitor_add(mdata.tileset, "/media/card/om-maps");
	e_nav_tileset_monitor_add(mdata.tileset, "/usr/share/diversity-nav/maps");

	evas_object_show(mdata.tileset);
     }

   standalone = dn_config_int_get(mdata.cfg, "standalone");
   if (!standalone && _e_mod_nav_dbus_init())
     {
	e_nav_world_set(mdata.nav, mdata.world);

	if (mdata.tileset)
	  {
	     E_DBus_Proxy *atlas;

	     atlas = _dbus_atlas_proxy_get();
	     if (atlas)
	       e_nav_tileset_proxy_set(mdata.tileset, atlas);
	  }

     }

   _e_mod_neo_me_init();
   e_nav_world_item_geometry_get(mdata.neo_me, &lon, &lat, NULL, NULL);

   e_nav_coord_set(mdata.nav, lon, lat, 0.0);
   e_nav_span_set(mdata.nav, span, 0.0);

   _e_mod_nav_update(evas);

   ecore_timer_add(2.0, handle_gps, NULL);
}

void
_e_mod_nav_update(Evas *evas)
{
   int w, h;
   
   if (!mdata.nav) return;

   evas_object_move(mdata.nav, 0, 0);
   evas_output_size_get(evas, &w, &h);
   evas_object_resize(mdata.nav, w, h);
   evas_object_resize(mdata.ctrl, w, h);
   evas_object_show(mdata.nav);
   evas_object_show(mdata.ctrl);
}

void
_e_mod_nav_shutdown(void)
{
   double lon, lat;
   int span;

   if (!mdata.nav)
     return;

   lon = e_nav_coord_lon_get(mdata.nav);
   lat = e_nav_coord_lat_get(mdata.nav);
   span = e_nav_span_get(mdata.nav);

   dn_config_float_set(mdata.cfg, "lon", lon);
   dn_config_float_set(mdata.cfg, "lat", lat);
   dn_config_int_set(mdata.cfg, "span", span);

   e_nav_world_item_geometry_get(mdata.neo_me, &lon, &lat, NULL, NULL);
   dn_config_float_set(mdata.cfg, "neo_me_lon", lon);
   dn_config_float_set(mdata.cfg, "neo_me_lat", lat);

   if (mdata.tileset)
     {
	E_DBus_Proxy *atlas;

	atlas = e_nav_tileset_proxy_get(mdata.tileset);
	if (atlas)
	  {
	     e_nav_tileset_proxy_set(mdata.tileset, NULL);
	     e_dbus_proxy_destroy(atlas);
	  }
     }
   evas_object_del(mdata.neo_me);

   _e_mod_nav_dbus_shutdown();

   /* FIXME xxx_ctrl in e_nav_item_location.c */
   //evas_object_del(mdata.ctrl);

   evas_object_del(mdata.nav);
   mdata.nav = NULL;
}
