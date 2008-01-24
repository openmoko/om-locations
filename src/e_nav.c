#include "e_nav.h"
#include "e_nav_tileset.h"

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
   struct { // where in the world it lives. x,y are the center. w,h the size
      // in latitudinal/longitudinal degrees
      double x, y, w, h;
   } geom;
   unsigned char scale : 1; // scale item with zoom or not
};

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   
   Evas_Object     *underlay;
   Evas_Object     *clip;
   Evas_Object     *stacking;
   Evas_Object     *overlay;
   Evas_Object     *event;
   
   /* the list of items in the world as we have been told by the backend */
   Evas_List       *world_items;
   
   /* directory to find theme .edj files from the module - if there is one */
   const char      *dir;

   /* these are the CONFIGURED state - what the user or API has ASKED the
    * nav to do. there are other "current" values that are used for
    * animation etc. */
   double           lat, lon;
   double           zoom; /* meters per pixel */
   
   struct {
      double           lat, lon;
      double           zoom;
   } conf;
   
   /* current state - dispay this currently */
   struct {
      struct {
	 double lat, lon;
	 double zoom;
	 double lat_lon_time;
	 double zoom_time;
      } start, target;
      unsigned char  mouse_down : 1;
      Ecore_Timer   *momentum_animator;
   } cur;
   
   struct {
      struct {
	 Evas_Coord    x, y;
	 double        lat, lon;
	 double        zoom;
      } start;
      struct {
	 Evas_Coord    x, y;
	 double        timestamp;
      } history[20];
      Ecore_Timer   *pause_timer;
   } moveng;

   Evas_List *tilesets;
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

static Evas_Object *_e_nav_theme_obj_new(Evas *e, const char *custom_dir, const char *group);
static void _e_nav_movengine(Evas_Object *obj, E_Nav_Movengine_Action action, Evas_Coord x, Evas_Coord y);
static void _e_nav_update(Evas_Object *obj);
static void _e_nav_overlay_update(Evas_Object *obj);
static int _e_nav_momentum_calc(Evas_Object *obj, double t);
static void _e_nav_wallpaper_update(Evas_Object *obj);
static int _e_nav_cb_animator_momentum(void *data);
static int _e_nav_cb_timer_moveng_pause(void *data);

static void _e_nav_cb_signal_drag(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_nav_cb_signal_drag_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_nav_cb_signal_drag_stop(void *data, Evas_Object *obj, const char *emission, const char *source);

static void _e_nav_to_offsets(Evas_Object *obj, double lat, double lon, double *x, double *y);
static void _e_nav_from_offsets(Evas_Object *obj, double x, double y, double *lat, double *lon);

static void _e_nav_world_item_free(E_Nav_World_Item *nwi);
static void _e_nav_world_item_move_resize(E_Nav_World_Item *nwi);

static void _e_nav_world_item_cb_item_del(void *data, Evas *evas, Evas_Object *obj, void *event);

static Evas_Smart *_e_smart = NULL;

#define E_NAV_ZOOM_MAX (M_EARTH_RADIUS / 50)
#define E_NAV_ZOOM_MIN 0.5

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_nav")) return ret

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
   
   sd->overlay = _e_nav_theme_obj_new(evas_object_evas_get(obj), sd->dir,
				      "modules/diversity_nav/main");
   evas_object_smart_member_add(sd->overlay, obj);
   evas_object_move(sd->overlay, sd->x, sd->y);
   evas_object_resize(sd->overlay, sd->w, sd->h);
   evas_object_clip_set(sd->overlay, sd->clip);

   evas_object_show(sd->overlay);

   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_cb_event_mouse_down, obj);
   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_UP,
				  _e_nav_cb_event_mouse_up, obj);
   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_MOVE,
				  _e_nav_cb_event_mouse_move, obj);
   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_WHEEL,
				  _e_nav_cb_event_mouse_wheel, obj);

   edje_object_signal_callback_add(sd->overlay, "drag", "*", _e_nav_cb_signal_drag, sd);
   edje_object_signal_callback_add(sd->overlay, "drag,start", "*", _e_nav_cb_signal_drag_start, sd);
   edje_object_signal_callback_add(sd->overlay, "drag,stop", "*", _e_nav_cb_signal_drag_stop, sd);
   edje_object_signal_callback_add(sd->overlay, "drag,step", "*", _e_nav_cb_signal_drag_stop, sd);
   edje_object_signal_callback_add(sd->overlay, "drag,set", "*", _e_nav_cb_signal_drag_stop, sd);
   
   _e_nav_wallpaper_update(obj);
   _e_nav_overlay_update(obj);
}

