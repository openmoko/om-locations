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
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "e_nav_theme.h"

static int
e_nav_theme_edje_object_set(Evas_Object *o, const char *category, const char *group)
{  
   char buf[PATH_MAX];
   int ok; 

   snprintf(buf, sizeof(buf), "%s/%s.edj", THEME_PATH, category);
   ok = edje_object_file_set(o, buf, group);

   return ok;
}

int
e_nav_theme_object_set(Evas_Object *o, const char *custom_dir, const char *group)
{
   int ok=0;  
   if (!e_nav_theme_edje_object_set(o, "default", group))
     {
	if (custom_dir)
	  {
	     char buf[PATH_MAX];
	     
	     snprintf(buf, sizeof(buf), "%s/default.edj", custom_dir);
	     ok = edje_object_file_set(o, buf, group);
	  }
     }
   return ok;
}

Evas_Object *
e_nav_theme_object_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;
   
   o = edje_object_add(e);
   if (!e_nav_theme_edje_object_set(o, "default", group))
     {
        if (custom_dir)
          {
             char buf[PATH_MAX];
             
             snprintf(buf, sizeof(buf), "%s/default.edj", custom_dir);
             edje_object_file_set(o, buf, group);
          }
     }
   return o;
}
