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
#include "e_nav_theme.h"
#include "e_nav_item_neo_me.h"
#include "e_flyingmenu.h"
#include "widgets/e_nav_dialog.h"
#include "e_nav_dbus.h"
#include "e_nav_item_location.h"

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

static void
location_save_dialog_show(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   double lon, lat;

   e_flyingmenu_deactivate(obj);

   e_nav_world_item_geometry_get(src_obj, &lon, &lat, NULL, NULL);
   e_nav_world_item_location_action_new(src_obj, lon, lat);
}

static void
_e_nav_world_item_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   e_nav_world_item_neo_me_activate(obj);
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
   o = e_nav_theme_object_new(evas_object_evas_get(nav), theme_dir,
				       "modules/diversity_nav/neo/me");
   edje_object_part_text_set(o, "e.text.name", _("???"));
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
e_nav_world_item_neo_me_activate(Evas_Object *item)
{
   Evas_Object *om;

   om = e_flyingmenu_add( evas_object_evas_get(item) );
   e_flyingmenu_theme_source_set(om, THEMEDIR);
   e_flyingmenu_autodelete_set(om, 1);
   e_flyingmenu_source_object_set(om, item);
   e_flyingmenu_theme_item_add(om, "modules/diversity_nav/tag_menu_item", 270, _("touch me!"),
			       location_save_dialog_show, item);
   evas_object_show(om);
   e_flyingmenu_activate(om);
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
