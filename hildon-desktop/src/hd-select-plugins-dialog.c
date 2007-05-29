/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
 *          Johan Bilien <johan.bilien@nokia.com>
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

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#ifdef HAVE_LIBHILDONHELP
#include <libosso.h>
#include <hildon/hildon-help.h>
#endif

#include "hd-select-plugins-dialog.h"
#include "hd-config.h"

#define HD_SELECT_PLUGINS_DIALOG_WIDTH  400 
#define HD_SELECT_PLUGINS_DIALOG_TITLE  _("home_ti_select_applets")
#define HD_SELECT_PLUGINS_DIALOG_OK     _("home_bd_select_applets_ok")
#define HD_SELECT_PLUGINS_DIALOG_CANCEL _("home_bd_select_applets_cancel")
#define HD_SELECT_PLUGINS_HELP_TOPIC    "uiframework_home_select_applets"

enum
{
  HD_SPD_COLUMN_NAME = 0,
  HD_SPD_COLUMN_ACTIVE,
  HD_SPD_COLUMN_DESKTOP_FILE,
  HD_SPD_N_COLUMNS
};

static GList *
hd_select_plugins_dialog_get_selected (GtkListStore *plugin_list)
{
  GtkTreeIter iter;
  gboolean valid;
  GList *selected_plugins = NULL;

  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (plugin_list), &iter);

  while (valid)
  {
    gboolean active;
    gchar *desktop_file;    
 
    gtk_tree_model_get (GTK_TREE_MODEL (plugin_list),
                        &iter,
                        HD_SPD_COLUMN_ACTIVE, &active,
                        HD_SPD_COLUMN_DESKTOP_FILE, &desktop_file,
                        -1);

    if (active)
    {
       selected_plugins = g_list_append (selected_plugins, desktop_file);
    }

    valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (plugin_list), &iter);
  }

  return selected_plugins;
}

static gint
hd_select_plugins_dialog_find_func (gconstpointer a, gconstpointer b)
{
  gint result = -1;
  gchar *id = NULL;

  g_object_get (G_OBJECT (a),
                "id", &id,
                NULL);

  if (!g_ascii_strcasecmp (id, b))
    result = 0;

  g_free (id);

  return result;
}

static void 
hd_select_plugins_dialog_plugin_toggled
                                (GtkCellRendererToggle *cell_renderer,
                                 gchar *path,
                                 gpointer user_data)
{
  GtkTreeIter iter;
  gboolean active;

  /* Get the GtkTreeModel iter */
  GtkTreeModel *model = GTK_TREE_MODEL (user_data);

  if (!gtk_tree_model_get_iter_from_string (model, &iter, path)) return;

  /* Get boolean value */
  gtk_tree_model_get (model,
                      &iter,
                      HD_SPD_COLUMN_ACTIVE, &active,
                      -1);

  /* Change the iter value on the TreeModel */
  gtk_list_store_set (GTK_LIST_STORE (model),
                      &iter,
                      HD_SPD_COLUMN_ACTIVE, !active,
                      -1);
}

static GtkListStore *
hd_select_plugins_dialog_get_store (GList *loaded_plugins,
                                    const gchar *plugin_dir)
{
  GDir *dir;
  GKeyFile *keyfile;
  const char *filename;
  GError *error = NULL;
  GtkListStore *store;
  GtkTreeIter iter;

  store = gtk_list_store_new (HD_SPD_N_COLUMNS, 
                              G_TYPE_STRING, 
                              G_TYPE_BOOLEAN,
                              G_TYPE_STRING); 

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                        HD_SPD_COLUMN_NAME,
                                        GTK_SORT_ASCENDING);

  dir = g_dir_open (plugin_dir, 0, &error);

  if (!dir)
  {
    g_warning ("Error reading plugin directory: %s", error->message);

    g_error_free (error);

    return NULL;
  }

  keyfile = g_key_file_new ();

  while ((filename = g_dir_read_name (dir)))
  {
    gchar *desktop_path = NULL;
    gchar *name = NULL;
    GList *active;
    error = NULL;

    /* Only consider .desktop files */
    if (!g_str_has_suffix (filename, ".desktop")) continue;

    desktop_path = g_build_filename (plugin_dir, filename, NULL);

    g_key_file_load_from_file (keyfile,
                               desktop_path,
                               G_KEY_FILE_NONE,
                               &error);

    if (error)
    {
      g_warning ("Error loading plugin configuration file: %s", error->message);

      g_error_free (error);
      g_dir_close (dir);

      return NULL;
    }

    name = g_key_file_get_locale_string (keyfile,
                                         HD_PLUGIN_CONFIG_GROUP,
                                         HD_PLUGIN_CONFIG_KEY_NAME,
                                         NULL /* current locale */,
                                         &error);

    if (error)
    {
      g_warning ("Error reading plugin configuration file: %s", error->message);

      g_error_free (error);
      g_dir_close (dir);

      return NULL;
    }

    active = g_list_find_custom (loaded_plugins, 
                                 desktop_path, 
                                 hd_select_plugins_dialog_find_func);

    gtk_list_store_append (GTK_LIST_STORE (store), &iter);

    gtk_list_store_set (GTK_LIST_STORE (store), &iter, 
                        HD_SPD_COLUMN_NAME, _(name),
                        HD_SPD_COLUMN_ACTIVE, active,
                        HD_SPD_COLUMN_DESKTOP_FILE, desktop_path,
                        -1);

    g_free (name);
  }

  g_dir_close (dir);
  return store;
}

