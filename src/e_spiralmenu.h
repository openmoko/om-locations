#ifndef E_SPIRALMENU_H
#define E_SPIRALMENU_H

/* types */

/* object management */
Evas_Object    *e_spiralmenu_add(Evas *e);
void            e_spiralmenu_theme_source_set(Evas_Object *obj, const char *custom_dir);
void            e_spiralmenu_source_object_set(Evas_Object *obj, Evas_Object *src_obj);
Evas_Object    *e_spiralmenu_source_object_get(Evas_Object *obj);
void            e_spiralmenu_autodelete_set(Evas_Object *obj, Evas_Bool autodelete);
Evas_Bool       e_spiralmenu_autodelete_get(Evas_Object *obj);
void            e_spiralmenu_deacdelete_set(Evas_Object *obj, Evas_Bool deacdelete);
Evas_Bool       e_spiralmenu_deacdelete_get(Evas_Object *obj);
void            e_spiralmenu_activate(Evas_Object *obj);
void            e_spiralmenu_deactivate(Evas_Object *obj);
void            e_spiralmenu_theme_item_add(Evas_Object *obj, const char *icon, Evas_Coord size, const char *label, void (*func) (void *data, Evas_Object *obj, Evas_Object *src_obj), void *data);
    
#endif
