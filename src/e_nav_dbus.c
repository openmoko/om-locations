#include <E_DBus.h>
#include <e.h>
#include "e_nav_dbus.h"

// We need a World Proxy, Viewport Proxy, Object Proxy
// that can be used to hook a signal and easily to call a method
// ToDo:   Proxy concept. 

void object_geometry_get(double *x, double *y, double *w, double *h);
void object_geometry_set(double x, double y, double w, double h);

World_Proxy *world_proxy_fill()
{
    get_e_dbus_connection(&e_conn);
    if(e_conn==NULL) {
        return NULL;
    }
    World_Proxy *proxy;
    proxy = malloc(sizeof(*proxy));
    if (!proxy) {
        return NULL;
    }
    proxy->connection = e_conn;
    proxy->viewport_add_func = add_viewport;
    proxy->viewport_remove_func = remove_viewport;
    return proxy;
}

Viewport_Proxy *viewport_proxy_fill(char* path, E_DBus_Connection *connection)
{
    if(connection==NULL) {
        return NULL;
    }
    Viewport_Proxy *proxy;
    proxy = malloc(sizeof(*proxy));
    if (!proxy) {
        return NULL;
    }
    proxy->path[PATH_MAX-1] = '\0';
    strncpy(proxy->path, path, PATH_MAX-1);
    proxy->connection = connection;
    E_DBus_Signal_Handler *handler;
    handler = e_nav_object_signal_handler_add(proxy->connection, proxy->path, DIVERSITY_VIEWPORT_DBUS_INTERFACE, "ObjectAdded", on_viewport_object_added);
    if(handler==NULL) 
        printf("handler NULL1 \n");
    handler = e_nav_object_signal_handler_add(proxy->connection, proxy->path,  DIVERSITY_VIEWPORT_DBUS_INTERFACE, "ObjectRemoved", on_viewport_object_removed);
    if(handler==NULL)
        printf("handler NULL2\n");
    return proxy;
}

Object_Proxy *object_proxy_fill(char* path, E_DBus_Connection *connection)
{
    if(connection==NULL) {
        return NULL;
    }
    Object_Proxy *proxy;
    proxy = malloc(sizeof(*proxy));
    if (!proxy) {
        return NULL;
    }
    proxy->path[PATH_MAX-1] = '\0';
    strncpy(proxy->path, path, PATH_MAX-1);
    proxy->connection = connection;
    proxy->geometry_get_func = object_geometry_get;
    proxy->geometry_set_func = object_geometry_set;
    
    e_nav_object_signal_handler_add(proxy->connection, proxy->path,  DIVERSITY_OBJECT_DBUS_INTERFACE, "GeometryChanged", on_object_geometry_changed);
    return proxy;
}

void object_geometry_get(double *x, double *y, double *w, double *h)
{
    printf("Object Geometry Get\n");
    
}

void object_geometry_set(double x, double y, double w, double h)
{
    printf("Object Geometry Set\n");
}
/*
void dbus_world_proxy_call(const char *method, ...)
{
    va_list args;
    DBusMessage *msg;
    msg = dbus_message_new_method_call(
        DIVERSITY_DBUS_SERVICE,  
        DIVERSITY_WORLD_DBUS_PATH, 
        DIVERSITY_WORLD_DBUS_INTERFACE,  
        method
    ); 
    va_start(args, method);
    
    //dbus_message_append_args(msg, DBUS_TYPE_DOUBLE, &lat1,
                                  DBUS_TYPE_DOUBLE, &lon1, 
                                  DBUS_TYPE_DOUBLE, &lat2, 
                                  DBUS_TYPE_DOUBLE, &lon2,
                                  DBUS_TYPE_INVALID);
    //e_dbus_message_send(e_conn, msg, viewport_add_reply, -1, NULL);
    dbus_message_unref(msg); 
    va_end(args); 
}
*/
void add_viewport(double lat1, double lon1, double lat2, double lon2)
{
   get_e_dbus_connection(&e_conn);
   
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

void
viewport_add_reply(void *data, DBusMessage *reply, DBusError *error)
{
    const char* viewport_path;
    if (dbus_error_is_set(error))
    {
        printf("Viewport add reply Error: %s - %s\n", error->name, error->message);
        return;
    }

    dbus_message_get_args(reply, error, DBUS_TYPE_OBJECT_PATH, &viewport_path, DBUS_TYPE_INVALID);
    printf("Viewport added: %s\n", viewport_path);
    // New a Viewport proxy
    Viewport_Proxy* proxy = viewport_proxy_fill(viewport_path, e_conn); 
    if(proxy==NULL) {
        return;
    }
}

void remove_viewport(char* object_path)
{
    get_e_dbus_connection(&e_conn);
    
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
        Object_Proxy* proxy = object_proxy_fill(object_path, e_conn); 
        if(proxy==NULL) {
            return;
        }
        //  New object in UI and show on the map
        e_nav_object_add(proxy); 
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

void get_e_dbus_connection(E_DBus_Connection **conn)
{
   if(*conn!=NULL) {
       return;    
   }
   ecore_init();
   ecore_string_init();
   e_dbus_init();
   *conn = e_dbus_bus_get(DBUS_BUS_SESSION);
   if(*conn) {
       printf("E_DBus: Connection OK !\n"); 
 
   }
   else {
       printf("E_DBus Error: could not connect to session bus.\n");
   } 
}
