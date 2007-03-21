/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
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


#include "hildon-home-area.h"

#include <libhildondesktop/hildon-desktop-home-item.h>

#include <string.h> /* strlen */
#include <errno.h>

#define APPLET_ADD_X_STEP       20
#define APPLET_ADD_Y_STEP       20


enum
{
  HILDON_HOME_AREA_PROPERTY_LAYOUT_MODE = 1,
  HILDON_HOME_AREA_PROPERTY_SNAP_TO_GRID
};

typedef struct HildonHomeAreaPriv_
{
  gboolean      layout_mode;
  gboolean      layout_changed;

  gboolean      snap_to_grid;

  GHashTable   *layout;

  GList        *to_add;
  gboolean      batch_add;
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

static void
hildon_home_area_finalize (GObject *object);


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
  GObjectClass *object_class;
  GtkContainerClass *container_class;
  GParamSpec   *pspec;

  object_class = G_OBJECT_CLASS (klass);
  container_class = GTK_CONTAINER_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private (klass, sizeof (HildonHomeAreaPriv));

  klass->layout_mode_start = hildon_home_area_layout_mode_start;
  klass->layout_mode_end   = hildon_home_area_layout_mode_end;
  klass->layout_changed    = hildon_home_area_layout_changed;
  klass->applet_added      = hildon_home_area_applet_added;

  container_class->add    = hildon_home_area_add;
  container_class->remove = hildon_home_area_remove;

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
                                   HILDON_HOME_AREA_PROPERTY_LAYOUT_MODE,
                                   pspec);
  
  pspec =  g_param_spec_boolean ("snap-to-grid",
                                 "Snap to grid",
                                 "Whether applets should snap to grid",
                                 TRUE,
                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   HILDON_HOME_AREA_PROPERTY_SNAP_TO_GRID,
                                   pspec);
                
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
      case HILDON_HOME_AREA_PROPERTY_LAYOUT_MODE:
          hildon_home_area_set_layout_mode (HILDON_HOME_AREA (object),
                                            g_value_get_boolean (value));
           break;

      case HILDON_HOME_AREA_PROPERTY_SNAP_TO_GRID:
           priv->snap_to_grid = g_value_get_boolean (value);
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
      case HILDON_HOME_AREA_PROPERTY_LAYOUT_MODE:
          g_value_set_boolean (value, priv->layout_mode);
          break;
      
      case HILDON_HOME_AREA_PROPERTY_SNAP_TO_GRID:
           g_value_set_boolean (value, priv->snap_to_grid);
           break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

static void
hildon_home_area_layout_mode_start (HildonHomeArea *area)
{
  HildonHomeAreaPriv      *priv;
  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  priv->layout_changed = FALSE;

  gtk_container_foreach (GTK_CONTAINER (area),
                         (GtkCallback)hildon_desktop_home_item_set_layout_mode,
                         (gpointer)TRUE);

  g_signal_emit_by_name (area, "layout-mode-started");
}

static void
hildon_home_area_layout_mode_end (HildonHomeArea *area)
{
  gtk_container_foreach (GTK_CONTAINER (area),
                        (GtkCallback)hildon_desktop_home_item_set_layout_mode,
                         (gpointer)FALSE);
  g_signal_emit_by_name (area, "layout-mode-ended");
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
  HildonHomeAreaPriv      *priv;
  g_return_if_fail (area);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);
      
  g_debug ("Adding Hildon Home applet %s",
           hildon_desktop_item_get_id (HILDON_DESKTOP_ITEM (applet)));

  if (priv->batch_add)
    {
      g_debug ("Batch adding it");
      priv->to_add = g_list_append (priv->to_add, applet);
      return;
    }

  if (priv->layout && HILDON_DESKTOP_IS_HOME_ITEM (applet))
    {
      g_debug ("Adding Hildon Home applet");
      GdkRectangle *rect;
      const gchar *name = hildon_desktop_item_get_id (
                                     HILDON_DESKTOP_ITEM (applet));
      
      g_debug ("Name: %s", name);

      rect = g_hash_table_lookup (priv->layout, name);

      if (rect)
        {
          g_debug ("Found layout configuration");
          gtk_fixed_put (GTK_FIXED (area), applet, rect->x, rect->y);
          if (rect->width > 0 && rect->height > 0)
            gtk_widget_set_size_request (applet, rect->width, rect->height);
        }

      else
        {
          g_debug ("No layout configuration");
          gtk_fixed_put (GTK_FIXED (area), applet, 0, 0);
        }

      if (priv->layout_mode)
        hildon_desktop_home_item_set_layout_mode (HILDON_DESKTOP_HOME_ITEM (applet),
                                            TRUE);
    }

}

