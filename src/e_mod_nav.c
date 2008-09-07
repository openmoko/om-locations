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

#include <Etk.h>

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
static Evas_Object *ctrl   = NULL;
static Evas_Object *nav    = NULL;
static Evas_Object *neo_me = NULL;
static Diversity_Nav_Config *cfg = NULL; 
Ecore_Timer *timer = NULL;

static Diversity_World *world = NULL;
static Diversity_Bard  *self  = NULL;
static Diversity_Viewport *worldview = NULL;
static void _e_mod_neo_me_init();
static void on_neo_other_geometry_changed(void *data, DBusMessage *msg);
static void on_neo_other_property_changed(void *data, DBusMessage *msg);
static void position_search_timer_start();
static void position_search_timer_stop();
static void alert_exit(void *data, Evas_Object *obj, Evas_Object *src_obj);
static void alert_gps_turn_on(void *data, Evas_Object *obj, Evas_Object *src_obj);
static void alert_gps_cancel(void *data, Evas_Object *obj, Evas_Object *src_obj);
static int check_gps_state();
static int turn_on_gps();
static int _e_nav_cb_timer_pos_search_pause(void *data);
static int handle_gps(void *data);

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
        else
          {
             return NULL;
          }
     }


   if (eqp)
     proxy = diversity_dbus_proxy_get((Diversity_DBus *) eqp, DIVERSITY_DBUS_IFACE_ATLAS);

   if (proxy)
     {
	const char *path;

	path = dn_config_string_get(cfg, "tile_path");
	nt = e_nav_tileset_add(nav,
	      E_NAV_TILESET_FORMAT_OSM, path);
	e_nav_tileset_proxy_set(nt, proxy);
	e_nav_tileset_monitor_add(nt, path);
     }

   if (!nt)
     nt = e_nav_tileset_add(nav, E_NAV_TILESET_FORMAT_OSM, NULL);

   return nt;
}

