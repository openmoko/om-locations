/* e_nav_theme.c -
 *
 * Copyright 2008 OpenMoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "e_nav_theme.h"

#include <Evas.h>
#include <Edje.h>

#define DEFAULT_THEME_NAME "default"

static char *theme_name;
static char *theme_path;

int
e_nav_theme_init(const char *theme)
{
   const char *dot, *slash;

   if (!theme)
     theme = DEFAULT_THEME_NAME;

   if (theme_name)
     return 1;

   dot = strrchr(theme, '.');
   slash = strrchr(theme, '/');

   if (dot || slash)
     {
	theme_path = strdup(theme);
	if (slash)
	  {
	     theme_name = strdup(slash + 1);
	     if (theme_name && dot)
	       {
		  dot = strrchr(theme_name, '.');
		  *((char *) dot) = '\0';
	       }
	  }
	else
	  {
	     int len = dot - theme;

	     theme_name = malloc(len + 1);
	     if (theme_name)
	       {
		  memcpy(theme_name, theme, len);
		  theme_name[len] = '\0';
	       }
	  }
     }
   else
     {
	theme_name = strdup(theme);
	theme_path = malloc(strlen(THEMEDIR) + 1 + strlen(theme) + 4 + 1);
	if (theme_path)
	  sprintf(theme_path, "%s/%s.edj", THEMEDIR, theme);
     }

   if (!theme_name || !theme_path)
     {
	e_nav_theme_shutdown();

	return 0;
     }

   return 1;
}

void
e_nav_theme_shutdown(void)
{
   if (theme_name)
     {
	free(theme_name);
	theme_name = NULL;
     }

   if (theme_path)
     {
	free(theme_path);
	theme_path = NULL;
     }
}

const char *e_nav_theme_name_get(void)
{
   return theme_name;
}

const char *e_nav_theme_path_get(void)
{
   return theme_path;
}

Evas_Object *
e_nav_theme_object_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *obj;

   obj = edje_object_add(e);
   if (!obj)
     return NULL;

   if (!e_nav_theme_object_set(obj, custom_dir, group))
     printf("failed to use group %s in theme %s\n", group, theme_name);

   return obj;
}

int
e_nav_theme_object_set(Evas_Object *obj, const char *custom_dir, const char *group)
{
   if (!theme_path)
     return 0;

   return edje_object_file_set(obj, theme_path, group);
}
