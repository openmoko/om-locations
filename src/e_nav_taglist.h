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

#include <Evas.h>

Evas_Object    *e_nav_taglist_add(Evas *e, const char *custom_dir);

void            e_nav_taglist_callback_add(Evas_Object *tl, void (*func)(void *data, Evas_Object *tl, Evas_Object *tag), void *data);
void            e_nav_taglist_callback_del(Evas_Object *tl, void *func, void *data);

void            e_nav_taglist_tag_add(Evas_Object *tl, Evas_Object *tag);
void            e_nav_taglist_tag_remove(Evas_Object *tl, Evas_Object *tag);
void            e_nav_taglist_tag_update(Evas_Object *tl, Evas_Object *tag);
void            e_nav_taglist_clear(Evas_Object *tl);

#endif /* E_NAV_TAGLIST_H */
