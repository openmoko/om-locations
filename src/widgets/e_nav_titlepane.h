/* e_nav_tilepane.h -
 *
 * Copyright 2008 OpenMoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
 *
 * This work is based on e17 project.  See also COPYING.e17.
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

#ifndef E_NAV_TITLEPANE_H
#define E_NAV_TITLEPANE_H
#include <Ecore.h>
#include <Evas.h>
#include <Edje.h>
#include <Ecore_Evas.h>

typedef void (* CallbackFunc) (void *data, Evas *evas, Evas_Object *obj, void *event);

Evas_Object * e_nav_titlepane_add(Evas *e);
void e_nav_titlepane_theme_source_set(Evas_Object *obj, const char *custom_dir);
void e_nav_titlepane_source_object_set(Evas_Object *obj, void *src_obj);
Evas_Object * e_nav_titlepane_source_object_get(Evas_Object *obj);
//int e_nav_titlepane_edje_object_set(Evas_Object *o, const char *category, const char *group);
void e_nav_titlepane_set_message(Evas_Object *obj, const char *message);
void e_nav_titlepane_set_left_button(Evas_Object *obj, const char *text, CallbackFunc cb, Evas_Object *src);  
void e_nav_titlepane_set_right_button(Evas_Object *obj, const char *text, CallbackFunc cb, Evas_Object *src);
void e_nav_titlepane_hide_buttons(Evas_Object *obj);

#endif
