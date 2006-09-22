/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
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

#include <stdio.h>
#include <glib.h>
#include <stdlib.h>

#define HN_USER_CONFIG   ".osso/hildon-navigator/plugins.conf"
#define HN_DESKTOP_DIR   "/usr/share/applications/hildon-navigator"
#define DEFAULT_PLUGIN_0 "hildon-task-navigator-bookmarks.desktop"
#define DEFAULT_PLUGIN_1 "osso-contact-plugin.desktop"
#define HN_MAX_PLUGINS 2
#define GROUP_DESKTOP    "Desktop Entry"
#define KEY_LIBRARY      "X-task-navigator-plugin"

static void 
insert_desktop_file (GKeyFile *keyfile, gchar *desktop_file, gint position)
{
  GError *error=NULL;
  GKeyFile *keyfile_desktop;
  gchar *path_desktop_file,
  	*library;

  path_desktop_file = 
    g_strdup_printf ("%s/%s",HN_DESKTOP_DIR,desktop_file);
  
  keyfile_desktop = g_key_file_new();

  g_key_file_load_from_file 
    (keyfile_desktop, path_desktop_file, G_KEY_FILE_NONE, &error);

  if (error != NULL)
  {
    g_error_free (error);
    g_free (path_desktop_file);
    return;
  }

  library = g_key_file_get_string (keyfile_desktop,
		  		   GROUP_DESKTOP,
				   KEY_LIBRARY,
				   NULL);

  if (!library)
    goto out;

  g_key_file_set_string  (keyfile,
		  	  desktop_file,
			  "Library",
			  library);

  g_key_file_set_string  (keyfile,
		  	  desktop_file,
			  "Desktop-file",
			  desktop_file);

  g_key_file_set_integer (keyfile,
		  	  desktop_file,
			  "Position",
			  position);

  g_debug("%s %s %d",library,desktop_file,position);
  
 out:
  g_free (library);
  g_free (path_desktop_file);
}

static int 
regenerate_configuration (void)
{
  gchar **groups;
  GKeyFile *keyfile = NULL;
  gchar *home, *path_config, *data;
  GError *error = NULL;
  gboolean plugins_used[HN_MAX_PLUGINS],
  	   positions_free[HN_MAX_PLUGINS];
  FILE *file;
  gint i;

  for (i=0;i<HN_MAX_PLUGINS;i++)
    positions_free[i] = plugins_used[i] = FALSE;

  home = (gchar *) getenv ("HOME");

  path_config = g_strdup_printf ("%s/%s",home,HN_USER_CONFIG);

  keyfile = g_key_file_new();

  g_key_file_load_from_file (keyfile, path_config, G_KEY_FILE_NONE, &error);

  if (error != NULL)
    goto out;

  groups = g_key_file_get_groups(keyfile, NULL);
  i = 0;
  
  while (groups[i] != NULL)
  {
    gchar *path_desktop;

    path_desktop = g_strdup_printf ("%s/%s",HN_DESKTOP_DIR,groups[i]);

    g_debug (path_desktop);
    
    if (!g_file_test (path_desktop, G_FILE_TEST_EXISTS))
    {
      g_key_file_remove_group (keyfile,groups[i],&error);

      if (error != NULL)
      {
	g_free (path_desktop);
        g_error_free (error);
	error = NULL;
	i++;
	continue;
      }
      positions_free[i] = TRUE;
    }
    else
    {
      if (g_str_equal (groups[i],DEFAULT_PLUGIN_0))
        plugins_used[0] = TRUE;

      if (g_str_equal (groups[i],DEFAULT_PLUGIN_1))
        plugins_used[1] = TRUE;  
    }
    
    g_free (path_desktop);
    i++;    
  }

  for (i=0;i<HN_MAX_PLUGINS;i++)
  {
    if (positions_free[i])
    {
      if (!plugins_used[0])  /* Oh my god, WTF is this sh*t?? */
      {
        insert_desktop_file (keyfile,DEFAULT_PLUGIN_0,i);
	plugins_used[0] = TRUE;
	continue;
      }
      
      if (!plugins_used[1])
      {	  
        insert_desktop_file (keyfile,DEFAULT_PLUGIN_1,i);
	plugins_used[1] = TRUE;
	continue;
      }
    }
  }

  data = g_key_file_to_data(keyfile, NULL, NULL);

  if (!data) goto out;

  file = fopen (path_config, "w");
  if (file)
    fprintf(file,"%s",data);
  fclose (file);

 out:
  
  if (keyfile)
    g_key_file_free (keyfile);
  g_free (path_config);

  return 0;
}

int
main (int argc, char **argv)
{
  return 
   regenerate_configuration();
}

