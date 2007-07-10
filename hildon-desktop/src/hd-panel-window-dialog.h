/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#ifndef __HD_PANEL_WINDOW_DIALOG_H__
#define __HD_PANEL_WINDOW_DIALOG_H__

#include <glib-object.h>
#include <libhildondesktop/hildon-desktop-panel-window-dialog.h>

G_BEGIN_DECLS

typedef struct _HDPanelWindowDialog HDPanelWindowDialog;
typedef struct _HDPanelWindowDialogClass HDPanelWindowDialogClass;
typedef struct _HDPanelWindowDialogPrivate HDPanelWindowDialogPrivate;

#define HD_TYPE_PANEL_WINDOW_DIALOG            (hd_panel_window_dialog_get_type ())
#define HD_PANEL_WINDOW_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_PANEL_WINDOW_DIALOG, HDPanelWindowDialog))
#define HD_PANEL_WINDOW_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_PANEL_WINDOW_DIALOG, HDPanelWindowDialogClass))
#define HD_IS_PANEL_WINDOW_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_PANEL_WINDOW_DIALOG))
#define HD_IS_PANEL_WINDOW_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_PANEL_WINDOW_DIALOG))
#define HD_PANEL_WINDOW_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_PANEL_WINDOW_DIALOG, HDPanelWindowDialogClass))

struct _HDPanelWindowDialog 
{
  HildonDesktopPanelWindowDialog parent;

  HDPanelWindowDialogPrivate *priv;
};

struct _HDPanelWindowDialogClass
{
  HildonDesktopPanelWindowDialogClass parent_class;
};

GType     hd_panel_window_dialog_get_type               (void);

gboolean  hd_panel_window_dialog_refresh_items_status   (HDPanelWindowDialog *window);

G_END_DECLS

#endif /*__HD_PANEL_WINDOW_DIALOG_H__*/
