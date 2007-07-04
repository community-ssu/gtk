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

#ifndef __HD_APPLICATIONS_MENU_SETTINGS_H__
#define __HD_APPLICATIONS_MENU_SETTINGS_H__

#include <gtk/gtkdialog.h>
#include <gtk/gtktreemodel.h>

G_BEGIN_DECLS

typedef struct _HDAppMenuDialog HDAppMenuDialog;
typedef struct _HDAppMenuDialogClass HDAppMenuDialogClass;
typedef struct _HDAppMenuDialogPrivate HDAppMenuDialogPrivate;

#define HD_TYPE_APP_MENU_DIALOG            (hd_app_menu_dialog_get_type ())
#define HD_APP_MENU_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_APP_MENU_DIALOG, HDAppMenuDialog))
#define HD_APP_MENU_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_APP_MENU_DIALOG, HDAppMenuDialogClass))
#define HD_IS_APP_MENU_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_APP_MENU_DIALOG))
#define HD_IS_APP_MENU_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_APP_MENU_DIALOG))
#define HD_APP_MENU_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_APP_MENU_DIALOG, HDAppMenuDialogClass))

struct _HDAppMenuDialog
{
  GtkDialog                     parent;

  HDAppMenuDialogPrivate       *priv;
};

struct _HDAppMenuDialogClass
{
  GtkDialogClass                parent_class;
};

GType   hd_app_menu_dialog_get_type (void);
void    hd_app_menu_dialog_set_model (HDAppMenuDialog  *dialog,
                                      GtkTreeModel     *model);

G_END_DECLS

#endif /* __HD_APPLICATIONS_MENU_SETTINGS_H__*/
