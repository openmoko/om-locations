/* e_nav.c -
 *
 * Copyright 2007-2008 Openmoko, Inc.
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
#include "e_ctrl.h"
#include "e_nav_tileset.h"
#include "e_nav_dbus.h"

/* navigator object */
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Nav_World_Block E_Nav_World_Block;
typedef struct _E_Nav_World_Item E_Nav_World_Item;

typedef enum _E_Nav_Movengine_Action
{
   E_NAV_MOVEENGINE_START,
   E_NAV_MOVEENGINE_STOP,
   E_NAV_MOVEENGINE_GO
} E_Nav_Movengine_Action;

struct _E_Nav_World_Item
{
   Evas_Object *obj; // the nav obj this nav item belongs to
   Evas_Object *item; // the tiem object itself
   E_Nav_World_Item_Type type;
   unsigned char scale : 1; // scale item with span or not

   struct {
      double px, py;
      double w, h;
   } geom;
};

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   
   Diversity_World *world;

   /* sorted by stack order */
   Evas_Object     *event;
   Evas_Object     *stacking;
   Evas_Object     *clip;
   Evas_Object     *underlay;

   Evas_Object     *ctrl;
   Evas_Object     *tileset;
   
   /* the list of items in the world as we have been told by the backend */
   Eina_List       *world_items;
   Evas_Object     *neo_me;
   
   /* directory to find theme .edj files from the module - if there is one */
   const char      *dir;

   /* current state - display this currently */
   double           px, py;
   double           span;
   
   /* configured state - the state asked to do */
   struct {
      double           px, py;
      double           span;
   } conf;
   
   /* needed information for animation */
   struct {
      struct {
	 double px, py;
	 double span;
	 double pos_time;
	 double span_time;
      } start, target;
      unsigned char  mouse_down : 1;
      Ecore_Timer   *momentum_animator;
   } anim;
   
   /* needed information for mouse gesture */
   struct {
      struct {
	 Evas_Coord    x, y;
	 double        px, py;
	 double        span;
      } start;
      struct {
	 Evas_Coord    x, y;
	 double        timestamp;
      } history[20];
      Ecore_Timer   *pause_timer;
   } moveng;
};

static void _e_nav_smart_init(void);
static void _e_nav_smart_add(Evas_Object *obj);
static void _e_nav_smart_del(Evas_Object *obj);
static void _e_nav_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_nav_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_nav_smart_show(Evas_Object *obj);
static void _e_nav_smart_hide(Evas_Object *obj);
static void _e_nav_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_nav_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_nav_smart_clip_unset(Evas_Object *obj);

static void _e_nav_cb_event_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_nav_cb_event_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_nav_cb_event_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_nav_cb_event_mouse_wheel(void *data, Evas *evas, Evas_Object *obj, void *event);

static void _e_nav_movengine(Evas_Object *obj, E_Nav_Movengine_Action action, Evas_Coord x, Evas_Coord y);
static void _e_nav_update(Evas_Object *obj);
static void _e_nav_overlay_update(Evas_Object *obj);
static int _e_nav_momentum_calc(Evas_Object *obj, double t);
static void _e_nav_wallpaper_update(Evas_Object *obj);
static int _e_nav_cb_animator_momentum(void *data);
#if 0
static int _e_nav_cb_timer_moveng_pause(void *data);
#endif

static void _e_nav_world_item_free(E_Nav_World_Item *nwi);
static void _e_nav_world_item_move_resize(E_Nav_World_Item *nwi);

static void _e_nav_world_item_cb_item_del(void *data, Evas *evas, Evas_Object *obj, void *event);

#define SMART_NAME "e_nav"
static Evas_Smart *_e_smart = NULL;

Evas_Object *
e_nav_add(Evas *e)
{
   _e_nav_smart_init();

   return evas_object_smart_add(e, _e_smart);
}

