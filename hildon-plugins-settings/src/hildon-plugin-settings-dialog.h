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

#ifndef __HILDON_PLUGIN_SETTINGS_DIALOG_H__
#define __HILDON_PLUGIN_SETTINGS_DIALOG_H__

#include <gtk/gtkdialog.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreemodelfilter.h>

G_BEGIN_DECLS

typedef struct _HildonPluginSettingsDialog HildonPluginSettingsDialog;
typedef struct _HildonPluginSettingsDialogClass HildonPluginSettingsDialogClass;
typedef struct _HildonPluginSettingsDialogPrivate HildonPluginSettingsDialogPrivate;

#define HILDON_PLUGIN_TYPE_SETTINGS_DIALOG ( hildon_plugin_settings_dialog_get_type() )
#define HILDON_PLUGIN_SETTINGS_DIALOG(obj) (GTK_CHECK_CAST (obj, HILDON_PLUGIN_TYPE_SETTINGS_DIALOG, HildonPluginSettingsDialog))
#define HILDON_PLUGIN_SETTINGS_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), HILDON_PLUGIN_TYPE_SETTINGS_DIALOG, HildonPluginSettingsDialogClass))
#define HILDON_PLUGIN_IS_SETTINGS_DIALOG(obj) (GTK_CHECK_TYPE (obj, HILDON_PLUGIN_TYPE_SETTINGS_DIALOG))
#define HILDON_PLUGIN_IS_SETTINGS_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), HILDON_PLUGIN_TYPE_SETTINGS_DIALOG))

struct _HildonPluginSettingsDialog
{
  GtkDialog parent;

  GtkTreeModel *sbtm;
  GtkTreeModel *tntm;

  HildonPluginSettingsDialogPrivate *priv;
};

struct _HildonPluginSettingsDialogClass
{
  GtkDialogClass parent_class;

  /* */
};

GType 
hildon_plugin_settings_dialog_get_type (void);

GtkWidget *
hildon_plugin_settings_dialog_new (void);

GtkTreeModel *
hildon_plugin_settings_dialog_set_visibility_filter (HildonPluginSettingsDialog *settings,
					             const gchar *container_name,
					             GtkTreeModelFilterVisibleFunc visible_func,
					             gpointer data,
						     GtkDestroyNotify destroy);

GtkTreeModel *
hildon_plugin_settings_dialog_set_modify_filter (HildonPluginSettingsDialog *settings,
                                                 const gchar *container_name,
                                                 gint n_columns,
                                                 GType *types,
                                                 GtkTreeModelFilterModifyFunc modify_func,
                                                 gpointer data,
                                                 GtkDestroyNotify destroy);

G_END_DECLS

#endif/*__HILDON_PLUGIN_SETTINGS_DIALOG_H__*/
