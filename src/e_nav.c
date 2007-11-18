#include <e.h>
#include "e_nav.h"

/* navigator object */
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Nav_Item E_Nav_Item;
typedef struct _E_Nav_Tileset E_Nav_Tileset;
typedef struct _E_Nav_World E_Nav_World;
typedef struct _E_Nav_World_Block E_Nav_World_Block;
  
typedef enum _E_Nav_Movengine_Action
{
   E_NAV_MOVEENGINE_START,
   E_NAV_MOVEENGINE_STOP,
   E_NAV_MOVEENGINE_GO
} E_Nav_Movengine_Action;

struct _E_Nav_Item
{
   Evas_Object *nav;
   E_Nav_World_Item *world_item;
   Evas_Object *obj;
   struct {
      double x, y, w, h;
   } pos;
};

struct _E_Nav_Tileset
{
   Evas_Object *obj;
   const char *map;
   const char *format;
   int min_level, max_level, level;
   struct {
      double tilesize;
      Evas_Coord offset_x, offset_y;
      int ox, oy, ow, oh;
      Evas_Object **objs;
   } tiles;
};

struct _E_Nav_World_Item
{
   Evas_Object *obj; // the nav obj this nav item belongs to
   E_Nav_World_Block *block;
   E_Nav_Item *item; // an instance of this world item in the real canvas widget
   E_Nav_World_Item_Type type;
   struct { // callback to add new instances of this item
      Evas_Object *(*func) (void *data, Evas *evas, const char *theme_dir);
      void *data;
   } add;
   struct { // where in the world it lives. x,y are the center. w,h the size
            // in latitudinal/longitudinal degrees
      double x, y, w, h;
   } geom;
   int level; // the summary level for this item if the summary flag is set
   unsigned char scale : 1; // scale item with zoom or not
   unsigned char summary : 1; // this is a summary item;
};

struct _E_Nav_World
{
   void *parent; // always NULL
   int   level; // the zoom level - world == 0
   Evas_List *summary_items; // a list of world items to summarise all of the
                             // world
   E_Nav_World_Block *blocks[360][180]; // the world is 360x180 degree blocks
};

struct _E_Nav_World_Block
{
   void *parent; // the parent item
   int   level; // the zoom level - sub blocks == 1, 2, 3
   Evas_List *summary_items; // a list of world items to summarise all 25x25
                             // sub-blocks with a list or world items to show
			     // when zoomd out so the sub-blocks are too
			     // small
   void *blocks[25][25]; // each world block is 25x25 sub-blocks. if the parent
                         // is a world struct, then u have another 2 levels of
			 // blocks then a list of world items (so it goes
                         // world -> block -> block-> block -> list). as it's
                         // a void * for blocks - they can nest infinitely
                         // if we want.
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
   
   Evas_List *tilesets;
   
   E_Nav_World *world;
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
static E_Nav_Tileset *_e_nav_tileset_add(Evas_Object *obj);
static void _e_nav_tileset_del(E_Nav_Tileset *nt);
static void _e_nav_wallpaper_update(Evas_Object *obj);
static void _e_nav_wallpaper_update_tileset(E_Nav_Tileset *nt);
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
   E_Nav_Tileset *nt;
   
   SMART_CHECK(obj, ;);
   
   sd->dir = custom_dir;

