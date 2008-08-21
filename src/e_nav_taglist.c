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

#include <Etk.h>
#include "e_nav_taglist.h"
#include "e_nav_item_location.h"
#include "e_nav_theme.h"
#include "e_nav.h"

typedef struct _Tag_List_Callback Tag_List_Callback;

struct _Tag_List_Callback
{
   void (*func)(void *data, Tag_List *tl, Evas_Object *tag);
   void *data;
};

struct _Tag_List
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *frame;
   Etk_Widget      *embed;
   Etk_Widget      *tree;
   Etk_Tree_Col    *col;

   Evas_List       *callbacks;
};

static Etk_Bool
_etk_test_tree_row_clicked_cb(Etk_Tree *tree, Etk_Tree_Row *row, Etk_Event_Mouse_Up *event, void *data)
{
   Tag_List *tl = data;
   Evas_Object *tag;
   Evas_List *l;

   tag = etk_tree_row_data_get(row);

   for (l = tl->callbacks; l; l = l->next)
     {
	Tag_List_Callback *cb = l->data;

	cb->func(cb->data, tl, tag);
     }

   etk_tree_row_unselect(row);

   return ETK_TRUE;
}

static int
_taglist_sort_compare_cb(Etk_Tree_Col *col, Etk_Tree_Row *row1, Etk_Tree_Row *row2, void *data)
{
   Evas_Object *tag1, *tag2;
   time_t t1, t2;

   tag1 = etk_tree_row_data_get(row1);
   t1 = e_nav_world_item_location_timestamp_get(tag1);

   tag2 = etk_tree_row_data_get(row2);
   t2 = e_nav_world_item_location_timestamp_get(tag2);

   return (t2 - t1);
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
   etk_tree_rows_height_set(ETK_TREE(tl->tree), 90);

   tl->col = etk_tree_col_new(ETK_TREE(tl->tree), NULL, 455, 0.0);

   etk_tree_col_model_add(tl->col, etk_tree_model_text_new());
   etk_tree_col_sort_set(tl->col, _taglist_sort_compare_cb, NULL);

   etk_tree_headers_visible_set(ETK_TREE(tl->tree), 0);

   etk_signal_connect_by_code(ETK_TREE_ROW_CLICKED_SIGNAL, ETK_OBJECT(tl->tree),
      ETK_CALLBACK(_etk_test_tree_row_clicked_cb), tl);

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
e_nav_taglist_destroy(Tag_List *tl)
{
   if (!tl)
     return;

   evas_object_del(tl->frame);
   tl->frame = NULL;
   etk_object_destroy(ETK_OBJECT(tl->embed));
   etk_object_destroy(ETK_OBJECT(tl->tree));
   etk_object_destroy(ETK_OBJECT(tl->col));
   free(tl);
}

void
e_nav_taglist_callback_add(Tag_List *tl, void (*func)(void *data, Tag_List *tl, Evas_Object *tag), void *data)
{
   Tag_List_Callback *cb;

   cb = malloc(sizeof(*cb));
   if (!cb)
     return;

   cb->func = func;
   cb->data = data;

   tl->callbacks = evas_list_prepend(tl->callbacks, cb);
}

void
e_nav_taglist_callback_del(Tag_List *tl, void *func, void *data)
{
   Evas_List *l;
   Tag_List_Callback *cb;

   for (l = tl->callbacks; l; l = l->next)
     {
	cb = l->data;

	if (cb->func == func && cb->data == data)
	  break;
     }

   if (!l)
     return;


   tl->callbacks = evas_list_remove_list(tl->callbacks, l);
   free(cb);
}

static int
text_style(char *buf, int len, Evas_Object *tag)
{
   const char *title, *desc;
   const char *style;
   Evas_Bool unread;

   title = e_nav_world_item_location_name_get(tag);
   if (!title)
     title = _("No Title");

   unread = e_nav_world_item_location_unread_get(tag);

   if (unread)
     {
	style = "glow";
	desc = _("NEW!");
     }
   else
     {
	style = "description";
	desc = e_nav_world_item_location_timestring_get(tag);
     }


   return snprintf(buf, len,
	 "<title>%s</title><br>"
	 "<p><%s>%s</%s>",
	 title,
	 style, desc, style);
}

void
e_nav_taglist_tag_add(Tag_List *tl, Evas_Object *tag)
{
   Etk_Tree_Row *tree_row;
   char text[1024];

   if (!tag)
     return;

   text_style(text, sizeof(text), tag);
   tree_row = etk_tree_row_prepend(ETK_TREE(tl->tree), NULL, tl->col, text, NULL);
   if (tree_row)
     etk_tree_row_data_set(tree_row, tag);
}

static Etk_Tree_Row *
_etk_tree_row_find_by_data(Etk_Tree *tree, void *data)
{
   Etk_Tree_Row *row;

   row = etk_tree_first_row_get(tree);
   while (row)
     {
	void *tmp = etk_tree_row_data_get(row);

	if (tmp == data)
	  break;

	row = etk_tree_row_next_get(row);
     }

   return row;
}

void
e_nav_taglist_tag_update(Tag_List *tl, Evas_Object *tag)
{
   Etk_Tree_Row *row;
   char text[1024];

   if (!tag)
     return;

   row = _etk_tree_row_find_by_data(ETK_TREE(tl->tree), tag);
   if (!row)
     return;

   text_style(text, sizeof(text), tag);
   etk_tree_row_fields_set(row, FALSE, tl->col, text, NULL);
}

void
e_nav_taglist_tag_remove(Tag_List *tl, Evas_Object *tag)
{
   Etk_Tree_Row *row;

   row = _etk_tree_row_find_by_data(ETK_TREE(tl->tree), tag);
   if (row)
     etk_tree_row_delete(row);
}

void
e_nav_taglist_clear(Tag_List *tl)
{
   etk_tree_clear(ETK_TREE(tl->tree));
}

static void
_e_taglist_update(Tag_List *tl)
{
   Evas_Coord screen_x, screen_y, screen_w, screen_h;

   evas_output_viewport_get(evas_object_evas_get(tl->frame),
	 &screen_x, &screen_y, &screen_w, &screen_h);

   evas_object_move(tl->frame, screen_x, screen_y);
   evas_object_resize(tl->frame, screen_w, screen_h);

   etk_tree_col_sort(tl->col, TRUE);  // ascendant sort

   evas_object_show(tl->frame);
   evas_object_raise(tl->frame);
   etk_widget_show_all(tl->embed);
   etk_widget_show(tl->tree);
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
