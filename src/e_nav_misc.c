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

#include "e_nav_misc.h"
#include <string.h>

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

