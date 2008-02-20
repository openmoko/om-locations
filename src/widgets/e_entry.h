/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
/* e_entry.h -
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
#ifndef E_ENTRY_H
#define E_ENTRY_H

EAPI Evas_Object *e_entry_add                 (Evas *evas);
EAPI void         e_entry_text_set            (Evas_Object *entry, const char *text);
EAPI const char  *e_entry_text_get            (Evas_Object *entry);
EAPI void         e_entry_clear               (Evas_Object *entry);
EAPI Evas_Object *e_entry_editable_object_get (Evas_Object *entry);

EAPI void         e_entry_password_set        (Evas_Object *entry, int password_mode);
EAPI void         e_entry_min_size_get        (Evas_Object *entry, Evas_Coord *minw, Evas_Coord *minh);
EAPI void         e_entry_focus               (Evas_Object *entry);
EAPI void         e_entry_unfocus             (Evas_Object *entry);
EAPI void         e_entry_enable              (Evas_Object *entry);
EAPI void         e_entry_disable             (Evas_Object *entry);

#endif
//#endif
