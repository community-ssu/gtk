/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
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


#include <glib-object.h>

#include <gtk/gtkalignment.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenushell.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkmain.h>

#ifdef HAVE_LIBHILDON
#include <hildon/hildon-helper.h>
#else
#include <hildon-widgets/hildon-defines.h>
#endif

#include "hildon-home-titlebar.h"
#include "hildon-home-area.h"
#include "hildon-home-window.h"

#define HILDON_HOME_TITLEBAR_HEIGHT	60
#define HILDON_HOME_TITLEBAR_MENU_LABEL_FONT  "osso-TitleFont"
#define HILDON_HOME_TITLEBAR_MENU_LABEL_COLOR "TitleTextColor"

#define HILDON_HOME_MENUAREA_LMARGIN    0
#define HILDON_HOME_MENUAREA_WIDTH      348

#define HILDON_HOME_MENU_WIDTH          348

#define PADDING_TOP  12
#define PADDING_LEFT 35

#define LAYOUT_MODE_CANCEL_BUTTON	 "qgn_indi_home_layout_reject"
#define LAYOUT_MODE_ACCEPT_BUTTON	 "qgn_indi_home_layout_accept"
#define LAYOUT_MODE_BUTTON_SIZE		 32
#define LAYOUT_MODE_BUTTON_SPACING       8
#define LAYOUT_MODE_BUTTON_PADDING_TOP	 15
#define LAYOUT_MODE_BUTTON_PADDING_RIGHT 3


#define HH_TITLEBAR_MENU_WIDGET_NAME    "menu_force_with_corners"


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

  PROP_MODE,
  PROP_MENU,
  PROP_MENU_TITLE,
  PROP_LAYOUT_MENU,
  PROP_LAYOUT_MENU_TITLE
};

