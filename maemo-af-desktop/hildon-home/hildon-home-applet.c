/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
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


/* Hildon include */
#include "hildon-home-applet.h"
#include "home-applet-handler.h"
#include "hildon-home-area.h"

#include "hildon-home-interface.h" /* .desktop keys */

#include <string.h> /* strcmp */
#include <gtk/gtkfixed.h>
#include <gtk/gtkicontheme.h>
#include <hildon-widgets/hildon-defines.h>



#define APPLET_RESIZE_HANDLE_ICON   "qgn_home_layoutmode_resize"
#define APPLET_RESIZE_HANDLE_WIDTH  40
#define APPLET_RESIZE_HANDLE_HEIGHT 40
#define APPLET_CLOSE_BUTTON_ICON    "qgn_home_layoutmode_close"
#define APPLET_CLOSE_BUTTON_WIDTH   26
#define APPLET_CLOSE_BUTTON_HEIGHT  26


#define DRAG_UPDATE_TIMEOUT 50
#define LAYOUT_MODE_HIGHLIGHT_WIDTH 4

#define HILDON_HOME_APPLET_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_TYPE_HOME_APPLET, HildonHomeAppletPriv));

typedef struct 
{
  gboolean above_child;
  GdkWindow *event_window;
} GtkEventBoxPrivate;

#define GTK_EVENT_BOX_GET_PRIVATE(obj)  G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_TYPE_EVENT_BOX, GtkEventBoxPrivate)


typedef enum 
{
  HILDON_HOME_APPLET_STATE_NORMAL,
  HILDON_HOME_APPLET_STATE_RESIZING,
  HILDON_HOME_APPLET_STATE_MOVING
} HildonHomeAppletState;

enum
{
  HILDON_HOME_APPLET_PROPERTY_RESIZE_TYPE = 1,
  HILDON_HOME_APPLET_PROPERTY_LAYOUT_MODE,
  HILDON_HOME_APPLET_PROPERTY_DESKTOP_FILE,
  HILDON_HOME_APPLET_PROPERTY_MINIMUM_WIDTH,
  HILDON_HOME_APPLET_PROPERTY_MINIMUM_HEIGHT
};

typedef struct HildonHomeAppletPriv_
{
  gchar        *desktop_file;
  gboolean      layout_mode;
  HildonHomeAppletResizeType resize_type;
  HomeAppletHandler *handler;

  GdkPixbuf    *close_button;
  GdkWindow    *close_button_window;
  GdkPixbuf    *resize_handle;
  GdkWindow    *resize_handle_window;

  gint          minimum_width;
  gint          minimum_height;

  /* Drag data */
  HildonHomeAppletState state;
  guint         x_offset;
  guint         y_offset;
  guint         timeout;
  gboolean      overlaps;
  GtkAllocation old_allocation;
} HildonHomeAppletPriv;

static GtkEventBoxClass *parent_class;

static void
hildon_home_applet_init (HildonHomeApplet * self);
static void
hildon_home_applet_class_init (HildonHomeAppletClass * applet_class);

static void
hildon_home_applet_destroy (GtkObject *applet);

static gboolean
hildon_home_applet_expose (GtkWidget * widget,
                           GdkEventExpose * event);

static void
hildon_home_applet_size_allocate (GtkWidget *widget,
                                  GtkAllocation *allocation);

static void
hildon_home_applet_remove (GtkContainer *applet, GtkWidget *child);

static void
hildon_home_applet_layout_mode_start (HildonHomeApplet *applet);

static void
hildon_home_applet_layout_mode_end (HildonHomeApplet *applet);

static void
hildon_home_applet_desktop_file_changed (HildonHomeApplet *applet);

static gboolean
hildon_home_applet_button_press_event (GtkWidget *applet,
                                       GdkEventButton   *event);

static gboolean
hildon_home_applet_button_release_event (GtkWidget *applet,
                                         GdkEventButton   *event);

static gboolean
hildon_home_applet_key_press_event (GtkWidget *applet,
                                    GdkEventKey *event);

static gboolean
hildon_home_applet_key_release_event (GtkWidget *applet,
                                      GdkEventKey *event);

static void
hildon_home_applet_check_overlap (HildonHomeApplet *applet1,
                                  HildonHomeApplet *applet2);

static gboolean
hildon_home_applet_visibility_notify_event (GtkWidget *applet,
                                            GdkEventVisibility *event);

static void
hildon_home_applet_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec);

static void
hildon_home_applet_get_property (GObject      *object,
                                 guint         property_id,
                                 GValue *value,
                                 GParamSpec   *pspec);


