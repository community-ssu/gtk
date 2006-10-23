/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#include "hildon-navigator-window.h"

/* For user config dir monitoring */
#include <hildon-base-lib/hildon-base-dnotify.h>
#include <glib/gstdio.h>
#include <osso-log.h>

/* Hildon includes */
#include "osso-manager.h"
#include "hildon-navigator.h"
#include "hildon-navigator-panel.h"
#include "hn-app-switcher.h"
#include "hn-others-button.h"

#include "hn-wm.h"
#include "kstrace.h"

/* Global external variables */
gboolean config_do_bgkill;
gboolean config_lowmem_dim;
gboolean config_lowmem_notify_enter;
gboolean config_lowmem_notify_leave;
gboolean config_lowmem_pavlov_dialog;
gboolean tn_is_sensitive=TRUE;

static GtkWindowClass *parent_class = NULL;

typedef struct _HildonNavigatorWindowPrivate HildonNavigatorWindowPrivate;

struct _HildonNavigatorWindowPrivate
{
  GtkWidget *panel;
  GtkWidget *app_switcher;
  GtkWidget *others_button;

  gboolean focus;
};

/* static declarations */
static void hildon_navigator_window_class_init (HildonNavigatorWindowClass 
						*hnwindow_class);

static gboolean getenv_yesno(const char *env, gboolean def);
static void set_focus_forall_cb (GtkWidget *widget, gpointer data);
static void configuration_changed (char *path, gpointer *data);
static void initialize_navigator_menus (HildonNavigatorWindowPrivate *priv);
static GObject *hn_window_constructor (GType gtype, guint n_params, GObjectConstructParam *params);
static void hildon_navigator_window_finalize (GObject *self);
static void hildon_navigator_window_init (HildonNavigatorWindow *window);
/* */

GType
hildon_navigator_window_get_type (void)
{
  static GType window_type = 0;

  if ( !window_type )
  {
    static const GTypeInfo window_info =
    {
      sizeof (HildonNavigatorWindowClass) ,
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) hildon_navigator_window_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (HildonNavigatorWindow),
      0,    /* n_preallocs */
      (GInstanceInitFunc) hildon_navigator_window_init,
    };
    
    window_type = g_type_register_static (GTK_TYPE_WINDOW,
                                         "HildonNavigatorWindow",
                                         &window_info,0);
  }

  return window_type;
}

static void 
hildon_navigator_window_class_init (HildonNavigatorWindowClass *hnwindow_class)
{
  GObjectClass      *object_class = G_OBJECT_CLASS   (hnwindow_class);
  /*GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS (hnwindow_class);*/

  parent_class = g_type_class_peek_parent (hnwindow_class);
  
  g_type_class_add_private (hnwindow_class, sizeof (HildonNavigatorWindowPrivate));
  
  object_class->finalize          = hildon_navigator_window_finalize;
  object_class->constructor	  = hn_window_constructor;
  hnwindow_class->set_sensitive   = hn_window_set_sensitive;
  hnwindow_class->set_focus       = hn_window_set_focus;
  hnwindow_class->get_others_menu = hn_window_get_others_menu;
}

/*FIXME: what do we do with this? */
static gboolean  
getenv_yesno(const char *env, gboolean def)
{
    char *val = getenv (env);

    if (val)
	return (strcmp(val, "yes") == 0);
    else
	return def;
}

#ifndef TN_ALWAYS_FOCUSABLE
static void
set_focus_forall_cb (GtkWidget *widget, gpointer data)
{
  gboolean focus = GPOINTER_TO_INT(data);
  static int depth = 0;

  depth++;

  HN_DBG("recursion depth %d", depth);
  HN_DBG("widget of type [%s]", 
	 g_type_name (G_TYPE_FROM_INSTANCE (widget)));

  if (GTK_IS_TOGGLE_BUTTON (widget))
  {
    HN_DBG("toggle button - making %sfocuable", focus ? "" : "un");
    g_object_set (G_OBJECT (widget), "can-focus", focus, NULL);
  }

  if (GTK_IS_CONTAINER (widget))
  {
    HN_DBG("child is a container -- making recursive call");
    gtk_container_forall (GTK_CONTAINER (widget), 
			  set_focus_forall_cb,
                          GINT_TO_POINTER(focus));
  }

  depth--;
}
#endif

static void 
configuration_changed (char *path, gpointer *data)
{
    gchar *config_file; 
    const char *home_dir;
    HildonNavigatorPanel *panel;

    g_assert (data != NULL);

    home_dir = getenv ("HOME");

    panel = (HildonNavigatorPanel *)data;

    config_file = g_strdup_printf("%s%s", home_dir, NAVIGATOR_USER_PLUGINS);
    hn_panel_unload_all_plugins (panel,TRUE);
    hn_panel_load_plugins_from_file (panel, config_file);

    g_free (config_file);
}

