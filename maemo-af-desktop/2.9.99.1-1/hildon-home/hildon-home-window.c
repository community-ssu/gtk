/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
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
#include "config.h"
#endif

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <libintl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libosso.h>
#include <libmb/mbutil.h>
#include <glob.h>
#include <osso-log.h>
#include <osso-helplib.h>

#include <glib.h>

#include <libgnomevfs/gnome-vfs.h>

#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-color-button.h>

#include "background-manager.h"
#include "home-applet-handler.h"

#include "hildon-home-area.h"
#include "hildon-home-titlebar.h"
#include "hildon-home-common.h"
#include "hildon-home-private.h"
#include "hildon-home-interface.h"
#include "hildon-home-window.h"
#include "hildon-plugin-list.h"
#include "hildon-home-background-dialog.h"
#include "hildon-home-select-applets-dialog.h"

#define HILDON_HOME_LEFT_PADDING	80	/* keep synced with the TN width */
#define HILDON_HOME_WINDOW_WIDTH	800
#define HILDON_HOME_WINDOW_HEIGHT	480
#define HILDON_HOME_TOP_BAR_HEIGHT	60

#define LAYOUT_OPENING_BANNER_TIMEOUT 2500

#define HH_AREA_CONFIGURATION_FILE      ".osso/hildon-home/applet_manager.conf"
#define HH_AREA_GLOBAL_CONFIGURATION_FILE "/etc/hildon-home/applet_manager.conf"
#define HH_DESKTOP_FILES_DIR DATADIR"/applications/hildon-home"


/* DBUS defines */
#define STATUSBAR_SERVICE_NAME          "statusbar"
#define STATUSBAR_INSENSITIVE_METHOD    "statusbar_insensitive"
#define STATUSBAR_SENSITIVE_METHOD      "statusbar_sensitive"

#define TASKNAV_SERVICE_NAME            "com.nokia.tasknav"
#define TASKNAV_GENERAL_PATH            "/com/nokia/tasknav"
#define TASKNAV_INSENSITIVE_INTERFACE \
        TASKNAV_SERVICE_NAME "." TASKNAV_INSENSITIVE_METHOD
#define TASKNAV_SENSITIVE_INTERFACE \
        TASKNAV_SERVICE_NAME "." TASKNAV_SENSITIVE_METHOD
#define TASKNAV_INSENSITIVE_METHOD      "tasknav_insensitive"
#define TASKNAV_SENSITIVE_METHOD        "tasknav_sensitive"

#define HILDON_HOME_WINDOW_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_TYPE_HOME_WINDOW, HildonHomeWindowPrivate))

struct _HildonHomeWindowPrivate
{
  BackgroundManager *bg_manager;
  HildonPluginList  *plugin_list;
  osso_context_t *osso_context;

  GtkWidget *align;
  
  GtkWidget *titlebar;
  GtkWidget *applet_area;
  GtkWidget *main_area;

  GtkWidget *layout_mode_banner;
  guint      layout_mode_banner_to;

  guint is_dimmed : 1;
  guint is_lowmem : 1;
  guint is_inactive : 1;
};

G_DEFINE_TYPE (HildonHomeWindow, hildon_home_window, GTK_TYPE_WINDOW);

enum
{
  PROP_0,

  PROP_OSSO_CONTEXT
};

static gboolean
destroy_banner (HildonHomeWindow *window)
{
  HildonHomeWindowPrivate *priv = window->priv;

  if (priv->layout_mode_banner)
    {
      gtk_widget_destroy (priv->layout_mode_banner);
      priv->layout_mode_banner = NULL;
      g_signal_handlers_disconnect_by_func (G_OBJECT (priv->applet_area),
                                            G_CALLBACK (destroy_banner),
                                            window);
    }
  priv->layout_mode_banner_to = 0;

  return FALSE;
}

static void
hildon_home_window_show_layout_mode_banner (HildonHomeWindow *window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  
  if (!priv->layout_mode_banner)
    {
      priv->layout_mode_banner = 
          hildon_banner_show_animation (GTK_WIDGET (window),
                                        NULL,
                                        LAYOUT_MODE_NOTIFICATION_MODE_BEGIN_TEXT);
      priv->layout_mode_banner_to = 
          g_timeout_add (LAYOUT_OPENING_BANNER_TIMEOUT,
                         (GSourceFunc)destroy_banner,
                         window);

      g_signal_connect_swapped (G_OBJECT (priv->applet_area),
                                "applet-change-start",
                                G_CALLBACK (destroy_banner),
                                window);


    }
    
}