GType
hildon_home_applet_resize_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0)
    {
    
      static const GEnumValue values[] = {
          { HILDON_HOME_APPLET_RESIZE_NONE,
            "HILDON_HOME_APPLET_RESIZE_NONE",
            "none" },
          { HILDON_HOME_APPLET_RESIZE_VERTICAL,
            "HILDON_HOME_APPLET_RESIZE_VERTICAL",
            "vertical" },
          { HILDON_HOME_APPLET_RESIZE_HORIZONTAL,
            "HILDON_HOME_APPLET_RESIZE_HORIZONTAL",
            "horizontal" },
          { HILDON_HOME_APPLET_RESIZE_BOTH,
            "HILDON_HOME_APPLET_RESIZE_BOTH",
            "both" },
          { 0, NULL, NULL }
      };
      etype = g_enum_register_static ("HildonHomeAppletResizeType", values);
  }
  return etype;
}


GType hildon_home_applet_get_type(void)
{
  static GType applet_type = 0;

  if (!applet_type) {
    static const GTypeInfo applet_info = {
      sizeof(HildonHomeAppletClass),
      NULL,       /* base_init */
      NULL,       /* base_finalize */
      (GClassInitFunc) hildon_home_applet_class_init,
      NULL,       /* class_finalize */
      NULL,       /* class_data */
      sizeof(HildonHomeApplet),
      0,  /* n_preallocs */
      (GInstanceInitFunc) hildon_home_applet_init,
    };
    applet_type = g_type_register_static(GTK_TYPE_EVENT_BOX,
                                         "HildonHomeApplet",
                                         &applet_info, 0);
  }
  return applet_type;
}

static void
hildon_home_applet_class_init (HildonHomeAppletClass * applet_class)
{
  GParamSpec *pspec;

  /* Get convenience variables */
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (applet_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (applet_class);
  GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS   (applet_class);
  GObjectClass   *object_class = G_OBJECT_CLASS   (applet_class);

  g_type_class_add_private (applet_class, sizeof (HildonHomeAppletPriv));

  object_class->set_property = hildon_home_applet_set_property;
  object_class->get_property = hildon_home_applet_get_property;

  gtkobject_class->destroy = hildon_home_applet_destroy;

  /* Set the widgets virtual functions */
  widget_class->expose_event = hildon_home_applet_expose;
  widget_class->size_allocate = hildon_home_applet_size_allocate;
  widget_class->button_press_event = hildon_home_applet_button_press_event;
  widget_class->button_release_event = hildon_home_applet_button_release_event;
  widget_class->visibility_notify_event = 
      hildon_home_applet_visibility_notify_event;
  widget_class->key_press_event   = hildon_home_applet_key_press_event;
  widget_class->key_release_event = hildon_home_applet_key_release_event;

  /* We override remove to destroy the applet when it's child is destroyed */
  container_class->remove = hildon_home_applet_remove;

  applet_class->layout_mode_start     = hildon_home_applet_layout_mode_start;
  applet_class->layout_mode_end       = hildon_home_applet_layout_mode_end;
  applet_class->desktop_file_changed  = hildon_home_applet_desktop_file_changed;

  parent_class = g_type_class_peek_parent (applet_class);

  g_signal_new ("layout-mode-start",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAppletClass, layout_mode_start),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  g_signal_new ("layout-mode-end",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAppletClass, layout_mode_end),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);
  
  g_signal_new ("desktop-file-changed",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAppletClass, desktop_file_changed),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  pspec =  g_param_spec_enum  ("resize-type",
                               "Type of resizability",
                               "Whether the applet can be resized "
                               "vertically, horizontally, or both",
                               HILDON_HOME_APPLET_RESIZE_TYPE_TYPE,
                               HILDON_HOME_APPLET_RESIZE_NONE,
                               G_PARAM_READWRITE);

  g_object_class_install_property (object_class,
                                   HILDON_HOME_APPLET_PROPERTY_RESIZE_TYPE,
                                   pspec);
  
  pspec =  g_param_spec_boolean ("layout-mode",
                                 "Layout mode",
                                 "Whether the applet is in layout "
                                 "mode",
                                 FALSE,
                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   HILDON_HOME_APPLET_PROPERTY_LAYOUT_MODE,
                                   pspec);
  
  pspec =  g_param_spec_string ("desktop-file",
                                "Desktop file name",
                                "Path the applet's desktop file",
                                "",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   HILDON_HOME_APPLET_PROPERTY_DESKTOP_FILE,
                                   pspec);
  
  pspec =  g_param_spec_int ("minimum-width",
                             "Minimum width",
                             "Minimum width for this applet",
                             -1,
                             G_MAXINT,
                             -1,
                             G_PARAM_READWRITE);

  g_object_class_install_property (object_class,
                                   HILDON_HOME_APPLET_PROPERTY_MINIMUM_WIDTH,
                                   pspec);
  
  pspec =  g_param_spec_int ("minimum-height",
                             "Minimum height",
                             "Minimum height for this applet",
                             -1,
                             G_MAXINT,
                             -1,
                             G_PARAM_READWRITE);

  g_object_class_install_property (object_class,
                                   HILDON_HOME_APPLET_PROPERTY_MINIMUM_HEIGHT,
                                   pspec);

}

