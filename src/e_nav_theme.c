/* e_nav_theme.c -
 *
 * Copyright 2008 OpenMoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
 *
 * This work is based on e17 project.  See also COPYING.e17.
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


#include <Edje.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "e_nav_theme.h"

#define DEFAULT_THEME "default"
const char *diversity_theme_name = NULL;

void
e_nav_theme_init(const char *theme_name)
{
   diversity_theme_name = theme_name;
   if(!diversity_theme_name)
     diversity_theme_name = strdup(DEFAULT_THEME);
}

const char *
e_nav_theme_name_get(void)
{
   return diversity_theme_name;
}

static int
_e_nav_theme_edje_object_set(Evas_Object *o, const char *category, const char *group)
{
   char buf[PATH_MAX];
   int ok=0;

   if(category==NULL) return ok;
   
   snprintf(buf, sizeof(buf), "%s/%s.edj", THEME_PATH, category);
   ok = edje_object_file_set(o, buf, group);

   printf("OK= %d, category:%s, group:%s\n", ok, category, group);
   return ok;
}

Evas_Object *
e_nav_theme_object_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;

   o = edje_object_add(e);
   if (!_e_nav_theme_edje_object_set(o, diversity_theme_name, group))
     {
        if (custom_dir)
          {
             char buf[PATH_MAX];

             snprintf(buf, sizeof(buf), "%s/%s.edj", custom_dir, DEFAULT_THEME);
             edje_object_file_set(o, buf, group);
          }
     }
   return o;
}

int
e_nav_theme_object_set(Evas_Object *o, const char *custom_dir, const char *group)
{
   int ok=0;  
   if (!_e_nav_theme_edje_object_set(o, diversity_theme_name, group))
     {
	if (custom_dir)
	  {
	     char buf[PATH_MAX];
	     
	     snprintf(buf, sizeof(buf), "%s/%s.edj", custom_dir, DEFAULT_THEME);
	     ok = edje_object_file_set(o, buf, group);
	  }
     }
   return ok;
}

