#include "e_nav.h"
#include <assert.h>
#include "e_nav_dbus.h"
#include "e_dbus_proxy.h"
#include "e_mod_nav.h"
#include "e_nav_item_ap.h"
#include "e_nav_item_neo_me.h"
#include "e_nav_item_neo_other.h"
#include "e_nav_item_link.h"

#define DIVERSITY_DBUS_BUS			DBUS_BUS_SESSION
#define DIVERSITY_DBUS_SERVICE			"org.openmoko.diversity"
#define DIVERSITY_DBUS_PATH			"/org/openmoko/diversity"
#define DIVERSITY_DBUS_INTERFACE		"org.openmoko.diversity"

#define DIVERSITY_WORLD_DBUS_PATH 		"/org/openmoko/diversity/world"
#define DIVERSITY_WORLD_DBUS_INTERFACE		"org.openmoko.diversity.world"

#define DIVERSITY_VIEWPORT_DBUS_PATH		"/org/openmoko/diversity/viewport"
#define DIVERSITY_VIEWPORT_DBUS_INTERFACE	"org.openmoko.diversity.viewport"

#define DIVERSITY_OBJECT_DBUS_PATH		"/org/openmoko/diversity/object"
#define DIVERSITY_OBJECT_DBUS_INTERFACE		"org.openmoko.diversity.object"

enum _DiversityObjectType {
	DIVERSITY_OBJECT_TYPE_OBJECT,
	DIVERSITY_OBJECT_TYPE_VIEWPORT,
	DIVERSITY_OBJECT_TYPE_TAG,
	DIVERSITY_OBJECT_TYPE_BARD,
	DIVERSITY_OBJECT_TYPE_AP,
	DIVERSITY_OBJECT_TYPE_MAP,
	NUM_DIVERSITY_OBJECT_TYPES,
};

static Ecore_Hash *hash_table = NULL;   // store neo_other and ap objects
static World_Proxy *worldProxy = NULL;   // singleton?
static Bard_Proxy *bardProxy = NULL;     // singleton?

static E_DBus_Connection* e_conn=NULL;
static void on_current_position_changed(void *data, DBusMessage *msg);
static void on_viewport_object_added(void *data, DBusMessage *msg);
static void on_viewport_object_removed(void *data, DBusMessage *msg);
static void on_object_geometry_changed(void *data, DBusMessage *msg);

void self_get();
void self_get_reply(void *data, DBusMessage *reply, DBusError *error);

E_DBus_Signal_Handler *
e_nav_object_signal_handler_add(E_DBus_Connection *conn, char* object_path, char* interface, char* signal_name, E_DBus_Signal_Cb cb_signal);

// call back functions
void e_nav_neo_me_add(Bard_Proxy *proxy, double lat, double lon );
void e_nav_ap_added(Object_Proxy* proxy, double lat, double lon);
void e_nav_neo_other_added(Object_Proxy* proxy, double lat, double lon);

//void e_nav_object_add(Object_Proxy* proxy, int type, double lat, double lon);
void e_nav_object_del(const char* obj_path);

void object_geometry_get(double *x, double *y, double *w, double *h);
void object_geometry_set(double x, double y, double w, double h);

void viewport_add(double lat1, double lon1, double lat2, double lon2);
void viewport_add_reply(void *data, DBusMessage *reply, DBusError *error);
void viewport_remove(World_Proxy *proxy, char* object_path);

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

Bard_Proxy *bard_proxy_new(char* path, E_DBus_Connection *connection)
{
    if(bardProxy) {
        return bardProxy;
    }
    Bard_Proxy *proxy;
    proxy = malloc(sizeof(*proxy));
    if(!proxy) {
        printf("proxy malloc is NULL\n");
        return NULL;
    }
    proxy->object_path[PATH_MAX-1] = '\0';
    strncpy(proxy->object_path, path, PATH_MAX-1);
    proxy->connection = connection;

    return proxy;
}

