/* e_nav_misc.c -
 *
 * Copyright 2007-2008 OpenMoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <X11/Xlib.h>
#include <Ecore_X.h>
#include <Ecore_X_Atoms.h>

#include <Evas.h>
#include <Ecore.h>

#include <string.h>
#include <stdlib.h>

#include "e_nav_misc.h"

static void kbd_protocol_send_event(MTPRemoteOperation op);
static MTPRemoteOperation keyboard_status = MTPRemoteNone;

void
e_misc_keyboard_launch()
{
   keyboard_status = MTPRemoteShow;
   kbd_protocol_send_event(MTPRemoteShow);
}

void
e_misc_keyboard_hide()
{
   keyboard_status = MTPRemoteHide;
   kbd_protocol_send_event(MTPRemoteHide);
}

int
e_misc_keyboard_status_get()
{
   return keyboard_status;
}

static void
kbd_protocol_send_event(MTPRemoteOperation op)
{
  XEvent xev;
  memset(&xev, 0, sizeof(XEvent));

  Ecore_X_Display *disp;
  disp = ecore_x_display_get();

  xev.xclient.type = ClientMessage;
  xev.xclient.display = disp;
  xev.xclient.window = ecore_x_window_root_first_get();
  xev.xclient.message_type = ecore_x_atom_get("_MTP_IM_INVOKER_COMMAND");
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = op;

  XSendEvent(disp,
             ecore_x_window_root_first_get(),
             False,
             SubstructureRedirectMask | SubstructureNotifyMask,
             &xev);

  XSync(ecore_x_display_get(), False);
}

struct _E_Nav_Drop_Data {
     double duration;
     void (*func)(void *data, Evas_Object *obj);
     void *data;
     Evas_Bool active;
     Evas_Bool resize;

     Evas_Object *obj;
     Evas_Coord x, y;
     Evas_Coord w, h;

     Ecore_Animator *animator;
     double start;
     double end;
};

static void
_drop_stop(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Nav_Drop_Data *dd = data;

   e_nav_drop_stop(dd, FALSE);
}

static int
_drop_anim(void *data)
{
   E_Nav_Drop_Data *dd = data;
   double t, dur;
   Evas_Coord oy, y;

   t = ecore_time_get();
   if (t >= dd->end)
     goto end;

   dur = dd->duration;

   t -= dd->start;
   t = dur - ((t - dur) * (t - dur) / dur);

   oy = -dd->h;
   y = oy + (dd->y - oy) * (t / dur);

   evas_object_move(dd->obj, dd->x, y);
   if (dd->resize)
     {
	evas_object_resize(dd->obj, dd->w, dd->h);
	dd->resize = 0;
     }

   return 1;

end:
   e_nav_drop_stop(dd, TRUE);

   return 0;
}

E_Nav_Drop_Data *
e_nav_drop_new(double duration, void (*func)(void *data, Evas_Object *obj), void *data)
{
   E_Nav_Drop_Data *dd;

   dd = calloc(1, sizeof(*dd));
   if (!dd)
     return NULL;

   if (duration < 0.01)
     duration = 0.01;

   dd->duration = duration;
   dd->func = func;
   dd->data = data;

   return dd;
}

void
e_nav_drop_apply(E_Nav_Drop_Data *dd, Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   if (dd->active)
     {
	if (dd->obj == obj)
	  {
	     dd->x = x;
	     dd->y = y;

	     if (dd->w != w || dd->h !=h)
	       {
		  dd->w = w;
		  dd->h = h;

		  dd->resize = 1;
	       }

	     return;
	  }
	else
	  {
	     e_nav_drop_stop(dd, FALSE);
	  }
     }

   dd->obj = obj;
   dd->x = x;
   dd->y = y;
   dd->w = w;
   dd->h = h;

   dd->start = ecore_time_get();
   dd->end = dd->start + dd->duration;

   dd->active = 1;
   dd->resize = 1;

   evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL, _drop_stop, dd);
   dd->animator = ecore_animator_add(_drop_anim, dd);
}

void
e_nav_drop_stop(E_Nav_Drop_Data *dd, Evas_Bool do_cb)
{
   if (!dd->active)
     return;

   evas_object_move(dd->obj, dd->x, dd->y);
   evas_object_resize(dd->obj, dd->w, dd->h);
   evas_object_event_callback_del(dd->obj, EVAS_CALLBACK_DEL, _drop_stop);

   ecore_animator_del(dd->animator);

   dd->active = 0;

   if (do_cb && dd->func)
     dd->func(dd->data, dd->obj);
}

Evas_Bool
e_nav_drop_active_get(E_Nav_Drop_Data *dd)
{
   return dd->active;
}

void
e_nav_drop_destroy(E_Nav_Drop_Data *dd)
{
   if (dd->active)
     e_nav_drop_stop(dd, FALSE);

   free(dd);
}
