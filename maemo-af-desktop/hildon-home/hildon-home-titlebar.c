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
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <libosso.h>
#include <libmb/mbutil.h>
#include <glob.h>
#include <osso-log.h>
#include <osso-helplib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <glib-object.h>
#include <glib/gi18n.h>

#include <gtk/gtkalignment.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenushell.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkmain.h>

#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-color-button.h>

#include "hildon-home-titlebar.h"
#include "hildon-home-common.h"
#include "hildon-home-private.h"
#include "hildon-home-background-dialog.h"
#include "hildon-home-area.h"
#include "hildon-home-window.h"

#define HILDON_HOME_TITLEBAR_WIDTH	720
#define HILDON_HOME_TITLEBAR_HEIGHT	60
#define HILDON_HOME_TITLEBAR_MENU_LABEL_FONT  "osso-TitleFont"
#define HILDON_HOME_TITLEBAR_MENU_LABEL_COLOR "TitleTextColor"

#define HILDON_HOME_MENU_WIDTH          348

#define PADDING_TOP  12
#define PADDING_LEFT 35

#define LAYOUT_MODE_CANCEL_BUTTON	 "qgn_indi_home_layout_reject"
#define LAYOUT_MODE_ACCEPT_BUTTON	 "qgn_indi_home_layout_accept"
#define LAYOUT_MODE_BUTTON_SIZE		 32
#define LAYOUT_MODE_BUTTON_SPACING       8
#define LAYOUT_MODE_BUTTON_PADDING_TOP	 15
#define LAYOUT_MODE_BUTTON_PADDING_RIGHT 3

#define LAYOUT_MENU_ITEM_SELECT_APPLETS     _("home_me_layout_select_applets")
#define LAYOUT_MENU_ITEM_ACCEPT_LAYOUT      _("home_me_layout_accept_layout")
#define LAYOUT_MENU_ITEM_CANCEL_LAYOUT      _("home_me_layout_cancel")
#define LAYOUT_MENU_ITEM_HELP               _("home_me_layout_help")

#define LAYOUT_MODE_MENU_LABEL_NAME         _("home_ti_layout_mode")

GType
hildon_home_titlebar_mode_get_type (void)
{
  static GType etype = 0;

  if (!etype)
    {
      static const GEnumValue values[] =
      {
        { HILDON_HOME_TITLEBAR_NORMAL, "HILDON_HOME_TITLEBAR_NORMAL", "normal" },
	{ HILDON_HOME_TITLEBAR_LAYOUT, "HILDON_HOME_TITLEBAR_LAYOUT", "layout" },
	{ 0, NULL, NULL }
      };

      etype = g_enum_register_static ("HildonHomeTitlebarMode", values);
    }

  return etype;
}

#define HILDON_HOME_TITLEBAR_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_TYPE_HOME_TITLEBAR, HildonHomeTitlebarPrivate))

struct _HildonHomeTitlebarPrivate
{
  HildonHomeTitlebarMode mode;
  
  osso_context_t *osso_context;
  
  GtkWidget *label;

  gchar *normal_text;
  gchar *layout_text;
  
  GtkWidget *menu;
  GtkWidget *tools_menu;
  GtkWidget *settings_menu;

  GtkWidget *select_applets_item;
  GtkWidget *layout_mode_item;
  GtkWidget *settings_item;

  GtkWidget *layout_menu;

  GtkWidget *layout_cancel;
  GtkWidget *layout_accept;

  guint menu_key_pressed : 1;
  guint menu_popup_status : 1;
};

enum
{
  PROP_0,

  PROP_OSSO_CONTEXT,
  PROP_MODE
};

enum
{
  SELECT_APPLETS_ACTIVATE,
  LAYOUT_MODE_ACTIVATE,
  APPLET_ACTIVATE,
  HELP_ACTIVATE,
  LAYOUT_ACCEPT,
  LAYOUT_CANCEL,
  APPLET_ADDED,
  APPLET_REMOVED,

  LAST_SIGNAL
};

