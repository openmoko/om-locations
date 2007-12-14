#ifndef E_NAV_ITEM_NEO_OTHER_H
#define E_NAV_ITEM_NEO_OTHER_H

Evas_Object            *e_nav_world_item_neo_other_add(Evas_Object *nav, const char *theme_dir, double lat, double lon);
void                    e_nav_world_item_neo_other_name_set(Evas_Object *item, const char *name);
const char             *e_nav_world_item_neo_other_name_get(Evas_Object *item);
    
#endif