static void
hildon_home_applet_init (HildonHomeApplet * self)
{
  HildonHomeAppletPriv      *priv;
  GtkIconTheme              *icon_theme;
  GError                    *error = NULL;
  
  priv = HILDON_HOME_APPLET_GET_PRIVATE (self);

  priv->layout_mode = FALSE;
  
  icon_theme = gtk_icon_theme_get_default();
  priv->close_button = gtk_icon_theme_load_icon (icon_theme,
                                                 APPLET_CLOSE_BUTTON_ICON,
                                                 APPLET_CLOSE_BUTTON_WIDTH,
                                                 GTK_ICON_LOOKUP_NO_SVG,
                                                 &error);

  if (error)
    {
      g_warning ("Could not load close button icon: %s", error->message);
      priv->close_button = NULL;
    }
  
  gtk_widget_add_events (GTK_WIDGET (self), GDK_VISIBILITY_NOTIFY_MASK);

  /* FIXME remove this from the theme */
  gtk_widget_set_name (GTK_WIDGET (self), "osso-home-layoutmode-applet");

}

static void
hildon_home_applet_destroy (GtkObject *applet)
{
  HildonHomeAppletPriv      *priv;
  priv = HILDON_HOME_APPLET_GET_PRIVATE (HILDON_HOME_APPLET (applet));

  if (priv->timeout)
    g_source_remove (priv->timeout);

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

  g_free (priv->desktop_file);
  priv->desktop_file = NULL;
  
  if (priv->close_button)
    gdk_pixbuf_unref (priv->close_button);
  priv->close_button = NULL;
  
  if (priv->resize_handle)
    gdk_pixbuf_unref (priv->resize_handle);
  priv->resize_handle = NULL;

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (applet);
  
  if (priv->handler)
    {
      home_applet_handler_deinitialize (priv->handler);
      g_object_unref (priv->handler);
      priv->handler = NULL;
    }

}

