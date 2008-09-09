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

typedef struct _E_Nav_DBus_Batch_Call E_Nav_DBus_Batch_Call;

struct _E_Nav_DBus_Batch_Call {
     E_DBus_Proxy *proxy;
     E_DBus_Proxy_Call *id;
     DBusMessage *reply;
};

struct _E_Nav_DBus_Batch {
     int call_total;
     int timeout;
     double hard_timeout;
     Ecore_Timer *hard_timer;
     void (*cb)(void *data, E_Nav_DBus_Batch *bat);
     void *cb_data;

     int ignore_notify;
     int call_ended;
     int call_replied;

     E_Nav_DBus_Batch_Call *calls;
};

static const char *diversity_ifaces[N_DIVERSITY_DBUS_IFACES] =
{
   DBUS_INTERFACE_PROPERTIES,
   "org.openmoko.Diversity",
   "org.openmoko.Diversity.World",
   "org.openmoko.Diversity.Object",
   "org.openmoko.Diversity.Viewport",
   "org.openmoko.Diversity.Ap",
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
   void *user_data;
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

struct _Diversity_Ap
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

static struct {
     int initialized;

     E_DBus_Connection *e_conn;

     E_DBus_Proxy *alive_proxy;
     void (*alive_notify)(void * data);
     void *alive_data;
} ddata = { -1, 0, };

#define DIVERSITY_DBUS_BUS			DBUS_BUS_SESSION
#define DIVERSITY_DBUS_SERVICE			"org.openmoko.Diversity"
#define DIVERSITY_DBUS_PATH			"/org/openmoko/Diversity"
#define DIVERSITY_WORLD_DBUS_PATH 		"/org/openmoko/Diversity/world"

static void
alive_check(void *data, DBusMessage *message)
{
   const char *name, *old_owner, *new_owner;

   if (!dbus_message_get_args(message, NULL,
			      DBUS_TYPE_STRING, &name,
			      DBUS_TYPE_STRING, &old_owner,
			      DBUS_TYPE_STRING, &new_owner,
			      DBUS_TYPE_INVALID))
     return;

   if (*new_owner == '\0' && strcmp(name, DIVERSITY_DBUS_SERVICE) == 0)
     ddata.alive_notify(ddata.alive_data);
}

int
e_nav_dbus_init(void (*notify)(void *data), void *data)
{
   if (ddata.initialized != -1)
     return ddata.initialized;

   ddata.initialized = 0;

   if (!e_dbus_init())
     return 0;

   ddata.e_conn = e_dbus_bus_get(DIVERSITY_DBUS_BUS);
   if (!ddata.e_conn)
     {
	e_dbus_shutdown();

	return 0;
     }

   if (notify)
     {
	ddata.alive_proxy = e_dbus_proxy_new_for_name(ddata.e_conn,
	      DBUS_SERVICE_DBUS,
	      DBUS_PATH_DBUS,
	      DBUS_INTERFACE_DBUS);

	e_dbus_proxy_connect_signal(ddata.alive_proxy, "NameOwnerChanged", alive_check, NULL);

	ddata.alive_notify = notify;
	ddata.alive_data = data;
     }

   ddata.initialized = 1;

   return 1;
}

void
e_nav_dbus_shutdown(void)
{
   if (ddata.alive_proxy)
     {
	e_dbus_proxy_destroy(ddata.alive_proxy);
	ddata.alive_proxy = NULL;
     }

   if (ddata.e_conn)
     {
	e_dbus_connection_close(ddata.e_conn);
	ddata.e_conn = NULL;

	e_dbus_shutdown();
     }

   ddata.initialized = -1;
}

E_DBus_Connection *
e_nav_dbus_connection_get(void)
{
   return ddata.e_conn;
}

static void
batch_call_end(E_Nav_DBus_Batch *bat, int id, int cancel)
{
   E_Nav_DBus_Batch_Call *call = &bat->calls[id];

   if (call->id)
     {
	if (cancel)
	  {
	     e_dbus_proxy_cancel_call(call->proxy, call->id);
	  }
	else
	  {
	     e_dbus_proxy_end_call(call->proxy, call->id, &call->reply);
	     if (call->reply)
	       bat->call_replied++;
	  }

	call->id = NULL;
     }

   bat->call_ended++;

   if (bat->call_ended > bat->call_total)
     {
	printf("more calls ended than there are: %d != %d\n",
	      bat->call_ended, bat->call_total);

	bat->call_ended = bat->call_total;
     }
}

static void
batch_cancel(E_Nav_DBus_Batch *bat, int do_cb)
{
   int i;

   bat->ignore_notify = 1;

   for (i = 0; i < bat->call_total; i++)
     {
	E_Nav_DBus_Batch_Call *call = &bat->calls[i];

	/* calls haven't begun or returned */
	if (!call->proxy || call->id)
	  batch_call_end(bat, i, 1);
     }

   if (bat->call_ended != bat->call_total)
     printf("%d un-ended calls after cancellation\n",
	   bat->call_total - bat->call_ended);

   if (bat->hard_timer)
     {
	ecore_timer_del(bat->hard_timer);
	bat->hard_timer = NULL;
     }

   if (do_cb)
     bat->cb(bat->cb_data, bat);
}

static int
on_batch_hard_timeout(void *data)
{
   E_Nav_DBus_Batch *bat = data;

   printf("batch %p hard timeout\n", bat);

   bat->hard_timer = NULL;
   batch_cancel(bat, 1);

   return 0;
}

static void
batch_reset(E_Nav_DBus_Batch *bat, int reinit)
{
   int i;

   batch_cancel(bat, 0);

   for (i = 0; i < bat->call_total; i++)
     {
	E_Nav_DBus_Batch_Call *call = &bat->calls[i];

	if (call->reply)
	  dbus_message_unref(call->reply);
     }

   if (!reinit)
     return;

   if (bat->hard_timeout > 0.0)
     bat->hard_timer = ecore_timer_add(bat->hard_timeout,
	   on_batch_hard_timeout, bat);
   else
     bat->hard_timer = NULL;

   bat->ignore_notify = 0;
   bat->call_ended = 0;
   bat->call_replied = 0;

   memset(bat->calls, 0, bat->call_total * sizeof(*bat->calls));
}

E_Nav_DBus_Batch *
e_nav_dbus_batch_new(int num_calls, double timeout, double hard_timeout, void (*cb)(void *data, E_Nav_DBus_Batch *bat), void *cb_data)
{
   E_Nav_DBus_Batch *bat;

   bat = malloc(sizeof(*bat));
   if (!bat)
     return NULL;

   bat->calls = calloc(num_calls, sizeof(*bat->calls));
   if (!bat->calls)
     {
	free(bat);

	return NULL;
     }

   bat->call_total = num_calls;

   if (hard_timeout < 0.0)
     hard_timeout = 0.0;

   if (timeout > hard_timeout)
     timeout = hard_timeout;
   else if (timeout < 0.0)
     timeout = 0.0;

   bat->hard_timeout = hard_timeout;

   if (hard_timeout > 0.0)
     bat->hard_timer = ecore_timer_add(hard_timeout,
	   on_batch_hard_timeout, bat);
   else
     bat->hard_timer = NULL;

   if (timeout > 0.0)
     bat->timeout = timeout * 1000;
   else
     bat->timeout = -1;

   bat->cb = cb;
   bat->cb_data = cb_data;

   bat->ignore_notify = 0;
   bat->call_ended = 0;
   bat->call_replied = 0;

   return bat;
}

void
e_nav_dbus_batch_reset(E_Nav_DBus_Batch *bat, void *cb_data)
{
   batch_reset(bat, 1);
   bat->cb_data = cb_data;
}

void
e_nav_dbus_batch_destroy(E_Nav_DBus_Batch *bat)
{
   batch_reset(bat, 0);

   free(bat->calls);
   free(bat);
}

static void
on_batch_reply(void *user_data, E_DBus_Proxy *proxy, E_DBus_Proxy_Call *call_id)
{
   E_Nav_DBus_Batch *bat = user_data;
   E_Nav_DBus_Batch_Call *call;
   int i;

   if (bat->ignore_notify)
     return;

   for (i = 0; i < bat->call_total; i++)
     {
	call = &bat->calls[i];

	if (call->id == call_id)
	  break;
     }

   if (i >= bat->call_total)
     {
	printf("strayed call %p %p\n", proxy, call_id);

	return;
     }

   if (call->proxy != proxy)
     {
	printf("expect proxy %p, not %p\n", call->proxy, proxy);

	return;
     }

   batch_call_end(bat, i, 0);

   if (bat->call_ended == bat->call_total)
     bat->cb(bat->cb_data, bat);
}

int
e_nav_dbus_batch_call_begin(E_Nav_DBus_Batch *bat, int id, E_DBus_Proxy *proxy, DBusMessage *message)
{
   E_Nav_DBus_Batch_Call *call;
   E_DBus_Proxy_Call *call_id;

   if (id < 0 || id >= bat->call_total || !proxy || !message)
     return 0;

   call = &bat->calls[id];

   call_id = e_dbus_proxy_begin_call_with_timeout(proxy, message,
	 on_batch_reply, bat, NULL, bat->timeout);

   call->proxy = proxy;
   call->id = call_id;

   if (!call_id)
     batch_call_end(bat, id, 0);

   return 1;
}

void
e_nav_dbus_batch_block(E_Nav_DBus_Batch *bat)
{
   int i;

   bat->ignore_notify = 1;

   for (i = 0; i < bat->call_total; i++)
     {
	E_Nav_DBus_Batch_Call *call = &bat->calls[i];

	/* calls haven't begun or returned */
	if (!call->proxy || call->id)
	  batch_call_end(bat, i, 0);
     }

   if (bat->call_ended != bat->call_total)
     printf("%d un-ended calls after block\n",
	   bat->call_total - bat->call_ended);

   bat->cb(bat->cb_data, bat);
}

void
e_nav_dbus_batch_cancel(E_Nav_DBus_Batch *bat)
{
   batch_cancel(bat, 0);
}

int
e_nav_dbus_batch_replied_get(E_Nav_DBus_Batch *bat)
{
   return bat->call_replied;
}

DBusMessage *e_nav_dbus_batch_reply_get(E_Nav_DBus_Batch *bat, int id)
{
   if (id < 0 || id >= bat->call_total)
     return NULL;

   return bat->calls[id].reply;
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
      case DIVERSITY_OBJECT_TYPE_AP:
	 size = sizeof(Diversity_Ap);
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

   return obj;
}

void
diversity_object_destroy(Diversity_Object *obj)
{
   switch (obj->type)
     {
     case DIVERSITY_OBJECT_TYPE_OBJECT:
     default:
	diversity_dbus_destroy((Diversity_DBus *) obj);
	break;
     case DIVERSITY_OBJECT_TYPE_VIEWPORT:
	diversity_viewport_destroy((Diversity_Viewport *) obj);
	break;
     case DIVERSITY_OBJECT_TYPE_TAG:
	diversity_tag_destroy((Diversity_Tag *) obj);
	break;
     case DIVERSITY_OBJECT_TYPE_BARD:
	diversity_bard_destroy((Diversity_Bard *) obj);
	break;
     case DIVERSITY_OBJECT_TYPE_AP:
	diversity_ap_destroy((Diversity_Ap *) obj);
	break;
     }
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

void
diversity_object_data_set(Diversity_Object *obj, void *data)
{
   obj->user_data = data;
}

void *
diversity_object_data_get(Diversity_Object *obj)
{
   return obj->user_data;
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

   path = diversity_dbus_path_get((Diversity_DBus *) view);
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

char *
diversity_world_tag_add(Diversity_World *world, double lon, double lat, const char *description)
{
   E_DBus_Proxy *proxy;
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

   return path;
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
   free(path);

   return self;
}

int
diversity_world_snapshot(Diversity_World *world)
{
   E_DBus_Proxy *proxy;

   if (!world) return 0;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) world,
	 	DIVERSITY_DBUS_IFACE_WORLD);
   if (!proxy) return 0;

   return e_dbus_proxy_simple_call(proxy,
				   "Snapshot", NULL,
				   DBUS_TYPE_INVALID,
				   DBUS_TYPE_INVALID);
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
   view->obj.type = DIVERSITY_OBJECT_TYPE_OBJECT;
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

void
diversity_viewport_rule_set(Diversity_Viewport *view, Diversity_Object_Type type, unsigned int mask, unsigned value)
{
   E_DBus_Proxy *proxy;
   DBusError error;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) view,
	 	DIVERSITY_DBUS_IFACE_VIEWPORT);
   if (!proxy) return;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "SetRule", &error,
				 DBUS_TYPE_INT32, &type,
				 DBUS_TYPE_UINT32, &mask,
				 DBUS_TYPE_UINT32, &value,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to set viewport rule: %s\n", error.message);
	dbus_error_free(&error);
     }
}