void
e_nav_theme_source_set(Evas_Object *obj, const char *custom_dir)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   
   sd->dir = custom_dir;
   sd->event = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->event, obj);
   evas_object_move(sd->event, sd->x, sd->y);
   evas_object_resize(sd->event, sd->w, sd->h);
   evas_object_color_set(sd->event, 0, 0, 0, 0);
   evas_object_clip_set(sd->event, sd->clip);
   evas_object_repeat_events_set(sd->event, 1);
   evas_object_show(sd->event);

   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_cb_event_mouse_down, obj);
   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_UP,
				  _e_nav_cb_event_mouse_up, obj);
   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_MOVE,
				  _e_nav_cb_event_mouse_move, obj);
   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_WHEEL,
				  _e_nav_cb_event_mouse_wheel, obj);

   _e_nav_wallpaper_update(obj);
   _e_nav_overlay_update(obj);
}

void
e_nav_world_set(Evas_Object *obj, void *world)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->world = world;
}

void *
e_nav_world_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, NULL;);

   return sd->world;
}

void
e_nav_world_neo_me_set(Evas_Object *obj, Evas_Object *me)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->neo_me = me;
}

Evas_Object *
e_nav_world_neo_me_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, NULL;);

   return sd->neo_me;
}

void
e_nav_world_ctrl_set(Evas_Object *obj, Evas_Object *ctrl)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   sd->ctrl = ctrl;
}

Evas_Object *
e_nav_world_ctrl_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, NULL;);

   return sd->ctrl;
}

void
e_nav_world_tileset_set(Evas_Object *obj, Evas_Object *nt)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (sd->tileset)
     evas_object_del(sd->tileset);

   sd->tileset = nt;
   if (nt)
     {
	evas_object_smart_member_add(nt, sd->obj);
	evas_object_clip_set(nt, sd->clip);
	evas_object_stack_below(nt, sd->clip);
     }

   _e_nav_update(obj);
}

void
e_nav_pos_set(Evas_Object *obj, double px, double py, double when)
{
   E_Smart_Data *sd;
   double t;
   double off;
   int span;
   
   SMART_CHECK(obj, ;);

   span = sd->conf.span;

   /* keep the world in the screen */
   off = (double) sd->w / 2.0 / span;
   if (px - off < 0.0 || off > 0.5)
     px = off;
   else if (px + off > 1.0)
     px = 1.0 - off;

   off = (double) sd->h / 2.0 / span;
   if (py - off < 0.0 || off > 0.5)
     py = off;
   else if (py + off > 1.0)
     py = 1.0 - off;
   
   if (when == 0.0)
     {
	sd->anim.target.pos_time = 0.0;
	sd->anim.start.pos_time = 0.0;

	sd->px = px;
	sd->py = py;
	sd->conf.px = px;
	sd->conf.py = py;

	_e_nav_update(obj);

	return;
     }

   t = ecore_time_get();
   _e_nav_momentum_calc(obj, t);

   sd->anim.start.pos_time = t;
   sd->anim.start.px = sd->px;
   sd->anim.start.py = sd->py;

   sd->anim.target.px = px;
   sd->anim.target.py = py;
   sd->anim.target.pos_time = sd->anim.start.pos_time + when;

   sd->conf.px = px;
   sd->conf.py = py;

   if (!sd->anim.momentum_animator)
     sd->anim.momentum_animator = ecore_animator_add(_e_nav_cb_animator_momentum,
						 obj);
}

void
e_nav_pos_get(Evas_Object *obj, double *px, double *py)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (px)
     *px = sd->conf.px;

   if (py)
     *py = sd->conf.py;
}

void
e_nav_coord_set(Evas_Object *obj, double lon, double lat, double when)
{
   E_Smart_Data *sd;
   double px, py;
   
   SMART_CHECK(obj, ;);

   if (sd->tileset)
     {
	e_nav_tileset_coord_to_pos(sd->tileset, lon, lat, &px, &py);
	e_nav_pos_set(obj, px, py, when);
     }
}

void
e_nav_coord_get(Evas_Object *obj, double *lon, double *lat)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (sd->tileset)
     {
	e_nav_tileset_coord_from_pos(sd->tileset,
	      sd->conf.px, sd->conf.py, lon, lat);
     }
   else
     {
	if (lon)
	  *lon = 0.0;

	if (lat)
	  *lat = 0.0;
     }
}

double
e_nav_coord_lon_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   double lon;
   
   SMART_CHECK(obj, 0.0;);

   if (sd->tileset)
     e_nav_tileset_coord_from_pos(sd->tileset, sd->conf.px, sd->conf.py, &lon, NULL);
   else
     lon = 0.0;

   return lon;
}