     {
	E_Nav_Item *ni;
	int i;
	
	for (i = 0; i < 30; i++)
	  {
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

	/* home !!! */
	     ni = calloc(1, sizeof(E_Nav_Item));
	     ni->obj = evas_object_rectangle_add(evas_object_evas_get(obj));
	     evas_object_smart_member_add(ni->obj, obj);
	     evas_object_color_set(ni->obj, rand() & 0xff, rand() & 0xff, rand() & 0xff, 255);
	     evas_object_clip_set(ni->obj, sd->clip);
	     evas_object_show(ni->obj);
	     ni->nav = obj;
	     ni->pos.x = 151.207114 - 0.001;
	     ni->pos.y = 33.867139 - 0.001;
	     ni->pos.w = 0.002;
	     ni->pos.h = 0.002;
	     sd->nav_items = evas_list_append(sd->nav_items, ni);
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

   nt = _e_nav_tileset_add(obj);
//   nt = _e_nav_tileset_add(obj);
//   nt->map = "map";
//   nt->format = "png";
//   nt->max_level = 5;
   _e_nav_wallpaper_update(obj);
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

/* world items */
/* nav world internal calls - move to the end later */
static void
_e_nav_item_free(E_Nav_Item *ni)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(ni->world_item->obj);
   sd->nav_items = evas_list_remove(sd->nav_items, ni);
   evas_object_del(ni->obj);
   sd->nav_items = evas_list_remove(sd->nav_items, ni);
   free(ni);
}

static void
_e_nav_world_item_nav_item_add(E_Nav_World_Item *nwi)
{
   E_Nav_Item *ni;
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(nwi->obj);
   ni = calloc(1, sizeof(E_Nav_Item));
   if (!ni) return;
   ni->world_item = nwi;
   ni->obj = nwi->add.func(nwi->add.data, evas_object_evas_get(nwi->obj), sd->dir);
   evas_object_smart_member_add(ni->obj, nwi->obj);
   evas_object_clip_set(ni->obj, sd->clip);
   evas_object_show(ni->obj);
   ni->nav = nwi->obj;
   ni->pos.x = nwi->geom.x - (nwi->geom.w / 2.0);
   ni->pos.y = nwi->geom.y - (nwi->geom.h / 2.0);
   ni->pos.w = nwi->geom.w;
   ni->pos.h = nwi->geom.h;
   sd->nav_items = evas_list_append(sd->nav_items, ni);
}

static void
_e_nav_world_item_free(E_Nav_World_Item *nwi)
{
   if (nwi->item) _e_nav_item_free(nwi->item);
   nwi->item = NULL;
   free(nwi);
}

static void
_e_nav_world_block_3_del(E_Nav_World_Block *blk)
{
   int i, j;
   Evas_List *items;
   
   items = blk->summary_items;
   while (items)
     {
	_e_nav_world_item_free(items->data);
	items = evas_list_remove_list(items, items);
     }
   blk->summary_items = NULL;
   for (j = 0; j < 25; j++)
     {
	for (i = 0; i < 25; i++)
	  {
	     items = blk->blocks[i][j];
	     while (items)
	       {
		  _e_nav_world_item_free(items->data);
		  items = evas_list_remove_list(items, items);
	       }
	     blk->blocks[i][j] = NULL;
	  }
     }
}
  
static void
_e_nav_world_block_2_del(E_Nav_World_Block *blk)
{
   int i, j;
   Evas_List *items;
   
   items = blk->summary_items;
   while (items)
     {
	_e_nav_world_item_free(items->data);
	items = evas_list_remove_list(items, items);
     }
   blk->summary_items = NULL;
   for (j = 0; j < 25; j++)
     {
	for (i = 0; i < 25; i++)
	  {
	     _e_nav_world_block_3_del(blk->blocks[i][j]);
	     blk->blocks[i][j] = NULL;
	  }
     }
}
  
static void
_e_nav_world_block_1_del(E_Nav_World_Block *blk)
{
   int i, j;
   Evas_List *items;
   
   items = blk->summary_items;
   while (items)
     {
	_e_nav_world_item_free(items->data);
	items = evas_list_remove_list(items, items);
     }
   blk->summary_items = NULL;
   for (j = 0; j < 25; j++)
     {
	for (i = 0; i < 25; i++)
	  {
	     if (blk->blocks[i][j])
	       {
		  _e_nav_world_block_2_del(blk->blocks[i][j]);
		  blk->blocks[i][j] = NULL;
	       }
	  }
     }
}
  
static void
_e_nav_world_add(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (sd->world) return;
   sd->world = calloc(1, sizeof(E_Nav_World));
   return;
}

static void
_e_nav_world_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   int i, j;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd->world) return;
   for (j = 0; j < 180; j++)
     {
	for (i = 0; i < 360; i++)
	  {
	     if (sd->world->blocks[i][j])
	       _e_nav_world_block_1_del(sd->world->blocks[i][j]);
	  }
     }
   free(sd->world);
   sd->world = NULL;
}

E_Nav_World_Item *
e_nav_world_item_add(Evas_Object *obj)
{
   E_Nav_World_Item *nwi;
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
   nwi = calloc(1, sizeof(E_Nav_World_Item));
   if (!nwi) return NULL;
   nwi->obj = obj;
   /* FIXME: add to sd->world */
   return nwi;
}

void
e_nav_world_item_del(E_Nav_World_Item *nwi)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(nwi->obj);
   if (nwi->block)
     {
	/* FIXME: set block ref to NULL */
     }
   _e_nav_world_item_free(nwi);
}

void
e_nav_world_item_type_set(E_Nav_World_Item *nwi, E_Nav_World_Item_Type type)
{
   nwi->type = type;
}

E_Nav_World_Item_Type
e_nav_world_item_type_get(E_Nav_World_Item *nwi)
{
   return nwi->type;
}

