/* e_ctrl.h -
 *
 * Copyright 2008 OpenMoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
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

#ifndef E_CTRL_H
#define E_CTRL_H

#include <Ecore.h>
#include <Evas.h>
#include "e_nav_item_neo_other.h"

Evas_Object        *e_ctrl_add(Evas *e);
void                e_ctrl_theme_source_set(Evas_Object *obj, const char *custom_dir);
void                e_ctrl_nav_set(Evas_Object *obj, Evas_Object *nav);

void                e_ctrl_span_drag_value_set(Evas_Object *obj, int span);
void                e_ctrl_span_text_value_set(Evas_Object *obj, const char* buf);
void                e_ctrl_longitude_set(Evas_Object *obj, const char* buf);
void                e_ctrl_latitude_set(Evas_Object *obj, const char* buf);
int                 e_ctrl_follow_get(Evas_Object* obj);
void                e_ctrl_follow_set(Evas_Object *obj, int follow);

unsigned int        e_ctrl_message_text_add(Evas_Object *obj, const char *msg, double timeout);
void                e_ctrl_message_text_del(Evas_Object *obj, unsigned int msg_id);

void                e_ctrl_taglist_tag_add(Evas_Object *obj, Evas_Object *loc);
void                e_ctrl_taglist_tag_set(Evas_Object *obj, Evas_Object *loc);
void                e_ctrl_taglist_tag_delete(Evas_Object *obj, Evas_Object *loc);
void                e_ctrl_taglist_freeze(Evas_Object *obj);
void                e_ctrl_taglist_thaw(Evas_Object *obj);

void                e_ctrl_contact_add(Evas_Object *obj, E_Nav_Card *card);
void                e_ctrl_contact_delete(Evas_Object *obj, E_Nav_Card *card);
E_Nav_Card         *e_ctrl_contact_get_by_name(Evas_Object *obj, const char *name);
E_Nav_Card         *e_ctrl_contact_get_by_number(Evas_Object *obj, const char *number);
/* the list is owned by e_ctrl */
Evas_List          *e_ctrl_contact_list(Evas_Object *obj);

void                e_ctrl_object_store_item_add(Evas_Object *obj, const char *obj_path, void *item);
void               *e_ctrl_object_store_item_get(Evas_Object *obj, const char *obj_path);
void               *e_ctrl_object_store_item_remove(Evas_Object *obj, const char *obj_path);

#endif