#ifdef SIMPLE_PLACEMENT
static gint
sort_by_area (GtkWidget *a, GtkWidget *b)
{
  GtkRequisition reqa, reqb;

  gtk_widget_size_request (a, &reqa);
  gtk_widget_size_request (b, &reqb);

  if (reqa.width * reqa.height > reqb.width * reqb.height)
    return -1;
  else if (reqa.width * reqa.height < reqb.width * reqb.height)
    return 1;
  else return 0;

}

static void
hildon_home_area_place (HildonHomeArea *area, GList *applets)
{
  HildonHomeAreaPriv      *priv;

  guint columns_last_width[2] = {0, 0};
  guint columns_last_y[2] = {0, 0};
  GList *i_applet, *to_place;
  gint i_column = 0;
  gint width = GTK_WIDGET (area)->allocation.width;
  gint height = GTK_WIDGET (area)->allocation.height;
  gint last_stacked_x = 0, last_stacked_y = 0;
  
  g_return_if_fail (area);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  if (!applets)
    return;

  applets = g_list_sort (applets, (GCompareFunc)sort_by_area);

  to_place = g_list_copy (applets);

#define PADDING 5
#define other_c ((i_column + 1) % 2)

  for (i_applet = to_place; i_applet; i_applet = g_list_next (i_applet))
    {
      GtkWidget *applet = GTK_WIDGET (i_applet->data);
      GtkRequisition req;
      const gchar *name = 
          hildon_desktop_item_get_id (HILDON_DESKTOP_ITEM (applet));
      gint x = -1, y = -1;
      gint selected_column = -1;
      
      g_debug ("Looking at applet %s", name);

      gtk_widget_size_request (applet, &req);

#if 0
      g_debug ("column %i, last y %i %i, last width %i %i",
               i_column,
               columns_last_y[0], columns_last_y[1],
               columns_last_width[0], columns_last_width[1]);

      g_debug ("Got applet with size %ix%i", req.width, req.height);
      g_debug ("Area has size %ix%i", width, height);
#endif

      if (columns_last_y[i_column] + req.height + PADDING < height
          && req.width + columns_last_width[(1+i_column)%2] < width)
        {
          x = i_column? (width - req.width): 0;
          y = columns_last_y[i_column] + PADDING;

          selected_column = i_column;
        }
      else if (columns_last_y[other_c] + req.height + PADDING < height
           && req.width + columns_last_width[i_column] < width)
        {
          x = other_c? (width - req.width): 0;
          y = columns_last_y[other_c] + PADDING;

          selected_column = other_c;
        }

        if (selected_column < 0)
        {
          /* Revert to top-left stacking */
          x = last_stacked_x;
          y = last_stacked_y;

          if (x + req.width > width)
            x = 0;
          
          if (y + req.height > height)
            y = 0;

          last_stacked_x = x + APPLET_ADD_X_STEP;
          last_stacked_y = y + APPLET_ADD_Y_STEP;
        }
          
      g_debug ("Placing applet %s at %ix%i", name, x,y);

      if (applet->parent)
        gtk_fixed_move (GTK_FIXED (area), applet, x, y);
      else
        {
          GdkRectangle *layout = g_new (GdkRectangle, 1);
          layout->x = x;
          layout->y = y;
          layout->width = -1;
          layout->height = -1;

          g_hash_table_insert (priv->layout, g_strdup (name), layout);
          gtk_container_add (GTK_CONTAINER (area), applet);
        }
      gtk_widget_show_all (applet);
/*      applets = g_list_remove (applets, applet);*/


      if (selected_column >= 0)
        {
          columns_last_y[selected_column] += req.height + PADDING;
          columns_last_width[selected_column] = req.width;

          if (columns_last_y[i_column] > columns_last_y[(i_column+1)%2])
            i_column = (i_column + 1)%2;

        }

    }

  g_list_free (to_place);

#undef PADDING

}
#endif

static void
remove_rectangle (GtkWidget *widget, GdkRegion *region)
{
  GdkRegion    *to_remove;
  GdkRectangle  rectangle = (GdkRectangle)widget->allocation;
  gint          x, y;

  gtk_container_child_get (GTK_CONTAINER (widget->parent), widget,
                           "x", &x,
                           "y", &y,
                           NULL);

  rectangle.x = x;
  rectangle.y = y;

  to_remove = gdk_region_rectangle (&rectangle);
  gdk_region_subtract (region, to_remove);
  gdk_region_destroy (to_remove);
}

