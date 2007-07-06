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

#include "hd-app-menu-dialog.h"
#include "hd-app-menu-tree.h"
#include "hd-applications-menu-settings-l10n.h"

#include <libhildondesktop/libhildonmenu.h>

#include <hildon/hildon-defines.h>
#include <hildon/hildon-caption.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-helper.h>

#define HD_APP_MENU_DIALOG_WIDTH        590
#define HD_APP_MENU_DIALOG_HEIGHT       (8*30 + 2*HILDON_MARGIN_DEFAULT)

enum
{
  RESPONSE_NEW = 1,
  RESPONSE_DELETE,
  RESPONSE_RENAME
};

enum
{
  PROP_MODEL = 1,
};

static GObject *
hd_app_menu_dialog_constructor (GType                   type,
                                guint                   n_construct_params,
                                GObjectConstructParam  *construct_params);

static void
hd_app_menu_dialog_set_property (GObject         *object,
                                 guint            property_id,
                                 const GValue    *value,
                                 GParamSpec      *pspec);

static void
hd_app_menu_dialog_get_property (GObject         *object,
                                 guint            property_id,
                                 GValue          *value,
                                 GParamSpec      *pspec);

static void
hd_app_menu_dialog_response (GtkDialog *dialog, gint response);

static void
hd_app_menu_dialog_new_category (HDAppMenuDialog *dialog);

static void
hd_app_menu_dialog_rename_category (HDAppMenuDialog *dialog);

static void
hd_app_menu_dialog_delete_category (HDAppMenuDialog *dialog);

static void
hd_app_menu_dialog_item_selected (HDAppMenuDialog      *dialog,
                                  GtkTreeIter          *iter);

struct _HDAppMenuDialogPrivate
{
  GtkTreeModel         *model;

  GtkWidget            *new_button, *rename_button, *delete_button, *done_button;
  GtkWidget            *tree;
};

G_DEFINE_TYPE (HDAppMenuDialog, hd_app_menu_dialog, GTK_TYPE_DIALOG);

static void
hd_app_menu_dialog_init (HDAppMenuDialog *dialog)
{
  dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
                                              HD_TYPE_APP_MENU_DIALOG,
                                              HDAppMenuDialogPrivate);

}

static void
hd_app_menu_dialog_class_init (HDAppMenuDialogClass *klass)
{
  GObjectClass         *object_class;
  GtkDialogClass       *dialog_class;
  GParamSpec           *pspec;

  object_class = G_OBJECT_CLASS (klass);
  dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->constructor = hd_app_menu_dialog_constructor;
  object_class->set_property = hd_app_menu_dialog_set_property;
  object_class->get_property = hd_app_menu_dialog_get_property;

  dialog_class->response = hd_app_menu_dialog_response;

  pspec = g_param_spec_object ("model",
                               "model",
                               "Model",
                               GTK_TYPE_TREE_MODEL,
                               G_PARAM_READWRITE);

  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   pspec);

  g_type_class_add_private (klass, sizeof (HDAppMenuDialogPrivate));
}

static GObject *
hd_app_menu_dialog_constructor (GType                   type,
                                guint                   n_construct_params,
                                GObjectConstructParam  *construct_params)
{
  HDAppMenuDialogPrivate       *priv;
  GtkWidget                    *widget;
  GtkDialog                    *dialog;
  GObject                      *object;

  object = G_OBJECT_CLASS (hd_app_menu_dialog_parent_class)->
      constructor (type, n_construct_params, construct_params);

  widget = GTK_WIDGET (object);
  dialog = GTK_DIALOG (object);
  priv = HD_APP_MENU_DIALOG (object)->priv;

  gtk_window_set_title (GTK_WINDOW (object), HD_APP_MENU_DIALOG_TITLE);
  gtk_window_set_modal (GTK_WINDOW (object), TRUE);

  gtk_dialog_set_has_separator (dialog, FALSE);

  priv->new_button = gtk_dialog_add_button (dialog,
                                            HD_APP_MENU_DIALOG_NEW,
                                            RESPONSE_NEW);
  gtk_widget_show (priv->new_button);

  priv->delete_button = gtk_dialog_add_button (dialog,
                                               HD_APP_MENU_DIALOG_DELETE,
                                               RESPONSE_DELETE);
  gtk_widget_show (priv->delete_button);
  hildon_helper_set_insensitive_message (priv->delete_button,
                                         HD_APP_MENU_DIALOG_NO_APP_DEL);

  priv->rename_button = gtk_dialog_add_button (dialog,
                                               HD_APP_MENU_DIALOG_MOVE,
                                               RESPONSE_RENAME);
  gtk_widget_show (priv->rename_button);
  hildon_helper_set_insensitive_message (priv->rename_button,
                                         HD_APP_MENU_DIALOG_NO_APP_REN);

  /* Use CANCEL so the ESC key closes the dialog */
  priv->done_button = gtk_dialog_add_button (dialog,
                                             HD_APP_MENU_DIALOG_DONE,
                                             GTK_RESPONSE_CANCEL);
  gtk_widget_show (priv->done_button);

  priv->tree = g_object_new (HD_TYPE_APP_MENU_TREE,
                             "visible", TRUE,
                             "width-request",  HD_APP_MENU_DIALOG_WIDTH,
                             "height-request", HD_APP_MENU_DIALOG_HEIGHT,
                             NULL);

  g_signal_connect_swapped (priv->tree, "item-selected",
                            G_CALLBACK (hd_app_menu_dialog_item_selected),
                            dialog);

  gtk_box_pack_end (GTK_BOX (dialog->vbox), priv->tree, TRUE, TRUE, 0);

  return object;

}

