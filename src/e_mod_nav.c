#include <e.h>
#include "e_mod_nav.h"
#include "e_nav.h"

/* create (and destroy) a nav object on the desktop bg */
/* setup and teardown */
static E_Module *mod = NULL;
static Evas_Object *nav = NULL;
static Evas *evas = NULL;

void
_e_mod_nav_init(E_Module *m)
{
   E_Zone *zone;
   
   mod = m; /* save the module handle */
   zone = e_util_container_zone_number_get(0, 0); /* get the first zone */
   if (!zone) return;
   evas = zone->container->bg_evas;
   
   nav = e_nav_add(evas);
   e_nav_theme_source_set(nav, e_module_dir_get(mod));
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
