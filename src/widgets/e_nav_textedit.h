#ifndef E_NAV_TEXTEDIT_H
#define E_NAV_TEXTEDIT_H

/* object management */
Evas_Object    *e_textedit_add(Evas *e);
//void            e_textedit_theme_source_set(Evas_Object *obj, const char *custom_dir);
void            e_textedit_theme_source_set(Evas_Object *obj, const char *custom_dir, void (*positive_func)(void *data, Evas *evas, Evas_Object *obj, void *event), void (*negative_func)(void *data, Evas *evas, Evas_Object *obj, void *event));
void            e_textedit_source_object_set(Evas_Object *obj, void *src_obj);
Evas_Object    *e_textedit_source_object_get(Evas_Object *obj);
//void            e_textedit_autodelete_set(Evas_Object *obj, Evas_Bool autodelete);
//Evas_Bool       e_textedit_autodelete_get(Evas_Object *obj);
void            e_textedit_activate(Evas_Object *obj);
void            e_textedit_deactivate(Evas_Object *obj);
//void            e_textedit_button_add(Evas_Object *obj, const char *label, void (*func) (void *data, Evas_Object *obj, Evas_Object *src_obj), void *data);
void            e_textedit_input_set(Evas_Object *obj, const char *name, const char *input);
//void            e_textedit_text_set(Evas_Object *obj, const char *text);
//void            e_textedit_textblock_add(Evas_Object *obj, const char *label, const char*input, void *data);
//void            e_textedit_component_add(Evas_Object *obj, Evas_Object *comp);
    
#endif

