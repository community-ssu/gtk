/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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

#include "hd-app-menu-tree.h"
#include "hd-applications-menu-settings-l10n.h"
#include <libhildondesktop/libhildonmenu.h>

enum
{
  PROP_MODEL = 1
};

static GObject *
hd_app_menu_tree_constructor (GType                   type,
                              guint                   n_construct_params,
                              GObjectConstructParam  *construct_params);

static void
hd_app_menu_tree_set_property (GObject         *object,
                               guint            property_id,
                               const GValue    *value,
                               GParamSpec      *pspec);

static void
hd_app_menu_tree_get_property (GObject         *object,
                               guint            property_id,
                               GValue          *value,
                               GParamSpec      *pspec);

static void
hd_app_menu_tree_navigation_changed (HDAppMenuTree *tree);

static void
hd_app_menu_tree_content_drag_received (HDAppMenuTree  *tree,
                                        GdkDragContext         *context,
                                        gint                    x,
                                        gint                    y,
                                        GtkSelectionData       *data);

static void
hd_app_menu_tree_navigation_drag_received (HDAppMenuTree          *tree,
                                           GdkDragContext         *context,
                                           gint                    x,
                                           gint                    y,
                                           GtkSelectionData       *data);

struct _HDAppMenuTreePrivate
{
  GtkTreeModel         *model, *navigation_model, *content_model;

  GtkWidget            *navigation_pane;
  GtkWidget            *content_pane;

  GtkWidget            *empty_label;

  GtkTreeSelection     *navigation_selection;
  GtkTreeSelection     *content_selection;
};

G_DEFINE_TYPE (HDAppMenuTree, hd_app_menu_tree, GTK_TYPE_HPANED);

static void
hd_app_menu_tree_init (HDAppMenuTree *tree)
{
  tree->priv = G_TYPE_INSTANCE_GET_PRIVATE (tree,
                                            HD_TYPE_APP_MENU_TREE,
                                            HDAppMenuTreePrivate);

}

static void
hd_app_menu_tree_class_init (HDAppMenuTreeClass *klass)
{
  GObjectClass         *object_class;
  GParamSpec           *pspec;

  object_class = G_OBJECT_CLASS (klass);

  object_class->constructor = hd_app_menu_tree_constructor;
  object_class->set_property = hd_app_menu_tree_set_property;
  object_class->get_property = hd_app_menu_tree_get_property;

  pspec = g_param_spec_object ("model",
                               "model",
                               "Model",
                               GTK_TYPE_TREE_MODEL,
                               G_PARAM_READWRITE);

  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   pspec);

  g_type_class_add_private (klass, sizeof (HDAppMenuTreePrivate));
}

