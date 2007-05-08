/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2005, 2006, 2007 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author: Johan Bilien <johan.bilien@nokia.com>
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

#include "hildon-desktop-home-item.h"
#include "hildon-home-area.h"
#include "hildon-desktop-marshalers.h"

#include <gtk/gtkfixed.h>
#include <gtk/gtkicontheme.h>

#include <X11/extensions/Xrender.h>

#ifdef HAVE_LIBHILDON
#include <hildon/hildon-defines.h>
#else
#include <hildon-widgets/hildon-defines.h>
#endif

#define APPLET_RESIZE_HANDLE_ICON   "qgn_home_layoutmode_resize"
#define APPLET_RESIZE_HANDLE_WIDTH  40
#define APPLET_RESIZE_HANDLE_HEIGHT 40
#define APPLET_CLOSE_BUTTON_ICON    "qgn_home_layoutmode_close"
#define APPLET_CLOSE_BUTTON_WIDTH   26
#define APPLET_CLOSE_BUTTON_HEIGHT  26
#define APPLET_DRAG_HANDLE_ICON     "qgn_list_presence_invisible"
#define APPLET_DRAG_HANDLE_WIDTH    26
#define APPLET_DRAG_HANDLE_HEIGHT   26

#define GRID_SIZE                   10

#define DRAG_UPDATE_TIMEOUT         50
#define LAYOUT_MODE_HIGHLIGHT_WIDTH 4

#define HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_DESKTOP_TYPE_HOME_ITEM, HildonDesktopHomeItemPriv));


typedef enum 
{
  HILDON_DESKTOP_HOME_ITEM_STATE_NORMAL = 0,
  HILDON_DESKTOP_HOME_ITEM_STATE_RESIZING,
  HILDON_DESKTOP_HOME_ITEM_STATE_MOVING
} HildonDesktopHomeItemState;

enum
{
  HILDON_DESKTOP_HOME_ITEM_PROPERTY_RESIZE_TYPE = 1,
  HILDON_DESKTOP_HOME_ITEM_PROPERTY_LAYOUT_MODE,
  HILDON_DESKTOP_HOME_ITEM_PROPERTY_LAYOUT_MODE_SUCKS,
  HILDON_DESKTOP_HOME_ITEM_PROPERTY_DESKTOP_FILE,
  HILDON_DESKTOP_HOME_ITEM_PROPERTY_MINIMUM_WIDTH,
  HILDON_DESKTOP_HOME_ITEM_PROPERTY_MINIMUM_HEIGHT,
  HILDON_DESKTOP_HOME_ITEM_PROPERTY_STATE
};

typedef struct HildonDesktopHomeItemPriv_
{
  gboolean      layout_mode;
  HildonDesktopHomeItemResizeType resize_type;

  GdkPixbuf    *close_button;
  GdkWindow    *close_button_window;
  GdkPixbuf    *resize_handle;
  GdkWindow    *resize_handle_window;
  GdkPixbuf    *drag_handle;
  GdkWindow    *drag_handle_window;

  GdkWindow    *event_window;

  gint          minimum_width;
  gint          minimum_height;

  /* Drag data */
  HildonDesktopHomeItemState state;
  guint         x_offset;
  guint         y_offset;
  gint          delta_x;
  gint          delta_y;
  guint         timeout;
  gboolean      overlaps;
  GtkAllocation old_allocation;

  gboolean      layout_mode_sucks;
} HildonDesktopHomeItemPriv;

static GtkEventBoxClass *parent_class;

static void
hildon_desktop_home_item_init (HildonDesktopHomeItem * self);
static void
hildon_desktop_home_item_class_init (HildonDesktopHomeItemClass * applet_class);

static void
hildon_desktop_home_item_destroy (GtkObject *applet);

static gboolean
hildon_desktop_home_item_expose (GtkWidget * widget,
                                 GdkEventExpose * event);

static void
hildon_desktop_home_item_size_allocate (GtkWidget *widget,
                                        GtkAllocation *allocation);

static void
hildon_desktop_home_item_remove (GtkContainer *applet, GtkWidget *child);

static void
hildon_desktop_home_item_layout_mode_start (HildonDesktopHomeItem *applet);

static void
hildon_desktop_home_item_layout_mode_end (HildonDesktopHomeItem *applet);

static GdkWindow *
hildon_desktop_home_item_create_icon_window (HildonDesktopHomeItem *applet,
                                             GdkPixbuf *icon,
                                             gint x,
                                             gint y);

static gboolean
hildon_desktop_home_item_button_press_event (GtkWidget *applet,
                                             GdkEventButton   *event);

static gboolean
hildon_desktop_home_item_button_release_event (GtkWidget *applet,
                                               GdkEventButton   *event);

static gboolean
hildon_desktop_home_item_key_press_event (GtkWidget *applet,
                                          GdkEventKey *event);

static gboolean
hildon_desktop_home_item_key_release_event (GtkWidget *applet,
                                            GdkEventKey *event);

static void
hildon_desktop_home_item_realize (GtkWidget *widget);

static void
hildon_desktop_home_item_unrealize (GtkWidget *widget);

static void
hildon_desktop_home_item_map (GtkWidget *widget);

static void
hildon_desktop_home_item_unmap (GtkWidget *widget);

static void
hildon_desktop_home_item_tap_and_hold (GtkWidget *widget);

static void
hildon_desktop_home_item_check_overlap (HildonDesktopHomeItem *applet1,
                                        HildonDesktopHomeItem *applet2);

static gboolean
hildon_desktop_home_item_visibility_notify_event (GtkWidget *applet,
                                                  GdkEventVisibility *event);

static void
hildon_desktop_home_item_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);

static void
hildon_desktop_home_item_get_property (GObject      *object,
                                       guint         property_id,
                                       GValue *value,
                                       GParamSpec   *pspec);
static GdkFilterReturn
window_event_filter (GdkXEvent *xevent,
                     GdkEvent *event,
                     HildonDesktopHomeItem *applet);

static void
hildon_desktop_home_item_snap_to_grid (HildonDesktopHomeItem *item);


GType
hildon_desktop_home_item_resize_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0)
    {
    
      static const GEnumValue values[] = {
          { HILDON_DESKTOP_HOME_ITEM_RESIZE_NONE,
            "HILDON_DESKTOP_HOME_ITEM_RESIZE_NONE",
            "none" },
          { HILDON_DESKTOP_HOME_ITEM_RESIZE_VERTICAL,
            "HILDON_DESKTOP_HOME_ITEM_RESIZE_VERTICAL",
            "vertical" },
          { HILDON_DESKTOP_HOME_ITEM_RESIZE_HORIZONTAL,
            "HILDON_DESKTOP_HOME_ITEM_RESIZE_HORIZONTAL",
            "horizontal" },
          { HILDON_DESKTOP_HOME_ITEM_RESIZE_BOTH,
            "HILDON_DESKTOP_HOME_ITEM_RESIZE_BOTH",
            "both" },
          { 0, NULL, NULL }
      };
      etype = g_enum_register_static ("HildonDesktopHomeItemResizeType", values);
  }
  return etype;
}