World_Proxy *world_proxy_new()
{
    if(worldProxy) {
        return worldProxy;
    }
    World_Proxy *proxy;
    proxy = malloc(sizeof(*proxy));
    if (!proxy) {
        return NULL;
    }
    proxy->connection = e_conn;
    //proxy->viewport_add_func = add_viewport;
    //proxy->viewport_remove_func = remove_viewport;
    return proxy;
}


Viewport_Proxy *viewport_proxy_new(char* path, E_DBus_Connection *connection)
{
    if(connection==NULL) {
        return NULL;
    }
    Viewport_Proxy *proxy;
    proxy = malloc(sizeof(*proxy));
    if (!proxy) {
        return NULL;
    }
    proxy->object_path[PATH_MAX-1] = '\0';
    strncpy(proxy->object_path, path, PATH_MAX-1);
    proxy->connection = connection;

    E_DBus_Signal_Handler *handler;
    handler = e_nav_object_signal_handler_add(proxy->connection, proxy->object_path, DIVERSITY_VIEWPORT_DBUS_INTERFACE, "ObjectAdded", on_viewport_object_added);
    if(handler==NULL) 
        printf("handler NULL1 \n");
    handler = e_nav_object_signal_handler_add(proxy->connection, proxy->object_path,  DIVERSITY_VIEWPORT_DBUS_INTERFACE, "ObjectRemoved", on_viewport_object_removed);
    if(handler==NULL)
        printf("handler NULL2\n");
    return proxy;
}

Object_Proxy *object_proxy_new(char* path, E_DBus_Connection *connection)
{
    if(connection==NULL) {
        return NULL;
    }
    Object_Proxy *proxy;
    proxy = malloc(sizeof(*proxy));
    if (!proxy) {
        return NULL;
    }
    proxy->object_path[PATH_MAX-1] = '\0';
    strncpy(proxy->object_path, path, PATH_MAX-1);
    proxy->connection = connection;

    //proxy->geometry_get_func = object_geometry_get;
    //proxy->geometry_set_func = object_geometry_set;
    
    e_nav_object_signal_handler_add(proxy->connection, proxy->object_path,  DIVERSITY_OBJECT_DBUS_INTERFACE, "GeometryChanged", on_object_geometry_changed);
    return proxy;
}

void self_get()
{
   if(e_conn) {
       printf("e_conn OK !\n");
   }
   else {
       printf("e_conn is NULL\n");
   }

   DBusMessage *msg;
   msg = dbus_message_new_method_call(
      DIVERSITY_DBUS_SERVICE,  
      DIVERSITY_WORLD_DBUS_PATH, 
      DIVERSITY_WORLD_DBUS_INTERFACE,  
      "SelfGet"    
   );
   e_dbus_message_send(e_conn, msg, self_get_reply, -1, NULL);
   dbus_message_unref(msg); 
  
}

void self_get_reply(void *data, DBusMessage *reply, DBusError *error)
{
    char* bard_path;
    if (dbus_error_is_set(error))
    {
        printf("Self get reply Error: %s - %s\n", error->name, error->message);
        return;
    }

    dbus_message_get_args(reply, error, DBUS_TYPE_OBJECT_PATH, &bard_path, DBUS_TYPE_INVALID);
    printf("bard path: %s\n", bard_path);
    bardProxy = bard_proxy_new(bard_path, e_conn); 
    if(bardProxy==NULL) {
        return;
    }
    E_DBus_Signal_Handler *handler;
    handler = e_nav_object_signal_handler_add(bardProxy->connection, bardProxy->object_path, DIVERSITY_OBJECT_DBUS_INTERFACE, "GeometryChanged", on_current_position_changed);
    if(!handler)
         printf("bard proxy add signal error\n");
    
#if 0
    e_nav_neo_me_add(bardProxy, 151.210000, 33.870000);  // last fix location
    if(bardProxy->object==NULL) {
        printf("bardProxy evasObj is NULL\n");
    }   
#endif
}

