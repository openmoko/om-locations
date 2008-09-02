/* e_nav_list.h -
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

#ifndef E_NAV_LIST_H
#define E_NAV_LIST_H

#include <Evas.h>

enum {
     E_NAV_LIST_TYPE_AP,
     E_NAV_LIST_TYPE_BARD,
     E_NAV_LIST_TYPE_TAG,
};

Evas_Object    *e_nav_list_add(Evas *e, int type);

void            e_nav_list_title_set(Evas_Object *li, const char *title);
void            e_nav_list_sort_set(Evas_Object *li, int (*func)(void *data, Evas_Object *obj1, Evas_Object *obj2), void *data);

void            e_nav_list_button_add(Evas_Object *li, const char *label, void (*func)(void *data, Evas_Object *li), void *data);
void            e_nav_list_button_remove(Evas_Object *li, void (*func)(void *data, Evas_Object *li), void *data);

void            e_nav_list_callback_add(Evas_Object *li, void (*func)(void *data, Evas_Object *li, Evas_Object *obj), void *data);
void            e_nav_list_callback_del(Evas_Object *li, void *func, void *data);

void            e_nav_list_object_add(Evas_Object *li, Evas_Object *obj);
void            e_nav_list_object_remove(Evas_Object *li, Evas_Object *obj);
void            e_nav_list_object_update(Evas_Object *li, Evas_Object *obj);
void            e_nav_list_clear(Evas_Object *li);

#endif /* E_NAV_LIST_H */