enum
{
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
layout_accept_clicked_cb (GtkWidget *widget,
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
    case PROP_MODE:
      hildon_home_titlebar_set_mode (titlebar,
		                     g_value_get_enum (value));
      break;
    case PROP_MENU:
      hildon_home_titlebar_set_menu (titlebar,
                                     GTK_WIDGET (g_value_get_object (value)));
      break;
    case PROP_MENU_TITLE:
      hildon_home_titlebar_set_menu_title (titlebar,
                                           g_value_get_string (value));
      break;
    case PROP_LAYOUT_MENU:
      hildon_home_titlebar_set_layout_menu (titlebar,
                                            GTK_WIDGET (g_value_get_object (value)));
      break;

    case PROP_LAYOUT_MENU_TITLE:
      hildon_home_titlebar_set_layout_menu_title (titlebar,
                                                  g_value_get_string (value));
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
    case PROP_MODE:
      g_value_set_enum (value, titlebar->priv->mode);
      break;
    case PROP_MENU:
      g_value_set_object (value, titlebar->priv->menu);
      break;
    case PROP_MENU_TITLE:
      g_value_set_string (value, titlebar->priv->normal_text);
      break;
    case PROP_LAYOUT_MENU:
      g_value_set_object (value, titlebar->priv->layout_menu);
      break;
    case PROP_LAYOUT_MENU_TITLE:
      g_value_set_string (value, titlebar->priv->layout_text);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
hildon_home_titlebar_class_init (HildonHomeTitlebarClass *klass)
{
  GObjectClass     *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass   *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class = GTK_WIDGET_CLASS (klass);
  GParamSpec       *pspec;

  gobject_class->finalize = hildon_home_titlebar_finalize;
  gobject_class->set_property = hildon_home_titlebar_set_property;
  gobject_class->get_property = hildon_home_titlebar_get_property;

  object_class->destroy = hildon_home_titlebar_destroy;

  widget_class->button_press_event = hildon_home_titlebar_button_press_event;
  widget_class->button_release_event = hildon_home_titlebar_button_release_event;

  pspec = g_param_spec_enum ("mode",
                             "Mode",
                             "Titlebar mode",
                             HILDON_TYPE_HOME_TITLEBAR_MODE,
                             HILDON_HOME_TITLEBAR_NORMAL,
                             G_PARAM_READWRITE);

  g_object_class_install_property (gobject_class,
                                   PROP_MODE,
                                   pspec);

  pspec = g_param_spec_object ("menu",
                               "Menu",
                               "Menu used in normal mode",
                               GTK_TYPE_MENU,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (gobject_class,
                                   PROP_MENU,
                                   pspec);
  
  pspec = g_param_spec_object ("layout-menu",
                               "Layout mode menu",
                               "Menu used in layout mode",
                               GTK_TYPE_MENU,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (gobject_class,
                                   PROP_LAYOUT_MENU,
                                   pspec);
  
  pspec = g_param_spec_string ("menu-title",
                               "Menu title",
                               "Menu title used in normal mode",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (gobject_class,
                                   PROP_MENU_TITLE,
                                   pspec);
  
  pspec = g_param_spec_string ("layout-menu-title",
                               "Layout mode menu title",
                               "Menu title used in layout mode",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (gobject_class,
                                   PROP_LAYOUT_MENU_TITLE,
                                   pspec);

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

  gtk_widget_set_size_request (GTK_WIDGET (titlebar),
                               -1,
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

  priv->label = gtk_label_new (priv->normal_text);
  gtk_widget_set_composite_name (priv->label, "hildon-home-titlebar-label");
  gtk_misc_set_alignment (GTK_MISC (priv->label), 0.0, 0.5);

#ifdef HAVE_LIBHILDON
  hildon_helper_set_logical_font (priv->label,
                                  HILDON_HOME_TITLEBAR_MENU_LABEL_FONT);
  hildon_helper_set_logical_color (priv->label,
                                   GTK_RC_FG,
                                   GTK_STATE_NORMAL,
                                   HILDON_HOME_TITLEBAR_MENU_LABEL_COLOR);
#else
  hildon_gtk_widget_set_logical_font (priv->label,
                                      HILDON_HOME_TITLEBAR_MENU_LABEL_FONT);
  hildon_gtk_widget_set_logical_color (priv->label,
                                       GTK_RC_FG,
                                       GTK_STATE_NORMAL,
                                       HILDON_HOME_TITLEBAR_MENU_LABEL_COLOR);
#endif

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
                    G_CALLBACK (layout_accept_clicked_cb),
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

  gtk_widget_pop_composite_child ();
}

GtkWidget *
hildon_home_titlebar_new ()
{
  return g_object_new (HILDON_TYPE_HOME_TITLEBAR,
                       "mode", HILDON_HOME_TITLEBAR_NORMAL,
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
          menu = GTK_MENU (priv->menu);
          break;
      case HILDON_HOME_TITLEBAR_LAYOUT:
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

void
hildon_home_titlebar_set_menu (HildonHomeTitlebar *titlebar,
                               GtkWidget *menu)
{
  HildonHomeTitlebarPrivate *priv;

  g_return_if_fail (HILDON_IS_HOME_TITLEBAR (titlebar));

  if (!GTK_IS_MENU (menu))
    return;

  priv = HILDON_HOME_TITLEBAR_GET_PRIVATE (titlebar);

  if (priv->menu)
    gtk_menu_detach (GTK_MENU (priv->menu));

  priv->menu = menu;

  gtk_widget_set_name (menu, HH_TITLEBAR_MENU_WIDGET_NAME);
  g_signal_connect (menu, "deactivate",
                    G_CALLBACK (titlebar_menu_deactivate_cb),
                    titlebar);

  g_object_notify (G_OBJECT (titlebar), "menu");
}

void
hildon_home_titlebar_set_layout_menu (HildonHomeTitlebar *titlebar,
                                      GtkWidget *menu)
{
  HildonHomeTitlebarPrivate *priv;

  g_return_if_fail (HILDON_IS_HOME_TITLEBAR (titlebar));
  
  if (!GTK_IS_MENU (menu))
    return;

  priv = HILDON_HOME_TITLEBAR_GET_PRIVATE (titlebar);

  if (priv->layout_menu)
    gtk_menu_detach (GTK_MENU (priv->layout_menu));

  priv->layout_menu = menu;

  gtk_widget_set_name (menu, HH_TITLEBAR_MENU_WIDGET_NAME);
  g_signal_connect (menu, "deactivate",
                    G_CALLBACK (titlebar_menu_deactivate_cb),
                    titlebar);

  g_object_notify (G_OBJECT (titlebar), "layout-menu");
}

void
hildon_home_titlebar_set_menu_title (HildonHomeTitlebar *titlebar,
                                     const gchar *label)
{
  HildonHomeTitlebarPrivate *priv;

  g_return_if_fail (HILDON_IS_HOME_TITLEBAR (titlebar));

  if (!label)
    return;

  priv = HILDON_HOME_TITLEBAR_GET_PRIVATE (titlebar);

  if (!priv->normal_text || !g_str_equal (priv->normal_text, label))
    {
      g_free (priv->normal_text);

      priv->normal_text = g_strdup (label);
      
      if (priv->mode == HILDON_HOME_TITLEBAR_NORMAL)
        gtk_label_set_text (GTK_LABEL (priv->label), priv->normal_text);
      
      g_object_notify (G_OBJECT (titlebar), "menu-title");
    }

}

void
hildon_home_titlebar_set_layout_menu_title (HildonHomeTitlebar *titlebar,
                                            const gchar *label)
{
  HildonHomeTitlebarPrivate *priv;

  g_return_if_fail (HILDON_IS_HOME_TITLEBAR (titlebar));

  if (!label)
    return;

  priv = HILDON_HOME_TITLEBAR_GET_PRIVATE (titlebar);

  if (!priv->layout_text || !g_str_equal (priv->layout_text, label))
    {
      g_free (priv->layout_text);

      priv->layout_text = g_strdup (label);

      if (priv->mode == HILDON_HOME_TITLEBAR_LAYOUT)
        gtk_label_set_text (GTK_LABEL (priv->label), priv->layout_text);
      
      g_object_notify (G_OBJECT (titlebar), "layout-menu-title");
    }

}
