/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
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

#ifndef __HILDON_FILE_CHOOSER_DIALOG_H__
#define __HILDON_FILE_CHOOSER_DIALOG_H__

#include <gtk/gtkdialog.h>
#include <gtk/gtkfilechooser.h>

G_BEGIN_DECLS
#define HILDON_TYPE_FILE_CHOOSER_DIALOG \
  ( hildon_file_chooser_dialog_get_type() )
#define HILDON_FILE_CHOOSER_DIALOG(obj) \
  (GTK_CHECK_CAST (obj, HILDON_TYPE_FILE_CHOOSER_DIALOG,\
   HildonFileChooserDialog))
#define HILDON_FILE_CHOOSER_DIALOG_CLASS(klass) \
  (GTK_CHECK_CLASS_CAST ((klass), HILDON_TYPE_FILE_CHOOSER_DIALOG, \
  HildonFileChooserDialogClass))
#define HILDON_IS_FILE_CHOOSER_DIALOG(obj) \
  (GTK_CHECK_TYPE (obj, HILDON_TYPE_FILE_CHOOSER_DIALOG))
#define HILDON_IS_FILE_CHOOSER_DIALOG_CLASS(klass) \
  (GTK_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_FILE_CHOOSER_DIALOG))
typedef struct _HildonFileChooserDialog HildonFileChooserDialog;
typedef struct _HildonFileChooserDialogClass HildonFileChooserDialogClass;

/**
 * HildonFileChooserDialogPrivate:
 *
 * This structure contains just private data and should not be accessed
 * directly.
 */
typedef struct _HildonFileChooserDialogPrivate
    HildonFileChooserDialogPrivate;

struct _HildonFileChooserDialog {
    GtkDialog parent;
    HildonFileChooserDialogPrivate *priv;
};

struct _HildonFileChooserDialogClass {
    GtkDialogClass parent_class;
};

GType hildon_file_chooser_dialog_get_type(void);

GtkWidget *hildon_file_chooser_dialog_new(GtkWindow * parent,
                                          GtkFileChooserAction action);
GtkWidget *hildon_file_chooser_dialog_new_with_properties(GtkWindow *
                                                          parent,
                                                          const gchar *
                                                          first_property,
                                                          ...);

/* These are not properties, because similar functions in
    for current folders are functions only in GtkFileChooser */
void hildon_file_chooser_dialog_set_safe_folder(
  HildonFileChooserDialog *self, const gchar *local_path);
void hildon_file_chooser_dialog_set_safe_folder_uri(
  HildonFileChooserDialog *self, const gchar *uri);
gchar *hildon_file_chooser_dialog_get_safe_folder(
  HildonFileChooserDialog *self);
gchar *hildon_file_chooser_dialog_get_safe_folder_uri(
  HildonFileChooserDialog *self);

/* 
    Note! Other functionality is provided by GtkFileChooser interface. See:
    
    http://developer.gnome.org/doc/API/2.0/gtk/GtkFileChooser.html 
*/

G_END_DECLS
#endif
