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
#include <time.h>

typedef struct _Tag_List Tag_List;

Tag_List       *e_nav_taglist_new(Evas_Object *obj, const char *custom_dir);
void            e_nav_taglist_destroy(Tag_List *tl);

void            e_nav_taglist_callback_add(Tag_List *tl, void (*func)(void *data, Tag_List *tl, Evas_Object *tag), void *data);
void            e_nav_taglist_callback_del(Tag_List *tl, void *func, void *data);

void            e_nav_taglist_activate(Tag_List *tl);
void            e_nav_taglist_deactivate(Tag_List *tl);

void            e_nav_taglist_tag_add(Tag_List *tl, Evas_Object *tag);
void            e_nav_taglist_tag_remove(Tag_List *tl, Evas_Object *tag);
void            e_nav_taglist_tag_update(Tag_List *tl, Evas_Object *tag);
void            e_nav_taglist_clear(Tag_List *tl);

#endif /* E_NAV_TAGLIST_H */