static void
titlebar_select_applets_activate_cb (HildonHomeTitlebar *titlebar,
                                     HildonHomeWindow   *window)
{
  GtkWidget *dialog;
  gint response;
  HildonHomeWindowPrivate *priv = window->priv;
  g_debug ("select applets activate\n");

  if (priv->is_lowmem)
    {
      hildon_banner_show_information (NULL,
                                      NULL,
                                      HILDON_HOME_LOWMEM_TEXT);
      return;
    }

  hildon_home_area_sync_to_list (HILDON_HOME_AREA (priv->applet_area),
                                 priv->plugin_list);

  dialog = hildon_home_select_applets_dialog_new_with_model 
                                    (GTK_TREE_MODEL (priv->plugin_list),
                                     priv->osso_context);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  
  if (response != GTK_RESPONSE_OK)
    return;

  hildon_home_area_sync_from_list (
                          HILDON_HOME_AREA (priv->applet_area),
                          priv->plugin_list);

}

static void
titlebar_set_background_activate_cb (HildonHomeTitlebar *titlebar,
                                     HildonHomeWindow   *window)
{
  home_bgd_dialog_run (GTK_WINDOW (window));
}

static void
titlebar_layout_mode_activate_cb (HildonHomeTitlebar *titlebar,
				  HildonHomeWindow   *window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  g_debug ("layout mode activate");
  
  if (priv->is_lowmem)
    {
      hildon_banner_show_information (NULL,
                                      NULL,
                                      HILDON_HOME_LOWMEM_TEXT);
      return;
    }

  hildon_home_area_set_layout_mode (HILDON_HOME_AREA (priv->applet_area),
                                    TRUE);

}

static void
titlebar_layout_accept_cb (HildonHomeTitlebar *titlebar,
			   HildonHomeWindow   *window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  gchar *filename;
  g_debug ("layout accepted");
  
  if (hildon_home_area_get_overlaps (HILDON_HOME_AREA (priv->applet_area)))
    {
      hildon_home_window_show_information_note (window,
                                                LAYOUT_MODE_NOTIFICATION_MODE_ACCEPT_TEXT);
      return;
    }
  hildon_home_area_set_layout_mode (HILDON_HOME_AREA (priv->applet_area),
                                    FALSE);
  
  filename = g_build_filename (g_getenv ("HOME"),
                               HH_AREA_CONFIGURATION_FILE,
                               NULL);
  
  if (hildon_home_area_save_configuration (HILDON_HOME_AREA (priv->applet_area),
                                       filename) < 0)
    hildon_home_window_show_information_note (window,
                                              HILDON_HOME_FLASH_FULL_TEXT); 

  g_free (filename);
}

static void
titlebar_layout_cancel_cb (HildonHomeTitlebar *titlebar,
			   HildonHomeWindow   *window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  g_debug ("layout discarded");

  if (hildon_home_area_get_layout_changed 
      (HILDON_HOME_AREA (priv->applet_area)))
    {
      GtkWidget *note;
      gint response;
      gchar *user_filename = NULL; 
      gchar *filename = NULL;

      note = hildon_note_new_confirmation_add_buttons 
          (NULL,
           LAYOUT_MODE_NOTIFICATION_MODE_CANCEL_TEXT,
           LAYOUT_MODE_NOTIFICATION_MODE_CANCEL_YES,
           GTK_RESPONSE_ACCEPT,
           LAYOUT_MODE_NOTIFICATION_MODE_CANCEL_NO,
           GTK_RESPONSE_CANCEL,
           NULL);

      response = gtk_dialog_run (GTK_DIALOG (note));
      gtk_widget_destroy (note);

      if (response != GTK_RESPONSE_ACCEPT)
        return;
  
      user_filename = g_build_filename (g_getenv ("HOME"),
                                        HH_AREA_CONFIGURATION_FILE,
                                        NULL);

      if (g_file_test (user_filename, G_FILE_TEST_EXISTS))
        filename = user_filename;
      else
        filename = HH_AREA_GLOBAL_CONFIGURATION_FILE;

      hildon_home_area_load_configuration (HILDON_HOME_AREA (priv->applet_area),
                                           filename);

      g_free (user_filename);
    }

  hildon_home_area_set_layout_mode (HILDON_HOME_AREA (priv->applet_area),
                                    FALSE);
}

