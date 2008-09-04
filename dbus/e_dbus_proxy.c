/* e_dbus_proxy.c - E_DBus_Proxy
 *
 * Copyright 2007-2008 OpenMoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
 *             Chia-I Wu <olv@openmoko.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Inspired by DBusGProxy.
 */

#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include <E_DBus.h>
#include <dbus/dbus.h>
#include "e_dbus_proxy.h"

#include <Ecore.h>
#include <Ecore_Data.h>

#ifdef MERGED_UPSTREAM
#include "e_dbus_private.h"
#else
/* XXX */
struct E_DBus_Connection
{
  DBusBusType shared_type;
  DBusConnection *conn;
};
#endif

#ifndef DEBUG
#define DEBUG
#endif

typedef struct E_DBus_Proxy_Manager E_DBus_Proxy_Manager;

struct E_DBus_Proxy
{
  E_DBus_Proxy_Manager *manager;
  char *name;
  char *path;
  char *interface;

  Ecore_Hash *pending_calls;
  /*
   * Serial numbers are used as the keys to the pending_calls.  We cannot rely
   * on DBusPendingCall to be unique since it is from libdbus.
   *
   * If E_DBus_Pending_Notify is used as the values of pending_calls, we do not
   * need the serial number for a unique key.  But in that case, we need a
   * E_DBus_Pending_Notify for every call, and this slows down
   * e_dbus_proxy_call.
   */
  unsigned int serial_number;

  Ecore_List *signal_handlers;

  E_DBus_Proxy_Call *name_call;
  unsigned int for_owner:1;
  unsigned int associated:1;
};

typedef struct
{
  E_DBus_Signal_Handler *sh;

  char *sig_s;
  void *sig_c;
  void *sig_d;
} E_DBus_Proxy_Signal_Signature;

struct E_DBus_Proxy_Manager
{
  E_DBus_Connection *e_connection;
  DBusConnection *connection;
};

static int e_dbus_proxy_manager_slot = -1;

/* XXX do we need a manager? for proxy association? */
static E_DBus_Proxy_Manager *
e_dbus_proxy_manager_get(E_DBus_Connection *connection)
{
  E_DBus_Proxy_Manager *manager;

  dbus_connection_allocate_data_slot(&e_dbus_proxy_manager_slot);
  if (e_dbus_proxy_manager_slot < 0)
    return NULL;

  manager = dbus_connection_get_data(connection->conn, e_dbus_proxy_manager_slot);
  if (manager)
    return manager;

  manager = calloc(1, sizeof(E_DBus_Proxy_Manager));
  if (!manager)
    goto fail;

  manager->e_connection = connection;
  manager->connection = connection->conn;

  if (!dbus_connection_set_data(connection->conn, e_dbus_proxy_manager_slot, manager, free))
    goto fail;

  return manager;

fail:
  if (manager)
    free(manager);

  dbus_connection_free_data_slot(&e_dbus_proxy_manager_slot);

  return NULL;
}

static void
e_dbus_proxy_manager_unref(E_DBus_Proxy_Manager *manager)
{
  dbus_connection_free_data_slot(&e_dbus_proxy_manager_slot);
}

static void
e_dbus_proxy_manager_unregister(E_DBus_Proxy_Manager *manager, E_DBus_Proxy *proxy)
{
}

static void
e_dbus_proxy_manager_register(E_DBus_Proxy_Manager *manager, E_DBus_Proxy *proxy)
{
}

static E_DBus_Proxy *
e_dbus_proxy_new(E_DBus_Connection *connection, const char *name, const char *path_name, const char *interface_name)
{
  E_DBus_Proxy *proxy;

  if (!connection || !path_name || !interface_name)
    return NULL;

  proxy = calloc(1, sizeof(E_DBus_Proxy));
  if (!proxy)
    return NULL;

  proxy->pending_calls = ecore_hash_new(ecore_direct_hash, NULL);
  if (!proxy->pending_calls)
    goto fail;
  ecore_hash_free_value_cb_set(proxy->pending_calls, (Ecore_Free_Cb) dbus_pending_call_unref);

  proxy->signal_handlers = ecore_list_new();
  if (!proxy->signal_handlers)
    goto fail;

  proxy->name_call = 0;
  proxy->associated = 0;

  if (name)
  {
    proxy->name = strdup(name);
    if (!proxy->name)
      goto fail;
  }
  proxy->for_owner = (!proxy->name || (proxy->name[0] == ':'));

  proxy->path = strdup(path_name);
  proxy->interface = strdup(interface_name);

  if (!proxy->path || !proxy->interface)
    goto fail;

  proxy->manager = e_dbus_proxy_manager_get(connection);
  if (!proxy->manager)
    goto fail;

  return proxy;

fail:
  e_dbus_proxy_destroy(proxy); 

  return NULL;
}

