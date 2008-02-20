/* e_nav_dbus.h -
 *
 * Copyright 2007-2008 OpenMoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
 *             Chia-I Wu <olv@openmoko.com>
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

#ifndef E_NAV_DBUS_H 
#define E_NAV_DBUS_H

#include "e_dbus_proxy.h"

typedef struct _E_Nav_World E_Nav_World;
typedef struct _E_Nav_Viewport E_Nav_Viewport;
typedef struct _E_Nav_Bard E_Nav_Bard;

typedef struct _Object_Proxy Object_Proxy;

int                 e_nav_dbus_init(void);
void                e_nav_dbus_shutdown(void);
E_DBus_Connection  *e_nav_dbus_connection_get(void);

E_Nav_World        *e_nav_world_new(void);
void                e_nav_world_destroy(E_Nav_World *world);
E_DBus_Proxy       *e_nav_world_proxy_get(E_Nav_World *world);
E_Nav_Viewport     *e_nav_world_viewport_add(E_Nav_World *world, double lon1, double lat1, double lon2, double lat2);
void                e_nav_world_viewport_remove(E_Nav_World *world, E_Nav_Viewport *view);
E_Nav_Bard         *e_nav_world_get_self(E_Nav_World *world);

E_Nav_Viewport     *e_nav_viewport_new(const char *path);
void                e_nav_viewport_destroy(E_Nav_Viewport *view);

E_Nav_Bard         *e_nav_bard_new(const char *path);
void                e_nav_bard_destroy(E_Nav_Bard *bard);
E_DBus_Proxy       *e_nav_bard_equipment_get(E_Nav_Bard *bard, const char *eqp, const char *interface);

#endif
