#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "e_ctrl_dialog.h"

typedef struct _E_Smart_Data E_Smart_Data;
static Evas_Object * _e_ctrl_dialog_theme_obj_new(Evas *e, const char *custom_dir, const char *group);

struct _E_Smart_Data
{
   Evas_Object *clip;
   Evas_Object *overlay;
   Evas_Object *obj;
   Evas_Coord x, y, w, h;
   const char      *dir;
};

static void _e_ctrl_dialog_smart_init(void);
static void _e_ctrl_dialog_smart_add(Evas_Object *obj);
static void _e_ctrl_dialog_smart_del(Evas_Object *obj);
static void _e_ctrl_dialog_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_ctrl_dialog_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_ctrl_dialog_smart_show(Evas_Object *obj);
static void _e_ctrl_dialog_smart_hide(Evas_Object *obj);
static void _e_ctrl_dialog_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_ctrl_dialog_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_ctrl_dialog_smart_clip_unset(Evas_Object *obj);

static Evas_Smart *_e_smart = NULL;

#define SMART_CHECK(obj, ret) \
   sd = evas_object_smart_data_get(obj); \
   if (!sd) return ret \
   if (strcmp(evas_object_type_get(obj), "e_ctrl_dialog")) return ret

Evas_Object *
e_ctrl_dialog_add(Evas *e)
{
   _e_ctrl_dialog_smart_init();
   return evas_object_smart_add(e, _e_smart);
}

void
e_ctrl_dialog_set_message(Evas_Object *obj, const char *message)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   edje_object_part_text_set(sd->overlay, "e.titlepane.message", message);
}

void
e_ctrl_dialog_set_left_button(Evas_Object *obj, const char *text, CallbackFunc cb, Evas_Object *src)  
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   edje_object_part_text_set(sd->overlay, "e.button.left.label", text);
   Evas_Object* o = edje_object_part_object_get(sd->overlay, "e.button.left");
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, cb, src);
}

void
e_ctrl_dialog_set_right_button(Evas_Object *obj, const char *text, CallbackFunc cb, Evas_Object *src)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   edje_object_part_text_set(sd->overlay, "e.button.right.label", text);
   Evas_Object* o = edje_object_part_object_get(sd->overlay, "e.button.right");
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, cb, src);
}

void
e_ctrl_dialog_hide_buttons(Evas_Object *obj)
{
   E_Smart_Data *sd;
   SMART_CHECK(obj, ;);
   Evas_Object *left_button = edje_object_part_object_get(sd->overlay, "e.button.left");
   Evas_Object *right_button = edje_object_part_object_get(sd->overlay, "e.button.right");
   evas_object_hide(left_button);
   evas_object_hide(right_button);
}

void
e_ctrl_dialog_theme_source_set(Evas_Object *obj, const char *custom_dir)
{
   E_Smart_Data *sd;
   
   SMART_CHECK(obj, ;);
   
   sd->dir = custom_dir;
   sd->overlay = _e_ctrl_dialog_theme_obj_new(evas_object_evas_get(obj), sd->dir,
				      "modules/diversity_nav/titlepane");
   evas_object_smart_member_add(sd->overlay, obj);
   evas_object_move(sd->overlay, sd->x, sd->y);
   evas_object_resize(sd->overlay, sd->w, sd->h);
   evas_object_clip_set(sd->overlay, sd->clip);

   evas_object_show(sd->overlay);
}

static Evas_Object *
_e_ctrl_dialog_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;
   
   o = edje_object_add(e);
   if (!e_ctrl_dialog_edje_object_set(o, "default", group))
     {
	if (custom_dir)
	  {
	     char buf[PATH_MAX];
	     
	     snprintf(buf, sizeof(buf), "%s/default.edj", custom_dir);
	     edje_object_file_set(o, buf, group);
	  }
     }
   return o;
}

int
e_ctrl_dialog_edje_object_set(Evas_Object *o, const char *category, const char *group)
{
   char buf[PATH_MAX];
   int ok;
   
   snprintf(buf, sizeof(buf), "%s/%s.edj", THEME_PATH, category);
   ok = edje_object_file_set(o, buf, group);
   return ok;
}

/* internal calls */
static void
_e_ctrl_dialog_smart_init(void)
{
   if (_e_smart) return;

   {
      static const Evas_Smart_Class sc =
      {
	 "e_ctrl_dialog",
	 EVAS_SMART_CLASS_VERSION,
	 _e_ctrl_dialog_smart_add,
	 _e_ctrl_dialog_smart_del,
	 _e_ctrl_dialog_smart_move,
	 _e_ctrl_dialog_smart_resize,
	 _e_ctrl_dialog_smart_show,
	 _e_ctrl_dialog_smart_hide,
	 _e_ctrl_dialog_smart_color_set,
	 _e_ctrl_dialog_smart_clip_set,
	 _e_ctrl_dialog_smart_clip_unset,

	 NULL /* data */
      };
      _e_smart = evas_smart_class_new(&sc);
   }
}

static void
_e_ctrl_dialog_smart_add(Evas_Object *obj)
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
   evas_object_smart_data_set(obj, sd);
}

static void
_e_ctrl_dialog_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_del(sd->clip);
   evas_object_del(sd->overlay);
   free(sd);
}

static void
_e_ctrl_dialog_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->x = x;
   sd->y = y;
   evas_object_move(sd->clip, sd->x, sd->y);
}

static void
_e_ctrl_dialog_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->clip, sd->w, sd->h);
}

static void
_e_ctrl_dialog_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_ctrl_dialog_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_ctrl_dialog_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_ctrl_dialog_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_ctrl_dialog_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}

