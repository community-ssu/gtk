/**
  @file ai-cp-applet.c

  Control panel applet for launching application installer.
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



#include <config.h>
#include <libosso.h>
#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>
#include <osso-log.h>

#include <glib.h>
#include <unistd.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <sys/types.h>
#include <sys/wait.h>

#define APP_BINARY "/usr/bin/osso_application_installer"

static void
ai_exited (GPid pid, gint status, gpointer data)
{
  g_spawn_close_pid (pid);
  g_main_loop_quit ((GMainLoop *) data);
}

osso_return_t execute (osso_context_t *osso, gpointer data,
                       gboolean user_activated)
{
  GMainLoop *loop;
  GPid child_pid;
  GError *error = NULL;
  gchar *argv[] = {
    APP_BINARY,
    NULL
  };

  loop = g_main_loop_new (NULL, 0);

  g_spawn_async (NULL,
		 argv, 
		 NULL,
		 G_SPAWN_DO_NOT_REAP_CHILD,
		 NULL,
		 NULL,
		 &child_pid,
		 &error);
  
  if (error == NULL)
    {
      /* We use an invisible widget here to grab all input.  This
	 makes the whole control panel insensitive so that no new
	 applets can be started until this one is done.
      */

      GtkWidget *grabber = gtk_invisible_new ();
      gtk_grab_add (grabber);
      g_child_watch_add (child_pid, ai_exited, loop);
      g_main_loop_run (loop);
      gtk_grab_remove (grabber);
      gtk_widget_destroy (grabber);
    }
  else
    {
      GtkWidget *dialog =
	hildon_note_new_information (NULL, error->message);
      gtk_widget_show_all (dialog);
      gtk_dialog_run (dialog);
      gtk_widget_destroy (dialog);
      g_error_free (error);
    }
    
  g_main_loop_unref (loop);

  return OSSO_OK;    
}
