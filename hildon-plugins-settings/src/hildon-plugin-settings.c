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

#define TN_MAX_ITEMS 5

/* HARDCODE_PARTY */
/* Plugins to not be shown */
#define HP_APPLICATION_SWITCHER "/usr/share/applications/hildon-navigator/app-switcher.desktop"
#define HP_SWITCHER_MENU "/usr/share/applications/hildon-navigator/switcher-menu.desktop"

GtkTreeRowReference *selected[TN_MAX_ITEMS];

static gboolean
_tn_visibility_filter (GtkTreeModel *model,
                   GtkTreeIter *iter,
                   gpointer data)
{
  gchar *name;

  gtk_tree_model_get (model,
                      iter,
                      0,
                      &name,
                      -1);

  if (g_str_equal (name,HP_APPLICATION_SWITCHER) ||
      g_str_equal (name,HP_SWITCHER_MENU))
    return FALSE;	  

  return TRUE;
}

static void 
_sb_cell_mandatory_data_func (GtkTreeViewColumn *tc,
                    	      GtkCellRenderer *cell,
                    	      GtkTreeModel *tm,
                    	      GtkTreeIter *iter,
                    	      gpointer data)
{
  gboolean mandatory;

  gtk_tree_model_get (tm, iter,
		      HP_COL_MANDATORY, &mandatory,
		      -1);
  
  g_object_set (G_OBJECT (cell), 
		"sensitive", !mandatory,
		"activatable", !mandatory,
		NULL);
}

static void 
_sb_cell_condition_data_func (GtkTreeViewColumn *tc,
                    	      GtkCellRenderer *cell,
                    	      GtkTreeModel *tm,
                    	      GtkTreeIter *iter,
                    	      gpointer data)
{

}

static void 
_tn_cell_selection_data_func (GtkTreeViewColumn *tc,
                    	      GtkCellRenderer *cell,
                    	      GtkTreeModel *tm,
                    	      GtkTreeIter *iter,
                    	      gpointer data)
{
  GtkTreeIter _iter;
  guint selection = 0;
  gboolean toggled;
  GtkTreeModel *real_tm = GTK_IS_TREE_MODEL_FILTER (tm) ?
    		          gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (tm)) :
			  tm;

  gtk_tree_model_get_iter_first (real_tm, &_iter);

  do
  {
    gtk_tree_model_get (real_tm, &_iter,
		        HP_COL_CHECKBOX, &toggled,
		        -1);

    if (toggled)
      selection++;
  }
  while (gtk_tree_model_iter_next (real_tm, &_iter));  

  g_debug ("selection %d", selection); 

  if (selection >= TN_MAX_ITEMS)
  {
  } 	  
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
    _tn_visibility_filter,
    NULL,
    NULL);    

  hildon_plugin_settings_dialog_set_cell_data_func
    (HILDON_PLUGIN_SETTINGS_DIALOG (dialog),
     HPSD_COLUMN_TOGGLE,
     "Statusbar",
    _sb_cell_mandatory_data_func,
    NULL,
    NULL);    

  hildon_plugin_settings_dialog_set_cell_data_func
    (HILDON_PLUGIN_SETTINGS_DIALOG (dialog),
     HPSD_COLUMN_PB,
     "Statusbar",
    _sb_cell_condition_data_func,
    NULL,
    NULL);    

  hildon_plugin_settings_dialog_set_cell_data_func
    (HILDON_PLUGIN_SETTINGS_DIALOG (dialog),
     HPSD_COLUMN_TOGGLE,
     "Tasknavigator",
     _tn_cell_selection_data_func,
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