static void
titlebar_applet_activate_cb (HildonHomeTitlebar *titlebar,
			     const gchar        *applet_path,
			     HildonHomeWindow   *window)
{
  osso_context_t *osso_ctx;
  osso_return_t res;

  osso_ctx = window->priv->osso_context;
  if (!osso_ctx)
    return;

  g_print ("osso_cp_plugin_execute: %s\n", applet_path);

  res = osso_cp_plugin_execute (osso_ctx,
                                applet_path,
		                        NULL,
                                TRUE);
  switch (res)
    {
    case OSSO_OK:
      break;
    case OSSO_ERROR:
      ULOG_ERR ("OSSO_ERROR (No plugin found)\n");
      break;
    case OSSO_RPC_ERROR:
      ULOG_ERR ("OSSO_RPC_ERROR (RPC failed for plugin)\n");
      break;
    case OSSO_INVALID:
      ULOG_ERR ("OSSO_INVALID (Invalid argument)\n");
      break;
    default:
      ULOG_ERR ("Unknown error!\n");
      break;
    }
}

static void
titlebar_help_activate_cb (HildonHomeTitlebar *titlebar,
		           HildonHomeWindow   *window)
{
  osso_context_t *osso_ctx;
  osso_return_t res;

  osso_ctx = window->priv->osso_context;
  if (!osso_ctx)
    return;

  if (hildon_home_area_get_layout_mode (
                       HILDON_HOME_AREA (window->priv->applet_area)))
    res = ossohelp_show (osso_ctx, HILDON_HOME_LAYOUT_HELP_TOPIC,
                         OSSO_HELP_SHOW_DIALOG);
  else
    res = ossohelp_show (osso_ctx, HILDON_HOME_NORMAL_HELP_TOPIC, 0);

  switch (res)
    {
    case OSSO_OK:
      break;
    case OSSO_ERROR:
      ULOG_ERR ("OSSO_ERROR (No help for such topic ID)\n");
      break;
    case OSSO_RPC_ERROR:
      ULOG_ERR ("OSSO_RPC_ERROR (Unable to contact HelpApp or Browser)\n");
      break;
    case OSSO_INVALID:
      ULOG_ERR ("OSSO_INVALID (Parameter not formally correct)\n");
      break;
    default:
      ULOG_ERR ("Unknown error!\n");
      break;
    }
}

static void
area_layout_mode_start (HildonHomeArea *area,
                        HildonHomeWindow   *window)
{
  hildon_home_window_set_desktop_dimmed (window, TRUE);

  hildon_home_window_show_layout_mode_banner (window);
}

static void
area_layout_mode_started (HildonHomeArea *area,
                          HildonHomeWindow   *window)
{
  HildonHomeWindowPrivate *priv = window->priv;

  hildon_home_titlebar_set_mode (HILDON_HOME_TITLEBAR (priv->titlebar),
                                 HILDON_HOME_TITLEBAR_LAYOUT);

}

static void
plugin_list_directory_changed_cb (HildonPluginList *list,
                                  HildonHomeWindow *window)
{
  HildonHomeWindowPrivate *priv = window->priv;

  hildon_home_area_sync_to_list (HILDON_HOME_AREA (priv->applet_area),
                                 priv->plugin_list);

  hildon_home_area_sync_from_list (
                          HILDON_HOME_AREA (priv->applet_area),
                          priv->plugin_list);
}


static void
area_layout_mode_ended (HildonHomeArea *area,
                        HildonHomeWindow   *window)
{
  HildonHomeWindowPrivate *priv = window->priv;

  hildon_home_titlebar_set_mode (HILDON_HOME_TITLEBAR (priv->titlebar),
                                 HILDON_HOME_TITLEBAR_NORMAL);

  hildon_home_window_set_desktop_dimmed (window, FALSE);

  if (priv->layout_mode_banner_to)
    {
      g_source_remove (priv->layout_mode_banner_to);
      if (priv->layout_mode_banner)
        {
          gtk_widget_destroy (priv->layout_mode_banner);
          priv->layout_mode_banner = NULL;
        }
    }
}

static void
area_add (HildonHomeArea   *area,
          GtkWidget        *applet,
          HildonHomeWindow *window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  g_signal_emit_by_name (priv->titlebar, "applet-added", area);
}

static void
area_applet_selected (HildonHomeArea   *area,
                      GtkWidget        *applet,
                      HildonHomeWindow *window)
{
  hildon_home_window_set_desktop_dimmed (window, TRUE);
}


