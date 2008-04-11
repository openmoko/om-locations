/* e_nav_item_neo_me.c -
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
#include "e_nav_item_neo_me.h"
#include "e_spiralmenu.h"
#include "e_flyingmenu.h"
#include "widgets/e_nav_dialog.h"
#include "e_nav_dbus.h"

typedef struct _Neo_Me_Data Neo_Me_Data;

struct _Neo_Me_Data
{
   const char             *name;
   const char             *alias;
   const char             *twitter;
   unsigned char           visible : 1;
   unsigned char           fixed : 1;
   Diversity_Bard         *self;
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

#if 0
/* FIXME: real menu callbacks */
static void
_e_nav_world_item_cb_menu_1(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *nav;
   double lon, lat;
   
   nav = e_nav_world_item_nav_get(src_obj);
   e_nav_world_item_geometry_get(src_obj, &lon, &lat, NULL, NULL);
   e_nav_coord_set(nav, lon, lat, 0.5);
   e_nav_zoom_set(nav, 400, 0.5);
   e_spiralmenu_deactivate(obj);
}

static void
_e_nav_world_item_cb_menu_2(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb2\n");
   e_spiralmenu_deactivate(obj);
}

static void
_e_nav_world_item_cb_menu_3(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb3\n");
   e_spiralmenu_deactivate(obj);
}

static void
_e_nav_world_item_cb_menu_4(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb4\n");
   e_spiralmenu_deactivate(obj);
}
#endif

static void
dialog_exit(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("dialog exit\n");
   e_dialog_deactivate(obj);
}

static void
dialog_location_save(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("location new\n");
   if(!src_obj) return;
   const char *name = e_dialog_textblock_text_get(obj, "Edit title");
   const char *note = e_dialog_textblock_text_get(obj, "Edit message");
   printf("title = %s\n", name);
   printf("note = %s\n", note);

   char *description; 
   description = malloc(strlen(name) + 1 + strlen(note) + 1);
   if (!description) return ;
   sprintf(description, "%s%c%s", name, '\n', note);
   printf("description is %s\n", description);

   double lat, lon;
   e_nav_world_item_geometry_get(src_obj, &lon, &lat, NULL, NULL);
   printf("New a tag: %f, %f\n", lon, lat);

   Diversity_World *world = (Diversity_World*)e_nav_world_get();
   diversity_world_tag_add(world, lon, lat, description);
   e_dialog_deactivate(obj);
}

static void 
save_current_location(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   e_flyingmenu_deactivate(obj);
   Evas_Object *od = e_dialog_add(evas_object_evas_get(obj));
   e_dialog_theme_source_set(od, THEME_PATH);  
   e_dialog_source_object_set(od, src_obj);  
   e_dialog_title_set(od, "Save your location", "Save your current location");
   const char *title = "";
   e_dialog_textblock_add(od, "Edit title", title, 40, obj);
   const char *message = "";
   e_dialog_textblock_add(od, "Edit message", message, 120, obj);
   e_dialog_button_add(od, "Save", dialog_location_save, od);
   e_dialog_button_add(od, "Cancel", dialog_exit, od);
   
   evas_object_show(od);
   e_dialog_activate(od);
}

static void
_e_nav_world_item_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *om;
   
   om = e_flyingmenu_add(evas);
   e_flyingmenu_theme_source_set(om, THEME_PATH);
   e_flyingmenu_autodelete_set(om, 1);
   e_flyingmenu_source_object_set(om, obj);
   e_flyingmenu_theme_item_add(om, "modules/diversity_nav/tag_menu_item", 270, "Touch Me!",
			       save_current_location, obj);  
   evas_object_show(om);
   e_flyingmenu_activate(om);
#if 0
   Evas_Object *om;
   
   om = e_spiralmenu_add(evas);
   e_spiralmenu_theme_source_set(om, data);
   e_spiralmenu_autodelete_set(om, 1);
   e_spiralmenu_deacdelete_set(om, 1);
   e_spiralmenu_source_object_set(om, obj);
   /* FIXME: real menu items */
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Zoom",
			       _e_nav_world_item_cb_menu_1, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Visible",
			       _e_nav_world_item_cb_menu_2, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Auto Join",
			       _e_nav_world_item_cb_menu_3, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Information",
			       _e_nav_world_item_cb_menu_4, NULL);
   evas_object_show(om);
   e_spiralmenu_activate(om);
#endif
}

static void
_e_nav_world_item_cb_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Neo_Me_Data *neod;
   
   neod = evas_object_data_get(obj, "nav_world_item_neo_me_data");
   if (!neod) return;
   if (neod->name) evas_stringshare_del(neod->name);
   free(neod);
}

