/* e_ctrl.h -
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

#ifndef E_CTRL_H
#define E_CTRL_H
#include <Ecore.h>
#include <Evas.h>
#include <Edje.h>

typedef enum _E_Nav_Show_Mode {
   E_NAV_SHOW_MODE_TAGLESS,
   E_NAV_SHOW_MODE_TAG,
} E_Nav_Show_Mode;

typedef enum _E_Nav_View_Mode {
   E_NAV_VIEW_MODE_MAP,
   E_NAV_VIEW_MODE_SAT,
   E_NAV_VIEW_MODE_LIST,
} E_Nav_View_Mode;

Evas_Object * e_ctrl_add(Evas *e);
void e_ctrl_theme_source_set(Evas_Object *obj, const char *custom_dir);
void e_ctrl_nav_set(Evas_Object* obj);
void e_ctrl_zoom_drag_value_set(double y); 
void e_ctrl_zoom_text_value_set(const char* buf);
void e_ctrl_longitude_set(const char* buf);
void e_ctrl_latitude_set(const char* buf);
int                    e_ctrl_edje_object_set(Evas_Object *o, const char *category, const char *group);

#endif
