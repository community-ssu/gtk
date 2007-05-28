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

#ifdef HAVE_LIBHILDONHELP
#include <hildon/hildon-help.h>
#endif

#ifdef HAVE_X_COMPOSITE
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>
#endif

#ifdef HAVE_LIBOSSO
#include <libosso.h>
#endif


#include <libhildonwm/hd-wm.h>

#include <libhildondesktop/hildon-home-area.h>
#include <libhildondesktop/hildon-desktop-home-item.h>

#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkspinbutton.h>

#ifdef HAVE_LIBHILDON
#include <hildon/hildon-banner.h>
#include <hildon/hildon-note.h>
#include <hildon/hildon-defines.h>
#define ENABLE_UNSTABLE_API
#include <hildon/hildon-helper.h>
#undef ENABLE_UNSTABLE_API
#else
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-defines.h>
#endif

#include "hd-home-l10n.h"
#include "hd-home-background.h"
#include "hd-home-background-dialog.h"

#define HCP_PLUGIN_PATH_PERSONALISATION "libpersonalisation.so"
#define HCP_PLUGIN_PATH_CALIBRATION     "tscalib.so"

#define HD_HOME_BACKGROUND_CONF_FILE        "home-background.conf"
#define HD_DESKTOP_USER_PATH                ".osso/hildon-desktop/"
#define HD_HOME_BACKGROUND_DEFAULT          HD_DESKTOP_BACKGROUNDS_PATH\
                                            "/default.desktop"

#define BG_DESKTOP_GROUP           "Desktop Entry"
#define BG_DESKTOP_IMAGE_FILENAME  "File"

#define LAYOUT_OPENING_BANNER_TIMEOUT   2500

#define HD_HOME_WINDOW_STYLE_NORTH_BORDER   "HildonHomeTitleBar"
#define HD_HOME_WINDOW_STYLE_WEST_BORDER    "HildonHomeLeftEdge"

#define HH_AREA_CONFIGURATION_FILE        ".osso/hildon-desktop/home-layout.conf"
#define HH_AREA_GLOBAL_CONFIGURATION_FILE "/etc/hildon-desktop/home-layout.conf"

#include "hd-home-window.h"

#define HD_HOME_WINDOW_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HD_TYPE_HOME_WINDOW, HDHomeWindowPrivate))

G_DEFINE_TYPE (HDHomeWindow, hd_home_window, HILDON_TYPE_HOME_WINDOW);

struct _HDHomeWindowPrivate
{
#ifdef HAVE_LIBOSSO
  osso_context_t   *osso_context;
#endif

  gboolean          layout_mode_sucks;

  GtkWidget        *normal_menu;
  GtkWidget        *layout_menu;

  GtkWidget        *settings_item;
  GtkWidget        *settings_menu;
  GtkWidget        *layout_mode_item;

  GtkWidget        *layout_mode_banner;
  gint              layout_mode_banner_to;

  gchar            *north_border;
  gchar            *west_border;

  HDHomeBackground *background;
  HDHomeBackground *previous_background;

  gboolean          is_inactive;

  gboolean          selecting_applets;
  guint             save_area_timeout;
};

/* Properties */
enum 
{
  PROP_0,
  PROP_OSSO_CONTEXT,
  PROP_LAYOUT_MODE_SUCKS,
  PROP_BACKGROUND
};

static GObject *
hd_home_window_constructor (GType                   gtype,
                            guint                   n_params,
                            GObjectConstructParam  *params);

static void
hd_home_window_get_property (GObject    *gobject,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec);

static void
hd_home_window_set_property (GObject      *gobject,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec);

static void
hd_home_window_notify (GObject     *object,
                       GParamSpec  *pspec);


#ifdef HAVE_LIBOSSO
static void
hd_home_window_set_osso_context (HDHomeWindow *window, osso_context_t *osso);

static void
hd_home_window_personalisation_activate (HDHomeWindow *window);

static void
hd_home_window_calibration_activate (HDHomeWindow *window);

#ifdef HAVE_LIBHILDONHELP
static void
hd_home_window_help_activate (HDHomeWindow *window);
#endif
#endif

static void
hd_home_window_set_background_activate (HDHomeWindow *window);

#ifndef HAVE_LIBHILDON
static void
hd_home_window_settings_insensitive_press_cb (HDHomeWindow *window);

static void
hd_home_window_layout_insensitive_press_cb (HDHomeWindow *window);
#endif

static void
hd_home_window_layout_mode_activate (HDHomeWindow *window);

static GtkWidget *
hd_home_window_build_main_menu (HDHomeWindow *window);

static GtkWidget *
hd_home_window_build_layout_menu (HDHomeWindow *window);

static void
hd_home_window_ensure_menu_status (HDHomeWindow *window);

static void
hd_home_window_cancel_layout (HDHomeWindow *window);

static void
hd_home_window_accept_layout (HDHomeWindow *window);

static void
hd_home_window_layout_mode_accept (HDHomeWindow *window);

static void
hd_home_window_layout_mode_cancel (HDHomeWindow *window);

static void
hd_home_window_snap_toggled (HDHomeWindow *window, GtkCheckMenuItem *item);

static void
hd_home_window_show_layout_mode_banner (HDHomeWindow *window);

static gboolean
hd_home_window_destroy_banner (HDHomeWindow *window);

static void
hd_home_window_set_background (HDHomeWindow *window,
                               HDHomeBackground *background);

static gboolean
hd_home_window_map_event (GtkWidget    *widget,
                          GdkEventAny  *event);

static void
hd_home_window_realize (GtkWidget *widget);

static void
hd_home_window_unrealize (GtkWidget *widget);

static gboolean
hd_home_window_key_press_event (GtkWidget *widget,
                                GdkEventKey *event);

static void
hd_home_window_style_set (GtkWidget    *widget,
                          GtkStyle     *style);

static void
hd_home_window_show_information_note (HDHomeWindow *window,
                                      const gchar *text);

#ifndef HAVE_LIBHILDON
static void
hd_home_window_show_information_banner (HDHomeWindow *window,
                                        const gchar *text);
#endif

static void
hd_home_window_background (HDHomeWindow        *window,
                           gboolean             background);

