/* e_nav_item_location.c -
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
#include "e_nav_theme.h"
#include "e_nav_item_location.h"
#include "e_flyingmenu.h"
#include "widgets/e_nav_dialog.h"
#include "widgets/e_nav_alert.h"
#include "widgets/e_nav_textedit.h"
#include "widgets/e_nav_contact_editor.h"
#include "e_ctrl.h"
#include <time.h>
#include <ctype.h>

static char *get_time_diff_string(time_t time_then);

typedef struct _Location_Data Location_Data;

struct _Location_Data
{
   const char             *name;
   const char             *note;
   unsigned char           visible : 1;
   double                  lat;
   double                  lon;
   time_t                  timestamp;
   Diversity_Tag          *tag;
   uint                    unread;
};

static void
alert_exit(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   e_alert_deactivate(obj);
}

static void
dialog_exit(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   e_dialog_deactivate(obj);
}

static void
location_save(Evas_Object *obj, Evas_Object *src_obj)
{
   Location_Data *locd;
   const char *name, *note;
   char *description;
   int result;

   locd = evas_object_data_get(src_obj, "nav_world_item_location_data");
   if (!locd) return;
   name = e_dialog_textblock_text_get(obj, "Edit title");
   note = e_dialog_textblock_text_get(obj, "Edit message");
   description = malloc(strlen(name) + 1 + strlen(note) + 1);
   if (!description) return ;
   sprintf(description, "%s%c%s", name, '\n', note);
   result = diversity_tag_prop_set(locd->tag, "description", description);
   if(result)
     {
        if (locd->name) evas_stringshare_del(locd->name);
        if (name) locd->name = evas_stringshare_add(name);
        else locd->name = NULL;

        if (locd->note) evas_stringshare_del(locd->note);
        if (note) locd->note = evas_stringshare_add(note);
        else locd->note = NULL;
        e_ctrl_taglist_tag_set(name, note, src_obj);  
        e_nav_world_item_location_name_set(src_obj, name);
     }
   free(description);
}

static void
dialog_location_save(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   location_save(obj, src_obj);
   e_dialog_deactivate(obj);
}

static void
alert_location_delete_cancelled(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   e_alert_deactivate(obj);
}

static void
alert_location_delete_confirmed(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Location_Data *locd;
   int ok;

   locd = evas_object_data_get(src_obj, "nav_world_item_location_data");
   if (!locd) return;
   Diversity_World *world = e_nav_world_get();
   ok = diversity_world_tag_remove(world, locd->tag);
   if(ok)
     {
        locd->tag = NULL;
     }
   e_alert_deactivate(obj);
}

static void
dialog_location_delete(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *oa = e_alert_add(evas_object_evas_get(obj));
   e_alert_theme_source_set(oa, THEME_PATH);
   e_alert_source_object_set(oa, src_obj);
   e_alert_title_set(oa, "DELETE", "Are you sure?");
   e_alert_title_color_set(oa, 255, 0, 0, 255);
   e_alert_button_add(oa, "Yes", alert_location_delete_confirmed, oa);
   e_alert_button_add(oa, "No", alert_location_delete_cancelled, oa);
   e_dialog_deactivate(obj);
   evas_object_show(oa);
   e_alert_activate(oa);
}

static int
is_phone_number(const char *input)
{
   int length;
   int i;
   if(input)
     length = strlen(input);

   if(!input || length<3)
     return FALSE;

   if(input[0]!='+' && !isdigit(input[0]))
     return FALSE;

   for(i=1; i<length; i++)
     if(!isdigit(input[i]))
       return FALSE;

   return TRUE;     
}

static Diversity_Equipment *
get_phone_equip()
{
   Diversity_Equipment *eqp = NULL;
   eqp = e_ctrl_self_equipment_get("qtopia");
   if(!eqp)
     eqp = e_ctrl_self_equipment_get("phonekit");

   return eqp;
}

static void
location_send(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Diversity_Equipment *eqp = NULL;
   Neo_Other_Data *neod = NULL;
   Evas_Object *alert_dialog;
   Evas_Object *location_object;
   Location_Data *locd;

   int ok = FALSE;
   const char *input = e_contact_editor_input_get(obj); 
   printf("location send\n");

   if(input==NULL)
     { 
        e_contact_editor_deactivate(obj);   
        return;
     }

   location_object = src_obj;
   locd = evas_object_data_get(location_object, "nav_world_item_location_data");
   if (!locd) 
     {
        e_contact_editor_deactivate(obj);   
        return;
     }

   eqp = get_phone_equip();
     
   if(!eqp) 
     {
        e_contact_editor_deactivate(obj);   
        return;
     }

   neod = e_ctrl_contact_get_by_name(input);
   if(!neod)
     neod = e_ctrl_contact_get_by_number(input);

   /* send by contact */
   if(neod) 
     {
        ok = diversity_sms_tag_share((Diversity_Sms *)eqp, neod->bard, locd->tag);
        alert_dialog = e_alert_add(evas_object_evas_get(obj));
        e_alert_theme_source_set(alert_dialog, THEME_PATH);
        e_alert_source_object_set(alert_dialog, src_obj);     
        if (ok)
          {
             e_alert_title_set(alert_dialog, "SUCCESS", "Tag sent");
             e_alert_title_color_set(alert_dialog, 0, 255, 0, 255);
             e_alert_button_add(alert_dialog, "OK", alert_exit, alert_dialog);
          }
        else 
          {
             e_alert_title_set(alert_dialog, "FAIL", "Tag sent fail");
             e_alert_title_color_set(alert_dialog, 255, 0, 0, 255);
             e_alert_button_add(alert_dialog, "OK", alert_exit, alert_dialog);
          }
        e_contact_editor_deactivate(obj);   
        evas_object_show(alert_dialog);
        e_alert_activate(alert_dialog); 
        return;
     }

   /* send by number */   
   if(is_phone_number(input))
     {
        ok = diversity_sms_tag_send((Diversity_Sms *)eqp, input, locd->tag);

        Evas_Object *od = e_alert_add(evas_object_evas_get(obj));
        e_alert_theme_source_set(od, THEME_PATH);
        e_alert_source_object_set(od, src_obj);     
        if(ok)
          {
             e_alert_title_set(od, "SUCCESS", "Tag sent");
             e_alert_title_color_set(od, 0, 255, 0, 255);
             e_alert_button_add(od, "OK", alert_exit, od);
          }
        else
          {
             e_alert_title_set(od, "FAIL", "Tag sent fail");
             e_alert_title_color_set(od, 255, 0, 0, 255);
             e_alert_button_add(od, "OK", alert_exit, od);
          }
        e_contact_editor_deactivate(obj);   
        evas_object_show(od);
        e_alert_activate(od); 
        return;
     }

   /* can't find contact and is not a phone number */
   alert_dialog = e_alert_add(evas_object_evas_get(obj));
   e_alert_theme_source_set(alert_dialog, THEME_PATH);
   e_alert_source_object_set(alert_dialog, src_obj);     
   e_alert_title_set(alert_dialog, "FAIL", "Contact not found");
   e_alert_title_color_set(alert_dialog, 255, 0, 0, 255);
   e_alert_button_add(alert_dialog, "OK", alert_exit, alert_dialog);
   e_contact_editor_deactivate(obj);   
   evas_object_show(alert_dialog);
   e_alert_activate(alert_dialog); 
}

