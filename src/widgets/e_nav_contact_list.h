/* e_nav_contact_list.h -
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

#ifndef E_NAV_CONTACT_LIST_H
#define E_NAV_CONTACT_LIST_H
#include <Etk.h>

typedef struct _Contact_List Contact_List;
struct _Contact_List
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *frame;
   Etk_Widget      *embed;
   Etk_Widget      *tree;
   Etk_Tree_Col    *col;
};

typedef struct _Contact_List_Item Contact_List_Item;
struct _Contact_List_Item 
{
   char *name;
   char *description;
   void (*func) (void *data, void *data2);
   void *data, *data2;
};

Contact_List       *e_nav_contact_list_new(Evas_Object *obj, const char *custom_dir);
void                e_nav_contact_list_destroy(Contact_List *obj);
void                e_nav_contact_list_item_add(Contact_List *obj, Contact_List_Item *item);
void                e_nav_contact_list_clear(Contact_List *obj);
void                e_nav_contact_list_activate(Contact_List *tl);
void                e_nav_contact_list_deactivate(Contact_List *tl);
void                e_nav_contact_list_coord_set(Contact_List *cl, int x, int y, int w, int h);

#endif
