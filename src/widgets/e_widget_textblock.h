/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
//#ifdef E_TYPEDEFS
//#else
#ifndef E_WIDGET_TEXTBLOCK_H
#define E_WIDGET_TEXTBLOCK_H

EAPI Evas_Object *e_widget_textblock_add(Evas *evas);
EAPI void e_widget_textblock_markup_set(Evas_Object *obj, const char *text);
EAPI void e_widget_textblock_plain_set(Evas_Object *obj, const char *text);
    
EAPI void e_widget_textblock_move(Evas_Object *obj, int x, int y);
EAPI void e_widget_textblock_resize(Evas_Object *obj, int w, int h);
EAPI void e_widget_textblock_event_callback_add(Evas_Object *obj, Evas_Callback_Type type, void(*func)(void *data, Evas *e, Evas_Object *obj, 
                                                   void *event_info), const void *data);
EAPI void e_widget_textblock_show(Evas_Object *obj);
#endif
//#endif
