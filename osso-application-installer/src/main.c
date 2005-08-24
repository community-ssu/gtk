/**
    @file main.c

    Main file for Application Installer
    <p>
*/

/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <stdarg.h>
#include "main.h"

char *
gettext_try_many (const char *logical_id, ...)
{
  const char *string;
  char *result;
  va_list ap;

  va_start (ap, logical_id);
  for (string = logical_id; string; string = va_arg (ap, const char *))
    {
      result = gettext (string);
      if (result != string)
	break;
    }
  va_end (ap);
  return result;
}

int main (int argc, char** argv)
{
  AppData *app_data;
  AppUIData *app_ui_data;
  gint result;
    
  /* Start logging */
  ULOG_OPEN(PACKAGE_NAME " " PACKAGE_VERSION);
  ULOG_INFO("Start logging..");
 
  /* Localization */
  setlocale(LC_ALL, "");
  bindtextdomain(GETTEXT_PACKAGE, APPLICATIONINSTALLERLOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  /* Set new PATH variable */
  GString *tmp = g_string_new(g_getenv(PATH_ENV));
  tmp = g_string_append(tmp, PATH_ADD);
  g_setenv(PATH_ENV, tmp->str, TRUE);

  /* Initialize GTK */
  gtk_init(&argc, &argv);

  /*
    Allocate memory for our application data.
    There's no need to check the result, because application will terminate
    if allocating fails (see GLib API documentation).
  */
  app_data = g_new0(AppData, 1);
  app_data->app_ui_data = g_new0(AppUIData, 1);
  app_data->app_osso_data = g_new0(AppOSSOData, 1);
  
  app_ui_data = app_data->app_ui_data;

#ifdef USE_GNOME_VFS
  /* GnomeVFS */
  if (!gnome_vfs_initialized()) {
    result= gnome_vfs_init();
    if (!result) {
      ULOG_CRIT("Could not init GnomeVFS");
      exit(-92);
    }
  }
#endif

  ui_create_main_dialog (app_data);

  /* Read params and maybe install directly 
   */
  if (argc > 1)
    do_install (argv[1], app_data);

  /* Init OSSO.  It has to be done after creating the main dialog
     since the osso callbacks are run from the main loop and might
     therefore be called too early if they are registered before the
     main dialog has been setup completely.
  */
  init_osso(app_data);

  gtk_main();

#ifdef USE_GNOME_VFS
  /* GnomeVFS cleanup */
  if (gnome_vfs_initialized()) {
    gnome_vfs_shutdown();
  }
#endif

  deinit_osso (app_data);
  
  /* Stop logging */
  ULOG_INFO("Stop logging..");
  LOG_CLOSE();
  
  return 0;
}
