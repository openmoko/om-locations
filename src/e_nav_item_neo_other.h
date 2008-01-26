#ifndef E_NAV_ITEM_NEO_OTHER_H
#define E_NAV_ITEM_NEO_OTHER_H

#include "e_nav_dbus.h"
Evas_Object            *e_nav_world_item_neo_other_add(Evas_Object *nav, const char *theme_dir, Object_Proxy* proxy, double lon, double lat);
void                    e_nav_world_item_neo_other_name_set(Evas_Object *item, const char *name);
const char             *e_nav_world_item_neo_other_name_get(Evas_Object *item);
    
#endif
