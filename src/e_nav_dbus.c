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
#include "e_nav_dbus.h"
#include "e_dbus_proxy.h"

static const char *diversity_ifaces[N_DIVERSITY_DBUS_IFACES] =
{
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
   /* FIXME waste of memory! */
   E_DBus_Proxy *proxies[N_DIVERSITY_DBUS_IFACES];
};

struct _Diversity_Object
{
   Diversity_DBus dbus;
   Diversity_Object_Type type;
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

static E_DBus_Connection *e_conn = NULL;
static int initialized = -1;

#define DIVERSITY_DBUS_BUS			DBUS_BUS_SESSION
#define DIVERSITY_DBUS_SERVICE			"org.openmoko.Diversity"
#define DIVERSITY_DBUS_PATH			"/org/openmoko/Diversity"
#define DIVERSITY_WORLD_DBUS_PATH 		"/org/openmoko/Diversity/world"

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

static const char *
sig_from_type(int type)
{
   const char *sig;

   switch (type)
     {
      case DBUS_TYPE_BYTE:
	 sig = DBUS_TYPE_BYTE_AS_STRING;
	 break;
      case DBUS_TYPE_BOOLEAN:
	 sig = DBUS_TYPE_BOOLEAN_AS_STRING;
	 break;
      case DBUS_TYPE_INT16:
	 sig = DBUS_TYPE_INT16_AS_STRING;
	 break;
      case DBUS_TYPE_UINT16:
	 sig = DBUS_TYPE_UINT16_AS_STRING;
	 break;
      case DBUS_TYPE_INT32:
	 sig = DBUS_TYPE_INT32_AS_STRING;
	 break;
      case DBUS_TYPE_UINT32:
	 sig = DBUS_TYPE_UINT32_AS_STRING;
	 break;
      case DBUS_TYPE_INT64:
	 sig = DBUS_TYPE_INT64_AS_STRING;
	 break;
      case DBUS_TYPE_UINT64:
	 sig = DBUS_TYPE_UINT64_AS_STRING;
	 break;
      case DBUS_TYPE_DOUBLE:
	 sig = DBUS_TYPE_DOUBLE_AS_STRING;
	 break;
      case DBUS_TYPE_STRING:
	 sig = DBUS_TYPE_STRING_AS_STRING;
	 break;
      case DBUS_TYPE_OBJECT_PATH:
	 sig = DBUS_TYPE_OBJECT_PATH_AS_STRING;
	 break;
      default:
	 sig = NULL;
	 break;
     }

   return sig;
}

static int
set_basic_variant(E_DBus_Proxy *proxy, DBusMessage *msg, DBusMessageIter *iter, int type, void *val)
{
   DBusMessage *reply;
   DBusMessageIter subiter;
   DBusError error;
   const char *sig;

   sig = sig_from_type(type);
   if (!sig) return 0;

   if (!dbus_message_iter_open_container(iter,
	    DBUS_TYPE_VARIANT, sig, &subiter))
	return 0;

   dbus_message_iter_append_basic(&subiter, type, val);
   dbus_message_iter_close_container(iter, &subiter);

   if (!e_dbus_proxy_call(proxy, msg, &reply))
     return 0;

   dbus_error_init(&error);
   if (dbus_set_error_from_message(&error, reply))
     {
	printf("%s returns error: %s\n",
	      dbus_message_get_member(msg), error.message);

	dbus_error_free(&error);
	dbus_message_unref(reply);

	return 0;
     }

   dbus_message_unref(reply);

   return 1;
}

static int
get_basic_variant(E_DBus_Proxy *proxy, DBusMessage *msg, void *val)
{
   DBusMessage *reply;
   DBusMessageIter iter, subiter;
   DBusError error;
   int type;

   if (!e_dbus_proxy_call(proxy, msg, &reply))
     return 0;

   dbus_error_init(&error);
   if (dbus_set_error_from_message(&error, reply))
     {
	printf("%s returns error: %s\n",
	      dbus_message_get_member(msg), error.message);

	dbus_error_free(&error);
	dbus_message_unref(reply);

	return 0;
     }

   dbus_message_iter_init(reply, &iter);
   if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT)
     {
	printf("%s returns non-variant\n",
	      dbus_message_get_member(msg));

	dbus_message_unref(reply);

	return 0;
     }

