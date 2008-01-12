#ifndef E_NAV_H
#define E_NAV_H

/* handy defines */
#define NAV_UNIT_M / ((40000.0 * 1000.0) / 360.0)
#define NAV_UNIT_KM / (40000.0 / 360.0)

/* types */
typedef struct _E_Nav_Location E_Nav_Location; /* opaque object */

typedef enum _E_Nav_World_Item_Type
{
   E_NAV_WORLD_ITEM_TYPE_WALLPAPER,
     E_NAV_WORLD_ITEM_TYPE_ITEM,
     E_NAV_WORLD_ITEM_TYPE_OVERLAY,
     E_NAV_WORLD_ITEM_TYPE_LINKED
} E_Nav_World_Item_Type;

/* object management */
Evas_Object    *e_nav_add(Evas *e);
//void            e_nav_theme_source_set(Evas_Object *obj, const char *custom_dir);
void            e_nav_theme_source_set(Evas_Object *obj, const char *custom_dir, const char *maps);

    
/* spatial & zoom controls */
void            e_nav_coord_set(Evas_Object *obj, double lat, double lon, double when);
double          e_nav_coord_lat_get(Evas_Object *obj);
double          e_nav_coord_lon_get(Evas_Object *obj);
void            e_nav_zoom_set(Evas_Object *obj, double zoom, double when);
double          e_nav_zoom_get(Evas_Object *obj);

/* world items */
void                   e_nav_world_item_add(Evas_Object *obj, Evas_Object *item);
void                   e_nav_world_item_type_set(Evas_Object *item, E_Nav_World_Item_Type type);
E_Nav_World_Item_Type  e_nav_world_item_type_get(Evas_Object *item);
void                   e_nav_world_item_geometry_set(Evas_Object *item, double x, double y, double w, double h);
void                   e_nav_world_item_geometry_get(Evas_Object *item, double *x, double *y, double *w, double *h);
void                   e_nav_world_item_scale_set(Evas_Object *item, Evas_Bool scale);
Evas_Bool              e_nav_world_item_scale_get(Evas_Object *item);
void                   e_nav_world_item_update(Evas_Object *item);
Evas_Object           *e_nav_world_item_nav_get(Evas_Object *item);

#endif
