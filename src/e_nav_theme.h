/* e_nav_theme.h -
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

#ifndef E_NAV_THEME_H
#define E_NAV_THEME_H

#include <Evas.h>

int              e_nav_theme_init(const char *theme);
void             e_nav_theme_shutdown(void);

const char      *e_nav_theme_name_get(void);
const char      *e_nav_theme_path_get(void);

/* custom_dir is ignored */
Evas_Object     *e_nav_theme_object_new(Evas *e, const char *custom_dir, const char *group);
int              e_nav_theme_object_set(Evas_Object *o, const char *custom_dir, const char *group);
Evas_Object     *e_nav_theme_component_new(Evas *e, const char *base_group, const char *comp, int check);
int              e_nav_theme_group_exist(const char *custom_dir, const char *group);

#endif /* E_NAV_THEME_H */
