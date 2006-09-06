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

#include "home-applet-manager.h"
#include "background-manager.h"
#include "layout-manager.h"
#include "home-select-applets-dialog.h"
#include "home-applet-handler.h"

#include "hildon-home-titlebar.h"
#include "hildon-home-common.h"
#include "hildon-home-private.h"
#include "hildon-home-window.h"

#define HILDON_HOME_LEFT_PADDING	80	/* keep synced with the TN width */
#define HILDON_HOME_WINDOW_WIDTH	800
#define HILDON_HOME_WINDOW_HEIGHT	480
#define HILDON_HOME_TOP_BAR_HEIGHT	60

#define HILDON_HOME_WINDOW_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_TYPE_HOME_WINDOW, HildonHomeWindowPrivate))

struct _HildonHomeWindowPrivate
{
  BackgroundManager *bg_manager;
  osso_context_t *osso_context;

  GtkWidget *align;
  
  GtkWidget *titlebar;
  GtkWidget *applet_area;
  GtkWidget *main_area;

  gboolean   menu_key_pressed;
};

G_DEFINE_TYPE (HildonHomeWindow, hildon_home_window, GTK_TYPE_WINDOW);

enum
{
  PROP_0,

  PROP_OSSO_CONTEXT
};

static void
titlebar_select_applets_activate_cb (HildonHomeTitlebar *titlebar,
				     HildonHomeWindow   *window)
{
  g_debug ("select applets activate\n");

  select_applets_selected (window->priv->osso_context,
		  	   titlebar,
			   GTK_FIXED (window->priv->applet_area));

  
}

static void
titlebar_layout_mode_activate_cb (HildonHomeTitlebar *titlebar,
				  HildonHomeWindow   *window)
{
  g_debug ("layout mode activate");

  layout_mode_begin (titlebar,
		     GTK_FIXED (window->priv->applet_area),
		     NULL, NULL);
}

static void
titlebar_layout_accept_cb (HildonHomeTitlebar *titlebar,
			   HildonHomeWindow   *window)
{
  g_debug ("layout accepted");

  layout_mode_accept ();
}

static void
titlebar_layout_cancel_cb (HildonHomeTitlebar *titlebar,
			   HildonHomeWindow   *window)
{
  g_debug ("layout discarded");

  /* this will rollback */
  layout_mode_cancel ();
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

  res = osso_cp_plugin_execute (osso_ctx, applet_path,
		                GTK_WIDGET (window),
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

static gint
hildon_home_window_key_press_cb (GtkWidget * widget,
				 GdkEventKey * keyevent,
				 gpointer data)
{
  HildonHomeWindow  * window = HILDON_HOME_WINDOW (widget);
  
  if (keyevent->keyval == HILDON_HARDKEY_MENU)
    {
      window->priv->menu_key_pressed = TRUE;
    }
  
  return FALSE;
}

static gint
hildon_home_window_key_release_cb (GtkWidget * widget,
				   GdkEventKey * keyevent,
				   gpointer data)
{
  if (keyevent->keyval == HILDON_HARDKEY_MENU)
    {
      HildonHomeWindow  * window = HILDON_HOME_WINDOW (widget);
      
      if (window->priv->menu_key_pressed)
        {
	  HildonHomeTitlebar * tb =
	    HILDON_HOME_TITLEBAR (window->priv->titlebar);
	  
	  gboolean state = hildon_home_titlebar_get_menu_active (tb);
	  
	  hildon_home_titlebar_set_menu_active (tb, !state);
	  window->priv->menu_key_pressed = FALSE;
        }
    }
  
  return FALSE;
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
      GtkWidget *widget = GTK_WIDGET (window);
      GdkColormap *colormap = NULL;
      GdkPixmap *pixmap;
      GdkBitmap *bit_mask;

      gtk_widget_set_app_paintable (widget, TRUE);
      gtk_widget_realize (widget);

      colormap =
	gdk_drawable_get_colormap (GDK_DRAWABLE (widget->window));

      gdk_pixbuf_render_pixmap_and_mask_for_colormap (pixbuf,
		      				      colormap,
						      &pixmap,
						      &bit_mask,
						      0);
      if (pixmap)
	gdk_window_set_back_pixmap (widget->window,
				    pixmap,
				    FALSE);

      if (bit_mask)
	g_object_unref (bit_mask);
    }
  
  /* force the event boxes to be window-less and avoid covering
   * the area we paint directly onto
   */
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (priv->titlebar), FALSE);
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (priv->main_area), FALSE);
  
  gtk_widget_queue_draw (GTK_WIDGET (window));
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

