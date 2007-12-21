#include <E_DBus.h>
#include <e.h>
#include "e_mod_nav.h"
#include "e_nav.h"
#include "e_spiralmenu.h"
#include "e_nav_item_ap.h"
#include "e_nav_item_neo_me.h"
#include "e_nav_item_neo_other.h"
#include "e_nav_item_link.h"
#include "e_nav_dbus.h"

/* FIXME: need objects:
 * 
 * link object
 * signal node object
 * 
 * ? user map object
 * ? map note object
 * ? user icon object
 * 
 */

/* create (and destroy) a nav object on the desktop bg */
/* setup and teardown */
static E_Module *mod = NULL;
static Evas_Object *nav = NULL;
static Evas *evas = NULL;

static Evas_Object *
theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;
   
   o = edje_object_add(e);
   if (!e_theme_edje_object_set(o, "base/theme/modules/diversity_nav", group))
     {
	if (custom_dir)
	  {
	     char buf[PATH_MAX];
	     
	     snprintf(buf, sizeof(buf), "%s/diversity_nav.edj", custom_dir);
	     edje_object_file_set(o, buf, group);
	  }
     }
   return o;
}

static void
city_menu_cb_1(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb1\n");
   e_spiralmenu_deactivate(obj);
}

static void
city_menu_cb_2(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb2\n");
   e_spiralmenu_deactivate(obj);
}

static void
city_menu_cb_3(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb3\n");
   e_spiralmenu_deactivate(obj);
}

static void
city_menu_cb_4(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb4\n");
   e_spiralmenu_deactivate(obj);
}

static void
city_menu_cb_5(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb5\n");
   e_spiralmenu_deactivate(obj);
}

static void
city_menu_cb_6(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb6\n");
   e_spiralmenu_deactivate(obj);
}

static void
city_mouse_cb_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *om;
   
   om = e_spiralmenu_add(evas);
   e_spiralmenu_theme_source_set(om, data);
   e_spiralmenu_autodelete_set(om, 1);
   e_spiralmenu_deacdelete_set(om, 1);
   e_spiralmenu_source_object_set(om, obj);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Silly", city_menu_cb_1, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Fish", city_menu_cb_2, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Onion", city_menu_cb_3, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Eggplant", city_menu_cb_4, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Potatoe", city_menu_cb_5, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Plum", city_menu_cb_6, NULL);
   evas_object_show(om);
   e_spiralmenu_activate(om);
   printf("menu activate for %p = %p\n", obj, om);
}

static void
map_resize(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Coord w, h;
   
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   evas_object_image_fill_set(obj, 0, 0, w, h);
}

