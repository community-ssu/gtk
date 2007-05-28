/*
 * This file is part of hildon-desktop

 * Copyright (C) 2006, 2007 Nokia Corporation.

 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author:  Johan Bilien <johan.bilien@nokia.com>

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.

 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA

 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "hildon-home-area.h"

#include <libhildondesktop/hildon-desktop-home-item.h>

#ifdef HAVE_X_COMPOSITE
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
#endif

#include <errno.h>
#include <string.h> /* malloc */

enum
{
  PROP_LAYOUT_MODE = 1,
  PROP_SNAP_TO_GRID,
  PROP_APPLET_PADDING,
  PROP_APPLET_DEFAULT_ALPHA,
  PROP_CHILD_DATA

};

#ifdef HAVE_X_COMPOSITE
typedef struct
{
  GtkWidget    *widget;
  GtkAllocation old_allocation;
  Window        window;
  Damage        damage;
  Picture       picture;
  Picture       alpha_mask;
  Picture       alpha_mask_unscaled;
  gint          background_width, background_height;
  gulong        realize_handler, style_set_handler, size_allocate_handler;
} ChildData;
#endif

typedef struct HildonHomeAreaPriv_
{
  gboolean      layout_mode;
  gboolean      layout_changed;

  gboolean      snap_to_grid;

  gint          applet_padding;

  GHashTable   *layout;

  GList        *to_add;
  gboolean      batch_add;

  GList        *children_data;

  gdouble       default_alpha;

#ifdef HAVE_X_COMPOSITE
  Picture       picture;
#endif
  gint          x_offset;
  gint          y_offset;

} HildonHomeAreaPriv;

#define HILDON_HOME_AREA_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_TYPE_HOME_AREA, HildonHomeAreaPriv));


static GtkFixedClass *parent_class;

static void
hildon_home_area_class_init (HildonHomeAreaClass *klass);

static void
hildon_home_area_init (HildonHomeArea *area);

static void
hildon_home_area_layout_mode_start (HildonHomeArea *area);

static void
hildon_home_area_layout_mode_end (HildonHomeArea *area);

static void
hildon_home_area_layout_changed (HildonHomeArea *area);

static void
hildon_home_area_applet_added (HildonHomeArea *area, GtkWidget *applet);

static void
hildon_home_area_add (GtkContainer *area, GtkWidget *applet);

static void
hildon_home_area_remove (GtkContainer *area, GtkWidget *applet);

static void
hildon_home_area_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec);

static void
hildon_home_area_get_property (GObject      *object,
                               guint         property_id,
                               GValue *value,
                               GParamSpec   *pspec);

#ifdef HAVE_X_COMPOSITE

static void
hildon_home_area_realize (GtkWidget *widget);

static void
hildon_home_area_unrealize (GtkWidget *widget);

static gboolean
hildon_home_area_expose (GtkWidget      *widget,
                         GdkEventExpose *event);

static void
hildon_home_area_set_child_property (GtkContainer    *container,
                                     GtkWidget       *child,
                                     guint            property_id,
                                     const GValue    *value,
                                     GParamSpec      *pspec);

static void
hildon_home_area_get_child_property (GtkContainer    *container,
                                     GtkWidget       *child,
                                     guint            property_id,
                                     GValue          *value,
                                     GParamSpec      *pspec);

static void
hildon_home_area_create_child_data (HildonHomeArea     *area,
                                    GtkWidget          *child);

static void
hildon_home_area_child_style_set (GtkWidget            *child,
                                  GtkStyle             *old_style,
                                  HildonHomeArea       *area);

static void
hildon_home_area_child_size_allocate (GtkWidget        *child,
                                      GtkAllocation    *allocation,
                                      HildonHomeArea   *area);

static void
hildon_home_area_child_build_alpha_mask_unscaled (HildonHomeArea       *area,
                                                  GtkWidget            *child);

static void
hildon_home_area_child_build_alpha_mask (HildonHomeArea       *area,
                                         GtkWidget            *child);

static void
hildon_home_area_child_realize (GtkWidget             *child,
                                HildonHomeArea        *area);

#endif


static void
hildon_home_area_finalize (GObject *object);



#ifdef HAVE_X_COMPOSITE
static void
child_data_free (ChildData *child_data)
{
  if (child_data->alpha_mask != None &&
      child_data->alpha_mask != child_data->alpha_mask_unscaled)
    XRenderFreePicture (GDK_DISPLAY (), child_data->alpha_mask);

  if (child_data->alpha_mask_unscaled != None)
    XRenderFreePicture (GDK_DISPLAY (), child_data->alpha_mask_unscaled);

  if (child_data->picture)
    XRenderFreePicture (GDK_DISPLAY (), child_data->picture);

  if (child_data->damage)
    XDamageDestroy (GDK_DISPLAY (), child_data->damage);

  g_free (child_data);

}
#endif

GType
hildon_home_area_get_type (void)
{
  static GType area_type = 0;

  if ( !area_type )
    {
      static const GTypeInfo area_info =
        {
          sizeof (HildonHomeAreaClass),
          NULL, /* base_init */
          NULL, /* base_finalize */
          (GClassInitFunc) hildon_home_area_class_init,
          NULL, /* class_finalize */
          NULL, /* class_data */
          sizeof (HildonHomeArea),
          0,    /* n_preallocs */
          (GInstanceInitFunc) hildon_home_area_init,
        };

      area_type = g_type_register_static (GTK_TYPE_FIXED,
                                          "HildonHomeArea",
                                          &area_info,0);
  }

  return area_type;
}



static void
hildon_home_area_class_init (HildonHomeAreaClass *klass)
{
  GObjectClass         *object_class;
  GtkWidgetClass       *widget_class;
  GtkContainerClass    *container_class;
  GParamSpec           *pspec;
#ifdef HAVE_X_COMPOSITE
  int                   damage_error, composite_error, composite_event_base;
#endif

  object_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);
  container_class = GTK_CONTAINER_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

#ifdef HAVE_X_COMPOSITE
  if (XDamageQueryExtension (GDK_DISPLAY (),
                             &klass->xdamage_event_base,
                             &damage_error) &&

      XCompositeQueryExtension (GDK_DISPLAY (),
                                &composite_event_base,
                                &composite_error))
    {
      klass->composite = TRUE;

      gdk_x11_register_standard_event_type (gdk_display_get_default (),
                                            klass->xdamage_event_base +
                                            XDamageNotify,
                                            1);
    }
  else
    klass->composite = FALSE;