static void
dialog_location_send(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *editor;
   Ecore_List *contacts;

   location_save(obj, src_obj); 

   editor = e_contact_editor_add( evas_object_evas_get(obj) );
   e_contact_editor_theme_source_set(editor, THEME_PATH, location_send, data, NULL, NULL); // data is the location item evas object 
   e_contact_editor_source_object_set(editor, data);  //  src_object is location item evas object 
   e_contact_editor_input_set(editor, "To:", "");

   contacts = e_ctrl_contacts_get(); 

   //FIXME: contacts need to be destroyed later.
   
   e_dialog_deactivate(obj);
   evas_object_show(editor);
   e_contact_editor_contacts_set(editor, contacts);
   e_contact_editor_activate(editor);
}

static void
_e_nav_world_item_cb_menu_1(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *location_object = (Evas_Object*)data;
   Location_Data *locd;
   locd = evas_object_data_get(location_object, "nav_world_item_location_data");
   if (!locd) return;

   Evas_Object *od = e_dialog_add(evas_object_evas_get(obj));
   e_dialog_theme_source_set(od, THEME_PATH);  
   e_dialog_source_object_set(od, src_obj);  
   e_dialog_title_set(od, "Edit your location", "Edit or delete your location");
   const char *title = e_nav_world_item_location_name_get(location_object);
   e_dialog_textblock_add(od, "Edit title", title, 40, 40, obj);
   const char *message = e_nav_world_item_location_note_get(location_object);
   e_dialog_textblock_add(od, "Edit message", message, 120, 80, obj);
   e_dialog_button_add(od, "Save", dialog_location_save, od);
   e_dialog_button_add(od, "Cancel", dialog_exit, od);
   e_dialog_button_add(od, "Delete", dialog_location_delete, od);
   
   e_flyingmenu_deactivate(obj);
   evas_object_show(od);
   e_dialog_activate(od);
}

