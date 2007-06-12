/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005-2007 Nokia Corporation.  All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
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

#ifndef __HILDON_FILE_SYSTEM_STORAGE_DIALOG_H__
#define __HILDON_FILE_SYSTEM_STORAGE_DIALOG_H__

#include <gtk/gtkdialog.h>

G_BEGIN_DECLS

#define HILDON_TYPE_FILE_SYSTEM_STORAGE_DIALOG         (hildon_file_system_storage_dialog_get_type ())
#define HILDON_FILE_SYSTEM_STORAGE_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), HILDON_TYPE_FILE_SYSTEM_STORAGE_DIALOG, HildonFileSystemStorageDialog))
#define HILDON_FILE_SYSTEM_STORAGE_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), HILDON_TYPE_FILE_SYSTEM_STORAGE_DIALOG, HildonFileSystemStorageDialogClass))
#define HILDON_IS_FILE_SYSTEM_STORAGE_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), HILDON_TYPE_FILE_SYSTEM_STORAGE_DIALOG))
#define HILDON_IS_FILE_SYSTEM_STORAGE_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), HILDON_TYPE_FILE_SYSTEM_STORAGE_DIALOG))
#define HILDON_FILE_SYSTEM_STORAGE_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), HILDON_TYPE_FILE_SYSTEM_STORAGE_DIALOG, HildonFileSystemStorageDialogClass))

typedef struct _HildonFileSystemStorageDialog      HildonFileSystemStorageDialog;
typedef struct _HildonFileSystemStorageDialogClass HildonFileSystemStorageDialogClass;
typedef struct _HildonFileSystemStorageDialogPriv  HildonFileSystemStorageDialogPriv;

struct _HildonFileSystemStorageDialog {
	GtkDialog      parent;
};

struct _HildonFileSystemStorageDialogClass {
	GtkDialogClass parent_class;
};

GType      hildon_file_system_storage_dialog_get_type (void) G_GNUC_CONST;
GtkWidget *hildon_file_system_storage_dialog_new      (GtkWindow   *parent,
						       const gchar *uri_str);
void       hildon_file_system_storage_dialog_set_uri  (GtkWidget   *widget,
						       const gchar *uri_str);

G_END_DECLS

#endif /* __HILDON_FILE_SYSTEM_STORAGE_DIALOG_H__ */

