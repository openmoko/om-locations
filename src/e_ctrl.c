#include "e_nav.h"    // we need to control nav (map) , like zoom in/out
#include "e_ctrl.h"
#include "e_nav_tileset.h"
#include "widgets/e_ilist.h"
#include "widgets/e_icon.h"

static Evas_Object *ctrl = NULL;

typedef struct _E_Smart_Data E_Smart_Data;
static Evas_Object * _e_ctrl_theme_obj_new(Evas *e, const char *custom_dir, const char *group);
static void _e_ctrl_cb_signal_drag(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_ctrl_cb_signal_drag_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_ctrl_cb_signal_drag_stop(void *data, Evas_Object *obj, const char *emission, const char *source);

struct _E_Smart_Data
{
   Evas_Object *obj;      // we draw or add evas obj on this evas obj
   Evas_Object *clip;
   Evas_Object *overlay;   // control panel that controls ap screen state
   Evas_Object *nav;
   Evas_Object *listview; 
   Evas_Object *tagview;
   Evas_Coord x, y, w, h;
   const char      *dir;
   Evas_List *tags;   // store tags
   E_Nav_Show_Mode show_mode;
   E_Nav_View_Mode view_mode;
   // screen state (ui state)
};

static void _e_ctrl_smart_init(void);
static void _e_ctrl_smart_add(Evas_Object *obj);
static void _e_ctrl_smart_del(Evas_Object *obj);
static void _e_ctrl_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_ctrl_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_ctrl_smart_show(Evas_Object *obj);
static void _e_ctrl_smart_hide(Evas_Object *obj);
static void _e_ctrl_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_ctrl_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_ctrl_smart_clip_unset(Evas_Object *obj);

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_ctrl")) return ret


Evas_Object *
e_ctrl_add(Evas *e)
{
   _e_ctrl_smart_init();
   if(ctrl) return ctrl;
   ctrl = evas_object_smart_add(e, _e_smart);
   return ctrl;
}

static void
_e_test_sel(void *data, void *data2)
{
   printf("test sel\n");
}

static void  // ilist 
_e_nav_list_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(data);
   sd->view_mode = E_NAV_VIEW_MODE_LIST;
   evas_object_hide(sd->nav);
   if(sd->listview)
     {
        evas_object_show(sd->listview);
        return;
     }
   Evas_Coord mw, mh; //, vw, vh;
   Evas_Object *oitem;
   sd->listview = e_ilist_add(evas);
   e_ilist_icon_size_set(sd->listview, 64, 64);
   oitem = e_icon_add(evas);
   e_icon_file_set(oitem, "/home/jeremy/icons/video_player.png");
   e_ilist_append(sd->listview, oitem, "Item 1", 0, _e_test_sel, NULL, NULL, NULL);

   oitem = e_icon_add(evas);
   e_icon_file_set(oitem, "/home/jeremy/icons/image_viewer.png");
   e_ilist_append(sd->listview, oitem, "Item 2", 0, _e_test_sel, NULL, NULL, NULL);

   e_ilist_min_size_get(sd->listview, &mw, &mh);
   evas_object_resize(sd->listview, 480, mh);
   evas_object_focus_set(sd->listview, 1);
   evas_object_show(sd->listview);

}

static void
_e_nav_tagbook_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(data);
   if(sd->show_mode == E_NAV_SHOW_MODE_TAG)
     { 
        sd->show_mode = E_NAV_SHOW_MODE_TAGLESS;
     }
   else if(sd->show_mode == E_NAV_SHOW_MODE_TAGLESS)
     {
        sd->show_mode = E_NAV_SHOW_MODE_TAG;
     }
}

static void
_e_nav_map_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(data);
   if(sd->view_mode == E_NAV_VIEW_MODE_LIST)
      evas_object_hide(sd->listview);
   evas_object_show(sd->nav);
   sd->view_mode = E_NAV_VIEW_MODE_MAP;
}

static void
_e_nav_sat_button_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(data);
   if(sd->view_mode == E_NAV_VIEW_MODE_LIST)
      evas_object_hide(sd->listview);
   evas_object_show(sd->nav);
   sd->view_mode = E_NAV_VIEW_MODE_MAP;
}

static void
_e_ctrl_panel_cb_set(Evas_Object *obj)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   Evas_Object* o = edje_object_part_object_get(sd->overlay, "e.button.tagbook");
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_nav_tagbook_button_cb_mouse_down, obj);
   o = edje_object_part_object_get(sd->overlay, "e.button.map");
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_nav_map_button_cb_mouse_down, obj);
   o = edje_object_part_object_get(sd->overlay, "e.button.satellite");
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_nav_sat_button_cb_mouse_down, obj);
   o = edje_object_part_object_get(sd->overlay, "e.button.list");
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_nav_list_button_cb_mouse_down, obj);
}

