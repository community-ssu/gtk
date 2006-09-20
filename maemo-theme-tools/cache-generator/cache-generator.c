/*
 * This file is a dummy gtk application that we use to generate
 * gtk theme rc file caches. 
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Tommi Komulainen <tommi.komulainen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

#include <gtk/gtk.h>

int
main (int argc, char **argv)
{
  GtkSettings *settings;
  int          i;
  
  g_unsetenv ("DISPLAY");
  gtk_init_check (&argc, &argv);
  
  if (argc < 2)
    {
	g_printerr ("Usage: %s THEME-DIR...\n", g_get_prgname ());
	return 1;
    }
   
  g_setenv ("GSCANNERCACHE_CREATE", "1", TRUE);
   
  for (i = 1; i < argc; i++)
    {
	const char *theme_path = argv[i];
	char *gtkrc;
        char *gtkrc_maemo;
          
	gtkrc = g_build_filename (theme_path, "gtk-2.0", "gtkrc", NULL);
	g_print ("%s\n", gtkrc);
	gtk_rc_parse (gtkrc);
        g_free (gtkrc);

	gtkrc_maemo = g_build_filename (theme_path, "gtk-2.0", "gtkrc.maemo_af_desktop", NULL);
	g_print ("%s\n", gtkrc_maemo);
	gtk_rc_parse (gtkrc_maemo);
        g_free (gtkrc_maemo);
    }
   
   settings = g_object_new (GTK_TYPE_SETTINGS, NULL);
   gtk_rc_reparse_all_for_settings (settings, TRUE);
   
   return 0;
}