double
e_nav_coord_lat_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   double lat;

   SMART_CHECK(obj, 0.0;);

   if (sd->tileset)
     e_nav_tileset_coord_from_pos(sd->tileset, sd->conf.px, sd->conf.py, NULL, &lat);
   else
     lat = 0.0;

   return lat;
}

void
e_nav_span_set(Evas_Object *obj, int span, double when)
{
   E_Smart_Data *sd;
   double t;

   SMART_CHECK(obj, ;);

   /* XXX keep the world in the screen? */

   if (span > E_NAV_SPAN_MAX)
     span = E_NAV_SPAN_MAX;
   else if (span < E_NAV_SPAN_MIN)
     span = E_NAV_SPAN_MIN;

   if (sd->ctrl)
     e_ctrl_span_drag_value_set(sd->ctrl, span);

   if (when == 0.0)
     {
	sd->anim.target.span_time = 0.0;
	sd->anim.start.span_time = 0.0;
	sd->span = span;
	sd->conf.span = span;
	_e_nav_update(obj);
	return;
     }
   t = ecore_time_get();
   _e_nav_momentum_calc(obj, t);
   sd->anim.start.span_time = t;
   sd->anim.start.span = sd->span;
   sd->anim.target.span = span;
   sd->anim.target.span_time = sd->anim.start.span_time + when;
   sd->conf.span = span;
   if (!sd->anim.momentum_animator)
     sd->anim.momentum_animator = ecore_animator_add(_e_nav_cb_animator_momentum,
						 obj);
}

int
e_nav_span_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0.0;);

   return sd->span;
}
 
void
e_nav_level_set(Evas_Object *obj, int level, double when)
{
   E_Smart_Data *sd;
   int span;

   SMART_CHECK(obj, ;);

   span = e_nav_tileset_level_to_span(sd->tileset, level);

   e_nav_span_set(obj, span, when);
}

int
e_nav_level_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0;);

   if (!sd->tileset)
     return 0;

   return e_nav_tileset_level_from_span(sd->tileset, sd->conf.span);
}

/* world items */
void
e_nav_world_item_add(Evas_Object *obj, Evas_Object *item)
{
   E_Smart_Data *sd;
   E_Nav_World_Item *nwi;
   
   SMART_CHECK(obj, ;);

   nwi = calloc(1, sizeof(E_Nav_World_Item));
   if (!nwi)
     return;

   nwi->obj = obj;
   nwi->item = item;
   evas_object_data_set(item, "nav_world_item", nwi);
   evas_object_event_callback_add(item, EVAS_CALLBACK_DEL,
	 _e_nav_world_item_cb_item_del, NULL);
   sd->world_items = eina_list_append(sd->world_items, item);
   evas_object_smart_member_add(nwi->item, nwi->obj);
   evas_object_clip_set(nwi->item, sd->clip);

   nwi->type = E_NAV_WORLD_ITEM_TYPE_ITEM;
   evas_object_stack_below(nwi->item, sd->stacking);
}

void
e_nav_world_item_delete(Evas_Object *obj, Evas_Object *item)
{
   E_Smart_Data *sd;
   E_Nav_World_Item *nwi;

   SMART_CHECK(obj, ;);

   nwi = evas_object_data_del(obj, "nav_world_item");
   if (!nwi)
     return;

   sd->world_items = eina_list_remove(sd->world_items, item);

   evas_object_event_callback_del(item, EVAS_CALLBACK_DEL,
	 _e_nav_world_item_cb_item_del);

   _e_nav_world_item_free(nwi);
}

void
e_nav_world_item_type_set(Evas_Object *item, E_Nav_World_Item_Type type)
{
   E_Smart_Data *sd;
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi)
     return;

   if (nwi->type != type)
     {
	SMART_CHECK(nwi->obj, ;);

	nwi->type = type;

	if (nwi->type == E_NAV_WORLD_ITEM_TYPE_WALLPAPER)
	  evas_object_stack_above(nwi->item, sd->clip);
	else if (nwi->type == E_NAV_WORLD_ITEM_TYPE_ITEM)
	  evas_object_stack_below(nwi->item, sd->stacking);
	else if (nwi->type == E_NAV_WORLD_ITEM_TYPE_OVERLAY)
	  evas_object_stack_above(nwi->item, sd->stacking);
	else if (nwi->type == E_NAV_WORLD_ITEM_TYPE_LINKED)
	  evas_object_stack_above(nwi->item, sd->stacking);
     }
}