static GObject *
hd_app_menu_tree_constructor (GType                   type,
                              guint                   n_construct_params,
                              GObjectConstructParam  *construct_params)
{
  HDAppMenuTreePrivate *priv;
  GObject              *object;
  GtkTreeViewColumn    *column;
  GtkCellRenderer      *renderer;
  const GtkTargetEntry  target_entries =
  {"HD_APP_MENU_ITEM", GTK_TARGET_SAME_APP, 0x42124212};

  object = G_OBJECT_CLASS (hd_app_menu_tree_parent_class)->
      constructor (type, n_construct_params, construct_params);

  priv = HD_APP_MENU_TREE (object)->priv;

  priv->empty_label = g_object_new (GTK_TYPE_LABEL,
                                    "label", HD_APP_MENU_DIALOG_EMPTY_CATEGORY,
                                    "yalign", 0.0,
                                    "visible", TRUE,
                                    NULL);
  g_object_ref (priv->empty_label);
  gtk_object_sink (GTK_OBJECT (priv->empty_label));

  priv->navigation_pane = g_object_new (GTK_TYPE_TREE_VIEW,
                                        "visible", TRUE,
                                        NULL);

  column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                         "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE,
                         NULL);

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column,
                                      renderer,
                                      "pixbuf",
                                      TREE_MODEL_ICON);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column,
                                      renderer,
                                      "text",
                                      TREE_MODEL_LOCALIZED_NAME);

  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->navigation_pane),
                               column);

  gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (priv->navigation_pane),
                                          GDK_MODIFIER_MASK,
                                          &target_entries,
                                          1,
                                          GDK_ACTION_MOVE);

  gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (priv->navigation_pane),
                                        &target_entries,
                                        1,
                                        GDK_ACTION_MOVE);

  g_signal_connect_swapped (priv->navigation_pane, "drag-data-received",
                            G_CALLBACK (hd_app_menu_tree_navigation_drag_received),
                            object);

  g_signal_connect_swapped (priv->navigation_pane, "cursor-changed",
                            G_CALLBACK (hd_app_menu_tree_navigation_changed),
                            object);

  priv->navigation_selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->navigation_pane));

  gtk_paned_add1 (GTK_PANED (object), priv->navigation_pane);

  priv->content_pane = g_object_new (GTK_TYPE_TREE_VIEW,
                                     "visible", TRUE,
                                     NULL);

  g_object_ref (priv->content_pane);
  gtk_object_sink (GTK_OBJECT (priv->content_pane));

  column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                         "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE,
                         NULL);

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column,
                                      renderer,
                                      "pixbuf",
                                      TREE_MODEL_ICON);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column,
                                      renderer,
                                      "text",
                                      TREE_MODEL_LOCALIZED_NAME);

  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->content_pane),
                               column);
  gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (priv->content_pane),
                                          GDK_MODIFIER_MASK,
                                          &target_entries,
                                          1,
                                          GDK_ACTION_MOVE);

  gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (priv->content_pane),
                                        &target_entries,
                                        1,
                                        GDK_ACTION_MOVE);

  g_signal_connect_swapped (priv->content_pane, "drag-data-received",
                            G_CALLBACK (hd_app_menu_tree_content_drag_received),
                            object);

  priv->content_selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->content_pane));

  gtk_paned_add2 (GTK_PANED (object), priv->empty_label);

  return object;

}

static void
hd_app_menu_tree_set_property (GObject         *object,
                               guint            property_id,
                               const GValue    *value,
                               GParamSpec      *pspec)
{
  switch (property_id)
  {
      case PROP_MODEL:
          hd_app_menu_tree_set_model (HD_APP_MENU_TREE (object),
                                      g_value_get_object (value));
          break;
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
  }

}

static void
hd_app_menu_tree_get_property (GObject         *object,
                               guint            property_id,
                               GValue          *value,
                               GParamSpec      *pspec)
{
  switch (property_id)
  {
      case PROP_MODEL:
          g_value_set_object (value,
                              HD_APP_MENU_TREE (object)->priv->model);
          break;
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
  }
}

static void
hd_app_menu_tree_copy_iter (GtkTreeModel       *model,
                            GtkTreeIter        *src,
                            GtkTreeIter        *dst)
{
  gint          i;

  for (i = 0; i < gtk_tree_model_get_n_columns (model); i++)
  {
    GValue dst_value = {0};
    GValue value = {0};

    gtk_tree_model_get_value (model, src, i, &value);
    g_value_init (&dst_value, gtk_tree_model_get_column_type (model, i));
    g_value_copy (&value, &dst_value);
    gtk_tree_store_set_value (GTK_TREE_STORE (model),
                              dst,
                              i,
                              &dst_value);
  }
}

static gboolean
hd_app_menu_tree_navigation_visible (GtkTreeModel      *model,
                                     GtkTreeIter       *iter,
                                     HDAppMenuTree     *tree)
{
  GtkTreePath  *path;
  gint          depth;

  path = gtk_tree_model_get_path (model, iter);
  depth = gtk_tree_path_get_depth (path);
  gtk_tree_path_free (path);

  return (depth == 1);
}

static void
hd_app_menu_tree_navigation_changed (HDAppMenuTree *tree)
{
  HDAppMenuTreePrivate *priv;
  GtkTreePath          *cursor_path, *main_path;
  GtkTreeIter           iter;

  priv = tree->priv;

  gtk_tree_view_get_cursor (GTK_TREE_VIEW (priv->navigation_pane),
                            &cursor_path,
                            NULL);

  if (!cursor_path) return;

  main_path =
      gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (priv->navigation_model),
                                                        cursor_path);

  if (priv->content_model)
    g_object_unref (priv->content_model);

  priv->content_model = gtk_tree_model_filter_new (priv->model, main_path);
  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->content_pane),
                           priv->content_model);

  if (gtk_tree_model_get_iter_first (priv->content_model, &iter))
  {
    if (priv->content_pane->parent != GTK_WIDGET (tree))
    {
      gtk_container_remove (GTK_CONTAINER (tree), priv->empty_label);
      gtk_paned_add2 (GTK_PANED (tree), priv->content_pane);
    }
  }
  else
  {
    if (priv->empty_label->parent != GTK_WIDGET (tree))
    {
      gtk_container_remove (GTK_CONTAINER (tree), priv->content_pane);
      gtk_paned_add2 (GTK_PANED (tree), priv->empty_label);
    }
  }

}

