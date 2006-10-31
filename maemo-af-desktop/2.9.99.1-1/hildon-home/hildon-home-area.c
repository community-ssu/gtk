/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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
#include "hildon-home-applet.h"

#include "hildon-home-private.h"
#include "hildon-home-interface.h" /* .desktop files keys */
#include <hildon-widgets/hildon-note.h>
#include <string.h> /* strlen */


enum
{
  HILDON_HOME_AREA_PROPERTY_LAYOUT_MODE = 1
};

typedef struct HildonHomeAreaPriv_
{
  gboolean      layout_mode;
  gboolean      layout_changed;
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

  container_class->remove = hildon_home_area_remove;

  object_class->set_property = hildon_home_area_set_property;
  object_class->get_property = hildon_home_area_get_property;

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
                                 G_PARAM_READABLE | G_PARAM_WRITABLE);

  g_object_class_install_property (object_class,
                                   HILDON_HOME_AREA_PROPERTY_LAYOUT_MODE,
                                   pspec);
                
}

static void
hildon_home_area_init (HildonHomeArea *area)
{
}

static void
hildon_home_area_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id)
    {
      case HILDON_HOME_AREA_PROPERTY_LAYOUT_MODE:
          hildon_home_area_set_layout_mode (HILDON_HOME_AREA (object),
                                            g_value_get_boolean (value));
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
                         (GtkCallback)hildon_home_applet_set_layout_mode,
                         (gpointer)TRUE);

  g_signal_emit_by_name (area, "layout-mode-started");
}

static void
hildon_home_area_layout_mode_end (HildonHomeArea *area)
{
  gtk_container_foreach (GTK_CONTAINER (area),
                        (GtkCallback)hildon_home_applet_set_layout_mode,
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
      if (HILDON_IS_HOME_APPLET (applet))
        hildon_home_applet_set_layout_mode (HILDON_HOME_APPLET (applet), TRUE);
      g_signal_emit_by_name (area, "layout-changed");
    }
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
                                     const gchar *path)
{
  GKeyFile *keyfile;
  FILE     *file;
  gchar    *buffer = NULL;
  guint     buffer_size;
  GError   *error = NULL;
  gint      ret;

  keyfile = g_key_file_new ();

  gtk_container_foreach (GTK_CONTAINER (area),
                         (GtkCallback)hildon_home_applet_save_position,
                         keyfile);

  file = fopen (path, "w");

  if (!file)
    {
      g_warning ("Could not open %s for saving the applet configuration", path);
      g_key_file_free (keyfile);
      return -1;
    }

  buffer = g_key_file_to_data (keyfile, &buffer_size, &error);

  if (error)
    {
      g_warning ("Could not create buffer to save applet configuration: %s",
                 error->message);
      g_error_free (error);
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
                                     const gchar *path)
{
  HildonHomeAreaPriv      *priv;
  GKeyFile *keyfile;
  GError *error = NULL;
  gchar **groups = NULL;
  gint n_groups;
  GList *applets = NULL;

  g_return_if_fail (area);
  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  applets = gtk_container_get_children (GTK_CONTAINER (area));

  keyfile = g_key_file_new ();

  g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, &error);
  
  if (error) goto cleanup;

  groups = g_key_file_get_groups (keyfile, &n_groups);

  while (n_groups > 0)
    {
      GtkWidget *applet;
      gint x,y,width,height;
      GList *list_element;

      x = g_key_file_get_integer (keyfile,
                                  groups[n_groups-1],
                                  APPLET_KEY_X,
                                  &error);
      
      y = g_key_file_get_integer (keyfile,
                                  groups[n_groups-1],
                                  APPLET_KEY_Y,
                                  &error);
      
      width = g_key_file_get_integer (keyfile,
                                      groups[n_groups-1],
                                      APPLET_KEY_WIDTH,
                                      &error);
      
      height = g_key_file_get_integer (keyfile,
                                       groups[n_groups-1],
                                       APPLET_KEY_HEIGHT,
                                       &error);
      if (error) goto cleanup;

      /* Do we already have this applet loaded? */

      list_element = g_list_find_custom (applets,
                                         groups[n_groups-1],
                                         (GCompareFunc)
                                         hildon_home_applet_find_by_name);

      if (list_element)
        {
          applet = GTK_WIDGET (list_element->data);
          gtk_fixed_move (GTK_FIXED (area),
                          applet,
                          x, 
                          y);

          applets = g_list_remove (applets, applet);
        }

      else
        {
          applet = hildon_home_applet_new_with_plugin (groups[n_groups-1]);
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
                  hildon_home_applet_set_layout_mode
                      (HILDON_HOME_APPLET (applet),
                       TRUE);
                }

            }
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

  if (error)
    {
      g_warning ("An error occurred when reading HildonHome applets' "
                 "configuration: %s",
               error->message);
      g_error_free (error);
    }

  g_key_file_free (keyfile);
}

