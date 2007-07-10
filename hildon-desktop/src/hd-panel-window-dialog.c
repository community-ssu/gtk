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
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib-bindings.h>

#include <libhildondesktop/hildon-desktop-panel-expandable.h>
#include <libhildonwm/hd-wm.h>
#include "hd-panel-window-dialog.h"
#include "hd-panel-window-dialog-service.h"

#define HD_PANEL_WINDOW_DIALOG_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HD_TYPE_PANEL_WINDOW_DIALOG, HDPanelWindowDialogPrivate))

G_DEFINE_TYPE (HDPanelWindowDialog, hd_panel_window_dialog, HILDON_DESKTOP_TYPE_PANEL_WINDOW_DIALOG);

struct _HDPanelWindowDialogPrivate
{
  DBusGConnection *connection;
};

#undef HD_PANEL_WINDOW_DIALOG_NEW_THEME

#define STATUSBAR_DBUS_NAME  "org.hildon.Statusbar" 
#define STATUSBAR_DBUS_PATH  "/org/hildon/Statusbar"

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
hd_panel_window_dialog_notify_condition (GObject *object, 
		                         gboolean condition, 
					 gpointer user_data)
{
  HDPanelWindowDialog *window;
  HildonDesktopItem *item;
  DBusMessage *message;
  const gchar *id;
  
  window = HD_PANEL_WINDOW_DIALOG (user_data);
  item = HILDON_DESKTOP_ITEM (object);
  
  id = hildon_desktop_item_get_id (item);
  
  message = dbus_message_new_signal ("/org/hildon/Statusbar",
                                     "org.hildon.Statusbar",
                                     "update-status");

  dbus_message_append_args (message,
                            DBUS_TYPE_STRING, &id,
                            DBUS_TYPE_BOOLEAN, &condition,
                            DBUS_TYPE_INVALID);

  dbus_connection_send (dbus_g_connection_get_connection (window->priv->connection), 
        	        message, 
        		NULL);
  
  dbus_message_unref (message);
}

static void
hd_panel_window_dialog_cadd (HildonDesktopPanelExpandable *container,
		      GtkWidget *widget,
		      gpointer user_data)
{
  gtk_widget_set_name (widget, HD_PANEL_WINDOW_DIALOG_BUTTON_NAME);
  gtk_widget_set_name (GTK_BIN (widget)->child, HD_PANEL_WINDOW_DIALOG_BUTTON_NAME);

  g_signal_connect (G_OBJECT (widget),
		    "notify::condition",
		    G_CALLBACK (hd_panel_window_dialog_notify_condition),
		    user_data);
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
                    object);

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

  g_type_class_add_private (window_class, sizeof (HDPanelWindowDialogPrivate));
}

static void
hd_panel_window_dialog_init (HDPanelWindowDialog *window)
{
  DBusGProxy *bus_proxy;
  GError *error = NULL;
  guint result;
  
  window->priv = HD_PANEL_WINDOW_DIALOG_GET_PRIVATE (window);

#ifndef HD_PANEL_WINDOW_DIALOG_NEW_THEME
  gtk_widget_set_name (GTK_WIDGET (window), "HildonStatusBar");
#endif

  window->priv->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (error != NULL)
  {
    g_warning ("Failed to open connection to bus: %s\n",
               error->message);
    
    g_error_free (error);

    return;
  }

  bus_proxy = dbus_g_proxy_new_for_name (window->priv->connection,
  					 DBUS_SERVICE_DBUS,
  					 DBUS_PATH_DBUS,
  					 DBUS_INTERFACE_DBUS);
 
  if (!org_freedesktop_DBus_request_name (bus_proxy,
                                          STATUSBAR_DBUS_NAME,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &result, 
                                          &error))
  {
    g_warning ("Could not register name: %s", error->message);

    g_error_free (error);
    
    return;
  }

  g_object_unref (bus_proxy);

  if (result == DBUS_REQUEST_NAME_REPLY_EXISTS) return;
  
  dbus_g_object_type_install_info (HD_TYPE_PANEL_WINDOW_DIALOG,
                                   &dbus_glib_hd_panel_window_dialog_service_object_info);
 
  dbus_g_connection_register_g_object (window->priv->connection,
                                       STATUSBAR_DBUS_PATH,
                                       G_OBJECT (window));
}

gboolean
hd_panel_window_dialog_refresh_items_status (HDPanelWindowDialog *window)
{
  HildonDesktopPanel *panel;

  panel = HILDON_DESKTOP_PANEL (HILDON_DESKTOP_WINDOW (window)->container);

  hildon_desktop_panel_refresh_items_status (panel);

  g_debug ("=======================================================");
  g_debug ("                   REFRESH ITEMS STATUS                ");
  g_debug ("=======================================================");
  
  return TRUE;
}
