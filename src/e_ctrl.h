#ifndef E_CTRL_H
#define E_CTRL_H
#include <Ecore.h>
#include <Evas.h>
#include <Edje.h>

typedef enum _E_Nav_Show_Mode {
   E_NAV_SHOW_MODE_TAGLESS,
   E_NAV_SHOW_MODE_TAG,
} E_Nav_Show_Mode;

typedef enum _E_Nav_View_Mode {
   E_NAV_VIEW_MODE_MAP,
   E_NAV_VIEW_MODE_SAT,
   E_NAV_VIEW_MODE_LIST,
} E_Nav_View_Mode;

Evas_Object * e_ctrl_add(Evas *e);
void e_ctrl_theme_source_set(Evas_Object *obj, const char *custom_dir);
void e_ctrl_nav_set(Evas_Object* obj);
void e_ctrl_zoom_drag_value_set(double y); 
void e_ctrl_zoom_text_value_set(const char* buf);
void e_ctrl_longitude_set(const char* buf);
void e_ctrl_latitude_set(const char* buf);
int                    e_ctrl_edje_object_set(Evas_Object *o, const char *category, const char *group);

#endif
