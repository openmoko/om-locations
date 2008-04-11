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
#include "e_nav_item_location.h"
#include "e_flyingmenu.h"
#include "widgets/e_nav_dialog.h"
#include "widgets/e_nav_textedit.h"
#include "e_ctrl.h"

typedef struct _Location_Data Location_Data;

struct _Location_Data
{
   const char             *name;
   const char             *note;
   unsigned char           visible : 1;
   double                  lat;
   double                  lon;
   Diversity_Tag          *tag;
};

static Evas_Object *
_e_nav_world_item_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
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
dialog_exit(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   e_dialog_deactivate(obj);
}

static void
dialog_location_save(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Location_Data *locd;
   locd = evas_object_data_get(src_obj, "nav_world_item_location_data");
   if (!locd) return;
   const char *name = e_dialog_textblock_text_get(obj, "Edit title");
   const char *note = e_dialog_textblock_text_get(obj, "Edit message");
   char *description; 
   description = malloc(strlen(name) + 1 + strlen(note) + 1);
   if (!description) return ;
   sprintf(description, "%s%c%s", name, '\n', note);
   int result = diversity_tag_prop_set(locd->tag, "description", description);
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
   e_dialog_deactivate(obj);
}

static void
dialog_location_delete(void *data, Evas_Object *obj, Evas_Object *src_obj)
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
   e_dialog_deactivate(obj);
}

static void
location_send(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Diversity_Bard *self;
   Diversity_Equipment *eqp = NULL;
   Textedit_List_Item *item;
   Neo_Other_Data *neod;

   int ok = FALSE;
   const char *input = e_textedit_input_get(obj); 

   if(input==NULL)
     { 
        e_textedit_deactivate(obj);   
        return;
     }

   Evas_Object *location_object = (Evas_Object*)data;
   Location_Data *locd;
   locd = evas_object_data_get(location_object, "nav_world_item_location_data");
   if (!locd) return;

   self = (Diversity_Bard *)e_ctrl_self_get();
   if (self)
     eqp = diversity_bard_equipment_get(self, "phonekit");
   if (!eqp) return;

   void *selected_item = e_textedit_list_selected_get(obj);
   if(selected_item)
     {
        item = (Textedit_List_Item *)selected_item;
        neod = (Neo_Other_Data *)item->data;
        if(!neod) return;
        if(!strcmp(neod->name, input))
          {
             printf("PhoneKit equipment Got , tag share ..... \n");
             ok = diversity_sms_tag_share((Diversity_Sms *)eqp, neod->bard, locd->tag);

             Evas_Object *od = e_dialog_add(evas_object_evas_get(obj));
             e_dialog_theme_source_set(od, THEME_PATH);
             e_dialog_source_object_set(od, src_obj);     // dialog's src_obj is location item
             if (ok)
               {
                  e_dialog_title_set(od, "Success", "Tag sent");
                  e_dialog_button_add(od, "OK", dialog_exit, od);
               }
             else 
               {
                  e_dialog_title_set(od, "Fail", "Tag sent");
                  e_dialog_button_add(od, "OK", dialog_exit, od);
               }
             e_textedit_deactivate(obj);   
             evas_object_show(od);
             e_dialog_activate(od); 
             return;
          }
     }

   /* lookup contact by name  */
   /* FIXME: add support of looking up by phone number*/
   item = e_textedit_list_item_get_by_name(obj, input);
   if(item)
     {
        neod = (Neo_Other_Data *)item->data;
        if(!neod) return;
        ok = diversity_sms_tag_share((Diversity_Sms *)eqp, neod->bard, locd->tag);
        Evas_Object *od = e_dialog_add(evas_object_evas_get(obj));
        e_dialog_theme_source_set(od, THEME_PATH);
        e_dialog_source_object_set(od, src_obj);     
        if (ok)
          {
             e_dialog_title_set(od, "Success", "Tag sent");
             e_dialog_button_add(od, "OK", dialog_exit, od);
          }
        else 
          {
             e_dialog_title_set(od, "Fail", "Tag sent");
             e_dialog_button_add(od, "OK", dialog_exit, od);
          }
        e_textedit_deactivate(obj);   
        evas_object_show(od);
        e_dialog_activate(od); 
        return;
     }
   
   /* Treat input as phone number; using phone number to send */
   char *phone_number;
   char *message;
   int ask_ds = 0;
   phone_number = strdup(input); 
   message = malloc(strlen(locd->name) + 1 + strlen(locd->note) + 1);
   if (!message) return ;
   sprintf(message, "%s%c%s", locd->name, '\n', locd->note);
   printf("phone number is %s, message is %s\n", phone_number, message);

   ok = diversity_sms_send((Diversity_Sms *)eqp, phone_number, message, ask_ds);
   free(phone_number);
   free(message);

   Evas_Object *od = e_dialog_add(evas_object_evas_get(obj));
   e_dialog_theme_source_set(od, THEME_PATH);
   e_dialog_source_object_set(od, src_obj);     
   if(ok)
     {
        e_dialog_title_set(od, "Success", "Tag sent");
        e_dialog_button_add(od, "OK", dialog_exit, od);
     }
   else
     {
        e_dialog_title_set(od, "Fail", "Tag sent");
        e_dialog_button_add(od, "OK", dialog_exit, od);
     }
   e_textedit_deactivate(obj);   
   evas_object_show(od);
   e_dialog_activate(od); 
}

