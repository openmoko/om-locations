/* e_nav_item_ap.c -
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
#include "e_nav_theme.h"
#include "e_nav_item_ap.h"
#include "e_spiralmenu.h"

typedef struct _AP_Data AP_Data;

struct _AP_Data
{
   Diversity_Object       *ap;
   const char             *essid;
   double                  range;
   E_Nav_Item_Ap_Key_Type  key_type;
   unsigned char           freed : 1;
   unsigned char           active : 1;
};

static void
on_zoom_clicked(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *nav;
   double lon, lat, w, h;
   
   nav = e_nav_world_item_nav_get(src_obj);
   e_nav_world_item_geometry_get(src_obj, &lon, &lat, &w, &h);
   e_nav_coord_set(nav, lon, lat, 0.5);
   e_nav_span_set(nav, E_NAV_SPAN_FROM_METERS(400), 0.5);
   e_spiralmenu_deactivate(obj);
}

static void
on_report_clicked(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("report\n");
   e_spiralmenu_deactivate(obj);
}

static void
on_information_clicked(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("information\n");
   e_spiralmenu_deactivate(obj);
}

static void
_e_nav_world_item_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *om;
   
   om = e_spiralmenu_add(evas);
   e_spiralmenu_theme_source_set(om, data);
   e_spiralmenu_autodelete_set(om, 1);
   e_spiralmenu_deacdelete_set(om, 1);
   e_spiralmenu_source_object_set(om, obj);

   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, _("Zoom"),
			       on_zoom_clicked, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, _("Report"),
			       on_report_clicked, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, _("Information"),
			       on_information_clicked, NULL);
   evas_object_show(om);
   e_spiralmenu_activate(om);
}

static void
_e_nav_world_item_cb_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   AP_Data *apd;
   
   apd = evas_object_data_get(obj, "nav_world_item_ap_data");
   if (!apd) return;
   if (apd->essid) eina_stringshare_del(apd->essid);
   free(apd);
}

/////////////////////////////////////////////////////////////////////////////
Evas_Object *
e_nav_world_item_ap_add(Evas_Object *nav, const char *theme_dir, double lon, double lat, Diversity_Object *ap)
{
   Evas_Object *o;
   AP_Data *apd;

   /* FIXME: allocate extra data struct for AP properites and attach to the
    * evas object */
   apd = calloc(1, sizeof(AP_Data));
   if (!apd) return NULL;
   o = e_nav_theme_object_new(evas_object_evas_get(nav), theme_dir,
				       "modules/diversity_nav/access_point");
   edje_object_part_text_set(o, "e.text.name", _("???"));
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_world_item_cb_mouse_down,
				  theme_dir);
   e_nav_world_item_add(nav, o);
   e_nav_world_item_type_set(o, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(o, lon, lat, 0, 0);
   e_nav_world_item_scale_set(o, 1);
   e_nav_world_item_update(o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
				  _e_nav_world_item_cb_del, NULL);

   apd->ap = ap;

   evas_object_data_set(o, "nav_world_item_ap_data", apd);
   evas_object_show(o);
   return o;
}

Diversity_Object *
e_nav_world_item_ap_ap_get(Evas_Object *item)
{
   AP_Data *apd;

   apd = evas_object_data_get(item, "nav_world_item_ap_data");
   if (!apd)
     return NULL;

   return apd->ap;
}

void
e_nav_world_item_ap_essid_set(Evas_Object *item, const char *essid)
{
   AP_Data *apd;
   
   apd = evas_object_data_get(item, "nav_world_item_ap_data");
   if (!apd) return;
   if (apd->essid) eina_stringshare_del(apd->essid);
   if (essid) apd->essid = eina_stringshare_add(essid);
   else apd->essid = NULL;
   edje_object_part_text_set(item, "e.text.name", apd->essid);
}

const char *
e_nav_world_item_ap_essid_get(Evas_Object *item)
{
   AP_Data *apd;
   
   apd = evas_object_data_get(item, "nav_world_item_ap_data");
   if (!apd) return NULL;
   return apd->essid;
}

void
e_nav_world_item_ap_key_type_set(Evas_Object *item, E_Nav_Item_Ap_Key_Type key)
{
   AP_Data *apd;
   
   apd = evas_object_data_get(item, "nav_world_item_ap_data");
   if (key == apd->key_type) return;
   if (!apd) return;
   apd->key_type = key;
   switch (apd->key_type)
     {
      case E_NAV_ITEM_AP_KEY_TYPE_NONE:
	edje_object_signal_emit(item, "e,state,key,none", "e");
	break;
      case E_NAV_ITEM_AP_KEY_TYPE_WEP:
	edje_object_signal_emit(item, "e,state,key,wep", "e");
	break;
      case E_NAV_ITEM_AP_KEY_TYPE_WPA:
	edje_object_signal_emit(item, "e,state,key,wpa", "e");
	break;
      default:
	break;
     }
}

E_Nav_Item_Ap_Key_Type
e_nav_world_item_ap_key_type_get(Evas_Object *item)
{
   AP_Data *apd;
   
   apd = evas_object_data_get(item, "nav_world_item_ap_data");
   if (!apd) return 0;
   return apd->key_type;
}

void
e_nav_world_item_ap_active_set(Evas_Object *item, Evas_Bool active)
{
   AP_Data *apd;
   
   apd = evas_object_data_get(item, "nav_world_item_ap_data");
   if (!apd) return;
   if ((active && apd->active) || ((!active) && (!apd->active))) return;
   apd->active = active;
   if (apd->active)
     edje_object_signal_emit(item, "e,state,active", "e");
   else
     edje_object_signal_emit(item, "e,state,passive", "e");
}

Evas_Bool
e_nav_world_item_ap_active_get(Evas_Object *item)
{
   AP_Data *apd;
   
   apd = evas_object_data_get(item, "nav_world_item_ap_data");
   if (!apd) return 0;
   return apd->active;
}

void
e_nav_world_item_ap_freed_set(Evas_Object *item, Evas_Bool freed)
{
   AP_Data *apd;
   
   apd = evas_object_data_get(item, "nav_world_item_ap_data");
   if (!apd) return;
   if ((freed && apd->freed) || ((!freed) && (!apd->freed))) return;
   apd->freed = freed;
   if (apd->freed)
     edje_object_signal_emit(item, "e,state,freed", "e");
   else
     edje_object_signal_emit(item, "e,state,closed", "e");
}

Evas_Bool
e_nav_world_item_ap_freed_get(Evas_Object *item)
{
   AP_Data *apd;
   
   apd = evas_object_data_get(item, "nav_world_item_ap_data");
   if (!apd) return 0;
   return apd->freed;
}

void
e_nav_world_item_ap_range_set(Evas_Object *item, double range)
{
   AP_Data *apd;
   double x, y;
   
   apd = evas_object_data_get(item, "nav_world_item_ap_data");
   if (!apd) return;
   if (apd->range == range) return;
   apd->range = range;
   e_nav_world_item_geometry_get(item, &x, &y, NULL, NULL);
   e_nav_world_item_geometry_set(item, x, y, apd->range * 2, apd->range * 2);
   e_nav_world_item_update(item);
}

double
e_nav_world_item_ap_range_get(Evas_Object *item)
{
   AP_Data *apd;
   
   apd = evas_object_data_get(item, "nav_world_item_ap_data");
   if (!apd) return 0.0;
   return apd->range;
}
