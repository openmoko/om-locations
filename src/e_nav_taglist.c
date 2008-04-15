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
#include "e_nav_theme.h"
#include <stdlib.h>
#include <time.h>

static void _e_taglist_update(Tag_List *sd);

static Ewl_Widget *
test_data_header_fetch(void *data , unsigned int column)
{
   Ewl_Widget *l;

   l = ewl_label_new();
   ewl_object_custom_h_set(EWL_OBJECT(l), 30);
   ewl_label_text_set(EWL_LABEL(l), "View Tags");
   ewl_widget_show(l);
   return l;
}

static char *
get_time_diff_string(time_t time_then)
{
   char time_diff_string[PATH_MAX];
   time_t time_now, time_diff;
   int days_diff;
   int today_secs;
   struct tm now, then;
   struct tm *now_p, *then_p;

   time(&time_now);  
   now_p = localtime(&time_now);
   memcpy(&now, now_p, sizeof(now));

   then_p = localtime(&time_then);
   memcpy(&then, then_p, sizeof(then));

   if(time_then > time_now) 
     {
        snprintf(time_diff_string, sizeof(time_diff_string),
                 "%s", ctime(&time_then));
     }

   if(now.tm_year != then.tm_year)
     {
        if(now.tm_year - then.tm_year == 1)
          snprintf(time_diff_string, sizeof(time_diff_string),
                   "Last year");
        else
          snprintf(time_diff_string, sizeof(time_diff_string),
                   "%d years ago", now.tm_year - then.tm_year);
        return strdup(time_diff_string);
     }
   else if(now.tm_mon != then.tm_mon)
     {
        if(now.tm_mon - then.tm_mon == 1)
          snprintf(time_diff_string, sizeof(time_diff_string),
                   "Last month");
        else
          snprintf(time_diff_string, sizeof(time_diff_string),
                   "%d months ago", now.tm_mon - then.tm_mon);
        return strdup(time_diff_string);
     }
   else 
     {
            today_secs = (now.tm_hour * 60 * 60) + (now.tm_min * 60) + now.tm_sec; 
            printf("today_secs = %d\n", today_secs);
            printf("time_now: %d, time_past: %d\n", (int)time_now, (int)time_then);
            time_diff = time_now - time_then;
            printf("time_diff = %d\n", (int)time_diff);
            if(time_diff >= today_secs) 
              {
                 days_diff = (time_diff - today_secs) / 86400;
                 if(days_diff == 0)
                   {
                      snprintf(time_diff_string, sizeof(time_diff_string),
                               "Yesterday");
                   }
                 else if((days_diff + 1) < 7)
                   {
                      snprintf(time_diff_string, sizeof(time_diff_string),
                               "%d days ago", days_diff + 1 );
                   }
                 else 
                   {
                      if(( (days_diff + 1) / 7 ) == 1)
                        snprintf(time_diff_string, sizeof(time_diff_string),
                                 "Last week");
                      else
                        snprintf(time_diff_string, sizeof(time_diff_string),
                                 "%d weeks ago", (days_diff + 1) / 7);
                   }

                 return strdup(time_diff_string);
              }
            else 
              {
                 return strdup("Today");
              } 
     }
}

static Ewl_Widget *
tree_test_cb_widget_fetch(void *data, unsigned int row, unsigned int column)
{
   Tag_List_Item *d;
   Ewl_Widget *vbox = NULL;
   Ewl_Widget *label1 = NULL;
   Ewl_Widget *label2 = NULL;
   char tmp[PATH_MAX];
   char *time_diff_string;

   snprintf(tmp, PATH_MAX, "%s/%s.edj", PACKAGE_DATA_DIR, "splinter");
   switch (column) {
      case 0:
         d = data;
         label1 = ewl_label_new();

         ewl_theme_data_reset(label1);
         ewl_theme_data_str_set(label1, "/label/file", tmp);
         ewl_theme_data_str_set(label1, "/label/group", "diversity/label");

         ewl_object_custom_h_set(EWL_OBJECT(label1), 60);
         ewl_label_text_set(EWL_LABEL(label1), d->name);

         label2 = ewl_label_new();
         ewl_object_custom_h_set(EWL_OBJECT(label2), 20);

         time_diff_string = get_time_diff_string(d->timestamp);
         ewl_label_text_set(EWL_LABEL(label2), time_diff_string);
         free(time_diff_string);
         time_diff_string = NULL;
         break;
   }

   vbox = ewl_box_new();
   ewl_box_orientation_set(EWL_BOX(vbox), EWL_ORIENTATION_VERTICAL);
   ewl_container_child_append(EWL_CONTAINER(vbox), label1);
   ewl_container_child_append(EWL_CONTAINER(vbox), label2);

   ewl_widget_show(vbox);
   ewl_widget_show(label1);
   ewl_widget_show(label2);

   return vbox;
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
e_nav_taglist_destroy(Tag_List *obj)
{
   ewl_widget_destroy(obj->tree);
   ewl_widget_destroy(obj->scrollpane);
   ewl_widget_destroy(obj->embed);
   evas_object_del(obj->embed_eo);
   evas_object_del(obj->frame);
}

void
e_nav_taglist_tag_add(Tag_List *obj, Tag_List_Item *item)
{
   Ecore_List *list;    

   list = ewl_mvc_data_get(EWL_MVC(obj->tree));
   ecore_list_prepend(list, item);
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
   ewl_mvc_selected_clear(EWL_MVC(tl->tree));
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

static int 
_list_sort_compare_cb(Tag_List_Item *item1, Tag_List_Item *item2) 
{
   if (item1->timestamp == 0) return 1;
   if (item2->timestamp == 0) return -1;
   return item1->timestamp <= item2->timestamp ? 1: -1;
}

static void
_e_taglist_update(Tag_List *tl)
{
   Evas_Coord screen_x, screen_y, screen_w, screen_h;
   Ecore_List *list;   

   if(tl==NULL) return;
   list = ewl_mvc_data_get(EWL_MVC(tl->tree));
   ecore_list_sort(list, ECORE_COMPARE_CB(_list_sort_compare_cb), ECORE_SORT_MIN);

   evas_output_viewport_get(evas_object_evas_get(tl->frame), &screen_x, &screen_y, &screen_w, &screen_h);
   evas_object_show(tl->frame);
   evas_object_move(tl->embed_eo, screen_x, screen_y);
   evas_object_resize(tl->embed_eo, screen_w, screen_h);
   evas_object_show(tl->embed_eo);
   ewl_widget_show(tl->embed);
   ewl_widget_show(tl->scrollpane);
   ewl_widget_show(tl->tree);
}

