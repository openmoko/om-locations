/* e_nav_contact_editor.h -
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

#ifndef E_NAV_CONTACT_EDITOR_H
#define E_NAV_CONTACT_EDITOR_H

#include <Evas.h>
#include <unistd.h>

Evas_Object    *e_contact_editor_add(Evas *e);
void            e_contact_editor_callbacks_set(Evas_Object *obj, void (*positive_func)(void *data, Evas_Object *obj), void *data1, void (*negative_func)(void *data, Evas_Object *obj), void *data2);

void            e_contact_editor_input_length_limit_set(Evas_Object *obj, size_t length_limit);
size_t          e_contact_editor_input_length_limit_get(Evas_Object *obj);
void            e_contact_editor_input_set(Evas_Object *obj, const char *name, const char *input);
const char     *e_contact_editor_input_get(Evas_Object *obj);

void            e_contact_editor_contacts_set(Evas_Object *obj, Evas_List *list);

#endif /* E_NAV_CONTACT_EDITOR_H */
