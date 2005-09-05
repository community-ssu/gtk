/**
  @file maemo-gtk-im-switch.c

  Utility to switch the current gtk input method globally.
      
  Copyright (C) 2005 Nokia Corporation.
    
  Contact: Tomas Junnonen <tomas.junnonen@nokia.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
	     
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA
*/

#include <gtk/gtk.h>
#include <string.h>

#define IM_ATOM_NAME "gtk-global-immodule"

static void set_im_property(gchar *id)
{
  GdkAtom atom;
  GdkScreen *screen;
  GdkWindow *root;

  atom = gdk_atom_intern(IM_ATOM_NAME, FALSE);
  screen = gdk_screen_get_default();
  root = gdk_screen_get_root_window(screen);

  gdk_property_change(root,
                      atom,
                      gdk_atom_intern ("STRING", FALSE),
                      8,
                      GDK_PROP_MODE_REPLACE,
                      id,
                      strlen(id)+1);
}

int main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);

  if (argc != 2) {
    printf("Usage: %s [input-method-name]\n", argv[0]);
    return 1;
  }

  set_im_property(argv[1]);

  gtk_main_iteration();

  return 0;
}
