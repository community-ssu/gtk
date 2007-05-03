/*
 * Copyright © 2004 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Red Hat not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Red Hat makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL RED HAT
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Matthias Clasen, Red Hat, Inc.
 */

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#include "clipboard-manager.h"

void
terminate_cb (void *data)
{
  int *terminated = data;
  
  if (!*terminated)
    {
      *terminated = True;
    }
}

static int error_code;

static void
error_trap_push (void)
{
  error_code = 0;

}

static int
error_trap_pop (void)
{
  return error_code;
}

static int
x_error (Display *display,
	 XErrorEvent *error)
{
  error_code = error->error_code;
  return 0; /* ignored */
}

int 
main (int argc, char *argv[])
{
  ClipboardManager *manager;
  int terminated = False;
  Display *display;

  display = XOpenDisplay (NULL);

  if (!display)
    {
      fprintf (stderr, "Could not open display. Is the DISPLAY environment variable set?\n");
      exit (1);
    }

  if (clipboard_manager_check_running (display))
    {
      fprintf (stderr, "You can only run one clipboard manager at a time; exiting\n");
      exit (1);
    }

  XSetErrorHandler (x_error);
  manager = clipboard_manager_new (display,
				   error_trap_push, error_trap_pop,
				   terminate_cb, &terminated);
  if (!manager)
    {
      fprintf (stderr, "Could not create clipboard manager!\n");
      exit (1);
    }

  while (!terminated)
    {
      XEvent event;

      XNextEvent (display, &event);

      clipboard_manager_process_event (manager, &event);
    }
  
  clipboard_manager_destroy (manager);

  return 0;

}
