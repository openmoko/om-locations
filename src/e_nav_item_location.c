#include "e_nav.h"
#include "e_nav_item_location.h"
#include "e_tagmenu.h"
#include "widgets/e_nav_dialog.h"
#include "widgets/e_nav_textedit.h"

typedef struct _Location_Data Location_Data;

struct _Location_Data
{
   const char             *name;
   const char             *description;
   unsigned char           visible : 1;
   double                  lat;
   double                  lon;
};

static Evas_Object *
_e_nav_world_item_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;
   
   o = edje_object_add(e);
   if (!e_nav_edje_object_set(o, "diversity_nav", group))
     {
	if (custom_dir)
	  {
	     char buf[PATH_MAX];
	     
	     snprintf(buf, sizeof(buf), "%s/diversity_nav.edj", custom_dir);
	     edje_object_file_set(o, buf, group);
	  }
     }
   return o;
}

static void
dialog_exit(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("dialog exit\n");
   e_dialog_deactivate(obj);
}

static void
dialog_location_save(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("location save\n");
   e_dialog_deactivate(obj);
   //ToDo:  Save to location book
}

static void
location_send(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   printf("Send SMS\n");
   e_textedit_deactivate(data);
   //ToDo:  call SMS to send
}

static void
dialog_location_send(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("location send\n");
   e_dialog_deactivate(obj);
   Evas_Object *teo = e_textedit_add( evas_object_evas_get(obj) );
   e_textedit_theme_source_set(teo, THEME_PATH, location_send, NULL);  
   e_textedit_source_object_set(teo, NULL); 
   e_textedit_input_set(teo, "To:", "");
   evas_object_show(teo);
   e_textedit_activate(teo);
   //ToDo: choose  contact, select one contact 
}

static void
_e_nav_world_item_cb_menu_1(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb1\n");
   Evas_Object *location_object = (Evas_Object*)data;
   Location_Data *locd;
   locd = evas_object_data_get(location_object, "nav_world_item_location_data");
   if (!locd) return;

   e_tagmenu_deactivate(obj);
   Evas_Object *od = e_dialog_add(evas_object_evas_get(obj));
   e_dialog_theme_source_set(od, THEME_PATH);  
   e_dialog_source_object_set(od, src_obj);  
   e_dialog_title_set(od, "Edit your location", "Send your favorite location to friends by SMS");
   const char *title = e_nav_world_item_location_name_get(location_object);
   e_dialog_textblock_add(od, "Edit title", title, 40, obj);
   const char *message = e_nav_world_item_location_description_get(location_object);
   e_dialog_textblock_add(od, "Edit message", message, 120, obj);
   e_dialog_button_add(od, "Save", dialog_location_save, od);
   e_dialog_button_add(od, "Cancel", dialog_exit, od);
   
   evas_object_show(od);
   e_dialog_activate(od);
}

static void
_e_nav_world_item_cb_menu_2(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb2\n");
   Evas_Object *location_object = (Evas_Object*)data;
   Location_Data *locd;
   locd = evas_object_data_get(location_object, "nav_world_item_location_data");
   if (!locd) return;

   e_tagmenu_deactivate(obj);
   Evas_Object *od = e_dialog_add(evas_object_evas_get(obj));
   e_dialog_theme_source_set(od, THEME_PATH);  
   e_dialog_source_object_set(od, src_obj);  
   e_dialog_title_set(od, "Send your location", "Send your favorite location to friends by SMS");
   const char *title = e_nav_world_item_location_name_get(location_object);
   e_dialog_textblock_add(od, "Edit title", title, 40, obj);
   const char *message = e_nav_world_item_location_description_get(location_object);
   e_dialog_textblock_add(od, "Edit message", message, 120, obj);
   e_dialog_button_add(od, "Send", dialog_location_send, od);
   e_dialog_button_add(od, "Cancel", dialog_exit, od);
   
   evas_object_show(od);
   e_dialog_activate(od);
}

static void
_e_nav_world_item_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *om;
   
   om = e_tagmenu_add(evas);
   e_tagmenu_theme_source_set(om, data);
   e_tagmenu_autodelete_set(om, 1);
   e_tagmenu_source_object_set(om, obj);
   /* FIXME: real menu items */
   e_tagmenu_theme_item_add(om, "modules/diversity_nav/tag_menu_item", 100, "Edit",
			       _e_nav_world_item_cb_menu_1, obj);
   e_tagmenu_theme_item_add(om, "modules/diversity_nav/tag_menu_item", 100, "Send",
			       _e_nav_world_item_cb_menu_2, obj);
   evas_object_show(om);
   e_tagmenu_activate(om);
}

static void
_e_nav_world_item_cb_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(obj, "nav_world_item_location_data");
   if (!locd) return;
   if (locd->name) evas_stringshare_del(locd->name);
   if (locd->description) evas_stringshare_del(locd->description);
   free(locd);
}

/////////////////////////////////////////////////////////////////////////////
Evas_Object *
e_nav_world_item_location_add(Evas_Object *nav, const char *theme_dir, double lon, double lat)
{
   Evas_Object *o;
   Location_Data *locd;

   /* FIXME: allocate extra data struct for AP properites and attach to the
    * evas object */
   locd = calloc(1, sizeof(Location_Data));
   if (!locd) return NULL;
   o = _e_nav_world_item_theme_obj_new(evas_object_evas_get(nav), theme_dir,
				       "modules/diversity_nav/location");
   edje_object_part_text_set(o, "e.text.name", "???");
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
e_nav_world_item_location_description_set(Evas_Object *item, const char *description)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return;
   if (locd->description) evas_stringshare_del(locd->description);
   if (description) locd->description = evas_stringshare_add(description);
   else locd->description = NULL;
}

const char *
e_nav_world_item_location_description_get(Evas_Object *item)
{
   Location_Data *locd;
   
   locd = evas_object_data_get(item, "nav_world_item_location_data");
   if (!locd) return NULL;
   return locd->description;
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

