#ifndef E_NAV_H
#define E_NAV_H

/* types */
typedef struct _E_Nav_Location E_Nav_Location; /* opaque object */

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
    
#endif
