/* e_nav_dbus.c -
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

#include "e_nav.h"
#include <assert.h>
#include "e_nav_dbus.h"
#include "e_dbus_proxy.h"

static const char *diversity_ifaces[N_DIVERSITY_DBUS_IFACES] = {
     DBUS_INTERFACE_PROPERTIES,
     "org.openmoko.Diversity",
     "org.openmoko.Diversity.World",
     "org.openmoko.Diversity.Object",
     "org.openmoko.Diversity.Viewport",
     "org.openmoko.Diversity.Bard",
     "org.openmoko.Diversity.Tag",
     "org.openmoko.Diversity.Equipment",
     "org.openmoko.Diversity.Atlas",
     "org.openmoko.Diversity.Sms",
};

struct _Diversity_DBus
{
   char *path;
   E_DBus_Proxy *proxies[N_DIVERSITY_DBUS_IFACES];
};

struct _Diversity_Object
{
   Diversity_DBus dbus;
   double lon, lat;
   double width, height;
};

struct _Diversity_World
{
   Diversity_DBus dbus;
};

struct _Diversity_Equipment
{
   Diversity_DBus dbus;
};

struct _Diversity_Viewport
{
   Diversity_Object obj;
};

struct _Diversity_Bard
{
   Diversity_Object obj;
};

struct _Diversity_Tag
{
   Diversity_Object obj;
};

struct _E_Nav_World
{
   Diversity_DBus dbus;
};

struct _E_Nav_Viewport
{
   E_DBus_Proxy *proxy;
   const char *path;
};

struct _E_Nav_Bard
{
   E_DBus_Proxy *proxy;
   const char *path;
   E_DBus_Proxy *object;
   E_DBus_Proxy *atlas;
   E_DBus_Proxy *gps;
};

static E_DBus_Connection* e_conn=NULL;
static int initialized = -1;

#define DIVERSITY_DBUS_BUS			DBUS_BUS_SESSION
#define DIVERSITY_DBUS_SERVICE			"org.openmoko.Diversity"
#define DIVERSITY_DBUS_PATH			"/org/openmoko/Diversity"
#define DIVERSITY_DBUS_INTERFACE		"org.openmoko.Diversity"

#define DIVERSITY_WORLD_DBUS_PATH 		"/org/openmoko/Diversity/world"
#define DIVERSITY_WORLD_DBUS_INTERFACE		"org.openmoko.Diversity.World"

#define DIVERSITY_VIEWPORT_DBUS_INTERFACE	"org.openmoko.Diversity.Viewport"
#define DIVERSITY_OBJECT_DBUS_INTERFACE		"org.openmoko.Diversity.Object"
#define DIVERSITY_BARD_DBUS_INTERFACE		"org.openmoko.Diversity.Bard"
#define DIVERSITY_TAG_DBUS_INTERFACE		"org.openmoko.Diversity.Tag"

#define DIVERSITY_EQUIPMENT_DBUS_INTERFACE	"org.openmoko.Diversity.Equipment"
#define DIVERSITY_ATLAS_DBUS_INTERFACE		"org.openmoko.Diversity.Atlas"
#define DIVERSITY_SMS_DBUS_INTERFACE		"org.openmoko.Diversity.Sms"

int
e_nav_dbus_init(void)
{
   if (initialized != -1)
     return initialized;

   initialized = 0;

   if (!e_dbus_init())
     return 0;

   e_conn = e_dbus_bus_get(DIVERSITY_DBUS_BUS);
   if (!e_conn)
     {
	e_dbus_shutdown();

	return 0;
     }

   initialized = 1;

   return 1;
}

void
e_nav_dbus_shutdown(void)
{
   if (e_conn)
   {
     e_dbus_connection_close(e_conn);
     e_conn = NULL;

     e_dbus_shutdown();
   }

   initialized = -1;
}

E_DBus_Connection *
e_nav_dbus_connection_get(void)
{
   return e_conn;
}

static Diversity_DBus *
diversity_dbus_new(const char *path, int size)
{
   E_DBus_Connection *connection;
   Diversity_DBus *dbus;

   if (!path || size < sizeof(Diversity_DBus)) return NULL;

   connection = e_nav_dbus_connection_get();
   if (!connection) return NULL;

   dbus = calloc(1, size);
   if (!dbus) return NULL;

   dbus->path = strdup(path);
   if (!dbus->path) {
	free(dbus);

	return NULL;
   }

   return dbus;
}

static void
diversity_dbus_destroy(Diversity_DBus *dbus)
{
   int i;

   for (i = 0; i < N_DIVERSITY_DBUS_IFACES; i++)
     {
	if (dbus->proxies[i])
	  e_dbus_proxy_destroy(dbus->proxies[i]);
     }

   free(dbus->path);
   free(dbus);
}

const char *
diversity_dbus_path_get(Diversity_DBus *dbus)
{
   return dbus->path;
}

E_DBus_Proxy *
diversity_dbus_proxy_get(Diversity_DBus *dbus, Diversity_DBus_IFace iface)
{
   E_DBus_Connection *connection;

   if (dbus->proxies[iface])
     return dbus->proxies[iface];

   connection = e_nav_dbus_connection_get();
   if (!connection) return NULL;

   dbus->proxies[iface] = e_dbus_proxy_new_for_name(connection,
	 					     DIVERSITY_DBUS_SERVICE,
						     dbus->path,
						     diversity_ifaces[iface]);

   return dbus->proxies[iface];
}

void
diversity_dbus_signal_connect(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *signal, E_DBus_Signal_Cb cb_signal, void *data)
{
   E_DBus_Proxy *proxy;

   proxy = diversity_dbus_proxy_get(dbus, iface);
   if (!proxy) return;

   e_dbus_proxy_connect_signal(proxy, signal, cb_signal, data);
}

void
diversity_dbus_signal_disconnect(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *signal, E_DBus_Signal_Cb cb_signal, void *data)
{
   E_DBus_Proxy *proxy;

   proxy = diversity_dbus_proxy_get(dbus, iface);
   if (!proxy) return;

   e_dbus_proxy_disconnect_signal(proxy, signal, cb_signal, data);
}

void
diversity_dbus_property_set(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *prop, void *val)
{
   E_DBus_Proxy *proxy;
   DBusMessage *message, *reply;
   DBusMessageIter iter, subiter;
   char *sig;
   int type;
  
   proxy = diversity_dbus_proxy_get(dbus, iface);
   if (!proxy) return;

   message = e_dbus_proxy_new_method_call(proxy, "Set");

   dbus_message_iter_init_append(message, &iter);

   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &diversity_ifaces[iface]);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &prop);

   sig = DBUS_TYPE_STRING_AS_STRING;
   type = DBUS_TYPE_STRING;

   if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, sig, &subiter))
     return;
   dbus_message_iter_append_basic(&subiter, type, val);
   dbus_message_iter_close_container(&iter, &subiter);

   e_dbus_proxy_call(proxy, message, &reply);

   if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
	printf("failed to set property\n");
   }

   dbus_message_unref(message);
   dbus_message_unref(reply);
}

void *
diversity_dbus_property_get(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *prop)
{
   E_DBus_Proxy *proxy;
   DBusMessage *message, *reply;
   DBusMessageIter iter, subiter;
   int type;
   void *val;
  
   proxy = diversity_dbus_proxy_get(dbus, iface);
   if (!proxy) return NULL;

   message = e_dbus_proxy_new_method_call(proxy, "Get");

   dbus_message_iter_init_append(message, &iter);

   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &diversity_ifaces[iface]);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &prop);

   e_dbus_proxy_call(proxy, message, &reply);

   if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
	printf("failed to get property\n");
   } else {
	dbus_message_iter_init(reply, &iter);
	if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
	     dbus_message_iter_recurse(&iter, &subiter);
	     type = dbus_message_iter_get_arg_type(&subiter);

	     if (dbus_type_is_basic(type)) {
		  dbus_message_iter_get_basic(&subiter, &val);
	     }
	}
   }

   dbus_message_unref(message);
   dbus_message_unref(reply);

   return val;
}

static void
on_geometry_changed(void *data, DBusMessage *msg)
{
   Diversity_Object *obj = data;

   if (!dbus_message_get_args(msg, NULL,
			      DBUS_TYPE_DOUBLE, &obj->lon,
			      DBUS_TYPE_DOUBLE, &obj->lat,
			      DBUS_TYPE_DOUBLE, &obj->width,
			      DBUS_TYPE_DOUBLE, &obj->height,
			      DBUS_TYPE_INVALID))
     return;
}

static Diversity_Object *
diversity_object_new(const char *path, int size)
{
   E_DBus_Connection *connection;
   Diversity_Object *obj;

   if (!path || size < sizeof(Diversity_Object)) return NULL;

   connection = e_nav_dbus_connection_get();
   if (!connection) return NULL;

   obj = (Diversity_Object *) diversity_dbus_new(path, size);
   if (!obj) return NULL;

   diversity_dbus_signal_connect((Diversity_DBus *) obj,
				 DIVERSITY_DBUS_IFACE_OBJECT,
				"GeometryChanged",
				on_geometry_changed,
				obj);

   return obj;
}

static void
diversity_object_destroy(Diversity_Object *obj)
{
   diversity_dbus_destroy((Diversity_DBus *) obj);
}

void
diversity_object_geometry_set(Diversity_Object *obj, double lon, double lat, double width, double height)
{
   E_DBus_Proxy *proxy;
   DBusError error;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) obj, DIVERSITY_DBUS_IFACE_OBJECT);

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "GeometrySet", &error,
				 DBUS_TYPE_DOUBLE, &lon,
				 DBUS_TYPE_DOUBLE, &lat,
				 DBUS_TYPE_DOUBLE, &width,
				 DBUS_TYPE_DOUBLE, &height,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to set geometry: %s\n", error.message);
	dbus_error_free(&error);
     }
}

void
diversity_object_geometry_get(Diversity_Object *obj, double *lon, double *lat, double *width, double *height)
{
	if (lon)
		*lon = obj->lon;

	if (lat)
		*lat = obj->lat;

	if (width)
		*width = obj->width;

	if (height)
		*height = obj->height;
}

Diversity_Viewport *
diversity_world_viewport_add(Diversity_World *world, double lon1, double lat1, double lon2, double lat2)
{
   E_DBus_Proxy *proxy;
   Diversity_Viewport *view;
   DBusError error;
   char *path;
   double tmp;

   if (!world) return NULL;

   if (lon1 > lon2)
     {
	tmp = lon1;
	lon1 = lon2;
	lon2 = tmp;
     }
   if (lat1 > lat2)
     {
	tmp = lat1;
	lat1 = lat2;
	lat2 = tmp;
     }

   if (lon1 < -180.0 || lon2 > 180.0 || lat1 < -90.0 || lat2 > 90.0)
     return NULL;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) world, DIVERSITY_DBUS_IFACE_WORLD);
   if (!proxy)
     return NULL;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "ViewportAdd", &error,
				 DBUS_TYPE_DOUBLE, &lon1,
				 DBUS_TYPE_DOUBLE, &lat1,
				 DBUS_TYPE_DOUBLE, &lon2,
				 DBUS_TYPE_DOUBLE, &lat2,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_OBJECT_PATH, &path,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to add viewport: %s\n", error.message);
	dbus_error_free(&error);

	return NULL;
     }

   view = diversity_viewport_new(path);
   free(path);

   return view;
}

void
diversity_world_viewport_remove(Diversity_World *world, Diversity_Viewport *view)
{
   E_DBus_Proxy *proxy;
   const char *path;

   if (!world || !view) return;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) world, DIVERSITY_DBUS_IFACE_WORLD);
   if (!proxy) return;

   path = diversity_dbus_path_get((Diversity_DBus *) world);
   e_dbus_proxy_simple_call(proxy,
			    "ViewportRemove", NULL,
			    DBUS_TYPE_OBJECT_PATH, &path,
			    DBUS_TYPE_INVALID,
			    DBUS_TYPE_INVALID);

   diversity_viewport_destroy(view);
}

Diversity_Bard *
diversity_world_get_self(Diversity_World *world)
{
   E_DBus_Proxy *proxy;
   Diversity_Bard *self;
   DBusError error;
   char *path;

   if (!world) return NULL;

   proxy = diversity_dbus_proxy_get(&world->dbus, DIVERSITY_DBUS_IFACE_WORLD);
   if (!proxy) return NULL;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "GetSelf", &error,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_OBJECT_PATH, &path,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to get self: %s\n", error.message);
	dbus_error_free(&error);

	return NULL;
     }

   self = diversity_bard_new(path);

   return self;
}

Diversity_Viewport *
diversity_viewport_new(const char *path)
{
   Diversity_Viewport *view;

   view = (Diversity_Viewport *)
      diversity_object_new(path, sizeof(Diversity_Viewport));

   return view;
}

void
diversity_viewport_destroy(Diversity_Viewport *view)
{
   diversity_object_destroy((Diversity_Object *) view);
}

Diversity_Bard *
diversity_bard_new(const char *path)
{
   Diversity_Bard *bard;

   bard = (Diversity_Bard *)
      diversity_object_new(path, sizeof(Diversity_Bard));

   return bard;
}

void
diversity_bard_destroy(Diversity_Bard *bard)
{
   diversity_object_destroy((Diversity_Object *) bard);
}

void
diversity_equipment_config_set(Diversity_Equipment *eqp, const char *key, void *val)
{
   E_DBus_Proxy *proxy;
   DBusMessage *message, *reply;
   DBusMessageIter iter, subiter;
   char *sig;
   int type;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) eqp, DIVERSITY_DBUS_IFACE_EQUIPMENT);
   if (!proxy) return;

   message = e_dbus_proxy_new_method_call(proxy, "SetConfig");

   dbus_message_iter_init_append(message, &iter);

   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &key);

   sig = DBUS_TYPE_STRING_AS_STRING;
   type = DBUS_TYPE_STRING;

   if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, sig, &subiter))
     return;
   dbus_message_iter_append_basic(&subiter, type, val);
   dbus_message_iter_close_container(&iter, &subiter);

   e_dbus_proxy_call(proxy, message, &reply);

   if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR)
	printf("failed to set config\n");

   dbus_message_unref(message);
   dbus_message_unref(reply);
}

void *
diversity_equipment_config_get(Diversity_Equipment *eqp, const char *key)
{
   E_DBus_Proxy *proxy;
   DBusMessage *message, *reply;
   DBusMessageIter iter, subiter;
   int type;
   void *val;
  
   proxy = diversity_dbus_proxy_get((Diversity_DBus *) eqp, DIVERSITY_DBUS_IFACE_EQUIPMENT);
   if (!proxy) return NULL;

   message = e_dbus_proxy_new_method_call(proxy, "GetConfig");

   dbus_message_iter_init_append(message, &iter);

   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &key);

   e_dbus_proxy_call(proxy, message, &reply);

   if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
	printf("failed to get config\n");
   } else {
	dbus_message_iter_init(reply, &iter);
	if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
	     dbus_message_iter_recurse(&iter, &subiter);
	     type = dbus_message_iter_get_arg_type(&subiter);

	     if (dbus_type_is_basic(type)) {
		  dbus_message_iter_get_basic(&subiter, &val);
	     }
	}
   }

   dbus_message_unref(message);
   dbus_message_unref(reply);

   return val;
}

E_Nav_World *
e_nav_world_new(void)
{
   return (E_Nav_World *) diversity_dbus_new(DIVERSITY_WORLD_DBUS_PATH, sizeof(E_Nav_World));
}

void
e_nav_world_destroy(E_Nav_World *world)
{
   if (world)
     diversity_dbus_destroy((Diversity_DBus *) world);
}

E_DBus_Proxy *
e_nav_world_proxy_get(E_Nav_World *world)
{
   if (!world) return NULL;

   return diversity_dbus_proxy_get(&world->dbus, DIVERSITY_DBUS_IFACE_WORLD);
}

E_Nav_Viewport *
e_nav_world_viewport_add(E_Nav_World *world, double lon1, double lat1, double lon2, double lat2)
{
   E_DBus_Proxy *proxy;
   E_Nav_Viewport *view;
   DBusError error;
   char *path;
   double tmp;

   if (!world) return NULL;

   if (lon1 > lon2)
     {
	tmp = lon1;
	lon1 = lon2;
	lon2 = tmp;
     }
   if (lat1 > lat2)
     {
	tmp = lat1;
	lat1 = lat2;
	lat2 = tmp;
     }

   if (lon1 < -180.0 || lon2 > 180.0 || lat1 < -90.0 || lat2 > 90.0)
     return NULL;

   proxy = e_nav_world_proxy_get(world);
   if (!proxy)
     return NULL;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "ViewportAdd", &error,
				 DBUS_TYPE_DOUBLE, &lon1,
				 DBUS_TYPE_DOUBLE, &lat1,
				 DBUS_TYPE_DOUBLE, &lon2,
				 DBUS_TYPE_DOUBLE, &lat2,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_OBJECT_PATH, &path,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to add viewport: %s\n", error.message);
	dbus_error_free(&error);

	return NULL;
     }

   view = e_nav_viewport_new(path);
   free(path);

   return view;
}

void
e_nav_world_viewport_remove(E_Nav_World *world, E_Nav_Viewport *view)
{
   E_DBus_Proxy *proxy;

   if (!world || !view) return;

   proxy = e_nav_world_proxy_get(world);
   if (!proxy) return;

   e_dbus_proxy_simple_call(proxy,
			    "ViewportRemove", NULL,
			    DBUS_TYPE_OBJECT_PATH, &view->path,
			    DBUS_TYPE_INVALID,
			    DBUS_TYPE_INVALID);

   e_nav_viewport_destroy(view);
}

E_Nav_Bard *
e_nav_world_get_self(E_Nav_World *world)
{
   E_DBus_Proxy *proxy;
   E_Nav_Bard *self;
   DBusError error;
   char *path;

   if (!world) return NULL;

   proxy = e_nav_world_proxy_get(world);
   if (!proxy) return NULL;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "GetSelf", &error,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_OBJECT_PATH, &path,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to get self: %s\n", error.message);
	dbus_error_free(&error);

	return NULL;
     }

   self = e_nav_bard_new(path);
   free(path);

   return self;
}

E_Nav_Viewport *
e_nav_viewport_new(const char *path)
{
   E_Nav_Viewport *view;
   E_DBus_Connection *connection;

   if (!path) return NULL;

   connection = e_nav_dbus_connection_get();
   if (!connection) return NULL;

   view = calloc(1, sizeof(E_Nav_Viewport));
   if (!view) return NULL;

   view->proxy = e_dbus_proxy_new_for_name(connection,
					   DIVERSITY_DBUS_SERVICE,
					   path,
					   DIVERSITY_VIEWPORT_DBUS_INTERFACE);
   if (!view->proxy)
     goto fail;

   view->path = e_dbus_proxy_get_path(view->proxy);

   return view;

fail:
   free(view);

   return NULL;
}

void
e_nav_viewport_destroy(E_Nav_Viewport *view)
{
   if (!view) return;

   e_dbus_proxy_destroy(view->proxy);

   free(view);
}

E_Nav_Bard *
e_nav_bard_new(const char *path)
{
   E_Nav_Bard *bard;
   E_DBus_Connection *connection;

   if (!path) return NULL;

   connection = e_nav_dbus_connection_get();
   if (!connection) return NULL;

   bard = calloc(1, sizeof(E_Nav_Bard));
   if (!bard) return NULL;

   bard->proxy = e_dbus_proxy_new_for_name(connection,
					   DIVERSITY_DBUS_SERVICE,
					   path,
					   DIVERSITY_BARD_DBUS_INTERFACE);
   if (!bard->proxy)
     goto fail;

   bard->path = e_dbus_proxy_get_path(bard->proxy);

   return bard;

fail:
   free(bard);

   return NULL;
}

void
e_nav_bard_destroy(E_Nav_Bard *bard)
{
   if (!bard) return;

   e_dbus_proxy_destroy(bard->proxy);
   if (bard->atlas)
      e_dbus_proxy_destroy(bard->atlas);

   free(bard);
}

E_DBus_Proxy *
e_nav_bard_equipment_get(E_Nav_Bard *bard, const char *eqp, const char *interface)
{
   if (strcmp(eqp, "osm") != 0 ||
       strcmp(interface, DIVERSITY_ATLAS_DBUS_INTERFACE) != 0)
     return NULL;

   if (!bard->atlas)
     {
	E_DBus_Connection *connection;
	char *path;

	connection = e_nav_dbus_connection_get();
	if (!connection) return NULL;

	path = malloc(strlen(bard->path) + 1 +
			strlen("equipments") + 1 + strlen(eqp) + 1);

	if (!path) return NULL;

	sprintf(path, "%s/equipments/%s", bard->path, eqp);

	bard->atlas = e_dbus_proxy_new_for_name(connection,
						DIVERSITY_DBUS_SERVICE,
						path,
						DIVERSITY_ATLAS_DBUS_INTERFACE);
	free(path);
     }

   return bard->atlas;
}
