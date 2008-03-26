/* e_nav_taglist.c -
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

#include "e_nav.h"
#include "e_nav_taglist.h"
#include "widgets/e_nav_theme.h"
#include <stdlib.h>
#define E_NEW(s, n) (s *)calloc(n, sizeof(s))
static void _e_taglist_update(Tag_List *sd);

static Ewl_Widget *
test_data_header_fetch(void *data , unsigned int column)
{
   Ewl_Widget *l;
   l = ewl_label_new();
   ewl_object_custom_h_set(EWL_OBJECT(l), 60);
   ewl_label_text_set(EWL_LABEL(l), "View Tags");
   ewl_widget_show(l);
   return l;
}

static Ewl_Widget *
tree_test_cb_widget_fetch(void *data, unsigned int row, unsigned int column)
{
   Tag_List_Item *d;
   Ewl_Widget *w = NULL;
   switch (column) {
      case 0:
         d = data;
         w = ewl_label_new();
         ewl_object_custom_h_set(EWL_OBJECT(w), 60);
         ewl_label_text_set(EWL_LABEL(w), d->name);
         break;
   }
   ewl_widget_show(w);
   return w;
}

static void
tree_cb_value_changed(Ewl_Widget *w, void *ev, void *data )
{
   Ecore_List *selected;
   Ewl_Selection *sel;
   selected = ewl_mvc_selected_list_get(EWL_MVC(w));
   ecore_list_first_goto(selected);

   while ((sel = ecore_list_next(selected)))
     {
        if (sel->type == EWL_SELECTION_TYPE_INDEX)
          {
             unsigned int col;
             Ewl_Selection_Idx *idx;
             idx = EWL_SELECTION_IDX(sel);
             col = idx->column;
             Tag_List_Item *ti;
             ti = sel->model->fetch(sel->data, idx->row, col);
             if (ti->func) ti->func(ti->data, ti->data2);
          }
     }
}

Tag_List *
e_nav_taglist_new(Evas_Object *obj, const char *custom_dir)
{
   Ecore_Evas *ee;
   Evas *evas;
   Ewl_Model *model;
   Ewl_View  *view;
   Ecore_List *data;
   Evas_Coord x, y, w, h;
  
   Tag_List * tl = (Tag_List *)malloc (sizeof(Tag_List));
   memset(tl, 0, sizeof(Tag_List)); 

   tl->frame = e_nav_theme_object_new(evas_object_evas_get(obj), custom_dir, "modules/diversity_nav/taglist"); 
   evas_object_smart_member_add(tl->frame, obj);

   /*
    * Setup the ewl embed
    */
   tl->embed = ewl_embed_new();
   ewl_object_fill_policy_set(EWL_OBJECT(tl->embed), EWL_FLAG_FILL_ALL);
   evas = evas_object_evas_get(obj);
   ee = ecore_evas_ecore_evas_get(evas);
   tl->embed_eo = ewl_embed_canvas_set(EWL_EMBED(tl->embed), evas,
                     (void *) ecore_evas_software_x11_window_get(ee));   
   ewl_embed_focus_set(EWL_EMBED(tl->embed), TRUE);

   /*
    * swallow it into the edje
    */
   evas_output_viewport_get(evas_object_evas_get(obj), &x, &y, &w, &h);
   evas_object_move(tl->embed_eo, x, y);
   evas_object_resize(tl->embed_eo, w, h);
   edje_object_part_swallow(tl->frame, "swallow", tl->embed_eo);

   /*
    * fill it with content
    */
   tl->scrollpane = ewl_scrollpane_new();
   ewl_container_child_append(EWL_CONTAINER(tl->embed), tl->scrollpane);

   data = ecore_list_new();

   model = ewl_model_ecore_list_get();
   view = ewl_label_view_get();
   ewl_view_header_fetch_set(view, test_data_header_fetch);
   ewl_view_widget_fetch_set(view, tree_test_cb_widget_fetch);

   tl->tree = ewl_tree_new();
   ewl_tree_headers_visible_set(EWL_TREE(tl->tree), TRUE);
   ewl_tree_fixed_rows_set(EWL_TREE(tl->tree), FALSE);

   ewl_tree_kinetic_scrolling_set(EWL_TREE(tl->tree), EWL_KINETIC_SCROLL_EMBEDDED);
   ewl_tree_kinetic_fps_set(EWL_TREE(tl->tree), 30);
   ewl_tree_kinetic_dampen_set(EWL_TREE(tl->tree), 0.99);
   ewl_tree_column_count_set(EWL_TREE(tl->tree), 1);
   ewl_mvc_model_set(EWL_MVC(tl->tree), model);
   ewl_mvc_view_set(EWL_MVC(tl->tree), view);
   ewl_mvc_data_set(EWL_MVC(tl->tree), data);
   ewl_container_child_append(EWL_CONTAINER(tl->scrollpane), tl->tree);
   ewl_callback_append(tl->tree, EWL_CALLBACK_VALUE_CHANGED,
                       tree_cb_value_changed, NULL);
   ewl_object_fill_policy_set(EWL_OBJECT(tl->tree), EWL_FLAG_FILL_ALL);
   return tl;
}