static void
hildon_home_area_batch_add (HildonHomeArea *area)
{
  HildonHomeAreaPriv      *priv;
#ifdef SIMPLE_PLACEMENT
  GList                   *children, *to_place;
  g_return_if_fail (area);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  children = gtk_container_get_children (GTK_CONTAINER (area));

  to_place = g_list_concat (children, priv->to_add);

  hildon_home_area_place (area, to_place);

  g_list_free (children);
/*  g_list_free (priv->to_add);*/

#else
  GList                *i;
  GList                *children;
  GdkRegion            *region, *clean_region;
  GdkRectangle          area_rectangle = {0};

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  area_rectangle.width  = GTK_WIDGET (area)->allocation.width;
  area_rectangle.height = GTK_WIDGET (area)->allocation.height;

  clean_region = gdk_region_rectangle (&area_rectangle);
  region = gdk_region_copy (clean_region);

  children = gtk_container_get_children (GTK_CONTAINER (area));
  g_list_foreach (children, (GFunc)remove_rectangle, region);

  i = priv->to_add;

  while (i)
    {
      GtkRequisition    req = {0};
      GdkRectangle     *rectangles;
      gint              n_rect, i_rect;
      GtkWidget        *w;
      const gchar      *name;

      w = GTK_WIDGET (i->data);
      name = hildon_desktop_item_get_id (HILDON_DESKTOP_ITEM (w));

      g_debug ("Placing %s", name);
              
      gtk_widget_size_request (w, &req);
      gdk_region_get_rectangles (region, &rectangles, &n_rect);

      g_debug ("Got %i rectangles", n_rect);

      for (i_rect = 0; i_rect < n_rect; i_rect++)
        {
          g_debug ("Rectangle %i: (%i,%i) %ix%i",
                   i_rect,
                   rectangles[i_rect].x,
                   rectangles[i_rect].y,
                   rectangles[i_rect].width,
                   rectangles[i_rect].height);
          if (rectangles[i_rect].width  >= req.width &&
              rectangles[i_rect].height >= req.height)
            {
              GdkRectangle     *layout = g_new (GdkRectangle, 1);
              GdkRegion        *layout_region;

              layout->x = rectangles[i_rect].x;
              layout->y = rectangles[i_rect].y;
              layout->width = -1;
              layout->height = -1;

              g_hash_table_insert (priv->layout, g_strdup (name), layout);
              gtk_container_add (GTK_CONTAINER (area), w);

              layout->width  = req.width;
              layout->height = req.height;

              layout_region = gdk_region_rectangle (layout);
              gdk_region_subtract (region, layout_region);
              gdk_region_destroy (layout_region);

              break;
            }

        }

      if (i_rect == n_rect)
        {
          g_debug ("Adding layer");
          /* Not enough place in this layer, we need to add one */
          gdk_region_destroy (region);
          region = gdk_region_copy (clean_region);
        }
      else
        i = g_list_next (i);

      g_free (rectangles);
    }

  gdk_region_destroy (clean_region);
  gdk_region_destroy (region);

  g_list_free (children);

#endif
  priv->to_add = NULL;

}

static void
hildon_home_area_remove (GtkContainer *area, GtkWidget *applet)
{
  HildonHomeAreaPriv      *priv;
  g_return_if_fail (area);

  priv = HILDON_HOME_AREA_GET_PRIVATE (area);
  if (priv->layout_mode)
    {
      g_signal_emit_by_name (area, "layout-changed");
    }

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
                         (GtkCallback)hildon_desktop_home_item_save_position,
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

  g_debug ("Loading Hildon Home layout from %s", path);

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

#if 0
      else
        {
          applet = hildon_desktop_home_item_new_with_plugin (groups[n_groups-1]);
          if (applet)
            {
              gtk_fixed_put (GTK_FIXED (area),
                             applet,
                             x, 
                             y);

              gtk_widget_show (applet);
              
              g_signal_emit_by_name (G_OBJECT (area), "applet-added", applet);
              if (priv->layout_mode)
                {
                  g_signal_emit_by_name (G_OBJECT (area), "layout-changed");
                  hildon_desktop_home_item_set_layout_mode
                      (HILDON_DESKTOP_HOME_ITEM (applet),
                       TRUE);
                }

            }
        }
#endif
      

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