EAPI void
e_dbus_proxy_destroy(E_DBus_Proxy *proxy)
{
  E_DBus_Proxy_Signal_Signature *sig;

  if (!proxy)
    return;

  if (proxy->pending_calls)
    ecore_hash_destroy(proxy->pending_calls);

  if (proxy->signal_handlers)
  {
    ecore_list_first_goto(proxy->signal_handlers);
    while ((sig = ecore_list_next(proxy->signal_handlers)))
    {
      e_dbus_signal_handler_del(proxy->manager->e_connection, sig->sh);
      free(sig->sig_s);
      free(sig);
    }

    ecore_list_destroy(proxy->signal_handlers);
  }

  if (proxy->name)
    free(proxy->name);

  if (proxy->path)
    free(proxy->path);

  if (proxy->interface)
    free(proxy->interface);

  if (proxy->manager)
    e_dbus_proxy_manager_unref(proxy->manager);

  free(proxy);
}

EAPI E_DBus_Proxy *
e_dbus_proxy_new_for_name(E_DBus_Connection *connection, const char *name, const char *path_name, const char *interface_name)
{
  if (!name)
    return NULL;

  return e_dbus_proxy_new(connection, name, path_name, interface_name);
}

EAPI E_DBus_Proxy *
e_dbus_proxy_new_from_proxy(E_DBus_Proxy *proxy, const char *path, const char *interface)
{
  if (!path)
    path = proxy->path;
  if (!interface)
    interface = proxy->interface;

  return e_dbus_proxy_new(proxy->manager->e_connection,
                proxy->name, path, interface);
}

EAPI void
e_dbus_proxy_set_interface(E_DBus_Proxy *proxy, const char *interface)
{
  if (!interface)
    return;

  e_dbus_proxy_manager_unregister(proxy->manager, proxy);
  free(proxy->interface);
  proxy->interface = strdup(interface);
  e_dbus_proxy_manager_register(proxy->manager, proxy);
}

EAPI const char *
e_dbus_proxy_get_bus_name(E_DBus_Proxy *proxy)
{
  return proxy->name;
}

EAPI const char *
e_dbus_proxy_get_path(E_DBus_Proxy *proxy)
{
  return proxy->path;
}

EAPI const char *
e_dbus_proxy_get_interface(E_DBus_Proxy *proxy)
{
  return proxy->interface;
}

EAPI int
e_dbus_proxy_send(E_DBus_Proxy *proxy, DBusMessage *message, dbus_uint32_t *serial)
{
  if (!proxy || !message)
    return 0;

  if (proxy->name)
  {
    if (!dbus_message_set_destination(message, proxy->name))
      return 0;
  }

  if (!dbus_message_set_path (message, proxy->path))
    return 0;

  if (!dbus_message_set_interface (message, proxy->interface))
    return 0;

  if (!dbus_connection_send(proxy->manager->connection, message, serial))
    return 0;

  return 1;
}

EAPI DBusMessage *
e_dbus_proxy_new_method_call(E_DBus_Proxy *proxy, const char *method)
{
  if (!proxy || !method)
    return NULL;

  return dbus_message_new_method_call(proxy->name, proxy->path, proxy->interface, method);
}

inline static int
e_dbus_proxy_is_method_call(E_DBus_Proxy *proxy, DBusMessage *message)
{
  if (!proxy || !message)
    return 0;

  return (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_METHOD_CALL);
}