GType hildon_desktop_home_item_get_type(void)
{
  static GType applet_type = 0;

  if (!applet_type) {
    static const GTypeInfo applet_info = {
      sizeof(HildonDesktopHomeItemClass),
      NULL,       /* base_init */
      NULL,       /* base_finalize */
      (GClassInitFunc) hildon_desktop_home_item_class_init,
      NULL,       /* class_finalize */
      NULL,       /* class_data */
      sizeof(HildonDesktopHomeItem),
      0,  /* n_preallocs */
      (GInstanceInitFunc) hildon_desktop_home_item_init,
    };
    applet_type = g_type_register_static(HILDON_DESKTOP_TYPE_ITEM,
                                         "HildonDesktopHomeItem",
                                         &applet_info, 0);
  }
  return applet_type;
}

static void
hildon_desktop_home_item_class_init (HildonDesktopHomeItemClass * applet_class)
{
  GParamSpec *pspec;
  GtkIconTheme              *icon_theme;
  GError                    *error = NULL;

  /* Get convenience variables */
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (applet_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (applet_class);
  GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS   (applet_class);
  GObjectClass   *object_class = G_OBJECT_CLASS   (applet_class);

  g_type_class_add_private (applet_class, sizeof (HildonDesktopHomeItemPriv));

  object_class->set_property = hildon_desktop_home_item_set_property;
  object_class->get_property = hildon_desktop_home_item_get_property;

  gtkobject_class->destroy = hildon_desktop_home_item_destroy;

  /* Set the widgets virtual functions */
  widget_class->expose_event = hildon_desktop_home_item_expose;
  widget_class->size_allocate = hildon_desktop_home_item_size_allocate;
  widget_class->button_press_event = hildon_desktop_home_item_button_press_event;
  widget_class->button_release_event = hildon_desktop_home_item_button_release_event;
  widget_class->visibility_notify_event =
      hildon_desktop_home_item_visibility_notify_event;
  widget_class->key_press_event   = hildon_desktop_home_item_key_press_event;
  widget_class->key_release_event = hildon_desktop_home_item_key_release_event;
  widget_class->realize = hildon_desktop_home_item_realize;
  widget_class->unrealize = hildon_desktop_home_item_unrealize;
  widget_class->map = hildon_desktop_home_item_map;
  widget_class->unmap = hildon_desktop_home_item_unmap;
  widget_class->tap_and_hold = hildon_desktop_home_item_tap_and_hold;

  /* We override remove to destroy the applet when it's child is destroyed */
  container_class->remove = hildon_desktop_home_item_remove;

  applet_class->layout_mode_start     = hildon_desktop_home_item_layout_mode_start;
  applet_class->layout_mode_end       = hildon_desktop_home_item_layout_mode_end;

  parent_class = g_type_class_peek_parent (applet_class);

  g_signal_new ("layout-mode-start",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonDesktopHomeItemClass, layout_mode_start),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  g_signal_new ("layout-mode-end",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonDesktopHomeItemClass, layout_mode_end),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  g_signal_new ("background",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonDesktopHomeItemClass, background),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  g_signal_new ("foreground",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonDesktopHomeItemClass, foreground),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  g_signal_new ("settings",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (HildonDesktopHomeItemClass, settings),
                NULL,
                NULL,
                g_cclosure_user_marshal_POINTER__POINTER,
                G_TYPE_POINTER,
                1,
                GTK_TYPE_WIDGET);

  pspec =  g_param_spec_enum  ("resize-type",
                               "Type of resizability",
                               "Whether the applet can be resized "
                               "vertically, horizontally, or both",
                               HILDON_DESKTOP_TYPE_HOME_ITEM_RESIZE_TYPE,
                               HILDON_DESKTOP_HOME_ITEM_RESIZE_NONE,
                               G_PARAM_READWRITE);

  g_object_class_install_property (object_class,
                                   HILDON_DESKTOP_HOME_ITEM_PROPERTY_RESIZE_TYPE,
                                   pspec);

  pspec =  g_param_spec_boolean ("layout-mode",
                                 "Layout mode",
                                 "Whether the applet is in layout "
                                 "mode",
                                 FALSE,
                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   HILDON_DESKTOP_HOME_ITEM_PROPERTY_LAYOUT_MODE,
                                   pspec);

  pspec =  g_param_spec_boolean ("layout-mode-sucks",
                                 "Layout mode sucks",
                                 "Whether or not the layout mode "
                                 "is considered to suck",
                                 TRUE,
                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   HILDON_DESKTOP_HOME_ITEM_PROPERTY_LAYOUT_MODE_SUCKS,
                                   pspec);

  pspec =  g_param_spec_int ("minimum-width",
                             "Minimum width",
                             "Minimum width for this applet",
                             -1,
                             G_MAXINT,
                             -1,
                             G_PARAM_READWRITE);

  g_object_class_install_property (object_class,
                                   HILDON_DESKTOP_HOME_ITEM_PROPERTY_MINIMUM_WIDTH,
                                   pspec);

  pspec =  g_param_spec_int ("minimum-height",
                             "Minimum height",
                             "Minimum height for this applet",
                             -1,
                             G_MAXINT,
                             -1,
                             G_PARAM_READWRITE);

  g_object_class_install_property (object_class,
                                   HILDON_DESKTOP_HOME_ITEM_PROPERTY_MINIMUM_HEIGHT,
                                   pspec);

  pspec =  g_param_spec_int ("state",
                             "state",
                             "state of the applet",
                             -1,
                             G_MAXINT,
                             -1,
                             G_PARAM_READABLE);

  g_object_class_install_property (object_class,
                                   HILDON_DESKTOP_HOME_ITEM_PROPERTY_STATE,
                                   pspec);

  pspec = g_param_spec_string ("background",
                               "Background",
                               "Background file, may include alpha channel",
                               "",
                               G_PARAM_READABLE);

  gtk_widget_class_install_style_property (widget_class, pspec);

  pspec = g_param_spec_boxed ("background-borders",
                              "Background borders",
                              "Background border, which shall not be stretched",
                              GTK_TYPE_BORDER,
                              G_PARAM_READABLE);

  gtk_widget_class_install_style_property (widget_class, pspec);

  /* FIXME: Make these configurable, maybe from the theme */
  icon_theme = gtk_icon_theme_get_default();
  applet_class->close_button =
      gtk_icon_theme_load_icon (icon_theme,
                                APPLET_CLOSE_BUTTON_ICON,
                                APPLET_CLOSE_BUTTON_WIDTH,
                                GTK_ICON_LOOKUP_NO_SVG,
                                &error);
  if (error)
    {
      g_warning ("Could not load close button icon: %s", error->message);
      applet_class->close_button = NULL;
      g_error_free (error);
      error = NULL;
    }

  applet_class->resize_handle =
      gtk_icon_theme_load_icon (icon_theme,
                                APPLET_RESIZE_HANDLE_ICON,
                                APPLET_RESIZE_HANDLE_WIDTH,
                                GTK_ICON_LOOKUP_NO_SVG,
                                &error);
  if (error)
    {
      g_warning ("Could not load resize handle icon: %s", error->message);
      applet_class->resize_handle = NULL;
      g_error_free (error);
      error = NULL;
    }

  applet_class->drag_handle =
      gtk_icon_theme_load_icon (icon_theme,
                                APPLET_DRAG_HANDLE_ICON,
                                APPLET_DRAG_HANDLE_WIDTH,
                                GTK_ICON_LOOKUP_NO_SVG,
                                &error);
  if (error)
    {
      g_warning ("Could not load drag handle icon: %s", error->message);
      applet_class->drag_handle = NULL;
      g_error_free (error);
      error = NULL;
    }
}

static void
hildon_desktop_home_item_init (HildonDesktopHomeItem * self)
{
  HildonDesktopHomeItemPriv    *priv;
  HildonDesktopHomeItemClass   *klass;

  klass = G_TYPE_INSTANCE_GET_CLASS ((self),
                                     HILDON_DESKTOP_TYPE_HOME_ITEM,
                                     HildonDesktopHomeItemClass);

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (self);

  priv->layout_mode = FALSE;

  if (klass->close_button)
    {
      g_object_ref (klass->close_button);
      priv->close_button = klass->close_button;
    }

  if (klass->resize_handle)
    {
      g_object_ref (klass->resize_handle);
      priv->resize_handle = klass->resize_handle;
    }

  if (klass->drag_handle)
    {
      g_object_ref (klass->drag_handle);
      priv->drag_handle = klass->drag_handle;
    }

  gtk_widget_add_events (GTK_WIDGET (self), GDK_VISIBILITY_NOTIFY_MASK);

}

static void
hildon_desktop_home_item_destroy (GtkObject *applet)
{
  HildonDesktopHomeItemPriv      *priv;
  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (HILDON_DESKTOP_HOME_ITEM (applet));

  if (priv->timeout)
    g_source_remove (priv->timeout);

  if (GDK_IS_WINDOW (priv->close_button_window))
    {
      gdk_window_destroy (priv->close_button_window);
      priv->close_button_window = NULL;
    }

  if (GDK_IS_WINDOW (priv->resize_handle_window))
    {
      gdk_window_destroy (priv->resize_handle_window);
      priv->resize_handle_window = NULL;
    }

  if (GDK_IS_WINDOW (priv->event_window))
    {
      gdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  if (priv->close_button)
    g_object_unref (priv->close_button);
  priv->close_button = NULL;

  if (priv->resize_handle)
    g_object_unref (priv->resize_handle);
  priv->resize_handle = NULL;

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (applet);

}

static void
hildon_desktop_home_item_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  HildonDesktopHomeItemPriv      *priv;
  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (HILDON_DESKTOP_HOME_ITEM (object));

  switch (property_id)
    {
      case HILDON_DESKTOP_HOME_ITEM_PROPERTY_RESIZE_TYPE:
          hildon_desktop_home_item_set_resize_type (HILDON_DESKTOP_HOME_ITEM (object),
                                              g_value_get_enum (value));
          break;
      case HILDON_DESKTOP_HOME_ITEM_PROPERTY_LAYOUT_MODE:
          hildon_desktop_home_item_set_layout_mode (HILDON_DESKTOP_HOME_ITEM (object),
                                              g_value_get_boolean (value));
          break;
      case HILDON_DESKTOP_HOME_ITEM_PROPERTY_LAYOUT_MODE_SUCKS:
          priv->layout_mode_sucks = g_value_get_boolean (value);
          if (priv->layout_mode_sucks)
            GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET (object), GTK_NO_WINDOW);
          break;
      case HILDON_DESKTOP_HOME_ITEM_PROPERTY_MINIMUM_WIDTH:
          g_object_notify (object, "minimum-width");
          priv->minimum_width = g_value_get_int (value);
          break;
      case HILDON_DESKTOP_HOME_ITEM_PROPERTY_MINIMUM_HEIGHT:
          g_object_notify (object, "minimum-height");
          priv->minimum_height = g_value_get_int (value);
          break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

static void
hildon_desktop_home_item_get_property (GObject      *object,
                                       guint         property_id,
                                       GValue       *value,
                                       GParamSpec   *pspec)
{
  HildonDesktopHomeItemPriv      *priv;
  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (HILDON_DESKTOP_HOME_ITEM (object));

  switch (property_id)
    {
      case HILDON_DESKTOP_HOME_ITEM_PROPERTY_RESIZE_TYPE:
          g_value_set_enum (value, priv->resize_type);
          break;
      case HILDON_DESKTOP_HOME_ITEM_PROPERTY_LAYOUT_MODE:
          g_value_set_boolean (value, priv->layout_mode);
          break;
      case HILDON_DESKTOP_HOME_ITEM_PROPERTY_LAYOUT_MODE_SUCKS:
          g_value_set_boolean (value, priv->layout_mode_sucks);
          break;
      case HILDON_DESKTOP_HOME_ITEM_PROPERTY_MINIMUM_WIDTH:
          g_value_set_int (value, priv->minimum_width);
          break;
      case HILDON_DESKTOP_HOME_ITEM_PROPERTY_MINIMUM_HEIGHT:
          g_value_set_int (value, priv->minimum_height);
          break;
      case HILDON_DESKTOP_HOME_ITEM_PROPERTY_STATE:
          g_value_set_int (value, priv->state);
          break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

static void
hildon_desktop_home_item_realize (GtkWidget *widget)
{
  HildonDesktopHomeItemPriv      *priv;

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (widget);

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  if (!GTK_WIDGET_NO_WINDOW (widget))
    {
      GdkWindowAttr              attributes;
      gint                       attributes_mask;
      GdkWindow                 *parent_window;

      attributes.x = widget->allocation.x;
      attributes.y = widget->allocation.y;
      attributes.width = widget->allocation.width;
      attributes.height = widget->allocation.height;
      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.event_mask = gtk_widget_get_events (widget)
          | GDK_BUTTON_MOTION_MASK
          | GDK_BUTTON_PRESS_MASK
          | GDK_BUTTON_RELEASE_MASK
          | GDK_EXPOSURE_MASK
          | GDK_ENTER_NOTIFY_MASK
          | GDK_LEAVE_NOTIFY_MASK;

      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.colormap = gtk_widget_get_colormap (widget);
      attributes.visual = gdk_colormap_get_visual (attributes.colormap);
      attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

      parent_window = gtk_widget_get_parent_window (widget);
      widget->window = gdk_window_new (parent_window,
                                       &attributes,
                                       attributes_mask);

      gdk_window_set_user_data (widget->window, widget);

#if 0
      gtk_style_set_background (widget->style,
                                widget->window,
                                GTK_STATE_NORMAL);
#endif


      if (0)
/*      if (!priv->layout_mode_sucks)*/
        {
          /* When layout mode is cool, we have an event window
           * on top of everything to catch the clicks */
          attributes.wclass = GDK_INPUT_ONLY;
          attributes_mask = GDK_WA_X | GDK_WA_Y;
          priv->event_window =  gdk_window_new (widget->window,
                                                &attributes,
                                                attributes_mask);
          gdk_window_set_user_data (priv->event_window, widget);
        }

      if (priv->layout_mode_sucks)
        {
#if 0
          priv->drag_handle_window =
              hildon_desktop_home_item_create_icon_window (HILDON_DESKTOP_HOME_ITEM (widget),
                                                     priv->drag_handle,
                                                     0,
                                                     0);
#endif

          if (priv->resize_type != HILDON_DESKTOP_HOME_ITEM_RESIZE_NONE)
            priv->resize_handle_window =
                hildon_desktop_home_item_create_icon_window
                    (HILDON_DESKTOP_HOME_ITEM (widget),
                     priv->resize_handle,
                     widget->allocation.width - APPLET_RESIZE_HANDLE_WIDTH,
                     widget->allocation.height - APPLET_RESIZE_HANDLE_HEIGHT);

          gdk_window_set_events (widget->window,
                                 gdk_window_get_events(widget->window) |
                                 GDK_SUBSTRUCTURE_MASK);
          gdk_window_add_filter (widget->window,
                                 (GdkFilterFunc)window_event_filter,
                                 widget);
        }

    }

  else
    {
      widget->window = gtk_widget_get_parent_window (widget);
      g_object_ref (widget->window);
    }

  widget->style = gtk_style_attach (widget->style, widget->window);

}


static void
hildon_desktop_home_item_unrealize (GtkWidget *widget)
{
  HildonDesktopHomeItemPriv      *priv;

  g_return_if_fail (GTK_IS_WIDGET (widget) && GTK_WIDGET_REALIZED (widget));

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (widget);

  if (GDK_IS_WINDOW (priv->event_window))
    {
      gdk_window_set_user_data (priv->event_window, NULL);
      gdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    GTK_WIDGET_CLASS (parent_class)->unrealize (widget);

}

static void
hildon_desktop_home_item_map (GtkWidget *widget)
{
  HildonDesktopHomeItemPriv      *priv;

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (widget);

  gdk_window_show (widget->window);

  if (GTK_WIDGET_CLASS (parent_class)->map)
    GTK_WIDGET_CLASS (parent_class)->map (widget);

  /* We map the event window after the other ones to be sure it ends up
   * on top */
  if (GDK_IS_WINDOW (priv->event_window))
    gdk_window_show (priv->event_window);

  if (priv->drag_handle_window)
    gdk_window_raise (priv->drag_handle_window);


}

static void
hildon_desktop_home_item_unmap (GtkWidget *widget)
{
  HildonDesktopHomeItemPriv      *priv;

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (widget);

  if (GDK_IS_WINDOW (priv->event_window))
    gdk_window_hide (priv->event_window);

  if (GTK_WIDGET_CLASS (parent_class)->unmap)
    GTK_WIDGET_CLASS (parent_class)->unmap (widget);

}

static void
hildon_desktop_home_item_tap_and_hold (GtkWidget *widget)
{
  HildonDesktopHomeItemPriv    *priv;

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (widget);

  /* If we were in moving or resizing, we should stop */
  if (priv->state != HILDON_DESKTOP_HOME_ITEM_STATE_NORMAL)
    {
      gboolean snap_to_grid = FALSE;
      g_source_remove (priv->timeout);

      if (widget->parent)
        g_object_get (G_OBJECT (widget->parent),
                      "snap-to-grid", &snap_to_grid,
                      NULL);

      if (snap_to_grid)
        hildon_desktop_home_item_snap_to_grid (HILDON_DESKTOP_HOME_ITEM (widget));

      g_source_remove (priv->timeout);
      priv->timeout = 0;
      priv->state = HILDON_DESKTOP_HOME_ITEM_STATE_NORMAL;

      gdk_pointer_ungrab (GDK_CURRENT_TIME);

      /* We need to update the rectangle */
      gtk_widget_queue_draw (widget);
      if (priv->old_allocation.x != widget->allocation.x ||
          priv->old_allocation.y != widget->allocation.y ||
          priv->old_allocation.width != widget->allocation.width ||
          priv->old_allocation.height != widget->allocation.height)

        {
          if (HILDON_IS_HOME_AREA (widget->parent))
            g_signal_emit_by_name (G_OBJECT (widget->parent), "layout-changed");
        }

      if (HILDON_IS_HOME_AREA (widget->parent))
        g_signal_emit_by_name (G_OBJECT (widget->parent),
                               "applet-change-end",
                               widget);
    }

  if (GTK_WIDGET_CLASS (parent_class)->tap_and_hold)
    GTK_WIDGET_CLASS (parent_class)->tap_and_hold(widget);
}



static gboolean
hildon_desktop_home_item_expose (GtkWidget *w, GdkEventExpose *event)
{
  HildonDesktopHomeItemPriv    *priv;

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (w);

  if (GTK_WIDGET_CLASS (parent_class)->expose_event)
    GTK_WIDGET_CLASS (parent_class)->expose_event (w, event);

  if (0)
  {
    GdkDrawable                *drawable;
    gint                        x_offset, y_offset;
    XRenderColor                color = {0};
    XRenderPictFormat          *format;
    XRenderPictureAttributes    pa;
    XRectangle                  rect;
    Picture                     picture;

    gdk_window_get_internal_paint_info (w->window,
                                        &drawable,
                                        &x_offset,
                                        &y_offset);

    format = XRenderFindVisualFormat (GDK_DISPLAY(),
                                        GDK_VISUAL_XVISUAL (gdk_drawable_get_visual (drawable)));

    pa.subwindow_mode = IncludeInferiors;

    picture = XRenderCreatePicture (GDK_DISPLAY (),
                                    GDK_DRAWABLE_XID (drawable),
                                    format,
                                    CPSubwindowMode,
                                    &pa);

    rect.x = rect.y = 0;
    rect.width = w->allocation.width;
    rect.height = w->allocation.height;

    XRenderFillRectangles (GDK_DISPLAY (),
                           PictOpSrc,
                           picture,
                           &color,
                           &rect,
                           1);
  }

  if (w->parent && (priv->layout_mode || priv->layout_mode_sucks))
    {
      /* Draw the rectangle around */

      if (priv->overlaps && !priv->layout_mode_sucks)
        {

          gdk_gc_set_line_attributes (w->style->fg_gc[GTK_STATE_PRELIGHT],
                                      LAYOUT_MODE_HIGHLIGHT_WIDTH,
                                      GDK_LINE_SOLID,
                                      GDK_CAP_BUTT,
                                      GDK_JOIN_MITER);
          gdk_draw_rectangle (w->window,
                              w->style->fg_gc[GTK_STATE_PRELIGHT], FALSE,
                              LAYOUT_MODE_HIGHLIGHT_WIDTH/2,
                              LAYOUT_MODE_HIGHLIGHT_WIDTH/2,
                              w->allocation.width-LAYOUT_MODE_HIGHLIGHT_WIDTH,
                              w->allocation.height-LAYOUT_MODE_HIGHLIGHT_WIDTH);
        }
      else if (priv->state == HILDON_DESKTOP_HOME_ITEM_STATE_RESIZING)
        {

          gdk_gc_set_line_attributes (w->style->fg_gc[GTK_STATE_ACTIVE],
                                      LAYOUT_MODE_HIGHLIGHT_WIDTH,
                                      GDK_LINE_SOLID,
                                      GDK_CAP_BUTT,
                                      GDK_JOIN_MITER);

          gdk_draw_rectangle (w->window,
                              w->style->fg_gc[GTK_STATE_ACTIVE], FALSE,
                              LAYOUT_MODE_HIGHLIGHT_WIDTH/2,
                              LAYOUT_MODE_HIGHLIGHT_WIDTH/2,
                              w->allocation.width-LAYOUT_MODE_HIGHLIGHT_WIDTH,
                              w->allocation.height-LAYOUT_MODE_HIGHLIGHT_WIDTH);
        }
      else if (!priv->layout_mode_sucks)/* Normal */
        {
          gdk_gc_set_line_attributes (w->style->fg_gc[GTK_STATE_NORMAL],
                                      LAYOUT_MODE_HIGHLIGHT_WIDTH,
                                      GDK_LINE_SOLID,
                                      GDK_CAP_BUTT,
                                      GDK_JOIN_MITER);

          gdk_draw_rectangle (w->window,
                              w->style->fg_gc[GTK_STATE_NORMAL], FALSE,
                              LAYOUT_MODE_HIGHLIGHT_WIDTH/2,
                              LAYOUT_MODE_HIGHLIGHT_WIDTH/2,
                              w->allocation.width-LAYOUT_MODE_HIGHLIGHT_WIDTH,
                              w->allocation.height-LAYOUT_MODE_HIGHLIGHT_WIDTH);
        }
    }


  return FALSE;
}

static void
hildon_desktop_home_item_size_allocate (GtkWidget *widget,
                                        GtkAllocation *allocation)
{
  HildonDesktopHomeItemPriv      *priv;
  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (HILDON_DESKTOP_HOME_ITEM(widget));

  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget) && !GTK_WIDGET_NO_WINDOW (widget))
    {

      if (GDK_IS_WINDOW (priv->resize_handle_window))
        gdk_window_move (priv->resize_handle_window,
                         allocation->width
                         - APPLET_RESIZE_HANDLE_WIDTH,
                         allocation->height
                         - APPLET_RESIZE_HANDLE_HEIGHT);

      if (GDK_IS_WINDOW (widget->window))
        gdk_window_move_resize (widget->window,
                                allocation->x,
                                allocation->y,
                                allocation->width,
                                allocation->height);

      if (GDK_IS_WINDOW (priv->event_window))
        gdk_window_move_resize (priv->event_window,
                                0,
                                0,
                                allocation->width,
                                allocation->height);

    }

  if (GTK_BIN (widget)->child)
    {
      GtkAllocation child_allocation = {0};

      child_allocation.width  = allocation->width;
      child_allocation.height = allocation->height;

      if (GTK_WIDGET_NO_WINDOW (widget))
        {
          child_allocation.x = allocation->x;
          child_allocation.y = allocation->y;
        }

      gtk_widget_size_allocate (GTK_BIN (widget)->child, &child_allocation);
    }
}

static void
hildon_desktop_home_item_remove (GtkContainer *applet,
                                 GtkWidget *child)
{
  if (GTK_CONTAINER_CLASS (parent_class)->remove)
    GTK_CONTAINER_CLASS (parent_class)->remove (applet, child);

  gtk_widget_destroy (GTK_WIDGET (applet));
}

static GdkWindow *
hildon_desktop_home_item_create_icon_window (HildonDesktopHomeItem *applet,
                                             GdkPixbuf *icon,
                                             gint x,
                                             gint y)
{
  HildonDesktopHomeItemPriv      *priv;
  GdkWindowAttr              attributes;
  gint                       attributes_mask;
  GdkBitmap                 *bitmask = NULL;
  GdkPixmap                 *pixmap = NULL;
  GdkWindow                 *window;
  GtkWidget                 *w;

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);
  w = GTK_WIDGET (applet);

  attributes.width = gdk_pixbuf_get_width (icon);
  attributes.height = gdk_pixbuf_get_height (icon);
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = 0;
  attributes.visual = gtk_widget_get_visual (w);
  attributes.colormap = gtk_widget_get_colormap (w);
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes_mask = GDK_WA_VISUAL | GDK_WA_COLORMAP;

  gdk_pixbuf_render_pixmap_and_mask_for_colormap (icon,
                                                  attributes.colormap,
                                                  &pixmap,
                                                  &bitmask,
                                                  127);


  window = gdk_window_new (w->window,
                           &attributes,
                           attributes_mask);

  gdk_window_set_events (window,
                         gdk_window_get_events (window)
                         | GDK_BUTTON_PRESS_MASK
                         | GDK_BUTTON_RELEASE_MASK);

  gdk_window_set_user_data (window, w);
  gdk_window_move (window, x, y);

  if (pixmap)
    {
      gdk_window_set_back_pixmap (window, pixmap, FALSE);
      g_object_unref (pixmap);
    }

  if (bitmask)
    {
      gdk_window_shape_combine_mask (window,
                                     bitmask,
                                     0,
                                     0);
      g_object_unref (bitmask);
    }

  gdk_window_show (window);
  gdk_window_raise (window);

  gdk_flush ();

  return window;
}

static GdkFilterReturn
window_event_filter (GdkXEvent *xevent,
                     GdkEvent *event,
                     HildonDesktopHomeItem *applet)
{
  XAnyEvent *aevent = (XAnyEvent *)xevent;

  if (aevent->type == MapNotify)
    {
      HildonDesktopHomeItemPriv *priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);
      /* The applet created a child window, we need to make sure that
       * we remain on top */
      if (GDK_IS_WINDOW (priv->event_window))
        gdk_window_raise (priv->event_window);

      if (priv->close_button_window)
          gdk_window_raise (priv->close_button_window);
      if (priv->resize_handle_window)
        gdk_window_raise (priv->resize_handle_window);
      if (priv->drag_handle_window)
        gdk_window_raise (priv->drag_handle_window);

      if (!priv->layout_mode_sucks && GTK_IS_WIDGET (GTK_BIN (applet)->child))
        {
          gtk_widget_set_sensitive (GTK_BIN (applet)->child, TRUE);
          gtk_widget_set_sensitive (GTK_BIN (applet)->child, FALSE);
        }
    }

  return GDK_FILTER_CONTINUE;

}


static void
hildon_desktop_home_item_layout_mode_start (HildonDesktopHomeItem *applet)
{
  HildonDesktopHomeItemPriv      *priv;
  GtkWidget                 *widget;
  gboolean                   visible;

  widget = GTK_WIDGET (applet);
  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);

  if (priv->layout_mode_sucks)
    return;

  priv->layout_mode = TRUE;

  visible = GTK_WIDGET_VISIBLE (widget);

  if (visible)
    gtk_widget_hide (widget);

  gtk_widget_unrealize (widget);
  GTK_WIDGET_UNSET_FLAGS (widget, GTK_NO_WINDOW);
  gtk_widget_realize (widget);

  if (visible)
    gtk_widget_show (widget);

  if (GTK_WIDGET_VISIBLE (widget))
    gtk_widget_queue_resize (widget);

  if (priv->close_button)
    {
      priv->close_button_window = hildon_desktop_home_item_create_icon_window 
          (applet,
           priv->close_button,
           HILDON_MARGIN_DEFAULT,
           HILDON_MARGIN_DEFAULT);
    }

  if (priv->resize_handle &&
      priv->resize_type != HILDON_DESKTOP_HOME_ITEM_RESIZE_NONE)
    {
      priv->resize_handle_window = hildon_desktop_home_item_create_icon_window 
          (applet,
           priv->resize_handle,
           widget->allocation.width - APPLET_RESIZE_HANDLE_WIDTH,
           widget->allocation.height - APPLET_RESIZE_HANDLE_HEIGHT);
    }

  if (GTK_IS_WIDGET (GTK_BIN (applet)->child))
    gtk_widget_set_sensitive (GTK_BIN (applet)->child, FALSE);

  gdk_window_set_events (widget->window,
                         gdk_window_get_events(widget->window) |
                           GDK_SUBSTRUCTURE_MASK);
  gdk_window_add_filter (widget->window,
                         (GdkFilterFunc)window_event_filter,
                         applet);

}