void
e_nav_world_item_add_func_set(E_Nav_World_Item *nwi, Evas_Object *(*func) (void *data, Evas *evas, const char *theme_dir), void *data)
{
   nwi->add.func = func;
   nwi->add.data = data;
}

void
e_nav_world_item_geometry_set(E_Nav_World_Item *nwi, double x, double y, double w, double h)
{
   nwi->geom.x = x;
   nwi->geom.y = y;
   nwi->geom.w = w;
   nwi->geom.h = h;
}

void
e_nav_world_item_geometry_get(E_Nav_World_Item *nwi, double *x, double *y, double *w, double *h)
{
   if (x) *x = nwi->geom.x;
   if (y) *y = nwi->geom.y;
   if (w) *w = nwi->geom.w;
   if (h) *h = nwi->geom.h;
}

void
e_nav_world_item_scale_set(E_Nav_World_Item *nwi, int scale)
{
   nwi->scale = scale;
}

int
e_nav_world_item_scale_get(E_Nav_World_Item *nwi)
{
   return nwi->scale;
}

void
e_nav_world_item_level_set(E_Nav_World_Item *nwi, int level)
{
   nwi->level = level;
}

int
e_nav_world_item_level_get(E_Nav_World_Item *nwi)
{
   return nwi->level;
}

void
e_nav_world_item_summary_set(E_Nav_World_Item *nwi, int summary)
{
   nwi->summary = summary;
}

int
e_nav_world_item_summary_get(E_Nav_World_Item *nwi)
{
   return nwi->summary;
}

void
e_nav_world_item_update(E_Nav_World_Item *nwi)
{
   if (nwi->block)
     {
	/* FIXME: remove from block */
     }
   /* FIXME: allocate to new block */
   /* FIXME; if summary item add to appropriate summary list */
   /* FIXME: determine if visible nav item shoucl be created (or old one 
    * destroyed */
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
   while (sd->tilesets) _e_nav_tileset_del(sd->tilesets->data);
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
   _e_nav_wallpaper_update(obj);
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

static E_Nav_Tileset *
_e_nav_tileset_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   E_Nav_Tileset *nt;
   
   sd = evas_object_smart_data_get(obj);
   nt = calloc(1, sizeof(E_Nav_Tileset));
   if (!nt) return;
   
   sd->tilesets = evas_list_append(sd->tilesets, nt);
   nt->obj = obj;
   nt->min_level = 1;
   nt->max_level = 3;
   nt->map = "sat";
   nt->format = "jpg";
   return nt;
}

static void
_e_nav_tileset_del(E_Nav_Tileset *nt)
{
   E_Smart_Data *sd;
   int i, j;
   
   sd = evas_object_smart_data_get(nt->obj);
   sd->tilesets = evas_list_remove(sd->tilesets, nt);
   if (nt->tiles.objs)
     {
	for (j = 0; j < nt->tiles.oh; j++)
	  {
	     for (i = 0; i < nt->tiles.ow; i++)
	       evas_object_del(nt->tiles.objs[(j * nt->tiles.ow) + i]);
	  }
	free(nt->tiles.objs);
     }
   free(nt);
}

static void
_e_nav_wallpaper_update(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_List *l;
   
   sd = evas_object_smart_data_get(obj);
   for (l = sd->tilesets; l; l = l->next)
     {
	E_Nav_Tileset *nt;
	
	nt = l->data;
	_e_nav_wallpaper_update_tileset(nt);
     }
}

