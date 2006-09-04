/*
  @file xset770.c

  This file contains the implementation of the 'xset770'.

  @author Tapani P&auml;lli <tapani.palli@nokia.com>

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

Display *dpy=NULL;


/*
 * set_keyrepeat
 *
 */
static void
set_keyrepeat (int key, int val)
{
  XKeyboardControl values;
  values.key = key;
  values.auto_repeat_mode = val;

  XChangeKeyboardControl(dpy,
                         KBKey | KBAutoRepeatMode,
			 &values);
}


static void usage(void)
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
  int i;
  int key = -1, val = 0; /* for keyrepeat */

  dpy = XOpenDisplay(NULL);

  if (!dpy)
  {
    dpy = XOpenDisplay(":0");
  }
  if (!dpy)
  {
    return 0;
  }

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
	set_keyrepeat (key, val);
      break;

    default :
	usage();
    }
  }



  XCloseDisplay (dpy);
  return 0;
}