#endif

  g_type_class_add_private (klass, sizeof (HildonHomeAreaPriv));

  klass->layout_mode_start = hildon_home_area_layout_mode_start;
  klass->layout_mode_end   = hildon_home_area_layout_mode_end;
  klass->layout_changed    = hildon_home_area_layout_changed;
  klass->applet_added      = hildon_home_area_applet_added;

#ifdef HAVE_X_COMPOSITE
  if (klass->composite)
    {
      widget_class->realize      = hildon_home_area_realize;
      widget_class->unrealize    = hildon_home_area_unrealize;
      widget_class->expose_event = hildon_home_area_expose;
    }
#endif

  container_class->add    = hildon_home_area_add;
  container_class->remove = hildon_home_area_remove;

#ifdef HAVE_X_COMPOSITE
  container_class->set_child_property = hildon_home_area_set_child_property;
  container_class->get_child_property = hildon_home_area_get_child_property;
#endif

  object_class->set_property = hildon_home_area_set_property;
  object_class->get_property = hildon_home_area_get_property;
  object_class->finalize = hildon_home_area_finalize;

  g_signal_new ("layout-mode-start",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, layout_mode_start),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  g_signal_new ("layout-mode-started",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, layout_mode_started),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  g_signal_new ("layout-mode-end",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, layout_mode_end),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  g_signal_new ("layout-mode-ended",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, layout_mode_ended),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  g_signal_new ("applet-added",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, applet_added),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__POINTER,
                G_TYPE_NONE,
                1,
                GTK_TYPE_WIDGET);

  g_signal_new ("applet-selected",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, applet_selected),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__POINTER,
                G_TYPE_NONE,
                1,
                GTK_TYPE_WIDGET);

  g_signal_new ("applet-change-start",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, applet_change_start),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__POINTER,
                G_TYPE_NONE,
                1,
                GTK_TYPE_WIDGET);

  g_signal_new ("applet-change-end",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, applet_change_end),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__POINTER,
                G_TYPE_NONE,
                1,
                GTK_TYPE_WIDGET);

  g_signal_new ("layout-changed",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeAreaClass, layout_changed),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);


  pspec =  g_param_spec_boolean ("layout-mode",
                                 "Layout mode",
                                 "Whether the home area is in layout "
                                 "mode",
                                 FALSE,
                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   PROP_LAYOUT_MODE,
                                   pspec);

  pspec =  g_param_spec_boolean ("snap-to-grid",
                                 "Snap to grid",
                                 "Whether applets should snap to grid",
                                 TRUE,
                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   PROP_SNAP_TO_GRID,
                                   pspec);


  pspec =  g_param_spec_int ("applet-padding",
                             "Applet padding",
                             "Padding between newly added applets",
                             0,
                             G_MAXINT,
                             10,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   PROP_APPLET_PADDING,
                                   pspec);

  pspec =  g_param_spec_int ("applet-default-alpha",
                             "Applet default alpha",
                             "Default value for the alpha channel "
                             "(in percentage)",
                             0,
                             100,
                             75,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   PROP_APPLET_DEFAULT_ALPHA,
                                   pspec);

#ifdef HAVE_X_COMPOSITE
  pspec = g_param_spec_pointer ("child-data",
                                "Child data",
                                "Data used for compositing",
                                G_PARAM_READWRITE);

  gtk_container_class_install_child_property (container_class,
                                              PROP_CHILD_DATA,
                                              pspec);
#endif

}

static void
hildon_home_area_init (HildonHomeArea *area)
{
  gtk_fixed_set_has_window (GTK_FIXED (area), FALSE);
}