/* spatial & zoom controls */
void
e_nav_coord_set(Evas_Object *obj, double lat, double lon, double when)
{
   E_Smart_Data *sd;
   double t;
   
   SMART_CHECK(obj, ;);
   if (lat < -180.0) lat = -180.0;
   else if (lat > 180.0) lat = 180.0;
   if (lon < -90.0) lon = -90.0;
   else if (lon > 90.0) lon = 90.0;
   if (when == 0.0)
     {
	sd->cur.target.lat_lon_time = 0.0;
	sd->cur.start.lat_lon_time = 0.0;
	sd->lat = lat;
	sd->lon = lon;
	sd->conf.lat = lat;
	sd->conf.lon = lon;
	_e_nav_update(obj);
	return;
     }
   t = ecore_time_get();
   _e_nav_momentum_calc(obj, t);
   sd->cur.start.lat_lon_time = t;
   sd->cur.start.lat = sd->lat;
   sd->cur.start.lon = sd->lon;
   sd->cur.target.lat = lat;
   sd->cur.target.lon = lon;
   sd->cur.target.lat_lon_time = sd->cur.start.lat_lon_time + when;
   sd->conf.lat = lat;
   sd->conf.lon = lon;
   if (!sd->cur.momentum_animator)
     sd->cur.momentum_animator = ecore_animator_add(_e_nav_cb_animator_momentum,
						 obj);
}

double
e_nav_coord_lat_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0.0;);
   return sd->lat;
}

double
e_nav_coord_lon_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0.0;);
   return sd->lon;
}

void
e_nav_zoom_set(Evas_Object *obj, double zoom, double when)
{
   E_Smart_Data *sd;
   double t, y;
   
   SMART_CHECK(obj, ;);
   if (zoom > E_NAV_ZOOM_MAX) zoom = E_NAV_ZOOM_MAX;
   else if (zoom < E_NAV_ZOOM_MIN) zoom = E_NAV_ZOOM_MIN;

   y = (zoom - E_NAV_ZOOM_MIN) / (E_NAV_ZOOM_MAX - E_NAV_ZOOM_MIN);
   y = sqrt(sqrt(y));
   edje_object_part_drag_value_set(sd->overlay, "e.dragable.zoom", 0.0, y);
   if (when == 0.0)
     {
	sd->cur.target.zoom_time = 0.0;
	sd->cur.start.zoom_time = 0.0;
	sd->zoom = zoom;
	sd->conf.zoom = zoom;
	_e_nav_update(obj);
	return;
     }
   t = ecore_time_get();
   _e_nav_momentum_calc(obj, t);
   sd->cur.start.zoom_time = t;
   sd->cur.start.zoom = sd->zoom;
   sd->cur.target.zoom = zoom;
   sd->cur.target.zoom_time = sd->cur.start.zoom_time + when;
   sd->conf.zoom = zoom;
   if (!sd->cur.momentum_animator)
     sd->cur.momentum_animator = ecore_animator_add(_e_nav_cb_animator_momentum,
						 obj);
}

double
e_nav_zoom_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0.0;);
   return sd->zoom;
}