static void
_e_nav_world_item_cb_menu_2(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *location_object = (Evas_Object*)data;
   Location_Data *locd;
   locd = evas_object_data_get(location_object, "nav_world_item_location_data");
   if (!locd) return;

   Evas_Object *od = e_dialog_add(evas_object_evas_get(obj));
   e_dialog_theme_source_set(od, THEME_PATH);  
   e_dialog_source_object_set(od, src_obj);  
   e_dialog_title_set(od, "Send your location", "Send your favorite location to friends by SMS");
   const char *title = e_nav_world_item_location_name_get(location_object);
   e_dialog_textblock_add(od, "Edit title", title, 40, 40, obj);
   const char *message = e_nav_world_item_location_note_get(location_object);
   e_dialog_textblock_add(od, "Edit message", message, 120, 80, obj);
   e_dialog_button_add(od, "Send", dialog_location_send, data);
   e_dialog_button_add(od, "Cancel", dialog_exit, od);
   
   e_flyingmenu_deactivate(obj);
   evas_object_show(od);
   e_dialog_activate(od);
}

static void
cb_menu_activate(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Evas_Object *om;
   om = e_flyingmenu_add(evas_object_evas_get(obj));
   e_flyingmenu_theme_source_set(om, data);  // data is THEME_PATH
   e_flyingmenu_autodelete_set(om, 1);
   e_flyingmenu_source_object_set(om, obj);   // obj is location evas object
   /* FIXME: real menu items */
   e_flyingmenu_theme_item_add(om, "modules/diversity_nav/tag_menu_item", 160, "edit",
			       _e_nav_world_item_cb_menu_1, obj);
   e_flyingmenu_theme_item_add(om, "modules/diversity_nav/tag_menu_item", 160, "send",
			       _e_nav_world_item_cb_menu_2, obj);
   evas_object_show(om);
   e_flyingmenu_activate(om);
}

static void
_e_nav_world_item_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *location;
   Location_Data *locd;
   const char *text_part_state;
   double val_ret;
   char *time_diff_string;
 
   location = (Evas_Object *)data;   
   if(!location) return;
   text_part_state = edje_object_part_state_get(location, "e.text.name", &val_ret);
   locd = evas_object_data_get(location, "nav_world_item_location_data");

   if(!strcmp(text_part_state, "default")) 
     {
        time_diff_string = get_time_diff_string(locd->timestamp);
        edje_object_part_text_set(location, "e.text.name2", time_diff_string);
        free(time_diff_string);
        edje_object_signal_emit(location, "e,state,active", "e");
     }
   else 
     {
        edje_object_signal_emit(location, "e,state,passive", "e");
     }
}

static void
_e_nav_world_item_cb_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Location_Data *locd;

   if(!obj) return;

   e_ctrl_taglist_tag_delete(obj);   

   locd = evas_object_data_get(obj, "nav_world_item_location_data");
   if (!locd) return;
   if (locd->name) evas_stringshare_del(locd->name);
   if (locd->note) evas_stringshare_del(locd->note);

   edje_object_signal_callback_del(obj, "MENU_ACTIVATE", "e.text.name", cb_menu_activate);
   free(locd);
}

