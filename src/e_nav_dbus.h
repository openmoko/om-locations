/* e_nav_dbus.h -
 *
 * Copyright 2007-2008 Openmoko, Inc.
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

typedef struct _E_Nav_DBus_Batch E_Nav_DBus_Batch;

typedef struct _Diversity_DBus Diversity_DBus;
typedef struct _Diversity_Object Diversity_Object;

typedef struct _Diversity_World Diversity_World;
typedef struct _Diversity_Equipment Diversity_Equipment;

typedef struct _Diversity_Viewport Diversity_Viewport;
typedef struct _Diversity_Ap Diversity_Ap;
typedef struct _Diversity_Bard Diversity_Bard;
typedef struct _Diversity_Tag Diversity_Tag;
typedef struct _Diversity_Sms Diversity_Sms;
typedef struct _Diversity_Rae Diversity_Rae;

typedef enum {
     DIVERSITY_DBUS_IFACE_PROPERTIES,
     DIVERSITY_DBUS_IFACE_DIVERSITY,
     DIVERSITY_DBUS_IFACE_WORLD,
     DIVERSITY_DBUS_IFACE_OBJECT,
     DIVERSITY_DBUS_IFACE_VIEWPORT,
     DIVERSITY_DBUS_IFACE_AP,
     DIVERSITY_DBUS_IFACE_BARD,
     DIVERSITY_DBUS_IFACE_TAG,
     DIVERSITY_DBUS_IFACE_EQUIPMENT,
     DIVERSITY_DBUS_IFACE_ATLAS,
     DIVERSITY_DBUS_IFACE_SMS,
     DIVERSITY_DBUS_IFACE_RAE,
     N_DIVERSITY_DBUS_IFACES
} Diversity_DBus_IFace;

typedef enum {
     DIVERSITY_OBJECT_TYPE_OBJECT,
     DIVERSITY_OBJECT_TYPE_VIEWPORT,
     DIVERSITY_OBJECT_TYPE_TAG,
     DIVERSITY_OBJECT_TYPE_BARD,
     DIVERSITY_OBJECT_TYPE_AP,
     DIVERSITY_OBJECT_TYPE_MAP,
     N_DIVERSITY_OBJECT_TYPES,
} Diversity_Object_Type;

typedef enum {
     DIVERSITY_OBJECT_ACCURACY_NONE,
     DIVERSITY_OBJECT_ACCURACY_DERIVED,
     DIVERSITY_OBJECT_ACCURACY_DIRECT, /* from GPS, and etc. */
     DIVERSITY_OBJECT_ACCURACY_EXACT,
}Diversity_Object_Accuracy;

int                 e_nav_dbus_init(void (*notify)(void *data), void *data);
void                e_nav_dbus_shutdown(void);
E_DBus_Connection  *e_nav_dbus_connection_get(void);

E_Nav_DBus_Batch   *e_nav_dbus_batch_new(int num_calls, double timeout, double hard_timeout, void (*cb)(void *data, E_Nav_DBus_Batch *bat), void *cb_data);
void                e_nav_dbus_batch_reset(E_Nav_DBus_Batch *bat, void *cb_data);
void                e_nav_dbus_batch_destroy(E_Nav_DBus_Batch *bat);
int                 e_nav_dbus_batch_call_begin(E_Nav_DBus_Batch *bat, int id, E_DBus_Proxy *proxy, DBusMessage *message);
void                e_nav_dbus_batch_block(E_Nav_DBus_Batch *bat);
void                e_nav_dbus_batch_cancel(E_Nav_DBus_Batch *bat);
int                 e_nav_dbus_batch_replied_get(E_Nav_DBus_Batch *bat);
DBusMessage        *e_nav_dbus_batch_reply_get(E_Nav_DBus_Batch *bat, int id);

