#include <e.h>
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
   bindtextdomain(PACKAGE, LOCALEDIR);
   bind_textdomain_codeset(PACKAGE, "UTF-8");
   
   return m; /* return NULL on failure, anything else on success. the pointer
	      * returned will be set as m->data for convenience tracking */
}

/* called on module shutdown - should clean up EVERYTHING or we leak */
EAPI int
e_modapi_shutdown(E_Module *m) 
{
   return 1; /* 1 for success, 0 for failure */
}

/* called by E when it thinks this module shoudl go save any config it has */
EAPI int
e_modapi_save(E_Module *m) 
{
   /* called to save config - none currently */
   return 1; /* 1 for success, 0 for failure */
}

/* this is now deprecated - leaving in as a #if for now, will go soon */
#if E_MODULE_API_VERSION <= 6
EAPI int
e_modapi_about(E_Module *m) 
{
   e_module_dialog_show(m, D_("Diversity"), D_("Navigator"));
   return 1;
}
#endif
