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
#include "e_nav_tree_model.h"

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _Tag_List_Callback Tag_List_Callback;

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *clip;
   char            *dir;

   Evas_Object     *frame;
   Etk_Widget      *embed;
   Etk_Widget      *tree;
   Etk_Tree_Col    *col;

   Evas_List       *callbacks;
};

struct _Tag_List_Callback
{
   void (*func)(void *data, Evas_Object *tl, Evas_Object *tag);
   void *data;
};

static void _e_nav_taglist_smart_init(void);
static void _e_nav_taglist_smart_add(Evas_Object *obj);
static void _e_nav_taglist_smart_del(Evas_Object *obj);
static void _e_nav_taglist_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_nav_taglist_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_nav_taglist_smart_show(Evas_Object *obj);
static void _e_nav_taglist_smart_hide(Evas_Object *obj);
static void _e_nav_taglist_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_nav_taglist_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_nav_taglist_smart_clip_unset(Evas_Object *obj);

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_nav_taglist")) return ret

static void
_taglist_hide_cb(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
   Etk_Tree_Row *row;

   sd = evas_object_smart_data_get(data);
   if (!sd)
     return;

   row = etk_tree_selected_row_get(ETK_TREE(sd->tree));
   if (row)
     etk_tree_row_unselect(row);
}

static Etk_Bool
_taglist_tree_row_clicked_cb(Etk_Tree *tree, Etk_Tree_Row *row, Etk_Event_Mouse_Up *event, void *data)
{
   Evas_Object *tl = data;
   E_Smart_Data *sd;
   Evas_Object *tag;
   Evas_List *l;

   SMART_CHECK(tl, ETK_TRUE;);

   tag = etk_tree_row_data_get(row);

   for (l = sd->callbacks; l; l = l->next)
     {
	Tag_List_Callback *cb = l->data;

	cb->func(cb->data, tl, tag);
     }

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

Evas_Object *
e_nav_taglist_add(Evas *e, const char *custom_dir)
{
   Evas_Object *tl;
   Evas_Object *embed_obj;
   E_Smart_Data *sd;

   _e_nav_taglist_smart_init();

   tl = evas_object_smart_add(e, _e_smart);
   if (!tl)
     return NULL;

   SMART_CHECK(tl, NULL;);

   if (custom_dir)
     sd->dir = strdup(custom_dir);

   sd->frame = e_nav_theme_object_new(e, sd->dir,
	 "modules/diversity_nav/taglist");
   edje_object_part_text_set(sd->frame, "title", _("View Tags"));
   evas_object_smart_member_add(sd->frame, sd->obj);
   evas_object_move(sd->frame, sd->x, sd->y);
   evas_object_resize(sd->frame, sd->w, sd->h);
   evas_object_clip_set(sd->frame, sd->clip);
   evas_object_show(sd->frame);

   evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_HIDE,
	 _taglist_hide_cb, tl);

   sd->tree = etk_tree_new();
   etk_tree_headers_visible_set(ETK_TREE(sd->tree), 0);
   etk_tree_mode_set(ETK_TREE(sd->tree), ETK_TREE_MODE_LIST);
   etk_tree_multiple_select_set(ETK_TREE(sd->tree), ETK_FALSE);
   etk_tree_rows_height_set(ETK_TREE(sd->tree), 90);
   etk_scrolled_view_policy_set(
	 etk_tree_scrolled_view_get(ETK_TREE(sd->tree)),
	 ETK_POLICY_HIDE, ETK_POLICY_HIDE);
   etk_scrolled_view_dragable_set(
	 ETK_SCROLLED_VIEW(etk_tree_scrolled_view_get(ETK_TREE(sd->tree))),
	 ETK_TRUE);

   etk_signal_connect_by_code(ETK_TREE_ROW_CLICKED_SIGNAL, ETK_OBJECT(sd->tree),
      ETK_CALLBACK(_taglist_tree_row_clicked_cb), tl);

   sd->col = etk_tree_col_new(ETK_TREE(sd->tree), NULL, 455, 0.0);
   etk_tree_col_model_add(sd->col, e_nav_tree_model_tag_new());
   etk_tree_col_sort_set(sd->col, _taglist_sort_compare_cb, NULL);
   etk_tree_col_sort(sd->col, TRUE);

   etk_tree_build(ETK_TREE(sd->tree));

   sd->embed = etk_embed_new(e);
   etk_container_add(ETK_CONTAINER(sd->embed), sd->tree);
   etk_widget_show_all(ETK_WIDGET(sd->embed));

   embed_obj = etk_embed_object_get(ETK_EMBED(sd->embed));
   edje_object_part_swallow(sd->frame, "swallow", embed_obj);

   return tl;
}

