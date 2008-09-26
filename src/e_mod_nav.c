/* e_mod_nav.c -
 *
 * Copyright 2007-2008 Openmoko, Inc.
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
#include "widgets/e_nav_dialog.h"

#include <time.h>

typedef struct _E_Module_Data E_Module_Data;

static struct _E_Module_Data {
     Diversity_Nav_Config *cfg;

     Evas_Object          *ctrl;
     Evas_Object          *nav;
     Evas_Object          *tileset; /* owned by nav */

     int                   daemon_dead;
     Diversity_World      *world;
     Diversity_Bard       *self;
     Diversity_Viewport   *worldview;

     int                   gps_state;
     Ecore_Timer          *gps_check_timer;
     Ecore_Timer          *gps_fix_timer;
     unsigned int          gps_msg_id;
} mdata;

static void on_neo_other_property_changed(void *data, DBusMessage *msg);
static void gps_search_stop();

static int
viewport_variant_get(DBusMessage *message, void *val)
{
   DBusMessageIter iter, subiter;
   DBusError error;
   int type;

   dbus_error_init(&error);
   if (dbus_set_error_from_message(&error, message))
     {
	dbus_error_free(&error);

	return 0;
     }

   dbus_message_iter_init(message, &iter);
   if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT)
     return 0;

   dbus_message_iter_recurse(&iter, &subiter);
   type = dbus_message_iter_get_arg_type(&subiter);
   if (!dbus_type_is_basic(type))
     return 0;

   dbus_message_iter_get_basic(&subiter, val);

   return 1;
}

static DBusMessage *
viewport_property_call_new(E_DBus_Proxy *proxy, Diversity_DBus_IFace iface, const char *prop)
{
   DBusMessage *message;
   DBusMessageIter iter;
   const char *iface_name;

   switch (iface)
     {
      case DIVERSITY_DBUS_IFACE_OBJECT:
	 iface_name = "org.openmoko.Diversity.Object";
	 break;
      case DIVERSITY_DBUS_IFACE_BARD:
	 iface_name = "org.openmoko.Diversity.Bard";
	 break;
      case DIVERSITY_DBUS_IFACE_TAG:
	 iface_name = "org.openmoko.Diversity.Tag";
	 break;
      default:
	 return NULL;

	 break;
     }

   message = e_dbus_proxy_new_method_call(proxy, "Get");
   if (!message)
     return NULL;

   dbus_message_iter_init_append(message, &iter);

   dbus_message_iter_append_basic(&iter,
	 DBUS_TYPE_STRING, &iface_name);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &prop);

   return message;
}

enum {
     VIEWPORT_CARD_FULLNAME = 0,
     VIEWPORT_CARD_PHONE,
     N_VIEWPORT_CARD_CALLS
};

enum {
     VIEWPORT_TAG_TIMESTAMP = 0,
     VIEWPORT_TAG_UNREAD,
     VIEWPORT_TAG_GEOMETRY,
     VIEWPORT_TAG_DESCRIPTION,
     N_VIEWPORT_TAG_CALLS
};

#if 0
static void
viewport_ap_update(Evas_Object *nwi)
{
   Evas_Object *nwi;
   double lon = 0.0, lat = 0.0, w = 0.0, h = 0.0;
   int accuracy = DIVERSITY_OBJECT_ACCURACY_NONE;
   char *ssid = NULL;
   int flags = 0;

   diversity_object_geometry_get((Diversity_Object *) ap,
	 &lon, &lat, &w, &h);
   diversity_dbus_property_get((Diversity_DBus *) ap,
	 DIVERSITY_DBUS_IFACE_OBJECT, "Accuracy",  &accuracy);

   /* XXX */
   if (accuracy == DIVERSITY_OBJECT_ACCURACY_NONE)
     return NULL;

   diversity_dbus_property_get((Diversity_DBus *) ap,
	 DIVERSITY_DBUS_IFACE_AP, "Ssid", &ssid);
   diversity_dbus_property_get((Diversity_DBus *) ap,
	 DIVERSITY_DBUS_IFACE_AP, "Flags", &flags);

   lon += w / 2;
   lat += h / 2;

   nwi = e_nav_world_item_ap_add(mdata.nav, THEMEDIR, lon, lat);

   e_nav_world_item_ap_range_set(nwi, w / 2);
   if (ssid)
     {
	e_nav_world_item_ap_essid_set(nwi, ssid);

	free(ssid);
     }

   if (flags)
     e_nav_world_item_ap_key_type_set(nwi, E_NAV_ITEM_AP_KEY_TYPE_WEP);

   return nwi;
}
#endif