static void
hildon_desktop_home_item_layout_mode_end (HildonDesktopHomeItem *applet)
{
  HildonDesktopHomeItemPriv      *priv;
  gboolean                   visible;
  GtkWidget                 *widget;

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);
  widget = GTK_WIDGET (applet);
  
  if (priv->layout_mode_sucks)
    return;
  
  priv->layout_mode = FALSE;

  if (priv->close_button_window)
    {
      gdk_window_destroy (priv->close_button_window);
      priv->close_button_window = NULL;
    }

  if (priv->resize_handle_window)
    {
      gdk_window_destroy (priv->resize_handle_window);
      priv->resize_handle_window = NULL;
    }

  if (GTK_IS_WIDGET (GTK_BIN (applet)->child))
    gtk_widget_set_sensitive (GTK_BIN (applet)->child, TRUE);

  gdk_window_remove_filter (GTK_WIDGET (applet)->window,
                            (GdkFilterFunc)window_event_filter,
                            applet);

  visible = GTK_WIDGET_VISIBLE (widget);

  if (visible)
    gtk_widget_hide (widget);

  gtk_widget_unrealize (widget);
  GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW);
  gtk_widget_realize (widget);

  if (visible)
    gtk_widget_show (widget);

}

static gboolean
hildon_desktop_home_item_visibility_notify_event (GtkWidget *applet,
                                                  GdkEventVisibility *event)
{
  HildonDesktopHomeItemPriv *priv;
  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (HILDON_DESKTOP_HOME_ITEM (applet));

  if (priv->layout_mode)
    {
      priv->overlaps = FALSE;
        {
          /* This is necessary, because it may overlapping with an underlying
           * window */
          gtk_container_foreach (GTK_CONTAINER (GTK_WIDGET (applet)->parent),
                                 (GtkCallback)hildon_desktop_home_item_check_overlap,
                                 applet);
        }
      gtk_widget_queue_draw (GTK_WIDGET (applet));
    }

  if (GTK_WIDGET_CLASS (parent_class)->visibility_notify_event)
    return GTK_WIDGET_CLASS (parent_class)->visibility_notify_event (applet,
                                                                     event);
  return FALSE;
}


