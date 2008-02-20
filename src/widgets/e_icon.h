/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
/* e_icon.h -
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
#ifndef E_ICON_H
#define E_ICON_H

EAPI Evas_Object *e_icon_add              (Evas *evas);
EAPI void         e_icon_file_set         (Evas_Object *obj, const char *file);
EAPI void         e_icon_file_key_set     (Evas_Object *obj, const char *file, const char *key);
EAPI void         e_icon_file_edje_set    (Evas_Object *obj, const char *file, const char *part);
EAPI void         e_icon_object_set       (Evas_Object *obj, Evas_Object *o);    
EAPI const char  *e_icon_file_get         (Evas_Object *obj);
EAPI void         e_icon_smooth_scale_set (Evas_Object *obj, int smooth);
EAPI int          e_icon_smooth_scale_get (Evas_Object *obj);
EAPI void         e_icon_alpha_set        (Evas_Object *obj, int smooth);
EAPI int          e_icon_alpha_get        (Evas_Object *obj);
EAPI void         e_icon_size_get         (Evas_Object *obj, int *w, int *h);
EAPI int          e_icon_fill_inside_get  (Evas_Object *obj);
EAPI void         e_icon_fill_inside_set  (Evas_Object *obj, int fill_inside);
EAPI void         e_icon_data_set         (Evas_Object *obj, void *data, int w, int h);
EAPI void        *e_icon_data_get         (Evas_Object *obj, int *w, int *h);
    
#endif
//#endif