/* world items */
void
e_nav_world_item_add(Evas_Object *obj, Evas_Object *item)
{
   E_Nav_World_Item *nwi;
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   nwi = calloc(1, sizeof(E_Nav_World_Item));
   if (!nwi) return;
   nwi->obj = obj;
   nwi->item = item;
   evas_object_data_set(item, "nav_world_item", nwi);
   evas_object_event_callback_del(item, EVAS_CALLBACK_DEL,
				  _e_nav_world_item_cb_item_del);
   sd->world_items = evas_list_append(sd->world_items, item);
   evas_object_smart_member_add(nwi->item, nwi->obj);
   evas_object_clip_set(nwi->item, sd->clip);
   if (nwi->type == E_NAV_WORLD_ITEM_TYPE_WALLPAPER)
     evas_object_stack_above(nwi->item, sd->clip);
   else if (nwi->type == E_NAV_WORLD_ITEM_TYPE_ITEM)
     evas_object_stack_below(nwi->item, sd->stacking);
   else if (nwi->type == E_NAV_WORLD_ITEM_TYPE_OVERLAY)
     evas_object_stack_above(nwi->item, sd->stacking);
   else if (nwi->type == E_NAV_WORLD_ITEM_TYPE_LINKED)
     evas_object_stack_above(nwi->item, sd->stacking);
}

void
e_nav_world_item_type_set(Evas_Object *item, E_Nav_World_Item_Type type)
{
   E_Nav_World_Item *nwi;
   E_Smart_Data *sd;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return;
   nwi->type = type;
   sd = evas_object_smart_data_get(nwi->obj);
   if (nwi->type == E_NAV_WORLD_ITEM_TYPE_WALLPAPER)
     evas_object_stack_above(nwi->item, sd->clip);
   else if (nwi->type == E_NAV_WORLD_ITEM_TYPE_ITEM)
     evas_object_stack_below(nwi->item, sd->stacking);
   else if (nwi->type == E_NAV_WORLD_ITEM_TYPE_OVERLAY)
     evas_object_stack_above(nwi->item, sd->stacking);
   else if (nwi->type == E_NAV_WORLD_ITEM_TYPE_LINKED)
     evas_object_stack_above(nwi->item, sd->stacking);
}

E_Nav_World_Item_Type
e_nav_world_item_type_get(Evas_Object *item)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return 0;
   return nwi->type;
}

void
e_nav_world_item_geometry_set(Evas_Object *item, double x, double y, double w, double h)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return;
   nwi->geom.x = x;
   nwi->geom.y = y;
   nwi->geom.w = w;
   nwi->geom.h = h;
}

void
e_nav_world_item_geometry_get(Evas_Object *item, double *x, double *y, double *w, double *h)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return;
   if (x) *x = nwi->geom.x;
   if (y) *y = nwi->geom.y;
   if (w) *w = nwi->geom.w;
   if (h) *h = nwi->geom.h;
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
e_nav_world_item_update(Evas_Object *item)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return;
   _e_nav_world_item_move_resize(nwi);
}

Evas_Object *
e_nav_world_item_nav_get(Evas_Object *item)
{
   E_Nav_World_Item *nwi;
   
   nwi = evas_object_data_get(item, "nav_world_item");
   if (!nwi) return NULL;
   return nwi->obj;
}

int
e_nav_edje_object_set(Evas_Object *o, const char *category, const char *group)
{
   char buf[PATH_MAX];
   int ok;
   
   snprintf(buf, sizeof(buf), "%s/%s.edj", THEME_PATH, category);
   ok = edje_object_file_set(o, buf, group);

   return ok;
}

/* world tilesets */
void
e_nav_world_tileset_add(Evas_Object *obj, Evas_Object *nt)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);
   sd->tilesets = evas_list_prepend(sd->tilesets, nt);
   evas_object_smart_member_add(nt, sd->obj);
   evas_object_clip_set(nt, sd->clip);
   evas_object_stack_below(nt, sd->clip);
}

/* internal calls */
static void
_e_nav_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     "e_nav",
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
   
   sd->lat = 0;
   sd->lon = 0;
   sd->zoom = E_NAV_ZOOM_MAX;
   
   sd->conf.lat = sd->lat;
   sd->conf.lon = sd->lon;
   sd->conf.zoom = sd->zoom;
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
   evas_object_del(sd->overlay);
   if (sd->cur.momentum_animator) ecore_animator_del(sd->cur.momentum_animator);
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
	     sd->world_items = evas_list_remove_list(sd->world_items,
						     sd->world_items);
	  }
     }
   while (sd->tilesets)
     {
	evas_object_del(sd->tilesets->data);
	sd->tilesets = evas_list_remove_list(sd->tilesets,
					     sd->tilesets);
     }

   free(sd);
}
                    