static void
viewport_item_bard_add_fini(void *data, E_Nav_DBus_Batch *bat)
{
   Diversity_Object *bard = data;
   E_Nav_Card *card;
   DBusMessage *reply;
   const char *fullname;
   const char *phone;

   if (e_nav_dbus_batch_replied_get(bat) != N_VIEWPORT_CARD_CALLS)
     {
	printf("failed to add card to %p\n", bard);

	e_nav_dbus_batch_destroy(bat);

	return;
     }

   card = e_nav_card_new();

   reply = e_nav_dbus_batch_reply_get(bat, VIEWPORT_CARD_FULLNAME);
   if (viewport_variant_get(reply, &fullname))
     e_nav_card_name_set(card, fullname);

   reply = e_nav_dbus_batch_reply_get(bat, VIEWPORT_CARD_PHONE);
   if (viewport_variant_get(reply, &phone))
     e_nav_card_phone_set(card, phone);

   e_nav_dbus_batch_destroy(bat);

   e_nav_card_bard_set(card, (Diversity_Bard *) bard);
   diversity_object_data_set(bard, card);

   diversity_dbus_signal_connect((Diversity_DBus *) bard,
	 DIVERSITY_DBUS_IFACE_OBJECT, "PropertyChanged",
	 on_neo_other_property_changed, card);

   e_ctrl_contact_add(mdata.ctrl, card);
}

static E_Nav_DBus_Batch *
viewport_item_bard_add(Diversity_Object *bard)
{
   E_Nav_DBus_Batch *bat;
   E_DBus_Proxy *proxy;
   DBusMessage *message;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) bard,
	 DIVERSITY_DBUS_IFACE_PROPERTIES);
   if (!proxy)
     return NULL;

   bat = e_nav_dbus_batch_new(N_VIEWPORT_CARD_CALLS, 0.0, 0.0,
	 viewport_item_bard_add_fini, bard);
   if (!bat)
     return NULL;

   message = viewport_property_call_new(proxy,
	 DIVERSITY_DBUS_IFACE_BARD, "fullname");
   e_nav_dbus_batch_call_begin(bat, VIEWPORT_CARD_FULLNAME, proxy, message);
   dbus_message_unref(message);

   message = viewport_property_call_new(proxy,
	 DIVERSITY_DBUS_IFACE_BARD, "phone");
   e_nav_dbus_batch_call_begin(bat, VIEWPORT_CARD_PHONE, proxy, message);
   dbus_message_unref(message);

   return bat;
}

