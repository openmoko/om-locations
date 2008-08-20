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

#include "e_nav_taglist.h"
#include "e_nav_item_location.h"
#include "e_nav_theme.h"
#include "e_nav.h"
#include <stdlib.h>
#include <time.h>

static void _e_taglist_update(Tag_List *sd);

static char *
get_time_diff_string(time_t time_then)
{
   char time_diff_string[PATH_MAX];
   time_t time_now, time_diff;
   int today_secs;
   struct tm now, then;
   struct tm *now_p, *then_p;
   int age;

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
	age = now.tm_year - then.tm_year;

	if (age == 1)
	  snprintf(time_diff_string, sizeof(time_diff_string),
		_("Last year"));
        else
	  snprintf(time_diff_string, sizeof(time_diff_string),
		ngettext("One year ago", "%d years ago", age), age);

        return strdup(time_diff_string);
     }
   else if(now.tm_mon != then.tm_mon)
     {
	age = now.tm_mon - then.tm_mon;

        if (age == 1)
	  snprintf(time_diff_string, sizeof(time_diff_string),
		_("Last month"));
        else
	  snprintf(time_diff_string, sizeof(time_diff_string),
		ngettext("One month ago", "%d months ago", age), age);

        return strdup(time_diff_string);
     }
   else 
     {
            today_secs = (now.tm_hour * 60 * 60) + (now.tm_min * 60) + now.tm_sec; 
            time_diff = time_now - time_then;

            if (time_diff >= today_secs) 
	      {
		 age = 1 + (time_diff - today_secs) / 86400;
		 if (age == 1)
		   {
		      snprintf(time_diff_string, sizeof(time_diff_string),
			    _("Yesterday"));
		   }
		 else if (age < 7)
		   {
		      snprintf(time_diff_string, sizeof(time_diff_string),
			    ngettext("One day ago", "%d days ago", age), age);
		   }
		 else 
		   {
		      age /= 7;

		      if (age == 1)
			snprintf(time_diff_string, sizeof(time_diff_string),
			      _("Last week"));
		      else
			snprintf(time_diff_string, sizeof(time_diff_string),
			      ngettext("One week ago", "%d weeks ago", age), age);
		   }

		 return strdup(time_diff_string);
	      }
            else 
              {
                 return strdup(_("Today"));
              } 
     }
}

static Etk_Bool
_etk_test_tree_row_clicked_cb(Etk_Object *object, Etk_Tree_Row *row, Etk_Event_Mouse_Up *event, void *data)
{
   Etk_Tree *tree;
   char *row_name;
   Tag_List_Item *ti;
 
   if (!(tree = ETK_TREE(object)) || !row || !event)
      return ETK_TRUE;

   etk_tree_row_fields_get(row, etk_tree_nth_col_get(tree, 0), NULL, NULL, &row_name, NULL);

   ti = (Tag_List_Item *)etk_tree_row_data_get(row); 

   if (ti->func) ti->func(ti->data, ti->data2);

   return ETK_TRUE;
}

static int
_taglist_sort_compare_cb(Etk_Tree_Col *col, Etk_Tree_Row *row1, Etk_Tree_Row *row2, void *data)
{
   Tag_List_Item *item1, *item2;

   item1 = (Tag_List_Item *)etk_tree_row_data_get(row1); 
   item2 = (Tag_List_Item *)etk_tree_row_data_get(row2); 

   if (item1->timestamp > item2->timestamp)
     return -1;
   else if (item1->timestamp < item2->timestamp)
     return 1;
   else
     return 0;
}

Tag_List *
e_nav_taglist_new(Evas_Object *obj, const char *custom_dir)
{
   Tag_List * tl = (Tag_List *)malloc (sizeof(Tag_List));
   memset(tl, 0, sizeof(Tag_List)); 

   tl->frame = e_nav_theme_object_new(evas_object_evas_get(obj), custom_dir, "modules/diversity_nav/taglist"); 
   edje_object_part_text_set(tl->frame, "title", _("View Tags"));

   evas_object_smart_member_add(tl->frame, obj);

   /*
    * Setup the etk embed
    */
   tl->embed  = etk_embed_new(evas_object_evas_get(obj));
   tl->tree  = etk_tree_new();
   etk_scrolled_view_policy_set(etk_tree_scrolled_view_get(ETK_TREE(tl->tree)), ETK_POLICY_HIDE, ETK_POLICY_HIDE);
   etk_scrolled_view_dragable_set(ETK_SCROLLED_VIEW(etk_tree_scrolled_view_get(ETK_TREE(tl->tree))),ETK_TRUE);

   etk_tree_mode_set(ETK_TREE(tl->tree), ETK_TREE_MODE_LIST);
   etk_tree_multiple_select_set(ETK_TREE(tl->tree), ETK_FALSE);
   etk_tree_rows_height_set (ETK_TREE(tl->tree), 90);
   etk_tree_thaw(ETK_TREE(tl->tree));

   tl->col = etk_tree_col_new(ETK_TREE(tl->tree), NULL, 455, 0.0);

   etk_tree_col_model_add(tl->col, etk_tree_model_text_new());
   etk_tree_col_sort_set(tl->col, _taglist_sort_compare_cb, NULL);

   etk_tree_headers_visible_set(ETK_TREE(tl->tree), 0);

   etk_signal_connect_by_code(ETK_TREE_ROW_CLICKED_SIGNAL, ETK_OBJECT(tl->tree),
      ETK_CALLBACK(_etk_test_tree_row_clicked_cb), NULL);

   etk_tree_build(ETK_TREE(tl->tree));

   etk_container_add(ETK_CONTAINER(tl->embed), tl->tree);
   etk_widget_show_all(ETK_WIDGET(tl->embed));
   etk_widget_show_all(ETK_WIDGET(tl->tree));

   /*
    * swallow it into the edje
    */
   edje_object_part_swallow(tl->frame, "swallow", etk_embed_object_get(ETK_EMBED(tl->embed)));

   return tl;
}