E_Nav_World_Item_Type
e_nav_world_item_type_get(Evas_Object *item)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return 0;
   return nwi->type;
}

Evas_Object *
e_nav_world_item_nav_get(Evas_Object *item)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return NULL;
   return nwi->obj;
}

void
e_nav_world_item_scale_set(Evas_Object *item, Evas_Bool scale)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return;
   nwi->scale = scale;
}

Evas_Bool
e_nav_world_item_scale_get(Evas_Object *item)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return 0;
   return nwi->scale;
}

void
e_nav_world_item_coord_set(Evas_Object *item, double lon, double lat)
{
   E_Smart_Data *sd;
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi)
     return;

   SMART_CHECK(nwi->obj, ;);

   if (!sd->tileset)
     {
	nwi->geom.px = 0.5;
	nwi->geom.py = 0.5;

	return;
     }

   e_nav_tileset_coord_to_pos(sd->tileset,
	 lon, lat, &nwi->geom.px, &nwi->geom.py);
}

void
e_nav_world_item_coord_get(Evas_Object *item, double *lon, double *lat)
{
   E_Smart_Data *sd;
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi)
     return;

   SMART_CHECK(nwi->obj, ;);

   e_nav_tileset_coord_from_pos(sd->tileset,
	 nwi->geom.px, nwi->geom.py, lon, lat);
}

void
e_nav_world_item_size_set(Evas_Object *item, double w, double h)
{
   E_Smart_Data *sd;
   E_Nav_World_Item *nwi;
   double lon, lat;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi)
     return;

   SMART_CHECK(nwi->obj, ;);

   if (!nwi->scale)
     {
	nwi->geom.w = w;
	nwi->geom.h = h;

	return;
     }
   else if (!sd->tileset)
     {
	nwi->geom.w = 0.0;
	nwi->geom.h = 0.0;

	return;
     }

   e_nav_tileset_coord_from_pos(sd->tileset,
	 nwi->geom.px, nwi->geom.py, &lon, &lat);

   lon += w;
   lat += h;

   e_nav_tileset_coord_to_pos(sd->tileset,
	 lon, lat, &nwi->geom.w, &nwi->geom.h);
}

void
e_nav_world_item_size_get(Evas_Object *item, double *w, double *h)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi)
     return;

   if (w)
     *w = nwi->geom.w;

   if (h)
     *h = nwi->geom.h;
}

void
e_nav_world_item_geometry_set(Evas_Object *item, double x, double y, double w, double h)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return;

   e_nav_world_item_coord_set(item, x, y);
   e_nav_world_item_size_set(item, w, h);
}

void
e_nav_world_item_geometry_get(Evas_Object *item, double *x, double *y, double *w, double *h)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return;

   e_nav_world_item_coord_get(item, x, y);
   e_nav_world_item_size_get(item, w, h);
}

void
e_nav_world_item_update(Evas_Object *item)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return;
   _e_nav_world_item_move_resize(nwi);
}

void
e_nav_world_item_lower(Evas_Object *item)
{
   E_Nav_World_Item *nwi;
   E_Smart_Data *sd;

   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi || nwi->type != E_NAV_WORLD_ITEM_TYPE_ITEM)
     return;

   sd = evas_object_smart_data_get(nwi->obj);

   /* should stack above TYPE_WALLPAPER */ 
   evas_object_stack_above(nwi->item, sd->clip);
}

void
e_nav_world_item_raise(Evas_Object *item)
{
   E_Nav_World_Item *nwi;
   E_Smart_Data *sd;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi || nwi->type != E_NAV_WORLD_ITEM_TYPE_ITEM)
     return;

   sd = evas_object_smart_data_get(nwi->obj);

   evas_object_stack_below(nwi->item, sd->stacking);
}

void
e_nav_world_item_focus(Evas_Object *item)
{
   E_Nav_World_Item *nwi;

   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi || nwi->type != E_NAV_WORLD_ITEM_TYPE_ITEM)
     return;

   e_nav_world_item_raise(item);
   e_nav_pos_set(nwi->obj, nwi->geom.px, nwi->geom.py, 0.0);
}