gint
hd_select_plugins_dialog_run (GList           *loaded_plugins,
#ifdef HAVE_LIBHILDONHELP
                              osso_context_t  *osso_context,
#endif
                              const gchar     *plugin_dir,
                              GList          **selected_plugins)
{
  GtkWidget *dialog;
  GtkWidget *scrollwindow;
  GtkWidget *treeview;
  GtkCellRenderer *cell_renderer;
  GtkListStore *plugin_list;
  GtkRequisition req;
  gint response;

  plugin_list = hd_select_plugins_dialog_get_store (loaded_plugins, 
                                                    plugin_dir);

  if (!plugin_list)
  {
    return GTK_RESPONSE_CANCEL;
  }

  dialog = gtk_dialog_new_with_buttons (HD_SELECT_PLUGINS_DIALOG_TITLE,
                                        NULL,
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT |
                                        GTK_DIALOG_NO_SEPARATOR,
                                        HD_SELECT_PLUGINS_DIALOG_OK,
                                        GTK_RESPONSE_OK,
                                        HD_SELECT_PLUGINS_DIALOG_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);

  gtk_widget_set_size_request (dialog, HD_SELECT_PLUGINS_DIALOG_WIDTH, -1);

#ifdef HAVE_LIBHILDONHELP
  /* Add help button to the dialog */
  hildon_help_dialog_help_enable (GTK_DIALOG (dialog),
                                  HD_SELECT_PLUGINS_HELP_TOPIC,
                                  osso_context);
#endif

  scrollwindow = gtk_scrolled_window_new (NULL, NULL);

  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwindow),
                                       GTK_SHADOW_ETCHED_IN);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwindow),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);


  treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (plugin_list));

  g_object_set (treeview, "allow-checkbox-mode", FALSE, NULL);

  cell_renderer = gtk_cell_renderer_toggle_new ();

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), 
                                               -1,
                                               NULL,
                                               cell_renderer,
                                               "active",
                                               HD_SPD_COLUMN_ACTIVE,
                                               NULL);

  g_object_set (cell_renderer, "activatable", TRUE, NULL);

  g_signal_connect (G_OBJECT (cell_renderer), 
                    "toggled",
                    G_CALLBACK (hd_select_plugins_dialog_plugin_toggled),
                    plugin_list);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
                                               -1,
                                               NULL,
                                               gtk_cell_renderer_text_new (),
                                               "text",
                                               HD_SPD_COLUMN_NAME,
                                               NULL);

  gtk_container_add (GTK_CONTAINER (scrollwindow), treeview);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), scrollwindow);

  gtk_widget_size_request (treeview, &req); 
  gtk_widget_set_size_request (scrollwindow, -1, req.height + 4);

  gtk_widget_show_all (GTK_DIALOG (dialog)->vbox);

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  switch (response)
  {
    case GTK_RESPONSE_OK:
      *selected_plugins = hd_select_plugins_dialog_get_selected (plugin_list);
      break;
  }

  gtk_widget_destroy (dialog);
  g_object_unref (plugin_list);

  return response;
}