static gboolean
hildon_desktop_home_item_drag_update (HildonDesktopHomeItem *applet)
{
  HildonDesktopHomeItemPriv *priv;
  gint                  x_applet, y_applet;
  GdkModifierType       mod;
  GtkWidget            *fixed;
  GtkWidget            *widget;
  gboolean              used_to_overlap;

  widget = GTK_WIDGET (applet);

  if (!widget->parent)
    return FALSE;

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);

  fixed = widget->parent;

  gdk_window_get_pointer (fixed->window, &x_applet, &y_applet, &mod);

  if (priv->state == HILDON_DESKTOP_HOME_ITEM_STATE_MOVING)
    {
      /* The Fixed has no window, thus the coordinates are relative to the
       * home's main window */

      x_applet -= fixed->allocation.x;
      y_applet -= fixed->allocation.y;

      x_applet -= priv->x_offset;
      y_applet -= priv->y_offset;

      if (x_applet < 0)
        x_applet = 0;

      if (y_applet < 0)
        y_applet = 0;

      if (x_applet + widget->allocation.width > fixed->allocation.width)
        x_applet =  fixed->allocation.width -
            widget->allocation.width;

      if (y_applet + widget->allocation.height > fixed->allocation.height)
        y_applet =  fixed->allocation.height - widget->allocation.height;

      if (x_applet != widget->allocation.x - fixed->allocation.x)
        priv->delta_x = x_applet - widget->allocation.x + fixed->allocation.x;

      if (y_applet != widget->allocation.y - fixed->allocation.y)
        priv->delta_y = y_applet - widget->allocation.y + fixed->allocation.y;

      gtk_fixed_move (GTK_FIXED (fixed), widget, x_applet, y_applet);
    }
  else /* Resizing */
    {
      gint width, height;

      if (priv->resize_type == HILDON_DESKTOP_HOME_ITEM_RESIZE_HORIZONTAL
          || priv->resize_type == HILDON_DESKTOP_HOME_ITEM_RESIZE_BOTH)
        width  = x_applet + priv->x_offset - widget->allocation.x;
      else
        width = widget->requisition.width;

      if (priv->resize_type == HILDON_DESKTOP_HOME_ITEM_RESIZE_VERTICAL
          || priv->resize_type == HILDON_DESKTOP_HOME_ITEM_RESIZE_BOTH)
        height = y_applet + priv->y_offset - widget->allocation.y;
      else
        height = widget->requisition.height;

      if (priv->minimum_width > 0 && width < priv->minimum_width)
        width = priv->minimum_width;

      if (priv->minimum_height > 0 && height < priv->minimum_height)
        height = priv->minimum_height;

      if (widget->allocation.x + width >
          fixed->allocation.x + fixed->allocation.width)
        width = fixed->allocation.x + fixed->allocation.width
                - widget->allocation.x;

      if (widget->allocation.y + height >
          fixed->allocation.y + fixed->allocation.height)
        height = fixed->allocation.y + fixed->allocation.height
                 - widget->allocation.y;

      if (width != widget->allocation.width)
        priv->delta_x = width  - widget->allocation.width;

      if (height != widget->allocation.height)
        priv->delta_y = height - widget->allocation.height;

      gtk_widget_set_size_request (widget, width, height);
    }

  used_to_overlap = priv->overlaps;
  priv->overlaps = FALSE;
  gtk_container_foreach (GTK_CONTAINER (GTK_WIDGET (applet)->parent),
                         (GtkCallback)hildon_desktop_home_item_check_overlap,
                         applet);
  if (used_to_overlap != priv->overlaps)
    gtk_widget_queue_draw (widget);


  return TRUE;
}

