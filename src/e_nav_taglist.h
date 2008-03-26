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
#include <ewl/Ewl.h>

typedef struct _Tag_List Tag_List;
struct _Tag_List
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *frame;
   Evas_Object     *embed_eo;
   Ewl_Widget      *scrollpane;
   Ewl_Widget      *embed;
   Ewl_Widget      *tree;
};

typedef struct _Tag_List_Item Tag_List_Item;
struct _Tag_List_Item 
{
   char *name;
   char *description;
   void (*func) (void *data, void *data2);
   void *data, *data2;
};

Tag_List        *e_nav_taglist_new(Evas_Object *obj, const char *custom_dir);
void            e_nav_taglist_tag_add(Tag_List *obj, const char *name, const char *description,  void (*func) (void *data, void *data2), void *data1, void *data2);
void            e_nav_taglist_tag_remove(Tag_List *obj, Evas_Object *tag);
void            e_nav_taglist_tag_update(Tag_List *obj, const char *name, const char *description, void *object);
void            e_nav_taglist_clear(Tag_List *obj);
void            e_nav_taglist_activate(Tag_List *tl);
void            e_nav_taglist_deactivate(Tag_List *tl);

#endif
