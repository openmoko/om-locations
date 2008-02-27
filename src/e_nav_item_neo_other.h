/* e_nav_item_neo_other.h -
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

#ifndef E_NAV_ITEM_NEO_OTHER_H
#define E_NAV_ITEM_NEO_OTHER_H

Evas_Object            *e_nav_world_item_neo_other_add(Evas_Object *nav, const char *theme_dir, double lon, double lat);
void                    e_nav_world_item_neo_other_name_set(Evas_Object *item, const char *name);
const char             *e_nav_world_item_neo_other_name_get(Evas_Object *item);
    
#endif