static void
hildon_home_area_finalize (GObject *object)
{
  HildonHomeAreaPriv *priv;
  priv = HILDON_HOME_AREA_GET_PRIVATE (object);

  if (priv->layout)
    {
      g_hash_table_destroy (priv->layout);
      priv->layout = NULL;
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
hildon_home_area_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  HildonHomeAreaPriv      *priv;
  priv = HILDON_HOME_AREA_GET_PRIVATE (HILDON_HOME_AREA (object));

  switch (property_id)
    {
      case PROP_LAYOUT_MODE:
          hildon_home_area_set_layout_mode (HILDON_HOME_AREA (object),
                                            g_value_get_boolean (value));
           break;

      case PROP_SNAP_TO_GRID:
           priv->snap_to_grid = g_value_get_boolean (value);
           break;

      case PROP_APPLET_PADDING:
           priv->applet_padding = g_value_get_int (value);
           break;

      case PROP_APPLET_DEFAULT_ALPHA:
           priv->default_alpha = (gdouble)g_value_get_int (value) / 100;
           break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

static void
hildon_home_area_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  HildonHomeAreaPriv      *priv;
  priv = HILDON_HOME_AREA_GET_PRIVATE (HILDON_HOME_AREA (object));

  switch (property_id)
    {
      case PROP_LAYOUT_MODE:
          g_value_set_boolean (value, priv->layout_mode);
          break;

      case PROP_SNAP_TO_GRID:
           g_value_set_boolean (value, priv->snap_to_grid);
           break;

      case PROP_APPLET_PADDING:
           g_value_set_int (value, priv->applet_padding);
           break;

      case PROP_APPLET_DEFAULT_ALPHA:
           g_value_set_int (value, (gint)(priv->default_alpha * 100));
           break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

#ifdef HAVE_X_COMPOSITE
static gint
find_by_widget (ChildData *data, GtkWidget *widget)
{
  return !(data->widget == widget);
}

static ChildData *
hildon_home_area_get_child_data (HildonHomeArea *area,
                                 GtkWidget      *child)
{
  HildonHomeAreaPriv   *priv = HILDON_HOME_AREA_GET_PRIVATE (area);
  GList                *list_item;

  list_item = g_list_find_custom (priv->children_data,
                                  child, 
                                  (GCompareFunc)find_by_widget);


  if (list_item)
    return (ChildData *)list_item->data;
  else
    return NULL;
}

static void
hildon_home_area_set_child_property (GtkContainer    *container,
                                     GtkWidget       *child,
                                     guint            property_id,
                                     const GValue    *value,
                                     GParamSpec      *pspec)
{
  HildonHomeArea       *area = HILDON_HOME_AREA (container);
  HildonHomeAreaPriv   *priv = HILDON_HOME_AREA_GET_PRIVATE (area);
  ChildData            *child_data;

  switch (property_id)
    {
      case PROP_CHILD_DATA:
          child_data = hildon_home_area_get_child_data (area, child);
          if (child_data)
            {
              priv->children_data = 
                  g_list_remove (priv->children_data, child_data);
              g_free (child_data);
            }
          priv->children_data = 
              g_list_append (priv->children_data, g_value_get_pointer (value));
          break;
      default:
          GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container,
                                                        property_id,
                                                        pspec);
          break;
    }

}

static void
hildon_home_area_get_child_property (GtkContainer    *container,
                                     GtkWidget       *child,
                                     guint            property_id,
                                     GValue          *value,
                                     GParamSpec      *pspec)
{
  HildonHomeArea       *area = HILDON_HOME_AREA (container);
  ChildData            *child_data;

  switch (property_id)
    {
      case PROP_CHILD_DATA:
          child_data = hildon_home_area_get_child_data (area, child);
          g_value_set_pointer (value, child_data);
          break;
      default:
          GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container,
                                                        property_id,
                                                        pspec);
          break;
    }
}

static void
hildon_home_area_realize (GtkWidget *widget)
{
  HildonHomeAreaPriv           *priv;
  XRenderPictureAttributes      pa;
  XRenderPictFormat            *format;

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  XCompositeRedirectSubwindows (GDK_DISPLAY (),
                                GDK_WINDOW_XID (widget->window),
                                CompositeRedirectManual);

  priv = HILDON_HOME_AREA_GET_PRIVATE (widget);

  format = XRenderFindVisualFormat (GDK_DISPLAY(),
                                    GDK_VISUAL_XVISUAL (
                                     gdk_drawable_get_visual (widget->window)));

  pa.subwindow_mode = IncludeInferiors;
  priv->picture = XRenderCreatePicture (GDK_DISPLAY (),
                                        GDK_WINDOW_XID (widget->window),
                                        format,
                                        CPSubwindowMode,
                                        &pa);
}

static void
hildon_home_area_unrealize (GtkWidget *widget)
{
  HildonHomeAreaPriv           *priv;

  priv = HILDON_HOME_AREA_GET_PRIVATE (widget);

  XCompositeUnredirectSubwindows (GDK_DISPLAY (),
                                  GDK_WINDOW_XID (widget->window),
                                  CompositeRedirectManual);

  if (priv->picture != None)
    XRenderFreePicture (GDK_DISPLAY (), priv->picture);

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}
#endif

static void
hildon_home_area_layout_mode_start (HildonHomeArea *area)
{
  HildonHomeAreaPriv      *priv;
  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  gtk_container_foreach (GTK_CONTAINER (area),
                         (GtkCallback)hildon_desktop_home_item_set_layout_mode,
                         (gpointer)TRUE);

  g_signal_emit_by_name (area, "layout-mode-started");
}

static void
hildon_home_area_layout_mode_end (HildonHomeArea *area)
{
  HildonHomeAreaPriv      *priv;
  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  gtk_container_foreach (GTK_CONTAINER (area),
                        (GtkCallback)hildon_desktop_home_item_set_layout_mode,
                         (gpointer)FALSE);
  g_signal_emit_by_name (area, "layout-mode-ended");

  priv->layout_changed = FALSE;

}

static void
hildon_home_area_layout_changed (HildonHomeArea *area)
{
  HildonHomeAreaPriv      *priv;
  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  priv->layout_changed = TRUE;
}


static void
hildon_home_area_applet_added (HildonHomeArea *area, GtkWidget *applet)
{
  HildonHomeAreaPriv      *priv;
  g_return_if_fail (area);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);
  if (priv->layout_mode)
    {
      if (HILDON_DESKTOP_IS_HOME_ITEM (applet))
        hildon_desktop_home_item_set_layout_mode (HILDON_DESKTOP_HOME_ITEM (applet),
                                                  TRUE);
      g_signal_emit_by_name (area, "layout-changed");
    }
}

static void
hildon_home_area_add (GtkContainer *area, GtkWidget *applet)
{
  HildonHomeAreaPriv   *priv;
#ifdef HAVE_X_COMPOSITE
  ChildData            *child_data;
#endif
  g_return_if_fail (area);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  g_debug ("Adding Hildon Home applet %s",
           hildon_desktop_item_get_id (HILDON_DESKTOP_ITEM (applet)));

  if (priv->batch_add)
    {
      priv->to_add = g_list_append (priv->to_add, applet);
      return;
    }

  if (priv->layout && HILDON_DESKTOP_IS_HOME_ITEM (applet))
    {
      GdkRectangle *rect;
      const gchar *name = hildon_desktop_item_get_id (
                                     HILDON_DESKTOP_ITEM (applet));

      rect = g_hash_table_lookup (priv->layout, name);

      if (rect)
        {
          gtk_fixed_put (GTK_FIXED (area), applet, rect->x, rect->y);
          if (rect->width > 0 && rect->height > 0)
            gtk_widget_set_size_request (applet, rect->width, rect->height);
        }

      else
        {
          gtk_fixed_put (GTK_FIXED (area), applet, 0, 0);
        }

      g_signal_emit_by_name (area, "layout-changed");

      if (priv->layout_mode)
        hildon_desktop_home_item_set_layout_mode (HILDON_DESKTOP_HOME_ITEM (applet),
                                            TRUE);
    }

#ifdef HAVE_X_COMPOSITE
  if (HILDON_HOME_AREA_GET_CLASS (area)->composite)
    {


      child_data = g_new0 (ChildData, 1);
      child_data->widget = applet;

      gtk_container_child_set (area, applet,
                               "child-data", child_data,
                               NULL);
      child_data->realize_handler =
          g_signal_connect_after (applet, "realize",
                                  G_CALLBACK (hildon_home_area_child_realize),
                                  area);
      child_data->style_set_handler =
          g_signal_connect_after (applet, "style-set",
                                  G_CALLBACK (hildon_home_area_child_style_set),
                                  area);
      child_data->size_allocate_handler =
          g_signal_connect (applet, "size-allocate",
                            G_CALLBACK (hildon_home_area_child_size_allocate),
                            area);

      if (GTK_WIDGET_REALIZED (applet))
        {
          hildon_home_area_create_child_data (HILDON_HOME_AREA (area), applet);
          hildon_home_area_child_style_set (applet,
                                            applet->style,
                                            HILDON_HOME_AREA (area));

          /* HACK: pretend the child is shaped to avoid GDK removing it
           * from the parent's clip (see #412882) */
          ((GdkWindowObject *)applet->window)->shaped = TRUE;
        }
    }
#endif

}

#ifdef HAVE_X_COMPOSITE
static void
hildon_home_area_child_compose (GtkWidget *child,
                                HildonHomeArea *area,
                                gboolean alpha)
{
  HildonHomeAreaPriv   *priv;
  ChildData            *child_data;
  GtkAllocation        *alloc;

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &child_data,
                           NULL);

  g_return_if_fail (child_data);

  if (child_data->picture == None)
    return;

  alloc = &(child->allocation);

  XRenderComposite (GDK_DISPLAY (),
                    (alpha)?PictOpOver:PictOpSrc,
                    child_data->picture,
                    (alpha)?child_data->alpha_mask:None,
                    priv->picture,
                    0,
                    0,
                    0,
                    0,
                    alloc->x - priv->x_offset,
                    alloc->y - priv->y_offset,
                    alloc->width,
                    alloc->height);

}