static void
hd_home_window_system_inactivity (HDHomeWindow     *window,
                                  gboolean              is_inactive);

static void
hd_home_window_area_add (HDHomeWindow *window);

static void
hd_home_window_area_remove (HDHomeWindow *window);

static void
hd_home_window_area_changed (HDHomeWindow *window);

static void
hd_home_window_layout_mode_started (HDHomeWindow *window);

static void
hd_home_window_layout_mode_ended (HDHomeWindow *window);

static void
hd_home_window_select_applets (HDHomeWindow *window);

static void
hd_home_window_load_area_layout (HDHomeWindow *window);

static void
hd_home_window_button1_clicked (HildonHomeWindow *window);

static void
hd_home_window_button2_clicked (HildonHomeWindow *window);

#if 0
static void
hd_home_window_adjust_alpha_activate (HDHomeWindow *window);
#endif

static void
hd_home_window_class_init (HDHomeWindowClass *window_class)
{
  GParamSpec               *pspec;
  GObjectClass             *object_class;
  GtkWidgetClass           *widget_class;
  HildonHomeWindowClass    *hhwindow_class;

  object_class = G_OBJECT_CLASS (window_class);
  widget_class = GTK_WIDGET_CLASS (window_class);
  hhwindow_class = HILDON_HOME_WINDOW_CLASS (window_class);

  object_class->constructor  = hd_home_window_constructor;
  object_class->set_property = hd_home_window_set_property;
  object_class->get_property = hd_home_window_get_property;
  object_class->notify       = hd_home_window_notify;

  widget_class->map_event = hd_home_window_map_event;
  widget_class->style_set = hd_home_window_style_set;
  widget_class->key_press_event = hd_home_window_key_press_event;
  widget_class->realize   = hd_home_window_realize;
  widget_class->unrealize = hd_home_window_unrealize;

  hhwindow_class->button1_clicked = hd_home_window_button1_clicked;
  hhwindow_class->button2_clicked = hd_home_window_button2_clicked;

  window_class->layout_mode_accept = hd_home_window_layout_mode_accept;
  window_class->layout_mode_cancel = hd_home_window_layout_mode_cancel;
  window_class->background         = hd_home_window_background;
  window_class->system_inactivity  = hd_home_window_system_inactivity;

  pspec = g_param_spec_pointer ("osso-context",
                                "Osso Context",
                                "Osso context to be used by the window",
                                (G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class,
                                   PROP_OSSO_CONTEXT,
                                   pspec);

  pspec = g_param_spec_object ("background",
                               "background",
                               "Current background applied",
                               HD_TYPE_HOME_BACKGROUND,
                               (G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class,
                                   PROP_BACKGROUND,
                                   pspec);

  pspec = g_param_spec_boolean ("layout-mode-sucks",
                                "Layout mode sucks",
                                "Whether or not the layout mode sucks",
                                TRUE,
                               (G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class,
                                   PROP_LAYOUT_MODE_SUCKS,
                                   pspec);

  g_signal_new ("background",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HDHomeWindowClass, background),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__BOOLEAN,
                G_TYPE_NONE,
                1,
                G_TYPE_BOOLEAN);

  g_signal_new ("system-inactivity",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HDHomeWindowClass, system_inactivity),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__BOOLEAN,
                G_TYPE_NONE,
                1,
                G_TYPE_BOOLEAN);

  g_signal_new ("lowmem",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HDHomeWindowClass, lowmem),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__BOOLEAN,
                G_TYPE_NONE,
                1,
                G_TYPE_BOOLEAN);

  g_signal_new ("layout-mode-accept",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HDHomeWindowClass, layout_mode_accept),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  g_signal_new ("layout-mode-cancel",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HDHomeWindowClass, layout_mode_cancel),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  g_signal_new ("io-error",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HDHomeWindowClass, io_error),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__POINTER,
                G_TYPE_NONE,
                1,
                G_TYPE_POINTER);




  g_type_class_add_private (window_class, sizeof (HDHomeWindowPrivate));

}

static void
hd_home_window_init (HDHomeWindow *window)
{

}

static GObject *
hd_home_window_constructor (GType                   gtype,
                            guint                   n_params,
                            GObjectConstructParam  *params)
{
  GObject              *retval;
  HDHomeWindow         *window;
  HDHomeWindowPrivate  *priv;
  HildonHomeWindow     *hhwindow;
  GtkWidget            *area;
  HDHomeBackground     *background = NULL;
  GError               *error = NULL;
  gchar                *conffile;

  retval = G_OBJECT_CLASS (hd_home_window_parent_class)->constructor (gtype,
                                                                      n_params,
                                                                      params);

  window = HD_HOME_WINDOW (retval);
  hhwindow = HILDON_HOME_WINDOW (retval);
  priv = HD_HOME_WINDOW_GET_PRIVATE (retval);

  priv->normal_menu = hd_home_window_build_main_menu (window);
  hildon_home_window_set_menu (hhwindow, GTK_MENU (priv->normal_menu));

  priv->layout_menu = hd_home_window_build_layout_menu (window);

  g_object_set (window,
                "menu", priv->normal_menu,
                "title", HH_MENU_TITLE,
                NULL);

  area = hildon_home_area_new ();
  gtk_widget_show (area);

  gtk_container_add (GTK_CONTAINER (retval), area);
  HILDON_DESKTOP_WINDOW (retval)->container = GTK_CONTAINER (area);

  g_signal_connect_swapped (area, "add",
                            G_CALLBACK (hd_home_window_area_add),
                            window);

  g_signal_connect_swapped (area, "remove",
                            G_CALLBACK (hd_home_window_area_remove),
                            window);

  g_signal_connect_swapped (area, "layout-mode-start",
                            G_CALLBACK (hd_home_window_show_layout_mode_banner),
                            window);

  g_signal_connect_swapped (area, "layout-mode-started",
                            G_CALLBACK (hd_home_window_layout_mode_started),
                            window);

  g_signal_connect_swapped (area, "layout-mode-ended",
                            G_CALLBACK (hd_home_window_layout_mode_ended),
                            window);

  if (priv->layout_mode_sucks)
    {
      g_signal_connect_swapped (area, "layout-changed",
                                G_CALLBACK (hd_home_window_area_changed),
                                window);
    }

  background = g_object_new (HD_TYPE_HOME_BACKGROUND, NULL);

  conffile = g_build_path (G_DIR_SEPARATOR_S,
                           g_get_home_dir (),
                           HD_DESKTOP_USER_PATH,
                           HD_HOME_BACKGROUND_CONF_FILE,
                           NULL);

  hd_home_background_load (background,
                           conffile,
                           &error);

  g_free (conffile);

  if (error)
    {
      GKeyFile         *keyfile;
      const GdkColor    default_color = {0, 0, 0, 0};
      gchar            *filename = NULL;

      g_clear_error (&error);

      /* Revert to the default background file */
      keyfile = g_key_file_new ();
      g_key_file_load_from_file (keyfile,
                                 HD_HOME_BACKGROUND_DEFAULT,
                                 G_KEY_FILE_NONE,
                                 &error);
      if (error) goto error;

      filename = g_key_file_get_string (keyfile,
                                        BG_DESKTOP_GROUP,
                                        BG_DESKTOP_IMAGE_FILENAME,
                                        &error);

      if (error) goto error;

      background = g_object_new (HD_TYPE_HOME_BACKGROUND,
                                 "filename", filename,
                                 "color", &default_color,
                                 NULL);

error:
      if (error)
        {
          g_warning ("Could not load default background: %s",
                     error->message);
          g_clear_error (&error);
          background = g_object_new (HD_TYPE_HOME_BACKGROUND,
                                     "filename", HD_HOME_BACKGROUND_NO_IMAGE,
                                     "color", &default_color,
                                     NULL);
        }

      g_free (filename);

    }


  if (background)
    hd_home_window_set_background (window, background);

  hd_home_window_load_area_layout (window);

  /* Necessary to avoid the default background to be reset */
  gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);

  return retval;
}


