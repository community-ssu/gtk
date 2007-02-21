/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
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

#include "hd-home-background.h"
#include "background-manager/hildon-background-manager.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <gdk/gdkcolor.h>
#include <gdk/gdkwindow.h>
#include <gdk/gdkpixmap.h>
#include <gdk/gdkx.h>

#include <glib/gkeyfile.h>

/* Key used in the home-background.conf */
#define HD_HOME_BACKGROUND_KEY_GROUP            "Hildon Home"
#define HD_HOME_BACKGROUND_KEY_URI              "BackgroundImage"
#define HD_HOME_BACKGROUND_KEY_SIDEBAR          "SidebarImage"
#define HD_HOME_BACKGROUND_KEY_TITLEBAR         "TitlebarImage"
#define HD_HOME_BACKGROUND_KEY_BOTTOMBAR        "BottombarImage"
#define HD_HOME_BACKGROUND_KEY_RIGHTBAR         "RightbarImage"
#define HD_HOME_BACKGROUND_KEY_RED              "Red"
#define HD_HOME_BACKGROUND_KEY_GREEN            "Green"
#define HD_HOME_BACKGROUND_KEY_BLUE             "Blue"
#define HD_HOME_BACKGROUND_KEY_MODE             "Mode"

/* Some pre-defined values */
#define HD_HOME_BACKGROUND_VALUE_NO_IMAGE       HD_HOME_BACKGROUND_NO_IMAGE
#define HD_HOME_BACKGROUND_VALUE_CENTERED       "Centered"
#define HD_HOME_BACKGROUND_VALUE_STRETCHED      "Stretched"
#define HD_HOME_BACKGROUND_VALUE_SCALED         "Scaled"
#define HD_HOME_BACKGROUND_VALUE_TILED          "Tiled"

enum
{
  PROP_FILENAME = 1,
  PROP_NORTH_BORDER,
  PROP_SOUTH_BORDER,
  PROP_EAST_BORDER,
  PROP_WEST_BORDER,
  PROP_MODE,
  PROP_COLOR
};

struct _HDHomeBackgroundPrivate
{
  GdkColor         *color;

  BackgroundMode    mode;

  gchar            *filename;
  gchar            *north_border;
  gchar            *west_border;
  gchar            *east_border;
  gchar            *south_border;

};

G_DEFINE_TYPE (HDHomeBackground, hd_home_background, G_TYPE_OBJECT);

static void hd_home_background_finalize (GObject *object);

static void hd_home_background_set_property (GObject       *object,
                                             guint          property_id,
                                             const GValue  *value,
                                             GParamSpec    *pspec);

static void hd_home_background_get_property (GObject       *object,
                                             guint          property_id,
                                             GValue  *value,
                                             GParamSpec    *pspec);

static void
hd_home_background_init (HDHomeBackground *background)
{
  background->priv = 
      G_TYPE_INSTANCE_GET_PRIVATE ((background),
                                   HD_TYPE_HOME_BACKGROUND,
                                   HDHomeBackgroundPrivate);
}

static void
hd_home_background_class_init (HDHomeBackgroundClass *klass)
{
  GObjectClass *object_class;
  GParamSpec   *pspec;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = hd_home_background_finalize;
  object_class->set_property = hd_home_background_set_property;
  object_class->get_property = hd_home_background_get_property;
  
  pspec = g_param_spec_string ("filename",
                               "filename",
                               "Image filename",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_FILENAME,
                                   pspec);
  
  pspec = g_param_spec_string ("north-border",
                               "North border",
                               "North border image filename",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_NORTH_BORDER,
                                   pspec);
  
  pspec = g_param_spec_string ("south-border",
                               "South border",
                               "South border image filename",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_SOUTH_BORDER,
                                   pspec);
  
  pspec = g_param_spec_string ("west-border",
                               "West border",
                               "West border image filename",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_WEST_BORDER,
                                   pspec);
  
  pspec = g_param_spec_string ("east-border",
                               "East border",
                               "East border image filename",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_EAST_BORDER,
                                   pspec);
  
  pspec = g_param_spec_int ("mode",
                            "Mode",
                            "Background stretching mode",
                            BACKGROUND_CENTERED,
                            BACKGROUND_TILED,
                            BACKGROUND_TILED,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_MODE,
                                   pspec);

  pspec = g_param_spec_pointer ("color",
                                "Color",
                                "Background color",
                                G_PARAM_READWRITE);
  g_object_class_install_property (object_class,
                                   PROP_COLOR,
                                   pspec);
                                
  g_type_class_add_private (klass, sizeof (HDHomeBackgroundPrivate));
}

