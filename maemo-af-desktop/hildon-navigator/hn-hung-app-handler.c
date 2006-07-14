/* 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int 
main(int argc, char* argv[])
{
  Display *xdpy;
  XEvent   xev;
  Atom     atom_hildon_frozen_app;
  Window   xwin_hung;

  if (argc < 3)
    exit(1);

  xdpy = XOpenDisplay(getenv("DISPLAY"));

  if (xdpy == NULL)
    exit(1);

  xwin_hung = (Window)strtol(argv[2], NULL, 10);

  atom_hildon_frozen_app = XInternAtom(xdpy, "_HILDON_FROZEN_WINDOW", False);

  memset(&xev, '\0', sizeof xev);
  xev.xclient.type         = ClientMessage;
  xev.xclient.window       = DefaultRootWindow(xdpy);
  xev.xclient.message_type = atom_hildon_frozen_app;
  xev.xclient.format        = 8;
  xev.xclient.data.l[0]     = xwin_hung; /* Window ID */
  xev.xclient.data.l[1]     = 0; /* detail - hung / unhung */
  
  if (argc > 3 && !strcmp(argv[3], "1"))
    xev.xclient.data.l[1] = 1;

  /* Send with prop mask as to lessen wake up of TN on other
   * root msg's.
  */
  XSendEvent(xdpy, DefaultRootWindow(xdpy) ,False,
	     PropertyChangeMask,
	     /* SubstructureRedirectMask|SubstructureNotifyMask, */ 
	     &xev);

  XSync(xdpy, False);

  XCloseDisplay(xdpy);

  return 0;
}