static void
hd_home_window_set_property (GObject      *gobject,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  HDHomeWindow *window = HD_HOME_WINDOW (gobject);
  HDHomeWindowPrivate *priv = HD_HOME_WINDOW_GET_PRIVATE (gobject);

  switch (prop_id)
  {
#ifdef HAVE_LIBOSSO
    case PROP_OSSO_CONTEXT:
      hd_home_window_set_osso_context (window,
                                       (osso_context_t *)
                                       g_value_get_pointer (value));

      break;
#endif

    case PROP_BACKGROUND:
      hd_home_window_set_background (window,
                                     g_value_get_object (value));
      break;

    case PROP_LAYOUT_MODE_SUCKS:
      priv->layout_mode_sucks = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
hd_home_window_get_property (GObject    *gobject,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  HDHomeWindowPrivate *priv = HD_HOME_WINDOW_GET_PRIVATE (gobject);

  switch (prop_id)
  {
#ifdef HAVE_LIBOSSO
    case PROP_OSSO_CONTEXT:
      g_value_set_pointer (value, priv->osso_context);
      break;
#endif

    case PROP_LAYOUT_MODE_SUCKS:
      g_value_set_boolean (value, priv->layout_mode_sucks);
      break;

    case PROP_BACKGROUND:
      g_value_set_object (value, priv->background);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
hd_home_window_notify (GObject     *object,
                       GParamSpec  *pspec)
{
  HDHomeWindowPrivate  *priv;
  priv = HD_HOME_WINDOW_GET_PRIVATE (object);

  if (g_str_equal (pspec->name, "work-area"))
    {
      g_debug ("Work area changed, reapplying the background");
      hd_home_window_set_background (HD_HOME_WINDOW (object),
                                     priv->background);
    }
}

static void
background_apply_callback (HDHomeBackground *background,
                           gint              pixmap_xid,
                           GError           *error,
                           HDHomeWindow     *window)
{
  HDHomeWindowPrivate  *priv;
  g_debug ("Background applied!");

  if (error)
    {
      g_warning ("Got error when apply background: %s",
                 error->message);
      return;
    }

  gdk_window_clear (GTK_WIDGET (window)->window);
  gtk_widget_queue_draw (GTK_WIDGET (window));

  priv = HD_HOME_WINDOW_GET_PRIVATE (window);

  if (background != priv->background)
    {
      if (priv->background)
        g_object_unref (priv->background);
      priv->background = g_object_ref (background);
    }

}

static void
background_apply_and_save_callback (HDHomeBackground *background,
                                    gint              pixmap_xid,
                                    GError           *error,
                                    HDHomeWindow     *window)
{
  HDHomeWindowPrivate  *priv;
  gchar                *conffile;
  GError               *save_error = NULL;

  background_apply_callback (background, pixmap_xid, error, window);

  /* Do not save if an error occurred */
  if (error)
    return;

  conffile = g_build_path (G_DIR_SEPARATOR_S,
                           g_get_home_dir (),
                           HD_DESKTOP_USER_PATH,
                           HD_HOME_BACKGROUND_CONF_FILE,
                           NULL);

  priv = HD_HOME_WINDOW_GET_PRIVATE (window);

  hd_home_background_save (priv->background,
                           conffile,
                           &save_error);

  if (save_error)
    {
      g_warning ("Error when saving background settings to %s: %s",
                 conffile,
                 error->message);
      g_error_free (save_error);
    }

  g_free (conffile);

}

static void
hd_home_window_realize (GtkWidget *widget)
{
  HDWM *wm;

  GTK_WIDGET_CLASS (hd_home_window_parent_class)->realize (widget);

#ifdef HAVE_X_COMPOSITE
  XCompositeRedirectWindow (GDK_DISPLAY (),
                            GDK_WINDOW_XID (widget->window),
                            CompositeRedirectAutomatic);
#endif

  wm = hd_wm_get_singleton ();
  g_object_set (wm,
                "desktop-window", (gint)GDK_WINDOW_XID (widget->window),
                NULL);

}

static void
hd_home_window_unrealize (GtkWidget *widget)
{
  HDWM *wm;
#ifdef HAVE_X_COMPOSITE
  XCompositeUnredirectWindow (GDK_DISPLAY (),
                              GDK_WINDOW_XID (widget->window),
                              CompositeRedirectAutomatic);
#endif

  wm = hd_wm_get_singleton ();
  g_object_set (wm,
                "desktop-window", (gint)None,
                NULL);

  GTK_WIDGET_CLASS (hd_home_window_parent_class)->unrealize (widget);
}

static gboolean
hd_home_window_map_event (GtkWidget    *widget,
                          GdkEventAny  *event)
{
  HDHomeWindowPrivate  *priv = HD_HOME_WINDOW_GET_PRIVATE (widget);
  GdkRectangle         *workarea;

  g_object_get (widget, "work-area", &workarea, NULL);

  if (priv->background)
    hd_home_background_apply_async (priv->background,
                                    widget->window,
                                    workarea,
                                    (HDHomeBackgroundApplyCallback)
                                    background_apply_callback,
                                    widget);

  return GTK_WIDGET_CLASS (hd_home_window_parent_class)->map_event (widget,
                                                                    event);
}

static const gchar *
hd_home_window_get_pixmap_name (HDHomeWindow *window,
                                const gchar *key)
{
  GtkStyle *style;
  GtkSettings *settings;

  settings = gtk_widget_get_settings (GTK_WIDGET (window));

  style = gtk_rc_get_style_by_paths (settings,
                                     key,
                                     NULL,
                                     G_TYPE_NONE);

  if (style && style->rc_style->bg_pixmap_name[0])
    return style->rc_style->bg_pixmap_name[0];

  return NULL;

}

static void
hd_home_window_style_set (GtkWidget *widget, GtkStyle *old_style)
{
  HDHomeWindow         *window;
  HDHomeWindowPrivate  *priv;
  const gchar          *north_border;
  const gchar          *west_border;

  window = HD_HOME_WINDOW (widget);
  priv = HD_HOME_WINDOW_GET_PRIVATE (window);

  if (GTK_WIDGET_CLASS (hd_home_window_parent_class)->style_set)
    GTK_WIDGET_CLASS (hd_home_window_parent_class)->style_set (widget,
                                                               old_style);
  north_border = hd_home_window_get_pixmap_name (window,
                                                 HD_HOME_WINDOW_STYLE_NORTH_BORDER);

  west_border  = hd_home_window_get_pixmap_name (window,
                                                 HD_HOME_WINDOW_STYLE_WEST_BORDER);


  /* avoid resetting the background when the window is exposed for the
   * first time
   */
  if (!old_style ||
      !(north_border && g_str_equal (priv->north_border, north_border)) ||
      !(west_border && g_str_equal (priv->west_border, west_border)))
    {
      if (north_border)
        priv->north_border = g_strdup (north_border);
      if (west_border)
        priv->west_border  = g_strdup (west_border);

      if (priv->background)
        {
          GValue value = {0};
          g_value_init (&value, G_TYPE_STRING);

          if (priv->north_border)
            {
              g_value_set_string (&value, priv->north_border);
              g_object_set_property (G_OBJECT (priv->background),
                                     "north-border",
                                     &value);
            }

          if (priv->west_border)
            {
              g_value_set_string (&value, priv->west_border);
              g_object_set_property (G_OBJECT (priv->background),
                                     "west-border",
                                     &value);
            }

          if (GTK_WIDGET_MAPPED (widget))
            {
              GdkRectangle *workarea;
              g_object_get (widget, "work-area", &workarea, NULL);

              hd_home_background_apply_async (priv->background,
                                              widget->window,
                                              workarea,
                                              (HDHomeBackgroundApplyCallback)
                                              background_apply_callback,
                                              widget);
            }
        }
    }
}

static gboolean
hd_home_window_key_press_event (GtkWidget *widget,
                                GdkEventKey *event)
{
  switch (event->keyval)
    {
      case HILDON_HARDKEY_MENU:
          hildon_home_window_toggle_menu (HILDON_HOME_WINDOW (widget));
          break;
      case HILDON_HARDKEY_ESC:
          hd_home_window_cancel_layout (HD_HOME_WINDOW (widget));

          break;
      default:
            return GTK_WIDGET_CLASS (
                     hd_home_window_parent_class)->key_press_event (widget,
                                                                    event);
    }

  return TRUE;
}

static GtkWidget *
hd_home_window_build_main_menu (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv;
  GtkWidget            *menu;
  GtkWidget            *settings_menu;
  GtkWidget            *menu_item;

  priv = HD_HOME_WINDOW_GET_PRIVATE (window);

  menu = gtk_menu_new ();

  /* 'Select applets' menu item */
  menu_item = gtk_menu_item_new_with_label (HH_MENU_SELECT);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect_swapped (menu_item, "activate",
                            G_CALLBACK (hd_home_window_select_applets),
                            window);
  gtk_widget_show (menu_item);
/*  priv->select_applets_item = menu_item;*/

  /* Applet settings menu item */
  settings_menu = gtk_menu_new ();
  menu_item = gtk_menu_item_new_with_label (HH_MENU_APPLET_SETTINGS);
#ifdef HAVE_LIBHILDON
  hildon_helper_set_insensitive_message (menu_item, HH_APPLET_SETTINGS_BANNER);
#else
  g_signal_connect_swapped (menu_item, "insensitive-press",
                            G_CALLBACK (hd_home_window_settings_insensitive_press_cb),
                            window);
#endif
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), settings_menu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_set_sensitive (menu_item, FALSE);
  gtk_widget_show (menu_item);
  priv->settings_item = menu_item;
  priv->settings_menu = settings_menu;

  /* set background */
  menu_item = gtk_menu_item_new_with_label (HH_MENU_SET_BACKGROUND);
  g_signal_connect_swapped (menu_item, "activate",
                            G_CALLBACK (hd_home_window_set_background_activate),
                            window);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);

#ifdef HAVE_LIBOSSO
  /* personalisation */
  menu_item = gtk_menu_item_new_with_label (HH_MENU_PERSONALISATION);
  g_signal_connect_swapped (menu_item, "activate",
                            G_CALLBACK (hd_home_window_personalisation_activate),
                            window);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);

  /* calibration */
  menu_item = gtk_menu_item_new_with_label (HH_MENU_CALIBRATION);
  g_signal_connect_swapped (menu_item, "activate",
                            G_CALLBACK (hd_home_window_calibration_activate),
                            window);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);

#ifdef HAVE_LIBHILDONHELP
  /* help */
  menu_item = gtk_menu_item_new_with_label (HH_MENU_HELP);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect_swapped (menu_item, "activate",
                            G_CALLBACK (hd_home_window_help_activate),
                            window);
  gtk_widget_show (menu_item);
#endif
#endif

  if (!priv->layout_mode_sucks)
    {
      /* layout mode */
      menu_item = gtk_menu_item_new_with_label (HH_MENU_EDIT_LAYOUT);
      g_signal_connect_swapped (menu_item, "activate",
                                G_CALLBACK (hd_home_window_layout_mode_activate),
                                window);
#ifdef HAVE_LIBHILDON
      hildon_helper_set_insensitive_message (menu_item, HH_LAYOUT_UNAVAIL_BANNER);
#else
      g_signal_connect_swapped (menu_item, "insensitive-press",
                                G_CALLBACK (hd_home_window_layout_insensitive_press_cb),
                                window);
#endif
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      gtk_widget_set_sensitive (menu_item, FALSE);
      gtk_widget_show (menu_item);
      priv->layout_mode_item = menu_item;
    }

#if 0
  else
    {
      GtkWidget *area;

      menu_item = gtk_check_menu_item_new_with_label (HH_MENU_LAYOUT_SNAP_TO_GRID);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      g_signal_connect_swapped (menu_item, "toggled",
                                G_CALLBACK (hd_home_window_snap_toggled),
                                window);
      area = hildon_home_window_get_area (HILDON_HOME_WINDOW (window));
      if (area)
        {
          gboolean snap_to_grid;
          g_object_get (G_OBJECT (area), "snap-to-grid", &snap_to_grid, NULL);
          gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item),
                                          snap_to_grid);
        }
      gtk_widget_show (menu_item);
    }

  menu_item = gtk_menu_item_new_with_label ("Adjust transparency");
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect_swapped (menu_item, "activate",
                            G_CALLBACK (hd_home_window_adjust_alpha_activate),
                            window);
  gtk_widget_show (menu_item);
