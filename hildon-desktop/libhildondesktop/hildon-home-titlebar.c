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

#include <gdk/gdkx.h>

#ifdef HAVE_LIBHILDON
#define ENABLE_UNSTABLE_API
#include <hildon/hildon-helper.h>
#undef ENABLE_UNSTABLE_API
#else
#include <hildon-widgets/hildon-defines.h>
#endif

#include "hildon-home-titlebar.h"
#include "hildon-home-window.h"

#include <libhildondesktop/hildon-desktop-picture.h>

#include <X11/extensions/Xfixes.h>

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

#define HILDON_HOME_TITLEBAR_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_TYPE_HOME_TITLEBAR, HildonHomeTitlebarPrivate))

struct _HildonHomeTitlebarPrivate
{
  GtkWidget    *label;

  gchar        *title;

  GtkWidget    *menu;

  GtkWidget    *button1;
  GtkWidget    *button2;

  guint menu_key_pressed : 1;
  guint menu_popup_status : 1;

  Picture       background_picture;
  Picture       background_mask;
  gint          background_width, background_height;
};

enum
{
  PROP_0,

  PROP_MENU,
  PROP_TITLE,
};

enum
{
  BUTTON1_CLICKED,
  BUTTON2_CLICKED,

  LAST_SIGNAL
};

G_DEFINE_TYPE (HildonHomeTitlebar, hildon_home_titlebar, GTK_TYPE_EVENT_BOX);

static guint titlebar_signals[LAST_SIGNAL] = { 0 };

static void
hildon_home_titlebar_finalize (GObject *object)
{
  HildonHomeTitlebarPrivate *priv = HILDON_HOME_TITLEBAR (object)->priv;

  g_free (priv->title);

  G_OBJECT_CLASS (hildon_home_titlebar_parent_class)->finalize (object);
}