static void
_e_nav_wallpaper_update_tileset(E_Nav_Tileset *nt)
{
   E_Smart_Data *sd;
   int i, j;
   int level, tiles_x, tiles_y, tiles_w, tiles_h, tiles_ox, tiles_oy;
   int wtilesx, wtilesy;
   double tpx, tpy;
   double span, tw;
   Evas_Coord x, y, xx, yy;
   const char *mapdir, *mapset, *mapformat;
   char mapbuf[PATH_MAX];
   enum {
      MODE_NONE,
	MODE_RESET,
	MODE_ORIGIN,
	MODE_RESIZE,
	MODE_MOVE
   };
   int mode = MODE_NONE;
   
   sd = evas_object_smart_data_get(nt->obj);

   snprintf(mapbuf, sizeof(mapbuf), "%s/maps", sd->dir);
   
   mapdir = mapbuf;
   mapset = nt->map;
   mapformat = nt->format;
   
   span = 360.0 / sd->zoom; /* world width is 'span' pixels */
   level = 1;
   if      (span >  43200) level =  7; /* 384x192 */
   else if (span >  21600) level =  6; /* 192x96 */
   else if (span >  10800) level =  5; /* 96x48 */
   else if (span >   5400) level =  4; /* 48x24 */
   else if (span >   2700) level =  3; /* 24x12 */
   else if (span >   1350) level =  2; /* 12x6 */
   if (level < nt->min_level) level = nt->min_level;
   else if (level > nt->max_level) level = nt->max_level;
   
   wtilesx = 6 << (level - 1);
   wtilesy = 3 << (level - 1);
   tw = span / ((double)wtilesx);
   tiles_w = 1 + (((double)sd->w + tw) / tw);
   tiles_h = 1 + (((double)sd->h + tw) / tw);
   tpx = ((180.0 + sd->lat - ((sd->w * sd->zoom) / 2)) * wtilesx) / 360.0;
   tpy = ((90.0 + sd->lon - ((sd->h * sd->zoom) / 2)) * wtilesy) / 180.0;
   tiles_ox = (int)tpx;
   tiles_oy = (int)tpy;
   tiles_x = -tw * (tpx - tiles_ox);
   tiles_y = -tw * (tpy - tiles_oy);

   if ((nt->tiles.ow != tiles_w) || (nt->tiles.oh != tiles_h) ||
       (nt->level != level))
     mode = MODE_RESET;
   else if ((nt->tiles.ox != tiles_ox) || (nt->tiles.oy != tiles_oy))
     mode = MODE_ORIGIN;
   else if (tw != nt->tiles.tilesize)
     mode = MODE_RESIZE;
   else if ((tiles_x != nt->tiles.offset_x) || (tiles_y != nt->tiles.offset_y))
     mode = MODE_MOVE;
     
   if (mode == MODE_NONE) return;
   
   if (mode == MODE_RESET)
     {
	// reallac entirely move and resize
	if (nt->tiles.objs)
	  {
	     for (j = 0; j < nt->tiles.oh; j++)
	       {
		  for (i = 0; i < nt->tiles.ow; i++)
		    evas_object_del(nt->tiles.objs[(j * nt->tiles.ow) + i]);
	       }
	     free(nt->tiles.objs);
	  }
     }
   
   nt->tiles.ow = tiles_w;
   nt->tiles.oh = tiles_h;
   nt->level = level;
   nt->tiles.ox = tiles_ox;
   nt->tiles.oy = tiles_oy;
   nt->tiles.tilesize = tw;
   nt->tiles.offset_x = tiles_x;
   nt->tiles.offset_y = tiles_y;
   
   if (mode == MODE_RESET)
     nt->tiles.objs = malloc(tiles_w * tiles_h * sizeof(Evas_Object *));
   
   if (nt->tiles.objs)
     {
	for (j = 0; j < nt->tiles.oh; j++)
	  {
	     y = (j * nt->tiles.tilesize);
	     yy = ((j + 1) * nt->tiles.tilesize);
	     for (i = 0; i < nt->tiles.ow; i++)
	       {
		  Evas_Object *o;
		  char buf[PATH_MAX];
		  
		  if (mode == MODE_RESET)
		    {
		       o = evas_object_image_add(evas_object_evas_get(nt->obj));
		       nt->tiles.objs[(j * nt->tiles.ow) + i] = o;
		       evas_object_smart_member_add(o, nt->obj);
		       evas_object_clip_set(o, sd->clip);
		       evas_object_pass_events_set(o, 1);
		       evas_object_stack_below(o, sd->clip);
		    }
		  else
		    o = nt->tiles.objs[(j * nt->tiles.ow) + i];
		  
		  if ((mode == MODE_RESET) || (mode == MODE_ORIGIN))
		    {
		       snprintf(buf, sizeof(buf), "%s/%s/w-%i-%i-%i.%s",
				mapdir, mapset,
				nt->level, 
				i + nt->tiles.ox,
				j + nt->tiles.oy,
				mapformat);
		       evas_object_image_file_set(o, buf, NULL);
		       if (evas_object_image_load_error_get(o) == EVAS_LOAD_ERROR_NONE)
			 evas_object_show(o);
		       else
			 evas_object_hide(o);
		    }
		  x = (i * nt->tiles.tilesize);
		  xx = ((i + 1) * nt->tiles.tilesize);
		  evas_object_move(o, 
				   sd->x + nt->tiles.offset_x + x,
				   sd->y + nt->tiles.offset_y + y);
		  if ((mode == MODE_RESET) || (mode == MODE_ORIGIN) ||
		      (mode == MODE_RESIZE))
		    {
		       evas_object_resize(o, xx - x, yy - y);
		       evas_object_image_fill_set(o, 0, 0, xx - x, yy - y);
		    }
	       }
	  }
     }
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