/////////////////////////////////////////////////////////////////////////////
Evas_Object *
e_nav_world_item_location_add(Evas_Object *nav, const char *theme_dir, double lon, double lat, Diversity_Object *tag)
{
   Evas_Object *o;
   Location_Data *locd;
   int x, y, w, h;

   /* FIXME: allocate extra data struct for AP properites and attach to the
    * evas object */
   locd = calloc(1, sizeof(Location_Data));
   if (!locd) return NULL;
   locd->tag = (Diversity_Tag *) tag;
   o = e_nav_theme_object_new(evas_object_evas_get(nav), theme_dir,
				       "modules/diversity_nav/location");
   edje_object_part_text_set(o, "e.text.name", "No Title");
   edje_object_signal_callback_add(o, "MENU_ACTIVATE", "e.text.name", cb_menu_activate, (void *)theme_dir);

   evas_object_event_callback_add(edje_object_part_object_get(o, "e.image.location"),
                                  EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_world_item_cb_mouse_down,
				  o);
   evas_object_geometry_get(edje_object_part_object_get(o, "e.image.location"), &x, &y, &w, &h);
   e_nav_world_item_add(nav, o);
   e_nav_world_item_type_set(o, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(o, lon, lat, w, h);
   e_nav_world_item_scale_set(o, 0);
   e_nav_world_item_update(o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
				  _e_nav_world_item_cb_del, NULL);
   evas_object_data_set(o, "nav_world_item_location_data", locd);
   e_nav_world_item_location_lat_set(o, lat);
   e_nav_world_item_location_lon_set(o, lon);
   evas_object_show(o);
   return o;
}

void
e_nav_world_item_location_name_set(Evas_Object *item, const char *name)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;
   if (locd->name) evas_stringshare_del(locd->name);
   if (name) locd->name = evas_stringshare_add(name);
   else locd->name = NULL;
   if(!locd->name || !strcmp(locd->name, ""))
     edje_object_part_text_set(item, "e.text.name", "No Title");
   else 
     edje_object_part_text_set(item, "e.text.name", locd->name);
}

const char *
e_nav_world_item_location_name_get(Evas_Object *item)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return NULL;
   return locd->name;
}

void
e_nav_world_item_location_note_set(Evas_Object *item, const char *note)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;
   if (locd->note) evas_stringshare_del(locd->note);
   if (note) locd->note = evas_stringshare_add(note);
   else locd->note = NULL;
}

const char *
e_nav_world_item_location_note_get(Evas_Object *item)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return NULL;
   return locd->note;
}

void
e_nav_world_item_location_visible_set(Evas_Object *item, Evas_Bool visible)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;
   if ((visible && locd->visible) || ((!visible) && (!locd->visible))) return;
   locd->visible = visible;
   if (locd->visible)
     edje_object_signal_emit(item, "e,state,visible", "e");
   else
     edje_object_signal_emit(item, "e,state,invisible", "e");
}

Evas_Bool
e_nav_world_item_location_visible_get(Evas_Object *item)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return 0;
   return locd->visible;
}

void
e_nav_world_item_location_lat_set(Evas_Object *item, double lat)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;
   if (lat > 90.0) lat=90.0;
   if (lat < -90.0) lat=-90.0;
   locd->lat = lat;
}

double
e_nav_world_item_location_lat_get(Evas_Object *item)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return 0;
   return locd->lat;
}

void
e_nav_world_item_location_lon_set(Evas_Object *item, double lon)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;
   if (lon > 180.0) lon=180.0;
   if (lon < -180.0) lon=-180.0;
   locd->lon = lon;
}

double
e_nav_world_item_location_lon_get(Evas_Object *item)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return 0;
   return locd->lon;
}

void
e_nav_world_item_location_unread_set(Evas_Object *item, uint unread)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;
   locd->unread = unread;
}

int
e_nav_world_item_location_unread_get(Evas_Object *item)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return 0;
   return locd->unread;
}

Diversity_Tag *
e_nav_world_item_location_tag_get(Evas_Object *item)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return 0;
   return locd->tag;
}

void
e_nav_world_item_location_timestamp_set(Evas_Object *item, time_t secs)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;
   locd->timestamp = secs;
}

int
e_nav_world_item_location_timestamp_get(Evas_Object *item)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return 0;
   return locd->timestamp;
}

void
e_nav_world_item_location_title_show(Evas_Object *location)
{
   Location_Data *locd;
   const char *text_part_state;
   double val_ret;
   char *time_diff_string;

   if(!location) return;
   text_part_state = edje_object_part_state_get(location, "e.text.name", &val_ret);
   locd = evas_object_data_get(location, "nav_world_item_location_data");

   if(!strcmp(text_part_state, "default")) 
     {
        time_diff_string = get_time_diff_string(locd->timestamp);
        edje_object_part_text_set(location, "e.text.name2", time_diff_string);
        free(time_diff_string);
        edje_object_signal_emit(location, "e,state,active", "e");
     }
}

