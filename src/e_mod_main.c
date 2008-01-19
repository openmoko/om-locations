#include "e_nav.h"
#include "e_mod_nav.h"
#include <libintl.h>

#ifdef AS_MODULE
#include "e_mod_main.h"

/* this is needed to advertise a label for the module IN the code (not just
 * the .desktop file) but more specifically the api version it was compiled
 * for so E can skip modules that are compiled for an incorrect API version
 * safely) */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION, "Diversity Navigator"
};

/* called first thing when E inits the module */
EAPI void *
e_modapi_init(E_Module *m) 
{
   char buf[PATH_MAX];
   
   snprintf(buf, sizeof(buf), "%s/locale", THEME_PATH);
   bindtextdomain(PACKAGE, buf);
   bind_textdomain_codeset(PACKAGE, "UTF-8");
   
   _e_mod_nav_init(m);
   
   return m; /* return NULL on failure, anything else on success. the pointer
	      * returned will be set as m->data for convenience tracking */
}

/* called on module shutdown - should clean up EVERYTHING or we leak */
EAPI int
e_modapi_shutdown(E_Module *m) 
{
   _e_mod_nav_shutdown();
   return 1; /* 1 for success, 0 for failure */
}

/* called by E when it thinks this module shoudl go save any config it has */
EAPI int
e_modapi_save(E_Module *m) 
{
   /* called to save config - none currently */
   return 1; /* 1 for success, 0 for failure */
}
#else /* AS_MODULE */

static void
on_delete_request(Ecore_Evas *ee)
{
   _e_mod_nav_shutdown();

   ecore_main_loop_quit();
}

static void
on_show_or_resize(Ecore_Evas *ee)
{
   Evas *evas;

   _e_mod_nav_shutdown();
   evas = ecore_evas_get(ee);
   _e_mod_nav_init(evas);
}

int
main(int argc, char **argv)
{
   Ecore_Evas *ee;

   bindtextdomain(PACKAGE, LOCALEDIR);
   bind_textdomain_codeset(PACKAGE, "UTF-8");

   if (!ecore_init()) { printf("failed to init ecore\n"); return -1; }
   if (!ecore_evas_init()) { printf("failed to init ecore_evas\n"); return -1; }
   if (!edje_init()) { printf("failed to init edje\n"); return -1; }

   ecore_app_args_set(argc, (const char **) argv);

   ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 480, 640);
   if (!ee) { printf("failed to get ecore_evas\n"); return -1; }

   ecore_evas_title_set(ee, PACKAGE_NAME);
   ecore_evas_callback_delete_request_set(ee, on_delete_request);
   ecore_evas_callback_show_set(ee, on_show_or_resize);
   ecore_evas_callback_resize_set(ee, on_show_or_resize);

   ecore_evas_show(ee);

   ecore_main_loop_begin();

   ecore_evas_free(ee);

   edje_shutdown();
   ecore_evas_shutdown();
   ecore_shutdown();

   return 0;
}
#endif /* !AS_MODULE */