/* internal calls */
static void
_e_nav_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
	       EVAS_SMART_CLASS_VERSION,
	       _e_nav_smart_add,
	       _e_nav_smart_del,
	       _e_nav_smart_move,
	       _e_nav_smart_resize,
	       _e_nav_smart_show,
	       _e_nav_smart_hide,
	       _e_nav_smart_color_set,
	       _e_nav_smart_clip_set,
	       _e_nav_smart_clip_unset,
	       
	       NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_nav_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   sd->obj = obj;
   
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   
   sd->underlay = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->underlay, obj);
   evas_object_move(sd->underlay, sd->x, sd->y);
   evas_object_resize(sd->underlay, sd->w, sd->h);
   evas_object_clip_set(sd->underlay, sd->clip);
   evas_object_color_set(sd->underlay, 180, 180, 180, 255);
   evas_object_lower(sd->underlay);
   evas_object_show(sd->underlay);

   sd->stacking = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->stacking, obj);
   evas_object_move(sd->stacking, sd->x, sd->y);
   evas_object_resize(sd->stacking, sd->w, sd->h);
   evas_object_clip_set(sd->stacking, sd->clip);
   
   evas_object_smart_data_set(obj, sd);
   
   sd->px = 0.0;
   sd->py = 0.0;
   sd->span = 640;
   
   sd->conf.px = sd->px;
   sd->conf.py = sd->py;
   sd->conf.span = sd->span;
}

static void
_e_nav_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_del(sd->underlay);
   evas_object_del(sd->clip);
   evas_object_del(sd->stacking);
   evas_object_del(sd->event);
   if (sd->anim.momentum_animator) ecore_animator_del(sd->anim.momentum_animator);
   if (sd->moveng.pause_timer) ecore_timer_del(sd->moveng.pause_timer);
   while (sd->world_items)
     {
	E_Nav_World_Item *nwi;
	
	nwi = evas_object_data_get(sd->world_items->data, "nav_world_item");
	if (nwi)
	  {
	     evas_object_event_callback_del(nwi->item, EVAS_CALLBACK_DEL,
					    _e_nav_world_item_cb_item_del);
	     evas_object_del(nwi->item);
	     _e_nav_world_item_free(nwi);
	     sd->world_items = eina_list_remove_list(sd->world_items,
						     sd->world_items);
	  }
     }

   e_nav_world_tileset_set(obj, NULL);

   free(sd);
}
                    
static void
_e_nav_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;
   evas_object_move(sd->underlay, sd->x, sd->y);
   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_move(sd->stacking, sd->x, sd->y);
   evas_object_move(sd->event, sd->x, sd->y);

   if (sd->tileset)
     evas_object_move(sd->tileset, sd->x, sd->y);
   
   _e_nav_update(obj);
}

static void
_e_nav_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->underlay, sd->w, sd->h);
   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_resize(sd->stacking, sd->w, sd->h);
   evas_object_resize(sd->event, sd->w, sd->h);

   if (sd->tileset)
     evas_object_resize(sd->tileset, sd->w, sd->h);

   /* this checks pos boundaries and update e_nav */
   e_nav_pos_set(obj, sd->px, sd->py, 0.0);
}

static void
_e_nav_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_nav_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_nav_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_nav_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_nav_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
} 

static void
_e_nav_cb_event_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Down *ev = event;
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(data);
   if (ev->button == 1)
     {
	sd->anim.mouse_down = 1;
	_e_nav_movengine(data, E_NAV_MOVEENGINE_START, ev->canvas.x, ev->canvas.y);
     }
}

static void
_e_nav_cb_event_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Up *ev = event;
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(data);
   if (ev->button == 1 && sd->anim.mouse_down)
     {
	_e_nav_movengine(data, E_NAV_MOVEENGINE_STOP, ev->canvas.x, ev->canvas.y);
	sd->anim.mouse_down = 0;
     }
}

static void
_e_nav_cb_event_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Move *ev = event;
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(data);
   if (sd->anim.mouse_down)
     _e_nav_movengine(data, E_NAV_MOVEENGINE_GO, ev->cur.canvas.x, ev->cur.canvas.y);
}

