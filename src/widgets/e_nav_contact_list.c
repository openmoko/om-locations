/* e_nav_contact_list.c -
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

#include "../e_nav.h"
#include "../e_nav_theme.h"
#include "e_nav_contact_list.h"

static void _e_contact_list_update(Contact_List *sd);

static Etk_Bool
_etk_test_tree_row_clicked_cb(Etk_Object *object, Etk_Tree_Row *row, Etk_Event_Mouse_Up *event, void *data)
{
   Etk_Tree *tree;
   char *row_name;
   Contact_List_Item *ti;
 
   if (!(tree = ETK_TREE(object)) || !row || !event)
      return ETK_TRUE;

   etk_tree_row_fields_get(row, etk_tree_nth_col_get(tree, 0), NULL, NULL, &row_name, NULL);

   ti = (Contact_List_Item *)etk_tree_row_data_get(row); 

   if (ti->func) ti->func(ti->data, ti->data2);

   return ETK_TRUE;
}

/* ascending sort */
static int
_contact_list_sort_compare_cb(Etk_Tree_Col *col, Etk_Tree_Row *row1, Etk_Tree_Row *row2, void *data)
{
   Contact_List_Item *item1, *item2;

   item1 = (Contact_List_Item *)etk_tree_row_data_get(row1); 
   item2 = (Contact_List_Item *)etk_tree_row_data_get(row2); 

   return strcmp(item1->name, item2->name);
}

static void
_e_nav_back_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Contact_List *cl;
   if(!data) return;
   cl = (Contact_List *)data;
   e_nav_contact_list_deactivate(cl);
}

Contact_List *
e_nav_contact_list_new(Evas_Object *obj, const char *custom_dir)
{
   Contact_List * cl = (Contact_List *)malloc (sizeof(Contact_List));
   memset(cl, 0, sizeof(Contact_List)); 

   cl->frame = e_nav_theme_object_new(evas_object_evas_get(obj), custom_dir, "modules/diversity_nav/contact_list"); 
   evas_object_smart_member_add(cl->frame, obj);

   /*
    * Setup the etk embed
    */
   cl->embed  = etk_embed_new(evas_object_evas_get(obj));
   cl->tree  = etk_tree_new();
   etk_scrolled_view_policy_set(etk_tree_scrolled_view_get(ETK_TREE(cl->tree)), ETK_POLICY_HIDE, ETK_POLICY_HIDE);
   etk_scrolled_view_dragable_set(ETK_SCROLLED_VIEW(etk_tree_scrolled_view_get(ETK_TREE(cl->tree))),ETK_TRUE);

   etk_tree_mode_set(ETK_TREE(cl->tree), ETK_TREE_MODE_LIST);
   etk_tree_multiple_select_set(ETK_TREE(cl->tree), ETK_FALSE);
   etk_tree_rows_height_set (ETK_TREE(cl->tree), 60);
   etk_tree_thaw(ETK_TREE(cl->tree));

   cl->col = etk_tree_col_new(ETK_TREE(cl->tree), NULL, 460, 0.0);

   etk_tree_col_model_add(cl->col, etk_tree_model_text_new());
   etk_tree_col_sort_set(cl->col, _contact_list_sort_compare_cb, NULL);

   etk_tree_headers_visible_set(ETK_TREE(cl->tree), 0);
   etk_tree_col_title_set(cl->col, "View Contacts");

   etk_signal_connect_by_code(ETK_TREE_ROW_CLICKED_SIGNAL, ETK_OBJECT(cl->tree),
      ETK_CALLBACK(_etk_test_tree_row_clicked_cb), NULL);

   etk_tree_build(ETK_TREE(cl->tree));

   etk_container_add(ETK_CONTAINER(cl->embed), cl->tree);
   etk_widget_show_all(ETK_WIDGET(cl->embed));
   etk_widget_show_all(ETK_WIDGET(cl->tree));

   /*
    * swallow it into the edje
    */
   edje_object_part_swallow(cl->frame, "swallow", etk_embed_object_get(ETK_EMBED(cl->embed)));

   Evas_Object *back_button; 
   back_button = edje_object_part_object_get(cl->frame, "button"); 

   evas_object_event_callback_add(back_button, EVAS_CALLBACK_MOUSE_UP,
				  _e_nav_back_button_cb_mouse_down, cl);
   return cl;
}

void
e_nav_contact_list_destroy(Contact_List *obj)
{
   if(!obj) return;
   evas_object_del(obj->frame);
   obj->frame = NULL;
   etk_object_destroy(ETK_OBJECT(obj->embed));
   etk_object_destroy(ETK_OBJECT(obj->tree));
   etk_object_destroy(ETK_OBJECT(obj->col));
   free(obj);
}

void
e_nav_contact_list_item_add(Contact_List *obj, Contact_List_Item *item)
{
   Etk_Tree_Row *tree_row;
   char *buf = (char *)malloc(strlen(item->name) + 128);
   
   sprintf(buf, "<b><font_size=48>%s</></b>", item->name);

   tree_row = etk_tree_row_prepend(ETK_TREE(obj->tree), NULL, obj->col, buf, NULL );
   free(buf);
   buf = NULL;
   if(tree_row)
     etk_tree_row_data_set(tree_row, item);
}

void 
e_nav_contact_list_clear(Contact_List *obj)
{
   etk_tree_clear(ETK_TREE(obj->tree));
   etk_tree_thaw(ETK_TREE(obj->tree));
}

void
e_nav_contact_list_activate(Contact_List *cl)
{
   etk_tree_unselect_all(ETK_TREE(cl->tree));
   _e_contact_list_update(cl);
}

void
e_nav_contact_list_deactivate(Contact_List *cl)
{
   evas_object_hide(cl->frame);
   etk_widget_hide(cl->embed);
   etk_widget_hide(cl->tree);
}

void
e_nav_contact_list_coord_set(Contact_List *cl, int x, int y, int w, int h)
{
   if(!cl) return;
   cl->x = x;
   cl->y = y;
   cl->w = w;
   cl->h = h;
}

static void
_e_contact_list_update(Contact_List *cl)
{
   Etk_Tree_Row *row;
   Contact_List_Item *item;
   char *buf;

   if(cl==NULL) return;

   evas_object_move(cl->frame, cl->x, cl->y);
   evas_object_resize(cl->frame, cl->w, cl->h);

   etk_tree_col_sort(cl->col, TRUE);  

   row = etk_tree_first_row_get(ETK_TREE(cl->tree));

   for (; row; row = etk_tree_row_next_get(row))
     {
        item = (Contact_List_Item *)etk_tree_row_data_get(row);
        buf = (char *)malloc(strlen(item->name) + 128);
     
        sprintf(buf, "<b><font_size=48>%s</></b>", item->name);
        
        etk_tree_row_fields_set(row, FALSE, cl->col, buf, NULL);
        free(buf);
        buf = NULL;
     }

   evas_object_show(cl->frame);
   evas_object_raise(cl->frame);
   etk_widget_show_all(cl->embed);
   etk_widget_show(cl->tree);
}