void
show_welcome_message(Evas_Object *item)
{
   Evas_Object *om;
   
   om = e_flyingmenu_add( evas_object_evas_get(item) );
   e_flyingmenu_theme_source_set(om, THEME_PATH);
   e_flyingmenu_autodelete_set(om, 1);
   e_flyingmenu_source_object_set(om, item);
   /* FIXME: real menu items */
   e_flyingmenu_theme_item_add(om, "modules/diversity_nav/tag_menu_item", 270, "Touch Me!",
			       save_current_location, item);  
   evas_object_show(om);
   e_flyingmenu_activate(om);
}

void
cosplay(Evas_Object *item, int fixed)
{
   if (fixed)
     edje_object_signal_emit(item, "FIXED", "phone");
   else
     edje_object_signal_emit(item, "NONFIXED", "phone");
}

/////////////////////////////////////////////////////////////////////////////
Evas_Object *
e_nav_world_item_neo_me_add(Evas_Object *nav, const char *theme_dir, double lon, double lat, Diversity_Bard *bard)
{
   Evas_Object *o;
   Neo_Me_Data *neod;
   int x, y, w, h;

   /* FIXME: allocate extra data struct for AP properites and attach to the
    * evas object */
   neod = calloc(1, sizeof(Neo_Me_Data));
   if (!neod) return NULL;
   neod->self = bard;
   o = _e_nav_world_item_theme_obj_new(evas_object_evas_get(nav), theme_dir,
				       "modules/diversity_nav/neo/me");
   edje_object_part_text_set(o, "e.text.name", "???");
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_world_item_cb_mouse_down,
				  theme_dir);
   evas_object_geometry_get(edje_object_part_object_get(o, "phone"), &x, &y, &w, &h);
   e_nav_world_item_add(nav, o);
   e_nav_world_item_type_set(o, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(o, lon, lat, w, h);
   e_nav_world_item_scale_set(o, 0);
   e_nav_world_item_update(o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
				  _e_nav_world_item_cb_del, NULL);
   evas_object_data_set(o, "nav_world_item_neo_me_data", neod);
   evas_object_show(o);
   return o;
}

void
e_nav_world_item_neo_me_name_set(Evas_Object *item, const char *name)
{
   Neo_Me_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_me_data");
   if (!neod) return;
   if (neod->name) evas_stringshare_del(neod->name);
   if (name) neod->name = evas_stringshare_add(name);
   else neod->name = NULL;
   edje_object_part_text_set(item, "e.text.name", neod->name);
}

const char *
e_nav_world_item_neo_me_name_get(Evas_Object *item)
{
   Neo_Me_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_me_data");
   if (!neod) return NULL;
   return neod->name;
}

void
e_nav_world_item_neo_me_visible_set(Evas_Object *item, Evas_Bool visible)
{
   Neo_Me_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_me_data");
   if (!neod) return;
   if ((visible && neod->visible) || ((!visible) && (!neod->visible))) return;
   neod->visible = visible;
   if (neod->visible)
     edje_object_signal_emit(item, "e,state,visible", "e");
   else
     edje_object_signal_emit(item, "e,state,invisible", "e");
}

Evas_Bool
e_nav_world_item_neo_me_visible_get(Evas_Object *item)
{
   Neo_Me_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_me_data");
   if (!neod) return 0;
   return neod->visible;
}

void
e_nav_world_item_neo_me_fixed_set(Evas_Object *item, Evas_Bool fixed)
{
   Neo_Me_Data *neod;

   neod = evas_object_data_get(item, "nav_world_item_neo_me_data");
   if (!neod) return;
   if ((fixed && neod->fixed) || ((!fixed) && (!neod->fixed))) return;
   neod->fixed = fixed;
   if (neod->fixed)
     edje_object_signal_emit(item, "FIXED", "phone");
   else
     edje_object_signal_emit(item, "NONFIXED", "phone");
}

Evas_Bool
e_nav_world_item_neo_me_fixed_get(Evas_Object *item)
{
   Neo_Me_Data *neod;

   neod = evas_object_data_get(item, "nav_world_item_neo_me_data");
   if (!neod) return 0;
   return neod->fixed;
}

Diversity_Bard *
e_nav_world_item_neo_me_bard_get(Evas_Object *item)
{
   Neo_Me_Data *neod;

   neod = evas_object_data_get(item, "nav_world_item_neo_me_data");
   if (!neod) return 0;
   return neod->self;
}

