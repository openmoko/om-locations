/* e_nav_dialog.h -
 *
 * Copyright 2008 Openmoko, Inc.
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

#ifndef E_NAV_DIALOG_H
#define E_NAV_DIALOG_H

enum {
     E_NAV_DIALOG_TYPE_NORMAL,
     E_NAV_DIALOG_TYPE_ALERT,
};

Evas_Object    *e_nav_dialog_add(Evas *e);
void            e_nav_dialog_type_set(Evas_Object *obj, int type);

void            e_nav_dialog_transient_for_set(Evas_Object *obj, Evas_Object *parent);
Evas_Object    *e_nav_dialog_transient_for_get(Evas_Object *obj);

void            e_nav_dialog_title_set(Evas_Object *obj, const char *title, const char *message);
void            e_nav_dialog_title_color_set(Evas_Object *obj, int r, int g, int b, int a);

void            e_nav_dialog_button_add(Evas_Object *obj, const char *label, void (*func) (void *data, Evas_Object *obj), void *data);
void            e_nav_dialog_textblock_add(Evas_Object *obj, const char *label, const char*input, Evas_Coord size, size_t length_limit, void *data);
const char     *e_nav_dialog_textblock_text_get(Evas_Object *obj, const char *label);

void            e_nav_dialog_activate(Evas_Object *obj);
void            e_nav_dialog_deactivate(Evas_Object *obj);

#endif /* E_NAV_DIALOG_H */