static void
hildon_desktop_home_item_check_overlap (HildonDesktopHomeItem *applet1,
                                        HildonDesktopHomeItem *applet2)
{
  HildonDesktopHomeItemPriv      *priv2;
  GdkRectangle               r;

  if (applet1 == applet2)
    return;

  priv2 = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet2);
  
  if (gdk_rectangle_intersect (&GTK_WIDGET (applet1)->allocation,
                               &(GTK_WIDGET (applet2)->allocation),
                               &r))
    {
      priv2->overlaps = TRUE;
    }


}

static void
hildon_desktop_home_item_snap_to_grid (HildonDesktopHomeItem *applet)
{
  HildonDesktopHomeItemPriv      *priv;
  gint x_grid = 0, y_grid = 0;
  gint x, y;
  GtkWidget     *parent, *widget;

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);

  widget = GTK_WIDGET (applet);
  parent = widget->parent;

  if (!GTK_IS_FIXED (parent))
    return;

  x = widget->allocation.x - parent->allocation.x;
  y = widget->allocation.y - parent->allocation.y;

  switch (priv->state)
    {
      case HILDON_DESKTOP_HOME_ITEM_STATE_MOVING:

          if (priv->delta_x || priv->delta_y)
            {
              x_grid = x - (x % GRID_SIZE);
              if (priv->delta_x > 0
                  && !(x_grid + GRID_SIZE + widget->allocation.width >
                       parent->allocation.width))
                x_grid += GRID_SIZE;

              y_grid = y - (y % GRID_SIZE);
              if (priv->delta_y > 0
                  && !(y_grid + GRID_SIZE + widget->allocation.height >
                       parent->allocation.height))
                y_grid += GRID_SIZE;

              gtk_fixed_move (GTK_FIXED (parent), widget, x_grid, y_grid);
            }

          break;
      case HILDON_DESKTOP_HOME_ITEM_STATE_RESIZING:

          if (priv->delta_x || priv->delta_y)
            {
              x_grid = x + widget->allocation.width;

              if (priv->resize_type == HILDON_DESKTOP_HOME_ITEM_RESIZE_HORIZONTAL ||
                  priv->resize_type == HILDON_DESKTOP_HOME_ITEM_RESIZE_BOTH)
                {
                  x_grid -= x_grid % GRID_SIZE;

                  if (priv->delta_x > 0
                      && !(x_grid + GRID_SIZE > parent->allocation.width))
                    x_grid += GRID_SIZE;
                }

              y_grid = y + widget->allocation.height;

              if (priv->resize_type == HILDON_DESKTOP_HOME_ITEM_RESIZE_VERTICAL ||
                  priv->resize_type == HILDON_DESKTOP_HOME_ITEM_RESIZE_BOTH)
                {
                  y_grid -= y_grid % GRID_SIZE;

                  if (priv->delta_y > 0
                      && !(y_grid + GRID_SIZE > parent->allocation.height))
                    y_grid += GRID_SIZE;
                }

              gtk_widget_set_size_request (widget, x_grid - x, y_grid - y);
            }

          break;
      default:
          break;
    }
}