#endif

  return menu;

}

static GtkWidget *
hd_home_window_build_layout_menu (HDHomeWindow *window)
{
  GtkWidget    *menu;
  GtkWidget    *mi;
  GtkWidget    *area;

  menu = gtk_menu_new ();

  mi = gtk_menu_item_new_with_label (HH_MENU_LAYOUT_SELECT);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect_swapped (mi, "activate",
                            G_CALLBACK (hd_home_window_select_applets),
                            window);
  gtk_widget_show (mi);

  mi = gtk_menu_item_new_with_label (HH_MENU_LAYOUT_ACCEPT);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect_swapped (mi, "activate",
                            G_CALLBACK (hd_home_window_accept_layout),
                            window);
  gtk_widget_show (mi);

  mi = gtk_menu_item_new_with_label (HH_MENU_LAYOUT_CANCEL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect_swapped (mi, "activate",
                            G_CALLBACK (hd_home_window_cancel_layout),
                            window);
  gtk_widget_show (mi);

  mi = gtk_check_menu_item_new_with_label (HH_MENU_LAYOUT_SNAP_TO_GRID);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect_swapped (mi, "toggled",
                            G_CALLBACK (hd_home_window_snap_toggled),
                            window);
  area = GTK_BIN (window)->child;
  if (HILDON_IS_HOME_AREA (area))
    {
      gboolean snap_to_grid;
      g_object_get (G_OBJECT (area), "snap-to-grid", &snap_to_grid, NULL);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), snap_to_grid);
    }
  gtk_widget_show (mi);

