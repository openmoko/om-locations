/* e_nav_taglist.h -
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

#ifndef E_NAV_TAGLIST_H
#define E_NAV_TAGLIST_H

Evas_Object    *e_nav_taglist_add(Evas *e);
void            e_nav_taglist_theme_source_set(Evas_Object *obj, const char *custom_dir);
void            e_nav_taglist_tag_add(Evas_Object *obj, const char *name, const char *description,  void (*func) (void *data, void *data2), void *data1, void *data2);
void            e_nav_taglist_tag_remove(Evas_Object *obj, Evas_Object *tag);
void            e_nav_taglist_tag_update(Evas_Object *obj, const char *name, const char *description, void *object);
void            e_nav_taglist_activate(Evas_Object *obj);
void            e_nav_taglist_deactivate(Evas_Object *obj);

#endif