   dbus_message_iter_recurse(&iter, &subiter);
   type = dbus_message_iter_get_arg_type(&subiter);
   if (!dbus_type_is_basic(type))
     {
	printf("%s returns non-basic variant\n",
	      dbus_message_get_member(msg));

	dbus_message_unref(reply);

	return 0;
     }

   dbus_message_iter_get_basic(&subiter, val);

   if (type == DBUS_TYPE_STRING || type == DBUS_TYPE_OBJECT_PATH)
     {
	char *p = *((char **) val);

	if (p)
	  p = strdup(p);

	*((char **) val) = p;
     }

   dbus_message_unref(reply);

   return 1;
}

int
diversity_dbus_property_set(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *prop, int type, void *val)
{
   E_DBus_Proxy *proxy;
   DBusMessage *message;
   DBusMessageIter iter;
   int ret;
  
   proxy = diversity_dbus_proxy_get(dbus, DIVERSITY_DBUS_IFACE_PROPERTIES);
   if (!proxy) return 0;

   message = e_dbus_proxy_new_method_call(proxy, "Set");

   dbus_message_iter_init_append(message, &iter);

   dbus_message_iter_append_basic(&iter,
	 DBUS_TYPE_STRING, &diversity_ifaces[iface]);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &prop);

   ret = set_basic_variant(proxy, message, &iter, type, val);
   dbus_message_unref(message);

   return ret;
}

int
diversity_dbus_property_get(Diversity_DBus *dbus, Diversity_DBus_IFace iface, const char *prop, void *val)
{
   E_DBus_Proxy *proxy;
   DBusMessage *message;
   DBusMessageIter iter;
   int ret;
  
   proxy = diversity_dbus_proxy_get(dbus, DIVERSITY_DBUS_IFACE_PROPERTIES);
   if (!proxy) return 0;

   message = e_dbus_proxy_new_method_call(proxy, "Get");

   dbus_message_iter_init_append(message, &iter);

   dbus_message_iter_append_basic(&iter,
	 DBUS_TYPE_STRING, &diversity_ifaces[iface]);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &prop);

   ret = get_basic_variant(proxy, message, val);
   dbus_message_unref(message);

   return ret;
}

#if 0
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
#endif

static void *
diversity_object_new_with_type(const char *path, Diversity_Object_Type type)
{
   E_DBus_Connection *connection;
   Diversity_Object *obj;
   int size;

   if (!path) return NULL;

   switch (type)
     {
      case DIVERSITY_OBJECT_TYPE_OBJECT:
	 size = sizeof(Diversity_Object);
	 break;
      case DIVERSITY_OBJECT_TYPE_VIEWPORT:
	 size = sizeof(Diversity_Viewport);
	 break;
      case DIVERSITY_OBJECT_TYPE_TAG:
	 size = sizeof(Diversity_Tag);
	 break;
      case DIVERSITY_OBJECT_TYPE_BARD:
	 size = sizeof(Diversity_Bard);
	 break;
      default:
	 printf("unknown type %d\n", type);
	 return NULL;
	 break;
     }

   connection = e_nav_dbus_connection_get();
   if (!connection) return NULL;

   obj = (Diversity_Object *) diversity_dbus_new(path, size);
   if (!obj) return NULL;

   obj->type = type;

#if 0
   diversity_dbus_signal_connect((Diversity_DBus *) obj,
				 DIVERSITY_DBUS_IFACE_OBJECT,
				"GeometryChanged",
				on_geometry_changed,
				obj);
#endif

   return obj;
}

