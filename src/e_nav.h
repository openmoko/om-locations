/* e_nav.h -
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

#if ENABLE_DEBUG
#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), SMART_NAME)) return ret
#else
#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj)
#endif

typedef enum _E_Nav_World_Item_Type
{
   E_NAV_WORLD_ITEM_TYPE_WALLPAPER,
   E_NAV_WORLD_ITEM_TYPE_ITEM,
   E_NAV_WORLD_ITEM_TYPE_OVERLAY,
   E_NAV_WORLD_ITEM_TYPE_LINKED
} E_Nav_World_Item_Type;

/* object management */
Evas_Object    *e_nav_add(Evas *e);
void            e_nav_theme_source_set(Evas_Object *obj, const char *custom_dir);

void            e_nav_world_set(Evas_Object *obj, void *world);
void           *e_nav_world_get(Evas_Object *obj);

void            e_nav_world_neo_me_set(Evas_Object *obj, Evas_Object *me);
Evas_Object    *e_nav_world_neo_me_get(Evas_Object *obj);

/* ctrl is owned by caller */
void            e_nav_world_ctrl_set(Evas_Object *obj, Evas_Object *ctrl);
Evas_Object    *e_nav_world_ctrl_get(Evas_Object *obj);

/* tileset will be added to e_nav */
void            e_nav_world_tileset_set(Evas_Object *obj, Evas_Object *nt);

/* spatial & zoom controls */
void            e_nav_pos_set(Evas_Object *obj, double px, double py, double when);
void            e_nav_pos_get(Evas_Object *obj, double *px, double *py);

void            e_nav_coord_set(Evas_Object *obj, double lon, double lat, double when);
void            e_nav_coord_get(Evas_Object *obj, double *lon, double *lat);

void            e_nav_span_set(Evas_Object *obj, int span, double when);
int             e_nav_span_get(Evas_Object *obj);

void            e_nav_level_set(Evas_Object *obj, int level, double when);
int             e_nav_level_get(Evas_Object *obj);

/* world items */
void                   e_nav_world_item_add(Evas_Object *obj, Evas_Object *item);
void                   e_nav_world_item_delete(Evas_Object *obj, Evas_Object *item);
void                   e_nav_world_item_type_set(Evas_Object *item, E_Nav_World_Item_Type type);

E_Nav_World_Item_Type  e_nav_world_item_type_get(Evas_Object *item);
Evas_Object           *e_nav_world_item_nav_get(Evas_Object *item);

void                   e_nav_world_item_scale_set(Evas_Object *item, Evas_Bool scale);
Evas_Bool              e_nav_world_item_scale_get(Evas_Object *item);
void                   e_nav_world_item_coord_set(Evas_Object *item, double lon, double lat);
void                   e_nav_world_item_coord_get(Evas_Object *item, double *lon, double *lat);
void                   e_nav_world_item_size_set(Evas_Object *item, double w, double h);
void                   e_nav_world_item_size_get(Evas_Object *item, double *w, double *h);
void                   e_nav_world_item_geometry_set(Evas_Object *item, double x, double y, double w, double h);
void                   e_nav_world_item_geometry_get(Evas_Object *item, double *x, double *y, double *w, double *h);

void                   e_nav_world_item_update(Evas_Object *item);

void                   e_nav_world_item_lower(Evas_Object *item);
void                   e_nav_world_item_raise(Evas_Object *item);
void                   e_nav_world_item_focus(Evas_Object *item);

#endif