char **
diversity_viewport_objects_list(Diversity_Viewport *view)
{
   E_DBus_Proxy *proxy;
   DBusError error;
   char **obj_pathes;
   int count;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) view,
	 	DIVERSITY_DBUS_IFACE_VIEWPORT);
   if (!proxy) return NULL;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "ListObjects", &error,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH,
				 &obj_pathes, &count,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to list viewport objects: %s\n", error.message);
	dbus_error_free(&error);

	obj_pathes = NULL;
     }

   return obj_pathes;
}

Diversity_Ap *
diversity_ap_new(const char *path)
{
   Diversity_Ap *ap;

   ap = (Diversity_Ap *)
      diversity_object_new_with_type(path, DIVERSITY_OBJECT_TYPE_AP);

   return ap;
}

void
diversity_ap_destroy(Diversity_Ap *ap)
{
   ap->obj.type = DIVERSITY_OBJECT_TYPE_OBJECT;
   diversity_object_destroy((Diversity_Object *) ap);
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
   bard->obj.type = DIVERSITY_OBJECT_TYPE_OBJECT;
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
       strcmp(eqp_name, "phonekit") != 0 &&
       strcmp(eqp_name, "qtopia") )
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

int
diversity_bard_prop_set(Diversity_Bard *bard, const char *key, const char *val)
{
   return diversity_dbus_property_set((Diversity_DBus *) bard, DIVERSITY_DBUS_IFACE_BARD, key, DBUS_TYPE_STRING, (void *)val);
}