static void
viewport_item_tag_add_fini(void *data, E_Nav_DBus_Batch *bat)
{
   Diversity_Object *tag = data;
   Evas_Object *nwi;
   DBusMessage *reply;
   DBusError error;
   unsigned int timestamp;
   int unread;
   double lon, lat, w, h;
   char *desc = NULL;

   if (e_nav_dbus_batch_replied_get(bat) != N_VIEWPORT_TAG_CALLS)
     {
	printf("failed to add tag to %p\n", tag);

	e_nav_dbus_batch_destroy(bat);

	return;
     }

   nwi = e_nav_world_item_location_add(mdata.nav,
	 THEMEDIR, 0.0, 0.0, tag);

   reply = e_nav_dbus_batch_reply_get(bat, VIEWPORT_TAG_TIMESTAMP);
   if (viewport_variant_get(reply, &timestamp))
     e_nav_world_item_location_timestamp_set(nwi, (time_t) timestamp);

   reply = e_nav_dbus_batch_reply_get(bat, VIEWPORT_TAG_UNREAD);
   if (viewport_variant_get(reply, &unread))
     e_nav_world_item_location_unread_set(nwi, unread);

   dbus_error_init(&error);
   reply = e_nav_dbus_batch_reply_get(bat, VIEWPORT_TAG_GEOMETRY);
   if (dbus_message_get_args(reply, &error,
	    DBUS_TYPE_DOUBLE, &lon,
	    DBUS_TYPE_DOUBLE, &lat,
	    DBUS_TYPE_DOUBLE, &w,
	    DBUS_TYPE_DOUBLE, &h,
	    DBUS_TYPE_INVALID))
     {
	e_nav_world_item_coord_set(nwi, lon, lat);
	e_nav_world_item_update(nwi);
     }
   else
     {
	printf("failed to get geometry: %s\n", error.message);
	dbus_error_free(&error);
     }

   dbus_error_init(&error);
   reply = e_nav_dbus_batch_reply_get(bat, VIEWPORT_TAG_DESCRIPTION);
   if (dbus_message_get_args(reply, &error,
	    DBUS_TYPE_STRING, &desc,
	    DBUS_TYPE_INVALID))
     {
	char *note;

	if (desc)
	  desc = strdup(desc);

	if (desc)
	  {
	     note = strchr(desc, '\n');
	     if (note)
	       *note++ = '\0';

	     e_nav_world_item_location_name_set(nwi, desc);
	     if (note)
	       e_nav_world_item_location_note_set(nwi, note);

	     free(desc);
	  }
     }
   else
     {
	printf("failed to get description: %s\n", error.message);
	dbus_error_free(&error);
     }

   e_nav_dbus_batch_destroy(bat);

   diversity_object_data_set(tag, nwi);
   e_ctrl_taglist_tag_add(mdata.ctrl, nwi);
}

static E_Nav_DBus_Batch *
viewport_item_tag_add(Diversity_Object *tag)
{
   E_Nav_DBus_Batch *bat;
   E_DBus_Proxy *proxy;
   DBusMessage *message;
   const char *desc;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) tag,
	 DIVERSITY_DBUS_IFACE_PROPERTIES);
   if (!proxy)
     return NULL;

   bat = e_nav_dbus_batch_new(N_VIEWPORT_TAG_CALLS, 0.0, 0.0,
	 viewport_item_tag_add_fini, tag);

   message = viewport_property_call_new(proxy,
	 DIVERSITY_DBUS_IFACE_OBJECT, "Timestamp");
   e_nav_dbus_batch_call_begin(bat, VIEWPORT_TAG_TIMESTAMP, proxy, message);
   dbus_message_unref(message);

   message = viewport_property_call_new(proxy,
	 DIVERSITY_DBUS_IFACE_TAG, "Unread");
   e_nav_dbus_batch_call_begin(bat, VIEWPORT_TAG_UNREAD, proxy, message);
   dbus_message_unref(message);

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) tag,
	 DIVERSITY_DBUS_IFACE_OBJECT);
   if (!proxy)
     {
	e_nav_dbus_batch_destroy(bat);

	return NULL;
     }

   message = e_dbus_proxy_new_method_call(proxy, "GeometryGet");
   e_nav_dbus_batch_call_begin(bat, VIEWPORT_TAG_GEOMETRY, proxy, message);
   dbus_message_unref(message);

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) tag,
	 DIVERSITY_DBUS_IFACE_TAG);
   if (!proxy)
     {
	e_nav_dbus_batch_destroy(bat);

	return NULL;
     }

   desc  = "description";

   message = e_dbus_proxy_new_method_call(proxy, "Get");
   if (!dbus_message_append_args(message,
	    DBUS_TYPE_STRING, &desc,
	    DBUS_TYPE_INVALID))
     {
	e_nav_dbus_batch_destroy(bat);

	return NULL;
     }

   e_nav_dbus_batch_call_begin(bat, VIEWPORT_TAG_DESCRIPTION, proxy, message);
   dbus_message_unref(message);

   return bat;
}

