#include <e.h>
#include "e_nav.h"

/* navigator object */
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Nav_Item E_Nav_Item;

typedef enum _E_Nav_Movengine_Action
{
   E_NAV_MOVEENGINE_START,
   E_NAV_MOVEENGINE_STOP,
   E_NAV_MOVEENGINE_GO
} E_Nav_Movengine_Action;

struct _E_Nav_Item
{
   Evas_Object *nav;
   Evas_Object *obj;
   struct {
      double x, y, w, h;
   } pos;
};

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   
   Evas_Object     *clip;
   Evas_Object     *overlay;
   Evas_Object     *event;
   
   /* the list of currently active items */
   Evas_List       *nav_items;
   
   /* directory to find theme .edj files from the module - if there is one */
   const char      *dir;
   
   /* these are the CONFIGURED state - what the user or API has ASKED the
    * nav to do. there are other "current" values that are used for
    * animation etc. */
   double           lat, lon;
   double           zoom;
   
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
      Ecore_Timer   *momentum_timer;
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
   
   struct {
      int min_level, max_level, level;
      struct {
	 Evas_Coord x, y, w, h;
	 int ow, oh;
	 Evas_Object *objs;
      } tiles;
   } zoominfo;
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
static void _e_nav_zoominfo_upate(Evas_Object *obj);
static int _e_nav_cb_timer_momemntum(void *data);
static int _e_nav_cb_timer_moveng_pause(void *data);

static Evas_Smart *_e_smart = NULL;

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

     {
	int i;
	
	for (i = 0; i < 30; i++)
	  {
	     E_Nav_Item *ni;
	     
	     ni = calloc(1, sizeof(E_Nav_Item));
	     ni->obj = evas_object_rectangle_add(evas_object_evas_get(obj));
	     evas_object_smart_member_add(ni->obj, obj);
	     evas_object_color_set(ni->obj, rand() & 0xff, rand() & 0xff, rand() & 0xff, 255);
	     evas_object_clip_set(ni->obj, sd->clip);
	     evas_object_show(ni->obj);
	     ni->nav = obj;
	     ni->pos.x = ((double)(rand() % 10000) / 1000.0) - 5.0;
	     ni->pos.y = ((double)(rand() % 10000) / 1000.0) - 5.0;
	     ni->pos.w = ((double)(rand() % 4500) / 1000.0) + 0.5;
	     ni->pos.h = ((double)(rand() % 4500) / 1000.0) + 0.5;
	     sd->nav_items = evas_list_append(sd->nav_items, ni);
	  }
     }

   sd->overlay = _e_nav_theme_obj_new(evas_object_evas_get(obj), sd->dir,
				   "modules/diversity_nav/main");
   evas_object_smart_member_add(sd->overlay, obj);
   evas_object_move(sd->overlay, sd->x, sd->y);
   evas_object_resize(sd->overlay, sd->w, sd->h);
   evas_object_clip_set(sd->overlay, sd->clip);
   evas_object_show(sd->overlay);

   sd->event = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->event, obj);
   evas_object_move(sd->event, sd->x, sd->y);
   evas_object_resize(sd->event, sd->w, sd->h);
   evas_object_color_set(sd->event, 0, 0, 0, 0);
   evas_object_clip_set(sd->event, sd->clip);
//   evas_object_repeat_events_set(sd->event, 1);
   evas_object_show(sd->event);
   
   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_nav_cb_event_mouse_down, obj);
   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_UP,
				  _e_nav_cb_event_mouse_up, obj);
   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_MOVE,
				  _e_nav_cb_event_mouse_move, obj);
   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_WHEEL,
				  _e_nav_cb_event_mouse_wheel, obj);

   _e_nav_zoominfo_upate(obj);
   _e_nav_overlay_update(obj);
}

/* location stack */
E_Nav_Location *
e_nav_location_push(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
}

void
e_nav_location_pop(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
}

void
e_nav_location_del(Evas_Object *obj, E_Nav_Location *loc)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
}

E_Nav_Location *
e_nav_location_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
}

/* spatial & zoom controls */
void
e_nav_coord_set(Evas_Object *obj, double lat, double lon, double when)
{
   E_Smart_Data *sd;
   double t;
   
   SMART_CHECK(obj, ;);
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
   if (!sd->cur.momentum_timer)
     sd->cur.momentum_timer = ecore_timer_add(1.0 / 60.0,
					      _e_nav_cb_timer_momemntum,
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
   double t;
   
   SMART_CHECK(obj, ;);
   if (zoom > 1.0) zoom = 1.0;
   else if (zoom < 0.000001) zoom = 0.000001;
   /* zoom: 1.0 == 1pixel == 1 degree lat/lon */
   /*       2.0 == 1pixel == 2 degrees lat/lon */
   /*       5.0 == 1pixel == 5 degrees lat/lon */
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
   if (!sd->cur.momentum_timer)
     sd->cur.momentum_timer = ecore_timer_add(1.0 / 60.0,
					      _e_nav_cb_timer_momemntum,
					      obj);
}

double
e_nav_zoom_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0.0;);
   return sd->zoom;
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
   
   evas_object_smart_data_set(obj, sd);
   
   sd->lat = 0;
   sd->lon = 0;
   sd->zoom = 2.0 / 1000.0;
   
   sd->conf.lat = sd->lat;
   sd->conf.lon = sd->lon;
   sd->conf.zoom = sd->zoom;
   
   sd->zoominfo.min_level = 1;
   sd->zoominfo.max_level = 3;
}