static void
hildon_home_applet_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  HildonHomeAppletPriv      *priv;
  priv = HILDON_HOME_APPLET_GET_PRIVATE (HILDON_HOME_APPLET (object));

  switch (property_id)
    {
      case HILDON_HOME_APPLET_PROPERTY_RESIZE_TYPE:
          hildon_home_applet_set_resize_type (HILDON_HOME_APPLET (object),
                                              g_value_get_enum (value));
          break;
      case HILDON_HOME_APPLET_PROPERTY_LAYOUT_MODE:
          hildon_home_applet_set_layout_mode (HILDON_HOME_APPLET (object),
                                              g_value_get_boolean (value));
          break;
      case HILDON_HOME_APPLET_PROPERTY_DESKTOP_FILE:
          hildon_home_applet_set_desktop_file (HILDON_HOME_APPLET (object),
                                               g_value_get_string (value));
          break;
      case HILDON_HOME_APPLET_PROPERTY_MINIMUM_WIDTH:
          g_object_notify (object, "minimum-width");
          priv->minimum_width = g_value_get_int (value);
          break;
      case HILDON_HOME_APPLET_PROPERTY_MINIMUM_HEIGHT:
          g_object_notify (object, "minimum-height");
          priv->minimum_height = g_value_get_int (value);
          break;
         
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

static void
hildon_home_applet_get_property (GObject      *object,
                                 guint         property_id,
                                 GValue       *value,
                                 GParamSpec   *pspec)
{
  HildonHomeAppletPriv      *priv;
  priv = HILDON_HOME_APPLET_GET_PRIVATE (HILDON_HOME_APPLET (object));

  switch (property_id)
    {
      case HILDON_HOME_APPLET_PROPERTY_RESIZE_TYPE:
          g_value_set_enum (value, priv->resize_type);
          break;
      case HILDON_HOME_APPLET_PROPERTY_LAYOUT_MODE:
          g_value_set_boolean (value, priv->layout_mode);
          break;
      case HILDON_HOME_APPLET_PROPERTY_DESKTOP_FILE:
          g_value_set_string (value, priv->desktop_file);
          break;
      case HILDON_HOME_APPLET_PROPERTY_MINIMUM_WIDTH:
          g_value_set_int (value, priv->minimum_width);
          break;
      case HILDON_HOME_APPLET_PROPERTY_MINIMUM_HEIGHT:
          g_value_set_int (value, priv->minimum_height);
          break;
         
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}


static gboolean
hildon_home_applet_expose (GtkWidget * w, GdkEventExpose * event)
{
  HildonHomeAppletPriv      *priv;
  
  priv = HILDON_HOME_APPLET_GET_PRIVATE (w);
  
  if (GTK_WIDGET_CLASS (parent_class)->expose_event)
    GTK_WIDGET_CLASS (parent_class)->expose_event(w, event);
  
  if (w->parent && priv->layout_mode)
    {
      /* Draw the rectangle around */

      if (priv->overlaps)
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
      else if (priv->state == HILDON_HOME_APPLET_STATE_RESIZING)
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
      else /* Normal */
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
hildon_home_applet_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  HildonHomeAppletPriv      *priv;
  priv = HILDON_HOME_APPLET_GET_PRIVATE (HILDON_HOME_APPLET(widget));

  if (priv->layout_mode && priv->resize_handle_window)
    {
      gdk_window_move (priv->resize_handle_window,
                       allocation->width - APPLET_RESIZE_HANDLE_WIDTH,
                       allocation->height - APPLET_RESIZE_HANDLE_HEIGHT);
    }

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
}

static void
hildon_home_applet_remove (GtkContainer *applet, GtkWidget *child)
{
  if (GTK_CONTAINER_CLASS (parent_class)->remove)
    GTK_CONTAINER_CLASS (parent_class)->remove (applet, child);

  gtk_widget_destroy (GTK_WIDGET (applet));
}

static GdkWindow *
hildon_home_applet_create_icon_window (HildonHomeApplet *applet,
                                       GdkPixbuf *icon,
                                       gint x,
                                       gint y)
{
  HildonHomeAppletPriv      *priv;
  GdkWindowAttr              attributes;
  gint                       attributes_mask;
  GdkBitmap                 *bitmask = NULL;
  GdkPixmap                 *pixmap = NULL;
  GdkWindow                 *window;
  GtkWidget                 *w;

  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);
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

  gdk_window_set_user_data (window, w);
  gdk_window_move (window, x, y);

  if (pixmap)
    gdk_window_set_back_pixmap (window, pixmap, FALSE);
  
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
                     HildonHomeApplet *applet)
{
  XAnyEvent *aevent = (XAnyEvent *)xevent;

  if (aevent->type == MapNotify)
    {
      HildonHomeAppletPriv *priv = HILDON_HOME_APPLET_GET_PRIVATE (applet); 
      GtkEventBoxPrivate *eb_priv = GTK_EVENT_BOX_GET_PRIVATE (applet);
      /* The applet created a child window, we need to make sure that
       * we remain on top */
#if 0
      gtk_event_box_set_above_child (GTK_EVENT_BOX (applet), FALSE);
      gtk_event_box_set_above_child (GTK_EVENT_BOX (applet), TRUE);
#endif
      if (eb_priv->event_window)
        gdk_window_raise (eb_priv->event_window);

      if (priv->close_button_window)
          gdk_window_raise (priv->close_button_window);
      if (priv->resize_handle_window)
        gdk_window_raise (priv->resize_handle_window);

      if (GTK_BIN (applet) && GTK_BIN (applet)->child)
        {
        gtk_widget_set_sensitive (GTK_BIN (applet)->child, TRUE);
        gtk_widget_set_sensitive (GTK_BIN (applet)->child, FALSE);
        }
    }

  return GDK_FILTER_CONTINUE;

}


static void
hildon_home_applet_layout_mode_start (HildonHomeApplet *applet)
{
  HildonHomeAppletPriv      *priv;
  GtkWidget                 *w;
 
  w = GTK_WIDGET (applet);
  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);

  gtk_widget_realize (w);

  gtk_event_box_set_above_child    (GTK_EVENT_BOX (applet), TRUE);
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (applet), TRUE);


  if (priv->close_button)
    {
      priv->close_button_window = hildon_home_applet_create_icon_window 
          (applet,
           priv->close_button,
           HILDON_MARGIN_DEFAULT,
           HILDON_MARGIN_DEFAULT);
    }

  if (priv->resize_handle)
    {
      priv->resize_handle_window = hildon_home_applet_create_icon_window 
          (applet,
           priv->resize_handle,
           w->allocation.width - APPLET_RESIZE_HANDLE_WIDTH,
           w->allocation.height - APPLET_RESIZE_HANDLE_HEIGHT);
    }

  if (GTK_BIN (applet) && GTK_BIN (applet)->child)
    gtk_widget_set_sensitive (GTK_BIN (applet)->child, FALSE);

  gdk_window_set_events (w->window,
                         gdk_window_get_events(w->window) |
                           GDK_SUBSTRUCTURE_MASK);
  gdk_window_add_filter (w->window,
                         (GdkFilterFunc)window_event_filter,
                         applet);
  
  gtk_widget_show (w);

  priv->layout_mode = TRUE;
}

