#include "e_nav.h"
#include "e_tagmenu.h"

/* navigator object */
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Tagmenu_Item E_Tagmenu_Item;

struct _E_Tagmenu_Item
{
   Evas_Object *obj;
   Evas_Object *item_obj;
   Evas_Coord sz;
   void (*func) (void *data, Evas_Object *obj, Evas_Object *src_obj);
   void *data;
};

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *src_obj;
   Evas_Object     *event;
   
   Evas_Object     *clip;

   Evas_List      *items;
   double           activate_time;
   int              activate_deactivate;
   unsigned char direction : 2;  // 0: North, 1: East, 2: South, 3: West
   Ecore_Animator  *animator;
   
   /* directory to find theme .edj files from the module - if there is one */
   const char      *dir;
   
   unsigned char autodelete : 1;
   unsigned char active : 1;
};

static void _e_tagmenu_smart_init(void);
static void _e_tagmenu_smart_add(Evas_Object *obj);
static void _e_tagmenu_smart_del(Evas_Object *obj);
static void _e_tagmenu_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_tagmenu_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_tagmenu_smart_show(Evas_Object *obj);
static void _e_tagmenu_smart_hide(Evas_Object *obj);
static void _e_tagmenu_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_tagmenu_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_tagmenu_smart_clip_unset(Evas_Object *obj);

static Evas_Object *_e_tagmenu_theme_obj_new(Evas *e, const char *custom_dir, const char *group);
static void _e_tagmenu_update(Evas_Object *obj);
static void _e_tagmenu_cb_src_obj_del(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_tagmenu_cb_src_obj_move(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_tagmenu_cb_src_obj_resize(void *data, Evas *evas, Evas_Object *obj, void *event);
static int _e_tagmenu_cb_animator(void *data);
static void _e_tagmenu_cb_event_down(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_tagmenu_cb_item_up(void *data, Evas *evas, Evas_Object *obj, void *event);

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_tagmenu")) return ret

Evas_Object *
e_tagmenu_add(Evas *e)
{
   _e_tagmenu_smart_init();
   return evas_object_smart_add(e, _e_smart);
}

void
e_tagmenu_theme_source_set(Evas_Object *obj, const char *custom_dir)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   
   sd->dir = custom_dir;
}

void
e_tagmenu_source_object_set(Evas_Object *obj, Evas_Object *src_obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   if (sd->src_obj)
     {
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_DEL,
				       _e_tagmenu_cb_src_obj_del);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_MOVE,
				       _e_tagmenu_cb_src_obj_move);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_RESIZE,
				       _e_tagmenu_cb_src_obj_resize);
     }
   sd->src_obj = src_obj;
   if (sd->src_obj)
     {
	evas_object_event_callback_add(sd->src_obj, EVAS_CALLBACK_DEL,
				       _e_tagmenu_cb_src_obj_del, obj);
	evas_object_event_callback_add(sd->src_obj, EVAS_CALLBACK_MOVE,
				       _e_tagmenu_cb_src_obj_move, obj);
	evas_object_event_callback_add(sd->src_obj, EVAS_CALLBACK_RESIZE,
				       _e_tagmenu_cb_src_obj_resize, obj);
     }
   _e_tagmenu_update(obj);
}

Evas_Object *
e_tagmenu_source_object_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, NULL;);
   return sd->src_obj;
}
  
void
e_tagmenu_autodelete_set(Evas_Object *obj, Evas_Bool autodelete)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   sd->autodelete = autodelete;
}

Evas_Bool
e_tagmenu_autodelete_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, 0;);
   return sd->autodelete;
}

void
e_tagmenu_activate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_List *l;
   E_Tagmenu_Item *si;
   
   SMART_CHECK(obj, ;);
   evas_object_show(sd->event);
   if (sd->active) return;
   sd->activate_deactivate = 1;
   sd->active = 1;
   sd->activate_time = ecore_time_get();
   for (l = sd->items; l; l = l->next)
     {
	si = l->data;
        evas_object_show(si->item_obj); 
     }
   
   if (sd->animator) return;
   sd->animator = ecore_animator_add(_e_tagmenu_cb_animator, obj);
   
}