static void
area_remove (HildonHomeArea   *area,
             GtkWidget        *applet,
             HildonHomeWindow *window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  g_signal_emit_by_name (priv->titlebar, "applet-removed", area);
  
  /* Only save if not in layout mode (if we just removed an applet). If in
   * layout mode it will be saved if the layout is accepted */
  if (!hildon_home_area_get_layout_mode (HILDON_HOME_AREA (priv->applet_area)))
    {
      gchar *filename = g_build_filename (g_getenv ("HOME"),
                                          HH_AREA_CONFIGURATION_FILE,
                                          NULL);

      if (hildon_home_area_save_configuration (
                                           HILDON_HOME_AREA (priv->applet_area),
                                           filename) < 0 )
        hildon_home_window_show_information_note (window,
                                                  HILDON_HOME_FLASH_FULL_TEXT);

      g_free (filename);
    }
}

static void
background_manager_load_complete_cb (BackgroundManager *manager,
                                     HildonHomeWindow  *window)
{
  g_debug (G_STRLOC ": load complete");
}

static void
background_manager_load_cancel_cb (BackgroundManager *manager,
                                   HildonHomeWindow  *window)
{
  g_debug (G_STRLOC ": load cancelled");
}

static void
background_manager_load_error_cb (BackgroundManager *manager,
                                  const GError      *error,
                                  HildonHomeWindow  *window)
{
  const gchar *text;
  
  g_return_if_fail (error);
  
  if (error->domain == background_manager_error_quark ())
    {
      switch (error->code)
        {
          case BACKGROUND_MANAGER_ERROR_IO:
              text = HILDON_HOME_INTERRUPTED_TEXT;
              break;
          case BACKGROUND_MANAGER_ERROR_CORRUPT:
              text = HILDON_HOME_CORRUPTED_TEXT;
              break;
          case BACKGROUND_MANAGER_ERROR_MMC_OPEN:
              text = HILDON_HOME_MMC_OPEN_TEXT;
              break;
          case BACKGROUND_MANAGER_ERROR_MEMORY:
          default:
              text = NULL;
              break;
        }
    }
  else
    text = NULL;

  if (text)
    hildon_home_window_show_information_note (window, text);
}

static void
background_manager_changed_cb (BackgroundManager *manager,
			       GdkPixbuf         *pixbuf,
			       HildonHomeWindow  *window)
{
  HildonHomeWindowPrivate *priv = window->priv;

  if (!pixbuf)
    return;
  else
    {
      priv = window->priv;
      
      /* force the event boxes to be window-less and avoid covering
       * the area we paint directly onto
       */
      gtk_event_box_set_visible_window (GTK_EVENT_BOX (priv->titlebar), FALSE);
      gtk_event_box_set_visible_window (GTK_EVENT_BOX (priv->main_area), FALSE);
      
      gtk_widget_queue_draw (GTK_WIDGET (window));
    }
}

static void
hildon_home_window_lowmem (HildonHomeWindow   *window,
                           gboolean            is_lowmem)
{
  HildonHomeWindowPrivate *priv = window->priv;

  g_debug ("HildonHome is lowmem: %i", is_lowmem);

  priv->is_lowmem = is_lowmem;

}

static void
hildon_home_window_background (HildonHomeWindow   *window,
                               gboolean            is_background)
{
  HildonHomeWindowPrivate *priv = window->priv;

  g_debug ("HildonHome is background: %i", is_background);

  if (!priv->is_inactive)
    {

      /* If we were in layout mode and went to background, we need
       * to cancel it */
      if (is_background && 
          hildon_home_area_get_layout_mode (HILDON_HOME_AREA (priv->applet_area)))
        {
          gchar *user_filename = NULL; 
          gchar *filename = NULL;

          hildon_home_area_set_layout_mode (HILDON_HOME_AREA (priv->applet_area),
                                            FALSE);

          user_filename = g_build_filename (g_getenv ("HOME"),
                                            HH_AREA_CONFIGURATION_FILE,
                                            NULL);

          if (g_file_test (user_filename, G_FILE_TEST_EXISTS))
            filename = user_filename;
          else
            filename = HH_AREA_GLOBAL_CONFIGURATION_FILE;

          hildon_home_area_load_configuration (HILDON_HOME_AREA (priv->applet_area),
                                               filename);

          g_free (user_filename);
        }

      gtk_container_foreach (GTK_CONTAINER (priv->applet_area),
                             (GtkCallback)hildon_home_applet_set_is_background,
                             (gpointer)is_background);
    }
}