void
e_nav_taglist_tag_add(Tag_List *obj, const char *name, const char *description, void (*func) (void *data, void *data2), void *data1, void *data2)
{
   Tag_List_Item *item;
   Ecore_List *list;    
   item = E_NEW(Tag_List_Item, 1);
   item->name = strdup(name); 
   item->description = strdup(description);
   item->func = func;
   item->data = data1;
   item->data2 = data2;

   list = ewl_mvc_data_get(EWL_MVC(obj->tree));
   ecore_list_append(list, item);
   ewl_mvc_data_set(EWL_MVC(obj->tree), list);
}

void
e_nav_taglist_tag_update(Tag_List *obj, const char *name, const char *description, void *object)
{
   Evas_Object *location_obj = NULL;
   int n;
   Ecore_List *list = ewl_mvc_data_get(EWL_MVC(obj->tree));   
   int count = ecore_list_count(list);
   Tag_List_Item *item;

   for(n=0; n<count; n++)
     {
        item = ecore_list_index_goto(list, n);
        location_obj = item->data2;
        if(location_obj == object)
          {
             if(name)
               {
                  free(item->name);
                  item->name = strdup(name);
               }
             if(description)
               {
                  free(item->description);
                  item->description = strdup(description);
               }
          }
        location_obj = NULL;
     }
   ewl_mvc_data_set(EWL_MVC(obj->tree), list);
}

void
e_nav_taglist_tag_remove(Tag_List *obj, Evas_Object *tag)
{
   Evas_Object *location_obj = NULL;
   int n;
   Ecore_List *list = ewl_mvc_data_get(EWL_MVC(obj->tree));   
   int count = ecore_list_count(list);
   Tag_List_Item *item;
   for(n=0; n<count; n++)
     {
        item = ecore_list_index_goto(list, n);
        location_obj = item->data2;
        if(location_obj == tag)
          {
             ecore_list_remove_destroy(list); 
             break;
          }
        location_obj = NULL;
     }

   ewl_mvc_data_set(EWL_MVC(obj->tree), list);
}

void 
e_nav_taglist_clear(Tag_List *obj)
{
   Ecore_List *list = ewl_mvc_data_get(EWL_MVC(obj->tree));   
   int n;
   int count = ecore_list_count(list);
   ecore_list_first_goto(list);
   for(n=0; n<count; n++)
     {
        ecore_list_remove_destroy(list);
     }
   ewl_mvc_data_set(EWL_MVC(obj->tree), list);
}

void
e_nav_taglist_activate(Tag_List *tl)
{
   _e_taglist_update(tl);
}

void
e_nav_taglist_deactivate(Tag_List *tl)
{
   evas_object_hide(tl->frame);
   evas_object_hide(tl->embed_eo);
   ewl_widget_hide(tl->embed);
   ewl_widget_hide(tl->scrollpane);
   ewl_widget_hide(tl->tree);
}

static void
_e_taglist_update(Tag_List *tl)
{
   Evas_Coord screen_x, screen_y, screen_w, screen_h;
   if(tl==NULL) return;
   evas_output_viewport_get(evas_object_evas_get(tl->frame), &screen_x, &screen_y, &screen_w, &screen_h);
   evas_object_show(tl->frame);
   evas_object_move(tl->embed_eo, screen_x, screen_y);
   evas_object_resize(tl->embed_eo, screen_w, screen_h);
   evas_object_show(tl->embed_eo);
   ewl_widget_show(tl->embed);
   ewl_widget_show(tl->scrollpane);
   ewl_widget_show(tl->tree);
}

