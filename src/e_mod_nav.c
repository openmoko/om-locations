#include <e.h>
#include "e_mod_nav.h"
#include "e_nav.h"

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

static Evas_Object *
city_add(void *data, Evas *e, const char *theme_dir)
{
   Evas_Object *o;
   
   o = theme_obj_new(e, theme_dir, "modules/diversity_nav/city");
   if (data) edje_object_part_text_set(o, "e.text.name", data);
   return o;
}

void
_e_mod_nav_init(E_Module *m)
{
   E_Zone *zone;
   E_Nav_World_Item *nwi;
   
   mod = m; /* save the module handle */
   zone = e_util_container_zone_number_get(0, 0); /* get the first zone */
   if (!zone) return;
   evas = zone->container->bg_evas;
   
   nav = e_nav_add(evas);
   e_nav_theme_source_set(nav, e_module_dir_get(mod));
   
   nwi = e_nav_world_item_add(nav);
   e_nav_world_item_type_set(nwi, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_add_func_set(nwi, city_add, "Sydney");
   e_nav_world_item_geometry_set(nwi, 151.207114, 33.867139, 32, 32);
   e_nav_world_item_scale_set(nwi, 0);
   e_nav_world_item_update(nwi);
   
   nwi = e_nav_world_item_add(nav);
   e_nav_world_item_type_set(nwi, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_add_func_set(nwi, city_add, "Taipei");
   e_nav_world_item_geometry_set(nwi, 121.549644, -25.046414, 32, 32);
   e_nav_world_item_scale_set(nwi, 0);
   e_nav_world_item_update(nwi);
   
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
