/*
 * This file is part of hildon-base-lib
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

/*
 * @file hildon-base-switch-to-desktop.c
 * 
 * This file contains the implementation of function used to switch to desktop.
 * 
 * DO NOT include directly, it is simply used by the other components.
 * 
 */

#include "hildon-base-switch-to-desktop.h"

#include <gdk/gdkx.h>
#include <string.h>

/* hildon, from mb's struct.h */
#define MB_CMD_DESKTOP 3




void
hildon_base_switch_to_desktop ()
{
    XEvent ev;
    Atom typeatom;

    typeatom = XInternAtom (GDK_DISPLAY(), "_MB_COMMAND", False);

    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = GDK_ROOT_WINDOW();
    ev.xclient.message_type = typeatom;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = MB_CMD_DESKTOP;
    XSendEvent (GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
                SubstructureRedirectMask, &ev);                
}
