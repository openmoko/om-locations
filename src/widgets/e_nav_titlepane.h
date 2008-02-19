#ifndef E_NAV_TITLEPANE_H
#define E_NAV_TITLEPANE_H
#include <Ecore.h>
#include <Evas.h>
#include <Edje.h>
#include <Ecore_Evas.h>

typedef void (* CallbackFunc) (void *data, Evas *evas, Evas_Object *obj, void *event);

Evas_Object * e_nav_titlepane_add(Evas *e);
void e_nav_titlepane_theme_source_set(Evas_Object *obj, const char *custom_dir);
//int e_nav_titlepane_edje_object_set(Evas_Object *o, const char *category, const char *group);
void e_nav_titlepane_set_message(Evas_Object *obj, const char *message);
void e_nav_titlepane_set_left_button(Evas_Object *obj, const char *text, CallbackFunc cb, Evas_Object *src);  
void e_nav_titlepane_set_right_button(Evas_Object *obj, const char *text, CallbackFunc cb, Evas_Object *src);
void e_nav_titlepane_hide_buttons(Evas_Object *obj);

#endif