static gboolean
hildon_desktop_home_item_button_press_event (GtkWidget *w,
                                             GdkEventButton   *event)
{
  HildonDesktopHomeItemPriv      *priv;

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (HILDON_DESKTOP_HOME_ITEM (w));

  if (!priv->layout_mode_sucks && !priv->layout_mode)
    {
      if (GTK_WIDGET_CLASS (parent_class)->button_press_event)
        return GTK_WIDGET_CLASS (parent_class)->button_press_event (w, event);
      else
        return FALSE;
    }

  /* Check if we clicked the close button */
  if (event->window == priv->close_button_window)
    {
      if (HILDON_IS_HOME_AREA (w->parent))
        g_signal_emit_by_name (G_OBJECT (w->parent),
                               "applet-change-start",
                               w);
      gtk_widget_destroy (w);
      return TRUE;
    }

  if (!priv->timeout)
    {
      if (HILDON_IS_HOME_AREA (w->parent))
        g_signal_emit_by_name (G_OBJECT (w->parent),
                               "applet-change-start",
                               w);

      gdk_window_raise (w->window);
      gdk_pointer_grab (event->window,
                        FALSE,
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK,
                        NULL,
                        NULL,
                        event->time);

      priv->old_allocation = w->allocation;

      if (event->window == priv->resize_handle_window)
        {
          gint window_x, window_y;
          gint window_width, window_height;

          gdk_window_get_position (priv->resize_handle_window,
                                   &window_x,
                                   &window_y);
          gdk_drawable_get_size (GDK_DRAWABLE (priv->resize_handle_window),
                                 &window_width,
                                 &window_height);

          priv->x_offset = w->allocation.width -
                           window_x - event->x;
          priv->y_offset = w->allocation.height -
                           window_y - event->y;
          priv->state = HILDON_DESKTOP_HOME_ITEM_STATE_RESIZING;
        }
      else
        {
          priv->x_offset = event->x;
          priv->y_offset = event->y;
          priv->state = HILDON_DESKTOP_HOME_ITEM_STATE_MOVING;
        }

      priv->timeout = g_timeout_add (DRAG_UPDATE_TIMEOUT,
                                     (GSourceFunc)
                                            hildon_desktop_home_item_drag_update,
                                     w);

    }

  return TRUE;
}