static void
hd_home_background_finalize (GObject *object)
{
  HDHomeBackgroundPrivate  *priv; 
  
  g_return_if_fail (HD_IS_HOME_BACKGROUND (object));
  priv = HD_HOME_BACKGROUND (object)->priv;

  g_free (priv->filename);
  priv->filename = NULL;

  g_free (priv->north_border);
  priv->north_border = NULL;

  g_free (priv->west_border);
  priv->west_border = NULL;

  g_free (priv->east_border);
  priv->east_border = NULL;

  g_free (priv->south_border);
  priv->south_border = NULL;

  if (priv->color)
    gdk_color_free (priv->color);
  priv->color = NULL;

}

static void
hd_home_background_set_property (GObject       *object,
                                 guint          property_id,
                                 const GValue  *value,
                                 GParamSpec    *pspec)
{
  HDHomeBackgroundPrivate  *priv = HD_HOME_BACKGROUND (object)->priv;

  switch (property_id)
    {
      case PROP_FILENAME:
          g_free (priv->filename);
          if (g_str_equal (g_value_get_string (value),
                           HD_HOME_BACKGROUND_NO_IMAGE))
            priv->filename = g_strdup ("");
          else
            priv->filename = g_strdup (g_value_get_string (value));
          break;
      case PROP_NORTH_BORDER:
          g_free (priv->north_border);
          priv->north_border = g_strdup (g_value_get_string (value));
          break;
      case PROP_WEST_BORDER:
          g_free (priv->west_border);
          priv->west_border = g_strdup (g_value_get_string (value));
          break;
      case PROP_EAST_BORDER:
          g_free (priv->east_border);
          priv->east_border = g_strdup (g_value_get_string (value));
          break;
      case PROP_SOUTH_BORDER:
          g_free (priv->south_border);
          priv->east_border = g_strdup (g_value_get_string (value));
          break;
      case PROP_MODE:
          priv->mode = g_value_get_int (value);
          break;
      case PROP_COLOR:
          if (priv->color)
            gdk_color_free (priv->color);
          priv->color = gdk_color_copy (g_value_get_pointer (value));
          break;
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
hd_home_background_get_property (GObject       *object,
                                 guint          property_id,
                                 GValue        *value,
                                 GParamSpec    *pspec)
{
  HDHomeBackgroundPrivate  *priv = HD_HOME_BACKGROUND (object)->priv;

  switch (property_id)
    {
      case PROP_FILENAME:
          g_value_set_string (value, priv->filename);
          break;
      case PROP_NORTH_BORDER:
          g_value_set_string (value, priv->north_border);
          break;
      case PROP_WEST_BORDER:
          g_value_set_string (value, priv->west_border);
          break;
      case PROP_EAST_BORDER:
          g_value_set_string (value, priv->east_border);
          break;
      case PROP_SOUTH_BORDER:
          g_value_set_string (value, priv->south_border);
          break;
      case PROP_MODE:
          g_value_set_int (value, priv->mode);
          break;
      case PROP_COLOR:
          g_value_set_pointer (value, priv->color);
          break;
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

void
hd_home_background_save (HDHomeBackground *background,
                         const gchar *filename,
                         GError **error)
{
  HDHomeBackgroundPrivate  *priv; 
  GKeyFile                 *keyfile;
  GError                   *local_error = NULL;
  gchar                    *buffer;
  gssize                    buffer_length;

  g_return_if_fail (HD_IS_HOME_BACKGROUND (background) && filename);
  priv = background->priv;

  keyfile = g_key_file_new ();

  /* Image files */
  if (priv->filename)
    g_key_file_set_string (keyfile,
                           HD_HOME_BACKGROUND_KEY_GROUP,
                           HD_HOME_BACKGROUND_KEY_URI,
                           priv->filename);

  else
    g_key_file_set_string (keyfile,
                           HD_HOME_BACKGROUND_KEY_GROUP,
                           HD_HOME_BACKGROUND_KEY_URI,
                           HD_HOME_BACKGROUND_VALUE_NO_IMAGE);

  if (priv->north_border)
    g_key_file_set_string (keyfile,
                           HD_HOME_BACKGROUND_KEY_GROUP,
                           HD_HOME_BACKGROUND_KEY_TITLEBAR,
                           priv->north_border);
  
  if (priv->south_border)
    g_key_file_set_string (keyfile,
                           HD_HOME_BACKGROUND_KEY_GROUP,
                           HD_HOME_BACKGROUND_KEY_BOTTOMBAR,
                           priv->south_border);
  
  if (priv->west_border)
    g_key_file_set_string (keyfile,
                           HD_HOME_BACKGROUND_KEY_GROUP,
                           HD_HOME_BACKGROUND_KEY_SIDEBAR,
                           priv->west_border);
  
  if (priv->east_border)
    g_key_file_set_string (keyfile,
                           HD_HOME_BACKGROUND_KEY_GROUP,
                           HD_HOME_BACKGROUND_KEY_RIGHTBAR,
                           priv->east_border);

  /* Color */
  if (priv->color)
    {
      g_key_file_set_integer (keyfile,
                              HD_HOME_BACKGROUND_KEY_GROUP,
                              HD_HOME_BACKGROUND_KEY_RED,
                              priv->color->red);
      g_key_file_set_integer (keyfile,
                              HD_HOME_BACKGROUND_KEY_GROUP,
                              HD_HOME_BACKGROUND_KEY_GREEN,
                              priv->color->green);
      g_key_file_set_integer (keyfile,
                              HD_HOME_BACKGROUND_KEY_GROUP,
                              HD_HOME_BACKGROUND_KEY_BLUE,
                              priv->color->blue);
    }

  /* Mode */
  switch (priv->mode)
    {
      case BACKGROUND_CENTERED:
          g_key_file_set_string (keyfile,
                                 HD_HOME_BACKGROUND_KEY_GROUP,
                                 HD_HOME_BACKGROUND_KEY_MODE,
                                 HD_HOME_BACKGROUND_VALUE_CENTERED);
          break;
      case BACKGROUND_SCALED:
          g_key_file_set_string (keyfile,
                                 HD_HOME_BACKGROUND_KEY_GROUP,
                                 HD_HOME_BACKGROUND_KEY_MODE,
                                 HD_HOME_BACKGROUND_VALUE_SCALED);
          break;
      case BACKGROUND_STRETCHED:
          g_key_file_set_string (keyfile,
                                 HD_HOME_BACKGROUND_KEY_GROUP,
                                 HD_HOME_BACKGROUND_KEY_MODE,
                                 HD_HOME_BACKGROUND_VALUE_STRETCHED);
          break;
      case BACKGROUND_TILED:
          g_key_file_set_string (keyfile,
                                 HD_HOME_BACKGROUND_KEY_GROUP,
                                 HD_HOME_BACKGROUND_KEY_MODE,
                                 HD_HOME_BACKGROUND_VALUE_TILED);
          break;
    }

  buffer = g_key_file_to_data (keyfile,
                               &buffer_length,
                               &local_error);
  if (local_error) goto cleanup;

  g_file_set_contents (filename,
                       buffer,
                       buffer_length,
                       &local_error);
  
cleanup:
  g_key_file_free (keyfile);

  g_free (buffer);

  if (local_error)
    g_propagate_error (error, local_error);
}

void
hd_home_background_load (HDHomeBackground *background,
                         const gchar *filename,
                         GError **error)
{
  HDHomeBackgroundPrivate  *priv; 
  GKeyFile                 *keyfile;
  GError                   *local_error = NULL;
  gint                      component;
  gchar                    *mode = NULL;
  
  g_return_if_fail (HD_IS_HOME_BACKGROUND (background) && filename);
  priv = background->priv;

  g_debug ("Loading background from %s", filename);

  keyfile = g_key_file_new ();
  g_key_file_load_from_file (keyfile,
                             filename,
                             G_KEY_FILE_NONE,
                             &local_error);
  if (local_error) goto cleanup;

  g_free (priv->filename);
  priv->filename = g_key_file_get_string (keyfile,
                                          HD_HOME_BACKGROUND_KEY_GROUP,
                                          HD_HOME_BACKGROUND_KEY_URI,
                                          &local_error);
  
  if (local_error) goto cleanup;

  if (g_str_equal (priv->filename, HD_HOME_BACKGROUND_VALUE_NO_IMAGE))
    {
      g_free (priv->filename);
      priv->filename = NULL;
    }

  /* the borders are not mandatory */
  g_free (priv->north_border);
  priv->north_border = g_key_file_get_string (keyfile,
                                              HD_HOME_BACKGROUND_KEY_GROUP,
                                              HD_HOME_BACKGROUND_KEY_TITLEBAR,
                                              NULL);
  g_free (priv->west_border);
  priv->west_border = g_key_file_get_string (keyfile,
                                             HD_HOME_BACKGROUND_KEY_GROUP,
                                             HD_HOME_BACKGROUND_KEY_SIDEBAR,
                                             NULL);
  g_free (priv->east_border);
  priv->east_border = g_key_file_get_string (keyfile,
                                             HD_HOME_BACKGROUND_KEY_GROUP,
                                             HD_HOME_BACKGROUND_KEY_RIGHTBAR,
                                             NULL);
  g_free (priv->south_border);
  priv->south_border = g_key_file_get_string (keyfile,
                                              HD_HOME_BACKGROUND_KEY_GROUP,
                                              HD_HOME_BACKGROUND_KEY_BOTTOMBAR,
                                              NULL);
  
  /* Color */
  component = g_key_file_get_integer (keyfile,
                                      HD_HOME_BACKGROUND_KEY_GROUP,
                                      HD_HOME_BACKGROUND_KEY_RED,
                                      &local_error);

  if (!priv->color)
    priv->color = g_new0 (GdkColor, 1);

  if (local_error)
    {
      priv->color->red = 0;
      g_clear_error (&local_error);
    }
  else
    priv->color->red = component;
  
  component = g_key_file_get_integer (keyfile,
                                      HD_HOME_BACKGROUND_KEY_GROUP,
                                      HD_HOME_BACKGROUND_KEY_GREEN,
                                      &local_error);
  if (local_error)
    {
      priv->color->green = 0;
      g_clear_error (&local_error);
    }
  else
    priv->color->green = component;
  
  component = g_key_file_get_integer (keyfile,
                                      HD_HOME_BACKGROUND_KEY_GROUP,
                                      HD_HOME_BACKGROUND_KEY_BLUE,
                                      &local_error);
  if (local_error)
    {
      priv->color->blue = 0;
      g_clear_error (&local_error);
    }
  else
    priv->color->blue = component;

  /* Mode */
  
  mode = g_key_file_get_string (keyfile,
                                HD_HOME_BACKGROUND_KEY_GROUP,
                                HD_HOME_BACKGROUND_KEY_MODE,
                                NULL);

  if (!mode)
    {
      priv->mode = BACKGROUND_TILED;
      goto cleanup;
    }

  if (g_str_equal (mode, HD_HOME_BACKGROUND_VALUE_CENTERED))
    priv->mode = BACKGROUND_CENTERED;
  else if (g_str_equal (mode, HD_HOME_BACKGROUND_VALUE_SCALED))
    priv->mode = BACKGROUND_SCALED;
  else if (g_str_equal (mode, HD_HOME_BACKGROUND_VALUE_STRETCHED))
    priv->mode = BACKGROUND_STRETCHED;
  else
    priv->mode = BACKGROUND_TILED;

cleanup:
  g_free (mode);
  g_key_file_free (keyfile);
  if (local_error)
    g_propagate_error (error, local_error);
}

static void
free_pixmap (Pixmap pixmap_xid)
{
  g_debug ("Freeing pixmap %i\n", (int)pixmap_xid);
  XFreePixmap (GDK_DISPLAY (), pixmap_xid);
}

void
hd_home_background_apply (HDHomeBackground *background,
                          GdkWindow        *window,
                          GdkRectangle     *area,
                          GError          **error)
{
  HDHomeBackgroundPrivate  *priv;
  DBusGProxy           *background_manager_proxy;
  DBusGConnection      *connection;
  GError               *local_error = NULL;
  gint                  pixmap_xid;
  GdkPixmap            *pixmap = NULL;
  gint32                top_offset, bottom_offset, right_offset, left_offset;

  g_return_if_fail (HD_IS_HOME_BACKGROUND (background) && window);
  priv = background->priv;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &local_error);
  if (local_error)
    {
      g_propagate_error (error, local_error);
      return;
    }

  background_manager_proxy =
      dbus_g_proxy_new_for_name (connection,
                                 HILDON_BACKGROUND_MANAGER_SERVICE,
                                 HILDON_BACKGROUND_MANAGER_OBJECT_PATH,
                                 HILDON_BACKGROUND_MANAGER_INTERFACE);

  top_offset = bottom_offset = right_offset = left_offset = 0;
  if (area)
    {
      gint width, height;
      gdk_drawable_get_size (GDK_DRAWABLE (window), &width, &height);

      top_offset = area->y;
      bottom_offset = MAX (0, height - area->height - area->y);
      left_offset = area->x;
      right_offset = MAX (0, width- area->width - area->x);
    }

  g_debug ("Trying to apply background to window: %x", (int)GDK_WINDOW_XID(window));

#define S(string) (string?string:"")
  org_maemo_hildon_background_manager_set_background (background_manager_proxy,
                                                      GDK_WINDOW_XID (window),
                                                      S(priv->filename),
                                                      S(priv->west_border),
                                                      S(priv->north_border),
                                                      priv->color->red,
                                                      priv->color->green,
                                                      priv->color->blue,
                                                      priv->mode,
                                                      top_offset,
                                                      bottom_offset,
                                                      left_offset,
                                                      right_offset,
                                                      &pixmap_xid,
                                                      error);
#undef S

  if (pixmap_xid)
    pixmap = gdk_pixmap_foreign_new (pixmap_xid);

  if (pixmap)
    {
      gdk_window_set_back_pixmap (window, pixmap, FALSE);
      g_object_unref (pixmap);
    }
  else
    g_warning ("No such pixmap: %i", pixmap_xid);

}

struct cb_data
{
  HDHomeBackground                 *background;
  HDHomeBackgroundApplyCallback     callback;
  gpointer                          user_data;
  GdkWindow                        *window;
};


static void
hd_home_background_apply_async_dbus_callback (DBusGProxy       *proxy,
                                              gint              pixmap_xid,
                                              GError           *error,
                                              struct cb_data   *data)
{
  GdkPixmap *pixmap;
  
  g_debug ("DBUS Callback");

  if (error)
    {
      g_warning ("Error when setting background: %s", error->message);
      goto cleanup;
    }

  if (!pixmap_xid)
    {
      g_warning ("No pixmap id returned");
      goto cleanup;
    }
    
  pixmap = gdk_pixmap_foreign_new (pixmap_xid);

  if (pixmap)
    {
      GdkColormap *colormap;

      g_object_weak_ref (G_OBJECT (pixmap),
                         (GWeakNotify)free_pixmap,
                         GINT_TO_POINTER((GDK_PIXMAP_XID (pixmap))));

      colormap = gdk_drawable_get_colormap (GDK_DRAWABLE (data->window));
      gdk_drawable_set_colormap (GDK_DRAWABLE (pixmap), colormap);

      gdk_window_set_back_pixmap (data->window, pixmap, FALSE);
      g_object_unref (pixmap);
    }
  else
    g_warning ("No such pixmap: %i", pixmap_xid);

  if (data->callback)
    data->callback (data->background, pixmap_xid, error, data->user_data);

cleanup:
  g_free (data);
}

void
hd_home_background_apply_async (HDHomeBackground               *background,
                                GdkWindow                      *window,
                                GdkRectangle                   *area,
                                HDHomeBackgroundApplyCallback   cb,
                                gpointer                        user_data)
{
  HDHomeBackgroundPrivate  *priv;
  DBusGProxy               *background_manager_proxy;
  DBusGConnection          *connection;
  GError                   *local_error = NULL;
  struct cb_data           *data;
  gint32                    top_offset, bottom_offset, right_offset, left_offset;

  g_return_if_fail (HD_IS_HOME_BACKGROUND (background) && window);
  priv = background->priv;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &local_error);
  if (local_error)
    {
      if (cb)
        cb (background, 0, local_error, user_data);
      g_error_free (local_error);
      return;
    }

  background_manager_proxy =
      dbus_g_proxy_new_for_name (connection,
                                 HILDON_BACKGROUND_MANAGER_SERVICE,
                                 HILDON_BACKGROUND_MANAGER_OBJECT_PATH,
                                 HILDON_BACKGROUND_MANAGER_INTERFACE);

  data = g_new (struct cb_data, 1);

  data->callback = cb;
  data->background = background;
  data->user_data = user_data;
  data->window = window;

  top_offset = bottom_offset = right_offset = left_offset = 0;
  if (area)
    {
      gint width, height;
      gdk_drawable_get_size (GDK_DRAWABLE (window), &width, &height);

      top_offset = area->y;
      bottom_offset = MAX (0, height - area->height - area->y);
      left_offset = area->x;
      right_offset = MAX (0, width- area->width - area->x);
    }

  g_debug ("Applying background %s aynchronously",
           priv->filename);

  /* Here goes */
#define S(string) (string?string:"")
  org_maemo_hildon_background_manager_set_background_async 
                                                (background_manager_proxy,
                                                 GDK_WINDOW_XID (window),
                                                 S(priv->filename),
                                                 S(priv->west_border),
                                                 S(priv->north_border),
                                                 priv->color->red,
                                                 priv->color->green,
                                                 priv->color->blue,
                                                 priv->mode,
                                                 top_offset,
                                                 bottom_offset,
                                                 left_offset,
                                                 right_offset,
                                                 (org_maemo_hildon_background_manager_set_background_reply) hd_home_background_apply_async_dbus_callback,
                                                 data);
#undef S
}

HDHomeBackground *
hd_home_background_copy (const HDHomeBackground *src)
{
  HDHomeBackgroundPrivate  *priv;
  HDHomeBackground         *dest;

  g_return_val_if_fail (HD_IS_HOME_BACKGROUND (src), NULL);

  priv = src->priv;

  dest = g_object_new (HD_TYPE_HOME_BACKGROUND,
                       "mode", priv->mode,
                       "color", priv->color,
                       "filename", priv->filename,
                       "west-border", priv->west_border,
                       "east-border", priv->east_border,
                       "south-border", priv->south_border,
                       "north-border", priv->north_border,
                       NULL);

  return dest;

}