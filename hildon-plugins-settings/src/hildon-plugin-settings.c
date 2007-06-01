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

#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>
#include <gtk/gtk.h>

#include "hildon-plugin-config-parser.h"

#include <libintl.h>
#define _(a) dgettext(PACKAGE, a)

osso_return_t 
execute (osso_context_t *osso,
	 gpointer user_data,
	 gboolean user_activated)
{
  GError *error = NULL;
  gint ret;

  GtkCellRenderer   *renderer_text = gtk_cell_renderer_text_new();
  GtkTreeViewColumn *text_column;

  GtkWidget *dialog = gtk_dialog_new ();
  HildonPluginConfigParser *cp = 
    HILDON_PLUGIN_CONFIG_PARSER 
      (hildon_plugin_config_parser_new ("/usr/share/applications/hildon-navigator","/tmp/hello.tmp"));

  hildon_plugin_config_parser_set_keys (cp,
		  			"Name", G_TYPE_STRING,
					"Icon", GDK_TYPE_PIXBUF,
					"Mandatory", G_TYPE_BOOLEAN,
					NULL);

  hildon_plugin_config_parser_load (cp,&error);

  if (!error)
  {	  
    GtkWidget *tw = gtk_tree_view_new_with_model (cp->tm);

    text_column = 
      gtk_tree_view_column_new_with_attributes
        (NULL, renderer_text, "text", 2, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW (tw), text_column);

    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
		       tw);
    gtk_widget_show (tw);
  }

  gtk_widget_show (dialog);

  ret = gtk_dialog_run (GTK_DIALOG (dialog));

  if (ret == GTK_RESPONSE_OK)
    return OSSO_OK;
    
  return OSSO_ERROR;  
}

