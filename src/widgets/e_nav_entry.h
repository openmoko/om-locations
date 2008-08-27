/* e_nav_entry.h -
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

#ifndef E_NAV_ENTRY_H
#define E_NAV_ENTRY_H

Evas_Object    *e_nav_entry_add(Evas *e);
void            e_nav_entry_theme_source_set(Evas_Object *entry, const char *custom_dir);

void            e_nav_entry_button_add(Evas_Object *entry, const char *label, void (*func)(void *data, Evas_Object *entry), void *data);
void            e_nav_entry_button_remove(Evas_Object *entry, void (*func)(void *data, Evas_Object *entry), void *data);

void            e_nav_entry_title_set(Evas_Object *entry, const char *title);
const char     *e_nav_entry_title_get(Evas_Object *entry);
void            e_nav_entry_text_set(Evas_Object *entry, const char *text);
const char     *e_nav_entry_text_get(Evas_Object *entry);
void            e_nav_entry_text_limit_set(Evas_Object *entry, size_t limit);
size_t          e_nav_entry_text_limit_get(Evas_Object *entry);

void            e_nav_entry_focus(Evas_Object *entry);
void            e_nav_entry_unfocus(Evas_Object *entry);

#endif /* E_NAV_ENTRY_H */