G_DEFINE_TYPE (HildonHomeTitlebar, hildon_home_titlebar, GTK_TYPE_EVENT_BOX);

static guint titlebar_signals[LAST_SIGNAL] = { 0 };

static void
hildon_home_titlebar_finalize (GObject *object)
{
  HildonHomeTitlebarPrivate *priv = HILDON_HOME_TITLEBAR (object)->priv;

  g_free (priv->normal_text);
  g_free (priv->layout_text);

  G_OBJECT_CLASS (hildon_home_titlebar_parent_class)->finalize (object);
}

static void
hildon_home_titlebar_destroy (GtkObject *object)
{
  HildonHomeTitlebar *titlebar = HILDON_HOME_TITLEBAR (object);

  if (titlebar->priv->menu)
    gtk_menu_detach (GTK_MENU (titlebar->priv->menu));
  
  if (titlebar->priv->layout_menu)
    gtk_menu_detach (GTK_MENU (titlebar->priv->layout_menu));

  GTK_OBJECT_CLASS (hildon_home_titlebar_parent_class)->destroy (object);
}

static void
titlebar_menu_deactivate_cb (GtkWidget *widget,
                             gpointer   user_data)
{
  HildonHomeTitlebar *titlebar = HILDON_HOME_TITLEBAR (user_data);

  titlebar->priv->menu_key_pressed = FALSE;
  titlebar->priv->menu_popup_status = FALSE;
}

static void
titlebar_normal_menu_detach (GtkWidget *widget,
                             GtkMenu   *menu)
{
  HildonHomeTitlebar *titlebar = HILDON_HOME_TITLEBAR (widget);

  titlebar->priv->menu = NULL;
}

static void
titlebar_layout_menu_detach (GtkWidget *widget,
                             GtkMenu   *menu)
{
  HildonHomeTitlebar *titlebar = HILDON_HOME_TITLEBAR (widget);

  titlebar->priv->layout_menu = NULL;
}


static void
titlebar_menu_position_func (GtkMenu  *menu,
                             gint     *x,
                             gint     *y,
                             gboolean *push_in,
                             gpointer  user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  gint h_offset, v_offset;
  
  gdk_window_get_origin (widget->window, x, y);
  gtk_widget_style_get (GTK_WIDGET (menu),
			"horizontal-offset", &h_offset,
			"vertical-offset", &v_offset,
			NULL);
 
  *x += widget->allocation.x + h_offset;
  *y += widget->allocation.y + widget->allocation.height + v_offset;

  *push_in = FALSE;
}

static void
select_applets_activate_cb (GtkMenuItem *menu_item,
                            gpointer     data)
{
  g_signal_emit (data, titlebar_signals[SELECT_APPLETS_ACTIVATE], 0);
}

static gboolean
settings_insensitive_press_cb (GtkWidget *widget,
			       GdkEvent  *event,
			       gpointer   user_data)
{
  hildon_banner_show_information (NULL, NULL,
		  		  HILDON_HOME_MENU_APPLET_SETTINGS_NOAVAIL);
  
  return TRUE;
}

static void
layout_accept_activate_cb (GtkWidget *widget,
			  gpointer   data)
{
  g_signal_emit (data, titlebar_signals[LAYOUT_ACCEPT], 0);
}

static void
layout_cancel_clicked_cb (GtkWidget *widget,
			  gpointer   data)
{
  g_signal_emit (data, titlebar_signals[LAYOUT_CANCEL], 0);
}

static void
layout_mode_activate_cb (GtkWidget *widget,
			 gpointer   data)
{
  g_signal_emit (data, titlebar_signals[LAYOUT_MODE_ACTIVATE], 0);
}

static gboolean
layout_mode_insensitive_press_cb (GtkWidget *widget,
				  GdkEvent  *event,
				  gpointer   user_data)
{
  hildon_banner_show_information (NULL, NULL,
		  		  HILDON_HOME_MENU_EDIT_LAYOUT_NOAVAIL);

  return TRUE;
}

