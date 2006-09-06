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
#include "others-menu.h"

#include "hn-wm.h"
#include "kstrace.h"

/* Global external variables */
gboolean config_do_bgkill;
gboolean config_lowmem_dim;
gboolean config_lowmem_notify_enter;
gboolean config_lowmem_notify_leave;
gboolean config_lowmem_pavlov_dialog;
gboolean tn_is_sensitive=TRUE;

typedef struct _HildonNavigatorWindowPrivate HildonNavigatorWindowPrivate;

struct _HildonNavigatorWindowPrivate
{
  GtkWidget *panel;
  GtkWidget *app_switcher;

  OthersMenu_t *others_menu;

  void *app_switcher_dnotify_cb;

  gboolean focus;
};

/* static declarations */
static void hildon_navigator_window_class_init (HildonNavigatorWindowClass 
						*hnwindow_class);

static gboolean getenv_yesno(const char *env, gboolean def);
static void set_focus_forall_cb (GtkWidget *widget, gpointer data);
static void configuration_changed (char *path, gpointer *data);
static void initialize_navigator_menus (HildonNavigatorWindowPrivate *priv);
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

  g_type_class_add_private (hnwindow_class, sizeof (HildonNavigatorWindowPrivate));
  
  object_class->finalize          = hildon_navigator_window_finalize;
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
    gchar *config_file, *home_dir; 
    HildonNavigatorPanel *panel;

    g_assert (data != NULL);

    home_dir = getenv ("HOME");

    panel = (HildonNavigatorPanel *)data;

    config_file = g_strdup_printf("%s%s", home_dir, NAVIGATOR_USER_PLUGINS);
    hn_panel_unload_all_plugins (panel,TRUE);
    hn_panel_load_plugins_from_file (panel, config_file);

    g_free (config_file);
    g_free (home_dir);
}

static void
initialize_navigator_menus (HildonNavigatorWindowPrivate *priv)
{
  g_assert (priv != NULL);
#if 0
  /*Initialize application switcher menu*/
  application_switcher_initialize_menu(tasknav->app_switcher);
#endif
  priv->app_switcher_dnotify_cb = hn_wm_dnotify_cb;

  /* Initialize others menu */
  others_menu_initialize_menu (priv->others_menu, hn_wm_dnotify_cb);
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

  others_menu_deinit (priv->others_menu);
}

static void 
hildon_navigator_window_init (HildonNavigatorWindow *window)
{
  HildonNavigatorWindowPrivate *priv;
  gchar *plugin_dir, *config_file, *home_dir;
  gboolean dnotify_ret;

  g_return_if_fail (window);

  GTK_WINDOW (window)->type = GTK_WINDOW_TOPLEVEL;

  home_dir = getenv("HOME");

  priv = HILDON_NAVIGATOR_WINDOW_GET_PRIVATE (window);

  /* Get configuration options from the environment.
     */
  config_do_bgkill = getenv_yesno("NAVIGATOR_DO_BGKILL", TRUE);
  config_lowmem_dim = getenv_yesno("NAVIGATOR_LOWMEM_DIM", FALSE);
  config_lowmem_notify_enter = getenv_yesno("NAVIGATOR_LOWMEM_NOTIFY_ENTER", FALSE);
  config_lowmem_notify_leave = getenv_yesno("NAVIGATOR_LOWMEM_NOTIFY_LEAVE", FALSE);
  config_lowmem_pavlov_dialog = getenv_yesno("NAVIGATOR_LOWMEM_PAVLOV_DIALOG", FALSE);

  priv->panel = GTK_WIDGET (hildon_navigator_panel_new ());

  /*FIXME: we inherit from gtk_window!*/
  gtk_window_set_type_hint( GTK_WINDOW(window),GDK_WINDOW_TYPE_HINT_DOCK);
  gtk_window_set_accept_focus( GTK_WINDOW(window),TN_DEFAULT_FOCUS);

#if 0 /* This doesn't make sense here but I keep it for further investigation ;) */
	g_signal_connect (tasknav->main_window, "focus-out-event",
					  G_CALLBACK (main_window_focus_out_cb),
					  NULL);
#endif
  gtk_widget_set_size_request(GTK_WIDGET (priv->panel), 
		  	      BUTTON_WIDTH,
		              gdk_screen_height());
  gtk_container_add (GTK_CONTAINER (window),GTK_WIDGET (priv->panel));

  plugin_dir  = g_build_filename (home_dir, NAVIGATOR_USER_DIR, NULL);
  config_file = g_strdup_printf  ("%s%s", home_dir, NAVIGATOR_USER_PLUGINS);
    
  /* Initialize Plugins */
  if (g_file_test (config_file, G_FILE_TEST_EXISTS))
    hn_panel_load_plugins_from_file (HILDON_NAVIGATOR_PANEL (priv->panel), 
				     config_file);
  else
    hn_panel_load_plugins_from_file (HILDON_NAVIGATOR_PANEL (priv->panel), 
				     NAVIGATOR_FACTORY_PLUGINS);
  
  /* This is moved here since it inits the dnotify stuff and initing them
     * twice would result in an error according to
     * hildon-base-lib/hildon-base-dnotify.h
     */
  priv->others_menu = others_menu_init();

  /* Create the directory if it does not exist, as we need to monitor
     * it for the TN applet to write its configuration */
  if (!g_file_test(plugin_dir, G_FILE_TEST_IS_DIR))
    g_mkdir(plugin_dir, 0755); /* THIS WILL CHANGE! CREATE IT ANYWAY! */

  dnotify_ret = hildon_dnotify_set_cb ((hildon_dnotify_cb_f *)configuration_changed,
                	      	       plugin_dir,
				       priv->panel);
  /*if (dnotify_ret != HILDON_OK)
    osso_log( LOG_ERR, "Error setting dir notify callback!\n" );*/

  if (priv->others_menu)
    hn_panel_add_button ( HILDON_NAVIGATOR_PANEL (priv->panel),
			  others_menu_get_button (priv->others_menu));

  priv->app_switcher = hn_app_switcher_new ();
  gtk_box_pack_start ( GTK_BOX (priv->panel), 
		       priv->app_switcher, 
		       TRUE, TRUE, 0);

  initialize_navigator_menus (priv);

  priv->focus = FALSE;

  /* cleanup */
  g_free (plugin_dir);
  g_free (config_file);
  g_free (home_dir);
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

  return others_menu_get_button (priv->others_menu);
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