void
diversity_object_destroy(Diversity_Object *obj)
{
   diversity_dbus_destroy((Diversity_DBus *) obj);
}

void *
diversity_object_new(const char *path)
{
   Diversity_Object *obj;
   E_DBus_Proxy *proxy;
   Diversity_Object_Type type;
   DBusError error;

   if (!path)
     return NULL;

   obj = diversity_object_new_with_type(path, DIVERSITY_OBJECT_TYPE_OBJECT);
   if (!obj)
     return NULL;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) obj,
	 	DIVERSITY_DBUS_IFACE_OBJECT);

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "GetType", &error,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_INT32, &type,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to get object type: %s\n", error.message);

	dbus_error_free(&error);

	return NULL;
     }

   if (type == DIVERSITY_OBJECT_TYPE_OBJECT)
     return obj;

   diversity_object_destroy(obj);

   obj = diversity_object_new_with_type(path, type);

   return obj;
}

void
diversity_object_geometry_set(Diversity_Object *obj, double lon, double lat, double width, double height)
{
   E_DBus_Proxy *proxy;
   DBusError error;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) obj,
	 	DIVERSITY_DBUS_IFACE_OBJECT);
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
   E_DBus_Proxy *proxy;
   DBusError error;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) obj,
	 	DIVERSITY_DBUS_IFACE_OBJECT);

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "GeometryGet", &error,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_DOUBLE, lon,
				 DBUS_TYPE_DOUBLE, lat,
				 DBUS_TYPE_DOUBLE, width,
				 DBUS_TYPE_DOUBLE, height,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to get geometry: %s\n", error.message);
	dbus_error_free(&error);
     }
}

Diversity_Object_Type
diversity_object_type_get(Diversity_Object *obj)
{
   if (!obj)
     return DIVERSITY_OBJECT_TYPE_OBJECT;

   return obj->type;
}

Diversity_World *
diversity_world_new(void)
{
   Diversity_World *world;

   world = (Diversity_World *)
      diversity_dbus_new(DIVERSITY_WORLD_DBUS_PATH, sizeof(Diversity_World));

   return world;
}

void
diversity_world_destroy(Diversity_World *world)
{
   if (world)
     diversity_dbus_destroy((Diversity_DBus *) world);
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

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) world,
	 	DIVERSITY_DBUS_IFACE_WORLD);
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

int
diversity_world_viewport_remove(Diversity_World *world, Diversity_Viewport *view)
{
   E_DBus_Proxy *proxy;
   const char *path;
   int ok = 0;

   if (!world || !view) return ok;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) world,
	 	DIVERSITY_DBUS_IFACE_WORLD);
   if (!proxy) return ok;

   path = diversity_dbus_path_get((Diversity_DBus *) world);
   ok = e_dbus_proxy_simple_call(proxy,
			    "ViewportRemove", NULL,
			    DBUS_TYPE_OBJECT_PATH, &path,
			    DBUS_TYPE_INVALID,
			    DBUS_TYPE_INVALID);
   if(ok)
     {
        diversity_viewport_destroy(view);
     }
   
   return ok;
}

Diversity_Tag *
diversity_world_tag_add(Diversity_World *world, double lon, double lat, const char *description)
{
   E_DBus_Proxy *proxy;
   Diversity_Tag *tag;
   DBusError error;
   char *path;

   if (!world) return NULL;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) world,
	 	DIVERSITY_DBUS_IFACE_WORLD);
   if (!proxy)
     return NULL;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "TagAdd", &error,
				 DBUS_TYPE_DOUBLE, &lon,
				 DBUS_TYPE_DOUBLE, &lat,
                                 DBUS_TYPE_STRING, &description,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_OBJECT_PATH, &path,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to add tag: %s\n", error.message);
	dbus_error_free(&error);

	return NULL;
     }

   tag = diversity_tag_new(path);
   free(path);

   return tag;
}

