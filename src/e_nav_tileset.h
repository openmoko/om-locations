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

#include "e_nav.h"

#define RADIANS(d) ((d) * M_PI / 180.0)
#define DEGREES(d) ((d) * 180.0 / M_PI)
#define M_LOG2		(0.693147181)

typedef enum _E_Nav_Tileset_Format {
   E_NAV_TILESET_FORMAT_OSM,
} E_Nav_Tileset_Format;

Evas_Object            *e_nav_tileset_add(Evas_Object *nav, E_Nav_Tileset_Format format, const char *dir);
void                    e_nav_tileset_update(Evas_Object *obj);

int                     e_nav_tileset_span_set(Evas_Object *obj, int span);
int                     e_nav_tileset_span_get(Evas_Object *obj);

void                    e_nav_tileset_pos_set(Evas_Object *obj, double px, double py, int scaled);
void                    e_nav_tileset_pos_get(Evas_Object *obj, double *px, double *py, int scaled);

int                     e_nav_tileset_to_pos(Evas_Object *obj, double lon, double lat, double *px, double *py, int scaled);
int                     e_nav_tileset_from_pos(Evas_Object *obj, double px, double py, double *lon, double *lat, int scaled);

/* decide span by map level */
int                     e_nav_tileset_level_set(Evas_Object *obj, int level);
int                     e_nav_tileset_level_get(Evas_Object *obj);
void                    e_nav_tileset_levels_list(Evas_Object *obj, int *max_level, int *min_level);

int                     e_nav_tileset_center_set(Evas_Object *obj, double lon, double lat);
int                     e_nav_tileset_center_get(Evas_Object *obj, double *lon, double *lat);

int                     e_nav_tileset_to_offsets(Evas_Object *obj, double lon, double lat, double *x, double *y);
int                     e_nav_tileset_from_offsets(Evas_Object *obj, double x, double y, double *lon, double *lat);

void                    e_nav_tileset_monitor_add(Evas_Object *obj, const char *dn);
void                    e_nav_tileset_monitor_del(Evas_Object *obj, const char *dn);

#include <e_dbus_proxy.h>
void                    e_nav_tileset_proxy_set(Evas_Object *obj, E_DBus_Proxy *proxy);
E_DBus_Proxy           *e_nav_tileset_proxy_get(Evas_Object *obj);

#endif /* E_NAV_TILESET_H */