static void
_e_nav_cb_event_mouse_wheel(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Wheel *ev = event;
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(data);
   if (ev->direction == 0)
     {
	if (ev->z > 0)
	  e_nav_span_set(data, sd->conf.span / 2, 1.0);
	else
	  e_nav_span_set(data, sd->conf.span * 2, 1.0);
     }
}

static void
_e_nav_movengine_plain(Evas_Object *obj, E_Nav_Movengine_Action action, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   double when = 0.0;
   double px, py;

   sd = evas_object_smart_data_get(obj);

   if (action == E_NAV_MOVEENGINE_START)
     {
	sd->moveng.start.x = x;
	sd->moveng.start.y = y;
	sd->moveng.start.px = sd->px;
	sd->moveng.start.py = sd->py;
	sd->moveng.start.span = sd->conf.span;

	e_nav_pos_set(obj, sd->px, sd->py, 0.0);

	return;
     }
   
   px = sd->moveng.start.px + (double) (sd->moveng.start.x - x) / sd->conf.span;
   py = sd->moveng.start.py + (double) (sd->moveng.start.y - y) / sd->conf.span;

   if (action == E_NAV_MOVEENGINE_GO)
     {
	/* set `when' to frametime to minimize screen update */
	when = ecore_animator_frametime_get();

	if (sd->ctrl)
	  e_ctrl_follow_set(sd->ctrl, FALSE);
     }

   e_nav_pos_set(obj, px, py, when);
}

static void
_e_nav_movengine(Evas_Object *obj, E_Nav_Movengine_Action action, Evas_Coord x, Evas_Coord y)
{
   /* TODO provide parameters instead of calling another engine */
#if 0
   E_Smart_Data *sd;
   double t;
   int i, count;
   Evas_Coord vx1, vy1, vx2, vy2;
   Evas_Coord dist;
   double lon, lat;
   double zoomout = 0.0;

   sd = evas_object_smart_data_get(obj);
   if (action == E_NAV_MOVEENGINE_START)
     {
	sd->moveng.start.x = x;
	sd->moveng.start.y = y;
	sd->moveng.start.lon = sd->lon;
	sd->moveng.start.lat = sd->lat;
	sd->moveng.start.zoom = sd->conf.zoom;
	memset(&(sd->moveng.history[0]), 0, sizeof(sd->moveng.history[0]) * 20);
     }
   t = ecore_time_get();
   memmove(&(sd->moveng.history[1]), &(sd->moveng.history[0]),
	   sizeof(sd->moveng.history[0]) * 19);
   sd->moveng.history[0].timestamp = t;
   sd->moveng.history[0].x = x;
   sd->moveng.history[0].y = y;
 
   vx1 = 0;
   vy1 = 0;
   count = 0;
   for (i = 0; i < 20; i++)
     {
	if ((t - sd->moveng.history[i].timestamp) < 0.2)
	  {
	     vx1 += sd->moveng.history[i].x;
	     vy1 += sd->moveng.history[i].y;
	     count++;
	  }
     }
   lon = sd->lon;
   lat = sd->lat;
   if (action == E_NAV_MOVEENGINE_START)
     {
	e_nav_coord_set(obj,
			lon, lat,
			2.0);
	e_nav_zoom_set(obj,
		       sd->moveng.start.zoom,
		       1.0);
     }
   
   if (sd->moveng.pause_timer) ecore_timer_del(sd->moveng.pause_timer);
   sd->moveng.pause_timer = ecore_timer_add(0.2,
					    _e_nav_cb_timer_moveng_pause,
					    obj);
   /* not enough points. useless vector info */
   if (count < 2)
     {
	if (action == E_NAV_MOVEENGINE_STOP)
	  {
	     e_nav_coord_set(obj, lon, lat, 2.0);
	     e_nav_zoom_set(obj, sd->moveng.start.zoom, 1.0);
	  }
	return;
     }
   vx1 /= count;
   vy1 /= count;
   vx2 = x;
   vy2 = y;
   dist = sqrt(((vx2 - vx1) * (vx2 - vx1)) + ((vy2 - vy1) * (vy2 - vy1)));

   zoomout = (double)(dist - 80) / 40;
   if (zoomout < 0.0) zoomout = 0.0;
   zoomout = zoomout * zoomout;
   
   if (action == E_NAV_MOVEENGINE_STOP)
     {
	double lon_off, lat_off;

	if (dist > 40)
	  {
	     _e_nav_from_offsets(obj,
				 (vx2 - vx1) * 5.0,
				 (vy2 - vy1) * 5.0,
				 &lon_off, &lat_off);
	     e_nav_coord_set(obj, lon - lon_off,
			     lat - lat_off,
			     2.0 + (zoomout / 16.0));
	  }
	else
	  e_nav_coord_set(obj, lon, lat,
			  2.0 + (zoomout / 16.0));
	e_nav_zoom_set(obj, sd->moveng.start.zoom, 1.0 + (zoomout / 4.0));
	if (sd->moveng.pause_timer) ecore_timer_del(sd->moveng.pause_timer);
	sd->moveng.pause_timer = NULL;
     }
   else
     {
	double lon_off, lat_off;

	_e_nav_from_offsets(obj,
			    sd->moveng.start.x - x,
			    sd->moveng.start.y - y,
			    &lon_off, &lat_off);

	e_nav_coord_set(obj, 
			sd->moveng.start.lon + lon_off,
			sd->moveng.start.lat + lat_off,
			0.1);
	e_nav_zoom_set(obj, 
		       sd->moveng.start.zoom * (1.0 + zoomout),
		       0.5);
     }
#else
   _e_nav_movengine_plain(obj, action, x, y);
#endif
}