static void
hildon_home_window_system_inactivity (HildonHomeWindow   *window,
                                      gboolean            is_inactive)
{
  HildonHomeWindowPrivate *priv = window->priv;

  g_debug ("HildonHome detected system inactivity: %i", is_inactive);

  priv->is_inactive = is_inactive;

  gtk_container_foreach (GTK_CONTAINER (priv->applet_area),
                         (GtkCallback)hildon_home_applet_set_is_background,
                         (gpointer)is_inactive);
}


static const gchar *
get_sidebar_image_from_theme (GtkWidget *widget)
{
  GtkStyle *style;
  
  style = gtk_rc_get_style_by_paths (gtk_widget_get_settings (widget),
		  		     HILDON_HOME_BLEND_IMAGE_SIDEBAR_NAME,
				     NULL,
				     G_TYPE_NONE);
  
  if (style && style->rc_style->bg_pixmap_name[0])
    return style->rc_style->bg_pixmap_name[0];

  return NULL;
}


static const gchar *
get_titlebar_image_from_theme (GtkWidget *widget)
{
  GtkStyle *style;

  style = gtk_rc_get_style_by_paths (gtk_widget_get_settings (widget),
		  		     HILDON_HOME_BLEND_IMAGE_TITLEBAR_NAME,
				     NULL,
				     G_TYPE_NONE);

  if (style && style->rc_style->bg_pixmap_name[0])
    return style->rc_style->bg_pixmap_name[0];

  return NULL;
}

#if 0
static void
hildon_home_window_show (GtkWidget *widget)
{
  gtk_widget_realize (widget);
  gdk_window_set_type_hint (widget->window,
		            GDK_WINDOW_TYPE_HINT_DESKTOP);

  background_manager_set_desktop (background_manager_get_default (),
                                  widget->window);
  
  GTK_WIDGET_CLASS (hildon_home_window_parent_class)->show (widget);
}
#endif

static void
hildon_home_window_style_set (GtkWidget *widget,
			      GtkStyle  *old_style)
{
  HildonHomeWindowPrivate *priv = HILDON_HOME_WINDOW (widget)->priv;
  static gchar *current_titlebar_image;
  static gchar *current_sidebar_image;
  const gchar *new_titlebar_image, *new_sidebar_image;

  if (GTK_WIDGET_CLASS (hildon_home_window_parent_class)->style_set)
    GTK_WIDGET_CLASS (hildon_home_window_parent_class)->style_set (widget,
		    						   old_style);
  
  new_titlebar_image = get_titlebar_image_from_theme (widget);
  new_sidebar_image = get_sidebar_image_from_theme (widget);

  /* avoid resetting the background when the window is exposed for the
   * first time
   */
  if (!old_style)
    {
      current_titlebar_image = g_strdup (new_titlebar_image);
      current_sidebar_image = g_strdup (new_sidebar_image);
      return;
    }

  /* avoid resetting the background if the theme hasn't changed */
  if (g_str_equal (current_titlebar_image, new_titlebar_image) &&
      g_str_equal (current_sidebar_image, new_sidebar_image) &&
      background_manager_refresh_from_cache (priv->bg_manager))
    {
      return;
    }

  g_free(current_titlebar_image);
  current_titlebar_image = g_strdup (new_titlebar_image);
  
  g_free(current_sidebar_image);
  current_sidebar_image = g_strdup (new_sidebar_image);

  background_manager_set_components (priv->bg_manager,
                                     new_titlebar_image,
                                     new_sidebar_image,
                                     TRUE);
}

static gboolean
hildon_home_window_key_press_event (GtkWidget *widget,
                                    GdkEventKey *event)
{
  HildonHomeWindowPrivate *priv = HILDON_HOME_WINDOW (widget)->priv;

  switch (event->keyval)
    {
      case HILDON_HARDKEY_MENU:
          hildon_home_titlebar_toggle_menu (HILDON_HOME_TITLEBAR (priv->titlebar));
          break;
      case HILDON_HARDKEY_ESC:
          /* FIXME: Have a signal in HomeWindow instead */
          if (hildon_home_area_get_layout_mode (
                                    HILDON_HOME_AREA(priv->applet_area)))
            g_signal_emit_by_name (G_OBJECT (priv->titlebar), "layout-cancel");

          break;
      default:
          if (GTK_WIDGET_CLASS (
                     hildon_home_window_parent_class)->key_press_event)
            return GTK_WIDGET_CLASS (
                     hildon_home_window_parent_class)->key_press_event (widget,
                                                                        event);
          else
            return FALSE;
    }

  return TRUE;
}

