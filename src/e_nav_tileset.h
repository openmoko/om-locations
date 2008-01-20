#ifndef E_NAV_TILESET_H
#define E_NAV_TILESET_H

#include "e_nav.h"

#define RADIANS(d) ((d) * M_PI / 180.0)
#define DEGREES(d) ((d) * 180.0 / M_PI)
#define M_EARTH_RADIUS	(6371.0 * 1000.0)
#define M_LOG2		(0.693147181)

typedef enum _E_Nav_Tileset_Format {
   E_NAV_TILESET_FORMAT_OSM,
} E_Nav_Tileset_Format;

Evas_Object            *e_nav_tileset_add(Evas_Object *nav, E_Nav_Tileset_Format format, const char *dir);
void                    e_nav_tileset_update(Evas_Object *obj);

/* decide span by map level */
void                    e_nav_tileset_level_set(Evas_Object *obj, int level);
int                     e_nav_tileset_level_get(Evas_Object *obj);
void                    e_nav_tileset_levels_list(Evas_Object *obj, int *max_level, int *min_level);

/* decide map level by span */
void                    e_nav_tileset_span_set(Evas_Object *obj, int span);
int                     e_nav_tileset_span_get(Evas_Object *obj);

/* set span by specifying meters per pixel */
void                    e_nav_tileset_scale_set(Evas_Object *obj, double scale);
double                  e_nav_tileset_scale_get(Evas_Object *obj);

void                    e_nav_tileset_center_set(Evas_Object *obj, double lon, double lat);
void                    e_nav_tileset_center_get(Evas_Object *obj, double *lon, double *lat);

void                    e_nav_tileset_to_offsets(Evas_Object *obj, double lon, double lat, double *x, double *y);
void                    e_nav_tileset_from_offsets(Evas_Object *obj, double x, double y, double *lon, double *lat);

#include <e_dbus_proxy.h>
void                    e_nav_tileset_proxy_set(Evas_Object *obj, E_DBus_Proxy *proxy);
E_DBus_Proxy           *e_nav_tileset_proxy_get(Evas_Object *obj);

#endif /* E_NAV_TILESET_H */