EAPI int
e_dbus_proxy_call(E_DBus_Proxy *proxy, DBusMessage *message, DBusMessage **reply)
{
  E_DBus_Proxy_Call *call;

  if (!e_dbus_proxy_is_method_call(proxy, message))
    return 0;

  call = e_dbus_proxy_begin_call(proxy, message, NULL, NULL, NULL);
  if (!call)
    return 0;

#ifdef DEBUG
  if (!reply)
  {
    DBusMessage *tmp_reply;
    DBusError error;

    if (!e_dbus_proxy_end_call(proxy, call, &tmp_reply))
      return 0;

    dbus_error_init(&error);
    if (dbus_set_error_from_message(&error, tmp_reply))
      printf("failed to call %s.%s: %s\n",
	  dbus_message_get_interface(message),
	  dbus_message_get_member(message),
	  error.message);

    dbus_error_free(&error);
    dbus_message_unref(tmp_reply);
  }
  else if (!e_dbus_proxy_end_call(proxy, call, reply))
    return 0;
#else
  if (!e_dbus_proxy_end_call(proxy, call, reply))
    return 0;
#endif

  return 1;
}

EAPI void
e_dbus_proxy_call_no_reply(E_DBus_Proxy *proxy, DBusMessage *message)
{
  if (!e_dbus_proxy_is_method_call(proxy, message))
    return;

  dbus_message_set_no_reply(message, TRUE);

  e_dbus_proxy_send(proxy, message, NULL);
}

typedef struct
{
  DBusPendingCall *pending;

  E_DBus_Proxy *proxy;
  unsigned int call_id;
  E_DBus_Proxy_Call_Func cb_func;
  void *data;
  E_DBus_Proxy_Free_Func free_func;
} E_DBus_Pending_Notify;

static void
async_proxy_call(DBusPendingCall *pending, void *data)
{
  E_DBus_Pending_Notify *notify = data;

  notify->cb_func(notify->data, notify->proxy,
		  (E_DBus_Proxy_Call *) ((unsigned long) notify->call_id));
  if (notify->free_func)
    notify->free_func(notify->data);
}

EAPI E_DBus_Proxy_Call *
e_dbus_proxy_begin_call_with_timeout(E_DBus_Proxy *proxy, DBusMessage *message, E_DBus_Proxy_Call_Func cb_func, void *data, E_DBus_Proxy_Free_Func free_func, int timeout)
{
  E_DBus_Pending_Notify *notify = NULL;
  DBusPendingCall *pending;
  unsigned long int call_id;

  if (!e_dbus_proxy_is_method_call(proxy, message))
    return NULL;

  if (cb_func)
  {
    notify = malloc(sizeof(E_DBus_Pending_Notify));
    if (!notify)
      return NULL;

    notify->proxy = proxy;
    notify->cb_func = cb_func;
    notify->data = data;
    notify->free_func = free_func;
  }

  if (!dbus_connection_send_with_reply(proxy->manager->connection, message, &pending, timeout))
  {
    free(notify);

    return NULL;
  }

  call_id = ++proxy->serial_number;

  if (notify)
  {
    notify->call_id = call_id;
    dbus_pending_call_set_notify(pending, async_proxy_call, notify, free);
  }

  ecore_hash_set(proxy->pending_calls, (void *) call_id, pending);

  return (E_DBus_Proxy_Call *) call_id;
}

EAPI E_DBus_Proxy_Call *
e_dbus_proxy_begin_call(E_DBus_Proxy *proxy, DBusMessage *message, E_DBus_Proxy_Call_Func cb_func, void *data, E_DBus_Proxy_Free_Func free_func)
{
  return e_dbus_proxy_begin_call_with_timeout(proxy, message, cb_func, data, free_func, -1);
}

EAPI int
e_dbus_proxy_end_call(E_DBus_Proxy *proxy, E_DBus_Proxy_Call *call, DBusMessage **reply)
{
  DBusPendingCall *pending;

  if (!proxy || !call)
    return 0;

  pending = ecore_hash_get(proxy->pending_calls, call);
  if (!pending)
    return 0;

  dbus_pending_call_block(pending);
  if (reply)
  {
    *reply = dbus_pending_call_steal_reply(pending);
    assert(*reply);
  }
  else
  {
#ifdef DEBUG
    DBusMessage *tmp_reply;
    DBusError error;

    dbus_error_init(&error);
    tmp_reply = dbus_pending_call_steal_reply(pending);
    assert(tmp_reply);
    if (dbus_set_error_from_message(&error, tmp_reply))
      printf("failed to end call: %s\n", error.message);

    dbus_error_free(&error);
    dbus_message_unref(tmp_reply);
#else
    dbus_pending_call_unref(pending);
#endif
  }

  ecore_hash_remove(proxy->pending_calls, call);

  return 1;
}