#ifdef HAVE_LIBHILDONHELP
  mi = gtk_menu_item_new_with_label (HH_MENU_LAYOUT_HELP);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect_swapped (mi, "activate",
                            G_CALLBACK (hd_home_window_help_activate),
                            window);
  gtk_widget_show (mi);
#endif

  return menu;
}

#ifdef HAVE_LIBOSSO
static void
hd_home_window_applet_activate (HDHomeWindow *window,
                                const gchar *applet_path)
{
  HDHomeWindowPrivate  *priv ;
  osso_return_t         res;

  priv = HD_HOME_WINDOW_GET_PRIVATE (window);

  if (!priv->osso_context)
    return;

  g_debug ("Launching controlpanel applet %s", applet_path);
  res = osso_cp_plugin_execute (priv->osso_context,
                                applet_path,
                                NULL,
                                TRUE);
  switch (res)
  {
    case OSSO_OK:
      break;

    case OSSO_ERROR:
      g_warning ("OSSO_ERROR (No plugin found)\n");
      break;

    case OSSO_RPC_ERROR:
      g_warning ("OSSO_RPC_ERROR (RPC failed for plugin)\n");
      break;

    case OSSO_INVALID:
      g_warning ("OSSO_INVALID (Invalid argument)\n");
      break;

    default:
      g_warning ("Unknown error!\n");
      break;
  }
}
#endif

#ifdef HAVE_LIBHILDONHELP
static void
hd_home_window_help_activate (HDHomeWindow *window)
{
  HildonDesktopWindow  *dwindow = HILDON_DESKTOP_WINDOW (window);
  HDHomeWindowPrivate  *priv;
  HildonHomeArea       *area;
  osso_return_t         res;

  g_return_if_fail (HILDON_IS_HOME_AREA (dwindow->container));
  area = HILDON_HOME_AREA (dwindow->container);

  priv = HD_HOME_WINDOW_GET_PRIVATE (window);

  if (!priv->osso_context)
    return;

  if (hildon_home_area_get_layout_mode (area))
    res = hildon_help_show (priv->osso_context,
                            HH_HELP_LAYOUT_MODE,
                            HILDON_HELP_SHOW_DIALOG);
  else
    res = hildon_help_show (priv->osso_context,
                            HH_HELP,
                            0);

  switch (res)
  {
    case OSSO_OK:
      break;

    case OSSO_ERROR:
      g_warning ("OSSO_ERROR (No help for such topic ID)\n");
      break;

    case OSSO_RPC_ERROR:
      g_warning ("OSSO_RPC_ERROR (Unable to contact HelpApp or Browser)\n");
      break;

    case OSSO_INVALID:
      g_warning ("OSSO_INVALID (Parameter not formally correct)\n");
      break;

    default:
      g_warning ("Unknown error!\n");
      break;
  }
}
#endif