static void
_e_nav_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   Evas_List *l;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;
   evas_object_move(sd->underlay, sd->x, sd->y);
   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_move(sd->stacking, sd->x, sd->y);
   evas_object_move(sd->overlay, sd->x, sd->y);
   evas_object_move(sd->event, sd->x, sd->y);

   for (l = sd->tilesets; l; l = l->next)
     evas_object_move(l->data, sd->x, sd->y);
   
   _e_nav_update(obj);
}

static void
_e_nav_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   Evas_List *l;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->underlay, sd->w, sd->h);
   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_resize(sd->stacking, sd->w, sd->h);
   evas_object_resize(sd->overlay, sd->w, sd->h);
   evas_object_resize(sd->event, sd->w, sd->h);

   for (l = sd->tilesets; l; l = l->next)
     evas_object_resize(l->data, sd->w, sd->h);

   _e_nav_update(obj);
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
	sd->cur.mouse_down = 1;
	_e_nav_movengine(data, E_NAV_MOVEENGINE_START, ev->canvas.x, ev->canvas.y);
     }
}

static void
_e_nav_cb_event_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Up *ev = event;
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(data);
   if (ev->button == 1)
     {
	_e_nav_movengine(data, E_NAV_MOVEENGINE_STOP, ev->canvas.x, ev->canvas.y);
	sd->cur.mouse_down = 0;
     }
}

static void
_e_nav_cb_event_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Move *ev = event;
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(data);
   if (sd->cur.mouse_down)
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
	if (ev->z > 0) e_nav_zoom_set(data, sd->conf.zoom * 2.0, 1.0);
       else e_nav_zoom_set(data, sd->conf.zoom / 2.0, 1.0);
     }
}

static Evas_Object *
_e_nav_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;
   
   o = edje_object_add(e);
   if (!e_nav_edje_object_set(o, "diversity_nav", group))
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
_e_nav_movengine_plain(Evas_Object *obj, E_Nav_Movengine_Action action, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (action == E_NAV_MOVEENGINE_START)
     {
	sd->moveng.start.x = x;
	sd->moveng.start.y = y;
	sd->moveng.start.lat = sd->lat;
	sd->moveng.start.lon = sd->lon;
	sd->moveng.start.zoom = sd->conf.zoom;

	e_nav_coord_set(obj, sd->lat, sd->lon, 0.0);
	e_nav_zoom_set(obj, sd->conf.zoom, 0.5);

	return;
     }
   
   if (action == E_NAV_MOVEENGINE_STOP)
     {
	e_nav_coord_set(obj, sd->lat, sd->lon, 0.0);
     }
   else
     {
	double lat_off, lon_off;

	_e_nav_from_offsets(obj,
			    sd->moveng.start.x - x,
			    sd->moveng.start.y - y,
			    &lat_off, &lon_off);
	e_nav_coord_set(obj, 
			sd->moveng.start.lat + lat_off,
			sd->moveng.start.lon + lon_off,
			0.0);
     }
}

