/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
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

#include <gtk/gtkmain.h>
#include <hildon/hildon-window.h>
#include "hildon-plugin-settings-dialog.h"

static void 
_exit (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}	

int 
main (int argc, char **argv)
{
  GtkWidget *dialog, *window;
	
  gtk_init (&argc, &argv);

  window = hildon_window_new ();
  
  dialog = GTK_WIDGET (g_object_new (HILDON_PLUGIN_TYPE_SETTINGS_DIALOG,
			  	     "window-type", HILDON_PLUGIN_SETTINGS_DIALOG_TYPE_WINDOW,
				     "hide-home", FALSE,
				     NULL));

  gtk_widget_realize (dialog);

  gtk_widget_reparent (GTK_BIN (dialog)->child, window);

  gtk_button_set_label (GTK_BUTTON (HILDON_PLUGIN_SETTINGS_DIALOG (dialog)->button_ok), "Apply");
  gtk_button_set_label (GTK_BUTTON (HILDON_PLUGIN_SETTINGS_DIALOG (dialog)->button_cancel), "Exit");
 
  g_signal_connect_after (HILDON_PLUGIN_SETTINGS_DIALOG (dialog)->button_cancel,
		          "clicked",
			  G_CALLBACK (_exit),
			  NULL);

  gtk_widget_show (window);

  gtk_main ();	

  return 0;
}	