static void e_nav_world_item_neo_me_position_change(Evas_Object* evasObj, double lat, double lon)
{
    if(evasObj==NULL) {
        printf("EvasObj==NULL\n");
        return;
    }
    e_nav_world_item_geometry_set(evasObj, lat, lon, 0, 0);
    e_nav_world_item_update(evasObj);
}

static void on_current_position_changed(void *data, DBusMessage *msg)
{
    DBusError err;
    double lat;
    double lon;
    double width;
    double height;
    dbus_error_init(&err);
    if(!dbus_message_get_args(msg, &err,
                 DBUS_TYPE_DOUBLE, &lon,
                 DBUS_TYPE_DOUBLE, &lat,
                 DBUS_TYPE_DOUBLE, &width,
                 DBUS_TYPE_DOUBLE, &height,
                 DBUS_TYPE_INVALID))
    {
        printf("Error: on_current_position_changed: %s\n", err.name);
        printf("%s\n", err.message);
        return;
    }
    else {
        e_nav_world_item_neo_me_position_change(bardProxy->object, lon, lat); 
    }
}

void viewport_add(double lat1, double lon1, double lat2, double lon2)
{
   if(e_conn) {
       printf("e_conn OK !\n");
   }
   else {
       printf("e_conn is NULL\n");
   }

   DBusMessage *msg;
   msg = dbus_message_new_method_call(
      DIVERSITY_DBUS_SERVICE,  
      DIVERSITY_WORLD_DBUS_PATH, 
      DIVERSITY_WORLD_DBUS_INTERFACE,  
    "ViewportAdd"    
   );
   dbus_message_append_args(msg, DBUS_TYPE_DOUBLE, &lat1,
                                 DBUS_TYPE_DOUBLE, &lon1, 
                                 DBUS_TYPE_DOUBLE, &lat2, 
                                 DBUS_TYPE_DOUBLE, &lon2,
                                 DBUS_TYPE_INVALID);
   e_dbus_message_send(e_conn, msg, viewport_add_reply, -1, NULL);
   dbus_message_unref(msg); 
}

void viewport_add_reply(void *data, DBusMessage *reply, DBusError *error)
{
    char* viewport_path;
    if (dbus_error_is_set(error))
    {
        printf("Viewport add reply Error: %s - %s\n", error->name, error->message);
        return;
    }

    dbus_message_get_args(reply, error, DBUS_TYPE_OBJECT_PATH, &viewport_path, DBUS_TYPE_INVALID);
    printf("Viewport added: %s\n", viewport_path);
    // New a Viewport proxy
    Viewport_Proxy* proxy = viewport_proxy_new(viewport_path, e_conn); 
    if(proxy==NULL) {
        return;
    }
}

void viewport_remove(World_Proxy *proxy, char* object_path)
{
    DBusMessage *msg;
    msg = dbus_message_new_method_call(
        DIVERSITY_DBUS_SERVICE,  
        DIVERSITY_WORLD_DBUS_PATH, 
        DIVERSITY_WORLD_DBUS_INTERFACE,  
        "ViewportRemove"    
    );
    dbus_message_append_args(msg, 
                             DBUS_TYPE_OBJECT_PATH, &object_path,
                             DBUS_TYPE_INVALID);
    e_dbus_message_send(e_conn, msg, NULL, -1, NULL);
    dbus_message_unref(msg); 
}

void object_type_get_reply(void *data, DBusMessage *reply, DBusError *error)
{
    int type;
    if (dbus_error_is_set(error))
    {
        printf("Type Get reply Error: %s - %s\n", error->name, error->message);
        return;
    }

    dbus_message_get_args(reply, error, DBUS_TYPE_INT32, &type, DBUS_TYPE_INVALID);
    printf("Object type: %d\n", type);
    if(type==DIVERSITY_OBJECT_TYPE_AP) { 
        //e_nav_object_add(proxy, lat, lon);  // hard code 
    }
    else if(type==DIVERSITY_OBJECT_TYPE_OBJECT) {

    }
}