static void
execute_cp_applet (HildonHomeTitlebar *titlebar,
		   gpointer            user_data)
{
  gchar *applet_name = user_data;
  GKeyFile *launcher_file;
  GError *error;
  gchar *launcher_path;
  gchar *applet_path;

  launcher_path = g_build_filename (CONTROLPANEL_ENTRY_DIR,
		  		    applet_name,
				    NULL);

  g_debug ("loading appled [%s]", launcher_path);
  
  launcher_file = g_key_file_new ();

  error = NULL;
  g_key_file_load_from_file (launcher_file,
		             launcher_path,
			     G_KEY_FILE_NONE,
			     &error);
  if (error)
    {
      ULOG_ERR ("Could not start control panel applet `%s': %s\n",
		applet_name,
		error->message);
      
      g_debug ("Could not start control panel applet `%s': %s\n",
	       applet_name,
	       error->message);
      
      g_error_free (error);
      g_key_file_free (launcher_file);
      g_free (launcher_path);

      return;
    }

  applet_path = g_key_file_get_string (launcher_file,
		  		       BG_DESKTOP_GROUP,
				       "X-control-panel-plugin",
				       &error);
  if (!applet_path || error)
    {
      ULOG_ERR ("Could not start control panel applet `%s': %s\n",
		applet_name,
		error ? error->message
		      : "X-control-panel-plugin not present");

      g_debug ("Could not start control panel applet `%s': %s\n",
	       applet_name,
	       error ? error->message
	       : "X-control-panel-plugin not present");

      g_error_free (error);
      g_key_file_free (launcher_file);
      g_free (launcher_path);

      return;
    }
  
  g_signal_emit (titlebar, titlebar_signals[APPLET_ACTIVATE], 0,
		 applet_path);

  g_free (applet_path);
  g_key_file_free (launcher_file);
  g_free (launcher_path);
}

static void
personalisation_activate_cb (GtkMenuItem *item,
			     gpointer     user_data)
{
  execute_cp_applet (HILDON_HOME_TITLEBAR (user_data),
		     HILDON_CP_PLUGIN_PERSONALISATION);
}

static void
set_background_activate_cb (GtkMenuItem *item,
			    gpointer     user_data)
{
  hildon_home_window_set_desktop_dimmed (HILDON_HOME_WINDOW (user_data),
                                         TRUE);
  home_bgd_dialog_run (GTK_WINDOW (user_data));
  hildon_home_window_set_desktop_dimmed (HILDON_HOME_WINDOW (user_data),
                                         FALSE);
}

static void
calibration_activate_cb (GtkMenuItem *item,
			 gpointer     user_data)
{
  execute_cp_applet (HILDON_HOME_TITLEBAR (user_data),
		     HILDON_CP_PLUGIN_CALIBRATION);
}

static void
help_activate_cb (GtkMenuItem *menu_item,
		  gpointer     user_data)
{
  g_debug ("Help activated");
  
  g_signal_emit (user_data, titlebar_signals[HELP_ACTIVATE], 0);
}

static void
build_layout_mode_titlebar_menu (HildonHomeTitlebar *titlebar)
{
  HildonHomeTitlebarPrivate *priv;
  GtkWidget *menu;
  GtkWidget *mi;

  priv = titlebar->priv;

  menu = gtk_menu_new ();
  gtk_widget_set_name (menu, HILDON_HOME_TITLEBAR_MENU_NAME);
  gtk_menu_attach_to_widget (GTK_MENU (menu),
                             GTK_WIDGET (titlebar),
                             titlebar_layout_menu_detach);

  mi = gtk_menu_item_new_with_label (LAYOUT_MENU_ITEM_SELECT_APPLETS);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect (mi, "activate",
                    G_CALLBACK (select_applets_activate_cb),
                   titlebar);
  gtk_widget_show (mi);

  mi = gtk_menu_item_new_with_label (LAYOUT_MENU_ITEM_ACCEPT_LAYOUT);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect (mi, "activate",
                    G_CALLBACK (layout_accept_activate_cb),
                    titlebar);
  gtk_widget_show (mi);

  mi = gtk_menu_item_new_with_label (LAYOUT_MENU_ITEM_CANCEL_LAYOUT);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect (mi, "activate",
                    G_CALLBACK (layout_cancel_clicked_cb),
                    titlebar);
  gtk_widget_show (mi);

  mi = gtk_menu_item_new_with_label (LAYOUT_MENU_ITEM_HELP);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect (mi, "activate",
                    G_CALLBACK (help_activate_cb),
                    titlebar);
  gtk_widget_show (mi);

  priv->layout_menu = menu;

}