static void
hildon_home_area_child_style_set (GtkWidget            *child,
                                  GtkStyle             *old_style,
                                  HildonHomeArea       *area)
{
  ChildData    *child_data;

  if (!HILDON_DESKTOP_IS_HOME_ITEM (child))
    return;

  if (!old_style)
    return;

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &child_data,
                           NULL);

  hildon_home_area_child_build_alpha_mask_unscaled (area, child);
  hildon_home_area_child_build_alpha_mask (area, child);

}

static void
hildon_home_area_child_size_allocate (GtkWidget        *child,
                                      GtkAllocation    *allocation,
                                      HildonHomeArea   *area)
{
  ChildData *data;

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &data,
                           NULL);

  if (data->old_allocation.width  != allocation->width ||
      data->old_allocation.height != allocation->height)
    hildon_home_area_child_build_alpha_mask (area, child);

  data->old_allocation = *allocation;


}

static void
hildon_home_area_child_build_alpha_mask_unscaled (HildonHomeArea       *area,
                                                  GtkWidget            *child)
{
  HildonHomeAreaPriv           *priv;
  GdkPixbuf                    *pixbuf = NULL;
  GC                            gc;
  XGCValues                     gc_values;
  XRenderPictureAttributes      pa;
  XRenderPictFormat            *format;
  XImage                       *image;
  Pixmap                        pixmap;
  ChildData                    *child_data;
  GError                       *error = NULL;
  const gchar                  *mask_file_name = NULL;

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &child_data,
                           NULL);

  if (child_data->alpha_mask_unscaled != None)
    {
      XRenderFreePicture  (GDK_DISPLAY (), child_data->alpha_mask_unscaled);
      child_data->alpha_mask_unscaled = None;
    }

  if (!child->style->rc_style)
    return;

  mask_file_name = child->style->rc_style->bg_pixmap_name[GTK_STATE_PRELIGHT];

  if (mask_file_name)
    {
      pixbuf = gdk_pixbuf_new_from_file (mask_file_name,
                                         &error);
    }

  if (error)
    {
      g_warning ("Could not open alpha mask from style: %s", error->message);
      g_error_free (error);

      pixbuf = NULL;
    }

  if (pixbuf)
    {
      gchar                        *data;
      guchar                       *pixbuf_data;
      guint                         i;
      gint                          pw, ph;

      child_data->background_width = pw = gdk_pixbuf_get_width (pixbuf);
      child_data->background_height = ph = gdk_pixbuf_get_height (pixbuf);

      pixmap = XCreatePixmap (GDK_DISPLAY (),
                              GDK_WINDOW_XID (GTK_WIDGET (area)->window),
                              pw,
                              ph,
                              8);

      /* Use malloc here because it is freed by Xlib */
      data = (gchar *) malloc (pw*ph);

      image = XCreateImage (GDK_DISPLAY (),
                            None,
                            8,         /* depth */
                            ZPixmap,
                            0,         /* offset */
                            data,
                            pw,
                            ph,
                            8,
                            pw);

      pixbuf_data = gdk_pixbuf_get_pixels (pixbuf);

      for (i = 0; i < pw * ph; i++)
        {
          data[i] = pixbuf_data[4*i + 3];
        }

      gc = XCreateGC (GDK_DISPLAY (),
                      pixmap,
                      0,
                      &gc_values);

      XPutImage (GDK_DISPLAY (),
                 pixmap,
                 gc,
                 image,
                 0, 0,
                 0, 0,
                 pw, ph);

      XFreeGC (GDK_DISPLAY (), gc);
      XDestroyImage (image);
      g_object_unref (pixbuf);
    }
  else
    {
      pixmap = XCreatePixmap (GDK_DISPLAY (),
                              GDK_WINDOW_XID (GTK_WIDGET (area)->window),
                              1,
                              1,
                              8);
    }

  if (pixmap == None)
    {
      g_warning ("Could not create pixmap for alpha_channel");
      return;
    }

  format = XRenderFindStandardFormat (GDK_DISPLAY (),
                                      PictStandardA8);

  pa.repeat = True;
  child_data->alpha_mask_unscaled = XRenderCreatePicture (GDK_DISPLAY (),
                                                          pixmap,
                                                          format,
                                                          CPRepeat,
                                                          &pa);

  if (!pixbuf)
    {
      XRenderColor c = {0};

/*      c.alpha = priv->default_alpha * 0xffff;*/
      c.alpha = 0xFFFF;

      XRenderFillRectangle (GDK_DISPLAY (),
                            PictOpSrc,
                            child_data->alpha_mask_unscaled,
                            &c,
                            0, 0, 1, 1);

      child_data->alpha_mask_unscaled = None;
    }

  XFreePixmap (GDK_DISPLAY (), pixmap);

}

