/* e_nav_item_neo_me.h -
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

#ifndef E_NAV_ITEM_NEO_ME_H
#define E_NAV_ITEM_NEO_ME_H

#include "e_nav_dbus.h"

Evas_Object            *e_nav_world_item_neo_me_add(Evas_Object *nav, const char *theme_dir, double lon, double lat, Diversity_Bard *bard);
Diversity_Bard         *e_nav_world_item_neo_me_bard_get(Evas_Object *item);
Diversity_Equipment    *e_nav_world_item_neo_me_equipment_get(Evas_Object *item, const char *eqp_name);

void                    e_nav_world_item_neo_me_name_set(Evas_Object *item, const char *name);
const char             *e_nav_world_item_neo_me_name_get(Evas_Object *item);
void                    e_nav_world_item_neo_me_fixed_set(Evas_Object *item, Evas_Bool fixed);
Evas_Bool               e_nav_world_item_neo_me_fixed_get(Evas_Object *item);

void                    e_nav_world_item_neo_me_visible_set(Evas_Object *item, Evas_Bool active);
Evas_Bool               e_nav_world_item_neo_me_visible_get(Evas_Object *item);

void                    e_nav_world_item_neo_me_activate(Evas_Object *item);

#endif