int
diversity_world_tag_remove(Diversity_World *world, Diversity_Tag *tag)
{
   E_DBus_Proxy *proxy;
   const char *path;
   int ok = 0;

   if (!world || !tag) return ok;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) world,
	 	DIVERSITY_DBUS_IFACE_WORLD);
   if (!proxy) return ok;

   path = diversity_dbus_path_get((Diversity_DBus *) tag);
   ok = e_dbus_proxy_simple_call(proxy,
			    "TagRemove", NULL,
			    DBUS_TYPE_OBJECT_PATH, &path,
			    DBUS_TYPE_INVALID,
			    DBUS_TYPE_INVALID);
   if(ok)
     {
        diversity_tag_destroy(tag);
     }

   return ok;
}

Diversity_Bard *
diversity_world_get_self(Diversity_World *world)
{
   E_DBus_Proxy *proxy;
   Diversity_Bard *self;
   DBusError error;
   char *path;

   if (!world) return NULL;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) world,
	 	DIVERSITY_DBUS_IFACE_WORLD);
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
      diversity_object_new_with_type(path, DIVERSITY_OBJECT_TYPE_VIEWPORT);

   return view;
}

void
diversity_viewport_destroy(Diversity_Viewport *view)
{
   diversity_object_destroy((Diversity_Object *) view);
}

void
diversity_viewport_start(Diversity_Viewport *view)
{
   E_DBus_Proxy *proxy;
   DBusError error;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) view,
	 	DIVERSITY_DBUS_IFACE_VIEWPORT);
   if (!proxy) return;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "Start", &error,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to start viewport: %s\n", error.message);
	dbus_error_free(&error);
     }
}

void
diversity_viewport_stop(Diversity_Viewport *view)
{
   E_DBus_Proxy *proxy;
   DBusError error;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) view,
	 	DIVERSITY_DBUS_IFACE_VIEWPORT);
   if (!proxy) return;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "Stop", &error,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to start viewport: %s\n", error.message);
	dbus_error_free(&error);
     }
}

Diversity_Bard *
diversity_bard_new(const char *path)
{
   Diversity_Bard *bard;

   bard = (Diversity_Bard *)
      diversity_object_new_with_type(path, DIVERSITY_OBJECT_TYPE_BARD);

   return bard;
}

void
diversity_bard_destroy(Diversity_Bard *bard)
{
   diversity_object_destroy((Diversity_Object *) bard);
}

Diversity_Equipment *
diversity_bard_equipment_get(Diversity_Bard *bard, const char *eqp_name)
{
   Diversity_Equipment *eqp;
   const char *bpath;
   char *path;

   if (strcmp(eqp_name, "osm") != 0 &&
       strcmp(eqp_name, "nmea") != 0 &&
       strcmp(eqp_name, "phonekit") != 0)
     return NULL;

   bpath = diversity_dbus_path_get((Diversity_DBus *) bard);
   if (!bpath) return NULL;

   path = malloc(strlen(bpath) + 1 +
	 strlen("equipments") + 1 + strlen(eqp_name) + 1);

   if (!path) return NULL;

   sprintf(path, "%s/equipments/%s", bpath, eqp_name);
   eqp = diversity_equipment_new(path);
   free(path);

   return eqp;
}

Diversity_Equipment *
diversity_equipment_new(const char *path)
{
   Diversity_Equipment *eqp;

   eqp = (Diversity_Equipment *)
      diversity_dbus_new(path, sizeof(Diversity_Equipment));

   return eqp;
}

void
diversity_equipment_destroy(Diversity_Equipment *eqp)
{
   diversity_dbus_destroy((Diversity_DBus *) eqp);
}

