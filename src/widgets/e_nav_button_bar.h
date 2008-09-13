/* e_nav_button_bar.h -
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

#ifndef E_NAV_BUTTON_BAR_H
#define E_NAV_BUTTON_BAR_H

#include <Evas.h>

enum {
     E_NAV_BUTTON_BAR_STYLE_SPREAD,
     E_NAV_BUTTON_BAR_STYLE_LEFT_ALIGNED,
     E_NAV_BUTTON_BAR_STYLE_RIGHT_ALIGNED,
     N_E_NAV_BUTTON_BAR_STYLES
};

Evas_Object            *e_nav_button_bar_add(Evas *e);
void                    e_nav_button_bar_embed_set(Evas_Object *bbar, Evas_Object *embedding, const char *group_base);
void                    e_nav_button_bar_style_set(Evas_Object *bbar, int style);
void                    e_nav_button_bar_paddings_set(Evas_Object *bbar, Evas_Coord front, Evas_Coord inter, Evas_Coord back);
void                    e_nav_button_bar_button_size_request(Evas_Object *bbar, Evas_Coord w, Evas_Coord h);
int                     e_nav_button_bar_num_buttons_get(Evas_Object *bbar);
Evas_Coord              e_nav_button_bar_width_min_calc(Evas_Object *bbar);
Evas_Coord              e_nav_button_bar_height_min_calc(Evas_Object *bbar);

void                    e_nav_button_bar_button_add(Evas_Object *bbar, const char *label, void (*func)(void *data, Evas_Object *obj), void *data);
void                    e_nav_button_bar_button_add_back(Evas_Object *bbar, const char *label, void (*func)(void *data, Evas_Object *obj), void *data);
void                    e_nav_button_bar_button_remove(Evas_Object *bbar, void (*func)(void *data, Evas_Object *bbar), void *data);

#endif /* E_NAV_BUTTON_BAR_H */
