#include <e.h>
#include "e_nav.h"
#include "e_nav_item_neo_me.h"
#include "e_spiralmenu.h"

typedef struct _Neo_Me_Data Neo_Me_Data;

struct _Neo_Me_Data
{
   const char             *name;
   unsigned char           visible : 1;
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


/* FIXME: real menu callbacks */
static void
_e_nav_world_item_cb_menu_1(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *nav;
   double lat, lon;
   
   nav = e_nav_world_item_nav_get(src_obj);
   e_nav_world_item_geometry_get(src_obj, &lat, &lon, NULL, NULL);
   e_nav_coord_set(nav, lat, lon, 0.5);
   e_nav_zoom_set(nav, 0.00001, 0.5);
   e_spiralmenu_deactivate(obj);
}

static void
_e_nav_world_item_cb_menu_2(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb2\n");
   e_spiralmenu_deactivate(obj);
}

static void
_e_nav_world_item_cb_menu_3(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb3\n");
   e_spiralmenu_deactivate(obj);
}

static void
_e_nav_world_item_cb_menu_4(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   printf("cb4\n");
   e_spiralmenu_deactivate(obj);
}

static void
_e_nav_world_item_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Object *om;
   
   om = e_spiralmenu_add(evas);
   e_spiralmenu_theme_source_set(om, data);
   e_spiralmenu_autodelete_set(om, 1);
   e_spiralmenu_deacdelete_set(om, 1);
   e_spiralmenu_source_object_set(om, obj);
   /* FIXME: real menu items */
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Zoom",
			       _e_nav_world_item_cb_menu_1, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Visible",
			       _e_nav_world_item_cb_menu_2, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Auto Join",
			       _e_nav_world_item_cb_menu_3, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, "Information",
			       _e_nav_world_item_cb_menu_4, NULL);
   evas_object_show(om);
   e_spiralmenu_activate(om);
}

static void
_e_nav_world_item_cb_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Neo_Me_Data *neod;
   
   neod = evas_object_data_get(obj, "nav_world_item_neo_me_data");
   if (!neod) return;
   if (neod->name) evas_stringshare_del(neod->name);
   free(neod);
}

/////////////////////////////////////////////////////////////////////////////
Evas_Object *
e_nav_world_item_neo_me_add(Evas_Object *nav, const char *theme_dir, double lat, double lon)
{
   Evas_Object *o;
   Neo_Me_Data *neod;

   /* FIXME: allocate extra data struct for AP properites and attach to the
    * evas object */
   neod = calloc(1, sizeof(Neo_Me_Data));
   if (!neod) return NULL;
   o = _e_nav_world_item_theme_obj_new(evas_object_evas_get(nav), theme_dir,
				       "modules/diversity_nav/neo/me");
   edje_object_part_text_set(o, "e.text.name", "???");
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_world_item_cb_mouse_down,
				  theme_dir);
   e_nav_world_item_add(nav, o);
   e_nav_world_item_type_set(o, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(o, lat, lon, 0, 0);
   e_nav_world_item_scale_set(o, 0);
   e_nav_world_item_update(o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
				  _e_nav_world_item_cb_del, NULL);
   evas_object_data_set(o, "nav_world_item_neo_me_data", neod);
   evas_object_show(o);
   return o;
}

void
e_nav_world_item_neo_me_name_set(Evas_Object *item, const char *name)
{
   Neo_Me_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_me_data");
   if (!neod) return;
   if (neod->name) evas_stringshare_del(neod->name);
   if (name) neod->name = evas_stringshare_add(name);
   else neod->name = NULL;
   edje_object_part_text_set(item, "e.text.name", neod->name);
}

const char *
e_nav_world_item_neo_me_name_get(Evas_Object *item)
{
   Neo_Me_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_me_data");
   if (!neod) return NULL;
   return neod->name;
}

void
e_nav_world_item_neo_me_visible_set(Evas_Object *item, Evas_Bool visible)
{
   Neo_Me_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_me_data");
   if (!neod) return;
   if ((visible && neod->visible) || ((!visible) && (!neod->visible))) return;
   neod->visible = visible;
   if (neod->visible)
     edje_object_signal_emit(item, "e,state,visible", "e");
   else
     edje_object_signal_emit(item, "e,state,invisible", "e");
}

Evas_Bool
e_nav_world_item_neo_me_visible_get(Evas_Object *item)
{
   Neo_Me_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_me_data");
   if (!neod) return 0;
   return neod->visible;
}