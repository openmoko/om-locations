#ifndef E_CTRL_DIALOG_H
#define E_CTRL_DIALOG_H
#include <Ecore.h>
#include <Evas.h>
#include <Edje.h>
#include <Ecore_Evas.h>

typedef void (* CallbackFunc) (void *data, Evas *evas, Evas_Object *obj, void *event);

Evas_Object * e_ctrl_dialog_add(Evas *e);
void e_ctrl_dialog_theme_source_set(Evas_Object *obj, const char *custom_dir);
int e_ctrl_dialog_edje_object_set(Evas_Object *o, const char *category, const char *group);
void e_ctrl_dialog_set_message(Evas_Object *obj, const char *message);
void e_ctrl_dialog_set_left_button(Evas_Object *obj, const char *text, CallbackFunc cb, Evas_Object *src);  
void e_ctrl_dialog_set_right_button(Evas_Object *obj, const char *text, CallbackFunc cb, Evas_Object *src);
void e_ctrl_dialog_hide_buttons(Evas_Object *obj);

#endif
