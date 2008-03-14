/* e_spreadmenu.h -
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

#ifndef E_SPREADMENU_H
#define E_SPREADMENU_H

/* types */

/* object management */
Evas_Object    *e_spreadmenu_add(Evas *e);
void            e_spreadmenu_theme_source_set(Evas_Object *obj, const char *custom_dir);
void            e_spreadmenu_source_object_set(Evas_Object *obj, Evas_Object *src_obj);
Evas_Object    *e_spreadmenu_source_object_get(Evas_Object *obj);
void            e_spreadmenu_autodelete_set(Evas_Object *obj, Evas_Bool autodelete);
Evas_Bool       e_spreadmenu_autodelete_get(Evas_Object *obj);
void            e_spreadmenu_activate(Evas_Object *obj);
void            e_spreadmenu_deactivate(Evas_Object *obj);
void            e_spreadmenu_theme_item_add(Evas_Object *obj, const char *icon, Evas_Coord size, const char *label, void (*func) (void *data, Evas_Object *obj, Evas_Object *src_obj), void *data);
    
#endif