static void
hildon_home_window_show (GtkWidget *widget)
{
  gtk_widget_realize (widget);
  gdk_window_set_type_hint (widget->window,
		            GDK_WINDOW_TYPE_HINT_DESKTOP);

  GTK_WIDGET_CLASS (hildon_home_window_parent_class)->show (widget);
}

static void
hildon_home_window_style_set (GtkWidget *widget,
			      GtkStyle  *old_style)
{
  HildonHomeWindowPrivate *priv = HILDON_HOME_WINDOW (widget)->priv;

  if (GTK_WIDGET_CLASS (hildon_home_window_parent_class)->style_set)
    GTK_WIDGET_CLASS (hildon_home_window_parent_class)->style_set (widget,
		    						   old_style);
  
  /* avoid resetting the background when the window is exposed for the
   * first time
   */
  if (!old_style)
    return;

  background_manager_set_components (priv->bg_manager,
		  		     get_titlebar_image_from_theme (widget),
				     get_sidebar_image_from_theme (widget));
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

static void
hildon_home_window_applets_init (HildonHomeWindow * window)
{
  GList *ids, *l;
  AppletManager *manager;
  gint applet_x = 20;
  gint applet_y = -100;

  manager = applet_manager_get_instance ();
  
  ids = applet_manager_get_identifier_all (manager);
  for (l = ids; l != NULL; l = l->next)
    {
      const gchar *id = l->data;

      applet_manager_get_coordinates (manager, id, &applet_x, &applet_y);
      g_debug ("adding applet <%s>[%d, %d]", id, applet_x, applet_y);
      
      if (applet_x != -1 && applet_y != -1)
        {
	  GtkWidget *applet;

	  applet = GTK_WIDGET (applet_manager_get_eventbox (manager, id));

	  g_debug ("adding applet <%s>[%d, %d]", id,
		   applet_x, applet_y - HILDON_HOME_TOP_BAR_HEIGHT);
	  gtk_fixed_put (GTK_FIXED (window->priv->applet_area),
			 applet,
			 applet_x,
			 applet_y - HILDON_HOME_TOP_BAR_HEIGHT);
	  gtk_widget_show_all (applet);

	  g_debug ("resulting allocation @ [%d,%d]",
		   applet->allocation.x,
		   applet->allocation.y);
	  
	}
    }
  
  applet_manager_foreground_all (manager);
  g_object_unref (manager);
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
  
  g_signal_connect (window, "key_press_event",
		    G_CALLBACK(hildon_home_window_key_press_cb), NULL);
  g_signal_connect (window, "key_release_event",
		    G_CALLBACK(hildon_home_window_key_release_cb), NULL);
  
  gtk_box_pack_start (GTK_BOX (hbox), priv->titlebar, TRUE, TRUE, 0);
  gtk_widget_show (priv->titlebar);

  priv->main_area = gtk_event_box_new ();
  gtk_widget_set_composite_name (priv->main_area, "hildon-home-main-area");
  gtk_box_pack_end (GTK_BOX (vbox), priv->main_area, TRUE, TRUE, 0);
  gtk_widget_show (priv->main_area);

  priv->applet_area = gtk_fixed_new();
  gtk_container_add( GTK_CONTAINER(priv->main_area), priv->applet_area );

  hildon_home_window_applets_init (window);
  gtk_widget_show_all (priv->applet_area);
  
  g_signal_connect (priv->bg_manager, "changed",
		    G_CALLBACK (background_manager_changed_cb),
		    window);
  background_manager_set_components (priv->bg_manager,
		  		     get_titlebar_image_from_theme (widget),
				     get_sidebar_image_from_theme (widget));
  
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
  
  widget_class->show = hildon_home_window_show;
  widget_class->style_set = hildon_home_window_style_set;

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
  priv->bg_manager = background_manager_get_default ();
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
hildon_home_window_show_flash_full_note (HildonHomeWindow *window)
{
  GtkWidget *flash_full_note = NULL;

  flash_full_note =
    hildon_note_new_information (GTK_WINDOW (window), 
				 HILDON_HOME_FLASH_FULL_TEXT);
		    
  gtk_dialog_run (GTK_DIALOG (flash_full_note));
  if (flash_full_note) 
    gtk_widget_destroy (GTK_WIDGET (flash_full_note));
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

