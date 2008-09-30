/* e_nav_item_ap.h -
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

#ifndef E_NAV_ITEM_AP_H
#define E_NAV_ITEM_AP_H

#include "e_nav_dbus.h"

typedef enum _E_Nav_Item_Ap_Key_Type
{
   E_NAV_ITEM_AP_KEY_TYPE_NONE,
     E_NAV_ITEM_AP_KEY_TYPE_WEP,
     E_NAV_ITEM_AP_KEY_TYPE_WPA
} E_Nav_Item_Ap_Key_Type;

Evas_Object            *e_nav_world_item_ap_add(Evas_Object *nav, const char *theme_dir, double lon, double lat, Diversity_Object *ap);
Diversity_Object       *e_nav_world_item_ap_ap_get(Evas_Object *item);

void                    e_nav_world_item_ap_essid_set(Evas_Object *item, const char *essid);
const char             *e_nav_world_item_ap_essid_get(Evas_Object *item);
void                    e_nav_world_item_ap_key_type_set(Evas_Object *item, E_Nav_Item_Ap_Key_Type key);
E_Nav_Item_Ap_Key_Type  e_nav_world_item_ap_key_type_get(Evas_Object *item);
void                    e_nav_world_item_ap_active_set(Evas_Object *item, Evas_Bool active);
Evas_Bool               e_nav_world_item_ap_active_get(Evas_Object *item);
void                    e_nav_world_item_ap_freed_set(Evas_Object *item, Evas_Bool freed);
Evas_Bool               e_nav_world_item_ap_freed_get(Evas_Object *item);
void                    e_nav_world_item_ap_range_set(Evas_Object *item, double range);
double                  e_nav_world_item_ap_range_get(Evas_Object *item);

#endif
