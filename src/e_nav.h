#ifndef E_NAV_H
#define E_NAV_H

/* types */
typedef struct _E_Nav_Location E_Nav_Location; /* opaque object */
typedef struct _E_Nav_World_Item E_Nav_World_Item; /* opaque item in the world */

typedef enum _E_Nav_World_Item_Type
{
   E_NAV_WORLD_ITEM_TYPE_WALLPAPER,
     E_NAV_WORLD_ITEM_TYPE_ITEM,
     E_NAV_WORLD_ITEM_TYPE_OVERLAY
} E_Nav_World_Item_Type;

/* object management */
Evas_Object    *e_nav_add(Evas *e);
void            e_nav_theme_source_set(Evas_Object *obj, const char *custom_dir);
    
/* location stack */
E_Nav_Location *e_nav_location_push(Evas_Object *obj);
void            e_nav_location_pop(Evas_Object *obj);
void            e_nav_location_del(Evas_Object *obj, E_Nav_Location *loc);
E_Nav_Location *e_nav_location_get(Evas_Object *obj);
   
/* spatial & zoom controls */
void            e_nav_coord_set(Evas_Object *obj, double lat, double lon, double when);
double          e_nav_coord_lat_get(Evas_Object *obj);
double          e_nav_coord_lon_get(Evas_Object *obj);
void            e_nav_zoom_set(Evas_Object *obj, double zoom, double when);
double          e_nav_zoom_get(Evas_Object *obj);

/* world items */
E_Nav_World_Item      *e_nav_world_item_add(Evas_Object *obj);
void                   e_nav_world_item_del(E_Nav_World_Item *nwi);
void                   e_nav_world_item_type_set(E_Nav_World_Item *nwi, E_Nav_World_Item_Type type);
E_Nav_World_Item_Type  e_nav_world_item_type_get(E_Nav_World_Item *nwi);
void                   e_nav_world_item_add_func_set(E_Nav_World_Item *nwi, Evas_Object *(*func) (void *data, Evas *evas, const char *theme_dir), void *data);
void                   e_nav_world_item_geometry_set(E_Nav_World_Item *nwi, double x, double y, double w, double h);
void                   e_nav_world_item_geometry_get(E_Nav_World_Item *nwi, double *x, double *y, double *w, double *h);
void                   e_nav_world_item_scale_set(E_Nav_World_Item *nwi, int scale);
int                    e_nav_world_item_scale_get(E_Nav_World_Item *nwi);
void                   e_nav_world_item_update(E_Nav_World_Item *nwi);

#endif
