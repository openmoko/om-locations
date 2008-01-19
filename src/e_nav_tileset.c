#include "e_nav.h"
#include "e_nav_tileset.h"
#include <e_dbus_proxy.h>
#include <math.h>

typedef struct _E_Smart_Data E_Smart_Data;

struct _E_Smart_Data
{
   Evas_Object *obj;
   Evas_Object *clip;
   Evas_Object *dummy;
   Evas_Coord x, y, w, h;

   Evas_Object *nav;
   E_DBus_Proxy *proxy;

   E_Nav_Tileset_Format format;
   char *dir;
   char *map;
   char *suffix;
   int size;
   int min_level, max_level, level;
   double lon, lat;

   Evas_Bool smooth;
   double smooth_level;

   struct {
      double tilesize;
      Evas_Coord offset_x, offset_y;
      double olevel;
      int ox, oy, ow, oh;
      Evas_Object **objs;
   } tiles;
};

static void _e_nav_tileset_smart_init(void);
static void _e_nav_tileset_smart_add(Evas_Object *obj);
static void _e_nav_tileset_smart_del(Evas_Object *obj);
static void _e_nav_tileset_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_nav_tileset_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_nav_tileset_smart_show(Evas_Object *obj);
static void _e_nav_tileset_smart_hide(Evas_Object *obj);
static void _e_nav_tileset_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_nav_tileset_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_nav_tileset_smart_clip_unset(Evas_Object *obj);

static void _e_nav_tileset_update(Evas_Object *obj);

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_nav_tileset")) return ret

#define TILE_VALID(lv, num) ((num) >= 0 && (num) < (1 << lv))

Evas_Object *
e_nav_tileset_add(Evas_Object *nav, E_Nav_Tileset_Format format, const char *dir)
{
   Evas_Object *obj;
   E_Smart_Data *sd;

   _e_nav_tileset_smart_init();
   obj = evas_object_smart_add(evas_object_evas_get(nav), _e_smart);
   if (!obj) return NULL;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
     {
	evas_object_del(obj);
	return NULL;
     }

   sd->nav = nav;

   sd->format = format;
   sd->dir = strdup(dir);
   sd->map = strdup("tah");
   sd->suffix = strdup("png");
   sd->size = 256;
   sd->min_level = 0;
   sd->max_level = 17;

   sd->level = (double) sd->min_level;
   sd->lon = sd->lat = 0.0;
   sd->smooth = 0;

   e_nav_world_tileset_add(nav, obj);

   return obj;
}

void
e_nav_tileset_update(Evas_Object *obj)
{
   _e_nav_tileset_update(obj);
}

void
e_nav_tileset_level_set(Evas_Object *obj, double level)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (sd->smooth)
     sd->smooth_level = level;

   if (level > (double) sd->max_level)
     level = (double) sd->max_level;
   else if (level < (double) sd->min_level)
     level = (double) sd->min_level;
   sd->level = ((int) (level + 0.5));
}

double
e_nav_tileset_level_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0.0;);

   return (sd->smooth) ? sd->smooth_level : (double) sd->level;
}

void
e_nav_tileset_levels_list(Evas_Object *obj, int *max_level, int *min_level)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (max_level)
     *max_level = sd->max_level;
   if (min_level)
     *min_level = sd->min_level;
}

void
e_nav_tileset_center_set(Evas_Object *obj, double lon, double lat)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   while (lon > 180.0) lon -= 360.0;
   while (lon < -180.0) lon += 360.0;
   if (lat > 90.0) lon = 90.0;
   if (lat < -90.0) lon = -90.0;

   sd->lon = lon;
   sd->lat = lat;
}

void
e_nav_tileset_center_get(Evas_Object *obj, double *lon, double *lat)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (lon)
      *lon = sd->lon;
   if (lat)
      *lat = sd->lat;
}

void
e_nav_tileset_smooth_set(Evas_Object *obj, Evas_Bool smooth)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (sd->smooth == smooth)
     return;

   sd->smooth = smooth;
   if (smooth)
     sd->smooth_level = (double) sd->level;
}

Evas_Bool
e_nav_tileset_smooth_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0;);

   return sd->smooth;
}

void
e_nav_tileset_scale_set(Evas_Object *obj, double scale)
{
   E_Smart_Data *sd;
   double perimeter, tiles, level;
   
   SMART_CHECK(obj, ;);

   if (scale < 0.0001)
     scale = 0.0001;

   perimeter = cos(RADIANS(sd->lat)) * M_EARTH_RADIUS * M_PI * 2;
   tiles = perimeter / scale / sd->size;
   
   level = log((double) tiles) / M_LOG2;

   e_nav_tileset_level_set(obj, level);
}