static void
hd_app_menu_tree_content_drag_received (HDAppMenuTree          *tree,
                                        GdkDragContext         *context,
                                        gint                    x,
                                        gint                    y,
                                        GtkSelectionData       *data)
{
  HDAppMenuTreePrivate *priv = tree->priv;

  if (gtk_drag_get_source_widget (context) == priv->content_pane)
  {
    GtkTreePath        *dest_path,   *translated_dest_path;
    GtkTreePath        *cursor_path, *translated_cursor_path;
    GtkTreeIter         dest_iter,  cursor_iter, selection_iter;
    GtkTreeViewDropPosition pos;

    if (!gtk_tree_selection_get_selected (priv->content_selection,
                                         NULL,
                                         &selection_iter))
      return;

    gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (priv->content_pane),
                                       x, y,
                                       &dest_path,
                                       &pos);

    if (!dest_path)
      return;

    translated_dest_path =
        gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (priv->content_model),
                                                          dest_path);


    gtk_tree_model_get_iter (priv->model, &dest_iter, translated_dest_path);


    cursor_path = gtk_tree_model_get_path (priv->content_model,
                                           &selection_iter);

    translated_cursor_path =
        gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (priv->content_model),
                                                          cursor_path);

    gtk_tree_model_get_iter (priv->model, &cursor_iter, translated_cursor_path);

    switch (pos)
    {
        case GTK_TREE_VIEW_DROP_BEFORE:
            gtk_tree_store_move_before (GTK_TREE_STORE (priv->model),
                                        &cursor_iter,
                                        &dest_iter);
            break;

        case GTK_TREE_VIEW_DROP_AFTER:
            gtk_tree_store_move_after (GTK_TREE_STORE (priv->model),
                                       &cursor_iter,
                                       &dest_iter);
            break;
        default:
            gtk_tree_store_swap (GTK_TREE_STORE (priv->model),
                                 &cursor_iter,
                                 &dest_iter);
            break;
    }
  }
}

static void
hd_app_menu_tree_navigation_drag_received (HDAppMenuTree          *tree,
                                           GdkDragContext         *context,
                                           gint                    x,
                                           gint                    y,
                                           GtkSelectionData       *data)
{
  HDAppMenuTreePrivate *priv = tree->priv;
  GtkWidget            *drag_source;

  drag_source = gtk_drag_get_source_widget (context);

  if (drag_source == priv->navigation_pane)
  {
    GtkTreePath        *dest_path,   *translated_dest_path;
    GtkTreePath        *cursor_path, *translated_cursor_path;
    GtkTreeIter         dest_iter,  cursor_iter, selection_iter;
    GtkTreeViewDropPosition pos;

    if (!gtk_tree_selection_get_selected (priv->navigation_selection,
                                         NULL,
                                         &selection_iter))
      return;

    gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (priv->navigation_pane),
                                       x, y,
                                       &dest_path,
                                       &pos);

    if (!dest_path)
      return;

    translated_dest_path =
        gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (priv->navigation_model),
                                                          dest_path);


    gtk_tree_model_get_iter (priv->model, &dest_iter, translated_dest_path);


    cursor_path = gtk_tree_model_get_path (priv->navigation_model,
                                           &selection_iter);

    translated_cursor_path =
        gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (priv->navigation_model),
                                                          cursor_path);

    gtk_tree_model_get_iter (priv->model, &cursor_iter, translated_cursor_path);

    switch (pos)
    {
        case GTK_TREE_VIEW_DROP_BEFORE:
            gtk_tree_store_move_before (GTK_TREE_STORE (priv->model),
                                        &cursor_iter,
                                        &dest_iter);
            break;

        case GTK_TREE_VIEW_DROP_AFTER:
        default:
            gtk_tree_store_move_after (GTK_TREE_STORE (priv->model),
                                       &cursor_iter,
                                       &dest_iter);
            break;
    }
  }
  else if (drag_source == priv->content_pane)
  {
    GtkTreePath        *dest_path,   *translated_dest_path;
    GtkTreePath        *cursor_path, *translated_cursor_path;
    GtkTreeIter         dest_iter,  cursor_iter, selection_iter, new_iter;
    GtkTreeViewDropPosition pos;

    if (!gtk_tree_selection_get_selected (priv->content_selection,
                                         NULL,
                                         &selection_iter))
      return;

    gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (priv->navigation_pane),
                                       x, y,
                                       &dest_path,
                                       &pos);

    if (!dest_path)
      return;

    if (pos != GTK_TREE_VIEW_DROP_INTO_OR_BEFORE &&
        pos != GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
      return;

    translated_dest_path =
        gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (priv->navigation_model),
                                                          dest_path);


    gtk_tree_model_get_iter (priv->model, &dest_iter, translated_dest_path);


    cursor_path = gtk_tree_model_get_path (priv->content_model,
                                           &selection_iter);

    translated_cursor_path =
        gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (priv->content_model),
                                                          cursor_path);

    gtk_tree_model_get_iter (priv->model, &cursor_iter, translated_cursor_path);

    gtk_tree_store_append (GTK_TREE_STORE (priv->model),
                           &new_iter,
                           &dest_iter);
    hd_app_menu_tree_copy_iter (priv->model, &cursor_iter, &new_iter);
    gtk_tree_store_remove (GTK_TREE_STORE (priv->model), &cursor_iter);

  }
}

