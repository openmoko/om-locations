#include "e_nav.h"
#include "e_nav_tileset.h"
#include <e_dbus_proxy.h>
#include <math.h>

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Nav_Tile_Job E_Nav_Tile_Job;

#define NUM_JOB_SLOTS 5
struct _E_Smart_Data
{
   Evas_Object *obj;
   Evas_Object *clip;
   Evas_Object *dummy;
   Evas_Coord x, y, w, h;

   Evas_Object *nav;
   E_DBus_Proxy *proxy;

   E_Nav_Tile_Job *job_slots;
   Ecore_Hash *jobs;

   E_Nav_Tileset_Format format;
   char *dir;
   char *map;
   char *suffix;
   int size;
   int min_level, max_level;

   double lon, lat;
   int span;
   int level;

   struct {
      double tilesize;
      Evas_Coord offset_x, offset_y;
      int olevel, ospan;
      int ox, oy, ow, oh;
      Evas_Object **objs;
   } tiles;
};

struct _E_Nav_Tile_Job
{
   int level;
   int x, y;
   int id;
   int timestamp;
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

static void job_level_set(Evas_Object *obj);
static unsigned int job_hash(const E_Nav_Tile_Job *job);
static int job_compare(const E_Nav_Tile_Job *job1, const E_Nav_Tile_Job *job2);
static int job_submit(Evas_Object *obj, int x, int y, char force);
static void job_completed_cb(void *data, DBusMessage *message);

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

   e_nav_world_tileset_add(nav, obj);

   e_nav_tileset_center_set(obj, 0.0, 0.0);
   e_nav_tileset_level_set(obj, sd->min_level);

   return obj;
}

void
e_nav_tileset_update(Evas_Object *obj)
{
   _e_nav_tileset_update(obj);
}

void
e_nav_tileset_level_set(Evas_Object *obj, int level)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (level > sd->max_level)
     level = sd->max_level;
   else if (level < sd->min_level)
     level = sd->min_level;

   if (sd->level != level)
     {
	sd->level = level;
	job_level_set(obj);
     }
   sd->span = sd->size << level;
}

int
e_nav_tileset_level_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0.0;);

   return sd->level;
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
e_nav_tileset_span_set(Evas_Object *obj, int span)
{
   E_Smart_Data *sd;
   int level;
   
   SMART_CHECK(obj, ;);

   level = (int) ((log((double) span / sd->size) / M_LOG2) + 0.5);

   sd->span = span;

   if (level > sd->max_level)
     level = sd->max_level;
   else if (level < sd->min_level)
     level = sd->min_level;

   if (sd->level != level)
     {
	sd->level = level;
	job_level_set(obj);
     }
}

int
e_nav_tileset_span_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0;);

   return sd->span;
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
e_nav_tileset_to_offsets(Evas_Object *obj, double lon, double lat, double *x, double *y)
{
   E_Smart_Data *sd;
   double tmp;

   SMART_CHECK(obj, ;);

   if (x)
     *x = (lon - sd->lon) * sd->span / 360.0;

   tmp = cos(RADIANS(sd->lat));
   if (tmp == 0.0)
     tmp = 0.0000001;

   if (y)
     *y = (lat - sd->lat) * sd->span / 360.0 / tmp;
}

void
e_nav_tileset_from_offsets(Evas_Object *obj, double x, double y, double *lon, double *lat)
{
   E_Smart_Data *sd;

   SMART_CHECK(obj, ;);

   if (lon)
     *lon = x * 360.0 / sd->span;
   if (lat)
     *lat = y * 360.0 * cos(RADIANS(sd->lat)) / sd->span;
}

void
e_nav_tileset_scale_set(Evas_Object *obj, double scale)
{
   E_Smart_Data *sd;
   double perimeter;
   
   SMART_CHECK(obj, ;);

   perimeter = cos(RADIANS(sd->lat)) * M_EARTH_RADIUS * M_PI * 2;

   /* perimeter <= 2^26; limit scale to avoid overflow */
   if (scale < 0.0001)
     scale = 0.0001;

   e_nav_tileset_span_set(obj, perimeter / scale);
}