double
e_nav_tileset_scale_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   double perimeter;
   unsigned int pixels;
   
   SMART_CHECK(obj, 0.0;);

   if (sd->smooth)
     pixels = (unsigned int) pow(2, sd->smooth_level);
   else
     pixels = (1 << sd->level);

   perimeter = cos(RADIANS(sd->lat)) * M_EARTH_RADIUS * M_PI * 2;

   return perimeter / pixels;
}

void
e_nav_tileset_proxy_set(Evas_Object *obj, E_DBus_Proxy *proxy)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   sd->proxy = proxy;
}

E_DBus_Proxy *
e_nav_tileset_proxy_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);

   return sd->proxy;
}

/* internal calls */
static void
_e_nav_tileset_smart_init(void)
{
   if (_e_smart) return;

   {
      static const Evas_Smart_Class sc =
      {
	 "e_nav_tileset",
	 EVAS_SMART_CLASS_VERSION,
	 _e_nav_tileset_smart_add,
	 _e_nav_tileset_smart_del,
	 _e_nav_tileset_smart_move,
	 _e_nav_tileset_smart_resize,
	 _e_nav_tileset_smart_show,
	 _e_nav_tileset_smart_hide,
	 _e_nav_tileset_smart_color_set,
	 _e_nav_tileset_smart_clip_set,
	 _e_nav_tileset_smart_clip_unset,

	 NULL /* data */
      };
      _e_smart = evas_smart_class_new(&sc);
   }
}

static void
_e_nav_tileset_smart_add(Evas_Object *obj)
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
   evas_object_color_set(sd->clip, 255, 255, 255, 255);

   sd->dummy = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->dummy, obj);
   evas_object_clip_set(sd->dummy, sd->clip);

   evas_object_smart_data_set(obj, sd);
}

static void
_e_nav_tileset_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   int i, j;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   if (sd->dir) free(sd->dir);
   if (sd->map) free(sd->map);
   if (sd->suffix) free(sd->suffix);

   if (sd->tiles.objs)
     {
	for (j = 0; j < sd->tiles.oh; j++)
	  {
	     for (i = 0; i < sd->tiles.ow; i++)
	       {
		  Evas_Object *o;

		  o = sd->tiles.objs[(j * sd->tiles.ow) + i];
		  if (o)
		    evas_object_del(o);
	       }
	  }
	free(sd->tiles.objs);
     }

   evas_object_del(sd->dummy);
   evas_object_del(sd->clip);

   free(sd);
}

static void
_e_nav_tileset_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;
   evas_object_move(sd->clip, sd->x, sd->y);
   _e_nav_tileset_update(obj);
}

static void
_e_nav_tileset_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->clip, sd->w, sd->h);
   _e_nav_tileset_update(obj);
}

static void
_e_nav_tileset_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_nav_tileset_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_nav_tileset_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_nav_tileset_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_nav_tileset_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
} 

inline static void mercator_project(double lon, double lat, double *x, double *y)
{
   double tmp;

   *x = (lon + 180.0) / 360.0;

   /* avoid NaN */
   if (lat > 89.99)
     lat = 89.99;
   else if (lat < -89.99)
     lat = -89.99;

   tmp = RADIANS(lat);
   tmp = log(tan(tmp) + 1.0 / cos(tmp));
   *y = (1.0 - tmp / M_PI) / 2.0;
}