int
diversity_bard_prop_get(Diversity_Bard *bard, const char *key, char **val)
{
   return diversity_dbus_property_get((Diversity_DBus *) bard, DIVERSITY_DBUS_IFACE_BARD, key, val);
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
   tag->obj.type = DIVERSITY_OBJECT_TYPE_OBJECT;
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

int
diversity_sms_tag_send(Diversity_Sms *sms, const char *number, Diversity_Tag *tag)
{
   E_DBus_Proxy *proxy;
   DBusError error;
   const char *tag_path;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) sms,
					DIVERSITY_DBUS_IFACE_SMS);
   if (!proxy) return FALSE;

   tag_path = diversity_dbus_path_get((Diversity_DBus *) tag);
   if (!tag_path) return FALSE;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "SendTag", &error,
				 DBUS_TYPE_STRING, &number,
				 DBUS_TYPE_OBJECT_PATH, &tag_path,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to send tag: %s | %s\n", error.name, error.message);
	dbus_error_free(&error);

	return FALSE;
     }

   return TRUE;
}

int
diversity_sms_tag_share(Diversity_Sms *sms, Diversity_Bard *bard, Diversity_Tag *tag)
{
   E_DBus_Proxy *proxy;
   DBusError error;
   const char *bard_path;
   const char *tag_path;

   proxy = diversity_dbus_proxy_get((Diversity_DBus *) sms,
	 	DIVERSITY_DBUS_IFACE_SMS);
   if (!proxy) return FALSE;

   bard_path = diversity_dbus_path_get((Diversity_DBus *)bard);
   tag_path = diversity_dbus_path_get((Diversity_DBus *)tag);
   if(!bard_path || !tag_path) return FALSE;

   dbus_error_init(&error);
   if (!e_dbus_proxy_simple_call(proxy,
				 "ShareTag", &error,
                                 DBUS_TYPE_OBJECT_PATH, &bard_path,
                                 DBUS_TYPE_OBJECT_PATH, &tag_path,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_INVALID))
     {
	printf("failed to Tas Share: %s | %s\n", error.name, error.message);
	dbus_error_free(&error);
        return FALSE;
     }
   return TRUE;
}