double
e_nav_tileset_scale_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   double perimeter;
   
   SMART_CHECK(obj, 0.0;);

   perimeter = cos(RADIANS(sd->lat)) * M_EARTH_RADIUS * M_PI * 2;

   return perimeter / sd->span;
}

void
e_nav_tileset_proxy_set(Evas_Object *obj, E_DBus_Proxy *proxy)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);

   if (sd->proxy == proxy)
     return;

   if (sd->proxy)
     e_dbus_proxy_disconnect_signal(sd->proxy, "OverlayChanged",
				    job_completed_cb, obj);

   sd->proxy = proxy;
   e_dbus_proxy_connect_signal(sd->proxy, "OverlayChanged",
			       job_completed_cb, obj);
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

   sd->jobs = ecore_hash_new((Ecore_Hash_Cb) job_hash,
			     (Ecore_Compare_Cb) job_compare);
   sd->job_slots = calloc(NUM_JOB_SLOTS, sizeof(E_Nav_Tile_Job));

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

   if (sd->proxy)
     e_dbus_proxy_disconnect_signal(sd->proxy, "OverlayChanged",
				    job_completed_cb, obj);
   ecore_hash_destroy(sd->jobs);
   free(sd->job_slots);

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

static void
job_level_set(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd || !sd->proxy) return;

   e_dbus_proxy_simple_call(sd->proxy, "SetLevel",
			    NULL,
			    DBUS_TYPE_INT32, &sd->level,
			    DBUS_TYPE_INVALID,
			    DBUS_TYPE_INVALID);
}

static unsigned int
job_hash(const E_Nav_Tile_Job *job)
{
   /* x and y are usually continuous; ignore higer bits */
   return ((job->level & 0x1f) << 27) |
	  ((job->y & 0x3ff) << 13) |
	  (job->x & 0x1ff);
}

static int
job_compare(const E_Nav_Tile_Job *job1, const E_Nav_Tile_Job *job2)
{
   if (job1->x < job2->x)
     return -1;
   else if (job1->x > job2->x)
     return 1;
   else if (job1->y < job2->y)
     return -1;
   else if (job1->y < job2->y)
     return 1;
   else
      return job1->level - job2->level;
}

static int
job_submit(Evas_Object *obj, int x, int y, char force)
{
   E_Smart_Data *sd;
   E_Nav_Tile_Job *job;
   int id, i;

   sd = evas_object_smart_data_get(obj);
   if (!sd || !sd->proxy) return 0;

   printf("submit job for tile (%d,%d)@%d\n", x, y, sd->level);

   for (i = 0; i < NUM_JOB_SLOTS; i++)
     {
	job = &sd->job_slots[i];
	if (!job->id)
	  break;
     }
   if (i == NUM_JOB_SLOTS)
     {
	printf("all slots used\n");

	return 0;
     }

   job->level = sd->level;
   job->x = x;
   job->y = y;

   if (ecore_hash_get(sd->jobs, job))
     {
	printf("job alredy on the way!\n");

	return 1;
     }

   if (!e_dbus_proxy_simple_call(sd->proxy, "GetTile",
				 NULL,
				 DBUS_TYPE_INT32, &x,
				 DBUS_TYPE_INT32, &y,
				 DBUS_TYPE_BOOLEAN, &force,
				 DBUS_TYPE_INVALID,
				 DBUS_TYPE_INT32, &id,
				 DBUS_TYPE_INVALID))
     return 0;

   printf("job %d submitted\n", id);

   job->id = id;
   job->timestamp = 0;

   ecore_hash_set(sd->jobs, job, job);

   return 1;
}

