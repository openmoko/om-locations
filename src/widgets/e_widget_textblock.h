/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
/* e_widget_textblock.h -
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

//#ifdef E_TYPEDEFS
//#else
#ifndef E_WIDGET_TEXTBLOCK_H
#define E_WIDGET_TEXTBLOCK_H

EAPI Evas_Object *e_widget_textblock_add(Evas *evas);
EAPI void e_widget_textblock_markup_set(Evas_Object *obj, const char *text);
EAPI void e_widget_textblock_plain_set(Evas_Object *obj, const char *text);
    
EAPI void e_widget_textblock_move(Evas_Object *obj, int x, int y);
EAPI void e_widget_textblock_resize(Evas_Object *obj, int w, int h);
EAPI void e_widget_textblock_event_callback_add(Evas_Object *obj, Evas_Callback_Type type, void(*func)(void *data, Evas *e, Evas_Object *obj, 
                                                   void *event_info), const void *data);
EAPI void e_widget_textblock_show(Evas_Object *obj);
#endif
//#endif
