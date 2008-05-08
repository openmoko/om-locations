/* e_nav_textedit.h -
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

#ifndef E_NAV_TEXTEDIT_H
#define E_NAV_TEXTEDIT_H

typedef enum {
     TEXTEDIT_CANDIDATE_MODE_NONE,
     TEXTEDIT_CANDIDATE_MODE_FALSE,
     TEXTEDIT_CANDIDATE_MODE_TRUE,
}TEXTEDIT_CANDIDATE_MODE;

typedef struct _Textedit_List_Item Textedit_List_Item;

struct _Textedit_List_Item
{
   const char             *name;
   void                   *data;
};

/* object management */
Evas_Object    *e_textedit_add(Evas *e);
void            e_textedit_theme_source_set(Evas_Object *obj, const char *custom_dir, void (*positive_func)(void *data, Evas_Object *obj, Evas_Object *src_obj), void *data1, void (*negative_func)(void *data, Evas_Object *obj, Evas_Object *src_obj), void *data2);
void            e_textedit_source_object_set(Evas_Object *obj, void *src_obj);
Evas_Object    *e_textedit_source_object_get(Evas_Object *obj);
void            e_textedit_activate(Evas_Object *obj);
void            e_textedit_deactivate(Evas_Object *obj);
void            e_textedit_input_length_limit_set(Evas_Object *obj, size_t length_limit);
size_t          e_textedit_input_length_limit_get(Evas_Object *obj);
void            e_textedit_input_set(Evas_Object *obj, const char *name, const char *input);
const char     *e_textedit_input_get(Evas_Object *obj);
void            e_textedit_candidate_list_set(Evas_Object *obj, Ecore_List *list);
Ecore_List     *e_textedit_candidate_list_get(Evas_Object *obj);
void            e_textedit_candidate_mode_set(Evas_Object *obj, unsigned int candidate_mode);
unsigned int    e_textedit_candidate_mode_get(Evas_Object *obj);
void           *e_textedit_list_selected_get(Evas_Object *obj);
void           *e_textedit_list_item_get_by_name(Evas_Object *obj, const char *name);
    
#endif

