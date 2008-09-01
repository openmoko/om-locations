/* tileman.h -
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

#ifndef TILEMAN_H
#define TILEMAN_H

#include <e_dbus_proxy.h>
#include "e_nav.h"

typedef struct _Tileman Tileman;

typedef enum _Tileman_Format {
   TILEMAN_FORMAT_OSM,
} Tileman_Format;

Tileman                *tileman_new(Evas *e, Tileman_Format format, const char *dir);
void                    tileman_destroy(Tileman *tman);

void                    tileman_levels_list(Tileman *tman, int *max_level, int *min_level);
int                     tileman_tile_size_get(Tileman *tman);

void                    tileman_proxy_set(Tileman *tman, E_DBus_Proxy *proxy);
E_DBus_Proxy           *tileman_proxy_get(Tileman *tman);
void                    tileman_proxy_level_set(Tileman *tman, int level);
int                     tileman_proxy_level_get(Tileman *tman);

Evas_Object            *tileman_tile_add(Tileman *tman);
int                     tileman_tile_load(Evas_Object *tile, int z, int x, int y);
int                     tileman_tile_image_set(Evas_Object *tile, const char *path, const char *key);

int                     tileman_tile_download(Evas_Object *tile, int z, int x, int y);
void                    tileman_tile_cancel(Evas_Object *tile);

#endif /* TILEMAN_H */