static void
_e_nav_movengine(Evas_Object *obj, E_Nav_Movengine_Action action, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   double t;
   int i, count;
   Evas_Coord vx1, vy1, vx2, vy2;
   Evas_Coord dist;
   double lat, lon;
   double zoomout = 0.0;

   /* TODO provide parameters instead of calling another engine */
   _e_nav_movengine_plain(obj, action, x, y);
   return;

   sd = evas_object_smart_data_get(obj);
   if (action == E_NAV_MOVEENGINE_START)
     {
	sd->moveng.start.x = x;
	sd->moveng.start.y = y;
	sd->moveng.start.lat = sd->lat;
	sd->moveng.start.lon = sd->lon;
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
   lat = sd->lat;
   lon = sd->lon;
   if (action == E_NAV_MOVEENGINE_START)
     {
	e_nav_coord_set(obj,
			lat, lon,
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
	     e_nav_coord_set(obj, lat, lon, 2.0);
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
	double lat_off, lon_off;

	if (dist > 40)
	  {
	     _e_nav_from_offsets(obj,
				 (vx2 - vx1) * 5.0,
				 (vy2 - vy1) * 5.0,
				 &lat_off, &lon_off);
	     e_nav_coord_set(obj, lat - lat_off,
			     lon - lon_off,
			     2.0 + (zoomout / 16.0));
	  }
	else
	  e_nav_coord_set(obj, lat, lon,
			  2.0 + (zoomout / 16.0));
	e_nav_zoom_set(obj, sd->moveng.start.zoom, 1.0 + (zoomout / 4.0));
	if (sd->moveng.pause_timer) ecore_timer_del(sd->moveng.pause_timer);
	sd->moveng.pause_timer = NULL;
     }
   else
     {
	double lat_off, lon_off;

	_e_nav_from_offsets(obj,
			    sd->moveng.start.x - x,
			    sd->moveng.start.y - y,
			    &lat_off, &lon_off);

	e_nav_coord_set(obj, 
			sd->moveng.start.lat + lat_off,
			sd->moveng.start.lon + lon_off,
			0.1);
	e_nav_zoom_set(obj, 
		       sd->moveng.start.zoom * (1.0 + zoomout),
		       0.5);
     }
}

static void
_e_nav_update(Evas_Object *obj)
{

   E_Smart_Data *sd;
   Evas_List *l;
   
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
   E_Smart_Data *sd;
   char buf[256];
   double z, lat, lon;
   char *xdir, *ydir;
   int latd, latm, lats, lond, lonm, lons;
   
   sd = evas_object_smart_data_get(obj);
   /* the little legend in the overlay theme i KNOW is 64 pixels lon */
   z = 64.0 * sd->zoom;
   /* if its more than 1000m lon - display the length in Km */
   if (z > 1000.0)
     snprintf(buf, sizeof(buf), "%1.2fKm", z / 1000.0);
   else
     /* otherwise in meters */
     snprintf(buf, sizeof(buf), "%1.2fm", z);
   /* and set the text that is there to what we snprintf'd into the buffer
    * aboe */
   edje_object_part_text_set(sd->overlay, "e.text.zoom", buf);
   
   lat = sd->lat;
   if (lat >= 0.0) xdir = "E";
   else 
     {
	xdir = "W";
	lat = -lat;
     }
   latd = (int)lat;
   lat = (lat - (double)latd) * 60.0;
   latm = (int)lat;
   lat = (lat - (double)latm) * 60.0;
   lats = (int)lat;
   snprintf(buf, sizeof(buf), "%i°%i'%i\"%s", latd, latm, lats, xdir);
   edje_object_part_text_set(sd->overlay, "e.text.latitude", buf);
   
   lon = sd->lon;
   if (lon >= 0.0) ydir = "S";
   else 
     {
	ydir = "N";
	lon = -lon;
     }
   lond = (int)lon;
   lon = (lon - (double)lond) * 60.0;
   lonm = (int)lon;
   lon = (lon - (double)lonm) * 60.0;
   lons = (int)lon;
   snprintf(buf, sizeof(buf), "%i°%i'%i\"%s", lond, lonm, lons, ydir);
   edje_object_part_text_set(sd->overlay, "e.text.longitude", buf);
}

static int
_e_nav_momentum_calc(Evas_Object *obj, double t)
{
   E_Smart_Data *sd;
   double v;
   int done = 0;
   
   sd = evas_object_smart_data_get(obj);
   if (sd->cur.target.lat_lon_time > sd->cur.start.lat_lon_time)
     {
	v = (t - sd->cur.start.lat_lon_time) / 
	  (sd->cur.target.lat_lon_time - sd->cur.start.lat_lon_time);
	if (v >= 1.0)
	  {
	     v = 1.0;
	     done++;
	  }
	v = 1.0 - v;
	v = 1.0 - (v * v * v * v);
	sd->lat = 
	  ((sd->cur.target.lat - sd->cur.start.lat) * v) +
	  sd->cur.start.lat;
	sd->lon = 
	  ((sd->cur.target.lon - sd->cur.start.lon) * v) +
	  sd->cur.start.lon;
     }
   else
     done++;
   
   if (sd->cur.target.zoom_time > sd->cur.start.zoom_time)
     {
	v = (t - sd->cur.start.zoom_time) / 
	  (sd->cur.target.zoom_time - sd->cur.start.zoom_time);
	if (v >= 1.0)
	  {
	     v = 1.0;
	     done++;
	  }
	v = 1.0 - v;
	v = 1.0 - (v * v * v * v);
	sd->zoom = 
	  ((sd->cur.target.zoom - sd->cur.start.zoom) * v) +
	  sd->cur.start.zoom;
     }
   else
     done++;
   return done;
}

static void
_e_nav_wallpaper_update(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_List *l;
   
   sd = evas_object_smart_data_get(obj);
   for (l = sd->tilesets; l; l = l->next)
     {
	Evas_Object *nt = l->data;

	e_nav_tileset_center_set(nt, sd->lat, -sd->lon);
	e_nav_tileset_scale_set(nt, sd->zoom);
	e_nav_tileset_update(nt);
     }
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
	sd->cur.target.lat_lon_time = 0.0;
	sd->cur.start.lat_lon_time = 0.0;
	sd->cur.target.zoom_time = 0.0;
	sd->cur.start.zoom_time = 0.0;
	sd->cur.momentum_animator = NULL;
	return 0;
     }
   return 1;
   
}

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

static void
_e_nav_cb_signal_drag(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   
   sd = data;
     {
	double x = 0, y = 0, z;
	
	edje_object_part_drag_value_get(sd->overlay, "e.dragable.zoom", &x, &y);
	y = (y * y) * (y * y);

	z = E_NAV_ZOOM_MIN + ((E_NAV_ZOOM_MAX - E_NAV_ZOOM_MIN) * y);
	e_nav_zoom_set(sd->obj, z, 0.2);
     }
}

static void
_e_nav_cb_signal_drag_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   
   sd = data;
     {
	double x = 0, y = 0;
	
	edje_object_part_drag_value_get(sd->overlay, "e.dragable.zoom", &x, &y);
     }
}

