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

#ifndef __HILDON_DESKTOP_PANEL_WINDOW_DIALOG_H__
#define __HILDON_DESKTOP_PANEL_WINDOW_DIALOG_H__

#include <gtk/gtkwindow.h>

#include <libhildondesktop/hildon-desktop-panel-window-composite.h>

G_BEGIN_DECLS

typedef struct _HildonDesktopPanelWindowDialog HildonDesktopPanelWindowDialog;
typedef struct _HildonDesktopPanelWindowDialogClass HildonDesktopPanelWindowDialogClass;
typedef struct _HildonDesktopPanelWindowDialogPrivate HildonDesktopPanelWindowDialogPrivate;

#define HILDON_DESKTOP_TYPE_PANEL_WINDOW_DIALOG            (hildon_desktop_panel_window_dialog_get_type())
#define HILDON_DESKTOP_PANEL_WINDOW_DIALOG(obj)            (GTK_CHECK_CAST (obj, HILDON_DESKTOP_TYPE_PANEL_WINDOW_DIALOG, HildonDesktopPanelWindowDialog))
#define HILDON_DESKTOP_PANEL_WINDOW_DIALOG_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), HILDON_DESKTOP_TYPE_PANEL_WINDOW_DIALOG, HildonDesktopPanelWindowDialogClass))
#define HILDON_DESKTOP_IS_PANEL_WINDOW_DIALOG(obj)         (GTK_CHECK_TYPE (obj, HILDON_DESKTOP_TYPE_PANEL_WINDOW_DIALOG))
#define HILDON_DESKTOP_IS_PANEL_WINDOW_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), HILDON_DESKTOP_TYPE_PANEL_WINDOW_DIALOG))

struct _HildonDesktopPanelWindowDialog
{
  HildonDesktopPanelWindowComposite parent;

  HildonDesktopPanelWindowDialogPrivate  *priv;
};

struct _HildonDesktopPanelWindowDialogClass
{
  HildonDesktopPanelWindowCompositeClass parent_class;
  
};

GType                hildon_desktop_panel_window_dialog_get_type       (void);

GtkWidget 	    *hildon_desktop_panel_window_dialog_new (void);

G_END_DECLS

#endif /* __HILDON_DESKTOP_PANEL_WINDOW_DIALOG_H__ */