static int
viewport_item_add(Diversity_Object *obj, int sync)
{
   E_Nav_DBus_Batch *bat = NULL;
   void *item = NULL;
   int type;

   type = diversity_object_type_get(obj);
   switch (type)
     {
      case DIVERSITY_OBJECT_TYPE_AP:
	 break;
      case DIVERSITY_OBJECT_TYPE_BARD:
	 bat = viewport_item_bard_add(obj);
	 break;
      case DIVERSITY_OBJECT_TYPE_TAG:
	 bat = viewport_item_tag_add(obj);
	 break;
      default:
	 break;
     }

   if (!bat)
     return FALSE;

   if (!sync)
     {
	printf("async item add is not supported\n");

	sync = 1;
     }

   if (sync)
     {
	e_nav_dbus_batch_block(bat);
	item = diversity_object_data_get(obj);
     }

   return (item != NULL);
}

static void
viewport_item_remove(Diversity_Object *obj)
{
   void *item;
   int type;

   item = diversity_object_data_get(obj);
   if (!item)
     return;

   type = diversity_object_type_get(obj);

   switch (type)
     {
      case DIVERSITY_OBJECT_TYPE_BARD:
	   {
	      E_Nav_Card *card = item;

	      e_ctrl_contact_delete(mdata.ctrl, card);
	      e_nav_card_destroy(card);
	   }
	 break;
      case DIVERSITY_OBJECT_TYPE_TAG:
	   {
	      Evas_Object *nwi = item;

	      e_ctrl_taglist_tag_delete(mdata.ctrl, nwi);
	      evas_object_del(nwi);
	   }
      default:
	 break;
     }

   diversity_object_data_set(obj, NULL);
}

static void
viewport_object_add(const char *obj_path, int type, int sync)
{
   Diversity_Object *obj;
   Evas_Object *neo_me;

   if (e_ctrl_object_store_item_get(mdata.ctrl, obj_path))
     return;

   switch (type)
     {
     case DIVERSITY_OBJECT_TYPE_TAG:
	obj = (Diversity_Object *) diversity_tag_new(obj_path);
	break;
     case DIVERSITY_OBJECT_TYPE_BARD:
	obj = (Diversity_Object *) diversity_bard_new(obj_path);
	break;
     case DIVERSITY_OBJECT_TYPE_AP:
	obj = (Diversity_Object *) diversity_ap_new(obj_path);
	break;
     default:
	obj = diversity_object_new(obj_path);
	break;
     }

   if (!viewport_item_add(obj, sync))
     {
	diversity_object_destroy(obj);

	return;
     }

   e_ctrl_object_store_item_add(mdata.ctrl, obj_path, obj);

   neo_me = e_nav_world_neo_me_get(mdata.nav);
   if (neo_me)
     e_nav_world_item_raise(neo_me);
}

static void
viewport_object_remove(const char *obj_path)
{
   Diversity_Object *obj;

   obj = e_ctrl_object_store_item_remove(mdata.ctrl, obj_path);
   if (!obj)
     return;

   viewport_item_remove(obj);
   diversity_object_destroy(obj);
}

static void
on_viewport_object_added(void *data, DBusMessage *msg)
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

   viewport_object_add(obj_path, -1, 1);
}

static void
on_viewport_object_removed(void *data, DBusMessage *msg)
{
   const char *obj_path;
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

   viewport_object_remove(obj_path);
}