void
e_ctrl_theme_source_set(Evas_Object *obj, const char *custom_dir)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   
   sd->dir = custom_dir;
   sd->overlay = _e_ctrl_theme_obj_new(evas_object_evas_get(obj), sd->dir,
				      "modules/diversity_nav/main");
   evas_object_smart_member_add(sd->overlay, obj);
   evas_object_move(sd->overlay, sd->x, sd->y);
   evas_object_resize(sd->overlay, sd->w, sd->h);
   evas_object_clip_set(sd->overlay, sd->clip);

   evas_object_show(sd->overlay);
   edje_object_signal_callback_add(sd->overlay, "drag", "*", _e_ctrl_cb_signal_drag, sd);
   edje_object_signal_callback_add(sd->overlay, "drag,start", "*", _e_ctrl_cb_signal_drag_start, sd);
   edje_object_signal_callback_add(sd->overlay, "drag,stop", "*", _e_ctrl_cb_signal_drag_stop, sd);
   edje_object_signal_callback_add(sd->overlay, "drag,step", "*", _e_ctrl_cb_signal_drag_stop, sd);
   edje_object_signal_callback_add(sd->overlay, "drag,set", "*", _e_ctrl_cb_signal_drag_stop, sd);

   _e_ctrl_panel_cb_set(obj);
}

static Evas_Object *
_e_ctrl_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;
   
   o = edje_object_add(e);
   if (!e_ctrl_edje_object_set(o, "diversity_nav", group))
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

int
e_ctrl_edje_object_set(Evas_Object *o, const char *category, const char *group)
{
   char buf[PATH_MAX];
   int ok;
   
   snprintf(buf, sizeof(buf), "%s/%s.edj", THEME_PATH, category);
   ok = edje_object_file_set(o, buf, group);
   return ok;
}

void e_ctrl_nav_set(Evas_Object* obj)
{
   if(!ctrl) return;
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   sd->nav = obj;
}

/* internal calls */
static void
_e_ctrl_smart_init(void)
{
   if (_e_smart) return;

   {
      static const Evas_Smart_Class sc =
      {
	 "e_ctrl",
	 EVAS_SMART_CLASS_VERSION,
	 _e_ctrl_smart_add,
	 _e_ctrl_smart_del,
	 _e_ctrl_smart_move,
	 _e_ctrl_smart_resize,
	 _e_ctrl_smart_show,
	 _e_ctrl_smart_hide,
	 _e_ctrl_smart_color_set,
	 _e_ctrl_smart_clip_set,
	 _e_ctrl_smart_clip_unset,

	 NULL /* data */
      };
      _e_smart = evas_smart_class_new(&sc);
   }
}

static void
_e_ctrl_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   sd->obj = obj;
   
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;

   sd->show_mode = E_NAV_SHOW_MODE_TAGLESS;
   sd->view_mode = E_NAV_VIEW_MODE_MAP;
   
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_smart_data_set(obj, sd);
}

static void
_e_ctrl_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_del(sd->clip);
   evas_object_del(sd->overlay);
   free(sd);
}

static void
_e_ctrl_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;

   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_move(sd->overlay, sd->x, sd->y);
}

static void
_e_ctrl_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_resize(sd->overlay, sd->w, sd->h);
}

static void
_e_ctrl_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_ctrl_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_ctrl_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_ctrl_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_ctrl_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}

void e_ctrl_zoom_drag_value_set(double y) 
{
   if(!ctrl) return;
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   edje_object_part_drag_value_set(sd->overlay, "e.dragable.zoom", 0.0, y);
}

void e_ctrl_zoom_text_value_set(const char* buf)
{
   if(!ctrl) return;
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   edje_object_part_text_set(sd->overlay, "e.text.zoom", buf);
}

void e_ctrl_latitude_set(const char* buf)
{
   if(!ctrl) return;
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   edje_object_part_text_set(sd->overlay, "e.text.latitude", buf);
}

void e_ctrl_longitude_set(const char* buf)
{
   if(!ctrl) return;
   E_Smart_Data *sd;
   sd = evas_object_smart_data_get(ctrl);
   if(!ctrl) return;
   edje_object_part_text_set(sd->overlay, "e.text.longitude", buf);
}
 
static void
_e_ctrl_cb_signal_drag(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   
   sd = data;
     {
	double x = 0, y = 0, z;
	
	edje_object_part_drag_value_get(sd->overlay, "e.dragable.zoom", &x, &y);
	y = (y * y) * (y * y);

	z = E_NAV_ZOOM_MIN + ((E_NAV_ZOOM_MAX - E_NAV_ZOOM_MIN) * y);
	e_nav_zoom_set(sd->nav, z, 0.2);
     }
}

static void
_e_ctrl_cb_signal_drag_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   
   sd = data;
     {
	double x = 0, y = 0;
	
	edje_object_part_drag_value_get(sd->overlay, "e.dragable.zoom", &x, &y);
     }
}

static void
_e_ctrl_cb_signal_drag_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   
   sd = data;
     {
	double x = 0, y = 0;
	
	edje_object_part_drag_value_get(sd->overlay, "e.dragable.zoom", &x, &y);
     }
}