#ifdef HAVE_LIBOSSO
static void
hd_home_window_calibration_activate (HDHomeWindow *window)
{
  hd_home_window_applet_activate (window,
                                  HCP_PLUGIN_PATH_CALIBRATION);

}
#endif

#ifdef HAVE_LIBOSSO
static void
hd_home_window_personalisation_activate (HDHomeWindow *window)
{
  hd_home_window_applet_activate (window,
                                  HCP_PLUGIN_PATH_PERSONALISATION);
}
#endif

#ifndef HAVE_LIBHILDON
static void
hd_home_window_settings_insensitive_press_cb (HDHomeWindow *window)
{
  hd_home_window_show_information_banner (window,
                                          HH_APPLET_SETTINGS_BANNER);

}

static void
hd_home_window_layout_insensitive_press_cb (HDHomeWindow *window)
{
  hd_home_window_show_information_banner (window,
                                          HH_LAYOUT_UNAVAIL_BANNER);

}
#endif

static void
hd_home_window_save_area_layout (HDHomeWindow *window)
{
  GtkWidget    *area = GTK_BIN (window)->child;

  if (HILDON_IS_HOME_AREA (area))
    {
      GError *error = NULL;
      gchar *filename = g_build_filename (g_get_home_dir (),
                                          HH_AREA_CONFIGURATION_FILE,
                                          NULL);

      hildon_home_area_save_configuration (HILDON_HOME_AREA (area),
                                           filename,
                                           &error);

      if (error)
        {
          g_signal_emit_by_name (window, "io-error", error);
          g_error_free (error);
        }

      g_free (filename);
    }
}

static void
hd_home_window_load_area_layout (HDHomeWindow *window)
{
  GtkWidget    *area = GTK_BIN (window)->child;

  if (HILDON_IS_HOME_AREA (area))
    {
      GError   *error = NULL;
      gchar    *user_filename = NULL;
      gchar    *filename = NULL;

      user_filename = g_build_filename (g_get_home_dir (),
                                        HH_AREA_CONFIGURATION_FILE,
                                        NULL);

      if (g_file_test (user_filename, G_FILE_TEST_EXISTS))
        filename = user_filename;
      else
        filename = HH_AREA_GLOBAL_CONFIGURATION_FILE;

      hildon_home_area_load_configuration (HILDON_HOME_AREA (area),
                                           filename,
                                           &error);

      if (error)
        {
          g_signal_emit_by_name (window, "io-error", error);
          g_error_free (error);
        }

      g_free (user_filename);
    }

}

static void
hd_home_window_button1_clicked (HildonHomeWindow *window)
{
  hd_home_window_accept_layout (HD_HOME_WINDOW (window));
}

static void
hd_home_window_button2_clicked (HildonHomeWindow *window)
{
  hd_home_window_cancel_layout (HD_HOME_WINDOW (window));
}


static void
hd_home_window_snap_toggled (HDHomeWindow *window, GtkCheckMenuItem *item)
{
  GtkWidget *area;

  area = GTK_BIN (window)->child;

  if (HILDON_IS_HOME_AREA (area))
    g_object_set (area,
                  "snap-to-grid", gtk_check_menu_item_get_active (item),
                  NULL);

}

static void
hd_home_window_area_add (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv;
  GtkWidget            *area;

  priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  area = GTK_BIN (window)->child;

  if (!priv->layout_mode_sucks)
    {
      if (priv->selecting_applets &&
          !hildon_home_area_get_layout_mode (HILDON_HOME_AREA (area)))
        hildon_home_area_set_layout_mode (HILDON_HOME_AREA (area), TRUE);

    }

  hd_home_window_ensure_menu_status (window);
}

static void
hd_home_window_area_remove (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv;
  GtkWidget            *area;

  priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  area = GTK_BIN (window)->child;

  if (!hildon_home_area_get_layout_mode (HILDON_HOME_AREA (area)))
    hd_home_window_save_area_layout (window);

  hd_home_window_ensure_menu_status (window);

}

static void
hd_home_window_layout_mode_started (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv = HD_HOME_WINDOW_GET_PRIVATE (window);

  g_object_set (window,
                "title", HH_MENU_LAYOUT_TITLE,
                "menu",  priv->layout_menu,
                NULL);
}

static void
hd_home_window_layout_mode_ended (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv = HD_HOME_WINDOW_GET_PRIVATE (window);

  hd_home_window_destroy_banner (window);

  g_object_set (window,
                "title", HH_MENU_TITLE,
                "menu",  priv->normal_menu,
                NULL);

}

static gboolean
hd_home_window_save_area_timeout (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv;

  priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  priv->save_area_timeout = 0;

  hd_home_window_save_area_layout (window);

  return FALSE;
}


#define SAVE_TIMEOUT            1000
static void
hd_home_window_area_changed (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv;

  priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  g_debug ("layout changed, saving");
  if (!priv->save_area_timeout)
    priv->save_area_timeout = g_timeout_add (SAVE_TIMEOUT,
                                             (GSourceFunc)
                                             hd_home_window_save_area_timeout,
                                             window);
}
#undef SAVE_TIMEOUT

