/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
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

#ifndef __HD_HOME_BACKGROUND_DIALOG_H__
#define __HD_HOME_BACKGROUND_DIALOG_H__

#include <gtk/gtkdialog.h>
#include "hd-home-background.h"

G_BEGIN_DECLS

typedef struct _HDHomeBackgroundDialog HDHomeBackgroundDialog;
typedef struct _HDHomeBackgroundDialogClass HDHomeBackgroundDialogClass;
typedef struct _HDHomeBackgroundDialogPrivate HDHomeBackgroundDialogPrivate;

#define HD_TYPE_HOME_BACKGROUND_DIALOG            (hd_home_background_dialog_get_type ())
#define HD_HOME_BACKGROUND_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_HOME_BACKGROUND_DIALOG, HDHomeBackgroundDialog))
#define HD_HOME_BACKGROUND_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_HOME_BACKGROUND_DIALOG, HDHomeBackgroundDialogClass))
#define HD_IS_HOME_BACKGROUND_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_HOME_BACKGROUND_DIALOG))
#define HD_IS_HOME_BACKGROUND_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_HOME_BACKGROUND_DIALOG))
#define HD_HOME_BACKGROUND_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_HOME_BACKGROUND_DIALOG, HDHomeBackgroundDialogClass))

#define HILDON_HOME_SET_BG_RESPONSE_IMAGE    GTK_RESPONSE_YES
#define HILDON_HOME_SET_BG_RESPONSE_PREVIEW  GTK_RESPONSE_APPLY


struct _HDHomeBackgroundDialog
{
  GtkDialog parent;

};

struct _HDHomeBackgroundDialogClass
{
  GtkDialogClass parent_class;

  void (* background_dir_changed)       (HDHomeBackgroundDialog *dialog);

};

GType  hd_home_background_dialog_get_type  (void);

HildonDesktopBackground *       hd_home_background_dialog_get_background
                                            (HDHomeBackgroundDialog *dialog);

G_END_DECLS

#endif /* __HD_HOME_BACKGROUND_DIALOG_H__ */
