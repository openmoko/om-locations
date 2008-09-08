/* e_mod_main.c -
 *
 * Copyright 2007-2008 OpenMoko, Inc.
 * Authored by Carsten Haitzler <raster@openmoko.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <Ecore_Evas.h>
#include <Edje.h>
#include <Etk.h>
#include <unistd.h>

#include "e_mod_nav.h"
#include "e_mod_config.h"
#include "e_nav_misc.h"

#include "config.h"

static int kbd_status = MTPRemoteNone;

static int
exit_handler(void *data, int type, void *event)
{
   _e_mod_nav_shutdown();
   ecore_main_loop_quit();

   return 1;
}

static void
on_delete_request(Ecore_Evas *ee)
{
   _e_mod_nav_shutdown();
   ecore_main_loop_quit();
}

static void
on_resize(Ecore_Evas *ee)
{
   Evas *evas;

   evas = ecore_evas_get(ee);
   _e_mod_nav_update(evas);
}

static void
on_focused_in(Ecore_Evas *ee)
{
   if (kbd_status == MTPRemoteShow)
     e_misc_keyboard_launch();
}

static void
on_focused_out(Ecore_Evas *ee)
{
   kbd_status = e_misc_keyboard_status_get();
   e_misc_keyboard_hide();
}

static void
usage(const char *prog)
{
   printf("Usage: %s [-s] [-t <theme>]\n", prog);
}

int
main(int argc, char **argv)
{
   Ecore_Evas *ee;
   Diversity_Nav_Config *cfg;
   int opt;
   
   bindtextdomain(PACKAGE, LOCALEDIR);
   bind_textdomain_codeset(PACKAGE, "UTF-8");

   if (!ecore_init()) { printf("failed to init ecore\n"); return -1; }
   ecore_event_handler_add(ECORE_EVENT_SIGNAL_HUP, exit_handler, NULL);
   ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, exit_handler, NULL);

   if (!ecore_evas_init()) { printf("failed to init ecore_evas\n"); return -1; }
   if (!edje_init()) { printf("failed to init edje\n"); return -1; }

   if (!etk_init(argc, argv)) {printf("failed to init etk\n"); return -1; }

   /* XXX should we use dgettext? */
   textdomain(PACKAGE);

   ecore_app_args_set(argc, (const char **) argv);

   cfg = dn_config_new();

   while ((opt = getopt(argc, argv, "st:")) != -1)
     {
	switch (opt)
	  {
	   case 's':
	      dn_config_int_set(cfg, "#standalone", 1);
	      break;
	   case 't':
	      dn_config_string_set(cfg, "#theme", optarg);
	      break;
	   default:
	      usage(argv[0]);
	      return -1;
	      break;
	  }
     }
   
   if (optind != argc)
     {
	usage(argv[0]);
	return -1;
     }

   ee = ecore_evas_software_x11_16_new(NULL, 0, 0, 0, 480, 640);
   if (!ee)
     ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 480, 640);
   if (!ee) { printf("failed to get ecore_evas\n"); return -1; }

   ecore_evas_title_set(ee, _("Locations"));
   ecore_evas_callback_delete_request_set(ee, on_delete_request);
   ecore_evas_callback_destroy_set(ee, on_delete_request);
   ecore_evas_callback_resize_set(ee, on_resize);

   ecore_evas_callback_focus_in_set(ee, on_focused_in);
   ecore_evas_callback_focus_out_set(ee, on_focused_out);

   _e_mod_nav_init(ecore_evas_get(ee), cfg);
   ecore_evas_show(ee);

   ecore_main_loop_begin();

   ecore_evas_free(ee);

   dn_config_save(cfg);
   dn_config_destroy(cfg);

   etk_shutdown();
   edje_shutdown();
   ecore_evas_shutdown();
   ecore_shutdown();

   return 0;
}