static gboolean
hildon_desktop_home_item_button_release_event (GtkWidget *applet,
                                               GdkEventButton   *event)
{
  HildonDesktopHomeItemPriv      *priv;

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);

  if (!priv->layout_mode_sucks && !priv->layout_mode)
    {
      if  (GTK_WIDGET_CLASS (parent_class)->button_release_event)
        return GTK_WIDGET_CLASS (parent_class)->button_release_event (applet, event);
      else
        return FALSE;
    }

  if (priv->timeout)
    {
      gboolean snap_to_grid = FALSE;

      if (applet->parent)
        g_object_get (G_OBJECT (applet->parent),
                      "snap-to-grid", &snap_to_grid,
                      NULL);

      if (snap_to_grid)
        hildon_desktop_home_item_snap_to_grid (HILDON_DESKTOP_HOME_ITEM (applet));

      g_source_remove (priv->timeout);
      priv->timeout = 0;
      priv->state = HILDON_DESKTOP_HOME_ITEM_STATE_NORMAL;

      gdk_pointer_ungrab (event->time);

      /* We need to update the rectangle */
      gtk_widget_queue_draw (GTK_WIDGET (applet));
      if (priv->old_allocation.x != applet->allocation.x ||
          priv->old_allocation.y != applet->allocation.y ||
          priv->old_allocation.width != applet->allocation.width ||
          priv->old_allocation.height != applet->allocation.height)

        {
          if (HILDON_IS_HOME_AREA (applet->parent))
            g_signal_emit_by_name (G_OBJECT (applet->parent), "layout-changed");
        }

      if (HILDON_IS_HOME_AREA (applet->parent))
        g_signal_emit_by_name (G_OBJECT (applet->parent),
                               "applet-change-end",
                               applet);
    }

  return TRUE;
}

