/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
/* e_box.h -
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
#ifndef E_BOX_H
#define E_BOX_H
#ifdef EAPI
#undef EAPI
#endif
#ifdef WIN32
# ifdef BUILDING_DLL
#  define EAPI __declspec(dllexport)
# else
#  define EAPI __declspec(dllimport)
# endif
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
/* BROKEN in gcc 4 on amd64 */
#if 0
#   pragma GCC visibility push(hidden)
#endif
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif



EAPI Evas_Object *e_box_add               (Evas *evas);
EAPI int          e_box_freeze            (Evas_Object *obj);
EAPI int          e_box_thaw              (Evas_Object *obj);
EAPI void         e_box_orientation_set   (Evas_Object *obj, int horizontal);
EAPI int          e_box_orientation_get   (Evas_Object *obj);
EAPI void         e_box_homogenous_set    (Evas_Object *obj, int homogenous);
EAPI int          e_box_pack_start        (Evas_Object *obj, Evas_Object *child);
EAPI int          e_box_pack_end          (Evas_Object *obj, Evas_Object *child);
EAPI int          e_box_pack_before       (Evas_Object *obj, Evas_Object *child, Evas_Object *before);
EAPI int          e_box_pack_after        (Evas_Object *obj, Evas_Object *child, Evas_Object *after);
EAPI int          e_box_pack_count_get    (Evas_Object *obj);
EAPI Evas_Object *e_box_pack_object_nth   (Evas_Object *obj, int n);
EAPI Evas_Object *e_box_pack_object_first (Evas_Object *obj);
EAPI Evas_Object *e_box_pack_object_last  (Evas_Object *obj);
EAPI void         e_box_pack_options_set  (Evas_Object *obj, int fill_w, int fill_h, int expand_w, int expand_h, double align_x, double align_y, Evas_Coord min_w, Evas_Coord min_h, Evas_Coord max_w, Evas_Coord max_h);
EAPI void         e_box_unpack            (Evas_Object *obj);
EAPI void         e_box_min_size_get      (Evas_Object *obj, Evas_Coord *minw, Evas_Coord *minh);
EAPI void         e_box_max_size_get      (Evas_Object *obj, Evas_Coord *maxw, Evas_Coord *maxh);
EAPI void         e_box_align_get         (Evas_Object *obj, double *ax, double *ay);
EAPI void         e_box_align_set         (Evas_Object *obj, double ax, double ay);
EAPI void	  e_box_align_pixel_offset_get (Evas_Object *obj, int *x, int *y);

#endif
//#endif
