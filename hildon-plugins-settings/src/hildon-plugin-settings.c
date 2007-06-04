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

#include "hildon-plugin-settings-dialog.h"

#include <libintl.h>
#define _(a) dgettext(PACKAGE, a)

static gboolean
filter_is_visible (GtkTreeModel *model,
                   GtkTreeIter *iter,
                   gpointer data)
{
  gchar *name;

  gtk_tree_model_get (model,
                      iter,
                      0,
                      &name,
                      -1);

  if (g_str_equal (name,"/usr/share/applications/hildon-navigator/hildon-task-navigator-bookmarks.desktop"))
    return FALSE;	  

  return TRUE;
}

osso_return_t 
execute (osso_context_t *osso,
	 gpointer user_data,
	 gboolean user_activated)
{
  gint ret;

  GtkWidget *dialog = hildon_plugin_settings_dialog_new ();

  hildon_plugin_settings_dialog_set_visibility_filter
    (HILDON_PLUGIN_SETTINGS_DIALOG (dialog),
     "Tasknavigator",
    filter_is_visible,
    NULL,
    NULL);    

  gtk_widget_show (dialog);

  ret = gtk_dialog_run (GTK_DIALOG (dialog));

  if (ret == GTK_RESPONSE_OK)
  {
  }
    
  gtk_widget_destroy (dialog);
    
  return OSSO_OK;  
}

