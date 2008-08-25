/* e_nav_item_neo_other.c -
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

#include "e_nav.h"
#include "e_nav_theme.h"
#include "e_nav_item_neo_other.h"
#include "e_spiralmenu.h"

typedef struct _Neo_Other_Data Neo_Other_Data;

struct _Neo_Other_Data
{
   const char             *name;
   const char             *phone;
   const char             *alias;
   const char             *twitter;
   int                     accuracy;
   Diversity_Bard         *bard;
};


/* FIXME: real menu callbacks */
static void
_e_nav_world_item_cb_menu_1(void *data, Evas_Object *obj, Evas_Object *src_obj)
{
   Evas_Object *nav;
   double lon, lat;
   
   nav = e_nav_world_item_nav_get(src_obj);
   e_nav_world_item_geometry_get(src_obj, &lon, &lat, NULL, NULL);
   e_nav_coord_set(nav, lon, lat, 0.5);
   e_nav_span_set(nav, E_NAV_SPAN_FROM_METERS(400), 0.5);
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
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, _("Zoom"),
			       _e_nav_world_item_cb_menu_1, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, _("Call"),
			       _e_nav_world_item_cb_menu_2, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, _("Message"),
			       _e_nav_world_item_cb_menu_3, NULL);
   e_spiralmenu_theme_item_add(om, "modules/diversity_nav/item", 48, _("Information"),
			       _e_nav_world_item_cb_menu_4, NULL);
   evas_object_show(om);
   e_spiralmenu_activate(om);
}

static void
_e_nav_world_item_cb_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Neo_Other_Data *neod;
   
   neod = evas_object_data_get(obj, "nav_world_item_neo_other_data");
   if (!neod) return;
   if (neod->name) evas_stringshare_del(neod->name);
   if (neod->phone) evas_stringshare_del(neod->phone);
   if (neod->alias) evas_stringshare_del(neod->alias);
   if (neod->twitter) evas_stringshare_del(neod->twitter);
   free(neod);
}

/////////////////////////////////////////////////////////////////////////////
Evas_Object *
e_nav_world_item_neo_other_add(Evas_Object *nav, const char *theme_dir, double lon, double lat, Diversity_Object *bard)
{
   Evas_Object *o;
   Neo_Other_Data *neod;

   neod = calloc(1, sizeof(Neo_Other_Data));
   if (!neod) return NULL;
   neod->bard = (Diversity_Bard *) bard;
   o = e_nav_theme_object_new(evas_object_evas_get(nav), theme_dir,
				       "modules/diversity_nav/neo/other");
   edje_object_part_text_set(o, "e.text.name", _("???"));
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_world_item_cb_mouse_down,
				  theme_dir);
   e_nav_world_item_add(nav, o);
   e_nav_world_item_type_set(o, E_NAV_WORLD_ITEM_TYPE_ITEM);
   e_nav_world_item_geometry_set(o, lon, lat, 0, 0);
   e_nav_world_item_scale_set(o, 0);
   e_nav_world_item_update(o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
				  _e_nav_world_item_cb_del, NULL);
   evas_object_data_set(o, "nav_world_item_neo_other_data", neod);
   //evas_object_show(o);
   return o;
}

void
e_nav_world_item_neo_other_name_set(Evas_Object *item, const char *name)
{
   Neo_Other_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_other_data");
   if (!neod) return;
   if (neod->name) evas_stringshare_del(neod->name);
   if (name) neod->name = evas_stringshare_add(name);
   else neod->name = NULL;
   edje_object_part_text_set(item, "e.text.name", neod->name);
}

const char *
e_nav_world_item_neo_other_name_get(Evas_Object *item)
{
   Neo_Other_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_other_data");
   if (!neod) return NULL;
   return neod->name;
}

void
e_nav_world_item_neo_other_phone_set(Evas_Object *item, const char *phone)
{
   Neo_Other_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_other_data");
   if (!neod) return;
   if (neod->phone) evas_stringshare_del(neod->phone);
   if (phone) neod->phone = evas_stringshare_add(phone);
   else neod->phone = NULL;
}

const char *
e_nav_world_item_neo_other_phone_get(Evas_Object *item)
{
   Neo_Other_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_other_data");
   if (!neod) return NULL;
   return neod->phone;
}

void
e_nav_world_item_neo_other_alias_set(Evas_Object *item, const char *alias)
{
   Neo_Other_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_other_data");
   if (!neod) return;
   if (neod->alias) evas_stringshare_del(neod->alias);
   if (alias) neod->alias = evas_stringshare_add(alias);
   else neod->alias = NULL;
}

const char *
e_nav_world_item_neo_other_alias_get(Evas_Object *item)
{
   Neo_Other_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_other_data");
   if (!neod) return NULL;
   return neod->alias;
}

void
e_nav_world_item_neo_other_twitter_set(Evas_Object *item, const char *twitter)
{
   Neo_Other_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_other_data");
   if (!neod) return;
   if (neod->twitter) evas_stringshare_del(neod->twitter);
   if (twitter) neod->twitter = evas_stringshare_add(twitter);
   else neod->twitter = NULL;
}

const char *
e_nav_world_item_neo_other_twitter_get(Evas_Object *item)
{
   Neo_Other_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_other_data");
   if (!neod) return NULL;
   return neod->twitter;
}

void
e_nav_world_item_neo_other_accuracy_set(Evas_Object *item, int accuracy)
{
   Neo_Other_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_other_data");
   if (!neod) return;
   neod->accuracy = accuracy;
   if(neod->accuracy != DIVERSITY_OBJECT_ACCURACY_NONE)
     evas_object_show(item);
}

int
e_nav_world_item_neo_other_accuracy_get(Evas_Object *item)
{
   Neo_Other_Data *neod;
   
   neod = evas_object_data_get(item, "nav_world_item_neo_other_data");
   if (!neod) return DIVERSITY_OBJECT_ACCURACY_NONE;
   return neod->accuracy;
}

Diversity_Bard *
e_nav_world_item_neo_other_bard_get(Evas_Object *item)
{
   Neo_Other_Data *neod;

   if(!item) return NULL;
   neod = evas_object_data_get(item, "nav_world_item_neo_other_data");
   if (!neod) return NULL;
   return neod->bard;
}

const char *
e_nav_world_item_neo_other_path_get(Evas_Object *item)
{
   Diversity_Bard *bard;
   if(!item) return NULL;
   bard = e_nav_world_item_neo_other_bard_get(item); 
   if(!bard) return NULL;
   return diversity_dbus_path_get((Diversity_DBus *)bard);
}

