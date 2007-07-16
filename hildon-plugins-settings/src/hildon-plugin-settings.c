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

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "hd-marshalers.h"

#include "hildon-plugin-settings-dialog.h"

#define SB_STATUS_NAME "org.hildon.Statusbar"
#define SB_STATUS_PATH "/org/hildon/Statusbar"
#define SB_STATUS_INTERFACE "org.hildon.Statusbar"
#define SB_STATUS_METHOD "RefreshItemsStatus"

#define SB_CATEGORY_TEMPORAL "temporal"
#define SB_CATEGORY_CONDITIONAL "conditional"

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
  if (!model || !iter)
    return TRUE;	  
	
  gchar *name = NULL;

  gtk_tree_model_get (model,
                      iter,
                      0,
                      &name,
                      -1);

  if (!name)
    return TRUE;	  

  if (g_str_equal (name,HP_APPLICATION_SWITCHER) ||
      g_str_equal (name,HP_SWITCHER_MENU))
    return FALSE;	  

  return TRUE;
}

static gboolean
_sb_visibility_filter (GtkTreeModel *model,
                       GtkTreeIter *iter,
                       gpointer data)
{
  if (!model || !iter)
    return TRUE;

  gboolean flag = TRUE;
  gchar *category = NULL;

  gtk_tree_model_get (model,iter,
		      HP_COL_FLAG, &flag,
		      HP_COL_CATEGORY, &category,
		      -1);

  if (!category)
    return TRUE;
  
  if (g_str_equal (category, SB_CATEGORY_CONDITIONAL) && !flag)		  
    return FALSE;

  return TRUE;  
}

static void 
_cell_mandatory_data_func (GtkTreeViewColumn *tc,
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
  gchar *category = NULL;
  gboolean status = TRUE, cb = TRUE;
	
  gtk_tree_model_get (tm, iter,
		      HP_COL_CATEGORY, &category,
		      HP_COL_FLAG, &status,
		      HP_COL_CHECKBOX, &cb,
		      -1);

  if (!category)
    return;

  if (g_str_equal (category, SB_CATEGORY_TEMPORAL) || 
      g_str_equal (category, SB_CATEGORY_CONDITIONAL))
    g_object_set (G_OBJECT (cell), "sensitive", status, NULL);
  else
  if (cb)	  
    g_object_set (G_OBJECT (cell), "sensitive", TRUE, NULL);	  
}

static void 
_sb_update_status (DBusGProxy *proxy,
		   gchar *name,
		   gboolean status,
		   GtkTreeModel *tm)
{
  GtkTreeIter iter;
  
  gtk_tree_model_get_iter_first (tm, &iter);

  do
  {
    gboolean old_status = TRUE;
    gchar *iter_name = NULL, *iter_category = NULL;

    gtk_tree_model_get (tm, &iter,
		        HP_COL_DESKTOP_FILE, &iter_name,
			HP_COL_FLAG, &old_status,
			HP_COL_CATEGORY, &iter_category,
			-1);

    if (!iter_name && !iter_category)
      continue;	    

    if (g_str_equal (name, iter_name))
    {	    
      if (old_status != status)
        gtk_list_store_set (GTK_LIST_STORE (tm), &iter,
	  		    HP_COL_FLAG, status,
		 	    -1);		   
    }		        

    g_free (iter_name);
    g_free (iter_category);    
  }
  while (gtk_tree_model_iter_next (tm, &iter));  
}

static void 
_sb_update_flag (GtkTreeModel *tm)
{
  GtkTreeIter iter;

  gtk_tree_model_get_iter_first (tm, &iter);

  do
  {
    gchar *iter_category = NULL;

    gtk_tree_model_get (tm, &iter,
                        HP_COL_CATEGORY, &iter_category,
                        -1);

    if (!iter_category)
      continue;
    
    if (g_str_equal (SB_CATEGORY_TEMPORAL, iter_category) ||
        g_str_equal (SB_CATEGORY_CONDITIONAL, iter_category))
    {
      gtk_list_store_set (GTK_LIST_STORE (tm), &iter,
                          HP_COL_FLAG, FALSE,
                          -1);
    }

    g_free (iter_category);
  }
  while (gtk_tree_model_iter_next (tm, &iter));
}
	

osso_return_t 
execute (osso_context_t *osso,
	 gpointer user_data,
	 gboolean user_activated)
{
  gint ret;
  DBusGConnection *conn = NULL;
  GError *error = NULL;
  GtkTreeModel *sbtm = NULL;

  GtkWidget *dialog = hildon_plugin_settings_dialog_new (GTK_WINDOW (user_data));

  hildon_plugin_settings_dialog_set_visibility_filter
    (HILDON_PLUGIN_SETTINGS_DIALOG (dialog),
     "Tasknavigator",
    _tn_visibility_filter,
    NULL,
    NULL);    

  hildon_plugin_settings_dialog_set_visibility_filter
    (HILDON_PLUGIN_SETTINGS_DIALOG (dialog),
     "Statusbar",
    _sb_visibility_filter,
    NULL,
    NULL);    

  hildon_plugin_settings_dialog_set_cell_data_func
    (HILDON_PLUGIN_SETTINGS_DIALOG (dialog),
     HPSD_COLUMN_TOGGLE,
     "Statusbar",
    _cell_mandatory_data_func,
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
    _cell_mandatory_data_func,
    NULL,
    NULL);    

  hildon_plugin_settings_dialog_set_choosing_limit 
    (HILDON_PLUGIN_SETTINGS_DIALOG (dialog),
     "Tasknavigator",
     3);

  sbtm = hildon_plugin_settings_dialog_get_model_by_name
          (HILDON_PLUGIN_SETTINGS_DIALOG (dialog),
           "Statusbar",
           FALSE);

  _sb_update_flag (sbtm);

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (!error)
  {
    DBusGProxy *proxy = dbus_g_proxy_new_for_name (conn,
		    				   SB_STATUS_NAME,
						   SB_STATUS_PATH,
						   SB_STATUS_INTERFACE);

    dbus_g_object_register_marshaller (g_cclosure_user_marshal_VOID__STRING_BOOLEAN,
                                        G_TYPE_NONE,
                                        G_TYPE_STRING,
                                        G_TYPE_BOOLEAN,
                                        G_TYPE_INVALID);

    dbus_g_proxy_add_signal (proxy,
		    	     "UpdateStatus",
			     G_TYPE_STRING,
			     G_TYPE_BOOLEAN,
			     G_TYPE_INVALID);

    dbus_g_proxy_connect_signal (proxy,
				 "UpdateStatus",
				 G_CALLBACK (_sb_update_status),
				 sbtm,
				 NULL);

    dbus_g_proxy_call (proxy,
		       "RefreshItemsStatus",
		       &error,
		       G_TYPE_INVALID);
    
    if (error)
    {
      g_warning ("Oops: %s", error->message);
      g_error_free (error);
    }		    
  }
  else
  {
    g_warning ("%s: I couldn't connect to the bus: %s",__FILE__,error->message);
    g_error_free (error);
  }
  	  

  gtk_widget_show (dialog);

  ret = gtk_dialog_run (GTK_DIALOG (dialog));

  if (ret == GTK_RESPONSE_OK)
  {
  }
    
  gtk_widget_destroy (dialog);
    
  return OSSO_OK;  
}