int
diversity_equipment_config_set(Diversity_Equipment *eqp, const char *key, int type, void *val)
{
   E_DBus_Proxy *proxy;
   DBusMessage *message;
   DBusMessageIter iter;
   int ret;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) eqp,
	 	DIVERSITY_DBUS_IFACE_EQUIPMENT);
   if (!proxy) return 0;

   message = e_dbus_proxy_new_method_call(proxy, "SetConfig");
   if (!message) return 0;

   dbus_message_iter_init_append(message, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &key);

   ret = set_basic_variant(proxy, message, &iter, type, val);
   dbus_message_unref(message);

   return ret;
}

int
diversity_equipment_config_get(Diversity_Equipment *eqp, const char *key, void *val)
{
   E_DBus_Proxy *proxy;
   DBusMessage *message;
   DBusMessageIter iter;
   int ret;
  
   proxy = diversity_dbus_proxy_get((Diversity_DBus *) eqp,
	 	DIVERSITY_DBUS_IFACE_EQUIPMENT);
   if (!proxy) return 0;

   message = e_dbus_proxy_new_method_call(proxy, "GetConfig");

   dbus_message_iter_init_append(message, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &key);

   ret = get_basic_variant(proxy, message, val);

   dbus_message_unref(message);

   return ret;
}

Diversity_Tag *
diversity_tag_new(const char *path)
{
   Diversity_Tag *tag;

   tag = (Diversity_Tag *)
      diversity_object_new_with_type(path, DIVERSITY_OBJECT_TYPE_TAG);

   return tag;
}

void
diversity_tag_destroy(Diversity_Tag *tag)
{
   diversity_object_destroy((Diversity_Object *) tag);
}

int
diversity_tag_prop_set(Diversity_Tag *tag, const char *key, const char *val)
{
   E_DBus_Proxy *proxy;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) tag,
	 	DIVERSITY_DBUS_IFACE_TAG);
   if (!proxy)
     return 0;

   if (!e_dbus_proxy_simple_call(proxy, "Set",
                                NULL,
                                DBUS_TYPE_STRING, &key,
                                DBUS_TYPE_STRING, &val,
                                DBUS_TYPE_INVALID,
                                DBUS_TYPE_INVALID))
     {
        printf("tag object prop set fail!\n");
        return 0;
     }

   return 1;
}

int
diversity_tag_prop_get(Diversity_Tag *tag, const char *key, char **val)
{
   E_DBus_Proxy *proxy;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) tag,
	 	DIVERSITY_DBUS_IFACE_TAG);
   if (!proxy)
     return 0;

   if (!e_dbus_proxy_simple_call(proxy, "Get",
                                NULL,
                                DBUS_TYPE_STRING, &key,
                                DBUS_TYPE_INVALID,
                                DBUS_TYPE_STRING, val,
                                DBUS_TYPE_INVALID))
     {
        printf("tag object prop get fail!\n");
        return 0;
     }

   return 1;
}

void
diversity_sms_send(Diversity_Sms *sms, const char *number, const char *message, int ask_ds)
{
   E_DBus_Proxy *proxy;
   DBusError error;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) sms,
	 	DIVERSITY_DBUS_IFACE_SMS);
   if (!proxy) return;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "Send", &error,
                                 DBUS_TYPE_STRING, &number,
                                 DBUS_TYPE_STRING, &message,
                                 DBUS_TYPE_BOOLEAN, &ask_ds,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to send SMS: %s | %s\n", error.name, error.message);
	dbus_error_free(&error);
     }
}

void
diversity_sms_tag_share(Diversity_Sms *sms, const char *self, const char *tag)
{
   E_DBus_Proxy *proxy;
   DBusError error;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) sms,
	 	DIVERSITY_DBUS_IFACE_SMS);
   if (!proxy) return;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "ShareTag", &error,
                                 DBUS_TYPE_OBJECT_PATH, &self,
                                 DBUS_TYPE_OBJECT_PATH, &tag,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to Tas Share: %s | %s\n", error.name, error.message);
	dbus_error_free(&error);
     }
}