static void
viewport_object_added(void *data, DBusMessage *msg)
{
   const char *obj_path;
   int ok;
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

        if(e_ctrl_object_store_item_get(obj_path))
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

             nwi = e_nav_world_item_location_add(nav, THEMEDIR,
				     lon, lat, obj);
             diversity_tag_prop_get((Diversity_Tag *) obj, "description", &description); 
	     diversity_dbus_property_get((Diversity_DBus *) obj,
		   DIVERSITY_DBUS_IFACE_TAG, "Unread", &unread);
             e_nav_world_item_location_unread_set(nwi, unread);

             name = strsep(&description, "\n");
             e_nav_world_item_location_name_set(nwi, name);
             e_nav_world_item_location_note_set(nwi, description);
             e_nav_world_item_location_timestamp_set(nwi, timep);
             e_ctrl_taglist_tag_add(name, description, timep, nwi); 
             e_ctrl_object_store_item_add((void *)(obj_path), (void *)nwi);           
          }
        else if(type==DIVERSITY_OBJECT_TYPE_BARD) 
          {
             char *name = NULL;
             char *phone = NULL;
             char *alias = NULL;
             char *twitter = NULL;
             int accuracy = DIVERSITY_OBJECT_ACCURACY_NONE;
             Neo_Other_Data *neod;

             diversity_bard_prop_get((Diversity_Bard *) obj, "fullname", &name); 
             diversity_bard_prop_get((Diversity_Bard *) obj, "phone", &phone); 
             diversity_bard_prop_get((Diversity_Bard *) obj, "alias", &alias); 
             diversity_bard_prop_get((Diversity_Bard *) obj, "twitter", &twitter); 
             neod = calloc(1, sizeof(Neo_Other_Data));
             if (!neod) return;
             if(name) neod->name = strdup(name);
             if(phone) neod->phone = strdup(phone);
             if(alias) neod->alias = strdup(alias);
             if(twitter) neod->twitter = strdup(twitter);
             neod->bard = (Diversity_Bard *) obj;
             printf("Add a bard contact: name:%s, phone:%s, alias:%s, twitter:%s, lon:%f, lat:%f\n", name, phone, alias, twitter, lon, lat);
             ok = e_ctrl_contact_add(obj_path, neod);
             if(!ok) printf("there is an error on add bard contact for %s\n", obj_path);

             diversity_dbus_property_get(((Diversity_DBus *)obj), DIVERSITY_DBUS_IFACE_OBJECT, "Accuracy",  &accuracy);

             nwi = e_nav_world_item_neo_other_add(nav, THEMEDIR, lon, lat, obj);
             e_nav_world_item_neo_other_name_set(nwi, name);
             e_nav_world_item_neo_other_phone_set(nwi, phone);
             e_nav_world_item_neo_other_alias_set(nwi, alias);
             e_nav_world_item_neo_other_twitter_set(nwi, twitter);
             e_ctrl_object_store_item_add((void *)obj_path, (void *)nwi);           
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
             nwi = e_nav_world_item_ap_add(nav, THEMEDIR, lon, -lat);
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

        if(neo_me)
          evas_object_raise(neo_me);
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
        if(e_ctrl_contact_get(obj_path))
          e_ctrl_contact_remove(obj_path);
        world_item = e_ctrl_object_store_item_get(obj_path);
        if(world_item) 
          {
             e_ctrl_object_store_item_remove(obj_path);
             e_nav_world_item_delete(nav, world_item);
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
   Neo_Other_Data *neod; 
   const char *path;

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
   path = e_nav_world_item_neo_other_path_get(neo_other_obj);
   neod = e_ctrl_contact_get(path);
   if (!neod) return;

   if(!strcasecmp(name, "fullname"))
     {
        e_nav_world_item_neo_other_name_set(neo_other_obj, value);
        if(neod->name) free((char *)neod->name);
        neod->name = strdup(value);
     }
   if(!strcasecmp(name, "phone"))
     {
        e_nav_world_item_neo_other_phone_set(neo_other_obj, value);
        if(neod->phone) free((char *)neod->phone);
        neod->phone = strdup(value);
     }
   if(!strcasecmp(name, "alias"))
     {
        e_nav_world_item_neo_other_alias_set(neo_other_obj, value);
        if(neod->alias) free((char *)neod->alias);
        neod->alias = strdup(value);
     }
   if(!strcasecmp(name, "twitter"))
     {
        e_nav_world_item_neo_other_twitter_set(neo_other_obj, value);
        if(neod->twitter) free((char *)neod->twitter);
        neod->twitter = strdup(value);
     }
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

   dn_config_float_set(cfg, "neo_me_lon", lon);
   dn_config_float_set(cfg, "neo_me_lat", lat);

   if(!neo_me)
     _e_mod_neo_me_init(); 

   nwi = neo_me;

   evas_object_geometry_get(edje_object_part_object_get(nwi, "phone"), NULL, NULL, &w, &h);
   e_nav_world_item_geometry_set(nwi, lon, lat, w, h);
   e_nav_world_item_update(nwi);

   follow = e_ctrl_follow_get(ctrl);
   if (follow) {
     e_nav_coord_set(nav, lon, lat, 0.0);
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

   diversity_dbus_property_get(((Diversity_DBus *)self), DIVERSITY_DBUS_IFACE_OBJECT, "Accuracy",  &accuracy);

   if(!neo_me && accuracy != DIVERSITY_OBJECT_ACCURACY_NONE)
     {
        diversity_object_geometry_get((Diversity_Object *) self,
                                      &lon, &lat, &dummy1, &dummy2);
        dn_config_float_set(cfg, "neo_me_lon", lon);
        dn_config_float_set(cfg, "neo_me_lat", lat);
        _e_mod_neo_me_init();
     }

   if(!neo_me) return;

   nwi = neo_me;
     
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
        e_ctrl_message_hide(ctrl);
     }
}

static void
on_daemon_dead_confirm(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   e_alert_deactivate(obj);

   _e_mod_nav_shutdown();
   ecore_main_loop_quit();
}

static void
on_daemon_dead(void *data)
{
   Evas_Object *ad;

   ad = e_alert_add(evas_object_evas_get(nav));
   e_alert_theme_source_set(ad, THEMEDIR);
   e_alert_source_object_set(ad, nav);
   e_alert_title_set(ad, _("ERROR"), _("DBus connection closed"));
   e_alert_title_color_set(ad, 255, 0, 0, 255);

   e_alert_button_add(ad, _("Exit"), on_daemon_dead_confirm, NULL);

   evas_object_show(ad);
   e_alert_activate(ad);
}

void
_e_mod_nav_init(Evas *evas, Diversity_Nav_Config *cfg_local)
{
   Evas_Object *nt;
   double lat, lon, scale;
   double neo_me_lat, neo_me_lon;
   const char *theme_name;

   if (nav) return;
   cfg = cfg_local;

   theme_name = dn_config_string_get(cfg, "theme");
   e_nav_theme_init(theme_name);

   e_nav_dbus_init(on_daemon_dead, NULL);
   world = diversity_world_new();

   nav = e_nav_add(evas, world);
   e_nav_theme_source_set(nav, THEMEDIR);

   nt = osm_tileset_add(nav);
   if (nt)
     {
	e_nav_tileset_monitor_add(nt, MAPSDIR);

	/* known places where maps are stored */
	e_nav_tileset_monitor_add(nt, "/usr/share/om-maps");
	e_nav_tileset_monitor_add(nt, "/usr/local/share/om-maps");
	e_nav_tileset_monitor_add(nt, "/media/card/om-maps");
	e_nav_tileset_monitor_add(nt, "/usr/share/diversity-nav/maps");
     }
   else 
     {
        _e_mod_nav_shutdown();
        ecore_main_loop_quit();
        return;
     } 

   evas_object_show(nt);

   ctrl = e_ctrl_add(evas);
   if(!ctrl) return;

   e_ctrl_theme_source_set(ctrl, THEMEDIR);
   e_ctrl_nav_set(nav);

   _e_mod_neo_me_init();

   e_ctrl_neo_me_set(neo_me);
   evas_object_show(ctrl);



   if(world) 
     {
	worldview = diversity_world_viewport_add(world, -180, 90, 180, -90); 
	diversity_dbus_signal_connect((Diversity_DBus *) worldview, 
                                      DIVERSITY_DBUS_IFACE_VIEWPORT, 
                                      "ObjectAdded", 
                                      viewport_object_added,
                                      NULL); 
        diversity_dbus_signal_connect((Diversity_DBus *) worldview, 
                                      DIVERSITY_DBUS_IFACE_VIEWPORT, 
                                      "ObjectRemoved", 
                                      viewport_object_removed,
                                      NULL); 
	diversity_viewport_start(worldview);
     }

     {
	Diversity_Equipment *eqp = NULL;
	const char *dev;

	if (self)
	  eqp = diversity_bard_equipment_get(self, "nmea");

	if (eqp) {
	     dev = "/dev/ttySAC1:9600";
	     diversity_equipment_config_set(eqp, "device-path",
		   DBUS_TYPE_STRING, &dev);
	     /*
	     dev = "/tmp/nmea.log";
	     diversity_equipment_config_set(eqp, "log",
		   DBUS_TYPE_STRING, &dev);
		   */
	}

	diversity_dbus_signal_connect((Diversity_DBus *) self,
	      DIVERSITY_DBUS_IFACE_OBJECT,
	      "GeometryChanged", on_geometry_changed, NULL);
	diversity_dbus_signal_connect((Diversity_DBus *) self,
	      DIVERSITY_DBUS_IFACE_OBJECT,
	      "PropertyChanged", on_property_changed, NULL);
     }

   /* start off at a zoom level and location instantly */
   lat = dn_config_float_get(cfg, "lat");
   lon = dn_config_float_get(cfg, "lon");
   scale = dn_config_float_get(cfg, "scale");

   /* Default is lowest zoom level if cfg file open error */
   if(scale < E_NAV_ZOOM_MIN) scale = E_NAV_ZOOM_MAX;

   neo_me_lat = dn_config_float_get(cfg, "neo_me_lat");
   neo_me_lon = dn_config_float_get(cfg, "neo_me_lon");

   e_nav_zoom_set(nav, scale, 0.0);
   e_nav_coord_set(nav, neo_me_lon, neo_me_lat, 0.0);
            
   _e_mod_nav_update(evas);
   evas_object_show(nav);
   evas_object_show(ctrl);

   Ecore_Timer * show_alert_timer;
   show_alert_timer = ecore_timer_add(2.0,
                            handle_gps,
                            NULL);
}

static int 
handle_gps(void *data)
{
   int gps_state;
   gps_state = check_gps_state();
   if(!gps_state)
     {
        Evas_Object *alert_dialog;
    
        alert_dialog = e_alert_add(evas_object_evas_get(nav));
        e_alert_theme_source_set(alert_dialog, THEMEDIR);
        e_alert_source_object_set(alert_dialog, neo_me);     
        e_alert_title_set(alert_dialog, _("GPS is off"), _("Turn on GPS?"));
        e_alert_title_color_set(alert_dialog, 255, 0, 0, 255);
        e_alert_button_add(alert_dialog, _("Yes"), alert_gps_turn_on, alert_dialog);
        e_alert_button_add(alert_dialog, _("No"), alert_gps_cancel, alert_dialog);
        evas_object_show(alert_dialog);
        e_alert_activate(alert_dialog); 
     }
   else
     {
        if(!e_nav_world_item_neo_me_fixed_get(neo_me))
          position_search_timer_start();
     }
   return FALSE;
}

static void
alert_gps_turn_on(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   int ret;
   e_alert_deactivate(obj);
   ret = turn_on_gps();
   if(ret)
     position_search_timer_start();
}

static void
alert_gps_cancel(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   e_alert_deactivate(obj);
}

static void
alert_exit(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   e_alert_deactivate(obj);
   e_ctrl_message_hide(ctrl);
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
   e_ctrl_message_show(ctrl);
   timer = ecore_timer_add(60.0,
                           _e_nav_cb_timer_pos_search_pause,
                           NULL);
}

static void
position_search_timer_stop()
{
   if(timer) ecore_timer_del(timer);
   timer = NULL;
}

static int
_e_nav_cb_timer_pos_search_pause(void *data)
{
   Evas_Object *alert_dialog;
   int fix_status;
    
   alert_dialog = e_alert_add(evas_object_evas_get(nav));
   e_alert_theme_source_set(alert_dialog, THEMEDIR);
   e_alert_source_object_set(alert_dialog, neo_me);     
   fix_status = e_nav_world_item_neo_me_fixed_get(neo_me);
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
   evas_object_show(alert_dialog);
   e_alert_activate(alert_dialog); 

   return 0;
}

/* 
 * Create neo_me evas object on map 
 */
static void
_e_mod_neo_me_init()
{
   Evas_Object *nwi;
   int accuracy;
   double neo_me_lat, neo_me_lon;

   if(neo_me) return;

   neo_me_lat = dn_config_float_get(cfg, "neo_me_lat");
   neo_me_lon = dn_config_float_get(cfg, "neo_me_lon");
   nwi = e_nav_world_item_neo_me_add(nav, THEMEDIR,
				     neo_me_lon, neo_me_lat, self);

   /* if already fixed, change the skin.   */
   accuracy = DIVERSITY_OBJECT_ACCURACY_NONE;   
   diversity_dbus_property_get(((Diversity_DBus *)self), DIVERSITY_DBUS_IFACE_OBJECT, "Accuracy",  &accuracy);
   if(accuracy != DIVERSITY_OBJECT_ACCURACY_NONE)   
     {
       e_nav_world_item_neo_me_fixed_set(nwi, 1);
     }

   e_nav_world_item_neo_me_name_set(nwi, _("Me"));
   show_welcome_message(nwi);

   neo_me = nwi;
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
   double lat, lon, scale;

   if (!nav) return;

   /* save lon, lat, scale to config file */
   lat = e_nav_coord_lat_get(nav);
   lon = e_nav_coord_lon_get(nav);
   scale = e_nav_zoom_get(nav);

   dn_config_float_set(cfg, "lat", lat);
   dn_config_float_set(cfg, "lon", lon);
   dn_config_float_set(cfg, "scale", scale);

   if (world)
     {
	if (self)
	  {
	     diversity_bard_destroy(self);
	     self = NULL;
	  }
	diversity_world_snapshot(world);
	diversity_world_destroy(world);
	world = NULL;
     }

   evas_object_del(nav);
   nav = NULL;
   e_nav_dbus_shutdown();

   e_nav_theme_shutdown();
}