void
_e_mod_nav_init(E_Module *m)
{
   E_Zone *zone;
   Evas_Object *nwi;
   
   mod = m; /* save the module handle */
   zone = e_util_container_zone_number_get(0, 0); /* get the first zone */
   if (!zone) return;
   evas = zone->container->bg_evas;
   
   nav = e_nav_add(evas);
   e_nav_theme_source_set(nav, e_module_dir_get(mod));

   /* testing items */
   nwi = theme_obj_new(evas, e_module_dir_get(mod),
		       "modules/diversity_nav/city");
   evas_object_event_callback_add(nwi, EVAS_CALLBACK_MOUSE_DOWN,
				  city_mouse_cb_down,
				  e_module_dir_get(mod));
   edje_object_part_text_set(nwi, "e.text.name", "Sydney");
   e_nav_world_item_add(nav, nwi);
   e_nav_world_item_type_set(nwi, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(nwi,
				 151.205907, 33.875938,
				 32, 32);
   e_nav_world_item_scale_set(nwi, 0);
   e_nav_world_item_update(nwi);
   evas_object_show(nwi);
   
   nwi = theme_obj_new(evas, e_module_dir_get(mod),
		       "modules/diversity_nav/city");
   evas_object_event_callback_add(nwi, EVAS_CALLBACK_MOUSE_DOWN,
				  city_mouse_cb_down,
				  e_module_dir_get(mod));
   edje_object_part_text_set(nwi, "e.text.name", "Taipei");
   e_nav_world_item_add(nav, nwi);
   e_nav_world_item_type_set(nwi, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(nwi,
				 121.549644, -25.046414,
				 32, 32);
   e_nav_world_item_scale_set(nwi, 0);
   e_nav_world_item_update(nwi);
   evas_object_show(nwi);
   
   nwi = theme_obj_new(evas, e_module_dir_get(mod),
		       "modules/diversity_nav/city");
   evas_object_event_callback_add(nwi, EVAS_CALLBACK_MOUSE_DOWN,
				  city_mouse_cb_down,
				  e_module_dir_get(mod));
   edje_object_part_text_set(nwi, "e.text.name", "Kings Cross");
   e_nav_world_item_add(nav, nwi);
   e_nav_world_item_type_set(nwi, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(nwi, 
				 151.223588, 33.873622,
				 32, 32);
   e_nav_world_item_scale_set(nwi, 0);
   e_nav_world_item_update(nwi);
   evas_object_show(nwi);
   
   nwi = evas_object_image_add(evas);
   evas_object_image_file_set(nwi, "/home/raster/sydney_city_map.png", NULL);
   evas_object_image_smooth_scale_set(nwi, 0);
   evas_object_event_callback_add(nwi, EVAS_CALLBACK_RESIZE, map_resize, NULL);
   e_nav_world_item_add(nav, nwi);
   e_nav_world_item_type_set(nwi, E_NAV_WORLD_ITEM_TYPE_WALLPAPER);
   e_nav_world_item_geometry_set(nwi, 
				 151.205907, 33.875938, 
				 2 * (151.232171 - 151.205907), 
				 (928 * 2 * (151.232171 - 151.205907)) / 1606);
   e_nav_world_item_scale_set(nwi, 1);
   e_nav_world_item_update(nwi);
   evas_object_show(nwi);

    World_Proxy* proxy = world_proxy_fill(); 
    if(proxy==NULL) {
        printf("!! proxy==NULL\n");
        return;
    }
    proxy->viewport_add_func(121.000000, -25.000000, 122.000000, -24.000000);  
 
   /* test AP object */
   nwi = e_nav_world_item_ap_add(nav, e_module_dir_get(mod),
				 151.220000, 33.874000);
   e_nav_world_item_ap_essid_set(nwi, "Office");
   e_nav_world_item_ap_key_type_set(nwi, E_NAV_ITEM_AP_KEY_TYPE_NONE);
   e_nav_world_item_ap_range_set(nwi, 100 NAV_UNIT_M);

   /* test NEO OTHER object */
   nwi = e_nav_world_item_neo_other_add(nav, e_module_dir_get(mod), NULL,
				     151.215000, 33.871000);
   e_nav_world_item_neo_other_name_set(nwi, "Sean");
   nwi = e_nav_world_item_neo_other_add(nav, e_module_dir_get(mod), NULL,
				     151.213000, 33.874000);
   e_nav_world_item_neo_other_name_set(nwi, "Olv");
   
   /* test NEO ME object */
   nwi = e_nav_world_item_neo_me_add(nav, e_module_dir_get(mod),
				     151.210000, 33.870000);
   e_nav_world_item_neo_me_name_set(nwi, "Me");
   
   /* start off at a zoom level and location instantly */
   e_nav_zoom_set(nav, 0.0001, 0.0);
   e_nav_coord_set(nav, 151.205907, 33.875938, 0.0);
   
   /* put the nav object somewhere useful at a decent size and show it */
   evas_object_move(nav, zone->x, zone->y);
   evas_object_resize(nav, zone->w, zone->h);
   evas_object_show(nav);
}

void
_e_mod_nav_shutdown(void)
{
   if (!nav) return;
   evas_object_del(nav);
   nav = NULL;
}

void e_nav_object_add(Object_Proxy* proxy)
{
   Evas_Object *nwi;
   // get type, position, geometry, etc. infomation
   // add a world item
   //nwi = e_nav_world_item_neo_other_add(nav, e_module_dir_get(mod), proxy,
     //                151.215000, 33.871000); 
} 