static void
build_titlebar_menu (HildonHomeTitlebar *titlebar)
{
  HildonHomeTitlebarPrivate *priv;
  GtkWidget *menu;
  GtkWidget *tools_menu;
  GtkWidget *settings_menu;
  GtkWidget *menu_item;

  priv = titlebar->priv;

  /* make sure we get called just when the menu has
   * not already been set
   */
  g_assert (priv->menu == NULL);

  menu = gtk_menu_new ();
  gtk_widget_set_name (menu, HILDON_HOME_TITLEBAR_MENU_NAME);
  gtk_menu_attach_to_widget (GTK_MENU (menu),
                             GTK_WIDGET (titlebar),
                             titlebar_normal_menu_detach);
  g_signal_connect (menu, "deactivate",
                    G_CALLBACK (titlebar_menu_deactivate_cb),
                    titlebar);

  /* 'Select applets' menu item */
  menu_item = gtk_menu_item_new_with_label (HILDON_HOME_TITLEBAR_MENU_SELECT_APPLETS);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (menu_item, "activate",
		    G_CALLBACK (select_applets_activate_cb),
		    titlebar);
  gtk_widget_show (menu_item);
  priv->select_applets_item = menu_item;

  /* Applet settings menu item */
  settings_menu = gtk_menu_new ();
  menu_item = gtk_menu_item_new_with_label (HILDON_HOME_TITLEBAR_MENU_APPLET_SETTINGS);
  g_signal_connect (menu_item, "insensitive-press",
		    G_CALLBACK (settings_insensitive_press_cb),
		    titlebar);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), settings_menu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_set_sensitive (menu_item, FALSE);
  gtk_widget_show (menu_item);
  priv->settings_item = menu_item;
  priv->settings_menu = settings_menu;

  /* layout mode */
  menu_item = gtk_menu_item_new_with_label (HILDON_HOME_TITLEBAR_MENU_EDIT_LAYOUT);
  g_signal_connect (menu_item, "activate",
		    G_CALLBACK (layout_mode_activate_cb),
		    titlebar);
  g_signal_connect (menu_item, "insensitive-press",
		    G_CALLBACK (layout_mode_insensitive_press_cb),
		    titlebar);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_set_sensitive (menu_item, FALSE);
  gtk_widget_show (menu_item);
  priv->layout_mode_item = menu_item;
  
  /* tools sub-menu */
  tools_menu = gtk_menu_new ();
  menu_item = gtk_menu_item_new_with_label (HILDON_HOME_TITLEBAR_MENU_TOOLS);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), tools_menu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);

  /* set background */
  menu_item = gtk_menu_item_new_with_label (HILDON_HOME_TITLEBAR_SUB_SET_BG);
  g_signal_connect (menu_item, "activate",
		    G_CALLBACK (set_background_activate_cb),
		    gtk_widget_get_toplevel (GTK_WIDGET (titlebar)));
  gtk_menu_shell_append (GTK_MENU_SHELL (tools_menu), menu_item);
  gtk_widget_show (menu_item);
  
  /* personalisation */
  menu_item = gtk_menu_item_new_with_label (HILDON_HOME_TITLEBAR_SUB_PERSONALISATION);
  g_signal_connect (menu_item, "activate",
		    G_CALLBACK (personalisation_activate_cb),
		    titlebar);
  gtk_menu_shell_append (GTK_MENU_SHELL (tools_menu), menu_item);
  gtk_widget_show (menu_item);

  /* calibration */
  menu_item = gtk_menu_item_new_with_label (HILDON_HOME_TITLEBAR_SUB_CALIBRATION);
  g_signal_connect (menu_item, "activate",
		    G_CALLBACK (calibration_activate_cb),
		    titlebar);
  gtk_menu_shell_append (GTK_MENU_SHELL (tools_menu), menu_item);
  gtk_widget_show (menu_item);

  /* help */
  menu_item = gtk_menu_item_new_with_label (HILDON_HOME_TITLEBAR_SUB_HELP);
  gtk_menu_shell_append (GTK_MENU_SHELL (tools_menu), menu_item);
  g_signal_connect (menu_item, "activate",
		    G_CALLBACK (help_activate_cb),
		    titlebar);
  gtk_widget_show (menu_item);

  priv->tools_menu = tools_menu;

  priv->menu = menu;
}