static void
_e_nav_update(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Eina_List *l;

   sd = evas_object_smart_data_get(obj);
   _e_nav_wallpaper_update(obj);
   
   for (l = sd->world_items; l; l = l->next)
     {
	E_Nav_World_Item *nwi;
	
	nwi = evas_object_data_get(l->data, "nav_world_item");
	if (nwi) _e_nav_world_item_move_resize(nwi);
     }
   
   _e_nav_overlay_update(obj);
}

static void
_e_nav_overlay_update(Evas_Object *obj)
{
#if 0
   E_Smart_Data *sd;
   char buf[256];
   double z, lon, lat;
   char *xdir, *ydir;
   int lond, lonm, lons, latd, latm, lats;
   
   sd = evas_object_smart_data_get(obj);
   /* the little legend in the overlay theme i KNOW is 64 pixels lat */
   z = 64.0 * sd->zoom;
   /* if its more than 1000m lat - display the length in Km */
   if (z > 1000.0)
     snprintf(buf, sizeof(buf), _("%1.2fKm"), z / 1000.0);
   else
     /* otherwise in meters */
     snprintf(buf, sizeof(buf), _("%1.2fm"), z);
   /* and set the text that is there to what we snprintf'd into the buffer
    * aboe */
   e_ctrl_span_text_value_set(buf);
   
   lon = sd->lon;
   if (lon >= 0.0) xdir = "E";
   else 
     {
	xdir = "W";
	lon = -lon;
     }
   lond = (int)lon;
   lon = (lon - (double)lond) * 60.0;
   lonm = (int)lon;
   lon = (lon - (double)lonm) * 60.0;
   lons = (int)lon;
   snprintf(buf, sizeof(buf), "%i°%i'%i\"%s", lond, lonm, lons, xdir);
   e_ctrl_longitude_set(buf);
   
   lat = sd->lat;
   if (lat >= 0.0) ydir = "N";
   else 
     {
	ydir = "S";
	lat = -lat;
     }
   latd = (int)lat;
   lat = (lat - (double)latd) * 60.0;
   latm = (int)lat;
   lat = (lat - (double)latm) * 60.0;
   lats = (int)lat;
   snprintf(buf, sizeof(buf), "%i°%i'%i\"%s", latd, latm, lats, ydir);
   e_ctrl_latitude_set(buf);
#endif
}

