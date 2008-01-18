#include "e_nav.h"
#include "e_mod_nav.h"

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

int
shutdown(void *data, int type, void *event)
{
   ecore_main_loop_quit();

   return 1;
}

int
main(int argc, char **argv)
{
   Ecore_Evas *ecore_evas;
   Evas *evas;

   bindtextdomain(PACKAGE, LOCALEDIR);
   bind_textdomain_codeset(PACKAGE, "UTF-8");

   if (!ecore_init()) return -1;
   if (!ecore_evas_init()) return -1;
   if (!edje_init()) return -1;

   ecore_app_args_set(argc, (const char **) argv);
   ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, shutdown, NULL);

   ecore_evas = ecore_evas_software_x11_new(NULL, 0, 0, 0, 480, 640);
   if (!ecore_evas) return -1;

   ecore_evas_title_set(ecore_evas, PACKAGE_NAME);
   ecore_evas_callback_delete_request_set(ecore_evas, (void (*)(Ecore_Evas *)) shutdown);
   ecore_evas_size_min_set(ecore_evas, 480, 640);
   ecore_evas_size_max_set(ecore_evas, 480, 640);

   evas = ecore_evas_get(ecore_evas);
   _e_mod_nav_init(evas);

   ecore_evas_show(ecore_evas);

   ecore_main_loop_begin();

   _e_mod_nav_shutdown();
   edje_shutdown();
   ecore_evas_shutdown();
   ecore_shutdown();

   return 0;
}
#endif /* !AS_MODULE */