static void
ensure_titlebar_menu_status (HildonHomeTitlebar *titlebar,
                             HildonHomeArea *area)
{
  HildonHomeTitlebarPrivate *priv = titlebar->priv;
  gboolean settings_item_active = FALSE;
  GList *items, *l;

  if (!priv->menu)
    build_titlebar_menu (titlebar);

  /* remove old children from the settings menu */
  items = gtk_container_get_children (GTK_CONTAINER (priv->settings_menu));

  for (l = items; l != NULL; l = l->next)
    {
      GtkWidget *item = GTK_WIDGET (l->data);

      gtk_container_remove (GTK_CONTAINER (priv->settings_menu), item);
    }

  g_list_free (items);
  items = NULL;

  items = gtk_container_get_children (GTK_CONTAINER (area));
  if (!items)
    {
      /* if no applets are set we disable the layout_mode item
       * and the settings item.
       */
      gtk_widget_set_sensitive (priv->layout_mode_item, FALSE);
      gtk_widget_set_sensitive (priv->settings_item, FALSE);
      return;
    }

  for (l = items; l != NULL; l = l->next)
    {
      HildonHomeApplet *applet;
      GtkWidget *item;
  
      if (!HILDON_IS_HOME_APPLET (l->data))
        continue;
      applet = HILDON_HOME_APPLET (l->data);

      item = hildon_home_applet_get_settings_menu_item (applet);
      if (item && GTK_IS_MENU_ITEM (item))
        {
          gtk_menu_append (GTK_MENU (priv->settings_menu), item);
          gtk_widget_show (item);

          settings_item_active = TRUE;
        }
    }

  gtk_widget_set_sensitive (priv->layout_mode_item, TRUE);
  gtk_widget_set_sensitive (priv->settings_item, settings_item_active);
  g_list_free (items);
}

static gboolean
hildon_home_titlebar_button_press_event (GtkWidget      *widget,
                                         GdkEventButton *event)
{
  HildonHomeTitlebar *titlebar;
  gdouble x_win, y_win;

  /* Make sure we don't step over the end of the title bar */
  gdk_event_get_coords ((GdkEvent *) event, &x_win, &y_win);
  if (x_win >= HILDON_HOME_MENU_WIDTH)
    return FALSE;

  titlebar = HILDON_HOME_TITLEBAR (widget);
  hildon_home_titlebar_toggle_menu (titlebar);
  return TRUE;
}

static gboolean
hildon_home_titlebar_button_release_event (GtkWidget       *widget,
                                           GdkEventButton  *event)
{
  HildonHomeTitlebar *titlebar = HILDON_HOME_TITLEBAR (widget);
  gdouble x_win, y_win;

  gdk_event_get_coords ((GdkEvent *) event, &x_win, &y_win);

  if (x_win < HILDON_HOME_MENUAREA_LMARGIN ||
      x_win > HILDON_HOME_MENUAREA_WIDTH + HILDON_HOME_MENUAREA_LMARGIN)
    {
      return TRUE;
    }
      
  hildon_home_titlebar_toggle_menu (titlebar);
  
  return FALSE;
}

