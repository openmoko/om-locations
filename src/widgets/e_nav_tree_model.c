/* e_nav_tree_model.c -
 *
 * Copyright 2008 OpenMoko, Inc.
 * Authored by Chia-I Wu <olv@openmoko.com>
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

#include <string.h>
#include <stdlib.h>
#include <etk_tree_model.h>
#include <etk_theme.h>
#include <Edje.h>

#include "e_nav_tree_model.h"
#include "../e_nav_item_location.h"
#include "../e_nav_item_neo_other.h"
#include "../e_nav.h"

typedef struct _Object_Data Object_Data;
typedef struct _Tag_Data Tag_Data;
typedef struct _Bard_Data Bard_Data;

typedef struct _Object Object;

struct _Object_Data
{
   Object *obj;
   Evas_Bool changed;
};

struct _Tag_Data
{
   Object_Data data;
   int w, h;
};

struct _Bard_Data
{
   Object_Data data;
   int w, h;
};

static void
_object_cell_data_free(Etk_Tree_Model *model, void *cell_data)
{
   Object_Data *odata = cell_data;

   if (!odata)
     return;
}

static void
_object_cell_data_set(Etk_Tree_Model *model, void *cell_data, va_list *args)
{
   Object_Data *odata = cell_data;
   Object *obj;

   if (!odata || !args)
     return;

   obj = va_arg(*args, Object *);
   odata->obj = obj;
   odata->changed = 1;
}

static void
_object_cell_data_get(Etk_Tree_Model *model, void *cell_data, va_list *args)
{
   Object_Data *odata = cell_data;
   Object **obj;

   if (!odata || ! args)
     return;

   obj = va_arg(*args, Object **);
   if (obj)
     *obj = odata->obj;
}

static void
_bard_objects_create(Etk_Tree_Model *model, Evas_Object *cell_objects[ETK_TREE_MAX_OBJECTS_PER_MODEL], Evas *evas)
{
   Evas_Object *cell;

   if (!evas)
     return;

   cell = edje_object_add(evas);

   etk_theme_edje_object_set_from_parent(cell, "text", ETK_WIDGET(model->tree));
   evas_object_pass_events_set(cell, 1);

   cell_objects[0] = cell;
}

static void
_bard_update(Evas_Object *bard, Bard_Data *bdata)
{
   char text[1024];

   if (!bdata->data.changed &&
	 evas_object_data_get(bard, "e_nav_bard") == bdata)
     return;

   snprintf(text, sizeof(text),
	 "<b><font_size=48>%s</></b>",
	 e_nav_world_item_neo_other_name_get((Evas_Object *) bdata->data.obj));

   edje_object_part_text_set(bard, "etk.text.label", text);
   edje_object_size_min_calc(bard, &bdata->w, &bdata->h);

   evas_object_data_set(bard, "e_nav_bard", bdata);
   bdata->data.changed = 0;
}

static Etk_Bool
_bard_render(Etk_Tree_Model *model, Etk_Tree_Row *row, Etk_Geometry geometry, void *cell_data, Evas_Object *cell_objects[ETK_TREE_MAX_OBJECTS_PER_MODEL], Evas *evas)
{
   Bard_Data *bdata = cell_data;
   Evas_Object * cell = cell_objects[0];

   if (!bdata || !cell)
     return ETK_FALSE;

   _bard_update(cell, bdata);

   evas_object_move(cell, geometry.x, geometry.y + ((geometry.h - bdata->h) / 2));
   evas_object_resize(cell, geometry.w, geometry.h);
   evas_object_show(cell);

   return ETK_FALSE;
}

static int
_bard_width_get(Etk_Tree_Model *model, void *cell_data, Evas_Object *cell_objects[ETK_TREE_MAX_OBJECTS_PER_MODEL])
{
   Bard_Data *bdata = cell_data;
   Evas_Object * cell = cell_objects[0];

   if (!cell)
      return 0;

   _bard_update(cell, bdata);

   return bdata->w;
}

static void
_tag_objects_create(Etk_Tree_Model *model, Evas_Object *cell_objects[ETK_TREE_MAX_OBJECTS_PER_MODEL], Evas *evas)
{
   Evas_Object *cell;

   if (!evas)
     return;

   cell = edje_object_add(evas);

   etk_theme_edje_object_set_from_parent(cell, "text", ETK_WIDGET(model->tree));
   evas_object_pass_events_set(cell, 1);

   cell_objects[0] = cell;
}

static int
_tag_style(char *buf, int len, Evas_Object *tag)
{
   const char *title, *desc;
   const char *style;
   Evas_Bool unread;

   title = e_nav_world_item_location_name_get(tag);
   if (!title || !*title)
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

static void
_tag_update(Evas_Object *tag, Tag_Data *tdata)
{
   char text[1024];

   if (!tdata->data.changed &&
	 evas_object_data_get(tag, "e_nav_tag") == tdata)
     return;

   _tag_style(text, sizeof(text), (Evas_Object *) tdata->data.obj);

   edje_object_part_text_set(tag, "etk.text.label", text);
   edje_object_size_min_calc(tag, &tdata->w, &tdata->h);

   evas_object_data_set(tag, "e_nav_tag", tdata);
   tdata->data.changed = 0;
}

static Etk_Bool
_tag_render(Etk_Tree_Model *model, Etk_Tree_Row *row, Etk_Geometry geometry, void *cell_data, Evas_Object *cell_objects[ETK_TREE_MAX_OBJECTS_PER_MODEL], Evas *evas)
{
   Tag_Data *tdata = cell_data;
   Evas_Object * cell = cell_objects[0];

   if (!tdata || !cell)
     return ETK_FALSE;

   _tag_update(cell, tdata);

   evas_object_move(cell, geometry.x, geometry.y + ((geometry.h - tdata->h) / 2));
   evas_object_resize(cell, geometry.w, geometry.h);
   evas_object_show(cell);

   return ETK_FALSE;
}

static int
_tag_width_get(Etk_Tree_Model *model, void *cell_data, Evas_Object *cell_objects[ETK_TREE_MAX_OBJECTS_PER_MODEL])
{
   Tag_Data *tdata = cell_data;
   Evas_Object * cell = cell_objects[0];

   if (!cell)
      return 0;

   _tag_update(cell, tdata);

   return tdata->w;
}

Etk_Tree_Model *e_nav_tree_model_bard_new(void)
{
   Etk_Tree_Model *model;

   model = calloc(1, sizeof(Etk_Tree_Model));
   if (!model)
     return NULL;

   model->cell_data_size = sizeof(Bard_Data);
   model->cell_data_free = _object_cell_data_free;
   model->cell_data_set = _object_cell_data_set;
   model->cell_data_get = _object_cell_data_get;
   model->objects_create = _bard_objects_create;
   model->render = _bard_render;
   model->width_get = _bard_width_get;
   model->cache_remove = NULL;

   return model;
}

Etk_Tree_Model *e_nav_tree_model_tag_new(void)
{
   Etk_Tree_Model *model;

   model = calloc(1, sizeof(Etk_Tree_Model));
   if (!model)
     return NULL;

   model->cell_data_size = sizeof(Tag_Data);
   model->cell_data_free = _object_cell_data_free;
   model->cell_data_set = _object_cell_data_set;
   model->cell_data_get = _object_cell_data_get;
   model->objects_create = _tag_objects_create;
   model->render = _tag_render;
   model->width_get = _tag_width_get;
   model->cache_remove = NULL;

   return model;
}