static void
hd_app_menu_dialog_set_property (GObject         *object,
                                 guint            property_id,
                                 const GValue    *value,
                                 GParamSpec      *pspec)
{
  switch (property_id)
  {
      case PROP_MODEL:
          hd_app_menu_dialog_set_model (HD_APP_MENU_DIALOG (object),
                                        g_value_get_object (value));
          break;
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
  }

}

static void
hd_app_menu_dialog_get_property (GObject         *object,
                                 guint            property_id,
                                 GValue          *value,
                                 GParamSpec      *pspec)
{
  switch (property_id)
  {
      case PROP_MODEL:
          g_value_set_object (value,
                              HD_APP_MENU_DIALOG (object)->priv->model);
          break;
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
  }
}

static void
hd_app_menu_dialog_response (GtkDialog *dialog, gint response)
{
  switch (response)
  {
      case RESPONSE_NEW:
          hd_app_menu_dialog_new_category (HD_APP_MENU_DIALOG (dialog));
          break;
      case RESPONSE_DELETE:
          hd_app_menu_dialog_delete_category (HD_APP_MENU_DIALOG (dialog));
          break;
      case RESPONSE_RENAME:
          hd_app_menu_dialog_rename_category (HD_APP_MENU_DIALOG (dialog));
          break;
      case GTK_RESPONSE_CANCEL:
          gtk_widget_hide (GTK_WIDGET (dialog));
          set_menu_contents (HD_APP_MENU_DIALOG (dialog)->priv->model);
          break;
  }

}

static void
hd_app_menu_dialog_new_category (HDAppMenuDialog *dialog)
{
  GtkWidget            *new_dialog;
  GtkWidget            *entry;
  GtkWidget            *caption;
  GtkSizeGroup         *size_group;
  gint                  response;
  const gchar          *name;

  new_dialog = gtk_dialog_new_with_buttons (HD_APP_MENU_DIALOG_NEW_TITLE,
                                            GTK_WINDOW (dialog),
                                            GTK_DIALOG_MODAL |
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_OK,
                                              GTK_RESPONSE_OK,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_CANCEL,
                                              NULL);
  gtk_dialog_set_has_separator (GTK_DIALOG (new_dialog), FALSE);

  entry = gtk_entry_new ();
  gtk_widget_show (entry);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  caption = hildon_caption_new (size_group,
                                HD_APP_MENU_DIALOG_NEW_NAME,
                                entry,
                                NULL,
                                HILDON_CAPTION_OPTIONAL);
  gtk_widget_show (caption);

  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (new_dialog)->vbox),
                    caption,
                    FALSE,
                    FALSE,
                    0);

  response = gtk_dialog_run (GTK_DIALOG (new_dialog));

  name = gtk_entry_get_text (GTK_ENTRY (entry));

  if (response == GTK_RESPONSE_OK && GTK_IS_TREE_STORE (dialog->priv->model))
  {
    GtkTreeIter         iter;
    GdkPixbuf          *icon;

    if (g_utf8_strlen (name, -1) == 0)
      return;

    icon = get_icon (ICON_FOLDER, ICON_SIZE);

    gtk_tree_store_append (GTK_TREE_STORE (dialog->priv->model), &iter, NULL);

    gtk_tree_store_set (GTK_TREE_STORE (dialog->priv->model), &iter,
                        TREE_MODEL_NAME, g_strdup (name),
                        TREE_MODEL_LOCALIZED_NAME, g_strdup (name),
                        TREE_MODEL_ICON, icon,
                        -1);

    g_object_unref (icon);

  }

  gtk_widget_destroy (new_dialog);

}

