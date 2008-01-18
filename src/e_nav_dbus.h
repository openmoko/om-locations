#ifndef E_NAV_DBUS_H 
#define E_NAV_DBUS_H

#include <limits.h>

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

typedef enum _DiversityObjectType DiversityObjectType;
enum _DiversityObjectType {
        DIVERSITY_OBJECT_TYPE_OBJECT,
        DIVERSITY_OBJECT_TYPE_VIEWPORT,
        DIVERSITY_OBJECT_TYPE_TAG,
        DIVERSITY_OBJECT_TYPE_BARD,
        DIVERSITY_OBJECT_TYPE_AP,
        DIVERSITY_OBJECT_TYPE_MAP,
        NUM_DIVERSITY_OBJECT_TYPES,
};
/*
enum DIVERSITY_ITEM_TYPE {
    DIVERSITY_ITEM_TYPE_AP,
    DIVERSITY_ITEM_TYPE_NEO_ME,
    DIVERSITY_ITEM_TYPE_NEO_OTHER
};
*/

static E_DBus_Connection* e_conn=NULL;
int e_nav_dbus_init();
void get_e_dbus_connection(E_DBus_Connection **conn);

static void on_current_position_changed(void *data, DBusMessage *msg);
static void on_viewport_object_added(void *data, DBusMessage *msg);
static void on_viewport_object_removed(void *data, DBusMessage *msg);
static void on_object_geometry_changed(void *data, DBusMessage *msg);


typedef struct _Bard_Proxy  Bard_Proxy;
typedef struct _World_Proxy World_Proxy;
typedef struct _Viewport_Proxy Viewport_Proxy;
typedef struct _Object_Proxy Object_Proxy;

Bard_Proxy *bard_proxy_new(char* path, E_DBus_Connection *connection);
World_Proxy *world_proxy_new();
Viewport_Proxy *viewport_proxy_new(char* path, E_DBus_Connection *connection);
Object_Proxy *object_proxy_new(char* path, E_DBus_Connection *connection);

void self_get();
void self_get_reply(void *data, DBusMessage *reply, DBusError *error);
void viewport_add(double lat1, double lon1, double lat2, double lon2);
void viewport_add_reply(void *data, DBusMessage *reply, DBusError *error);
void viewport_remove(World_Proxy *proxy, char* object_path);
void e_nav_connect_ap(char* object_path);
void e_nav_ap_info_get(char* object_path);

struct _Bard_Proxy {
    char object_path[PATH_MAX];  // remote viewport path
    E_DBus_Connection *connection;
    Evas_Object *object;   // Neo_me
};

struct _World_Proxy {
    E_DBus_Connection *connection;
    Evas_Object *object;
};

struct _Viewport_Proxy {
    char object_path[PATH_MAX];  // remote viewport path
    E_DBus_Connection *connection;
    Evas_Object *object;
};

struct _Object_Proxy {
    char object_path[PATH_MAX];   // remote object path
    E_DBus_Connection *connection;
    Evas_Object *object;
};


E_DBus_Signal_Handler *
e_nav_object_signal_handler_add(E_DBus_Connection *conn, char* object_path, char* interface, char* signal_name, E_DBus_Signal_Cb cb_signal);

Evas_Object* get_e_nav_object(const char* obj_path);
void remove_e_nav_object(const char* obj_path);
int add_e_nav_object(const char *obj_path, Evas_Object* o);

// call back functions
void e_nav_neo_me_add(Bard_Proxy *proxy, double lat, double lon );
void e_nav_ap_added(Object_Proxy* proxy, double lat, double lon);
void e_nav_neo_other_added(Object_Proxy* proxy, double lat, double lon);

//void e_nav_object_add(Object_Proxy* proxy, int type, double lat, double lon);
void e_nav_object_del(const char* obj_path);

#endif