void object_type_get(Object_Proxy *proxy)
{
    DBusMessage *msg;
    msg = dbus_message_new_method_call(
        DIVERSITY_DBUS_SERVICE,  
        proxy->object_path,
        DIVERSITY_OBJECT_DBUS_INTERFACE, 
        "TypeGet"    
    );
    e_dbus_message_send(e_conn, msg, object_type_get_reply, -1, proxy);     
    dbus_message_unref(msg); 
}

static void on_viewport_object_added(void *data, DBusMessage *msg)
{
    DBusError err;
    char *object_path;
    dbus_error_init(&err);
    if(!dbus_message_get_args(msg, &err,
                 DBUS_TYPE_OBJECT_PATH, &object_path,
                 DBUS_TYPE_INVALID))
    {
        printf("Error: on_viewport_object_added: %s\n", err.name);
        printf("%s\n", err.message);
        return;
    }
    else {
        printf("Object Added: %s \n", object_path);
        //  New a Proxy
        Object_Proxy* proxy = object_proxy_new(object_path, e_conn); 
        if(proxy==NULL) {
            return;
        }
        // get type
        object_type_get(proxy);

        //  Test: New object in UI and show on the map
        double lat=122.0, lon=23.0;
        int i=rand()/2;
        if(i==1) {
            lat = lat+drand48();
        }
        else {
            lat = lat-drand48();
        }
        i=rand()/2;
        if(i==1) {
            lon = lon+drand48();
        }
        else {
           lon = lon-drand48();
        }
        // get type
        
        //e_nav_object_add(proxy, DIVERSITY_ITEM_TYPE_AP, lat, lon);  // hard code 
    }
}

static void on_viewport_object_removed(void *data, DBusMessage *msg)
{
    DBusError err;
    char *object_path;
    dbus_error_init(&err);
    if(!dbus_message_get_args(msg, &err,
                 DBUS_TYPE_OBJECT_PATH, &object_path,
                 DBUS_TYPE_INVALID))
    {
        printf("Error: on_viewport_object_removed\n");
        return;
    }
    else {
        printf("Object Removed: %s \n", object_path);
        e_nav_object_del(object_path);
    }
}

static void on_object_geometry_changed(void *data, DBusMessage *msg)
{
    DBusError err;
    double lon, lat, width, height;
    dbus_error_init(&err);
    if(!dbus_message_get_args(msg, &err,
                 DBUS_TYPE_DOUBLE, &lon,
                 DBUS_TYPE_DOUBLE, &lat,
                 DBUS_TYPE_DOUBLE, &width,
                 DBUS_TYPE_DOUBLE, &height,
                 DBUS_TYPE_INVALID))
    {
        printf("Error: on_object_geometry_changed: %s\n", err.name);
        printf("%s\n", err.message);
        return;
    }
    else {
        printf("Object geometry changed: %f,%f %f,%f \n", lon, lat, width, height);
    }
}

void e_nav_connect_ap(char* object_path)
{
    DBusMessage *msg;
    msg = dbus_message_new_method_call(
        DIVERSITY_DBUS_SERVICE,
        DIVERSITY_WORLD_DBUS_PATH,
        DIVERSITY_WORLD_DBUS_INTERFACE,
        "ConnectAp"
    );
    dbus_message_append_args(msg,
                             DBUS_TYPE_OBJECT_PATH, &object_path,
                             DBUS_TYPE_INVALID);
    e_dbus_message_send(e_conn, msg, NULL, -1, NULL);
    dbus_message_unref(msg);
}