Evas_Object *
e_nav_world_item_location_new(Evas_Object *nav, Diversity_Object *obj)
{
   Evas_Object *nwi;
   double lon, lat, width, height;
   char *name = NULL;
   char *description = NULL;
   int secs = 0;
   time_t timep;
   int unread;
   const char *obj_path;

   if(!obj) return NULL;
   obj_path = diversity_dbus_path_get((Diversity_DBus *)obj);

   diversity_object_geometry_get(obj, &lon, &lat, &width, &height);
   lat = -lat;
   diversity_dbus_property_get((Diversity_DBus *) obj, DIVERSITY_DBUS_IFACE_OBJECT, "Timestamp",  &secs);
   timep = (time_t)secs;
   nwi = e_nav_world_item_location_add(nav, THEME_PATH,
               lon, lat, obj);
   if(!nwi) return NULL;
   diversity_tag_prop_get((Diversity_Tag *) obj, "description", &description); 
   diversity_dbus_property_get((Diversity_DBus *) obj,
         DIVERSITY_DBUS_IFACE_TAG, "Unread", &unread);
   e_nav_world_item_location_unread_set(nwi, unread);

   name = strsep(&description, "\n");
   e_nav_world_item_location_name_set(nwi, name);
   e_nav_world_item_location_note_set(nwi, description);
   e_nav_world_item_location_timestamp_set(nwi, timep);

   e_ctrl_taglist_tag_add(name, description, timep, nwi); 

   e_ctrl_object_store_item_add( (void *)strdup(obj_path), (void *)nwi);
   e_nav_world_item_location_title_show(nwi);
   return nwi;
}

static char *
get_time_diff_string(time_t time_then)
{
   char time_diff_string[PATH_MAX];
   time_t time_now, time_diff;
   int days_diff;
   int today_secs;
   struct tm now, then;
   struct tm *now_p, *then_p;

   time(&time_now);  
   now_p = localtime(&time_now);
   memcpy(&now, now_p, sizeof(now));

   then_p = localtime(&time_then);
   memcpy(&then, then_p, sizeof(then));

   if(time_then > time_now) 
     {
        snprintf(time_diff_string, sizeof(time_diff_string),
                 "%s", ctime(&time_then));
     }

   if(now.tm_year != then.tm_year)
     {
        if(now.tm_year - then.tm_year == 1)
          snprintf(time_diff_string, sizeof(time_diff_string),
                   "Last year");
        else
          snprintf(time_diff_string, sizeof(time_diff_string),
                   "%d years ago", now.tm_year - then.tm_year);
        return strdup(time_diff_string);
     }
   else if(now.tm_mon != then.tm_mon)
     {
        if(now.tm_mon - then.tm_mon == 1)
          snprintf(time_diff_string, sizeof(time_diff_string),
                   "Last month");
        else
          snprintf(time_diff_string, sizeof(time_diff_string),
                   "%d months ago", now.tm_mon - then.tm_mon);
        return strdup(time_diff_string);
     }
   else 
     {
            today_secs = (now.tm_hour * 60 * 60) + (now.tm_min * 60) + now.tm_sec; 
            printf("today_secs = %d\n", today_secs);
            printf("time_now: %d, time_past: %d\n", (int)time_now, (int)time_then);
            time_diff = time_now - time_then;
            printf("time_diff = %d\n", (int)time_diff);
            if(time_diff >= today_secs) 
              {
                 days_diff = (time_diff - today_secs) / 86400;
                 if(days_diff == 0)
                   {
                      snprintf(time_diff_string, sizeof(time_diff_string),
                               "Yesterday");
                   }
                 else if((days_diff + 1) < 7)
                   {
                      snprintf(time_diff_string, sizeof(time_diff_string),
                               "%d days ago", days_diff + 1 );
                   }
                 else 
                   {
                      if(( (days_diff + 1) / 7 ) == 1)
                        snprintf(time_diff_string, sizeof(time_diff_string),
                                 "Last week");
                      else
                        snprintf(time_diff_string, sizeof(time_diff_string),
                                 "%d weeks ago", (days_diff + 1) / 7);
                   }

                 return strdup(time_diff_string);
              }
            else 
              {
                 return strdup("Today");
              } 
     }
}
