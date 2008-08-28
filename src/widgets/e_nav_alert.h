/* e_nav_alert.h -
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

#ifndef E_NAV_ALERT_H
#define E_NAV_ALERT_H

/* object management */
Evas_Object    *e_alert_add(Evas *e);
void            e_alert_theme_source_set(Evas_Object *obj, const char *custom_dir);
void            e_alert_transient_for_set(Evas_Object *obj, Evas_Object *parent);
Evas_Object    *e_alert_transient_for_get(Evas_Object *obj);
void            e_alert_activate(Evas_Object *obj);
void            e_alert_deactivate(Evas_Object *obj);
void            e_alert_button_add(Evas_Object *obj, const char *label, void (*func) (void *data, Evas_Object *obj), void *data);
void            e_alert_title_set(Evas_Object *obj, const char *title, const char *message);
void            e_alert_title_color_set(Evas_Object *obj, int r, int g, int b, int a);

#endif