#if 0
void e_nav_ap_info_get(char* object_path)
{
    DBusMessage *msg;
    msg = dbus_message_new_method_call(
        DIVERSITY_DBUS_SERVICE,
        object_path,
        "org.freedesktop.DBus.Properties",
        "GetAll"
    );
    dbus_message_append_args(msg,
                             DBUS_TYPE_OBJECT_PATH, DIVERSITY_OBJECT_DBUS_INTERFACE,
                             DBUS_TYPE_INVALID);
    e_dbus_message_send(e_conn, msg, NULL, -1, NULL);
    dbus_message_unref(msg);
}
#endif

E_DBus_Signal_Handler *
e_nav_object_signal_handler_add(E_DBus_Connection *conn, char* object_path, char* interface, char* signal_name, E_DBus_Signal_Cb cb_signal)
{
    return e_dbus_signal_handler_add(
        conn,
        DIVERSITY_DBUS_SERVICE, 
        object_path, 
        interface,
        signal_name,
        cb_signal, 
        NULL);
}


Evas_Object* get_e_nav_object(const char* obj_path)
{
    return  ecore_hash_get(hash_table, obj_path);
}

void remove_e_nav_object(const char* obj_path)
{
    ecore_hash_remove(hash_table, obj_path);
}

int add_e_nav_object(const char *obj_path, Evas_Object* o)
{
     return ecore_hash_set(hash_table, (void *) obj_path, o);
}

void e_nav_object_del(const char* obj_path)
{
    Evas_Object *nwi = get_e_nav_object(obj_path);
    if(nwi) {
        evas_object_del(nwi);
    }
    remove_e_nav_object(obj_path);
}

#if 0
// when we zoom in and zoom out or move the map, we should change the current viewport
// when zoom in, we double size the viewport, when zoom out, we half the size of the viewport
// when move the map or the user move his position, we change the coordinate of the left-up corner and the buttom-down corner
void change_viewport()
{

}

// we can have more than one viewport, and we can change to the viewport we want to see
// use a list to manage viewport
void e_nav_viewport_add(Viewport_Proxy* proxy)
{
    
}

void e_nav_viewport_del()
{

}

void e_nav_neo_me_add(Bard_Proxy *proxy, double lat, double lon )
{
    Evas_Object *nwi;
    nwi = e_nav_world_item_neo_me_add(nav,  THEME_PATH, lat, lon);
    e_nav_world_item_neo_me_name_set(nwi, "Me");
    proxy->object = nwi;

}


void e_nav_ap_added(Object_Proxy* proxy, double lat, double lon)
{
    Evas_Object *nwi;
    nwi = e_nav_world_item_ap_add(nav,  THEME_PATH, proxy, lat, lon);
    e_nav_world_item_ap_essid_set(nwi, "Office");   // hard code
    e_nav_world_item_ap_key_type_set(nwi, E_NAV_ITEM_AP_KEY_TYPE_NONE); // hard code
    e_nav_world_item_ap_range_set(nwi, 100 NAV_UNIT_M);  // hard code
    proxy->object = nwi; 
    add_e_nav_object(proxy->object_path, nwi);
}

void e_nav_neo_other_added(Object_Proxy* proxy, double lat, double lon)
{
    Evas_Object *nwi;
    nwi = e_nav_world_item_neo_other_add(nav,  THEME_PATH, proxy, lat, lon);
    e_nav_world_item_neo_other_name_set(nwi, "Sean");   // hard code  
    proxy->object = nwi;
    add_e_nav_object(proxy->object_path, nwi);
}
#endif

int
e_nav_dbus_init(void)
{
   static int initialized = -1;

   if (initialized != -1)
     return initialized;

   initialized = 0;

   if (!e_dbus_init())
     return 0;

   e_conn = e_dbus_bus_get(DIVERSITY_DBUS_BUS);
   if (!e_conn)
     return 0;

   initialized = 1;

   return 1;
}

void
e_nav_dbus_shutdown(void)
{
   if (e_conn)
     e_dbus_connection_close(e_conn);
}

E_DBus_Connection *
e_nav_dbus_connection_get()
{
   return e_conn;
}