static void
hildon_home_area_child_build_alpha_mask (HildonHomeArea *area,
                                         GtkWidget *child)
{
  ChildData    *child_data;
  GtkBorder    *borders = NULL;


  if (!GTK_WIDGET_REALIZED (child))
    return;

  gtk_widget_style_get (child,
                        "background-borders", &borders,
                        NULL);

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &child_data,
                           NULL);

  if (child_data->alpha_mask_unscaled == None)
    hildon_home_area_child_build_alpha_mask_unscaled (area, child);

  if (child_data->alpha_mask_unscaled == None)
    return;

  if (child_data->alpha_mask != None)
    XRenderFreePicture (GDK_DISPLAY (),
                        child_data->alpha_mask);

  child_data->alpha_mask = None;

  if (borders && child_data->alpha_mask_unscaled)
    {
      Pixmap                    p;
      gint                      w, h, x, y;
      gint                      center_w, center_h, scenter_w, scenter_h;
      XRenderPictureAttributes  pa;
      XRenderPictFormat        *format;

      w = child->allocation.width;
      h = child->allocation.height;

      if (w < 0 || h < 0)
        return;

      center_w = w - borders->left - borders->right;
      center_h = h - borders->top  - borders->bottom;

      scenter_w = child_data->background_width - borders->left - borders->right;
      scenter_h = child_data->background_height - borders->top - borders->bottom;

      if (scenter_h < 0 || scenter_w < 0)
        {
          g_warning ("Invalid border values, alpha mask will not be applied");
          return;
        }

      p = XCreatePixmap (GDK_DISPLAY (),
                         GDK_WINDOW_XID (child->window),
                         child->allocation.width,
                         child->allocation.height,
                         8);

      format = XRenderFindStandardFormat (GDK_DISPLAY (),
                                          PictStandardA8);

      pa.repeat = True;
      child_data->alpha_mask = XRenderCreatePicture (GDK_DISPLAY (),
                                                     p,
                                                     format,
                                                     CPRepeat,
                                                     &pa);

      XRenderComposite (GDK_DISPLAY (),
                        PictOpSrc,
                        child_data->alpha_mask_unscaled,
                        None,
                        child_data->alpha_mask,
                        0, 0,
                        0, 0,
                        0, 0,
                        borders->left, borders->top);

      x = borders->left;
      while (x < borders->left + center_w)
        {
          XRenderComposite (GDK_DISPLAY (),
                            PictOpSrc,
                            child_data->alpha_mask_unscaled,
                            None,
                            child_data->alpha_mask,
                            borders->left, 0,
                            0, 0,
                            x, 0,
                            scenter_w, borders->top);

          x += scenter_w;
        }

      XRenderComposite (GDK_DISPLAY (),
                        PictOpSrc,
                        child_data->alpha_mask_unscaled,
                        None,
                        child_data->alpha_mask,
                        borders->left + scenter_w, 0,
                        0, 0,
                        borders->left + center_w, 0,
                        borders->right, borders->top);

      y = borders->top;
      while (y < borders->top + center_h)
        {
          XRenderComposite (GDK_DISPLAY (),
                            PictOpSrc,
                            child_data->alpha_mask_unscaled,
                            None,
                            child_data->alpha_mask,
                            0, borders->top,
                            0, 0,
                            0, y,
                            borders->left, scenter_h);
          y += scenter_h;
        }

      y = borders->top;
      while (y < borders->top + center_h)
        {
          x = borders->left;
          while (x < borders->left + center_w)
            {
              XRenderComposite (GDK_DISPLAY (),
                                PictOpSrc,
                                child_data->alpha_mask_unscaled,
                                None,
                                child_data->alpha_mask,
                                borders->left, borders->top,
                                0, 0,
                                x, y,
                                scenter_w, scenter_h);
              x += scenter_w;
            }
          y += scenter_h;
        }

      y = borders->top;
      while (y < borders->top + center_h)
        {
          XRenderComposite (GDK_DISPLAY (),
                            PictOpSrc,
                            child_data->alpha_mask_unscaled,
                            None,
                            child_data->alpha_mask,
                            borders->left + scenter_w, borders->top,
                            0, 0,
                            borders->left + center_w, y,
                            borders->right, scenter_h);
          y += scenter_h;
        }

      XRenderComposite (GDK_DISPLAY (),
                        PictOpSrc,
                        child_data->alpha_mask_unscaled,
                        None,
                        child_data->alpha_mask,
                        0, borders->top + scenter_h,
                        0, 0,
                        0, borders->top + center_h,
                        borders->left, borders->bottom);

      x = borders->left;
      while (x < borders->left + center_w)
        {
          XRenderComposite (GDK_DISPLAY (),
                            PictOpSrc,
                            child_data->alpha_mask_unscaled,
                            None,
                            child_data->alpha_mask,
                            borders->left, borders->top + scenter_h,
                            0, 0,
                            x, borders->top + center_h,
                            scenter_w, borders->bottom);
          x += scenter_w;
        }

      XRenderComposite (GDK_DISPLAY (),
                        PictOpSrc,
                        child_data->alpha_mask_unscaled,
                        None,
                        child_data->alpha_mask,
                        borders->left + scenter_w, borders->top + scenter_h,
                        0, 0,
                        borders->left + center_w, borders->top + center_h,
                        borders->right, borders->bottom);

      XFreePixmap (GDK_DISPLAY (), p);
      g_free (borders);
    }
  else
    child_data->alpha_mask = child_data->alpha_mask_unscaled;

}

static gint
find_by_window (GtkWidget *widget, Window *w)
{
  return !(widget->window && 
           GDK_WINDOW_XID (widget->window) == *w);
}

static void
child_is_dragged (GtkWidget *child, gboolean *is_dragged)
{
  gint  state;

  g_object_get (child,
                "state", &state,
                NULL);
  if (!*is_dragged && state)
    *is_dragged = TRUE;
}

static gboolean
hildon_home_area_expose (GtkWidget *widget,
                         GdkEventExpose *event)
{
  HildonHomeAreaPriv *priv;

  priv = HILDON_HOME_AREA_GET_PRIVATE (widget);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GList                    *children;
      XRectangle                rectangle;
      XserverRegion             region;
      GdkDrawable              *drawable;
      XRenderPictFormat        *format;
      XRenderPictureAttributes  pa;
      Window                   *wchildren, root, parent;
      int                       n_children, i;
      gboolean                  is_dragged = FALSE;

      gdk_window_get_internal_paint_info (widget->window,
                                          &drawable,
                                          &priv->x_offset,
                                          &priv->y_offset);

      rectangle.x = event->area.x - priv->x_offset;
      rectangle.y = event->area.y - priv->y_offset;
      rectangle.width = event->area.width;
      rectangle.height = event->area.height;

      format = XRenderFindVisualFormat (GDK_DISPLAY(),
                                        GDK_VISUAL_XVISUAL (gdk_drawable_get_visual (drawable)));

      pa.subwindow_mode = IncludeInferiors;

      priv->picture = XRenderCreatePicture (GDK_DISPLAY (),
                                            GDK_DRAWABLE_XID (drawable),
                                            format,
                                            CPSubwindowMode,
                                            &pa);

      region = XFixesCreateRegion (GDK_DISPLAY (),
                                   &rectangle,
                                   1);

      XFixesSetPictureClipRegion (GDK_DISPLAY (),
                                  priv->picture,
                                  0,
                                  0,
                                  region);

      gdk_error_trap_push ();
      XQueryTree (GDK_DISPLAY (),
                  GDK_WINDOW_XID (widget->window),
                  &root,
                  &parent,
                  &wchildren,
                  &n_children);
      if (gdk_error_trap_pop ())
        {
          if (wchildren)
            XFree (wchildren);

          XRenderFreePicture (GDK_DISPLAY (),
                              priv->picture);
          XFixesDestroyRegion (GDK_DISPLAY (),
                               region);
          return FALSE;
        }

      children = gtk_container_get_children (GTK_CONTAINER (widget));
      g_list_foreach (children, (GFunc)child_is_dragged, &is_dragged);

      for (i = 0; i < n_children; i++)
        {
          GList        *l = NULL;

          l = g_list_find_custom (children,
                                  &wchildren[i],
                                  (GCompareFunc)find_by_window);

          if (l)
            hildon_home_area_child_compose (GTK_WIDGET (l->data),
                                            HILDON_HOME_AREA (widget),
                                            !is_dragged);
        }

      XFree (wchildren);

      XFixesDestroyRegion (GDK_DISPLAY (), region);
      XRenderFreePicture (GDK_DISPLAY (), priv->picture);

      g_list_free (children);
    }

  return TRUE;
}

