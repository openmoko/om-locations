/* e_nav_tileset.h -
 *
 * Copyright 2008 OpenMoko, Inc.
 * Authored by Chia-I Wu <olv@openmoko.com>
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

#ifndef E_NAV_TILESET_H
#define E_NAV_TILESET_H

#include <Evas.h>
#include <e_dbus_proxy.h>

/* this should be kept the same as Tileman_Format */
typedef enum _E_Nav_Tileset_Format {
   E_NAV_TILESET_FORMAT_OSM,
} E_Nav_Tileset_Format;

Evas_Object            *e_nav_tileset_add(Evas *e, E_Nav_Tileset_Format format, const char *dir);
void                    e_nav_tileset_update(Evas_Object *obj);

void                    e_nav_tileset_pos_set(Evas_Object *obj, double px, double py);
void                    e_nav_tileset_pos_get(Evas_Object *obj, double *px, double *py);

void                    e_nav_tileset_span_set(Evas_Object *obj, int span);
int                     e_nav_tileset_span_get(Evas_Object *obj);
void                    e_nav_tileset_span_range(Evas_Object *obj, int *min_span, int *max_span);

void                    e_nav_tileset_coord_to_pos(Evas_Object *obj, double lon, double lat, double *px, double *py);
void                    e_nav_tileset_coord_from_pos(Evas_Object *obj, double px, double py, double *lon, double *lat);
void                    e_nav_tileset_coord_range(Evas_Object *obj, double *abs_lon, double *abs_lat);

int                     e_nav_tileset_level_to_span(Evas_Object *obj, int level);
int                     e_nav_tileset_level_from_span(Evas_Object *obj, int span);
void                    e_nav_tileset_level_range(Evas_Object *obj, int *min_level, int *max_level);

void                    e_nav_tileset_monitor_add(Evas_Object *obj, const char *dn);
void                    e_nav_tileset_monitor_del(Evas_Object *obj, const char *dn);

void                    e_nav_tileset_proxy_set(Evas_Object *obj, E_DBus_Proxy *proxy);
E_DBus_Proxy           *e_nav_tileset_proxy_get(Evas_Object *obj);

#endif /* E_NAV_TILESET_H */
