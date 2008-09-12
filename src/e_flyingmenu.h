/* e_flyingmenu.h -
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

#ifndef E_FLYINGMENU_H
#define E_FLYINGMENU_H

Evas_Object    *e_flyingmenu_add(Evas *e);
void            e_flyingmenu_theme_source_set(Evas_Object *obj, const char *custom_dir);

void            e_flyingmenu_source_object_set(Evas_Object *obj, Evas_Object *src_obj);
Evas_Object    *e_flyingmenu_source_object_get(Evas_Object *obj);
void            e_flyingmenu_autodelete_set(Evas_Object *obj, Evas_Bool autodelete);
Evas_Bool       e_flyingmenu_autodelete_get(Evas_Object *obj);

void            e_flyingmenu_item_size_min_set(Evas_Object *obj, Evas_Coord size);
Evas_Coord      e_flyingmenu_item_size_min_get(Evas_Object *obj);
void            e_flyingmenu_item_add(Evas_Object *obj, const char *label, void (*func) (void *data, Evas_Object *obj), void *data);

void            e_flyingmenu_activate(Evas_Object *obj);

#endif /* E_FLYINGMENU_H */