static void
_e_nav_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_del(sd->clip);
   evas_object_del(sd->event);
   evas_object_del(sd->overlay);
   if (sd->cur.momentum_timer) ecore_timer_del(sd->cur.momentum_timer);
   if (sd->moveng.pause_timer) ecore_timer_del(sd->moveng.pause_timer);
   while (sd->nav_items)
     {
	E_Nav_Item *ni;
	
	ni = sd->nav_items->data;
	evas_object_del(ni->obj);
	free(ni);
	sd->nav_items = evas_list_remove_list(sd->nav_items, sd->nav_items);
     }
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
   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_move(sd->overlay, sd->x, sd->y);
   evas_object_move(sd->event, sd->x, sd->y);
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
   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_resize(sd->overlay, sd->w, sd->h);
   evas_object_resize(sd->event, sd->w, sd->h);
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
_e_nav_movengine(Evas_Object *obj, E_Nav_Movengine_Action action, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   double t;
   int i, count;
   Evas_Coord vx1, vy1, vx2, vy2;
   Evas_Coord dist;
   double plat, plon, lat, lon;
   double zoomout = 0.0;

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
   plat = sd->lat + ((x - (sd->x + (sd->w / 2))) * sd->zoom);
   plon = sd->lon + ((y - (sd->y + (sd->h / 2))) * sd->zoom);
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
	if (dist > 40)
	  {
	     printf("momentum %3.3f\n", zoomout);
	     e_nav_coord_set(obj, lat - ((vx2 - vx1) * (sd->zoom * 5.0)),
			     lon - ((vy2 - vy1) * (sd->zoom * 5.0)),
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
	e_nav_coord_set(obj, 
			sd->moveng.start.lat +
			((sd->moveng.start.x - x) * sd->zoom),
			sd->moveng.start.lon +
			((sd->moveng.start.y - y) * sd->zoom),
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
   
   sd = evas_object_smart_data_get(obj);
   _e_nav_zoominfo_upate(obj);
     {
	Evas_List *l;
	
	for (l = sd->nav_items; l; l = l->next)
	  {
	     E_Nav_Item *ni;
	     double x, y, w, h;
	     
	     ni = l->data;
	     x = ni->pos.x - sd->lat;
	     y = ni->pos.y - sd->lon;
	     w = ni->pos.w;
	     h = ni->pos.h;
	     x = sd->x + (sd->w / 2) + (x / sd->zoom);
	     y = sd->y + (sd->h / 2) + (y / sd->zoom);
	     w = w / sd->zoom;
	     h = h / sd->zoom;
	     evas_object_move(ni->obj, x, y);
	     evas_object_resize(ni->obj, w, h);
	  }
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
   z = ((1000.0 * 40000.0 * 64.0) / 360.0) * sd->zoom;
   if (z > 1000.0)
     snprintf(buf, sizeof(buf), "%1.2fKm", z / 1000.0);
   else
     snprintf(buf, sizeof(buf), "%1.2fm", z);
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
_e_nav_zoominfo_upate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   int i;
   int level, tiles_x, tiles_y, tiles_w, tiles_h, tiles_ox, tiles_oy;
   int wtilesx, wtilesy;
   double tpx, tpy;
   double span, tw;
   
   sd = evas_object_smart_data_get(obj);
   span = 360.0 / sd->zoom; /* world width is 'span' pixels */
   printf("zoom = %3.9f\n", sd->zoom);
   printf("  world width = %3.3f pxiels\n", span);
   level = 1;
   /* 1 tile = 225x225 - each level has world in NxM tiles */
   /* FIXME: support more zoom levels here - if ever */
   if      (span >=  21600) level =  5; /* 96x48 */
   else if (span >=  10800) level =  4; /* 48x24 */
   else if (span >=   5400) level =  3; /* 24x12 */
   else if (span >=   2700) level =  2; /* 12x6 */
   else if (span >=   1350) level =  1; /* 6x3 */
   if (level < sd->zoominfo.min_level) level = sd->zoominfo.min_level;
   else if (level > sd->zoominfo.max_level) level = sd->zoominfo.max_level;
   printf("  level = %i\n", level);
   wtilesx = 6 << (level - 1);
   wtilesy = 3 << (level - 1);
   tw = span / ((double)wtilesx);
   printf("  tile size = %3.1fx%3.1f\n", tw, tw);
   tiles_w = 1 + (((double)sd->w + tw) / tw);
   tiles_h = 1 + (((double)sd->h + tw) / tw);
   printf("  tile count = %i x %i\n", tiles_w, tiles_h);
   tpx = ((180.0 + sd->lat - ((sd->w * sd->zoom) / 2)) * wtilesx) / 360.0;
   tpy = ((90.0 + sd->lon - ((sd->h * sd->zoom) / 2)) * wtilesy) / 180.0;
   printf("    tpx = %3.3f tpy = %3.3f\n", tpx, tpy);
   tiles_ox = (int)tpx;
   tiles_oy = (int)tpy;
   tiles_x = -tw * (tpx - tiles_ox);
   tiles_y = -tw * (tpy - tiles_oy);
   printf("   tiles origin = %i %i\n", tiles_ox, tiles_oy);
   printf("   tiles offset = %i %i\n", tiles_x, tiles_y);
   
   /* MOVE each zoominfo.tiles.objs objs
    *   if (tiles_ox || tiles_oy) changed;
    * MOVE + RESIZE zoominfo.tiles.objs objs
    *   if (tw) changed
    * MOVE obj handles zoominfo.tiles.objs by a memcpy()
    *   if (tiles_ox || tiles_oy) changed.
    * RE-ALLOC zoominfo.tiles.objs
    *   if (tiles_w || tiles_h || level) changed
    *
    */
}

static int
_e_nav_cb_timer_momemntum(void *data)
{
   Evas_Object *obj;
   E_Smart_Data *sd;
   double t, v;
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
	sd->cur.momentum_timer = NULL;
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
