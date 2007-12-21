#ifndef E_NAV_DBUS_H 
#define E_NAV_DBUS_H
//#include "e_mod_nav.h"

#define DIVERSITY_DBUS_BUS              DBUS_BUS_SESSION
#define DIVERSITY_DBUS_SERVICE          "org.openmoko.diversity"
#define DIVERSITY_DBUS_PATH             "/org/openmoko/diversity"
#define DIVERSITY_DBUS_INTERFACE        "org.openmoko.diversity"

#define DIVERSITY_WORLD_DBUS_PATH       "/org/openmoko/diversity/world"
#define DIVERSITY_WORLD_DBUS_INTERFACE  "org.openmoko.diversity.world"

#define DIVERSITY_VIEWPORT_DBUS_PATH    "/org/openmoko/diversity/viewport"
#define DIVERSITY_VIEWPORT_DBUS_INTERFACE    "org.openmoko.diversity.viewport"

#define DIVERSITY_OBJECT_DBUS_PATH      "/org/openmoko/diversity/object"
#define DIVERSITY_OBJECT_DBUS_INTERFACE      "org.openmoko.diversity.object"

//#include <E_DBus.h>

static E_DBus_Connection* e_conn=NULL;
void get_e_dbus_connection(E_DBus_Connection **conn);
void viewport_add_reply(void *data, DBusMessage *reply, DBusError *error);
void add_viewport(double lat1, double lon1, double lat2, double lon2);
void remove_viewport(char* object_path);
static void on_viewport_object_added(void *data, DBusMessage *msg);
static void on_viewport_object_removed(void *data, DBusMessage *msg);
static void on_object_geometry_changed(void *data, DBusMessage *msg);

typedef void (viewport_add_t)(double lon1, double lat1, double lon2, double lat2);
typedef void (viewport_remove_t)(char* object_path);

typedef void (geometry_get_t)(double *x, double *y, double *w, double *h);
typedef void (geometry_set_t)(double x, double y, double w, double h);

typedef struct _World_Proxy World_Proxy;
typedef struct _Viewport_Proxy Viewport_Proxy;
typedef struct _Object_Proxy Object_Proxy;

struct _World_Proxy {
    //char path[PATH_MAX];
    E_DBus_Connection *connection;
    viewport_add_t    * viewport_add_func;
    viewport_remove_t * viewport_remove_func;
};

struct _Viewport_Proxy {
      char path[PATH_MAX];
      E_DBus_Connection *connection;
};

struct _Object_Proxy {
      char path[PATH_MAX];
      E_DBus_Connection *connection;
      //signals[];
      //addSignal();
      //deleteSignal();
      //int (*cb)(struct gsmd_atcmd *cmd, void *ctx, char *resp);
      geometry_get_t * geometry_get_func;
      geometry_set_t * geometry_set_func;
//      E_DBus_Signal_Handler * singal_handlers[];
};

World_Proxy *world_proxy_fill();

E_DBus_Signal_Handler *
e_nav_object_signal_handler_add(E_DBus_Connection *conn, char* object_path, char* interface, char* signal_name, E_DBus_Signal_Cb cb_signal);
void e_nav_object_add(Object_Proxy* proxy);

#endif