static void
hildon_home_applet_layout_mode_end (HildonHomeApplet *applet)
{
  HildonHomeAppletPriv      *priv;
  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);

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
  if (GTK_BIN (applet) && GTK_BIN (applet)->child)
    gtk_widget_set_sensitive (GTK_BIN (applet)->child, TRUE);

  gdk_window_remove_filter (GTK_WIDGET (applet)->window,
                            (GdkFilterFunc)window_event_filter,
                            applet); 

  gtk_event_box_set_visible_window (GTK_EVENT_BOX (applet), FALSE);
  gtk_event_box_set_above_child    (GTK_EVENT_BOX (applet), FALSE);
  gtk_widget_show (GTK_WIDGET (applet));
}

static void
hildon_home_applet_desktop_file_changed (HildonHomeApplet *applet)
{
  HildonHomeAppletPriv *priv;
  GtkWidget *applet_widget;
  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);
  gboolean horizontal, vertical;
  gint width, height;
  gint state_size = 0;
  void *state_data = NULL;

  if (!priv->desktop_file)
    return;
  
  priv->handler = home_applet_handler_new (priv->desktop_file,
                                           state_data,
                                           &state_size);

  if (!priv->handler)
    return;

  applet_widget = home_applet_handler_get_widget (priv->handler);
  
  if (!applet_widget)
    return;

  if (GTK_IS_WIDGET (applet_widget))
    gtk_container_add (GTK_CONTAINER (applet), applet_widget);

  home_applet_handler_get_resizable (priv->handler, &horizontal, &vertical);

  if (horizontal && vertical)
    hildon_home_applet_set_resize_type (applet, HILDON_HOME_APPLET_RESIZE_BOTH);
  else if (horizontal)
    hildon_home_applet_set_resize_type (applet,
                                        HILDON_HOME_APPLET_RESIZE_HORIZONTAL);
  else if (vertical)
    hildon_home_applet_set_resize_type (applet,
                                        HILDON_HOME_APPLET_RESIZE_VERTICAL);
  else
    hildon_home_applet_set_resize_type (applet, HILDON_HOME_APPLET_RESIZE_NONE);

#if 0
  home_applet_handler_get_size (priv->handler, &width, &height);
  gtk_widget_set_size_request (GTK_WIDGET (applet), width, height);
#endif
  
  home_applet_handler_get_minimum_size (priv->handler, &width, &height);

  if (width > 0)
    {
      g_object_notify (G_OBJECT (applet), "minimum-width");
      priv->minimum_width = width;
    }

  if (height > 0)
    {
      g_object_notify (G_OBJECT (applet), "minimum-height");
      priv->minimum_height = height;
    }

  /* FIXME: This should not be called, but some applets (clock)
   * currently require this */
  gtk_widget_show_all (applet_widget);

}