static void
job_completed_cb(void *data, DBusMessage *message)
{
   E_Smart_Data *sd;
   E_Nav_Tile_Job *job;
   int id, status;
   int i;

   if (!message)
     return;

   sd = evas_object_smart_data_get(data);
   if (!sd || !sd->proxy) return;

   if (!dbus_message_get_args(message, NULL,
			      DBUS_TYPE_INT32, &id,
			      DBUS_TYPE_INT32, &status,
			      DBUS_TYPE_INVALID))
     return;

   for (i = 0; i < NUM_JOB_SLOTS; i++)
     {
	job = &sd->job_slots[i];
	if (job->id == id)
	  break;
     }
   if (i == NUM_JOB_SLOTS)
     return;

   printf("job %d completed with status %d\n", id, status);

   job->id = 0;
   ecore_hash_remove(sd->jobs, job);
}

inline static void
mercator_project(double lon, double lat, double *x, double *y)
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
_e_nav_tileset_tile_get(Evas_Object *obj, Evas_Object *tile, int x, int y)
{
   E_Smart_Data *sd;
   char buf[PATH_MAX];

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   snprintf(buf, sizeof(buf), "%s/%s_%i_%i_%i.%s",
			   sd->dir, sd->map,
			   sd->level, x, y,
			   sd->suffix);

   evas_object_image_file_set(tile, buf, NULL);
   if (evas_object_image_load_error_get(tile) == EVAS_LOAD_ERROR_NONE)
      evas_object_show(tile);
   else
   {
      job_submit(obj, x, y, 0);
      evas_object_hide(tile);
   }
}

enum {
   MODE_NONE,
   MODE_RESET,
   MODE_ORIGIN,
   MODE_RESIZE,
   MODE_MOVE
};

static int
_e_nav_tileset_check(Evas_Object *obj)
{
   E_Smart_Data *sd;
   int i, j;
   int tiles_ow, tiles_oh, tiles_ox, tiles_oy;
   int offset_x, offset_y;
   int num_tiles;
   double tilesize;
   double tpx, tpy;
   int mode = MODE_NONE;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return MODE_NONE;

   num_tiles = (1 << sd->level);

   tilesize = sd->span / num_tiles;
   if (tilesize < 1.0) return MODE_NONE;

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
       (sd->tiles.olevel != sd->level))
     mode = MODE_RESET;
   else if ((sd->tiles.ox != tiles_ox) || (sd->tiles.oy != tiles_oy))
     mode = MODE_ORIGIN;
   else if (tilesize != sd->tiles.tilesize)
     mode = MODE_RESIZE;
   else if ((offset_x != sd->tiles.offset_x) || (offset_y != sd->tiles.offset_y))
     mode = MODE_MOVE;
     
   if (mode == MODE_RESET)
     {
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

	sd->tiles.objs = calloc(tiles_ow * tiles_oh, sizeof(Evas_Object *));
     }

   if (mode != MODE_NONE)
     {
	sd->tiles.ospan = sd->span;
	sd->tiles.olevel = sd->level;
	sd->tiles.ow = tiles_ow;
	sd->tiles.oh = tiles_oh;
	sd->tiles.ox = tiles_ox;
	sd->tiles.oy = tiles_oy;
	sd->tiles.tilesize = tilesize;
	sd->tiles.offset_x = offset_x;
	sd->tiles.offset_y = offset_y;
     }

   /*
   printf("(%d, %d), (%d, %d), (%d, %d), level %d, tilesize %f\n",
		   tiles_ox, tiles_oy,
		   tiles_ow, tiles_oh,
		   offset_x, offset_y, sd->level, tilesize);
		   */

   return mode;
}

static void
_e_nav_tileset_update(Evas_Object *obj)
{
   E_Smart_Data *sd;
   int mode, i, j;
   Evas_Coord x, y, xx, yy;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   mode = _e_nav_tileset_check(obj);
   if (mode == MODE_NONE)
     return;

   if (!sd->tiles.objs)
     return;

   for (j = 0; j < sd->tiles.oh; j++)
     {
	y = (j * sd->tiles.tilesize);
	yy = ((j + 1) * sd->tiles.tilesize);

	if (!TILE_VALID(sd->level, sd->tiles.oy + j))
	  continue;

	for (i = 0; i < sd->tiles.ow; i++)
	  {
	     Evas_Object *o;
	    
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
		  _e_nav_tileset_tile_get(obj, o, sd->tiles.ox + i, sd->tiles.oy + j);
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