void
e_tagmenu_deactivate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_List *l;
   E_Tagmenu_Item *si;
   
   SMART_CHECK(obj, ;);
   evas_object_hide(sd->event);

   if (!sd->active) return;
   sd->activate_deactivate = -1;
   sd->activate_time = ecore_time_get();
   
   for (l = sd->items; l; l = l->next)
     {
	si = l->data;
        evas_object_hide(si->item_obj);
     }

   if (sd->animator) return;
   sd->animator = ecore_animator_add(_e_tagmenu_cb_animator, obj);
    
}

void
e_tagmenu_theme_item_add(Evas_Object *obj, const char *icon, Evas_Coord size, const char *label, void (*func) (void *data, Evas_Object *obj, Evas_Object *src_obj), void *data)
{
   E_Smart_Data *sd;
   E_Tagmenu_Item *si;
   
   SMART_CHECK(obj, ;);
   /* add menu item */
   si = calloc(1, sizeof(E_Tagmenu_Item));
   si->obj = obj;
   si->sz = size;
   si->func = func;
   si->data = data;
   si->item_obj = _e_tagmenu_theme_obj_new(evas_object_evas_get(obj), sd->dir, icon);
   evas_object_smart_member_add(si->item_obj, obj);
   evas_object_clip_set(si->item_obj, sd->clip);
   evas_object_event_callback_add(si->item_obj, EVAS_CALLBACK_MOUSE_UP,
				  _e_tagmenu_cb_item_up, si);
   
   edje_object_part_text_set(si->item_obj, "text", label);
   sd->items = evas_list_append(sd->items, si);
}

/* internal calls */
static void
_e_tagmenu_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     "e_tagmenu",
	       EVAS_SMART_CLASS_VERSION,
	       _e_tagmenu_smart_add,
	       _e_tagmenu_smart_del,
	       _e_tagmenu_smart_move,
	       _e_tagmenu_smart_resize,
	       _e_tagmenu_smart_show,
	       _e_tagmenu_smart_hide,
	       _e_tagmenu_smart_color_set,
	       _e_tagmenu_smart_clip_set,
	       _e_tagmenu_smart_clip_unset,
	       
	       NULL /* data */
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_tagmenu_smart_add(Evas_Object *obj)
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
   evas_object_move(sd->clip, -10000, -10000);
   evas_object_resize(sd->clip, 30000, 30000);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_show(sd->clip);

   sd->event = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->event, obj);
   evas_object_move(sd->event, -10000, -10000);
   evas_object_resize(sd->event, 30000, 30000);
   evas_object_color_set(sd->event, 255, 255, 255, 0);
   evas_object_clip_set(sd->event, sd->clip);
   evas_object_event_callback_add(sd->event, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_tagmenu_cb_event_down, obj);
   
   evas_object_smart_data_set(obj, sd);
}

static void
_e_tagmenu_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->src_obj)
     {
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_DEL,
				       _e_tagmenu_cb_src_obj_del);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_MOVE,
				       _e_tagmenu_cb_src_obj_move);
	evas_object_event_callback_del(sd->src_obj, EVAS_CALLBACK_RESIZE,
				       _e_tagmenu_cb_src_obj_resize);
     }
   while (sd->items)
     {
	E_Tagmenu_Item *si;
	
	si = sd->items->data;
	sd->items = evas_list_remove_list(sd->items, sd->items);
	evas_object_del(si->item_obj);
	free(si);
     }
   evas_object_del(sd->clip);
   evas_object_del(sd->event);
   free(sd);
}
                    
static void
_e_tagmenu_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;
   _e_tagmenu_update(obj);
}

static void
_e_tagmenu_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   _e_tagmenu_update(obj);
}

static void
_e_tagmenu_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_tagmenu_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_tagmenu_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_tagmenu_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_tagmenu_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
} 

