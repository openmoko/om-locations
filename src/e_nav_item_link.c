#include <e.h>
#include "e_nav.h"
#include "e_nav_item_link.h"

typedef struct _Link_Data Link_Data;

struct _Link_Data
{
   Evas_Object            *neo, *ap;
};

static Evas_Object *
_e_nav_world_item_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
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

static void
_e_nav_world_item_cb_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Link_Data *linkd;
   
   linkd = evas_object_data_get(obj, "nav_world_item_link_data");
   if (!linkd) return;
//   if (linkd->name) evas_stringshare_del(linkd->name);
   free(linkd);
}

/////////////////////////////////////////////////////////////////////////////
Evas_Object *
e_nav_world_item_link_add(Evas_Object *nav, const char *theme_dir, Evas_Object *neo, Evas_Object *ap)
{
   Evas_Object *o;
   Link_Data *linkd;

   /* FIXME: allocate extra data struct for AP properites and attach to the
    * evas object */
   linkd = calloc(1, sizeof(Link_Data));
   if (!linkd) return NULL;
   linkd->neo = neo;
   linkd->ap = ap;
   /* FIXME: this needs to be a smart obj */
   o = _e_nav_world_item_theme_obj_new(evas_object_evas_get(nav), theme_dir,
				       "modules/diversity_nav/link");
   e_nav_world_item_add(nav, o);
   e_nav_world_item_type_set(o, E_NAV_WORLD_ITEM_TYPE_LINKED);
   e_nav_world_item_update(o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
				  _e_nav_world_item_cb_del, NULL);
   evas_object_data_set(o, "nav_world_item_link_data", linkd);
   evas_object_show(o);
   return o;
}

/*
void
e_nav_world_item_link_name_set(Evas_Object *item, const char *name)
{
   Link_Data *linkd;
   
   linkd = evas_object_data_get(item, "nav_world_item_link_data");
   if (!linkd) return;
   if (linkd->name) evas_stringshare_del(linkd->name);
   if (name) linkd->name = evas_stringshare_add(name);
   else linkd->name = NULL;
   edje_object_part_text_set(item, "e.text.name", linkd->name);
}

const char *
e_nav_world_item_link_name_get(Evas_Object *item)
{
   Link_Data *linkd;
   
   linkd = evas_object_data_get(item, "nav_world_item_link_data");
   if (!linkd) return NULL;
   return linkd->name;
}
*/