void
hd_app_menu_tree_set_model (HDAppMenuTree *tree, GtkTreeModel *model)
{
  HDAppMenuTreePrivate *priv;

  g_return_if_fail (HD_IS_APP_MENU_TREE (tree));
  g_return_if_fail (!model || GTK_IS_TREE_MODEL (model));

  priv = tree->priv;

  if (priv->model)
    g_object_unref (priv->model);

  if (priv->navigation_model)
  {
    g_object_unref (priv->navigation_model);
    priv->navigation_model = NULL;
  }

  if (priv->content_model)
  {
    g_object_unref (priv->content_model);
    priv->content_model = NULL;
  }

  priv->model = model;
  if (priv->model)
  {
    GtkTreePath        *path = gtk_tree_path_new_first ();

    g_object_ref (priv->model);

    priv->navigation_model = gtk_tree_model_filter_new (priv->model, NULL);

    gtk_tree_model_filter_set_visible_func (
                           GTK_TREE_MODEL_FILTER (priv->navigation_model),
                           (GtkTreeModelFilterVisibleFunc)
                                hd_app_menu_tree_navigation_visible,
                           tree,
                           NULL);

    gtk_tree_view_set_model (GTK_TREE_VIEW (priv->navigation_pane),
                             priv->navigation_model);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->navigation_pane),
                              path,
                              NULL,
                              FALSE);

    gtk_tree_path_free (path);

  }

  g_object_notify (G_OBJECT (tree), "model");
}

gboolean
hd_app_menu_tree_get_selected_category (HDAppMenuTree *tree, GtkTreeIter *iter)
{
  HDAppMenuTreePrivate *priv;
  GtkTreePath          *cursor_path, *main_path;
  gboolean              result;
  g_return_val_if_fail (HD_IS_APP_MENU_TREE (tree) && iter, FALSE);
  priv = tree->priv;

  if (!GTK_IS_TREE_MODEL (priv->model))
    return FALSE;

  g_debug ("Running get_selected");

  gtk_tree_view_get_cursor (GTK_TREE_VIEW (priv->navigation_pane),
                            &cursor_path,
                            NULL);

  if (!cursor_path)
  {
    g_debug ("No currently selected cat");
    return FALSE;
  }

  main_path =
      gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (priv->navigation_model),
                                                        cursor_path);

  gtk_tree_path_free (cursor_path);
  if (!main_path)
  {
    g_debug ("Could not translate path");
    return FALSE;
  }

  result = gtk_tree_model_get_iter (priv->model, iter, main_path);


  gtk_tree_path_free (main_path);
  g_debug ("Returning: %i", result);

  return result;
}
