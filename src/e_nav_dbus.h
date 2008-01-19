#ifndef E_NAV_DBUS_H 
#define E_NAV_DBUS_H

typedef struct _Bard_Proxy  Bard_Proxy;
typedef struct _World_Proxy World_Proxy;
typedef struct _Viewport_Proxy Viewport_Proxy;
typedef struct _Object_Proxy Object_Proxy;

int                 e_nav_dbus_init(void);
void                e_nav_dbus_shutdown(void);
E_DBus_Connection  *e_nav_dbus_connection_get();

#endif
