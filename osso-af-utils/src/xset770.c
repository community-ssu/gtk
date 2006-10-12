/*
  @file xset770.c

  This file contains the implementation of the 'xset770'.

  @author Tapani P&auml;lli <tapani.palli@nokia.com>

  Copyright (C) 2005-2006 Nokia Corporation. All rights reserved.
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
   
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA

  @libraries used
 
  - Xlib
  - xsp extension

  ---------------------------------------------------------
  770 keycodes ...

  MENU  70
  HOME  71
  FSCR  72
   -    73
   +    74
  PWR  124

  example : turn keyrepeat on homekey off : "xset770 -r 70 0"
  ---------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/Xsp.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XInput.h>

Display *dpy=NULL;

static int
get_feedback_id (XDevice *device)
{
  int i = 0;
  
  for (i = 0; i < device->num_classes; i++)
  {
    if (device->classes[i].input_class == KbdFeedbackClass)
      return i;
  }
  return -1;
}

/*
 * set_keyrepeat
 */
static void
set_keyrepeat (int id, int key, int val)
{
  XKbdFeedbackControl *evalues;
  XKeyboardControl cvalues;
  XDevice *device;
  int feedback_id;

  if (id > 0) {
    device = XOpenDevice(dpy, id);
    evalues = (XKbdFeedbackControl *)calloc(sizeof(XKbdFeedbackControl), 1);
    if (!device)
      return;

    feedback_id = get_feedback_id (device);
    if (feedback_id < 0)
      return;

    evalues->id = feedback_id;
    evalues->key = key;
    evalues->auto_repeat_mode = val;
    evalues->class = KbdFeedbackClass;

    XChangeFeedbackControl (dpy, device,
                            DvKey | DvAutoRepeatMode,
                            (XFeedbackControl *) evalues);
    free(evalues);
  }
  else {
    cvalues.key = key;
    cvalues.auto_repeat_mode = val;
    XChangeKeyboardControl (dpy, KBKey | KBAutoRepeatMode,
                            &cvalues);
  }
}


void usage()
{
  fprintf(stderr, "usage : -r <key> <value>\n");
  exit (1);
}

/*
 * main
 *
 */
int main (int argc, char*argv[])
{
  int i, n_devices = 0;
  int key = -1, val = 0; /* for keyrepeat */
  XDeviceInfo *devices = NULL;

  dpy = XOpenDisplay(NULL);

  if (!dpy)
  {
    dpy = XOpenDisplay(":0");
  }
  if (!dpy)
  {
    return 0;
  }

  devices = XListInputDevices(dpy, &n_devices);

  if (argc < 2)
  {
    /* no arguments */
    usage();
  }
  i=0;
  while (++i < argc)
  {
    if (argv[i][0] != '-')
    {
      /* not an option */
      break;
    }
    if (argv[i][2])
    {
      /* not a single letter option */
      usage();
    }

    switch(argv[i][1])
    {
    case 'r':
      if (i+2 >= argc)
      {
	usage();
      }

      key = atoi(argv[++i]);
      val = atoi(argv[++i]);

      /* check validity */
      if (key <= 0)
      {
	usage();
      }
      else
      {
        for (i = 0; i < n_devices; i++)
        {
          if (devices[i].use == IsXExtensionKeyboard)
            set_keyrepeat (-1, key, val);
        }
      }

      break;

    default :
	usage();
    }
  }

  if (devices)
    XFreeDeviceList(devices);
  XCloseDisplay (dpy);

  return 0;
}

