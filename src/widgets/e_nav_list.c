/* e_nav_list.c -
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
#include "e_nav_list.h"
#include "e_nav_tree_model.h"
#include "e_nav_button_bar.h"
#include "../e_nav_theme.h"
#include "../e_nav.h"

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _List_Row_Callback List_Row_Callback;

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *clip;
   int              type;

   Evas_Object     *frame;
   Etk_Widget      *embed;
   Etk_Widget      *tree;
   Etk_Tree_Col    *col;

   Evas_Object     *bbar;

   int frozen;

   Evas_List       *callbacks;

   int            (*sort_cb)(void *data, E_Nav_List_Item *item1, E_Nav_List_Item *item2);
   void            *sort_data;
};

struct _List_Row_Callback
{
   void (*func)(void *data, Evas_Object *li, E_Nav_List_Item *item);
   void *data;
};

static void _e_nav_list_smart_init(void);
static void _e_nav_list_smart_add(Evas_Object *obj);
static void _e_nav_list_smart_del(Evas_Object *obj);
static void _e_nav_list_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_nav_list_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_nav_list_smart_show(Evas_Object *obj);
static void _e_nav_list_smart_hide(Evas_Object *obj);
static void _e_nav_list_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_nav_list_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_nav_list_smart_clip_unset(Evas_Object *obj);

#define SMART_NAME "e_nav_list"
static Evas_Smart *_e_smart = NULL;

static int
_list_compare(Etk_Tree_Col *col, Etk_Tree_Row *row1, Etk_Tree_Row *row2, void *data)
{
   E_Smart_Data *sd = data;
   E_Nav_List_Item *item1, *item2;

   item1 = etk_tree_row_data_get(row1);
   if (!row1 || !item1)
     return 1;

   item2 = etk_tree_row_data_get(row2);
   if (!row2 || !item2)
     return -1;

   return sd->sort_cb(sd->sort_data, item1, item2);
}

static void
_list_hide_cb(void *data, Evas *evas, Evas_Object *li, void *event)
{
   E_Smart_Data *sd = data;
   Etk_Tree_Row *row;

   row = etk_tree_selected_row_get(ETK_TREE(sd->tree));
   if (row)
     etk_tree_row_unselect(row);
}

static Etk_Bool
_list_tree_row_clicked_cb(Etk_Tree *tree, Etk_Tree_Row *row, Etk_Event_Mouse_Up *event, void *data)
{
   Evas_Object *li = data;
   E_Smart_Data *sd;
   E_Nav_List_Item *item;
   Evas_List *l;

   SMART_CHECK(li, ETK_TRUE;);

   item = etk_tree_row_data_get(row);

   for (l = sd->callbacks; l; l = l->next)
     {
	List_Row_Callback *cb = l->data;

	cb->func(cb->data, li, item);
     }

   return ETK_TRUE;
}

Evas_Object *
e_nav_list_add(Evas *e, int type)
{
   Evas_Object *li;
   Evas_Object *embed_obj;
   E_Smart_Data *sd;
   Etk_Tree_Model *model;

   _e_nav_list_smart_init();

   li = evas_object_smart_add(e, _e_smart);
   if (!li)
     return NULL;

   SMART_CHECK(li, NULL;);

   sd->type = type;

   sd->frame = e_nav_theme_object_new(e, NULL, "modules/diversity_nav/list");
   evas_object_smart_member_add(sd->frame, sd->obj);
   evas_object_move(sd->frame, sd->x, sd->y);
   evas_object_resize(sd->frame, sd->w, sd->h);
   evas_object_clip_set(sd->frame, sd->clip);
   evas_object_show(sd->frame);

   evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_HIDE,
	 _list_hide_cb, sd);

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
      ETK_CALLBACK(_list_tree_row_clicked_cb), li);

   sd->col = etk_tree_col_new(ETK_TREE(sd->tree), NULL, 455, 0.0);
   switch (type)
     {
      case E_NAV_LIST_TYPE_CARD:
	 model = e_nav_tree_model_card_new();
	 break;
      case E_NAV_LIST_TYPE_TAG:
      default:
	 model = e_nav_tree_model_tag_new();
	 break;
     }
   etk_tree_col_model_add(sd->col, model);

   etk_tree_build(ETK_TREE(sd->tree));

   sd->embed = etk_embed_new(e);
   etk_container_add(ETK_CONTAINER(sd->embed), sd->tree);
   etk_widget_show_all(ETK_WIDGET(sd->embed));

   embed_obj = etk_embed_object_get(ETK_EMBED(sd->embed));
   edje_object_part_swallow(sd->frame, "swallow", embed_obj);

   if (type == E_NAV_LIST_TYPE_TAG)
     edje_object_signal_emit(sd->frame, "e,state,taglist", "e");

   return li;
}

void
e_nav_list_title_set(Evas_Object *li, const char *title)
{
   E_Smart_Data *sd;

   SMART_CHECK(li, ;);
   edje_object_part_text_set(sd->frame, "title", title);
}

void
e_nav_list_sort_set(Evas_Object *li, int (*func)(void *data, E_Nav_List_Item *item1, E_Nav_List_Item *item2), void *data)
{
   E_Smart_Data *sd;

   SMART_CHECK(li, ;);

   sd->sort_cb = func;
   sd->sort_data = data;

   if (sd->sort_cb)
     {
	etk_tree_col_sort_set(sd->col, _list_compare, sd);

	if (!sd->frozen)
	  etk_tree_col_sort(sd->col, TRUE);
     }
   else
     {
	etk_tree_col_sort_set(sd->col, NULL, NULL);
     }
}

void
e_nav_list_button_add(Evas_Object *li, const char *label, void (*func)(void *data, Evas_Object *li), void *data)
{
   E_Smart_Data *sd;

   SMART_CHECK(li, ;);

   if (!sd->bbar)
     {
	sd->bbar = e_nav_button_bar_add(evas_object_evas_get(li));
	e_nav_button_bar_embed_set(sd->bbar, sd->obj,
	      "modules/diversity_nav/button_bar/list");
	e_nav_button_bar_style_set(sd->bbar,
	      E_NAV_BUTTON_BAR_STYLE_RIGHT_ALIGNED);

	edje_object_part_swallow(sd->frame, "button_bar", sd->bbar);
     }

   e_nav_button_bar_button_add(sd->bbar, label, func, data);
   edje_object_signal_emit(sd->frame, "e,state,active", "e");
}

void
e_nav_list_button_remove(Evas_Object *li, void (*func)(void *data, Evas_Object *li), void *data)
{
   E_Smart_Data *sd;

   SMART_CHECK(li, ;);

   if (!sd->bbar)
     return;

   e_nav_button_bar_button_remove(sd->bbar, func, data);
   if (!e_nav_button_bar_num_buttons_get(sd->bbar))
     {
	edje_object_signal_emit(sd->frame, "e,state,passive", "e");

	evas_object_del(sd->bbar);
	sd->bbar = NULL;
     }
}

void
e_nav_list_callback_add(Evas_Object *li, void (*func)(void *data, Evas_Object *li, E_Nav_List_Item *item), void *data)
{
   E_Smart_Data *sd;
   List_Row_Callback *cb;

   SMART_CHECK(li, ;);

   cb = malloc(sizeof(*cb));
   if (!cb)
     return;

   cb->func = func;
   cb->data = data;

   sd->callbacks = evas_list_prepend(sd->callbacks, cb);
}

void
e_nav_list_callback_del(Evas_Object *li, void *func, void *data)
{
   E_Smart_Data *sd;
   List_Row_Callback *cb;
   Evas_List *l;

   SMART_CHECK(li, ;);

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
e_nav_list_item_add(Evas_Object *li, E_Nav_List_Item *item)
{
   E_Smart_Data *sd;
   Etk_Tree_Row *tree_row;

   SMART_CHECK(li, ;);

   if (!item)
     return;

   tree_row = etk_tree_row_prepend(ETK_TREE(sd->tree), NULL, sd->col, item, NULL);
   if (tree_row)
     {
	etk_tree_row_data_set(tree_row, item);

	if (!sd->frozen)
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
e_nav_list_item_update(Evas_Object *li, E_Nav_List_Item *item)
{
   E_Smart_Data *sd;
   Etk_Tree_Row *row;

   SMART_CHECK(li, ;);

   if (!item)
     return;

   row = _etk_tree_row_find_by_data(ETK_TREE(sd->tree), item);
   if (!row)
     return;

   etk_tree_row_fields_set(row, FALSE, sd->col, item, NULL);

   if (!sd->frozen)
     etk_tree_col_sort(sd->col, TRUE);
}

void
e_nav_list_item_remove(Evas_Object *li, E_Nav_List_Item *item)
{
   E_Smart_Data *sd;
   Etk_Tree_Row *row;

   SMART_CHECK(li, ;);

   row = _etk_tree_row_find_by_data(ETK_TREE(sd->tree), item);
   if (row)
     etk_tree_row_delete(row);
}

void
e_nav_list_clear(Evas_Object *li)
{
   E_Smart_Data *sd;

   SMART_CHECK(li, ;);

   etk_tree_clear(ETK_TREE(sd->tree));
}

void e_nav_list_freeze(Evas_Object *li)
{
   E_Smart_Data *sd;

   SMART_CHECK(li, ;);

   sd->frozen = 1;
   etk_tree_freeze(ETK_TREE(sd->tree));
}

void e_nav_list_thaw(Evas_Object *li)
{
   E_Smart_Data *sd;

   SMART_CHECK(li, ;);

   etk_tree_thaw(ETK_TREE(sd->tree));
   sd->frozen = 0;

   etk_tree_col_sort(sd->col, TRUE);
}

/* internal calls */
static void
_e_nav_list_smart_init(void)
{
   if (!_e_smart)
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
	     EVAS_SMART_CLASS_VERSION,
	     _e_nav_list_smart_add,
	     _e_nav_list_smart_del,
	     _e_nav_list_smart_move,
	     _e_nav_list_smart_resize,
	     _e_nav_list_smart_show,
	     _e_nav_list_smart_hide,
	     _e_nav_list_smart_color_set,
	     _e_nav_list_smart_clip_set,
	     _e_nav_list_smart_clip_unset,

	     NULL /* data */
	  };


	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_nav_list_smart_add(Evas_Object *obj)
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
_e_nav_list_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   while (sd->callbacks)
     {
	free(sd->callbacks->data);
	sd->callbacks = evas_list_remove_list(sd->callbacks, sd->callbacks);
     }

   etk_object_destroy(ETK_OBJECT(sd->embed));

   if (sd->bbar)
     evas_object_del(sd->bbar);

   evas_object_del(sd->frame);
   evas_object_del(sd->clip);

   free(sd);
}

static void
_e_nav_list_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
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
_e_nav_list_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
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
_e_nav_list_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   evas_object_show(sd->clip);
}

static void
_e_nav_list_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   evas_object_hide(sd->clip);
}

static void
_e_nav_list_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_nav_list_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   evas_object_clip_set(sd->clip, clip);
}

static void
_e_nav_list_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     return;

   evas_object_clip_unset(sd->clip);
}