static void
on_neo_other_property_changed(void *data, DBusMessage *msg)
{
   E_Nav_Card *card = data;
   DBusMessageIter iter;
   DBusMessageIter subiter;
   const char *name, *value;
   int type;

   if (!dbus_message_iter_init(msg, &iter))
     return;

   type = dbus_message_iter_get_arg_type(&iter);
   if (type != DBUS_TYPE_STRING)
     return;

   dbus_message_iter_get_basic(&iter, &name);

   if (!dbus_message_iter_next(&iter))
     return;

   type = dbus_message_iter_get_arg_type(&iter);
   if (type != DBUS_TYPE_VARIANT)
     return;

   dbus_message_iter_recurse(&iter, &subiter);
   type = dbus_message_iter_get_arg_type(&subiter);
   if (type != DBUS_TYPE_STRING)
     return;

   dbus_message_iter_get_basic(&subiter, &value); 

   if (strcasecmp(name, "fullname") == 0)
     e_nav_card_name_set(card, value);
   else if (strcasecmp(name, "phone") == 0)
     e_nav_card_phone_set(card, value);
}

static void
on_neo_me_geometry_changed(void *data, DBusMessage *msg)
{
   Evas_Object *neo_me;
   DBusError err;
   double lon, lat;
   double w, h;

   dbus_error_init(&err);

   neo_me = e_nav_world_neo_me_get(mdata.nav);
   if (!neo_me)
     return;

   if (!dbus_message_get_args(msg, &err,
	    DBUS_TYPE_DOUBLE, &lon,
	    DBUS_TYPE_DOUBLE, &lat,
	    DBUS_TYPE_DOUBLE, &w,
	    DBUS_TYPE_DOUBLE, &h,
	    DBUS_TYPE_INVALID))
     {
	printf("Error: %s - %s\n", err.name, err.message);
	dbus_error_free(&err);

	return;
     }

   e_nav_world_item_coord_set(neo_me, lon, lat);
   e_nav_world_item_update(neo_me);

   if (e_ctrl_follow_get(mdata.ctrl))
     e_nav_coord_set(mdata.nav, lon, lat, 0.0);
}

static void
on_neo_me_property_changed(void *data, DBusMessage *msg)
{
   Evas_Object *neo_me;
   int fixed;

   neo_me = e_nav_world_neo_me_get(mdata.nav);
   if (!neo_me)
     return;

   fixed = DIVERSITY_OBJECT_ACCURACY_NONE;
   diversity_dbus_property_get(((Diversity_DBus *) mdata.self),
	 DIVERSITY_DBUS_IFACE_OBJECT, "Accuracy", &fixed);

   fixed = (fixed != DIVERSITY_OBJECT_ACCURACY_NONE);
   e_nav_world_item_neo_me_fixed_set(neo_me, fixed);

   if (mdata.gps_fix_timer && fixed)
     gps_search_stop();
}

#define GPS_DEVICE_NAME "/sys/bus/platform/drivers/neo1973-pm-gps/neo1973-pm-gps.0/pwron"
enum {
     GPS_STATE_NA = 0,
     GPS_STATE_OFF,
     GPS_STATE_ON,
};

static int
gps_state_get(void)
{
   FILE *fp;
   int state = GPS_STATE_NA;

   fp = fopen(GPS_DEVICE_NAME, "r");
   if (fp)
     {
	char buf;
	size_t ret;

	ret = fread(&buf, 1, 1, fp);
	fclose(fp);

	if (ret == 1)
	  state = (buf == '1') ? GPS_STATE_ON : GPS_STATE_OFF;
     }

   if (state == GPS_STATE_NA)
     printf("failed to get GPS device state\n");

   return state;
}

static int
gps_state_set(int state)
{
   FILE *fp;
   char buf;

   if (state == GPS_STATE_NA)
     return GPS_STATE_NA;

   fp = fopen(GPS_DEVICE_NAME, "w");
   if (!fp)
     return GPS_STATE_NA;

   buf = (state == GPS_STATE_ON) ? '1' : '0';
   fwrite(&buf, 1, 1, fp);
   fclose(fp);

   return gps_state_get();
}

static void
gps_search_confirm(void *data, Evas_Object *obj)
{
   e_nav_dialog_deactivate(obj);
   gps_search_stop();
}