static void
dialog_location_send(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *teo = e_textedit_add( evas_object_evas_get(obj) );
   e_textedit_theme_source_set(teo, THEME_PATH, location_send, data, NULL, NULL); // data is the location item evas object 
   e_textedit_source_object_set(teo, data);  //  src_object is location item evas object 
   e_textedit_input_set(teo, "To:", "");
   Ecore_List *contacts = e_ctrl_contacts_get(); 
   int count = ecore_list_count(contacts);
   int n;
   Ecore_List *list;
   Textedit_List_Item *tli;
   list = ecore_list_new();
   for(n=0; n<count; n++)
     {
        Neo_Other_Data *neod = ecore_list_index_goto(contacts, n);
        if(neod)
          {
              tli = calloc(1, sizeof(Textedit_List_Item));
              if (!tli) return; 
              if (neod->name)
                tli->name = strdup(neod->name);
              else 
                tli->name = strdup("");
              tli->data = neod;
              ecore_list_append(list, tli);
          }
     }
   e_textedit_candidate_list_set(teo, list);
   e_textedit_candidate_mode_set(teo, TEXTEDIT_CANDIDATE_MODE_TRUE);
   e_dialog_deactivate(obj);
   evas_object_show(teo);
   e_textedit_activate(teo);
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
   e_dialog_textblock_add(od, "Edit title", title, 40, obj);
   const char *message = e_nav_world_item_location_note_get(location_object);
   e_dialog_textblock_add(od, "Edit message", message, 120, obj);
   e_dialog_button_add(od, "Save", dialog_location_save, od);
   e_dialog_button_add(od, "Delete", dialog_location_delete, od);
   e_dialog_button_add(od, "Cancel", dialog_exit, od);
   
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
   e_dialog_textblock_add(od, "Edit title", title, 40, obj);
   const char *message = e_nav_world_item_location_note_get(location_object);
   e_dialog_textblock_add(od, "Edit message", message, 120, obj);
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
   e_flyingmenu_theme_item_add(om, "modules/diversity_nav/tag_menu_item", 100, "Edit",
			       _e_nav_world_item_cb_menu_1, obj);
   e_flyingmenu_theme_item_add(om, "modules/diversity_nav/tag_menu_item", 100, "Send",
			       _e_nav_world_item_cb_menu_2, obj);
   evas_object_show(om);
   e_flyingmenu_activate(om);
}

static void
_e_nav_world_item_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   const char *text_part_state;
   double val_ret;
   
   text_part_state = edje_object_part_state_get(obj, "e.text.name", &val_ret);

   if(!strcmp(text_part_state, "default")) 
     {
        edje_object_signal_emit(obj, "e,state,active", "e");
     }
   else 
     {
        edje_object_signal_emit(obj, "e,state,passive", "e");
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

   /* FIXME: allocate extra data struct for AP properites and attach to the
    * evas object */
   locd = calloc(1, sizeof(Location_Data));
   if (!locd) return NULL;
   locd->tag = (Diversity_Tag *) tag;
   o = _e_nav_world_item_theme_obj_new(evas_object_evas_get(nav), theme_dir,
				       "modules/diversity_nav/location");
   edje_object_part_text_set(o, "e.text.name", "???");
   edje_object_signal_callback_add(o, "MENU_ACTIVATE", "e.text.name", cb_menu_activate, (void *)theme_dir);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_world_item_cb_mouse_down,
				  theme_dir);
   e_nav_world_item_add(nav, o);
   e_nav_world_item_type_set(o, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(o, lon, lat, 0, 0);
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
     edje_object_part_text_set(item, "e.text.name", "???");
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


