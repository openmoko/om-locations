#ifndef E_NAV_ITEM_AP_H
#define E_NAV_ITEM_AP_H
#include "e_nav_dbus.h"

typedef enum _E_Nav_Item_Ap_Key_Type
{
   E_NAV_ITEM_AP_KEY_TYPE_NONE,
     E_NAV_ITEM_AP_KEY_TYPE_WEP,
     E_NAV_ITEM_AP_KEY_TYPE_WPA
} E_Nav_Item_Ap_Key_Type;

Evas_Object            *e_nav_world_item_ap_add(Evas_Object *nav, const char *theme_dir, Object_Proxy *proxy, double lat, double lon);
void                    e_nav_world_item_ap_essid_set(Evas_Object *item, const char *essid);
const char             *e_nav_world_item_ap_essid_get(Evas_Object *item);
void                    e_nav_world_item_ap_key_type_set(Evas_Object *item, E_Nav_Item_Ap_Key_Type key);
E_Nav_Item_Ap_Key_Type  e_nav_world_item_ap_key_type_get(Evas_Object *item);
void                    e_nav_world_item_ap_active_set(Evas_Object *item, Evas_Bool active);
Evas_Bool               e_nav_world_item_ap_active_get(Evas_Object *item);
void                    e_nav_world_item_ap_freed_set(Evas_Object *item, Evas_Bool freed);
Evas_Bool               e_nav_world_item_ap_freed_get(Evas_Object *item);
void                    e_nav_world_item_ap_range_set(Evas_Object *item, double range);
double                  e_nav_world_item_ap_range_get(Evas_Object *item);
    
#endif
