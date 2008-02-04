
#ifndef E_NAV_ITEM_LOCATION_H
#define E_NAV_ITEM_LOCATION_H

Evas_Object            *e_nav_world_item_location_add(Evas_Object *nav, const char *theme_dir, double lon, double lat);
void                    e_nav_world_item_location_name_set(Evas_Object *item, const char *name);
const char             *e_nav_world_item_location_name_get(Evas_Object *item);
void                    e_nav_world_item_location_description_set(Evas_Object *item, const char *description);
const char             *e_nav_world_item_location_description_get(Evas_Object *item);
void                    e_nav_world_item_location_visible_set(Evas_Object *item, Evas_Bool active);
Evas_Bool               e_nav_world_item_location_visible_get(Evas_Object *item);
void                    e_nav_world_item_location_lat_set(Evas_Object *item, double lat);
double                  e_nav_world_item_location_lat_get(Evas_Object *item);
void                    e_nav_world_item_location_lon_set(Evas_Object *item, double lon);
double                  e_nav_world_item_location_lon_get(Evas_Object *item);
    
#endif