static void
hildon_home_titlebar_set_property (GObject      *gobject,
				   guint         prop_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
  HildonHomeTitlebar *titlebar = HILDON_HOME_TITLEBAR (gobject);

  switch (prop_id)
    {
    case PROP_OSSO_CONTEXT:
      titlebar->priv->osso_context = g_value_get_pointer (value);
      break;
    case PROP_MODE:
      hildon_home_titlebar_set_mode (titlebar,
		                     g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
hildon_home_titlebar_get_property (GObject    *gobject,
				   guint       prop_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
  HildonHomeTitlebar *titlebar = HILDON_HOME_TITLEBAR (gobject);

  switch (prop_id)
    {
    case PROP_OSSO_CONTEXT:
      g_value_set_pointer (value, titlebar->priv->osso_context);
      break;
    case PROP_MODE:
      g_value_set_enum (value, titlebar->priv->mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
hildon_home_titlebar_applet_added (HildonHomeTitlebar *titlebar,
                                   HildonHomeArea     *area)
{
  ensure_titlebar_menu_status (titlebar, area);
}

static void
hildon_home_titlebar_applet_removed (HildonHomeTitlebar *titlebar,
                                     HildonHomeArea     *area)
{
  ensure_titlebar_menu_status (titlebar, area);
}

    static void
hildon_home_titlebar_class_init (HildonHomeTitlebarClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->finalize = hildon_home_titlebar_finalize;
  gobject_class->set_property = hildon_home_titlebar_set_property;
  gobject_class->get_property = hildon_home_titlebar_get_property;

  object_class->destroy = hildon_home_titlebar_destroy;

  widget_class->button_press_event = hildon_home_titlebar_button_press_event;
  widget_class->button_release_event = hildon_home_titlebar_button_release_event;

  klass->applet_added = hildon_home_titlebar_applet_added;
  klass->applet_removed = hildon_home_titlebar_applet_removed;

  g_object_class_install_property (gobject_class,
                                   PROP_OSSO_CONTEXT,
                                   g_param_spec_pointer ("osso-context",
                                                         "Osso Context",
                                                         "Osso context to be used",
                                                         (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));
  g_object_class_install_property (gobject_class,
                                   PROP_MODE,
                                   g_param_spec_enum ("mode",
                                                      "Mode",
                                                      "Titlebar mode",
                                                      HILDON_TYPE_HOME_TITLEBAR_MODE,
                                                      HILDON_HOME_TITLEBAR_NORMAL,
                                                      G_PARAM_READWRITE));

  titlebar_signals[SELECT_APPLETS_ACTIVATE] =
      g_signal_new ("select-applets-activate",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (HildonHomeTitlebarClass, select_applets_activate),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);
  titlebar_signals[LAYOUT_MODE_ACTIVATE] =
      g_signal_new ("layout-mode-activate",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (HildonHomeTitlebarClass, layout_mode_activate),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);
  titlebar_signals[APPLET_ACTIVATE] =
      g_signal_new ("applet-activate",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (HildonHomeTitlebarClass, applet_activate),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__STRING,
                    G_TYPE_NONE,
                    1, G_TYPE_STRING);
  titlebar_signals[HELP_ACTIVATE] =
      g_signal_new ("help-activate",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (HildonHomeTitlebarClass, help_activate),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);
  titlebar_signals[LAYOUT_ACCEPT] =
      g_signal_new ("layout-accept",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (HildonHomeTitlebarClass, layout_accept),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);
  titlebar_signals[LAYOUT_CANCEL] =
      g_signal_new ("layout-cancel",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (HildonHomeTitlebarClass, layout_cancel),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);

  titlebar_signals[APPLET_ADDED] =
      g_signal_new ("applet-added",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (HildonHomeTitlebarClass, applet_added),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__POINTER,
                    G_TYPE_NONE,
                    1,
                    HILDON_TYPE_HOME_AREA);

  titlebar_signals[APPLET_REMOVED] =
      g_signal_new ("applet-removed",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (HildonHomeTitlebarClass, applet_removed),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__POINTER,
                    G_TYPE_NONE,
                    1,
                    HILDON_TYPE_HOME_AREA);

  g_type_class_add_private (gobject_class, sizeof (HildonHomeTitlebarPrivate));
}

    static void
hildon_home_titlebar_init (HildonHomeTitlebar *titlebar)
{
  HildonHomeTitlebarPrivate *priv;
  GtkWidget *align;
  GtkWidget *hbox;

  titlebar->priv = priv = HILDON_HOME_TITLEBAR_GET_PRIVATE (titlebar);

  priv->mode = HILDON_HOME_TITLEBAR_NORMAL;
  priv->menu_key_pressed = FALSE;
  priv->menu_popup_status = FALSE;

  priv->osso_context = NULL;

  gtk_widget_set_size_request (GTK_WIDGET (titlebar),
                               HILDON_HOME_TITLEBAR_WIDTH,
                               HILDON_HOME_TITLEBAR_HEIGHT);

  gtk_event_box_set_visible_window (GTK_EVENT_BOX (titlebar), FALSE);

  gtk_widget_push_composite_child ();

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_composite_name (hbox, "hildon-home-titlebar-box");
  gtk_container_add (GTK_CONTAINER (titlebar), hbox);
  gtk_widget_show (hbox);

  align = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_widget_set_composite_name (align, "hildon-home-titlebar-align");
  gtk_alignment_set_padding (GTK_ALIGNMENT (align),
                             PADDING_TOP, 0,
                             PADDING_LEFT, 0);
  gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);
  gtk_widget_show (align);

  priv->normal_text = g_strdup (HILDON_HOME_TITLEBAR_MENU_LABEL);

  priv->label = gtk_label_new (priv->normal_text);
  gtk_widget_set_composite_name (priv->label, "hildon-home-titlebar-label");
  gtk_misc_set_alignment (GTK_MISC (priv->label), 0.0, 0.5);
  hildon_gtk_widget_set_logical_font (priv->label,
                                      HILDON_HOME_TITLEBAR_MENU_LABEL_FONT);
  hildon_gtk_widget_set_logical_color (priv->label,
                                       GTK_RC_FG,
                                       GTK_STATE_NORMAL,
                                       HILDON_HOME_TITLEBAR_MENU_LABEL_COLOR);
  gtk_container_add (GTK_CONTAINER (align), priv->label);
  gtk_widget_show (priv->label);

  align = gtk_alignment_new (1.0, 0.0, 0.0, 0.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align),
                             LAYOUT_MODE_BUTTON_PADDING_TOP,
                             0,  /* layout padding bottom */
                             0, /* layout padding left */
                             LAYOUT_MODE_BUTTON_PADDING_RIGHT);
  gtk_box_pack_end (GTK_BOX (hbox), align, FALSE, FALSE, 0);
  gtk_widget_show (align);

  hbox = gtk_hbox_new (FALSE, LAYOUT_MODE_BUTTON_SPACING);
  gtk_container_add (GTK_CONTAINER (align), hbox);
  gtk_widget_show (hbox);

  priv->layout_accept = gtk_button_new_with_label ("");
  g_object_set (priv->layout_accept,
                "image", gtk_image_new_from_icon_name (LAYOUT_MODE_ACCEPT_BUTTON,
                                                       GTK_ICON_SIZE_BUTTON),
                NULL);
  gtk_widget_set_size_request (priv->layout_accept,
                               LAYOUT_MODE_BUTTON_SIZE,
                               LAYOUT_MODE_BUTTON_SIZE);
  g_signal_connect (priv->layout_accept, "clicked",
                    G_CALLBACK (layout_accept_activate_cb),
                    titlebar);
  gtk_box_pack_start (GTK_BOX (hbox), priv->layout_accept, FALSE, FALSE, 0);

  priv->layout_cancel = gtk_button_new_with_label ("");
  g_object_set (priv->layout_cancel,
                "image", gtk_image_new_from_icon_name (LAYOUT_MODE_CANCEL_BUTTON,
                                                       GTK_ICON_SIZE_BUTTON),
                NULL);
  gtk_widget_set_size_request (priv->layout_cancel,
                               LAYOUT_MODE_BUTTON_SIZE,
                               LAYOUT_MODE_BUTTON_SIZE);
  g_signal_connect (priv->layout_cancel, "clicked",
                    G_CALLBACK (layout_cancel_clicked_cb),
                    titlebar);
  gtk_box_pack_start (GTK_BOX (hbox), priv->layout_cancel, FALSE, FALSE, 0);

  priv->layout_text = g_strdup (LAYOUT_MODE_MENU_LABEL_NAME);

  gtk_widget_pop_composite_child ();
}

GtkWidget *
hildon_home_titlebar_new (osso_context_t *context)
{
  return g_object_new (HILDON_TYPE_HOME_TITLEBAR,
		       "osso-context", context,
		       NULL);
}


void
hildon_home_titlebar_set_mode (HildonHomeTitlebar     *titlebar,
                               HildonHomeTitlebarMode  mode)
{
  HildonHomeTitlebarPrivate *priv;

  g_return_if_fail (HILDON_IS_HOME_TITLEBAR (titlebar));

  priv = titlebar->priv;

  if (priv->mode != mode)
    {
      g_object_ref (titlebar);

      priv->mode = mode;

      switch (mode)
        {
          case HILDON_HOME_TITLEBAR_NORMAL:
              gtk_label_set_text (GTK_LABEL (priv->label),
                                  priv->normal_text);

              gtk_widget_hide (priv->layout_accept);
              gtk_widget_hide (priv->layout_cancel);
              break;
          case HILDON_HOME_TITLEBAR_LAYOUT:
              gtk_label_set_text (GTK_LABEL (priv->label),
                                  priv->layout_text);

              gtk_widget_show_all (priv->layout_accept);
              gtk_widget_show_all (priv->layout_cancel);
              gtk_widget_grab_focus (priv->layout_cancel);
              break;
          default:
              g_assert_not_reached ();
              break;
        }

      g_object_notify (G_OBJECT (titlebar), "mode");
      g_object_unref (titlebar);
    }
}

void
hildon_home_titlebar_toggle_menu (HildonHomeTitlebar *titlebar)
{
  GtkMenu * menu = NULL;
  HildonHomeTitlebarPrivate *priv = titlebar->priv;

  switch (priv->mode)
    {
      case HILDON_HOME_TITLEBAR_NORMAL:
          if (!priv->menu)
            build_titlebar_menu (titlebar);

          menu = GTK_MENU (priv->menu);
          break;
      case HILDON_HOME_TITLEBAR_LAYOUT:
          if (!priv->layout_menu)
            build_layout_mode_titlebar_menu (titlebar);

          menu = GTK_MENU (priv->layout_menu);
          break;
      default:
          g_assert_not_reached ();
          break;
    }

  if (!menu)
    {
      g_warning ("Titlebar mode `%s', but no menu defined",
                 priv->mode == HILDON_HOME_TITLEBAR_NORMAL ? "normal"
                 : "layout");

      return;
    }

  if (!GTK_WIDGET_MAPPED (GTK_WIDGET (menu)))
    {
      GtkRequisition req;

      /* force resizing */
      gtk_widget_set_size_request (GTK_WIDGET (menu), -1, -1);
      gtk_widget_size_request (GTK_WIDGET (menu), &req);
      gtk_widget_set_size_request (GTK_WIDGET (menu),
                                   MIN (req.width, HILDON_HOME_MENU_WIDTH),
                                   -1);

      gtk_menu_popup (menu,
                      NULL, NULL,
                      titlebar_menu_position_func, titlebar,
                      0,
                      gtk_get_current_event_time());
      titlebar->priv->menu_popup_status = TRUE;

    }
  else
    {
      gtk_menu_popdown (menu);
      titlebar->priv->menu_popup_status = FALSE;
    }

}