EAPI void
e_dbus_proxy_connect_signal(E_DBus_Proxy *proxy, const char *signal_name, E_DBus_Signal_Cb cb_signal, void *data)
{
  E_DBus_Proxy_Signal_Signature *sig;

  if (!proxy || !signal_name || !cb_signal)
    return;

  sig = malloc(sizeof(E_DBus_Proxy_Signal_Signature));
  if (!sig)
    return;

  sig->sig_s = strdup(signal_name);
  if (!sig->sig_s)
  {
    free(sig);

    return;
  }

  sig->sh = e_dbus_signal_handler_add(proxy->manager->e_connection,
                                      proxy->name,
                                      proxy->path,
                                      proxy->interface,
                                      signal_name,
                                      cb_signal,
                                      data);

  if (!sig->sh)
  {
    free(sig->sig_s);
    free(sig);

    return;
  }

  sig->sig_c = cb_signal;
  sig->sig_d = data;

  ecore_list_prepend(proxy->signal_handlers, sig);
}

EAPI void
e_dbus_proxy_disconnect_signal(E_DBus_Proxy *proxy, const char *signal_name, E_DBus_Signal_Cb cb_signal, void *data)
{
  E_DBus_Proxy_Signal_Signature *sig;

  if (!proxy || !signal_name || !cb_signal)
    return;

  ecore_list_first_goto(proxy->signal_handlers);
  while ((sig = ecore_list_current(proxy->signal_handlers)))
  {
    if (strcmp(sig->sig_s, signal_name) == 0 &&
        sig->sig_c == cb_signal &&
        sig->sig_d == data)
      break;

    ecore_list_next(proxy->signal_handlers);
  }

  if (sig)
  {
    ecore_list_remove(proxy->signal_handlers);

    e_dbus_signal_handler_del(proxy->manager->e_connection, sig->sh);
    free(sig);
  }
}

EAPI int
e_dbus_proxy_simple_call(E_DBus_Proxy *proxy, const char *method, DBusError *error, int first_arg_type, ...)
{
  DBusMessage *message, *reply = NULL;
  va_list args, args_cp;
  int type;

  va_start(args, first_arg_type);

  message = e_dbus_proxy_new_method_call(proxy, method);
  if (!message)
  {
    dbus_set_error_const(error, DBUS_ERROR_FAILED,
        "failed to create method call");

    goto fail;
  }

  type = first_arg_type;
  va_copy(args_cp, args);
  if (!dbus_message_append_args_valist(message, type, args_cp))
  {
    va_end(args_cp);

    goto fail;
  }
  va_end(args_cp);

  if (!e_dbus_proxy_call(proxy, message, &reply))
  {
    dbus_set_error_const(error, DBUS_ERROR_FAILED,
        "failed to do proxy call");

    goto fail;
  }

  if (dbus_set_error_from_message(error, reply))
    goto fail;

  /* skip INs */
  while (type != DBUS_TYPE_INVALID)
  {
    va_arg(args, void *);
    type = va_arg(args, int);
  }

  type = va_arg(args, int);
  va_copy(args_cp, args);
  if (!dbus_message_get_args_valist(reply, error, type, args_cp))
  {
    va_end(args_cp);

    goto fail;
  }
  va_end(args_cp);

  /* steal OUTs */
  while (type != DBUS_TYPE_INVALID)
  {
    union {
      char *str;
    } *val;

    if (type == DBUS_TYPE_ARRAY)
    {
	    type = va_arg(args, int);
	    val = va_arg(args, void *);
	    val = va_arg(args, void *);

	    type = va_arg(args, int);

	    continue;
    }

    val = va_arg(args, void *);
    if (val)
    {
      switch (type)
      {
        case DBUS_TYPE_STRING:
        case DBUS_TYPE_OBJECT_PATH:
          if (val->str)
            val->str = strdup(val->str);
          break;
        default:
          break;
      }
    }
    type = va_arg(args, int);
  }

  dbus_message_unref(reply);
  dbus_message_unref(message);
  va_end(args);

  return 1;

fail:
  if (reply)
    dbus_message_unref(reply);
  if (message)
    dbus_message_unref(message);
  va_end(args);

  if (error && !dbus_error_is_set(error))
    dbus_set_error(error, DBUS_ERROR_FAILED,
                   "failed to call %s.%s",
                   proxy->interface, method);

  return 0;
}
