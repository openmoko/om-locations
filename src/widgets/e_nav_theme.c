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