static void
hildon_home_titlebar_destroy (GtkObject *object)
{
  HildonHomeTitlebar *titlebar = HILDON_HOME_TITLEBAR (object);

  if (titlebar->priv->menu)
    gtk_menu_detach (GTK_MENU (titlebar->priv->menu));

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

#if 0
static void
titlebar_menu_detach (GtkWidget *widget,
                      GtkMenu   *menu)
{
  HildonHomeTitlebar *titlebar = HILDON_HOME_TITLEBAR (widget);

  titlebar->priv->menu = NULL;
}
#endif

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
button1_clicked_cb (HildonHomeTitlebar *titlebar)
{
  g_signal_emit (titlebar, titlebar_signals[BUTTON1_CLICKED], 0);
}

static void
button2_clicked_cb (HildonHomeTitlebar *titlebar)
{
  g_signal_emit (titlebar, titlebar_signals[BUTTON2_CLICKED], 0);
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
hildon_home_titlebar_style_set (GtkWidget      *widget,
                                GtkStyle       *old_style)
{
  HildonHomeTitlebarPrivate    *priv =
      HILDON_HOME_TITLEBAR_GET_PRIVATE (widget);
  const gchar  *filename;

  GTK_WIDGET_CLASS (hildon_home_titlebar_parent_class)->style_set (widget,
                                                                   old_style);

  if (!GTK_IS_STYLE (widget->style) ||
      !GTK_IS_RC_STYLE (widget->style->rc_style) ||
      !widget->style->rc_style->bg_pixmap_name[GTK_STATE_NORMAL])
    return;

  if (priv->background_picture != None)
    {
      XRenderFreePicture (GDK_DISPLAY (), priv->background_picture);
      priv->background_picture = None;
    }

  if (priv->background_mask != None)
    {
      XRenderFreePicture (GDK_DISPLAY (), priv->background_mask);
      priv->background_mask = None;
    }

  filename = widget->style->rc_style->bg_pixmap_name[GTK_STATE_NORMAL];

  if (!filename)
    return;

  if (!GTK_IS_STYLE (old_style) ||
      !GTK_IS_RC_STYLE (old_style) ||
      !old_style->rc_style->bg_pixmap_name ||
      !g_str_equal (filename,
                    old_style->rc_style->bg_pixmap_name[GTK_STATE_NORMAL]))
    {
      hildon_desktop_picture_and_mask_from_file (filename,
                                                 &priv->background_picture,
                                                 &priv->background_mask,
                                                 &priv->background_width,
                                                 &priv->background_height);
    }

}

static void
hildon_home_titlebar_realize (GtkWidget        *widget)
{
  GTK_WIDGET_CLASS (hildon_home_titlebar_parent_class)->realize (widget);
  hildon_home_titlebar_style_set (widget, widget->style);
}

static gboolean
hildon_home_titlebar_expose (GtkWidget         *widget,
                             GdkEventExpose    *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      HildonHomeTitlebarPrivate    *priv =
          HILDON_HOME_TITLEBAR_GET_PRIVATE (widget);
      GdkDrawable      *drawable;
      gint              x_offset, y_offset;
      Picture           picture;
      XRectangle        clip_rect;
      XserverRegion     clip_region;

      gdk_window_get_internal_paint_info (widget->window,
                                          &drawable,
                                          &x_offset, &y_offset);

      picture = hildon_desktop_picture_from_drawable (drawable);

      if (picture == None)
        return FALSE;

      clip_rect.x = event->area.x - x_offset;
      clip_rect.y = event->area.y - y_offset;
      clip_rect.width = event->area.width;
      clip_rect.height = event->area.height;

      clip_region = XFixesCreateRegion (GDK_DISPLAY (),
                                        &clip_rect,
                                        1);

      XFixesSetPictureClipRegion (GDK_DISPLAY (),
                                  picture,
                                  0, 0,
                                  clip_region);

      if (priv->background_picture)
        XRenderComposite (GDK_DISPLAY (),
                          PictOpOver,
                          priv->background_picture,
                          priv->background_mask,
                          picture,
                          0, 0,
                          0, 0,
                          widget->allocation.x - x_offset,
                          widget->allocation.y - y_offset,
                          priv->background_width,
                          priv->background_height);

      XFixesDestroyRegion (GDK_DISPLAY (), clip_region);
      XRenderFreePicture (GDK_DISPLAY (), picture);

      return GTK_WIDGET_CLASS (hildon_home_titlebar_parent_class)->
                               expose_event (widget, event);

    }

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
    case PROP_MENU:
      hildon_home_titlebar_set_menu (titlebar,
                                     GTK_WIDGET (g_value_get_object (value)));
      break;
    case PROP_TITLE:
      hildon_home_titlebar_set_title (titlebar,
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
    case PROP_MENU:
      g_value_set_object (value, titlebar->priv->menu);
      break;
    case PROP_TITLE:
      g_value_set_string (value, titlebar->priv->title);
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
  widget_class->style_set = hildon_home_titlebar_style_set;
  widget_class->expose_event = hildon_home_titlebar_expose;
  widget_class->realize = hildon_home_titlebar_realize;

  pspec = g_param_spec_object ("menu",
                               "Menu",
                               "Menu",
                               GTK_TYPE_MENU,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (gobject_class,
                                   PROP_MENU,
                                   pspec);

  pspec = g_param_spec_string ("title",
                               "Title",
                               "Title",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (gobject_class,
                                   PROP_TITLE,
                                   pspec);

  titlebar_signals[BUTTON1_CLICKED] =
      g_signal_new ("button1-clicked",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (HildonHomeTitlebarClass, button1_clicked),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);
  titlebar_signals[BUTTON2_CLICKED] =
      g_signal_new ("button2-clicked",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (HildonHomeTitlebarClass, button2_clicked),
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

  priv->label = gtk_label_new (priv->title);
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

  priv->button1 = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (priv->button1),
                     gtk_image_new_from_icon_name (LAYOUT_MODE_ACCEPT_BUTTON,
                                                   GTK_ICON_SIZE_BUTTON));
  gtk_widget_set_size_request (priv->button1,
                               LAYOUT_MODE_BUTTON_SIZE,
                               LAYOUT_MODE_BUTTON_SIZE);
  g_signal_connect_swapped (priv->button1, "clicked",
                            G_CALLBACK (button1_clicked_cb),
                            titlebar);


  gtk_box_pack_start (GTK_BOX (hbox), priv->button1, FALSE, FALSE, 0);


  priv->button2 = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (priv->button2),
                     gtk_image_new_from_icon_name (LAYOUT_MODE_CANCEL_BUTTON,
                                                   GTK_ICON_SIZE_BUTTON));
  gtk_widget_set_size_request (priv->button2,
                               LAYOUT_MODE_BUTTON_SIZE,
                               LAYOUT_MODE_BUTTON_SIZE);

  gtk_box_pack_start (GTK_BOX (hbox), priv->button2, FALSE, FALSE, 0);
  g_signal_connect_swapped (priv->button2, "clicked",
                            G_CALLBACK (button2_clicked_cb),
                            titlebar);

  gtk_widget_pop_composite_child ();
}

GtkWidget *
hildon_home_titlebar_new ()
{
  return g_object_new (HILDON_TYPE_HOME_TITLEBAR,
                       NULL);
}

void
hildon_home_titlebar_toggle_menu (HildonHomeTitlebar *titlebar)
{
  HildonHomeTitlebarPrivate    *priv = titlebar->priv;
  GtkMenu                      *menu = GTK_MENU (priv->menu);

  if (!menu)
    {
      g_warning ("Titlebar menu toggled but no menu defined");
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

#if 0
  gtk_menu_attach_to_widget (GTK_MENU (priv->menu),
                             GTK_WIDGET (titlebar),
                             titlebar_menu_detach);
#endif
  gtk_widget_set_name (menu, HH_TITLEBAR_MENU_WIDGET_NAME);
  g_signal_connect (menu, "deactivate",
                    G_CALLBACK (titlebar_menu_deactivate_cb),
                    titlebar);

  g_object_notify (G_OBJECT (titlebar), "menu");
}

void
hildon_home_titlebar_set_title (HildonHomeTitlebar *titlebar,
                                const gchar *label)
{
  HildonHomeTitlebarPrivate *priv;

  g_return_if_fail (HILDON_IS_HOME_TITLEBAR (titlebar));

  if (!label)
    return;

  priv = HILDON_HOME_TITLEBAR_GET_PRIVATE (titlebar);

  if (!priv->title || !g_str_equal (priv->title, label))
    {
      g_free (priv->title);

      priv->title = g_strdup (label);

      gtk_label_set_text (GTK_LABEL (priv->label), label);

      g_object_notify (G_OBJECT (titlebar), "title");
    }

}
