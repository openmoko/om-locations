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

#ifndef E_DBUS_PROXY_H
#define E_DBUS_PROXY_H

#include <E_DBus.h>
#include <dbus/dbus.h>

#ifdef __cplusplus
extern "C" {
#endif

   typedef struct E_DBus_Proxy E_DBus_Proxy;
   typedef struct E_DBus_Proxy_Call E_DBus_Proxy_Call;

   typedef void (*E_DBus_Proxy_Call_Func) (void *user_data, E_DBus_Proxy *proxy, E_DBus_Proxy_Call *call_id);
   typedef void (*E_DBus_Proxy_Free_Func) (void *data);

   EAPI E_DBus_Proxy *e_dbus_proxy_new_for_name(E_DBus_Connection *connection,
                                                const char *name,
                                                const char *path_name,
                                                const char *interface_name);

   EAPI void e_dbus_proxy_destroy(E_DBus_Proxy *proxy);

   EAPI E_DBus_Proxy *e_dbus_proxy_new_from_proxy(E_DBus_Proxy *proxy,
                                                  const char *interface,
                                                  const char *path);

   EAPI void e_dbus_proxy_set_interface(E_DBus_Proxy *proxy,
                                        const char *interface);

   EAPI const char *e_dbus_proxy_get_bus_name(E_DBus_Proxy *proxy);
   EAPI const char *e_dbus_proxy_get_path(E_DBus_Proxy *proxy);
   EAPI const char *e_dbus_proxy_get_interface(E_DBus_Proxy *proxy);

   EAPI int e_dbus_proxy_send(E_DBus_Proxy *proxy, DBusMessage *message,
                              dbus_uint32_t *serial);

   EAPI DBusMessage *e_dbus_proxy_new_method_call(E_DBus_Proxy *proxy, const char *method);

   EAPI int e_dbus_proxy_call(E_DBus_Proxy *proxy,
                              DBusMessage *message,
                              E_DBus_Callback_Func cb_func,
                              void *data);

   EAPI void e_dbus_proxy_call_no_reply(E_DBus_Proxy *proxy,
                                        DBusMessage *message);

   EAPI E_DBus_Proxy_Call *e_dbus_proxy_begin_call_with_timeout(E_DBus_Proxy *proxy,
                                                                DBusMessage *message,
                                                                E_DBus_Proxy_Call_Func cb_func,
                                                                void *data,
                                                                E_DBus_Proxy_Free_Func free_func,
                                                                int timeout);

   EAPI E_DBus_Proxy_Call *e_dbus_proxy_begin_call(E_DBus_Proxy *proxy,
                                                   DBusMessage *message,
                                                   E_DBus_Proxy_Call_Func cb_func,
                                                   void *data,
                                                   E_DBus_Proxy_Free_Func free_func);

   EAPI int e_dbus_proxy_end_call(E_DBus_Proxy *proxy,
                                  E_DBus_Proxy_Call *call,
                                  DBusMessage **reply);

   EAPI void e_dbus_proxy_cancel_call(E_DBus_Proxy *proxy,
                                      E_DBus_Proxy_Call *call);

   EAPI void e_dbus_proxy_connect_signal(E_DBus_Proxy *proxy,
                                         const char *signal_name,
                                         E_DBus_Signal_Cb cb_signal,
                                         void *data);

   EAPI void e_dbus_proxy_disconnect_signal(E_DBus_Proxy *proxy,
                                            const char *signal_name,
                                            E_DBus_Signal_Cb cb_signal,
                                            void *data);

#ifdef __cplusplus
}
#endif

#endif /* E_DBUS_PROXY_H */