void
e_nav_taglist_destroy(Tag_List *obj)
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
e_nav_taglist_tag_add(Tag_List *obj, Tag_List_Item *item)
{
   Etk_Tree_Row *tree_row;
   char *time_diff_string;
   char *buf;

   if(!item) return;
   if(item->name != NULL && !strcmp(item->name, "") )
     {
        free(item->name);
        item->name = strdup(_("No Title"));
     }

   buf = (char *)malloc(strlen(item->name) + 128);

   time_diff_string = get_time_diff_string(item->timestamp);

   sprintf(buf, "<title>%s</title><br><p><description>%s</description>", item->name, time_diff_string );

   free(time_diff_string);
   time_diff_string = NULL;

   tree_row = etk_tree_row_prepend(ETK_TREE(obj->tree), NULL, obj->col, buf, NULL );
   free(buf);
   buf = NULL;
   if(tree_row)
     etk_tree_row_data_set(tree_row, item);
}

void
e_nav_taglist_tag_update(Tag_List *obj, const char *name, const char *description, void *object)
{
   Evas_Object *location_obj;
   Etk_Tree_Row *row;
   Tag_List_Item *item;
   char *time_diff_string;
   char *buf;

   if(!obj || !object) return; 

   for (row = etk_tree_first_row_get(ETK_TREE(obj->tree)); row; row = etk_tree_row_next_get(row))
     {
        item = (Tag_List_Item *)etk_tree_row_data_get(row);
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
             if(item->name != NULL && !strcmp(item->name, "") )
	       {
                  free(item->name);
                  item->name = strdup(_("No Title"));
               }
             buf = (char *)malloc(strlen(item->name) + 128);
             time_diff_string = get_time_diff_string(item->timestamp);
               sprintf(buf, "<title>%s</title><br><p><description>%s</description>",
                       item->name, time_diff_string );
             
             free(time_diff_string);
             time_diff_string = NULL;
             etk_tree_row_fields_set(row, FALSE, obj->col, buf, NULL);
             free(buf);
             buf = NULL;
          }
        location_obj = NULL;
         
     }
}

void
e_nav_taglist_tag_remove(Tag_List *obj, Evas_Object *tag)
{
   Evas_Object *location_obj;
   Etk_Tree_Row *row;
   Tag_List_Item *item;

   if(!obj || !tag) return; 

   for (row = etk_tree_first_row_get(ETK_TREE(obj->tree)); row; row = etk_tree_row_next_get(row))
     {
        item = (Tag_List_Item *)etk_tree_row_data_get(row);
        location_obj = item->data2;
        if(location_obj == tag)
          {
             etk_tree_row_delete(row);
             break;
          }
        location_obj = NULL;
         
     }
}

void 
e_nav_taglist_clear(Tag_List *obj)
{
   etk_tree_clear(ETK_TREE(obj->tree));
   etk_tree_thaw(ETK_TREE(obj->tree));
}

void
e_nav_taglist_activate(Tag_List *tl)
{
   etk_tree_unselect_all(ETK_TREE(tl->tree));
   _e_taglist_update(tl);
}

void
e_nav_taglist_deactivate(Tag_List *tl)
{
   evas_object_hide(tl->frame);
   etk_widget_hide(tl->embed);
   etk_widget_hide(tl->tree);
}

static void
_e_taglist_update(Tag_List *tl)
{
   Evas_Coord screen_x, screen_y, screen_w, screen_h;
   Etk_Tree_Row *row;
   Tag_List_Item *item;
   char *time_diff_string;
   char *buf;
   Evas_Object *location_obj;

   if(tl==NULL) return;

   evas_output_viewport_get(evas_object_evas_get(tl->frame), &screen_x, &screen_y, &screen_w, &screen_h);
   evas_object_move(tl->frame, screen_x, screen_y);
   evas_object_resize(tl->frame, screen_w, screen_h);

   etk_tree_col_sort(tl->col, TRUE);  // ascendant sort

   row = etk_tree_first_row_get(ETK_TREE(tl->tree));

   int unread;
   for (; row; row = etk_tree_row_next_get(row))
     {
        item = (Tag_List_Item *)etk_tree_row_data_get(row);
        buf = (char *)malloc(strlen(item->name) + 128);
        location_obj = (Evas_Object *)item->data2;
     
        unread = e_nav_world_item_location_unread_get(location_obj);
        if(unread)
          {
             sprintf(buf, "<title>%s</title><br><p><glow>%s</glow>", item->name, _("NEW!"));
          }
        else 
          {
             time_diff_string = get_time_diff_string(item->timestamp);
             sprintf(buf, "<title>%s</title><br><p><description>%s</description>", item->name, time_diff_string );
          }

        free(time_diff_string);
        time_diff_string = NULL;
        
        etk_tree_row_fields_set(row, FALSE, tl->col, buf, NULL);
        free(buf);
        buf = NULL;
     }

   evas_object_show(tl->frame);
   evas_object_raise(tl->frame);
   etk_widget_show_all(tl->embed);
   etk_widget_show(tl->tree);
}