static gboolean
hildon_desktop_home_item_key_press_event (GtkWidget *widget,
                                          GdkEventKey *event)
{
  HildonDesktopHomeItemPriv      *priv;
  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (HILDON_DESKTOP_HOME_ITEM(widget));

  if (priv->layout_mode)
    return TRUE;

  if (GTK_WIDGET_CLASS (parent_class)->key_press_event)
    return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
    
  return FALSE;
}

static gboolean
hildon_desktop_home_item_key_release_event (GtkWidget *widget,
                                            GdkEventKey *event)
{
  HildonDesktopHomeItemPriv      *priv;
  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (HILDON_DESKTOP_HOME_ITEM(widget));

  if (priv->layout_mode)
    return TRUE;

  if (GTK_WIDGET_CLASS (parent_class)->key_release_event)
    return GTK_WIDGET_CLASS (parent_class)->key_release_event (widget, event);
    
  return FALSE;
}


/********************/
/* public functions */
/********************/

GtkWidget *
hildon_desktop_home_item_new (void)
{
  HildonDesktopHomeItem *newapplet = g_object_new(HILDON_DESKTOP_TYPE_HOME_ITEM,
                                             NULL);

  return GTK_WIDGET(newapplet);
}

gboolean
hildon_desktop_home_item_get_layout_mode (HildonDesktopHomeItem *applet)
{
  HildonDesktopHomeItemPriv      *priv;

  g_return_val_if_fail (applet, FALSE);

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);
  
  return priv->layout_mode;
}

void
hildon_desktop_home_item_set_layout_mode (HildonDesktopHomeItem *applet,
                                          gboolean layout_mode)
{
  HildonDesktopHomeItemPriv      *priv;
  g_return_if_fail (applet);

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);
  
  if (priv->layout_mode != layout_mode)
    {
      g_object_notify (G_OBJECT (applet), "layout-mode");
      priv->layout_mode = layout_mode;

      if (priv->layout_mode)
        g_signal_emit_by_name (G_OBJECT (applet), "layout-mode-start");
      else
        g_signal_emit_by_name (G_OBJECT (applet), "layout-mode-end");

    }
}

HildonDesktopHomeItemResizeType
hildon_desktop_home_item_get_resize_type (HildonDesktopHomeItem *applet)
{
  HildonDesktopHomeItemPriv      *priv;
  g_return_val_if_fail (applet, FALSE);
  
  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);
  
  return priv->resize_type;
}

void
hildon_desktop_home_item_set_resize_type (HildonDesktopHomeItem *applet,
                                          HildonDesktopHomeItemResizeType resize_type)
{
  HildonDesktopHomeItemPriv      *priv;
  g_return_if_fail (applet);

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);
          
  if (priv->resize_type != resize_type)
    {
      g_object_notify (G_OBJECT (applet), "resize-type");
      priv->resize_type = resize_type;
    }
}

GtkWidget *
hildon_desktop_home_item_get_settings_menu_item (HildonDesktopHomeItem *applet)
{
  HildonDesktopHomeItemPriv      *priv;
  GtkWindow                 *window;
  GtkWidget                 *item = NULL;
  g_return_val_if_fail (applet, NULL);
  
  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);
  
  window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (applet)));

  g_signal_emit_by_name (applet, "settings", window, &item);

  return item;
}

gboolean
hildon_desktop_home_item_get_overlaps (HildonDesktopHomeItem *applet)
{
  HildonDesktopHomeItemPriv      *priv;
  g_return_val_if_fail (applet, FALSE);

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);

  return priv->overlaps;
}

void
hildon_desktop_home_item_set_is_background (HildonDesktopHomeItem *applet,
                                            gboolean is_background)
{
  HildonDesktopHomeItemPriv      *priv;
  g_return_if_fail (applet);

  priv = HILDON_DESKTOP_HOME_ITEM_GET_PRIVATE (applet);

  if (is_background)
    g_signal_emit_by_name (applet, "background");
  else
    g_signal_emit_by_name (applet, "foreground");
}