static gboolean
hildon_home_applet_visibility_notify_event (GtkWidget *applet,
                                            GdkEventVisibility *event)
{
  HildonHomeAppletPriv *priv;
  priv = HILDON_HOME_APPLET_GET_PRIVATE (HILDON_HOME_APPLET (applet));

  if (priv->layout_mode)
    {
      priv->overlaps = FALSE;
        {
          /* This is necessary, because it may overlapping with an underlying
           * window */
          gtk_container_foreach (GTK_CONTAINER (GTK_WIDGET (applet)->parent),
                                 (GtkCallback)hildon_home_applet_check_overlap,
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
hildon_home_applet_drag_update (HildonHomeApplet *applet)
{
  HildonHomeAppletPriv *priv;
  gint                  x_applet, y_applet;
  GdkModifierType       mod;
  GtkWidget            *fixed;
  GtkWidget            *widget;
  gboolean              used_to_overlap;

  widget = GTK_WIDGET (applet);

  if (!widget->parent)
    return FALSE;

  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);

  fixed = widget->parent;

  gdk_window_get_pointer (fixed->window, &x_applet, &y_applet, &mod);

  if (priv->state == HILDON_HOME_APPLET_STATE_MOVING)
    {
      /* The Fixed has no window, thus the coordinates are relative to the
       * home's main window */

      x_applet -= fixed->allocation.x;
      y_applet -= fixed->allocation.y;

      x_applet -= priv->x_offset;
      y_applet -= priv->y_offset;

      if (x_applet < LAYOUT_AREA_LEFT_BORDER_PADDING)
        x_applet = LAYOUT_AREA_LEFT_BORDER_PADDING;

      if (y_applet < LAYOUT_AREA_TOP_BORDER_PADDING)
        y_applet = LAYOUT_AREA_TOP_BORDER_PADDING;

      if (x_applet + widget->allocation.width > fixed->allocation.width
          - LAYOUT_AREA_RIGHT_BORDER_PADDING)
        x_applet =  fixed->allocation.width -
            widget->allocation.width - LAYOUT_AREA_RIGHT_BORDER_PADDING;


      if (y_applet + widget->allocation.height > fixed->allocation.height
          - LAYOUT_AREA_BOTTOM_BORDER_PADDING)
        y_applet =  fixed->allocation.height -
            widget->allocation.height - LAYOUT_AREA_BOTTOM_BORDER_PADDING;

      gtk_fixed_move (GTK_FIXED (fixed), widget, x_applet, y_applet);
    }
  else /* Resizing */
    {
      gint width, height;
      
      if (priv->resize_type == HILDON_HOME_APPLET_RESIZE_HORIZONTAL
          || priv->resize_type == HILDON_HOME_APPLET_RESIZE_BOTH)
        width  = x_applet + priv->x_offset - widget->allocation.x;
      else
        width = widget->requisition.width;

      if (priv->resize_type == HILDON_HOME_APPLET_RESIZE_VERTICAL
          || priv->resize_type == HILDON_HOME_APPLET_RESIZE_BOTH)
        height = y_applet + priv->y_offset - widget->allocation.y;
      else
        height = widget->requisition.height;
      
      if (priv->minimum_width > 0 && width < priv->minimum_width)
        width = priv->minimum_width;
      
      if (priv->minimum_height > 0 && height < priv->minimum_height)
        height = priv->minimum_height;

      if (widget->allocation.x + width > 
          fixed->allocation.x + fixed->allocation.width
          - LAYOUT_AREA_RIGHT_BORDER_PADDING)
        width = fixed->allocation.x + fixed->allocation.width
          - LAYOUT_AREA_RIGHT_BORDER_PADDING - widget->allocation.x;
      
      if (widget->allocation.y + height > 
          fixed->allocation.y + fixed->allocation.height
          - LAYOUT_AREA_RIGHT_BORDER_PADDING)
        height = fixed->allocation.y + fixed->allocation.height
          - LAYOUT_AREA_BOTTOM_BORDER_PADDING - widget->allocation.y;

      gtk_widget_set_size_request (widget, width, height);
#if 0
      if (priv->resize_handle_window)
        gdk_window_move (priv->resize_handle_window,
                         width - APPLET_RESIZE_HANDLE_WIDTH,
                         height - APPLET_RESIZE_HANDLE_HEIGHT);
#endif
    }
  
  used_to_overlap = priv->overlaps;
  priv->overlaps = FALSE;
  gtk_container_foreach (GTK_CONTAINER (GTK_WIDGET (applet)->parent),
                         (GtkCallback)hildon_home_applet_check_overlap,
                         applet);
  if (used_to_overlap != priv->overlaps)
    gtk_widget_queue_draw (widget);


  return TRUE;
}

static void
hildon_home_applet_check_overlap (HildonHomeApplet *applet1,
                                  HildonHomeApplet *applet2)
{
  HildonHomeAppletPriv      *priv2;
  GdkRectangle               r;

  if (applet1 == applet2)
    return;

  priv2 = HILDON_HOME_APPLET_GET_PRIVATE (applet2);
  
  if (gdk_rectangle_intersect (&GTK_WIDGET (applet1)->allocation,
                               &(GTK_WIDGET (applet2)->allocation),
                               &r))
    {
      priv2->overlaps = TRUE;
    }


}

static gboolean
hildon_home_applet_button_press_event (GtkWidget *w,
                                       GdkEventButton   *event)
{
  HildonHomeAppletPriv      *priv;

  priv = HILDON_HOME_APPLET_GET_PRIVATE (HILDON_HOME_APPLET (w));

  if (!priv->layout_mode)
    {
      if (GTK_WIDGET_CLASS (parent_class)->button_press_event)
        return GTK_WIDGET_CLASS (parent_class)->button_press_event (w, event);
      else
        return FALSE;
    }
  
  /* Check if we clicked the close button */
  if (event->x > HILDON_MARGIN_DEFAULT && 
      event->x < HILDON_MARGIN_DEFAULT + APPLET_CLOSE_BUTTON_WIDTH &&
      event->y > HILDON_MARGIN_DEFAULT &&
      event->y < HILDON_MARGIN_DEFAULT + APPLET_CLOSE_BUTTON_HEIGHT)
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
      gdk_pointer_grab (w->window,
                        FALSE,
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK,
                        NULL,
                        NULL,
                        event->time);

      priv->old_allocation = w->allocation;

      if (priv->resize_type != HILDON_HOME_APPLET_RESIZE_NONE &&
          (w->allocation.width - event->x) < APPLET_RESIZE_HANDLE_WIDTH &&
          (w->allocation.height - event->y) < APPLET_RESIZE_HANDLE_HEIGHT)
        {
          priv->x_offset = w->allocation.width - event->x;
          priv->y_offset = w->allocation.height - event->y;
          priv->state = HILDON_HOME_APPLET_STATE_RESIZING;
        }
      else
        {
          priv->x_offset = event->x;
          priv->y_offset = event->y;
          priv->state = HILDON_HOME_APPLET_STATE_MOVING;
        }

      priv->timeout = g_timeout_add (DRAG_UPDATE_TIMEOUT,
                                     (GSourceFunc)
                                            hildon_home_applet_drag_update,
                                     w);

    }
  return TRUE;
}

static gboolean
hildon_home_applet_button_release_event (GtkWidget *applet,
                                         GdkEventButton   *event)
{
  HildonHomeAppletPriv      *priv;

  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);
  if (!priv->layout_mode)
    {
      if  (GTK_WIDGET_CLASS (parent_class)->button_release_event)
        return GTK_WIDGET_CLASS (parent_class)->button_release_event (applet, event);
      else
        return FALSE;
    }

  if (priv->timeout)
    {
      g_source_remove (priv->timeout);
      priv->timeout = 0;
      priv->state = HILDON_HOME_APPLET_STATE_NORMAL;

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
hildon_home_applet_key_press_event (GtkWidget *widget, GdkEventKey *event)
{
  HildonHomeAppletPriv      *priv;
  priv = HILDON_HOME_APPLET_GET_PRIVATE (HILDON_HOME_APPLET(widget));

  if (priv->layout_mode)
    return TRUE;

  if (GTK_WIDGET_CLASS (parent_class)->key_press_event)
    return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
    
  return FALSE;
}

static gboolean
hildon_home_applet_key_release_event (GtkWidget *widget, GdkEventKey *event)
{
  HildonHomeAppletPriv      *priv;
  priv = HILDON_HOME_APPLET_GET_PRIVATE (HILDON_HOME_APPLET(widget));

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
hildon_home_applet_new (void)
{
  HildonHomeApplet *newapplet = g_object_new(HILDON_TYPE_HOME_APPLET,
                                             "visible-window", FALSE,
                                             "above-child", FALSE,
                                             NULL);

  return GTK_WIDGET(newapplet);
}

GtkWidget *
hildon_home_applet_new_with_plugin (const gchar *desktop_file)
{
  HildonHomeAppletPriv      *priv;
  HildonHomeApplet *newapplet = g_object_new (HILDON_TYPE_HOME_APPLET,
                                              "desktop-file",
                                              desktop_file,   
                                             "visible-window", FALSE,
                                             "above-child", FALSE,
                                              NULL);
  /* FIXME */
  priv = HILDON_HOME_APPLET_GET_PRIVATE (newapplet);
  if (!priv->handler)
    return NULL;
    
  return GTK_WIDGET (newapplet);
}

gboolean
hildon_home_applet_get_layout_mode (HildonHomeApplet *applet)
{
  HildonHomeAppletPriv      *priv;

  g_return_val_if_fail (applet, FALSE);

  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);
  
  return priv->layout_mode;
}

void
hildon_home_applet_set_layout_mode (HildonHomeApplet *applet,
                                    gboolean layout_mode)
{
  HildonHomeAppletPriv      *priv;
  g_return_if_fail (applet);

  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);
  
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

HildonHomeAppletResizeType
hildon_home_applet_get_resize_type (HildonHomeApplet *applet)
{
  HildonHomeAppletPriv      *priv;
  g_return_val_if_fail (applet, FALSE);
  
  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);
  
  return priv->resize_type;
}

void
hildon_home_applet_set_resize_type (HildonHomeApplet *applet,
                                    HildonHomeAppletResizeType resize_type)
{
  HildonHomeAppletPriv      *priv;
  g_return_if_fail (applet);

  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);
          
  if (priv->resize_type != resize_type)
    {
      g_object_notify (G_OBJECT (applet), "resize-type");
      priv->resize_type = resize_type;
      if (resize_type != HILDON_HOME_APPLET_RESIZE_NONE && !priv->resize_handle)
        {
          GtkIconTheme              *icon_theme;
          GError                    *error = NULL;

          icon_theme = gtk_icon_theme_get_default();
          priv->resize_handle = 
              gtk_icon_theme_load_icon (icon_theme,
                                        APPLET_RESIZE_HANDLE_ICON,
                                        APPLET_RESIZE_HANDLE_WIDTH,
                                        GTK_ICON_LOOKUP_NO_SVG,
                                        &error);

          if (error)
            {
              g_warning ("Could not load resize handle icon: %s", error->message);
              priv->resize_handle = NULL;
            }
        }
    }
}

void
hildon_home_applet_set_desktop_file (HildonHomeApplet *applet,
                                     const gchar *desktop_file)
{
  HildonHomeAppletPriv      *priv;
  g_return_if_fail (applet);

  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);

  if ((desktop_file && !priv->desktop_file) || 
      (!desktop_file && priv->desktop_file) ||
      strcmp (desktop_file, priv->desktop_file))
    {
      g_object_notify (G_OBJECT (applet), "desktop-file");
      g_free (priv->desktop_file);
      priv->desktop_file = g_strdup (desktop_file);
        
      g_signal_emit_by_name (G_OBJECT (applet), "desktop-file-changed");
    }
}

