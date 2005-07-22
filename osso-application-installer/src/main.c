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



#include "main.h"

int main (int argc, char** argv)
{
    AppData *app_data;
    AppUIData *app_ui_data;
    gboolean direct_install = FALSE;
    gint result;
    
    /* Start logging */
    ULOG_OPEN(PACKAGE_NAME " " PACKAGE_VERSION);
    ULOG_INFO("Start logging..");
 
    /* Localization */
    ULOG_INFO("Setting locale..");
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, APPLICATIONINSTALLERLOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    /* Set new PATH variable */
    ULOG_INFO("Setting path..");
    GString *tmp = g_string_new(g_getenv(PATH_ENV));
    tmp = g_string_append(tmp, PATH_ADD);
    g_setenv(PATH_ENV, tmp->str, TRUE);
    ULOG_INFO("main: PATH=%s", g_getenv(PATH_ENV));

    /* Initialize GTK */
    ULOG_INFO("Initializing GTK..");
    gtk_init(&argc, &argv);

    /*
    Allocate memory for our application data.
    There's no need to check the result, because application will terminate
    if allocating fails (see GLib API documentation).
    */
    ULOG_INFO("Allocating memory for application data..");
    app_data = g_new0(AppData, 1);
    app_data->app_ui_data = g_new0(AppUIData, 1);
    app_data->app_osso_data = g_new0(AppOSSOData, 1);
    
    app_ui_data = app_data->app_ui_data;
    app_ui_data->param = NULL;

    /* Read params */
    if ( (argc > 1) && (argv[1] != NULL) ) {
      fprintf (stderr,
	       "XXX - Direct install not supported right now, sorry.\n");
      exit (1);
    }

    /* Init OSSO */
    ULOG_INFO("Initializing OSSO..");
    init_osso(app_data);

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

    /* Create applet UI */
    ULOG_INFO("Creating applet UI..");
    ui_create_main_dialog(app_data);

    /* Start GTK main loop */
    ULOG_INFO("Starting GTK main loop..");
    if (!direct_install)
      gtk_main();
    else
      fprintf(stderr, "Skipping main loop\n");

#ifdef USE_GNOME_VFS
    /* GnomeVFS cleanup */
    if (gnome_vfs_initialized()) {
                gnome_vfs_shutdown();
    }
#endif

    /* Deinit OSSO */
    ULOG_INFO("Deinitializing OSSO..");
    deinit_osso(app_data);
    
    /* Stop logging */
    ULOG_INFO("Stop logging..");
    LOG_CLOSE();
    
    return 0;
}