static GdkFilterReturn
hildon_home_area_child_window_filter (GdkXEvent        *xevent,
                                      GdkEvent         *event,
                                      GtkWidget        *child)
{
  XAnyEvent            *aevent = xevent;
  HildonHomeArea       *area = HILDON_HOME_AREA (child->parent);
  HildonHomeAreaClass  *klass = HILDON_HOME_AREA_GET_CLASS (area);


  if (aevent->type == klass->xdamage_event_base + XDamageNotify)
    {
      XDamageNotifyEvent *ev = xevent;
      GdkRectangle rect;
      XserverRegion parts;

      rect.x = ev->area.x + child->allocation.x;
      rect.y = ev->area.y + child->allocation.y;
      rect.width = ev->area.width;
      rect.height = ev->area.height;

      parts = XFixesCreateRegion (GDK_DISPLAY (), 0, 0);
      XDamageSubtract (GDK_DISPLAY (), ev->damage, None, parts);
      XFixesDestroyRegion (GDK_DISPLAY (), parts);

      gdk_window_invalidate_rect (child->parent->window, &rect, FALSE);
    }

  return GDK_FILTER_CONTINUE;
}


static void
hildon_home_area_create_child_data (HildonHomeArea     *area,
                                    GtkWidget          *child)
{
  ChildData            *child_data;
  XWindowAttributes     attr;

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &child_data,
                           NULL);

  g_return_if_fail (child_data);

  child_data->window = GDK_WINDOW_XID (child->window);

  XGetWindowAttributes (GDK_DISPLAY(), child_data->window, &attr);

  if (attr.class == InputOutput)
    {
      XRenderPictFormat            *format;
      XRenderPictureAttributes      pa;

      format = XRenderFindVisualFormat (GDK_DISPLAY(), attr.visual);

      pa.subwindow_mode = IncludeInferiors;

      if (child_data->picture)
        XRenderFreePicture (GDK_DISPLAY (),
                            child_data->picture);

      child_data->picture = XRenderCreatePicture (GDK_DISPLAY (),
                                                  child_data->window,
                                                  format,
                                                  CPSubwindowMode,
                                                  &pa);

      child_data->damage = XDamageCreate (GDK_DISPLAY (),
                                          child_data->window,
                                          XDamageReportNonEmpty);

      gdk_window_add_filter (child_data->widget->window,
                             (GdkFilterFunc)
                                 hildon_home_area_child_window_filter,
                             child_data->widget);
    }
}

static void
hildon_home_area_child_realize (GtkWidget             *child,
                                HildonHomeArea        *area)
{
  ChildData    *data;

  gtk_container_child_get (GTK_CONTAINER (area), child,
                           "child-data", &data,
                           NULL);

  /* HACK: Pretend the child shaped so GDK does not add the
   * child to the parent's clip list (see #412882) */
  ((GdkWindowObject *)child->window)->shaped = TRUE;

  if (data->picture == None)
    hildon_home_area_create_child_data (HILDON_HOME_AREA (area), child);

  if (data->alpha_mask == None)
    hildon_home_area_child_build_alpha_mask (area, child);
}
#endif

static GdkRectangle *
create_rectangle (gint x, gint y, gint w, gint h)
{
  GdkRectangle *r;

  r = g_new (GdkRectangle, 1);
  r->x = x;
  r->y = y;
  r->width = w;
  r->height = h;

  return r;
}

static void
substract_rectangle (GdkRectangle     *original,
                     GdkRectangle     *rectangle,
                     GList           **result)
{
  *result = NULL;

  /* top */
  if (rectangle->y > original->y)
    *result = g_list_append (*result,
                             create_rectangle (original->x,
                                               original->y,
                                               original->width,
                                               rectangle->y - original->y));

  /* left */
  if (rectangle->x > original->x)
    *result = g_list_append (*result,
                             create_rectangle (original->x,
                                               original->y,
                                               rectangle->x - original->x,
                                               original->height));

  /* right */
  if (rectangle->x + rectangle->width < original->x + original->width)
    *result = g_list_append (*result,
                             create_rectangle (rectangle->x + rectangle->width,
                                               original->y,
                                               original->x + original->width -
                                               rectangle->x - rectangle->width,
                                               original->height));

  /* bottom */
  if (rectangle->y + rectangle->height < original->y + original->height)
    *result = g_list_append (*result,
                             create_rectangle (original->x,
                                               rectangle->y + rectangle->height,
                                               original->width,
                                               original->y + original->height -
                                               rectangle->y - rectangle->height));

}

static void
substract_rectangle_from_region (GList        *region,
                                 GdkRectangle  *rectangle)
{
  GList *i = region;

  while (i)
    {
      GdkRectangle     *r = (GdkRectangle *)i->data;
      GdkRectangle      tmp;

      if (gdk_rectangle_intersect (r, rectangle, &tmp))
        {
          GList *pieces = NULL;

          substract_rectangle (r, rectangle, &pieces);

          if (pieces)
            {
              GList *last_piece = g_list_last (pieces);

              /* Insert the new list of rectangles as a replacement of the
               * original */

              last_piece->next = i->next;
              (pieces)->prev = i->prev;

              if (i->prev)
                {
                  i->prev->next = pieces;
                  if (i->next)
                    i->next->prev = last_piece;

                  g_free (i->data);
                  g_list_free_1 (i);
                  i = last_piece->next;

                }
              else
                {
                  /* In this case, we keep the first element of region
                   * and get rid of the first element of pieces */

                  if (i->next)
                    {
                      i->next->prev = (pieces->next)?last_piece:region;
                    }

                  region->data = pieces->data;
                  if (pieces->next)
                    {
                      region->next = pieces->next;
                      pieces->next->prev = region;
                    }

                  i = last_piece->next;

                  g_list_free_1 (pieces);
                }

            }
          else
            i = i->next;
        }
      else
        i = i->next;
    }
}