static void
_e_nav_tileset_update(Evas_Object *obj)
{
   E_Smart_Data *sd;
   int i, j;
   int tiles_ow, tiles_oh, tiles_ox, tiles_oy;
   int offset_x, offset_y;
   int num_tiles;
   double tilesize;
   double tpx, tpy;
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
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   snprintf(mapbuf, sizeof(mapbuf), "%s/maps", sd->dir);
   
   mapdir = mapbuf;
   mapset = sd->map;
   mapformat = sd->suffix;
   
   num_tiles = (1 << sd->level);

   if (sd->smooth)
     tilesize = pow(2, sd->smooth_level) * sd->size / num_tiles;
   else
     tilesize = sd->size;

   if (tilesize < 1.0) return;

   tiles_ow = 1 + (((double)sd->w + tilesize) / tilesize);
   tiles_oh = 1 + (((double)sd->h + tilesize) / tilesize);

   mercator_project(sd->lon, sd->lat, &tpx, &tpy);

   /* starting from left-top corner */
   tpx = (tpx * num_tiles) - (sd->w / 2.0) / tilesize;
   tpy = (tpy * num_tiles) - (sd->h / 2.0) / tilesize;

   tiles_ox = (int)tpx;
   tiles_oy = (int)tpy;

   offset_x = -tilesize * (tpx - tiles_ox);
   offset_y = -tilesize * (tpy - tiles_oy);

   if ((sd->tiles.ow != tiles_ow) || (sd->tiles.oh != tiles_oh) ||
       (sd->smooth && sd->smooth_level != sd->tiles.olevel) ||
       (!sd->smooth && (double) sd->level != sd->tiles.olevel))
     mode = MODE_RESET;
   else if ((sd->tiles.ox != tiles_ox) || (sd->tiles.oy != tiles_oy))
     mode = MODE_ORIGIN;
   else if (tilesize != sd->tiles.tilesize)
     mode = MODE_RESIZE;
   else if ((offset_x != sd->tiles.offset_x) || (offset_y != sd->tiles.offset_y))
     mode = MODE_MOVE;
     
   if (mode == MODE_NONE) return;

   if (mode == MODE_RESET)
     {
	// reallac entirely move and resize
	if (sd->tiles.objs)
	  {
	     for (j = 0; j < sd->tiles.oh; j++)
	       {
		  for (i = 0; i < sd->tiles.ow; i++)
		    {
		       Evas_Object *o;

		       o = sd->tiles.objs[(j * sd->tiles.ow) + i];
		       if (o)
			 evas_object_del(o);
		    }
	       }
	     free(sd->tiles.objs);
	  }
     }
   
   sd->tiles.olevel = (sd->smooth) ? sd->smooth_level : sd->level;
   sd->tiles.ow = tiles_ow;
   sd->tiles.oh = tiles_oh;
   sd->tiles.ox = tiles_ox;
   sd->tiles.oy = tiles_oy;
   sd->tiles.tilesize = tilesize;
   sd->tiles.offset_x = offset_x;
   sd->tiles.offset_y = offset_y;

   if (mode == MODE_RESET)
     sd->tiles.objs = calloc(tiles_ow * tiles_oh, sizeof(Evas_Object *));
   
   if (!sd->tiles.objs)
     return;

   /*
   printf("(%d, %d), (%d, %d), (%d, %d), level %d, tilesize %f\n",
		   tiles_ox, tiles_oy,
		   tiles_ow, tiles_oh,
		   offset_x, offset_y, sd->level, tilesize);
		   */

   for (j = 0; j < sd->tiles.oh; j++)
     {
	y = (j * sd->tiles.tilesize);
	yy = ((j + 1) * sd->tiles.tilesize);

	if (!TILE_VALID(sd->level, sd->tiles.oy + j))
	  continue;

	for (i = 0; i < sd->tiles.ow; i++)
	  {
	     Evas_Object *o;
	     char buf[PATH_MAX];
	    
	     if (!TILE_VALID(sd->level, sd->tiles.ox + i))
	       continue;

	     if (mode == MODE_RESET)
	       {
		  o = evas_object_image_add(evas_object_evas_get(sd->obj));
		  sd->tiles.objs[(j * sd->tiles.ow) + i] = o;
		  evas_object_smart_member_add(o, sd->obj);
		  evas_object_clip_set(o, sd->clip);
		  evas_object_pass_events_set(o, 1);
		  evas_object_stack_below(o, sd->clip);
		  evas_object_image_smooth_scale_set(o, 0);
	       }
	     else
	       o = sd->tiles.objs[(j * sd->tiles.ow) + i];
	    
	     if ((mode == MODE_RESET) || (mode == MODE_ORIGIN))
	       {
		  snprintf(buf, sizeof(buf), "%s/%s_%i_%i_%i.%s",
			   mapdir, mapset,
			   sd->level, 
			   i + sd->tiles.ox,
			   j + sd->tiles.oy,
			   mapformat);
		  evas_object_image_file_set(o, buf, NULL);
		  if (evas_object_image_load_error_get(o) == EVAS_LOAD_ERROR_NONE)
		    evas_object_show(o);
		  else
		    evas_object_hide(o);
	       }
	     x = (i * sd->tiles.tilesize);
	     xx = ((i + 1) * sd->tiles.tilesize);
	     evas_object_move(o, 
			      sd->x + sd->tiles.offset_x + x,
			      sd->y + sd->tiles.offset_y + y);

	     if ((mode == MODE_RESET) || (mode == MODE_ORIGIN) ||
		 (mode == MODE_RESIZE))
	       {
		  evas_object_resize(o, xx - x, yy - y);
		  evas_object_image_fill_set(o, 0, 0, xx - x, yy - y);
	       }
	  }
     }
}
