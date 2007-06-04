/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005 Nokia Corporation.  All rights reserved.
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

void hildon_file_chooser_dialog_focus_to_input(HildonFileChooserDialog *d);

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

void     hildon_file_chooser_dialog_set_show_upnp (HildonFileChooserDialog *self, gboolean value);
gboolean hildon_file_chooser_dialog_get_show_upnp (HildonFileChooserDialog *self);

/* Add WIDGET to the dialog, below the "Name" and "Location" fields.
   When WIDGET is a HildonCaption, care is taken that the labels line
   up with existing HildonCaptions.
*/
void hildon_file_chooser_dialog_add_extra (HildonFileChooserDialog *self,
					   GtkWidget *widget);

/* Create and add a combo box widget with a list of file extensions.
   This combobox will track and modify the extension of the current
   filename, it is not a filter.

   EXTENSIONS should be a vector of strings, terminated by NULL.  The
   strings in it are the extensions, without a leading '.'.

   EXT_NAMES, when non-NULL, is a vector parallel to EXTENSIONS that
   determines the names of the extensions to use in the UI.  When
   EXT_NAMES is NULL, the exentions themselved are used as the names.
*/
GtkWidget *
hildon_file_chooser_dialog_add_extensions_combo (HildonFileChooserDialog *self,
						 char **extensions,
						 char **ext_names);

/* 

    Note! Other functionality is provided by GtkFileChooser interface. See:
    
    http://developer.gnome.org/doc/API/2.0/gtk/GtkFileChooser.html 
*/

G_END_DECLS
#endif