static void
hd_app_menu_dialog_rename_category (HDAppMenuDialog *dialog)
{
  GtkWidget            *new_dialog;
  GtkWidget            *entry;
  GtkWidget            *caption;
  GtkSizeGroup         *size_group;
  gint                  response;
  const gchar          *name;
  GtkTreeIter           iter;

  if (!hd_app_menu_tree_get_selected_category (HD_APP_MENU_TREE (dialog->priv->tree),
                                               &iter))
    return;

  gtk_tree_model_get (dialog->priv->model, &iter,
                      TREE_MODEL_LOCALIZED_NAME, &name,
                      -1);

  new_dialog = gtk_dialog_new_with_buttons (HD_APP_MENU_DIALOG_RENAME_TITLE,
                                            GTK_WINDOW (dialog),
                                            GTK_DIALOG_MODAL |
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_OK,
                                              GTK_RESPONSE_OK,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_CANCEL,
                                              NULL);
  gtk_dialog_set_has_separator (GTK_DIALOG (new_dialog), FALSE);

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), name);
  gtk_widget_show (entry);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  caption = hildon_caption_new (size_group,
                                HD_APP_MENU_DIALOG_NEW_NAME,
                                entry,
                                NULL,
                                HILDON_CAPTION_OPTIONAL);
  gtk_widget_show (caption);

  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (new_dialog)->vbox),
                    caption,
                    FALSE,
                    FALSE,
                    0);

  response = gtk_dialog_run (GTK_DIALOG (new_dialog));

  name = gtk_entry_get_text (GTK_ENTRY (entry));

  if (response == GTK_RESPONSE_OK && GTK_IS_TREE_STORE (dialog->priv->model))
  {
    if (g_utf8_strlen (name, -1) == 0)
    {
      /* FIXME: Show banner foo*/
      return;
    }

    gtk_tree_store_set (GTK_TREE_STORE (dialog->priv->model), &iter,
                        TREE_MODEL_NAME, name,
                        TREE_MODEL_LOCALIZED_NAME, name,
                        -1);

  }

  gtk_widget_destroy (new_dialog);

}

static void
hd_app_menu_dialog_delete_category (HDAppMenuDialog *dialog)
{
  GtkTreeIter           iter;

  if (!hd_app_menu_tree_get_selected_category (HD_APP_MENU_TREE (dialog->priv->tree),
                                               &iter))
    return;

  if (gtk_tree_model_iter_has_child (dialog->priv->model, &iter))
  {
    hildon_banner_show_information (GTK_WIDGET (dialog),
                                    NULL,
                                    HD_APP_MENU_DIALOG_ONLY_EMPTY);
    return;
  }

  gtk_tree_store_remove (GTK_TREE_STORE (dialog->priv->model), &iter);

}

static void
hd_app_menu_dialog_item_selected (HDAppMenuDialog      *dialog,
                                  GtkTreeIter          *iter)
{
  HDAppMenuDialogPrivate       *priv;

  priv = dialog->priv;

  if (iter != NULL)
  {
    gtk_widget_set_sensitive (priv->delete_button, FALSE);
    gtk_widget_set_sensitive (priv->rename_button, FALSE);
  }
  else
  {
    gtk_widget_set_sensitive (priv->delete_button, TRUE);
    gtk_widget_set_sensitive (priv->rename_button, TRUE);
  }
}

void
hd_app_menu_dialog_set_model (HDAppMenuDialog  *dialog,
                              GtkTreeModel     *model)
{
  HDAppMenuDialogPrivate *priv;

  g_return_if_fail (HD_IS_APP_MENU_DIALOG (dialog));
  g_return_if_fail (!model || GTK_IS_TREE_MODEL (model));

  priv = dialog->priv;

  if (priv->model)
    g_object_unref (priv->model);

  priv->model = model;

  if (priv->model)
    g_object_ref (priv->model);

  hd_app_menu_tree_set_model (HD_APP_MENU_TREE (priv->tree), model);

  g_object_notify (G_OBJECT (dialog), "model");

}
