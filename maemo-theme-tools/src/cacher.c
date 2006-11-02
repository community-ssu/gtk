/* 
 * GPL license, Copyright (c) 2006 by Nokia Corporation                       
 *                                                                            
 * Authors:                                                                   
 *      Tommi Komulainen <tommi.komulainen@nokia.com>
 *                                                                            
 * This program is free software; you can redistribute it and/or modify it    
 * under the terms of the GNU General Public License as published by the      
 * Free Software Foundation, version 2.                                                                   
 *                                                                            
 * This program is distributed in the hope that it will be useful, but        
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   
 * for more details.                                                          
 *                                                                            
 * You should have received a copy of the GNU General Public License along    
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.                      
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