void
e_nav_taglist_callback_add(Evas_Object *tl, void (*func)(void *data, Evas_Object *tl, Evas_Object *tag), void *data)
{
   E_Smart_Data *sd;
   Tag_List_Callback *cb;

   SMART_CHECK(tl, ;);

   cb = malloc(sizeof(*cb));
   if (!cb)
     return;

   cb->func = func;
   cb->data = data;

   sd->callbacks = evas_list_prepend(sd->callbacks, cb);
}

void
e_nav_taglist_callback_del(Evas_Object *tl, void *func, void *data)
{
   E_Smart_Data *sd;
   Tag_List_Callback *cb;
   Evas_List *l;

   SMART_CHECK(tl, ;);

   for (l = sd->callbacks; l; l = l->next)
     {
	cb = l->data;

	if (cb->func == func && cb->data == data)
	  break;
     }

   if (!l)
     return;

   sd->callbacks = evas_list_remove_list(sd->callbacks, l);
   free(cb);
}

void
e_nav_taglist_tag_add(Evas_Object *tl, Evas_Object *tag)
{
   E_Smart_Data *sd;
   Etk_Tree_Row *tree_row;

   SMART_CHECK(tl, ;);

   if (!tag)
     return;

   tree_row = etk_tree_row_prepend(ETK_TREE(sd->tree), NULL, sd->col, tag, NULL);
   if (tree_row)
     {
	etk_tree_row_data_set(tree_row, tag);
	etk_tree_col_sort(sd->col, TRUE);
     }
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
e_nav_taglist_tag_update(Evas_Object *tl, Evas_Object *tag)
{
   E_Smart_Data *sd;
   Etk_Tree_Row *row;

   SMART_CHECK(tl, ;);

   if (!tag)
     return;

   row = _etk_tree_row_find_by_data(ETK_TREE(sd->tree), tag);
   if (!row)
     return;

   etk_tree_row_fields_set(row, FALSE, sd->col, tag, NULL);
   etk_tree_col_sort(sd->col, TRUE);
}

void
e_nav_taglist_tag_remove(Evas_Object *tl, Evas_Object *tag)
{
   E_Smart_Data *sd;
   Etk_Tree_Row *row;

   SMART_CHECK(tl, ;);

   row = _etk_tree_row_find_by_data(ETK_TREE(sd->tree), tag);
   if (row)
     etk_tree_row_delete(row);
}

void
e_nav_taglist_clear(Evas_Object *tl)
{
   E_Smart_Data *sd;

   SMART_CHECK(tl, ;);

   etk_tree_clear(ETK_TREE(sd->tree));
}

/* internal calls */
static void
_e_nav_taglist_smart_init(void)
{
   if (!_e_smart)
     {
	static const Evas_Smart_Class sc =
	  {
	     "e_nav_taglist",
	     EVAS_SMART_CLASS_VERSION,
	     _e_nav_taglist_smart_add,
	     _e_nav_taglist_smart_del,
	     _e_nav_taglist_smart_move,
	     _e_nav_taglist_smart_resize,
	     _e_nav_taglist_smart_show,
	     _e_nav_taglist_smart_hide,
	     _e_nav_taglist_smart_color_set,
	     _e_nav_taglist_smart_clip_set,
	     _e_nav_taglist_smart_clip_unset,

	     NULL /* data */
	  };


	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_nav_taglist_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd)
     return;

   sd->obj = obj;

   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;

   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);

   evas_object_smart_data_set(obj, sd);
}

static void
_e_nav_taglist_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   if (sd->dir)
     free(sd->dir);

   evas_object_del(sd->clip);
   evas_object_del(sd->frame);

   free(sd);
}

static void
_e_nav_taglist_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   sd->x = x;
   sd->y = y;

   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_move(sd->frame, sd->x, sd->y);
}

static void
_e_nav_taglist_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   sd->w = w;
   sd->h = h;

   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_resize(sd->frame, sd->w, sd->h);
}

static void
_e_nav_taglist_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   evas_object_show(sd->clip);
}

static void
_e_nav_taglist_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   evas_object_hide(sd->clip);
}

static void
_e_nav_taglist_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_nav_taglist_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   evas_object_clip_set(sd->clip, clip);
}

static void
_e_nav_taglist_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   evas_object_clip_unset(sd->clip);
}
