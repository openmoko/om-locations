#ifndef E_TAGMENU_H
#define E_TAGMENU_H

/* types */

/* object management */
Evas_Object    *e_tagmenu_add(Evas *e);
void            e_tagmenu_theme_source_set(Evas_Object *obj, const char *custom_dir);
void            e_tagmenu_source_object_set(Evas_Object *obj, Evas_Object *src_obj);
Evas_Object    *e_tagmenu_source_object_get(Evas_Object *obj);
void            e_tagmenu_autodelete_set(Evas_Object *obj, Evas_Bool autodelete);
Evas_Bool       e_tagmenu_autodelete_get(Evas_Object *obj);
void            e_tagmenu_activate(Evas_Object *obj);
void            e_tagmenu_deactivate(Evas_Object *obj);
void            e_tagmenu_theme_item_add(Evas_Object *obj, const char *icon, Evas_Coord size, const char *label, void (*func) (void *data, Evas_Object *obj, Evas_Object *src_obj), void *data);
    
#endif