static void
_e_nav_cb_signal_drag_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   
   sd = data;
     {
	double x = 0, y = 0;
	
	edje_object_part_drag_value_get(sd->overlay, "e.dragable.zoom", &x, &y);
     }
}

static void _e_nav_to_offsets(Evas_Object *obj, double lat, double lon, double *x, double *y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd || !sd->tilesets)
     {
	*x = 0.0;
	*y = 0.0;

	return;
     }

   e_nav_tileset_to_offsets(sd->tilesets->data, lat, -lon, x, y);
   *y = -*y;
}

static void _e_nav_from_offsets(Evas_Object *obj, double x, double y, double *lat, double *lon)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd || !sd->tilesets)
     {
	*lat = 0.0;
	*lon = 0.0;

	return;
     }

   e_nav_tileset_from_offsets(sd->tilesets->data, x, y, lat, lon);
}

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
	x = nwi->geom.x - (nwi->geom.w / 2.0);
	y = nwi->geom.y - (nwi->geom.h / 2.0);
	_e_nav_to_offsets(nwi->obj, x, y, &x, &y);

	w = nwi->geom.x + (nwi->geom.w / 2.0);
	h = nwi->geom.y + (nwi->geom.h / 2.0);
	_e_nav_to_offsets(nwi->obj, w, h, &w, &h);

	w = w - x;
	h = h - y;

	x = (sd->x + (sd->w / 2) + x);
	y = (sd->y + (sd->h / 2) + y);
     }
   else
     {
	_e_nav_to_offsets(nwi->obj, nwi->geom.x, nwi->geom.y, &x, &y);

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
   sd->world_items = evas_list_remove(sd->world_items, nwi);
   _e_nav_world_item_free(nwi);
}
