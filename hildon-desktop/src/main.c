/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <locale.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>

#include <sys/types.h> // Required for file io (creat etc.)
#include <sys/stat.h> // required for file io
#include <fcntl.h> // required for file io

#include "hd-desktop.h"
#include "hd-crash-manager.h"

#define OSSO_USER_DIR        ".osso"
#define HILDON_DESKTOP_GTKRC "current-gtk-theme.maemo_af_desktop"

#define STAMP_DIR                     "/tmp/osso-appl-states/"
#define HILDON_DESKTOP_STAMP_DIR      "/tmp/osso-appl-states/hildon-desktop/"
#define HILDON_DESKTOP_STAMP_FILE     HILDON_DESKTOP_STAMP_DIR"/stamp"

int main (int argc, char **argv)
{
  GObject *desktop;
  gchar *gtkrc = NULL;
  gboolean safe_mode = FALSE;

  g_thread_init (NULL);
  setlocale (LC_ALL, "");

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);

  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  textdomain (GETTEXT_PACKAGE);

  /* Check for safe mode */
  /* Added by karoliina 31.7.2007 */
  if (g_file_test (HILDON_DESKTOP_STAMP_FILE, G_FILE_TEST_EXISTS))
  {
      g_warning ("hildon-desktop did not exit properly on the previous "
                  "session. All home plugins will be disabled.");
      safe_mode = TRUE;
  }
  else
  {
      int fd;
      mkdir (STAMP_DIR, 0755);
      mkdir (HILDON_DESKTOP_STAMP_DIR, 0755);
      fd = creat(HILDON_DESKTOP_STAMP_FILE, S_IRUSR|S_IWUSR);
      if (fd >= 0)
          close (fd);
  }



  /* Read the maemo-af-desktop gtkrc file */
  gtkrc = g_build_filename (g_get_home_dir (), 
                            OSSO_USER_DIR,
                            HILDON_DESKTOP_GTKRC,
                            NULL);
  
  if (gtkrc && g_file_test ((gtkrc), G_FILE_TEST_EXISTS))
  {
    gtk_rc_add_default_file (gtkrc);
  }

  g_free (gtkrc);

  /* If gtk_init was called already (maemo-launcher is used),
   * re-parse the gtkrc to include the maemo-af-desktop specific one */
  if (gdk_screen_get_default ())
  {
    gtk_rc_reparse_all_for_settings (
            gtk_settings_get_for_screen (gdk_screen_get_default ()),
                                         TRUE);
  }

  gtk_init (&argc, &argv);

  gnome_vfs_init ();
  
  desktop = hd_desktop_new ();

  hd_desktop_run (HD_DESKTOP (desktop));

  gtk_main ();

  return 0;
}