static void
initialize_navigator_menus (HildonNavigatorWindowPrivate *priv)
{
  g_assert (priv != NULL);
#if 0
  /*Initialize application switcher menu*/
  application_switcher_initialize_menu(tasknav->app_switcher);
#endif
}

static void 
hn_window_load_plugins (HildonNavigatorWindow *window, 
		        GdkEventExpose *event,
			gpointer panelptr)
{
  gchar *config_file;
  const char *home_dir;
  HildonNavigatorPanel *panel = HILDON_NAVIGATOR_PANEL (panelptr);

  home_dir = getenv ("HOME");
  
  config_file = g_strdup_printf  ("%s%s", home_dir, NAVIGATOR_USER_PLUGINS);

  /* Initialize Plugins */
  if (g_file_test (config_file, G_FILE_TEST_EXISTS))
    hn_panel_load_plugins_from_file (panel, config_file);
  else
    hn_panel_load_plugins_from_file (panel, NAVIGATOR_FACTORY_PLUGINS);

  g_free (config_file);
}


static GObject *
hn_window_constructor (GType gtype,
                       guint n_params,
		       GObjectConstructParam *params)
{

  GObject *self;
  HildonNavigatorWindow *window;
  HildonNavigatorWindowPrivate *priv;
  gchar *plugin_dir;
  const char *home_dir;
  gboolean dnotify_ret;
  hildon_return_t dnotify_status;

  self = G_OBJECT_CLASS (parent_class)->constructor (gtype,
                                                     n_params,
                                                     params);

  window = HILDON_NAVIGATOR_WINDOW (self);
  priv   = HILDON_NAVIGATOR_WINDOW_GET_PRIVATE (window);

  home_dir = getenv ("HOME");

  gtk_widget_push_composite_child ();
  
  priv->panel = GTK_WIDGET (hildon_navigator_panel_new ());

  gtk_widget_set_size_request(GTK_WIDGET (priv->panel), 
		  	      BUTTON_WIDTH,
		              gdk_screen_height());
  gtk_container_add (GTK_CONTAINER (window),GTK_WIDGET (priv->panel));

  plugin_dir  = g_build_filename (home_dir, NAVIGATOR_USER_DIR, NULL);

  /* Initialize this before any plugin or whatever */
  dnotify_status = hildon_dnotify_handler_init();
  
  hn_panel_load_dummy_buttons (HILDON_NAVIGATOR_PANEL (priv->panel));

  g_signal_connect_after (G_OBJECT (window), 
		          "map-event", 
		          G_CALLBACK(hn_window_load_plugins),
		          priv->panel);
   
  priv->others_button = hn_others_button_new ();
  
  if (priv->others_button)
    hn_panel_add_button ( HILDON_NAVIGATOR_PANEL (priv->panel),
			  priv->others_button);
  
  priv->app_switcher = hn_app_switcher_new ();
  gtk_box_pack_start ( GTK_BOX (priv->panel), 
		       priv->app_switcher, 
		       FALSE, FALSE, 0);

  initialize_navigator_menus (priv);

  priv->focus = FALSE;

  /* Create the directory if it does not exist, as we need to monitor
     * it for the TN applet to write its configuration */
  if (!g_file_test(plugin_dir, G_FILE_TEST_IS_DIR))
    g_mkdir(plugin_dir, 0755); /* THIS WILL CHANGE! CREATE IT ANYWAY! */

  if (dnotify_status == HILDON_OK)
  { 
    dnotify_ret =
      hildon_dnotify_set_cb ((hildon_dnotify_cb_f *)configuration_changed,
		            plugin_dir,
			    priv->panel);
      
    hn_others_button_dnotify_register (HN_OTHERS_BUTTON (priv->others_button));

    hn_wm_dnotify_register ();
  }
  else
    g_debug ("failed to initialize dnotify");
 
  /* cleanup */
  g_free (plugin_dir);

  gtk_widget_pop_composite_child (); 

  return self;
}

static void 
hildon_navigator_window_finalize (GObject *self)
{
  HildonNavigatorWindow        *window;
  HildonNavigatorWindowPrivate *priv;

  g_assert (self);
  g_assert (HILDON_IS_NAVIGATOR_WINDOW (self));
  
  window = HILDON_NAVIGATOR_WINDOW (self);
  priv   = HILDON_NAVIGATOR_WINDOW_GET_PRIVATE (window);

  /* deinit dnotify */
  hildon_dnotify_handler_clear ();
  
  gtk_widget_destroy (priv->others_button);
}

