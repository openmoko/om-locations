#ifndef E_NAV_DBUS_H 
#define E_NAV_DBUS_H

#include "e_dbus_proxy.h"

typedef struct _E_Nav_World E_Nav_World;
typedef struct _E_Nav_Viewport E_Nav_Viewport;

typedef struct _Object_Proxy Object_Proxy;

int                 e_nav_dbus_init(void);
void                e_nav_dbus_shutdown(void);
E_DBus_Connection  *e_nav_dbus_connection_get(void);

E_Nav_World        *e_nav_world_new(void);
void                e_nav_world_destroy(E_Nav_World *world);
E_DBus_Proxy       *e_nav_world_proxy_get(E_Nav_World *world);
E_Nav_Viewport     *e_nav_world_viewport_add(E_Nav_World *world, double lon1, double lat1, double lon2, double lat2);
void                e_nav_world_viewport_remove(E_Nav_World *world, E_Nav_Viewport *view);

E_Nav_Viewport     *e_nav_viewport_new(const char *path);
void                e_nav_viewport_destroy(E_Nav_Viewport *view);
E_DBus_Proxy       *e_nav_viewport_atlas_proxy_get(E_Nav_Viewport *view);

#endif