static Evas_Object *
_e_tagmenu_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
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
_e_tagmenu_update(Evas_Object *obj)
{

   E_Smart_Data *sd;
   Evas_List *l;
   E_Tagmenu_Item *si;
   Evas_Coord x, y, w, h, xx, yy, ww, hh;
   double t;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->items) return;
   evas_object_geometry_get(sd->src_obj, &x, &y, &w, &h);
   int tsize = 0;
   if (sd->activate_deactivate == 0)
     {
	for (l = sd->items; l; l = l->next)
          {
	     si = l->data;
             tsize = tsize + si->sz; 
          }
        int size = 0;
	for (l = sd->items; l; l = l->next)
	  {
	     si = l->data;
	     if (!sd->active)
	       evas_object_hide(si->item_obj);
	     else
	       {
		  xx =  (-tsize/2) + size + (si->sz/2);
		  yy =  (-h/2) -20 - (50/2) ;
		  ww = si->sz ;
		  hh = 50 ;
		  evas_object_move(si->item_obj, 
				   x + (w / 2) + xx - (ww / 2), 
				   y + (h / 2) + yy - (hh / 2));
		  evas_object_resize(si->item_obj, ww, hh);
		  evas_object_show(si->item_obj);
                  size = size + si->sz;
	       }
	  }
	if (!sd->active)
	  evas_object_hide(sd->event);
	else
	  evas_object_show(sd->event);
     }
   else if (sd->activate_deactivate == 1)
     {
	t = ecore_time_get() - sd->activate_time;
	t = t / 1.0; /* anim time */
	if (t >= 1.0) t = 1.0;
	t = 1.0 - ((1.0 - t) * (1.0 - t)); /* decelerate */
	if (t >= 1.0) sd->activate_deactivate = 0;
	evas_object_geometry_get(sd->src_obj, &x, &y, &w, &h);
	for (l = sd->items; l; l = l->next)
          {
	     si = l->data;
             tsize = tsize + si->sz; 
          }
        int size = 0;
	for (l = sd->items; l; l = l->next)
	  {
	     si = l->data;
	     evas_object_move(si->item_obj, x+(w/2)-(tsize/2)+size, (y*t)-50-20);
	     evas_object_resize(si->item_obj, si->sz, 50);
	     evas_object_show(si->item_obj);
             size = size + si->sz;
	  }

     }
   else if (sd->activate_deactivate == -1)
     {
	sd->activate_deactivate = 0;
	sd->active = 0;
	for (l = sd->items; l; l = l->next)
	  {
	     si = l->data;
	     if (!sd->active) evas_object_hide(si->item_obj);
	  }
	evas_object_hide(sd->event);

     }
   
}

static void
_e_tagmenu_cb_src_obj_del(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   if (sd->autodelete)
     evas_object_del(sd->obj);
}

static void
_e_tagmenu_cb_src_obj_move(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   _e_tagmenu_update(sd->obj);
}

static void
_e_tagmenu_cb_src_obj_resize(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return;
   _e_tagmenu_update(sd->obj);
}

static int
_e_tagmenu_cb_animator(void *data)
{
   E_Smart_Data *sd;
     
   sd = evas_object_smart_data_get(data);
   if (!sd) return 0;
 
   _e_tagmenu_update(sd->obj);
   if (sd->activate_deactivate == 0)
     {
	sd->animator = NULL;
	if (!sd->active) evas_object_del(sd->obj);
	return 0;
     }
   return 1;
}

static void
_e_tagmenu_cb_event_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   e_tagmenu_deactivate(data);
}

static void
_e_tagmenu_cb_item_up(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Tagmenu_Item *si;
   E_Smart_Data *sd;
     
   si = data;
   if (!si) return;
   sd = evas_object_smart_data_get(si->obj);
   if (!sd) return;
   if (!sd->src_obj) return;
   if (!sd->active) return;
   if (si->func) si->func(si->data, si->obj, sd->src_obj);
}