static int
gps_search_timeout(void *data)
{
   Evas_Object *neo_me;
   Evas_Object *ad;
   int fixed = 0;

   neo_me = e_nav_world_neo_me_get(mdata.nav);
   if (neo_me)
     fixed = e_nav_world_item_neo_me_fixed_get(neo_me);

   ad = e_nav_dialog_add(evas_object_evas_get(mdata.nav));
   e_nav_dialog_type_set(ad, E_NAV_DIALOG_TYPE_ALERT);
   e_nav_dialog_transient_for_set(ad, mdata.nav);
   if (fixed)
     {
	e_nav_dialog_title_set(ad, _("GPS FIX"), _("Your approximate location"));
	e_nav_dialog_title_color_set(ad, 0, 255, 0, 255);
     }
   else
     {
	e_nav_dialog_title_set(ad, _("ERROR"), _("Unable to locate a fix"));
	e_nav_dialog_title_color_set(ad, 255, 0, 0, 255);
     }

   e_nav_dialog_button_add(ad, _("OK"), gps_search_confirm, NULL);

   e_nav_dialog_activate(ad);
   evas_object_show(ad);

   mdata.gps_fix_timer = NULL;

   return 0;
}

static void
gps_search_stop()
{
   if (mdata.gps_fix_timer)
     {
	ecore_timer_del(mdata.gps_fix_timer);
	mdata.gps_fix_timer = NULL;
     }

   if (mdata.gps_msg_id)
     {
	e_ctrl_message_text_del(mdata.ctrl, mdata.gps_msg_id);
	mdata.gps_msg_id = 0;
     }
}

static void
gps_search_start(void)
{
   if (mdata.gps_fix_timer)
     ecore_timer_del(mdata.gps_fix_timer);

   mdata.gps_fix_timer = ecore_timer_add(120.0,
                           gps_search_timeout,
                           NULL);

   if (!mdata.gps_msg_id)
     mdata.gps_msg_id = e_ctrl_message_text_add(mdata.ctrl,
	   _("Searching for your location"), 0.0);

}

static void
gps_check_yes(void *data, Evas_Object *obj)
{
   e_nav_dialog_deactivate(obj);

   if (gps_state_set(GPS_STATE_ON) == GPS_STATE_ON)
     {
	mdata.gps_state = GPS_STATE_ON;
	gps_search_start();
     }
}

static void
gps_check_no(void *data, Evas_Object *obj)
{
   e_nav_dialog_deactivate(obj);
}

