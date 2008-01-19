#ifndef E_NAV_ITEM_NEO_ME_H
#define E_NAV_ITEM_NEO_ME_H

Evas_Object            *e_nav_world_item_neo_me_add(Evas_Object *nav, const char *theme_dir, double lat, double lon);
void                    e_nav_world_item_neo_me_name_set(Evas_Object *item, const char *name);
const char             *e_nav_world_item_neo_me_name_get(Evas_Object *item);
void                    e_nav_world_item_neo_me_visible_set(Evas_Object *item, Evas_Bool active);
Evas_Bool               e_nav_world_item_neo_me_visible_get(Evas_Object *item);
    
#endif
