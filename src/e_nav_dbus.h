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

typedef struct _Diversity_DBus Diversity_DBus;
typedef struct _Diversity_Object Diversity_Object;

typedef struct _Diversity_World Diversity_World;
typedef struct _Diversity_Equipment Diversity_Equipment;

typedef struct _Diversity_Viewport Diversity_Viewport;
typedef struct _Diversity_Bard Diversity_Bard;
typedef struct _Diversity_Tag Diversity_Tag;

typedef enum {
     DIVERSITY_DBUS_IFACE_PROPERTIES,
     DIVERSITY_DBUS_IFACE_DIVERSITY,
     DIVERSITY_DBUS_IFACE_WORLD,
     DIVERSITY_DBUS_IFACE_OBJECT,
     DIVERSITY_DBUS_IFACE_VIEWPORT,
     DIVERSITY_DBUS_IFACE_BARD,
     DIVERSITY_DBUS_IFACE_TAG,
     DIVERSITY_DBUS_IFACE_EQUIPMENT,
     DIVERSITY_DBUS_IFACE_ATLAS,
     DIVERSITY_DBUS_IFACE_SMS,
     N_DIVERSITY_DBUS_IFACES
} Diversity_DBus_IFace;

typedef struct _Object_Proxy Object_Proxy;

int                 e_nav_dbus_init(void);
void                e_nav_dbus_shutdown(void);
E_DBus_Connection  *e_nav_dbus_connection_get(void);

const char         *diversity_dbus_path_get(Diversity_DBus *dbus);
E_DBus_Proxy       *diversity_dbus_proxy_get(Diversity_DBus *dbus, Diversity_DBus_IFace iface);
void                diversity_dbus_signal_connect(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *signal, E_DBus_Signal_Cb cb_signal, void *data);
void                diversity_dbus_signal_disconnect(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *signal, E_DBus_Signal_Cb cb_signal, void *data);
void                diversity_dbus_property_set(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *prop, void *val);
void               *diversity_dbus_property_get(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *prop);

void                diversity_object_geometry_set(Diversity_Object *obj, double lon, double lat, double width, double height);
void                diversity_object_geometry_get(Diversity_Object *obj, double *lon, double *lat, double *width, double *height);

Diversity_World    *diversity_world_new(void);
void                diversity_world_destroy(Diversity_World *world);
Diversity_Viewport *diversity_world_viewport_add(Diversity_World *world, double lon1, double lat1, double lon2, double lat2);
void                diversity_world_viewport_remove(Diversity_World *world, Diversity_Viewport *view);
Diversity_Bard     *diversity_world_get_self(Diversity_World *world);

Diversity_Equipment *diversity_equipment_new(const char *path);
void                diversity_equipment_destroy(Diversity_Equipment *eqp);
void                diversity_equipment_config_set(Diversity_Equipment *eqp, const char *key, void *val);
void               *diversity_equipment_config_get(Diversity_Equipment *eqp, const char *key);

Diversity_Viewport *diversity_viewport_new(const char *path);
void                diversity_viewport_destroy(Diversity_Viewport *view);

Diversity_Bard     *diversity_bard_new(const char *path);
void                diversity_bard_destroy(Diversity_Bard *bard);
Diversity_Equipment *diversity_bard_equipment_get(Diversity_Bard *bard, const char *eqp_name);

#endif
