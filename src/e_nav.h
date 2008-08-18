/* e_nav.h -
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

#ifndef E_NAV_H
#define E_NAV_H

#include "config.h"
#include <Ecore.h>
#include <Evas.h>
#include <Ecore_Data.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <E_DBus.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define M_EARTH_RADIUS	(6371.0 * 1000.0)

/* handy defines */
#define E_NAV_SPAN_MAX (256 * (1 << 16))
#define E_NAV_SPAN_MIN (256 * 4)
#define E_NAV_SPAN_FROM_METERS(m) ((int) (M_EARTH_RADIUS * M_PI * 2 / m))

/* Etk.h defines _ */
#ifdef _
#undef _
#endif

#if ENABLE_NLS
#include <libintl.h>
#define _(str) gettext(str)
#define N_(str) (str)
#else
#define _(str) (str)
#define N_(str) (str)
#endif

/* types */
typedef struct _E_Nav_Location E_Nav_Location; /* opaque object */

typedef enum _E_Nav_World_Item_Type
{
   E_NAV_WORLD_ITEM_TYPE_WALLPAPER,
     E_NAV_WORLD_ITEM_TYPE_ITEM,
     E_NAV_WORLD_ITEM_TYPE_OVERLAY,
     E_NAV_WORLD_ITEM_TYPE_LINKED
} E_Nav_World_Item_Type;

/* object management */
Evas_Object    *e_nav_add(Evas *e, void *world);
void            e_nav_theme_source_set(Evas_Object *obj, const char *custom_dir);

void           *e_nav_world_get();
    
/* spatial & zoom controls */
void            e_nav_coord_set(Evas_Object *obj, double lon, double lat, double when);
double          e_nav_coord_lon_get(Evas_Object *obj);
double          e_nav_coord_lat_get(Evas_Object *obj);
void            e_nav_span_set(Evas_Object *obj, int span, double when);
int             e_nav_span_get(Evas_Object *obj);

void            e_nav_level_up(Evas_Object *obj);
void            e_nav_level_down(Evas_Object *obj);
void            e_nav_move_up(Evas_Object *obj);
void            e_nav_move_down(Evas_Object *obj);
void            e_nav_move_left(Evas_Object *obj);
void            e_nav_move_right(Evas_Object *obj);

/* world items */
void                   e_nav_world_item_add(Evas_Object *obj, Evas_Object *item);
void                   e_nav_world_item_delete(Evas_Object *obj, Evas_Object *item);
void                   e_nav_world_item_type_set(Evas_Object *item, E_Nav_World_Item_Type type);
E_Nav_World_Item_Type  e_nav_world_item_type_get(Evas_Object *item);
void                   e_nav_world_item_geometry_set(Evas_Object *item, double x, double y, double w, double h);
void                   e_nav_world_item_geometry_get(Evas_Object *item, double *x, double *y, double *w, double *h);
void                   e_nav_world_item_scale_set(Evas_Object *item, Evas_Bool scale);
Evas_Bool              e_nav_world_item_scale_get(Evas_Object *item);
void                   e_nav_world_item_update(Evas_Object *item);
Evas_Object           *e_nav_world_item_nav_get(Evas_Object *item);

int                    e_nav_edje_object_set(Evas_Object *o, const char *category, const char *group);

/* world tilesets */
void                   e_nav_world_tileset_add(Evas_Object *obj, Evas_Object *nt);
#endif