static void
hd_home_window_ensure_menu_status (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv;
  GtkWidget            *area;
  gboolean              settings_item_active = FALSE;
  GList                *items, *l;

  priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  area = GTK_BIN (window)->child;

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
      if (!priv->layout_mode_sucks)
        gtk_widget_set_sensitive (priv->layout_mode_item, FALSE);
      gtk_widget_set_sensitive (priv->settings_item, FALSE);
      return;
    }

  for (l = items; l != NULL; l = l->next)
    {
      HildonDesktopHomeItem    *applet;
      GtkWidget                *item;

      if (!HILDON_DESKTOP_IS_HOME_ITEM (l->data))
        continue;
      applet = HILDON_DESKTOP_HOME_ITEM (l->data);

      item = hildon_desktop_home_item_get_settings_menu_item (applet);
      if (item && GTK_IS_MENU_ITEM (item))
        {
          gtk_menu_append (GTK_MENU (priv->settings_menu), item);
          gtk_widget_show (item);

          settings_item_active = TRUE;
        }
    }

  if (!priv->layout_mode_sucks)
    gtk_widget_set_sensitive (priv->layout_mode_item, TRUE);
  gtk_widget_set_sensitive (priv->settings_item, settings_item_active);
  g_list_free (items);
}

#ifdef HAVE_LIBOSSO
static void
hd_home_window_set_osso_context (HDHomeWindow *window,
                                 osso_context_t *osso_context)
{
  HDHomeWindowPrivate *priv;

  g_return_if_fail (HD_IS_HOME_WINDOW (window));

  priv = HD_HOME_WINDOW_GET_PRIVATE (window);

  if (priv->osso_context != osso_context)
  {
    priv->osso_context = osso_context;
    g_object_notify (G_OBJECT (window), "osso-context");

  }
}
#endif

static gboolean
hd_home_window_destroy_banner (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  GtkWidget            *area = GTK_BIN (window)->child;

  if (priv->layout_mode_banner)
    {
      gtk_widget_destroy (priv->layout_mode_banner);
      priv->layout_mode_banner = NULL;
      g_signal_handlers_disconnect_by_func (G_OBJECT (area),
                                            G_CALLBACK (hd_home_window_destroy_banner),
                                            window);
    }
  priv->layout_mode_banner_to = 0;

  return FALSE;
}

static void
hd_home_window_show_layout_mode_banner (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  GtkWidget            *area = GTK_BIN (window)->child;

  if (!priv->layout_mode_banner)
    {
      priv->layout_mode_banner =
          hildon_banner_show_animation (GTK_WIDGET (window),
                                        NULL,
                                        HH_LAYOUT_MODE_BANNER);
      priv->layout_mode_banner_to =
          g_timeout_add (LAYOUT_OPENING_BANNER_TIMEOUT,
                         (GSourceFunc)hd_home_window_destroy_banner,
                         window);

      g_signal_connect_swapped (G_OBJECT (area),
                                "applet-change-start",
                                G_CALLBACK (hd_home_window_destroy_banner),
                                window);


    }

}

static void
hd_home_window_layout_mode_accept (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  GtkWidget            *area;

  area = GTK_BIN (window)->child;
  g_return_if_fail (HILDON_IS_HOME_AREA (area));

  if (hildon_home_area_get_overlaps (HILDON_HOME_AREA (area)))
    {
      hd_home_window_show_information_note (HD_HOME_WINDOW (window),
                                            HH_LAYOUT_OVERLAP_TEXT);
      return;
    }

  if (!priv->layout_mode_sucks)
    {
      hildon_home_area_set_layout_mode (HILDON_HOME_AREA (area),
                                        FALSE);
    }

  hd_home_window_save_area_layout (HD_HOME_WINDOW (window));
  g_signal_emit_by_name (window, "save", NULL);

}

static void
hd_home_window_layout_mode_cancel (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  GtkWidget            *area;

  area = GTK_BIN (window)->child;
  g_return_if_fail (HILDON_IS_HOME_AREA (area));

  if (hildon_home_area_get_layout_changed (HILDON_HOME_AREA (area)))
    {
      GtkWidget *note;
      gint response;

      note = hildon_note_new_confirmation_add_buttons
          (NULL,
           HH_LAYOUT_CANCEL_TEXT,
           HH_LAYOUT_CANCEL_YES,
           GTK_RESPONSE_ACCEPT,
           HH_LAYOUT_CANCEL_NO,
           GTK_RESPONSE_CANCEL,
           NULL);

      response = gtk_dialog_run (GTK_DIALOG (note));
      gtk_widget_destroy (note);

      if (response != GTK_RESPONSE_ACCEPT)
        return;
    }

  hd_home_window_load_area_layout (HD_HOME_WINDOW (window));
  g_signal_emit_by_name (window, "load", NULL);

  if (!priv->layout_mode_sucks)
    hildon_home_area_set_layout_mode (HILDON_HOME_AREA (area),
                                      FALSE);

}

static void
hd_home_window_background (HDHomeWindow    *window,
                           gboolean             is_background)
{
  HDHomeWindowPrivate  *priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  GtkWidget            *area;

  area = GTK_BIN (window)->child;
  g_return_if_fail (HILDON_IS_HOME_AREA (area));

  if (!priv->is_inactive)
    {

      /* If we were in layout mode and went to background, we need
       * to cancel it */
      if (is_background &&
          hildon_home_area_get_layout_mode (HILDON_HOME_AREA (area)))
        {
          hildon_home_area_set_layout_mode (HILDON_HOME_AREA (area),
                                            FALSE);

          hd_home_window_save_area_layout (window);
        }

      gtk_container_foreach (GTK_CONTAINER (area),
                             (GtkCallback)hildon_desktop_home_item_set_is_background,
                             (gpointer)is_background);
    }

}

static void
hd_home_window_system_inactivity (HDHomeWindow         *window,
                                  gboolean              is_inactive)
{
  HDHomeWindowPrivate  *priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  GtkWidget            *area;

  area = GTK_BIN (window)->child;
  g_return_if_fail (HILDON_IS_HOME_AREA (area));

  priv->is_inactive = is_inactive;

  gtk_container_foreach (GTK_CONTAINER (area),
                         (GtkCallback)hildon_desktop_home_item_set_is_background,
                         (gpointer)is_inactive);

}

static void
hd_home_window_set_background (HDHomeWindow *window,
                               HDHomeBackground *background)
{
  HDHomeWindowPrivate  *priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  GdkRectangle *workarea;

  g_object_get (window, "work-area", &workarea, NULL);

  if (priv->background)
    g_object_unref (priv->background);

  priv->background = background;
  if (background)
    g_object_ref (background);
  g_object_notify (G_OBJECT (window), "background");

  if (background && GTK_WIDGET_MAPPED (window))
    {
      hd_home_background_apply_async (background,
                                      GTK_WIDGET (window)->window,
                                      workarea,
                                      (HDHomeBackgroundApplyCallback)
                                      background_apply_callback,
                                      window);
    }

}

