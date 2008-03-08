/* e_nav_item_location.h -
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

#ifndef E_NAV_ITEM_LOCATION_H
#define E_NAV_ITEM_LOCATION_H
#include "e_nav_dbus.h"

Evas_Object            *e_nav_world_item_location_add(Evas_Object *nav, const char *theme_dir, double lon, double lat, Diversity_Object *tag);
void                    e_nav_world_item_location_name_set(Evas_Object *item, const char *name);
const char             *e_nav_world_item_location_name_get(Evas_Object *item);
void                    e_nav_world_item_location_note_set(Evas_Object *item, const char *note);
const char             *e_nav_world_item_location_note_get(Evas_Object *item);
void                    e_nav_world_item_location_visible_set(Evas_Object *item, Evas_Bool active);
Evas_Bool               e_nav_world_item_location_visible_get(Evas_Object *item);
void                    e_nav_world_item_location_lat_set(Evas_Object *item, double lat);
double                  e_nav_world_item_location_lat_get(Evas_Object *item);
void                    e_nav_world_item_location_lon_set(Evas_Object *item, double lon);
double                  e_nav_world_item_location_lon_get(Evas_Object *item);
    
#endif
