#ifndef E_NAV_DIALOG_H
#define E_NAV_DIALOG_H

/* object management */
Evas_Object    *e_dialog_add(Evas *e);
void            e_dialog_theme_source_set(Evas_Object *obj, const char *custom_dir);
void            e_dialog_source_object_set(Evas_Object *obj, Evas_Object *src_obj);
Evas_Object    *e_dialog_source_object_get(Evas_Object *obj);
void            e_dialog_autodelete_set(Evas_Object *obj, Evas_Bool autodelete);
Evas_Bool       e_dialog_autodelete_get(Evas_Object *obj);
void            e_dialog_activate(Evas_Object *obj);
void            e_dialog_deactivate(Evas_Object *obj);
void            e_dialog_button_add(Evas_Object *obj, const char *label, void (*func) (void *data, Evas_Object *obj, Evas_Object *src_obj), void *data);
void            e_dialog_title_set(Evas_Object *obj, const char *title, const char *message);
void            e_dialog_textblock_add(Evas_Object *obj, const char *label, const char*input, Evas_Coord size, void *data);
void            e_dialog_textblock_text_set(void *tb_obj, const char *input);
    
#endif