static int
gps_check(void *data)
{
   Evas_Object *neo_me;
   int state;

   neo_me = e_nav_world_neo_me_get(mdata.nav);
   state = gps_state_get();
   if (!neo_me || state == GPS_STATE_NA)
     {
	mdata.gps_state = GPS_STATE_NA;
	mdata.gps_check_timer = NULL;

	return FALSE;
     }

   if (state == mdata.gps_state)
     return TRUE;

   /* this is the first time we are here */
   if (mdata.gps_state == GPS_STATE_NA)
     {
	if (state == GPS_STATE_ON)
	  {
	     if (!e_nav_world_item_neo_me_fixed_get(neo_me))
	       gps_search_start();
	  }

	ecore_timer_interval_set(mdata.gps_check_timer, 60.0);
     }

   mdata.gps_state = state;

   if (state == GPS_STATE_OFF)
     {
	Evas_Object *ad;

	ad = e_nav_dialog_add(evas_object_evas_get(mdata.nav));
	e_nav_dialog_type_set(ad, E_NAV_DIALOG_TYPE_ALERT);
	e_nav_dialog_transient_for_set(ad, mdata.nav);
	e_nav_dialog_title_set(ad, _("GPS is off"), _("Turn on GPS?"));
	e_nav_dialog_title_color_set(ad, 255, 0, 0, 255);
	e_nav_dialog_button_add(ad, _("Yes"), gps_check_yes, NULL);
	e_nav_dialog_button_add(ad, _("No"), gps_check_no, NULL);

	gps_search_stop();

	e_nav_dialog_activate(ad);
	evas_object_show(ad);
     }

   return TRUE;
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

static Evas_Object *
_e_mod_neo_me_init()
{
   Evas_Object *neo_me;
   double lon, lat;

   lon = dn_config_float_get(mdata.cfg, "neo_me_lon");
   lat = dn_config_float_get(mdata.cfg, "neo_me_lat");

   neo_me = e_nav_world_item_neo_me_add(mdata.nav,
	 THEMEDIR, lon, lat, mdata.self);
   if (!neo_me)
     return NULL;

   e_nav_world_item_neo_me_name_set(neo_me, _("Me"));

   if (mdata.self)
     {
	int accuracy;

	accuracy = DIVERSITY_OBJECT_ACCURACY_NONE;
	diversity_dbus_property_get(((Diversity_DBus *) mdata.self),
	      DIVERSITY_DBUS_IFACE_OBJECT, "Accuracy",  &accuracy);

	if (accuracy != DIVERSITY_OBJECT_ACCURACY_NONE)
	  {
	     double w, h;

	     diversity_object_geometry_get(
		   (Diversity_Object *) mdata.self,
		   &lon, &lat, &w, &h);

	     e_nav_world_item_coord_set(neo_me,
		   lon, lat);
	     e_nav_world_item_neo_me_fixed_set(neo_me, 1);

	     e_nav_world_item_update(neo_me);
	  }
     }

   return neo_me;
}

static void
_e_mod_nav_dbus_shutdown(void)
{
   if (mdata.worldview)
     {
	if (!mdata.daemon_dead)
	  diversity_viewport_stop(mdata.worldview);
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
	if (!mdata.daemon_dead)
	  diversity_world_snapshot(mdata.world);
	diversity_world_destroy(mdata.world);

	mdata.world = NULL;
     }

   e_nav_dbus_shutdown();
}

static void
on_daemon_dead_confirm(void *data, Evas_Object *obj)
{
   e_nav_dialog_deactivate(obj);

   mdata.daemon_dead = 1;

   _e_mod_nav_shutdown();
   ecore_main_loop_quit();
}

static void
on_daemon_dead(void *data)
{
   Evas_Object *ad;

   ad = e_nav_dialog_add(evas_object_evas_get(mdata.nav));
   e_nav_dialog_type_set(ad, E_NAV_DIALOG_TYPE_ALERT);
   e_nav_dialog_transient_for_set(ad, mdata.nav);
   e_nav_dialog_title_color_set(ad, 255, 0, 0, 255);
   /* actually, dbus connection is still opened */
   e_nav_dialog_title_set(ad, _("ERROR"), _("DBus connection closed"));

   e_nav_dialog_button_add(ad, _("Exit"), on_daemon_dead_confirm, NULL);

   e_nav_dialog_activate(ad);
   evas_object_show(ad);
}

static int
_e_mod_nav_dbus_init(void)
{
   Diversity_Equipment *eqp;

   if (!e_nav_dbus_init(on_daemon_dead, NULL))
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
	 on_viewport_object_added,
	 NULL); 
   diversity_dbus_signal_connect((Diversity_DBus *) mdata.worldview, 
	 DIVERSITY_DBUS_IFACE_VIEWPORT, 
	 "ObjectRemoved", 
	 on_viewport_object_removed,
	 NULL); 

   eqp = diversity_bard_equipment_get(mdata.self, "nmea");
   if (eqp)
     {
	const char *dev, *log;

	dev = dn_config_string_get(mdata.cfg, "gps_device");
	if (dev && diversity_equipment_config_set(eqp, "device-path",
		 DBUS_TYPE_STRING, &dev))
	  {
	     log = dn_config_string_get(mdata.cfg, "gps_log");
	     if (log)
	       {
		  char buf[PATH_MAX];

		  if (!ecore_file_exists(log) || ecore_file_is_dir(log))
		    {
		       struct tm *tm;
		       time_t now;
		       char tstr[32];

		       if (!ecore_file_exists(log))
			 ecore_file_mkpath(log);

		       now = time(NULL);
		       tm = localtime(&now);
		       strftime(tstr, sizeof(tstr), "%Y-%m-%d.%H%M", tm);

		       snprintf(buf, sizeof(buf), "%s/%s.log", log, tstr);

		       log = buf;
		    }

		  diversity_equipment_config_set(eqp, "log",
			DBUS_TYPE_STRING, &log);
	       }

	     diversity_dbus_signal_connect((Diversity_DBus *) mdata.self,
		   DIVERSITY_DBUS_IFACE_OBJECT,
		   "GeometryChanged", on_neo_me_geometry_changed, NULL);
	     diversity_dbus_signal_connect((Diversity_DBus *) mdata.self,
		   DIVERSITY_DBUS_IFACE_OBJECT,
		   "PropertyChanged", on_neo_me_property_changed, NULL);
	  }

	diversity_equipment_destroy(eqp);
     }

   /* add tags before the UI is shown */
   /* XXX ugly!! */
   if (1)
     {
	Diversity_Viewport *view;
	char **obj_pathes, **p;

	printf("retrieving tags\n");

	view = diversity_world_viewport_add(mdata.world, -180.0, -90.0, 180.0, 90.0);
	diversity_viewport_rule_set(view, DIVERSITY_OBJECT_TYPE_TAG, 0, 0);
	diversity_viewport_start(view);

	obj_pathes = diversity_viewport_objects_list(view);
	if (obj_pathes)
	  {
	     printf("importing tags\n");

	     e_ctrl_taglist_freeze(mdata.ctrl);

	     for (p = obj_pathes; *p; p++)
	       viewport_object_add(*p, DIVERSITY_OBJECT_TYPE_TAG, 1);

	     e_ctrl_taglist_thaw(mdata.ctrl);

	     dbus_free_string_array(obj_pathes);
	  }

	printf("tags imported\n");
	diversity_world_viewport_remove(mdata.world, view);
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
   Evas_Object *neo_me;

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
   mdata.tileset = e_nav_tileset_add(evas,
	 E_NAV_TILESET_FORMAT_OSM, tile_path);
   if (mdata.tileset)
     {
	e_nav_world_tileset_set(mdata.nav, mdata.tileset);

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

   neo_me = _e_mod_neo_me_init();
   if (neo_me)
     {
	e_nav_world_item_neo_me_activate(neo_me);
	e_nav_world_item_geometry_get(neo_me,
	      &lon, &lat, NULL, NULL);
     }

   e_nav_coord_set(mdata.nav, lon, lat, 0.0);
   e_nav_span_set(mdata.nav, span, 0.0);

   _e_mod_nav_update(evas);

   mdata.gps_check_timer =
      ecore_timer_add(2.0, gps_check, NULL);
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
   Evas_Object *neo_me;
   Ecore_List *obj_list;
   const char *obj_path;
   double lon, lat;
   int span;

   if (!mdata.nav)
     return;

   e_nav_coord_get(mdata.nav, &lon, &lat);
   span = e_nav_span_get(mdata.nav);

   dn_config_float_set(mdata.cfg, "lon", lon);
   dn_config_float_set(mdata.cfg, "lat", lat);
   dn_config_int_set(mdata.cfg, "span", span);

   neo_me = e_nav_world_neo_me_get(mdata.nav);
   if (neo_me)
     {
	e_nav_world_item_geometry_get(neo_me, &lon, &lat, NULL, NULL);
	dn_config_float_set(mdata.cfg, "neo_me_lon", lon);
	dn_config_float_set(mdata.cfg, "neo_me_lat", lat);
     }

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

   obj_list = e_ctrl_object_store_keys(mdata.ctrl);
   ecore_list_first_goto(obj_list);

   while ((obj_path = ecore_list_next(obj_list)))
     viewport_object_remove(obj_path);

   ecore_list_destroy(obj_list);

   _e_mod_nav_dbus_shutdown();

   evas_object_del(mdata.ctrl);

   evas_object_del(mdata.nav);
   mdata.nav = NULL;

   e_nav_theme_shutdown();
}