const gchar *
hildon_home_applet_get_desktop_file (HildonHomeApplet *applet)
{
  HildonHomeAppletPriv      *priv;
  g_return_val_if_fail (applet, FALSE);
  
  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);
  
  return priv->desktop_file;
}

GtkWidget *
hildon_home_applet_get_settings_menu_item (HildonHomeApplet *applet)
{
  HildonHomeAppletPriv      *priv;
  GtkWindow                 *window;
  g_return_val_if_fail (applet, NULL);
  
  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);

  if (!priv->handler)
    return NULL;

  window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (applet)));

  return home_applet_handler_settings (priv->handler, window);
}

void
hildon_home_applet_save_position (HildonHomeApplet *applet, GKeyFile *keyfile)
{
  HildonHomeAppletPriv      *priv;
  g_return_if_fail (applet && keyfile);
  
  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);

  if (!priv->desktop_file)
    return;

  g_key_file_set_integer (keyfile,
                          priv->desktop_file,
                          APPLET_KEY_X,
                          GTK_WIDGET (applet)->allocation.x - 
                          GTK_WIDGET (applet)->parent->allocation.x);

  g_key_file_set_integer (keyfile,
                          priv->desktop_file,
                          APPLET_KEY_Y,
                          GTK_WIDGET (applet)->allocation.y -
                          GTK_WIDGET (applet)->parent->allocation.y);

  g_key_file_set_integer (keyfile,
                          priv->desktop_file,
                          APPLET_KEY_WIDTH,
                          GTK_WIDGET (applet)->allocation.width);

  g_key_file_set_integer (keyfile,
                          priv->desktop_file,
                          APPLET_KEY_HEIGHT,
                          GTK_WIDGET (applet)->allocation.height);

}

gint
hildon_home_applet_find_by_name (HildonHomeApplet *applet, const gchar *name)
{
  HildonHomeAppletPriv      *priv;
  g_return_val_if_fail (applet && name, -2);

  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);

  if (!priv->desktop_file)
    return -1;

  return strcmp (priv->desktop_file, name);
  
}

gboolean
hildon_home_applet_get_overlaps (HildonHomeApplet *applet)
{
  HildonHomeAppletPriv      *priv;
  g_return_val_if_fail (applet, FALSE);

  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);

  return priv->overlaps;
}

void
hildon_home_applet_set_is_background (HildonHomeApplet *applet,
                                      gboolean is_background)
{
  HildonHomeAppletPriv      *priv;
  g_return_if_fail (applet);

  priv = HILDON_HOME_APPLET_GET_PRIVATE (applet);

  if (!priv->handler)
    return;

  if (is_background)
    home_applet_handler_background (priv->handler);
  else
    home_applet_handler_foreground (priv->handler);
}