static void
remove_widget (GtkWidget *widget, GList *region)
{
  GdkRectangle r = {0};
  gint x, y;
  gint padding;
  const gchar *name;

  gtk_container_child_get (GTK_CONTAINER (widget->parent), widget,
                           "x", &x,
                           "y", &y,
                           NULL);
  g_object_get (widget, "id", &name, NULL);

  g_object_get (widget->parent,
                "applet-padding", &padding,
                NULL);

  r.x = x?x-padding:0;
  r.y = y?y-padding:0;

  r.width = widget->allocation.width;
  if (x + widget->allocation.width != widget->parent->allocation.width)
    r.width += padding;
  if (x)
    r.width += padding;

  r.height = widget->allocation.height;
  if (y + widget->allocation.height != widget->parent->allocation.height)
    r.height += padding;
  if (y)
    r.height += padding;

  substract_rectangle_from_region (region, &r);
}

/* Sorts top - left first */
static gint
sort_rectangles (GdkRectangle *r1, GdkRectangle *r2)
{
  return (r1->y - r2->y)?
      r1->y - r2->y:
      r1->x - r2->x;

}

static void
hildon_home_area_batch_add (HildonHomeArea *area)
{
  HildonHomeAreaPriv      *priv;
  GdkRectangle         *area_rectangle;
  GList                *region = NULL, *i;

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  area_rectangle = create_rectangle (0,
                                     0,
                                     GTK_WIDGET (area)->allocation.width,
                                      GTK_WIDGET (area)->allocation.height);

  region = g_list_append (region, area_rectangle);

  gtk_container_foreach (GTK_CONTAINER (area),
                         (GtkCallback)remove_widget,
                         region);

  region = g_list_sort (region, (GCompareFunc)sort_rectangles);
  i = priv->to_add;

  while (i)
    {
      GtkRequisition    req = {0};
      GList            *i_rect;
      GtkWidget        *w;
      const gchar      *name;

      w = GTK_WIDGET (i->data);
      name = hildon_desktop_item_get_id (HILDON_DESKTOP_ITEM (w));

      gtk_widget_size_request (w, &req);

      i_rect = region;

      while (i_rect)
        {
          GdkRectangle *r = (GdkRectangle *)i_rect->data;

          if (r->width  >= req.width &&
              r->height >= req.height)
            {
              GdkRectangle     *layout = create_rectangle (r->x,
                                                           r->y,
                                                           req.width,
                                                           req.height);
              GdkRectangle     *padded_layout = g_new (GdkRectangle, 1);

              g_hash_table_insert (priv->layout, g_strdup (name), layout);
              gtk_container_add (GTK_CONTAINER (area), w);

              *padded_layout = *layout;

              if (padded_layout->x + padded_layout->width <
                  area_rectangle->width)
                padded_layout->width += priv->applet_padding;

              if (padded_layout->y + padded_layout->height <
                  area_rectangle->height)
                padded_layout->height += priv->applet_padding;

              if (padded_layout->x)
                {
                  padded_layout->x -= priv->applet_padding;
                  padded_layout->width += priv->applet_padding;
                }

              if (padded_layout->y)
                {
                  padded_layout->y-= priv->applet_padding;
                  padded_layout->height += priv->applet_padding;
                }

              substract_rectangle_from_region (region, padded_layout);
              region = g_list_sort (region, (GCompareFunc)sort_rectangles);

              break;
            }

          i_rect = g_list_next (i_rect);

        }

      if (!i_rect)
        {
          g_debug ("Adding layer");
          /* Not enough place in this layer, we need to add one */
          g_list_foreach (region, (GFunc)g_free, NULL);
          g_list_free (region);
          region = NULL;
          region = g_list_append (region, area_rectangle);
        }
      else
        i = g_list_next (i);

    }

  g_list_foreach (region, (GFunc)g_free, NULL);
  g_list_free (region);

  priv->to_add = NULL;

}

static void
hildon_home_area_remove (GtkContainer *area, GtkWidget *applet)
{
  HildonHomeAreaPriv   *priv;
#ifdef HAVE_X_COMPOSITE
  ChildData            *child_data;
#endif
  g_return_if_fail (area);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);
  if (priv->layout_mode)
    {
      g_signal_emit_by_name (area, "layout-changed");
    }

#ifdef HAVE_X_COMPOSITE
  if (HILDON_HOME_AREA_GET_CLASS (area)->composite)
    {

      gtk_container_child_get (area, applet,
                               "child-data", &child_data,
                               NULL);

      if (child_data)
        {

          g_signal_handler_disconnect (applet,
                                       child_data->realize_handler);
          g_signal_handler_disconnect (applet,
                                       child_data->style_set_handler);
          g_signal_handler_disconnect (applet,
                                       child_data->size_allocate_handler);

          priv->children_data = g_list_remove (priv->children_data, child_data);

          child_data_free (child_data);
        }
    }
#endif

  if (GTK_CONTAINER_CLASS (parent_class)->remove)
    GTK_CONTAINER_CLASS (parent_class)->remove (area, applet);
}

/* Public functions */

GtkWidget *
hildon_home_area_new ()
{
  return g_object_new (HILDON_TYPE_HOME_AREA, NULL);
}

void
hildon_home_area_set_layout_mode (HildonHomeArea *area, gboolean layout_mode)
{
  HildonHomeAreaPriv      *priv;
  g_return_if_fail (area);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  if (priv->layout_mode != layout_mode)
    {
      g_object_notify (G_OBJECT (area), "layout-mode");
      priv->layout_mode = layout_mode;

      if (priv->layout_mode)
        g_signal_emit_by_name (G_OBJECT (area), "layout-mode-start");
      else
        g_signal_emit_by_name (G_OBJECT (area), "layout-mode-end");

    }
}

gboolean
hildon_home_area_get_layout_mode (HildonHomeArea *area)
{
  HildonHomeAreaPriv      *priv;

  g_return_val_if_fail (area, FALSE);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  return priv->layout_mode;
}