static void 
hildon_navigator_window_init (HildonNavigatorWindow *window)
{
  g_assert (HILDON_IS_NAVIGATOR_WINDOW (window));

  /* Get configuration options from the environment.
     */
  config_do_bgkill = getenv_yesno("NAVIGATOR_DO_BGKILL", TRUE);
  config_lowmem_dim = getenv_yesno("NAVIGATOR_LOWMEM_DIM", FALSE);
  config_lowmem_notify_enter = getenv_yesno("NAVIGATOR_LOWMEM_NOTIFY_ENTER", FALSE);
  config_lowmem_notify_leave = getenv_yesno("NAVIGATOR_LOWMEM_NOTIFY_LEAVE", FALSE);
  config_lowmem_pavlov_dialog = getenv_yesno("NAVIGATOR_LOWMEM_PAVLOV_DIALOG", FALSE);
  
  /*FIXME: we inherit from gtk_window!*/
  gtk_window_set_type_hint( GTK_WINDOW(window),GDK_WINDOW_TYPE_HINT_DOCK);
  gtk_window_set_accept_focus( GTK_WINDOW(window),TN_DEFAULT_FOCUS);

}

HildonNavigatorWindow *
hildon_navigator_window_new (void)
{
  return g_object_new (HILDON_NAVIGATOR_WINDOW_TYPE,NULL);
}

void 
hn_window_set_sensitive (HildonNavigatorWindow *window,
  		         gboolean sensitive)
{
  HildonNavigatorWindowPrivate *priv;

  g_return_if_fail (window);
  g_return_if_fail (HILDON_IS_NAVIGATOR_WINDOW (window));

  priv = HILDON_NAVIGATOR_WINDOW_GET_PRIVATE (window);
  
  if (sensitive)
    gtk_widget_set_sensitive (priv->panel,TRUE);
  else
    gtk_widget_set_sensitive (priv->panel,FALSE);

  tn_is_sensitive = sensitive;
  /* parent window as well? */
}

void 
hn_window_set_focus (HildonNavigatorWindow *window,
  		     gboolean focus)
{
  HildonNavigatorWindowPrivate *priv;

  g_return_if_fail (window);
  g_return_if_fail (HILDON_IS_NAVIGATOR_WINDOW (window));

  priv = HILDON_NAVIGATOR_WINDOW_GET_PRIVATE (window);

  if (priv->focus == focus)
  {
    HN_DBG ("requested focus state [%s] already current",focus ? "TRUE":"FALSE");
    /*return;*/
  }

  gtk_window_set_accept_focus (GTK_WINDOW (window), focus);

#ifndef TN_ALWAYS_FOCUSABLE
  gtk_container_forall (GTK_CONTAINER (priv->panel), 
			set_focus_forall_cb,
                        GINT_TO_POINTER (focus));
#endif

  if (focus)
    gtk_window_present   (GTK_WINDOW (window));
  else
    gtk_window_set_focus (GTK_WINDOW (window), NULL);
   
  priv->focus = focus;
}

GtkWidget *
hn_window_get_others_menu (HildonNavigatorWindow *window)
{
  HildonNavigatorWindowPrivate *priv;

  g_return_val_if_fail (window, NULL);
  g_return_val_if_fail (HILDON_IS_NAVIGATOR_WINDOW (window), NULL);

  priv = HILDON_NAVIGATOR_WINDOW_GET_PRIVATE (window);

  return priv->others_button;
}

void 
hn_window_close_others_menu (HildonNavigatorWindow *window)
{
  HildonNavigatorWindowPrivate *priv;

  g_return_if_fail (window);
  g_return_if_fail (HILDON_IS_NAVIGATOR_WINDOW (window));

  priv = HILDON_NAVIGATOR_WINDOW_GET_PRIVATE (window);

  gtk_toggle_button_set_active 
    (GTK_TOGGLE_BUTTON (priv->others_button),FALSE);

  hn_others_button_close_menu 
    (HN_OTHERS_BUTTON (priv->others_button));
}


GtkWidget *
hn_window_get_button_focus (HildonNavigatorWindow *window, gint what)
{
  GList *plugins;
  GtkWidget *ret = NULL;
  HildonNavigatorWindowPrivate *priv;
	
  g_return_val_if_fail (what < HN_TN_ACTIVATE_LAST, NULL);
  g_return_val_if_fail (window, NULL);
  g_return_val_if_fail (HILDON_IS_NAVIGATOR_WINDOW (window),NULL);

  priv = HILDON_NAVIGATOR_WINDOW_GET_PRIVATE (window);

  switch (what)
  {
    case HN_TN_ACTIVATE_OTHERS_MENU:
     ret = hn_window_get_others_menu (window);
     break;
    case HN_TN_ACTIVATE_MAIN_MENU:
     break;
    default:
     plugins = 
       hn_panel_peek_plugins (HILDON_NAVIGATOR_PANEL (priv->panel));

     if (plugins)
       ret = GTK_WIDGET (plugins->data);

     g_list_free (plugins);
     break;
  }

  return ret;
}