const char         *diversity_dbus_path_get(Diversity_DBus *dbus);
E_DBus_Proxy       *diversity_dbus_proxy_get(Diversity_DBus *dbus, Diversity_DBus_IFace iface);
void                diversity_dbus_signal_connect(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *signal, E_DBus_Signal_Cb cb_signal, void *data);
void                diversity_dbus_signal_disconnect(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *signal, E_DBus_Signal_Cb cb_signal, void *data);
int                 diversity_dbus_property_set(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *prop, int type, void *val);
int                 diversity_dbus_property_get(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *prop, void *val);

void               *diversity_object_new(const char *path);
void                diversity_object_destroy(Diversity_Object *obj);
void                diversity_object_geometry_set(Diversity_Object *obj, double lon, double lat, double width, double height);
void                diversity_object_geometry_get(Diversity_Object *obj, double *lon, double *lat, double *width, double *height);
Diversity_Object_Type diversity_object_type_get(Diversity_Object *obj);
void                diversity_object_data_set(Diversity_Object *obj, void *data);
void               *diversity_object_data_get(Diversity_Object *obj);

Diversity_World    *diversity_world_new(void);
void                diversity_world_destroy(Diversity_World *world);
Diversity_Viewport *diversity_world_viewport_add(Diversity_World *world, double lon1, double lat1, double lon2, double lat2);
int                 diversity_world_viewport_remove(Diversity_World *world, Diversity_Viewport *view);
char               *diversity_world_tag_add(Diversity_World *world, double lon, double lat, const char *description);
int                 diversity_world_tag_remove(Diversity_World *world, Diversity_Tag *tag);
Diversity_Bard     *diversity_world_get_self(Diversity_World *world);
int                 diversity_world_snapshot(Diversity_World *world);

Diversity_Equipment *diversity_equipment_new(const char *path);
void                diversity_equipment_destroy(Diversity_Equipment *eqp);
int                 diversity_equipment_config_set(Diversity_Equipment *eqp, const char *key, int type, void *val);
int                 diversity_equipment_config_get(Diversity_Equipment *eqp, const char *key, void *val);

Diversity_Viewport *diversity_viewport_new(const char *path);
void                diversity_viewport_destroy(Diversity_Viewport *view);
void                diversity_viewport_start(Diversity_Viewport *view);
void                diversity_viewport_stop(Diversity_Viewport *view);
void                diversity_viewport_rule_set(Diversity_Viewport *view, Diversity_Object_Type type, unsigned int mask, unsigned value);

/* the returned value should be freed with dbus_free_string_array */
char **             diversity_viewport_objects_list(Diversity_Viewport *view);

Diversity_Ap       *diversity_ap_new(const char *path);
void                diversity_ap_destroy(Diversity_Ap *ap);

Diversity_Bard     *diversity_bard_new(const char *path);
void                diversity_bard_destroy(Diversity_Bard *bard);
Diversity_Equipment *diversity_bard_equipment_get(Diversity_Bard *bard, const char *eqp_name);
int                 diversity_bard_prop_set(Diversity_Bard *bard, const char *key, const char *val);
int                 diversity_bard_prop_get(Diversity_Bard *bard, const char *key, char **val);

Diversity_Tag      *diversity_tag_new(const char *path);
void                diversity_tag_destroy(Diversity_Tag *tag);
int                 diversity_tag_prop_set(Diversity_Tag *tag, const char *key, const char *val);
int                 diversity_tag_prop_get(Diversity_Tag *tag, const char *key, char **val);

int                 diversity_sms_tag_send(Diversity_Sms *sms, const char *number, Diversity_Tag *tag);
int                 diversity_sms_tag_share(Diversity_Sms *sms, Diversity_Bard *bard, Diversity_Tag *tag);

unsigned int        diversity_rae_request_login(Diversity_Rae *rae, const char *username, const char *password);
unsigned int        diversity_rae_request_query(Diversity_Rae *rae, double lon, double lat, double radius);
unsigned int        diversity_rae_request_report(Diversity_Rae *rae, Diversity_Ap *ap);

#endif