static void
hd_home_window_select_applets (HDHomeWindow *window)
{
  HDHomeWindowPrivate  *priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  GtkWidget            *area = GTK_BIN (window)->child;

  g_return_if_fail (HILDON_IS_HOME_AREA (area));

  priv->selecting_applets = TRUE;

  hildon_home_area_set_batch_add (HILDON_HOME_AREA (area), TRUE);
  g_signal_emit_by_name (window, "select-plugins", NULL);
  hildon_home_area_set_batch_add (HILDON_HOME_AREA (area), FALSE);
  priv->selecting_applets = FALSE;

  if (HILDON_IS_HOME_AREA (area))
    {
      /* If we are in layout mode, we only save when layout mode is
       * accepted */
      if (!hildon_home_area_get_layout_mode (HILDON_HOME_AREA (area)))
        g_signal_emit_by_name (window, "save", NULL);
    }

}

static void
hd_home_window_cancel_layout (HDHomeWindow *window)
{
  g_signal_emit_by_name (window, "layout-mode-cancel");
}

static void
hd_home_window_accept_layout (HDHomeWindow *window)
{
  g_signal_emit_by_name (window, "layout-mode-accept");
}

static void
hd_home_window_set_background_reponse (HDHomeWindow *window,
                                       gint response,
                                       GtkDialog *dialog)
{
  HDHomeWindowPrivate  *priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  HDHomeBackground     *background;
  GdkRectangle         *workarea;

  g_object_get (window, "work-area", &workarea, NULL);

  background =
      hd_home_background_dialog_get_background (HD_HOME_BACKGROUND_DIALOG (dialog));

  switch (response)
    {
      case GTK_RESPONSE_OK:
          gtk_widget_hide (GTK_WIDGET(dialog));
          g_debug ("BACKGROUND_OK");
          if (!hd_home_background_equal (priv->background,
                                         background))
            hd_home_background_apply_async 
                (background,
                 GTK_WIDGET (window)->window,
                 workarea,
                 (HDHomeBackgroundApplyCallback)background_apply_and_save_callback,
               window);
          g_object_unref (priv->previous_background);
          break;
      case HILDON_HOME_SET_BG_RESPONSE_PREVIEW:
          hd_home_background_apply_async 
              (background,
               GTK_WIDGET (window)->window,
               workarea,
               (HDHomeBackgroundApplyCallback)background_apply_callback,
               window);

          break;
      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_DELETE_EVENT:
          if (!hd_home_background_equal (priv->background,
                                         priv->previous_background))
            hd_home_background_apply_async 
                (priv->previous_background,
                 GTK_WIDGET (window)->window,
                 workarea,
                 (HDHomeBackgroundApplyCallback)background_apply_callback,
                 window);

          gtk_widget_hide (GTK_WIDGET (dialog));
          g_object_unref (priv->previous_background);
          break;
      default:
          break;
    }
}

static void
hd_home_window_layout_mode_activate (HDHomeWindow *window)
{
  GtkWidget    *area = GTK_BIN (window)->child; 

  g_return_if_fail (HILDON_IS_HOME_AREA (area));

  hildon_home_area_set_layout_mode (HILDON_HOME_AREA (area),
                                    TRUE);

}

static void
hd_home_window_set_background_activate (HDHomeWindow *window)
{
  HDHomeWindowPrivate      *priv = HD_HOME_WINDOW_GET_PRIVATE (window);
  GtkWidget                *dialog;

  priv->previous_background = hd_home_background_copy (priv->background);

  dialog = g_object_new (HD_TYPE_HOME_BACKGROUND_DIALOG,
#ifdef HAVE_LIBOSSO
                         "osso-context", priv->osso_context,
#endif
                         "background-dir", HD_DESKTOP_BACKGROUNDS_PATH,
                         "background", priv->background,
                         NULL);

  g_signal_connect_swapped (dialog, "response",
                            G_CALLBACK (hd_home_window_set_background_reponse),
                            window);

  gtk_widget_show (dialog);

}

static void
hd_home_window_show_information_note (HDHomeWindow *window,
                                      const gchar *text)
{
  GtkWidget *note = NULL;

  note = hildon_note_new_information (NULL, text);

  gtk_dialog_run (GTK_DIALOG (note));
  if (note)
    gtk_widget_destroy (GTK_WIDGET (note));
}

#ifndef HAVE_LIBHILDON
static void
hd_home_window_show_information_banner (HDHomeWindow *window,
                                        const gchar *text)
{
  g_return_if_fail (GTK_IS_WIDGET (window) && text);

  hildon_banner_show_information (GTK_WIDGET (window),
                                  NULL,
                                  text);
}
#endif


#if 0
static void
scale_changed (GtkRange *range, GtkWidget *area)
{
  gint  alpha;
  
  alpha = (gint) gtk_range_get_value (range);

  g_debug ("value_changed");

  g_object_set (area,
                "applet-default-alpha", alpha,
                NULL);
}

static void
hd_home_window_adjust_alpha_activate (HDHomeWindow *window)
{
  GtkWidget    *area, *dialog, *scale;
  gint          alpha;
  gint          response;

  area = hildon_home_window_get_area (HILDON_HOME_WINDOW (window));

  g_object_get (area,
                "applet-default-alpha", &alpha,
                NULL);

  dialog = gtk_dialog_new_with_buttons ("Adjust applets' transparency",
                                        GTK_WINDOW (window),
                                        GTK_DIALOG_MODAL | 
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                        NULL);
  gtk_widget_set_size_request (dialog, 300, -1);

  scale = gtk_hscale_new_with_range (0.0,
                                     100.0,
                                     1.0);
  gtk_widget_show (scale);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_changed),
                    area);
  gtk_range_set_value (GTK_RANGE (scale), (gdouble)alpha);
  
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), scale);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  
  gtk_widget_destroy (dialog);
}

#endif
