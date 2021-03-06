/* e_nav_item_neo_other.h -
 *
 * Copyright 2007-2008 Openmoko, Inc.
 * Authored by Carsten Haitzler <raster@openmoko.org>
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

#ifndef E_NAV_ITEM_NEO_OTHER_H
#define E_NAV_ITEM_NEO_OTHER_H

#include "e_nav_dbus.h"

Evas_Object            *e_nav_world_item_neo_other_add(Evas_Object *nav, const char *theme_dir, double lon, double lat, Diversity_Object *bard);
void                    e_nav_world_item_neo_other_name_set(Evas_Object *item, const char *name);
const char             *e_nav_world_item_neo_other_name_get(Evas_Object *item);
void                    e_nav_world_item_neo_other_phone_set(Evas_Object *item, const char *phone);
const char             *e_nav_world_item_neo_other_phone_get(Evas_Object *item);
void                    e_nav_world_item_neo_other_alias_set(Evas_Object *item, const char *alias);
const char             *e_nav_world_item_neo_other_alias_get(Evas_Object *item);
void                    e_nav_world_item_neo_other_twitter_set(Evas_Object *item, const char *twitter);
const char             *e_nav_world_item_neo_other_twitter_get(Evas_Object *item);
void                    e_nav_world_item_neo_other_accuracy_set(Evas_Object *item, int accuracy);
int                     e_nav_world_item_neo_other_accuracy_get(Evas_Object *item);
Diversity_Bard         *e_nav_world_item_neo_other_bard_get(Evas_Object *item);
const char             *e_nav_world_item_neo_other_path_get(Evas_Object *item);

/* hey, these should be in another file */

typedef struct _E_Nav_Card E_Nav_Card;

E_Nav_Card         *e_nav_card_new(void);
void                e_nav_card_destroy(E_Nav_Card *card);
void                e_nav_card_bard_set(E_Nav_Card *card, Diversity_Bard *bard);
Diversity_Bard     *e_nav_card_bard_get(E_Nav_Card *card);
void                e_nav_card_name_set(E_Nav_Card *card, const char *name);
const char         *e_nav_card_name_get(E_Nav_Card *card);
void                e_nav_card_phone_set(E_Nav_Card *card, const char *phone);
const char         *e_nav_card_phone_get(E_Nav_Card *card);

#endif
