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
#include <Edje.h>
#include "e_nav_item_neo_other.h"

Evas_Object        *e_ctrl_add(Evas *e);
void                e_ctrl_theme_source_set(Evas_Object *obj, const char *custom_dir);
void                e_ctrl_nav_set(Evas_Object* obj);
void                e_ctrl_neo_me_set(Evas_Object *obj);
void               *e_ctrl_neo_me_get(void);
Diversity_Equipment *e_ctrl_self_equipment_get(const char *eqp_name);
void                e_ctrl_zoom_drag_value_set(double y); 
void                e_ctrl_zoom_text_value_set(const char* buf);
void                e_ctrl_longitude_set(const char* buf);
void                e_ctrl_latitude_set(const char* buf);
void                e_ctrl_taglist_tag_add(const char *name, const char *note, time_t timestamp, void *loc_object);
void                e_ctrl_taglist_tag_set(const char *name, const char *note, void *object);
void                e_ctrl_taglist_tag_delete(void *loc_object);
int                 e_ctrl_contact_add(const char *id, Neo_Other_Data *data);
Neo_Other_Data     *e_ctrl_contact_get(const char *id);
Neo_Other_Data     *e_ctrl_contact_get_by_name(const char *name);
Neo_Other_Data     *e_ctrl_contact_get_by_number(const char *number);
Ecore_List         *e_ctrl_contacts_get(void);
void                e_ctrl_contact_remove(const char *id);
int                 e_ctrl_follow_get(Evas_Object* obj);
void                e_ctrl_follow_set(int follow);
void                e_ctrl_message_text_set(Evas_Object *obj, const char *msg);
void                e_ctrl_message_hide(Evas_Object *obj);
void                e_ctrl_message_show(Evas_Object *obj);

#endif