static void
hildon_home_area_child_save_position (GtkWidget *widget, GKeyFile *keyfile)
{
  const gchar  *id;
  gint          x, y;

  if (!HILDON_DESKTOP_IS_HOME_ITEM (widget))
    return;

  id = hildon_desktop_item_get_id (HILDON_DESKTOP_ITEM (widget));
  g_return_if_fail (id);

  gtk_container_child_get (GTK_CONTAINER (widget->parent), widget,
                           "x", &x,
                           "y", &y,
                           NULL);

  g_key_file_set_integer (keyfile,
                          id,
                          HH_APPLET_KEY_X,
                          x);

  g_key_file_set_integer (keyfile,
                          id,
                          HH_APPLET_KEY_Y,
                          y);

  g_key_file_set_integer (keyfile,
                          id,
                          HH_APPLET_KEY_WIDTH,
                          widget->allocation.width);

  g_key_file_set_integer (keyfile,
                          id,
                          HH_APPLET_KEY_HEIGHT,
                          widget->allocation.height);

}

gint
hildon_home_area_save_configuration (HildonHomeArea *area,
                                     const gchar *path,
                                     GError **error)
{
  GKeyFile *keyfile;
  FILE     *file;
  gchar    *buffer = NULL;
  guint     buffer_size;
  GError   *local_error = NULL;
  gint      ret;

  keyfile = g_key_file_new ();

  gtk_container_foreach (GTK_CONTAINER (area),
                         (GtkCallback)hildon_home_area_child_save_position,
                         keyfile);

  file = fopen (path, "w");

  if (!file)
    {
      g_warning ("Could not open %s for saving the applet configuration", path);
      g_key_file_free (keyfile);
      g_set_error (error,
                   G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   strerror (errno));
      return -1;
    }

  buffer = g_key_file_to_data (keyfile, &buffer_size, &local_error);

  if (local_error)
    {
      g_warning ("Could not create buffer to save applet configuration: %s",
                 local_error->message);
      g_propagate_error (error, local_error);
      if (file)
        fclose (file);
      return 1;
    }

  g_key_file_free (keyfile);

  if (buffer_size == 0)
    {
      const char *empty_file = "# No applet";
      int len =  strlen (empty_file);
      ret = fwrite (empty_file, len, 1, file);
    }

  else
    {
      ret = fwrite (buffer, buffer_size, 1, file);

      g_free (buffer);
    }

  if (file)
    fclose (file);

  if (ret != 1)
    return -1;

  return 0;
}

void
hildon_home_area_load_configuration (HildonHomeArea *area,
                                     const gchar *path,
                                     GError **error)
{
  HildonHomeAreaPriv      *priv;
  GKeyFile *keyfile;
  GError *local_error = NULL;
  gchar **groups = NULL;
  guint n_groups;
  GList *applets = NULL;

  g_return_if_fail (area);
  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  applets = gtk_container_get_children (GTK_CONTAINER (area));

  keyfile = g_key_file_new ();

  g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, &local_error);

  if (local_error) goto cleanup;

  if (priv->layout)
    g_hash_table_destroy (priv->layout);

  priv->layout = g_hash_table_new_full (g_str_hash,
                                        g_str_equal,
                                        (GDestroyNotify)g_free,
                                        (GDestroyNotify)g_free);

  groups = g_key_file_get_groups (keyfile, &n_groups);

  while (n_groups > 0)
    {
      GtkWidget     *applet = NULL;
      gint           x,y,width,height;
      GList         *list_element;

      x = g_key_file_get_integer (keyfile,
                                  groups[n_groups-1],
                                  HH_APPLET_KEY_X,
                                  &local_error);
      y = g_key_file_get_integer (keyfile,
                                  groups[n_groups-1],
                                  HH_APPLET_KEY_Y,
                                  &local_error);

      width = g_key_file_get_integer (keyfile,
                                      groups[n_groups-1],
                                      HH_APPLET_KEY_WIDTH,
                                      &local_error);

      height = g_key_file_get_integer (keyfile,
                                       groups[n_groups-1],
                                       HH_APPLET_KEY_HEIGHT,
                                       &local_error);
      if (local_error) goto cleanup;

      else
        {
          GdkRectangle *rect = g_new (GdkRectangle, 1);

          rect->x = x;
          rect->y = y;
          rect->width = width;
          rect->height = height;

          g_hash_table_insert (priv->layout,
                               g_strdup (groups[n_groups-1]),
                               rect);
        }

      /* Do we already have this applet loaded? */

      list_element = g_list_find_custom (applets,
                                         groups[n_groups-1],
                                         (GCompareFunc)
                                         hildon_desktop_item_find_by_id);

      if (list_element)
        {
          applet = GTK_WIDGET (list_element->data);
          gtk_fixed_move (GTK_FIXED (area),
                          applet,
                          x,
                          y);

          applets = g_list_remove (applets, applet);
        }


      if (applet)
        {
          gtk_widget_set_size_request (applet, width, height);
        }
      n_groups --;
    }

  /* Remove all the applets left in the list, they are no longer  */
  g_list_foreach (applets, (GFunc)gtk_widget_destroy, NULL);

cleanup:
  g_list_free (applets);
  if (groups)
    g_strfreev (groups);

  if (local_error)
    {
      g_warning ("An error occurred when reading HildonHome applets' "
                 "configuration: %s",
               local_error->message);
      g_propagate_error (error, local_error);
    }

  g_key_file_free (keyfile);
}

gboolean
hildon_home_area_get_layout_changed (HildonHomeArea *area)
{
  HildonHomeAreaPriv      *priv;
  g_return_val_if_fail (area, FALSE);
  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  return priv->layout_changed;
}

gboolean
hildon_home_area_get_overlaps (HildonHomeArea *area)
{
  gboolean overlap;
  GList *applets, *l;

  g_return_val_if_fail (area, FALSE);

  overlap = FALSE;

  applets = gtk_container_get_children (GTK_CONTAINER (area));

  for (l = applets; l && !overlap; l = g_list_next (l))
    {
      if (!HILDON_DESKTOP_IS_HOME_ITEM (l->data))
        continue;

      overlap = hildon_desktop_home_item_get_overlaps
          (HILDON_DESKTOP_HOME_ITEM (l->data));
    }

  g_list_free (applets);

  return overlap;
}

void
hildon_home_area_set_batch_add (HildonHomeArea *area, gboolean batch_add)
{
  HildonHomeAreaPriv      *priv;
  g_return_if_fail (area);
  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  if (priv->batch_add && !batch_add)
    {
      priv->batch_add = FALSE;
      hildon_home_area_batch_add (area);
    }

  priv->batch_add = batch_add;
}