static void
hildon_home_window_finalize (GObject *gobject)
{
  HildonHomeWindowPrivate *priv = HILDON_HOME_WINDOW (gobject)->priv;

  g_signal_handlers_disconnect_by_func (priv->bg_manager,
		                        background_manager_changed_cb,
					HILDON_HOME_WINDOW (gobject));
  
  G_OBJECT_CLASS (hildon_home_window_parent_class)->finalize (gobject);
}

static void
hildon_home_window_set_property (GObject      *gobject,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
  HildonHomeWindow *window = HILDON_HOME_WINDOW (gobject);

  switch (prop_id)
    {
    case PROP_OSSO_CONTEXT:
      window->priv->osso_context = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
hildon_home_window_get_property (GObject    *gobject,
				 guint       prop_id,
				 GValue     *value,
				 GParamSpec *pspec)
{
  HildonHomeWindow *window = HILDON_HOME_WINDOW (gobject);

  switch (prop_id)
    {
    case PROP_OSSO_CONTEXT:
      g_value_set_pointer (value, window->priv->osso_context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

void
hildon_home_window_applets_init (HildonHomeWindow * window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  gchar *user_filename = NULL; 
  gchar *filename = NULL;

  user_filename = g_build_filename (g_getenv ("HOME"),
                                    HH_AREA_CONFIGURATION_FILE,
                                    NULL);

  if (g_file_test (user_filename, G_FILE_TEST_EXISTS))
    filename = user_filename;
  else
    filename = HH_AREA_GLOBAL_CONFIGURATION_FILE;
  
  hildon_home_area_load_configuration (HILDON_HOME_AREA (priv->applet_area),
                                       filename);

  g_free (user_filename);
}

static GObject *
hildon_home_window_constructor (GType                  gtype,
				guint                  n_params,
				GObjectConstructParam *params)
{
  GObject *retval;
  HildonHomeWindow *window;
  HildonHomeWindowPrivate *priv;
  GtkWidget *widget;
  GtkWidget *vbox;
  GtkWidget *hbox;

  retval = G_OBJECT_CLASS (hildon_home_window_parent_class)->constructor (gtype,
		  							  n_params,
									  params);
  widget = GTK_WIDGET (retval);
  window = HILDON_HOME_WINDOW (retval);
  priv = window->priv;

  gtk_widget_set_app_paintable (widget, TRUE);

  gtk_widget_push_composite_child ();
  
  /* we push everything inside an alignment because we don't want
   * to be hidden by the task navigator
   */
  priv->align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (priv->align),
		  	     0, 0, /* top, bottom */
			     HILDON_HOME_LEFT_PADDING, 0);
  gtk_widget_set_composite_name (priv->align, "hildon-home-main-alignment");
  gtk_container_add (GTK_CONTAINER (window), priv->align);
  gtk_widget_show (priv->align);
  
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_composite_name (vbox, "hildon-home-main-vbox");
  gtk_container_add (GTK_CONTAINER (priv->align), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_size_request (hbox, -1, HILDON_HOME_TOP_BAR_HEIGHT);
  gtk_widget_set_composite_name (hbox, "hildon-home-top-bar");
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  priv->titlebar = hildon_home_titlebar_new (priv->osso_context);
  gtk_widget_set_composite_name (priv->titlebar, "hildon-home-titlebar");
  g_signal_connect (priv->titlebar, "select-applets-activate",
		    G_CALLBACK (titlebar_select_applets_activate_cb),
		    window);
  g_signal_connect (priv->titlebar, "layout-mode-activate",
		    G_CALLBACK (titlebar_layout_mode_activate_cb),
		    window);
  g_signal_connect (priv->titlebar, "set-background-activate",
                    G_CALLBACK (titlebar_set_background_activate_cb),
                    window);
  g_signal_connect (priv->titlebar, "layout-accept",
		    G_CALLBACK (titlebar_layout_accept_cb),
		    window);
  g_signal_connect (priv->titlebar, "layout-cancel",
		    G_CALLBACK (titlebar_layout_cancel_cb),
		    window);
  g_signal_connect (priv->titlebar, "applet-activate",
		    G_CALLBACK (titlebar_applet_activate_cb),
		    window);
  g_signal_connect (priv->titlebar, "help-activate",
		    G_CALLBACK (titlebar_help_activate_cb),
		    window);
  gtk_box_pack_start (GTK_BOX (hbox), priv->titlebar, TRUE, TRUE, 0);
  gtk_widget_show (priv->titlebar);

  priv->main_area = gtk_event_box_new ();
  gtk_widget_set_composite_name (priv->main_area, "hildon-home-main-area");
  gtk_box_pack_end (GTK_BOX (vbox), priv->main_area, TRUE, TRUE, 0);
  gtk_widget_show (priv->main_area);

  priv->applet_area = hildon_home_area_new ();/*gtk_fixed_new();*/
  gtk_container_add( GTK_CONTAINER(priv->main_area), priv->applet_area );
  g_signal_connect (priv->applet_area, "layout-mode-start",
                    G_CALLBACK (area_layout_mode_start),
                    window);
  g_signal_connect (priv->applet_area, "layout-mode-started",
                    G_CALLBACK (area_layout_mode_started),
                    window);
  g_signal_connect (priv->applet_area, "layout-mode-ended",
                    G_CALLBACK (area_layout_mode_ended),
                    window);

  g_signal_connect (priv->applet_area, "applet-added",
                    G_CALLBACK (area_add),
                    window);
  g_signal_connect (priv->applet_area, "applet-selected",
                    G_CALLBACK (area_applet_selected),
                    window);
  g_signal_connect (priv->applet_area, "remove",
                    G_CALLBACK (area_remove),
                    window);

  gtk_widget_show_all (priv->applet_area);

  /* realize and set the window type hint; then bind the desktop
   * GdkWindow to the background manager
   */
  gtk_widget_realize (widget);
  gdk_window_set_type_hint (widget->window, GDK_WINDOW_TYPE_HINT_DESKTOP);
  
  priv->bg_manager = background_manager_get_default ();
  
  g_signal_connect (priv->bg_manager, "changed",
		    G_CALLBACK (background_manager_changed_cb),
		    window);
  g_signal_connect (priv->bg_manager, "preview",
                    G_CALLBACK (background_manager_changed_cb),
                    window);
  g_signal_connect (priv->bg_manager, "load-complete",
                    G_CALLBACK (background_manager_load_complete_cb),
                    window);
  g_signal_connect (priv->bg_manager, "load-cancel",
                    G_CALLBACK (background_manager_load_cancel_cb),
                    window);
  g_signal_connect (priv->bg_manager, "load-error",
                    G_CALLBACK (background_manager_load_error_cb),
                    window);

  background_manager_set_components (priv->bg_manager,
		  		     get_titlebar_image_from_theme (widget),
				     get_sidebar_image_from_theme (widget),
                                     FALSE);
  background_manager_set_desktop (priv->bg_manager, widget->window);
  
  gtk_widget_pop_composite_child ();
  
  return retval;
}

static void
hildon_home_window_class_init (HildonHomeWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->constructor = hildon_home_window_constructor;
  gobject_class->set_property = hildon_home_window_set_property;
  gobject_class->get_property = hildon_home_window_get_property;
  gobject_class->finalize = hildon_home_window_finalize;
  
  widget_class->style_set = hildon_home_window_style_set;
  widget_class->key_press_event = hildon_home_window_key_press_event;

  klass->background = hildon_home_window_background;
  klass->lowmem = hildon_home_window_lowmem;
  klass->system_inactivity = hildon_home_window_system_inactivity;
  
  g_signal_new ("background",
                G_OBJECT_CLASS_TYPE (gobject_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeWindowClass, background),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__BOOLEAN,
                G_TYPE_NONE,
                1,
                G_TYPE_BOOLEAN);
  
  g_signal_new ("system-inactivity",
                G_OBJECT_CLASS_TYPE (gobject_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeWindowClass, system_inactivity),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__BOOLEAN,
                G_TYPE_NONE,
                1,
                G_TYPE_BOOLEAN);
  
  g_signal_new ("lowmem",
                G_OBJECT_CLASS_TYPE (gobject_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeWindowClass, lowmem),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__BOOLEAN,
                G_TYPE_NONE,
                1,
                G_TYPE_BOOLEAN);
  
  g_object_class_install_property (gobject_class,
		                   PROP_OSSO_CONTEXT,
				   g_param_spec_pointer ("osso-context",
					   		 "Osso Context",
							 "Osso context to be used by the window",
							 (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));
  
  g_type_class_add_private (klass, sizeof (HildonHomeWindowPrivate));
}

static void
hildon_home_window_init (HildonHomeWindow *window)
{
  HildonHomeWindowPrivate *priv;
  GtkWidget *widget;

  gtk_window_set_has_frame (GTK_WINDOW (window), FALSE);

  widget = GTK_WIDGET (window);
  gtk_widget_set_size_request (widget, HILDON_HOME_WINDOW_WIDTH,
                               HILDON_HOME_WINDOW_HEIGHT);


  window->priv = priv = HILDON_HOME_WINDOW_GET_PRIVATE (window);

  priv->osso_context = NULL;
  priv->plugin_list = g_object_new (HILDON_TYPE_PLUGIN_LIST,
                                    "name-key", APPLET_KEY_NAME,
                                    "library-key", APPLET_KEY_LIBRARY,
                                    "group", APPLET_GROUP,
                                    "default-text-domain", "maemo-af-desktop",
                                    NULL);

  hildon_plugin_list_set_directory (priv->plugin_list, HH_DESKTOP_FILES_DIR);

  g_signal_connect (G_OBJECT (priv->plugin_list), "directory-changed",
                    G_CALLBACK (plugin_list_directory_changed_cb),
                    window);

  priv->is_dimmed = FALSE;
}

GtkWidget *
hildon_home_window_new (osso_context_t *osso_context)
{
  return g_object_new (HILDON_TYPE_HOME_WINDOW,
		       "osso-context", osso_context,
		       NULL);
}

GtkWidget *
hildon_home_window_get_titlebar (HildonHomeWindow *window)
{
  g_return_val_if_fail (HILDON_IS_HOME_WINDOW (window), NULL);

  return window->priv->titlebar;
}

GtkWidget *
hildon_home_window_get_main_area (HildonHomeWindow *window)
{
  g_return_val_if_fail (HILDON_IS_HOME_WINDOW (window), NULL);

  return window->priv->main_area;
}


void
hildon_home_window_show_information_note (HildonHomeWindow *window,
                                          const gchar *text)
{
  GtkWidget *note = NULL;

  note =
    hildon_note_new_information (NULL, 
				                 text);
		    
  gtk_dialog_run (GTK_DIALOG (note));
  if (note) 
    gtk_widget_destroy (GTK_WIDGET (note));
}

void
hildon_home_window_set_osso_context (HildonHomeWindow *window,
				     osso_context_t   *osso_context)
{
  g_return_if_fail (HILDON_IS_HOME_WINDOW (window));

  if (window->priv->osso_context != osso_context)
    {
      window->priv->osso_context = osso_context;

      g_object_notify (G_OBJECT (window), "osso-context");
    }
}

void
hildon_home_window_set_desktop_dimmed (HildonHomeWindow *window,
                                       gboolean dimmed)
{
  HildonHomeWindowPrivate *priv;
  
  g_return_if_fail (HILDON_IS_HOME_WINDOW (window));
  priv = window->priv;

  if (priv->is_dimmed == dimmed)
    return;

  if (dimmed)
    {
      osso_rpc_run_with_defaults (priv->osso_context,
                                  STATUSBAR_SERVICE_NAME,
                                  STATUSBAR_INSENSITIVE_METHOD,
                                  NULL,
                                  0,
                                  NULL);

      osso_rpc_run (priv->osso_context,
                    TASKNAV_SERVICE_NAME,
                    TASKNAV_GENERAL_PATH,
                    TASKNAV_INSENSITIVE_INTERFACE,
                    TASKNAV_INSENSITIVE_METHOD,
                    NULL,
                    0,
                    NULL);
    }
  else
    {
      osso_rpc_run_with_defaults (priv->osso_context,
                                  STATUSBAR_SERVICE_NAME,
                                  STATUSBAR_SENSITIVE_METHOD,
                                  NULL,
                                  0,
                                  NULL);

      osso_rpc_run (priv->osso_context,
                    TASKNAV_SERVICE_NAME,
                    TASKNAV_GENERAL_PATH,
                    TASKNAV_SENSITIVE_INTERFACE,
                    TASKNAV_SENSITIVE_METHOD,
                    NULL,
                    0,
                    NULL);
    }

  priv->is_dimmed = dimmed;
}
