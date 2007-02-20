/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author: Johan Bilien <johan.bilien@nokia.com>
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


#include "hildon-home-select-applets-dialog.h"
#include <libhildondesktop/hildon-plugin-list.h>
#include "hildon-home-l10n.h"

#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtktreeselection.h>
#include <gdk/gdkkeysyms.h>

/* FIXME */
#define DIALOG_WIDTH 400

static
void hildon_home_select_applets_dialog_applet_toggled 
                                (GtkCellRendererToggle *cell_renderer,
                                 gchar *path,
                                 gpointer user_data);

static gboolean 
hildon_home_select_applets_dialog_key_press_event (GtkWidget *widget,
                                                   GdkEventKey *event);




GtkWidget *
hildon_home_select_applets_dialog_new_with_model (GtkTreeModel *model)
{
  GtkWidget        *dialog;
  GtkWidget        *scrollwindow;
  GtkWidget        *treeview;
  GtkCellRenderer  *cell_renderer;

  dialog = gtk_dialog_new_with_buttons (HH_SELECT_DIALOG_TITLE,
                                        NULL,
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT |
                                        GTK_DIALOG_NO_SEPARATOR,
                                        HH_SELECT_DIALOG_OK,
                                        GTK_RESPONSE_OK,
                                        HH_SELECT_DIALOG_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);

  gtk_widget_set_size_request (dialog, DIALOG_WIDTH, -1);

  scrollwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwindow),
                                       GTK_SHADOW_ETCHED_IN);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwindow),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_NEVER);

  treeview = gtk_tree_view_new_with_model (model);
  g_object_set (treeview, "allow-checkbox-mode", FALSE, NULL);

  cell_renderer = gtk_cell_renderer_toggle_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), 
                                               -1,
                                               NULL,
                                               cell_renderer,
                                               "active",
                                               HILDON_PLUGIN_LIST_COLUMN_ACTIVE,
                                               NULL);
  g_object_set (cell_renderer, "activatable", TRUE, NULL);

  g_signal_connect (G_OBJECT (cell_renderer), "toggled",
                    G_CALLBACK (
                         hildon_home_select_applets_dialog_applet_toggled),
                   model);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(treeview),
                                               -1,
                                               NULL,
                                               gtk_cell_renderer_text_new(),
                                               "text",
                                               HILDON_PLUGIN_LIST_COLUMN_NAME,
                                               NULL);

  gtk_container_add (GTK_CONTAINER (scrollwindow), treeview);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), scrollwindow);

  /* Work around Hildon GTK bug */
  g_signal_connect (G_OBJECT (treeview), "key-press-event",
                    G_CALLBACK (
                          hildon_home_select_applets_dialog_key_press_event),
                    NULL);

  gtk_widget_show_all (GTK_DIALOG (dialog)->vbox);

  return dialog;
}


static
void hildon_home_select_applets_dialog_applet_toggled 
                                (GtkCellRendererToggle *cell_renderer,
                                 gchar *path,
                                 gpointer user_data)
{
  GtkTreeIter iter;
  gboolean active;

  /* Get the GtkTreeModel iter */
  GtkTreeModel *model = GTK_TREE_MODEL (user_data);

  if (!gtk_tree_model_get_iter_from_string (model, &iter, path))
    {
      return;
    }

  /* Get boolean value */
  gtk_tree_model_get (model,
                      &iter,
                      HILDON_PLUGIN_LIST_COLUMN_ACTIVE,
                      &active,
                      -1);

  /* Change the iter value on the TreeModel */
  gtk_list_store_set (GTK_LIST_STORE (model),
                      &iter,
                      HILDON_PLUGIN_LIST_COLUMN_ACTIVE, 
                      !active,
                      -1);
}

static gboolean 
hildon_home_select_applets_dialog_key_press_event (GtkWidget *widget,
                                                   GdkEventKey *event)
{
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gboolean active;


  switch (event->keyval)
    {
      case GDK_Return:
      case GDK_KP_Enter:

          if (!(model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget))))
            return FALSE;
          
          if (!(selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(widget))))
            return FALSE;
          
          if (!(gtk_tree_selection_get_selected (selection, &model, &iter)))
            return FALSE;

          /* Get boolean value */
          gtk_tree_model_get (model,
                              &iter,
                              HILDON_PLUGIN_LIST_COLUMN_ACTIVE,
                              &active,
                              -1);
          /* Toggle item selected value on the TreeModel */
          gtk_list_store_set (GTK_LIST_STORE(model),
                              &iter,
                              HILDON_PLUGIN_LIST_COLUMN_ACTIVE,
                              !active,
                              -1);
          return TRUE;
      default:
          return FALSE;
    }

  return FALSE;
}
