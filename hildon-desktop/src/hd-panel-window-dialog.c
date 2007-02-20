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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <libhildondesktop/hildon-desktop-panel-expandable.h>
#include <libhildonwm/hd-wm.h>
#include "hd-panel-window-dialog.h"

#define HD_PANEL_WINDOW_DIALOG_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HD_TYPE_PANEL_WINDOW, HDPanelWindowDialogPrivate))

G_DEFINE_TYPE (HDPanelWindowDialog, hd_panel_window_dialog, HILDON_DESKTOP_TYPE_PANEL_WINDOW_DIALOG);

#undef HD_PANEL_WINDOW_DIALOG_NEW_THEME

#define HD_PANEL_WINDOW_DIALOG_NAME_TOP     "hildon-status-bar-panel-top"
#define HD_PANEL_WINDOW_DIALOG_NAME_BOTTOM  "hildon-status-bar-panel-bottom"
#define HD_PANEL_WINDOW_DIALOG_NAME_LEFT    "hildon-status-bar-panel-left"
#define HD_PANEL_WINDOW_DIALOG_NAME_RIGHT   "hildon-status-bar-panel-right"
#define HD_PANEL_WINDOW_DIALOG_BUTTON_NAME  "HildonStatusBarItem"

#ifdef HD_PANEL_WINDOW_DIALOG_NEW_THEME
static void
hd_panel_window_dialog_set_style (HDPanelWindowDialog *window, 
                           HildonDesktopPanelWindowOrientation orientation)
{
  switch (orientation)
  {
    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP:
      gtk_widget_set_name (GTK_WIDGET (window), HD_PANEL_WINDOW_DIALOG_NAME_TOP);
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM:
      gtk_widget_set_name (GTK_WIDGET (window), HD_PANEL_WINDOW_DIALOG_NAME_BOTTOM);
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT:
      gtk_widget_set_name (GTK_WIDGET (window), HD_PANEL_WINDOW_DIALOG_NAME_LEFT);
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT:
      gtk_widget_set_name (GTK_WIDGET (window), HD_PANEL_WINDOW_DIALOG_NAME_RIGHT);
      break;

    default:
      gtk_widget_set_name (GTK_WIDGET (window), HD_PANEL_WINDOW_DIALOG_NAME_TOP);
      break;
  }
}

static void
hd_panel_window_dialog_orientation_changed (HildonDesktopPanelWindow *window, 
                                     HildonDesktopPanelWindowOrientation new_orientation)
{
  hd_panel_window_dialog_set_style (HD_PANEL_WINDOW_DIALOG (window), new_orientation);
}
#endif

static void
hd_panel_window_dialog_cadd (HildonDesktopPanelExpandable *container,
		      GtkWidget *widget,
		      gpointer user_data)
{
  gtk_widget_set_name (widget, HD_PANEL_WINDOW_DIALOG_BUTTON_NAME);
  gtk_widget_set_name (GTK_BIN (widget)->child, HD_PANEL_WINDOW_DIALOG_BUTTON_NAME);
}

static void 
hd_panel_window_dialog_fullscreen (HDWM *hdwm, gboolean fullscreen, gpointer _data)
{
  GtkWidget *window = GTK_WIDGET (_data);

  if (!fullscreen)
    gtk_widget_show (window);
  else
    gtk_widget_hide (window);
}

static GObject *
hd_panel_window_dialog_constructor (GType gtype,
                             guint n_params,
                             GObjectConstructParam  *params)
{
  GObject *object;
#ifdef HD_PANEL_WINDOW_DIALOG_NEW_THEME
  HildonDesktopPanelWindowOrientation orientation;
#endif

  HDWM *hdwm = hd_wm_get_singleton ();
  
  object = G_OBJECT_CLASS (hd_panel_window_dialog_parent_class)->constructor (gtype,
                                                                       n_params,
                                                                       params);

  g_signal_connect (G_OBJECT (HILDON_DESKTOP_WINDOW (object)->container), 
                    "queued-button",
                    G_CALLBACK (hd_panel_window_dialog_cadd),
                    NULL);

  g_signal_connect (G_OBJECT (hdwm),
		    "fullscreen",
		    G_CALLBACK (hd_panel_window_dialog_fullscreen),
		    (gpointer)object);

#ifdef HD_PANEL_WINDOW_DIALOG_NEW_THEME
  g_object_get (G_OBJECT (object), 
                "orientation", &orientation,
                NULL);
  
  hd_panel_window_dialog_set_style (HD_PANEL_WINDOW_DIALOG (object), orientation);
#endif
  
  return object;
}
 
static void
hd_panel_window_dialog_class_init (HDPanelWindowDialogClass *window_class)
{
  GObjectClass *object_class;
  HildonDesktopPanelWindowClass *panel_window_class;
  
  object_class = G_OBJECT_CLASS (window_class);
  panel_window_class = HILDON_DESKTOP_PANEL_WINDOW_CLASS (window_class);
  
  object_class->constructor = hd_panel_window_dialog_constructor;

#ifdef HD_PANEL_WINDOW_DIALOG_NEW_THEME
  panel_window_class->orientation_changed = hd_panel_window_dialog_orientation_changed;
#endif
}

static void
hd_panel_window_dialog_init (HDPanelWindowDialog *window)
{
#ifndef HD_PANEL_WINDOW_DIALOG_NEW_THEME
  gtk_widget_set_name (GTK_WIDGET (window), "HildonStatusBar");
#endif
}