void
hildon_home_area_sync_to_list (HildonHomeArea *area, HildonPluginList *list)
{
  GtkTreeIter i;
  GList *applets;
  gboolean valid;

  g_return_if_fail (area && list);

  applets = gtk_container_get_children (GTK_CONTAINER (area));
  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list), &i);

  while (valid)
    {
      gchar *desktop_file = NULL;
      GtkWidget *applet = NULL;
      GList *list_element;
      
      gtk_tree_model_get (GTK_TREE_MODEL (list),
                          &i, 
                          HILDON_PLUGIN_LIST_COLUMN_DESKTOP_FILE,
                          &desktop_file,
                          -1);

      list_element = g_list_find_custom (applets,
                                         desktop_file,
                                         (GCompareFunc)
                                         hildon_home_applet_find_by_name);

      if (list_element)
        applet = GTK_WIDGET (list_element->data);

      gtk_list_store_set (GTK_LIST_STORE (list),
                          &i,
                          HILDON_PLUGIN_LIST_COLUMN_ACTIVE,
                          applet? TRUE:FALSE,
                          -1);

      g_free (desktop_file);
      valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (list), &i);
    }
  
  g_list_free (applets);
}

gint
hildon_home_area_sync_from_list (HildonHomeArea *area, HildonPluginList *list)
{
  HildonHomeAreaPriv      *priv;
  GtkTreeIter i;
  GList *applets = NULL;
  GSList *applets_to_remove = NULL;
  gint x = LAYOUT_AREA_LEFT_BORDER_PADDING;
  gint y = LAYOUT_AREA_TOP_BORDER_PADDING;
  gboolean valid;
  gint n_added = 0;

  g_return_val_if_fail (area && list, n_added);
  priv = HILDON_HOME_AREA_GET_PRIVATE (area);

  applets = gtk_container_get_children (GTK_CONTAINER (area));
  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list), &i);

  while (valid)
    {
      gchar *desktop_file = NULL;
      gboolean active;
      GtkWidget *applet = NULL;
      GList *list_element;
      
      gtk_tree_model_get (GTK_TREE_MODEL (list),
                          &i, 
                          HILDON_PLUGIN_LIST_COLUMN_DESKTOP_FILE,
                          &desktop_file,
                          HILDON_PLUGIN_LIST_COLUMN_ACTIVE,
                          &active,
                          -1);

      list_element = g_list_find_custom (applets,
                                         desktop_file,
                                         (GCompareFunc)
                                         hildon_home_applet_find_by_name);

      if (list_element)
        applet = GTK_WIDGET (list_element->data);

      if (active && !applet)
        {
          applet = hildon_home_applet_new_with_plugin (desktop_file);

          if (applet)
            {
              g_signal_emit_by_name (G_OBJECT (area), "applet-selected", applet);
              if (x + applet->requisition.width >
                  GTK_WIDGET (area)->allocation.width 
                    - LAYOUT_AREA_LEFT_BORDER_PADDING
                    - LAYOUT_AREA_RIGHT_BORDER_PADDING)
                x = LAYOUT_AREA_LEFT_BORDER_PADDING;
              
              if (y + applet->requisition.height >
                  GTK_WIDGET (area)->allocation.height
                    - LAYOUT_AREA_TOP_BORDER_PADDING
                    - LAYOUT_AREA_BOTTOM_BORDER_PADDING)
                y = LAYOUT_AREA_TOP_BORDER_PADDING;

              gtk_fixed_put (GTK_FIXED (area), applet, x, y);
              g_signal_emit_by_name (G_OBJECT (area), "applet-added", applet);
              x += APPLET_ADD_X_STEP;
              y += APPLET_ADD_Y_STEP;
              
              gtk_widget_show (applet);

              n_added ++;
            }
        }
      else if (applet)
        {
          applets = g_list_remove (applets, applet);
          if (!active)
            applets_to_remove = g_slist_append (applets_to_remove, applet);
        }
      
      g_free (desktop_file);
      valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (list), &i);
    }

  /* First go to layout mode ... */
  if (n_added)
    {
      hildon_home_area_set_layout_mode (area, TRUE);
      g_signal_emit_by_name (area, "layout-changed");
    }

  /* ... then remove the applets  which are no longer active */
  g_slist_foreach (applets_to_remove,
                   (GFunc)gtk_widget_destroy,
                   NULL);
  /* and finally these which were not in the list */
  g_list_foreach (applets,
                  (GFunc)gtk_widget_destroy,
                  NULL);
  g_slist_free (applets_to_remove);
  g_list_free (applets);

  return n_added;
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
      if (!HILDON_IS_HOME_APPLET (l->data))
        continue;

      overlap = hildon_home_applet_get_overlaps 
          (HILDON_HOME_APPLET (l->data));
    }

  g_list_free (applets);

  return overlap;
}