static int
_e_nav_momentum_calc(Evas_Object *obj, double t)
{
   E_Smart_Data *sd;
   double v;
   int done = 0;
   
   sd = evas_object_smart_data_get(obj);
   if (sd->anim.target.pos_time > sd->anim.start.pos_time)
     {
	v = (t - sd->anim.start.pos_time) / 
	  (sd->anim.target.pos_time - sd->anim.start.pos_time);
	if (v >= 1.0)
	  {
	     v = 1.0;
	     done++;
	  }
	v = 1.0 - v;
	v = 1.0 - (v * v * v * v);
	sd->px =
	  ((sd->anim.target.px - sd->anim.start.px) * v) +
	  sd->anim.start.px;
	sd->py =
	  ((sd->anim.target.py - sd->anim.start.py) * v) +
	  sd->anim.start.py;
     }
   else
     done++;
   
   if (sd->anim.target.span_time > sd->anim.start.span_time)
     {
	v = (t - sd->anim.start.span_time) / 
	  (sd->anim.target.span_time - sd->anim.start.span_time);
	if (v >= 1.0)
	  {
	     v = 1.0;
	     done++;
	  }
	v = 1.0 - v;
	v = 1.0 - (v * v * v * v);
	sd->span = 
	  ((sd->anim.target.span - sd->anim.start.span) * v) +
	  sd->anim.start.span;
     }
   else
     done++;
   return done;
}

static void
_e_nav_wallpaper_update(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd->tileset)
     return;

   e_nav_tileset_span_set(sd->tileset, sd->span);
   e_nav_tileset_pos_set(sd->tileset, sd->px, sd->py);
   e_nav_tileset_update(sd->tileset);
}

static int
_e_nav_cb_animator_momentum(void *data)
{
   Evas_Object *obj;
   E_Smart_Data *sd;
   int done = 0;
   
   obj = data;
   sd = evas_object_smart_data_get(obj);
   done = _e_nav_momentum_calc(obj, ecore_time_get());
   _e_nav_update(obj);
   if (done >= 2)
     {
	sd->anim.target.pos_time = 0.0;
	sd->anim.start.pos_time = 0.0;
	sd->anim.target.span_time = 0.0;
	sd->anim.start.span_time = 0.0;
	sd->anim.momentum_animator = NULL;
	return 0;
     }
   return 1;
}

#if 0
static int
_e_nav_cb_timer_moveng_pause(void *data)
{
   Evas_Object *obj;
   E_Smart_Data *sd;
   
   obj = data;
   sd = evas_object_smart_data_get(obj);
   e_nav_zoom_set(obj,
		  sd->moveng.start.zoom,
		  1.0);
   sd->moveng.pause_timer = NULL;
   return 0;
}
#endif

/* nav world internal calls - move to the end later */
static void
_e_nav_world_item_free(E_Nav_World_Item *nwi)
{
   free(nwi);
}

static void
_e_nav_world_item_move_resize(E_Nav_World_Item *nwi)
{
   E_Smart_Data *sd;
   double x, y, w, h;
   
   sd = evas_object_smart_data_get(nwi->obj);
   if (nwi->type == E_NAV_WORLD_ITEM_TYPE_LINKED) return;
   if (nwi->scale)
     {
	x = (nwi->geom.px - (nwi->geom.w / 2.0) - sd->px) * sd->span;
	y = (nwi->geom.py - (nwi->geom.h / 2.0) - sd->py) * sd->span;

	w = (nwi->geom.px + (nwi->geom.w / 2.0) - sd->px) * sd->span;
	w -= x;

	h = (nwi->geom.py + (nwi->geom.h / 2.0) - sd->py) * sd->span;
	h -= y;

	x = (sd->x + (sd->w / 2) + x);
	y = (sd->y + (sd->h / 2) + y);
     }
   else
     {
	x = (nwi->geom.px - sd->px) * sd->span;
	y = (nwi->geom.py - sd->py) * sd->span;

	w = nwi->geom.w;
	h = nwi->geom.h;

	x = (sd->x + (sd->w / 2) + x) - (w / 2.0);
	y = (sd->y + (sd->h / 2) + y) - (h / 2.0);
     }
   evas_object_move(nwi->item, x, y);
   evas_object_resize(nwi->item, w, h);
}

static void
_e_nav_world_item_cb_item_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
   E_Nav_World_Item *nwi;

   nwi = evas_object_data_get(obj, "nav_world_item");
   if (!nwi) return;
   sd = evas_object_smart_data_get(nwi->obj);
   sd->world_items = eina_list_remove(sd->world_items, obj);
   _e_nav_world_item_free(nwi);
}
