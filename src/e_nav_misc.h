/* e_nav_misc.h -
 *
 * Copyright 2007-2008 OpenMoko, Inc.
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

#ifndef E_NAV_MISC_H
#define E_NAV_MISC_H

#include <X11/Xlib.h>
#include <Ecore_X.h>
#include "Ecore_X_Atoms.h"

typedef enum {
        MTPRemoteNone = 0,
        MTPRemoteShow,
        MTPRemoteHide,
        MTPRemoteToggle,
} MTPRemoteOperation;


void            e_misc_keyboard_launch();
void            e_misc_keyboard_hide();
int             e_misc_keyboard_status_get();

#endif
