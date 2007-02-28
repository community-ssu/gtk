/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of hildon-desktop
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "background-manager.h"
#include "background-manager-glue.h"

#include <gtk/gtkmain.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkx.h>

#ifdef HAVE_LIBOSSO
/*SAW (allocation watchdog facilities)*/
#include <osso-mem.h>
#endif

#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf-client.h>


#define HILDON_HOME_IMAGE_FORMAT           "png"
#define HILDON_HOME_IMAGE_ALPHA_FULL       255
#define HILDON_HOME_GCONF_MMC_COVER_OPEN   "/system/osso/af/mmc-cover-open"

#define HILDON_HOME_ENV_MMC_MOUNTPOINT     "MMC_MOUNTPOINT"


extern GMainLoop *main_loop;

GQuark
background_manager_error_quark (void)
{
  return g_quark_from_static_string ("background-manager-error-quark");
}

GType
background_mode_get_type (void)
{
  static GType etype = 0;

  if (!etype)
    {
      static const GEnumValue values[] =
      {
        { BACKGROUND_CENTERED, "BACKGROUND_CENTERED", "centered" },
        { BACKGROUND_SCALED, "BACKGROUND_SCALED", "scaled" },
        { BACKGROUND_STRETCHED, "BACKGROUND_STRETCHED", "stretched" },
        { BACKGROUND_TILED, "BACKGROUND_TILED", "tiled" },
        { 0, NULL, NULL }
      };
      
      etype = g_enum_register_static ("BackgroundMode", values);
    }

  return etype;
}

G_DEFINE_TYPE (BackgroundManager, background_manager, G_TYPE_OBJECT);

static void
background_manager_set_property (GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
background_manager_get_property (GObject    *object,
				 guint       prop_id,
				 GValue     *value,
				 GParamSpec *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

#ifdef HAVE_LIBOSSO
static void
load_image_oom_cb (size_t  current_size,
		   size_t  max_size,
		   void   *context)
{
  *(gboolean *) context = TRUE;
}
#endif

#define BUFFER_SIZE	8192

static GdkPixbuf *
load_image_from_uri (const gchar  *uri,
                     gboolean      oom_check,
                     gboolean      cancellable,
                     GError      **error)
{
  GConfClient *client;
  GdkPixbufLoader *loader;
  GdkPixbuf *retval = NULL;
  GnomeVFSHandle *handle = NULL;
  GnomeVFSResult result;
  guchar buffer[BUFFER_SIZE];
  gchar *mmc_mount_point;
  gboolean image_from_mmc = FALSE;
  gboolean mmc_cover_open = FALSE;
  gboolean oom = FALSE;

  g_return_val_if_fail (uri != NULL, NULL);

  client = gconf_client_get_default ();
  g_assert (GCONF_IS_CLIENT (client));

  mmc_mount_point = g_strconcat ("file://",
                                 g_getenv (HILDON_HOME_ENV_MMC_MOUNTPOINT),
                                 NULL);
  if (g_str_has_prefix (uri, mmc_mount_point))
    {
      GError *gconf_error = NULL;

      image_from_mmc = TRUE;

      mmc_cover_open = gconf_client_get_bool (client,
                                              HILDON_HOME_GCONF_MMC_COVER_OPEN,
                                              &gconf_error);
      if (gconf_error)
        {
          g_set_error (error, BACKGROUND_MANAGER_ERROR,
                       BACKGROUND_MANAGER_ERROR_SYSTEM_RESOURCES,
                       "Unable to check key `%s' from GConf: %s",
                       HILDON_HOME_GCONF_MMC_COVER_OPEN,
                       gconf_error->message);

          g_error_free (gconf_error);
          g_object_unref (client);

          return NULL;
        }

      if (mmc_cover_open)
        {
          g_set_error (error, BACKGROUND_MANAGER_ERROR,
                       BACKGROUND_MANAGER_ERROR_MMC_OPEN,
                       "MMC cover is open");

          g_object_unref (client);

          return NULL;
        }
    }

  g_free (mmc_mount_point);

  result = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_READ);
  if (result != GNOME_VFS_OK)
    {
      g_set_error (error, BACKGROUND_MANAGER_ERROR,
                   BACKGROUND_MANAGER_ERROR_UNREADABLE,
                   "Unable to open `%s': %s",
                   uri,
                   gnome_vfs_result_to_string (result));

      g_object_unref (client);

      return NULL;
    }

#ifdef HAVE_LIBOSSO
  if (!oom_check &&
      osso_mem_saw_enable (3 << 20, 32767, load_image_oom_cb, (void *) &oom))
    {
      oom = TRUE;
    }
#endif

  loader = gdk_pixbuf_loader_new ();

  result = GNOME_VFS_OK;
  while (!oom && (result == GNOME_VFS_OK) && (!image_from_mmc || !mmc_cover_open))
    {
      GnomeVFSFileSize bytes_read;

      result = gnome_vfs_read (handle, buffer, BUFFER_SIZE, &bytes_read);
      if (result == GNOME_VFS_ERROR_IO)
        {
          gdk_pixbuf_loader_close (loader, NULL);
          gnome_vfs_close (handle);

          g_set_error (error, BACKGROUND_MANAGER_ERROR,
                       BACKGROUND_MANAGER_ERROR_IO,
                       "Unable to load `%s': read failed",
                       uri);
          g_debug ((*error)->message);

          g_object_unref (loader);
          g_object_unref (client);

          retval = NULL;
          break;
        }

      if ((result == GNOME_VFS_ERROR_EOF) || (bytes_read == 0))
        {
          g_debug ("Reached EOF of `%s', building the pixbuf", uri);

          gdk_pixbuf_loader_close (loader, NULL);
          gnome_vfs_close (handle);

          retval = gdk_pixbuf_loader_get_pixbuf (loader);
          if (!retval)
            {
              g_set_error (error, BACKGROUND_MANAGER_ERROR,
                           BACKGROUND_MANAGER_ERROR_CORRUPT,
                           "Unable to load `%s': loader failed",
                           uri);

              g_object_unref (loader);
              g_object_unref (client);

              retval = NULL;
              break;
            }
          else
            {
              GdkPixbufFormat *format;
              gchar *name;

              format = gdk_pixbuf_loader_get_format (loader);
              name = gdk_pixbuf_format_get_name (format);

              g_debug ("we got the pixbuf (w:%d, h:%d), format: %s",
                       gdk_pixbuf_get_width (retval),
                       gdk_pixbuf_get_height (retval),
                       name);

              g_free (name);

              g_object_ref (retval);

              g_object_unref (client);
              g_object_unref (loader);

              break;
            }
        }

      if (!oom)
        {
          GError *load_error = NULL;

          gdk_pixbuf_loader_write (loader, buffer, bytes_read, &load_error);
          if (load_error &&
              (load_error->domain == GDK_PIXBUF_ERROR) &&
              ((load_error->code == GDK_PIXBUF_ERROR_CORRUPT_IMAGE) ||
               (load_error->code == GDK_PIXBUF_ERROR_UNKNOWN_TYPE)))
            {
              g_set_error (error, BACKGROUND_MANAGER_ERROR,
                           BACKGROUND_MANAGER_ERROR_CORRUPT,
                           "Unable to load `%s': %s",
                           uri,
                           load_error->message);

              g_error_free (load_error);
              g_object_unref (client);

              retval = NULL;
              break;
            }
        }

      if (!oom && image_from_mmc)
        {
          GError *gconf_error = NULL;

          g_debug ("checking if the mmc cover has been opened");

          mmc_cover_open = gconf_client_get_bool (client,
                                                  HILDON_HOME_GCONF_MMC_COVER_OPEN,
                                                  &gconf_error);
          if (gconf_error)
            {
              g_set_error (error, BACKGROUND_MANAGER_ERROR,
                           BACKGROUND_MANAGER_ERROR_SYSTEM_RESOURCES,
                           "Unable to check key `%s' from GConf: %s",
                           HILDON_HOME_GCONF_MMC_COVER_OPEN,
                           gconf_error->message);

              g_error_free (gconf_error);
              gdk_pixbuf_loader_close (loader, NULL);
              gnome_vfs_close (handle);
              g_object_unref (client);

              retval = NULL;
              break;
            }

          if (mmc_cover_open)
            {
              g_set_error (error, BACKGROUND_MANAGER_ERROR,
                           BACKGROUND_MANAGER_ERROR_MMC_OPEN,
                           "MMC cover is open");

              gdk_pixbuf_loader_close (loader, NULL);
              gnome_vfs_close (handle);
              g_object_unref (client);

              retval = NULL;
              break;
            }
        }
    }

#ifdef HAVE_LIBOSSO
  if (oom_check)
    osso_mem_saw_disable ();
#endif

  return retval;
}

static GdkPixbuf *
load_image_from_file (const gchar  *filename,
                      gboolean cancellable,
                      GError      **error)
{
  gchar *filename_uri;
  GdkPixbuf *retval;
  GError *uri_error;
  GError *load_error;

  uri_error = NULL;
  filename_uri = g_filename_to_uri (filename, NULL, &uri_error);
  if (uri_error)
    {
      g_set_error (error, BACKGROUND_MANAGER_ERROR,
		   BACKGROUND_MANAGER_ERROR_UNKNOWN,
		   "Unable to convert `%s' to a valid URI: %s",
		   filename,
		   uri_error->message);
      g_error_free (uri_error);

      return NULL;
    }

  load_error = NULL;
  retval = load_image_from_uri (filename_uri, TRUE, cancellable, &load_error);
  if (load_error)
    {
      g_propagate_error (error, load_error);
    }

  g_free (filename_uri);

  return retval;
}


static GdkPixbuf *
create_background_from_color (const GdkColor  *src,
                              gint             width,
                              gint             height)
{
  GdkPixbuf *dest;
  guint32 color = 0;

  g_return_val_if_fail (src != NULL, NULL);

  color = (guint8) (src->red >> 8) << 24 |
	  (guint8) (src->green >> 8) << 16 |
	  (guint8) (src->blue >> 8) << 8 |
	  0xff;

  dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
  gdk_pixbuf_fill (dest, color);

  return dest;
}

static GdkPixbuf *
create_background_from_pixbuf (const GdkPixbuf  *src,
                               const GdkColor   *fill,
                               BackgroundMode    mode,
                               gint              width,
                               gint              height,
                               gint              top_offset,
                               gint              bottom_offset,
                               gint              left_offset,
                               gint              right_offset,
                               GError          **error)
{
  GdkPixbuf *dest = NULL;
  gint src_width, src_height;
  gint dest_x, dest_y;
  gint area_width, area_height;
  gdouble scaling_ratio;
  gdouble off_x, off_y;

  g_return_val_if_fail (src != NULL, NULL);
  g_return_val_if_fail (fill != NULL, NULL);

  dest = create_background_from_color (fill, width, height);
  if (!dest)
    {
      g_set_error (error, BACKGROUND_MANAGER_ERROR,
		   BACKGROUND_MANAGER_ERROR_SYSTEM_RESOURCES,
		   "Unable to create background color");

      return NULL;
    }

  src_width = gdk_pixbuf_get_width (src);
  src_height = gdk_pixbuf_get_height (src);
  area_width = width - left_offset - right_offset;
  area_height = height - top_offset - bottom_offset;

  g_debug ("*** background: (w:%d, h:%d), mode: %d",
           src_width,
           src_height,
           mode);

  if (src_width == area_width &&
      src_height == area_height)
    {
      gdk_pixbuf_composite (src,
                            dest,
                            0, 0,
                            width, height,
                            left_offset, top_offset,
                            1.0, 1.0,
                            GDK_INTERP_NEAREST,
                            0xFF);
      if (!dest)
        {
          g_set_error (error, BACKGROUND_MANAGER_ERROR,
                       BACKGROUND_MANAGER_ERROR_SYSTEM_RESOURCES,
                       "Unable to composite the background color with the image");

          return NULL;
        }

      g_debug ("*** We got a background pixbuf");

      return dest;
    }

  switch (mode)
    {
      case BACKGROUND_CENTERED:
          off_x = (area_width - src_width) / 2 + left_offset;
          off_y = (area_height - src_height) / 2 + top_offset;

          dest_x = MAX (left_offset, off_x);

          dest_y = MAX (top_offset, off_y);

          gdk_pixbuf_composite (src, dest,
                                dest_x, dest_y,
                                MIN (src_width, area_width),
                                MIN (src_height, area_height),
                                off_x, off_y,
                                1.0, 1.0,
                                GDK_INTERP_NEAREST,
                                HILDON_HOME_IMAGE_ALPHA_FULL);
          break;
      case BACKGROUND_SCALED:
          scaling_ratio = MIN (((gdouble) area_width / src_width),
                               (gdouble) area_height / src_height);
          dest_x = (gint) (MAX (left_offset,
                                left_offset +
                                (area_width
                                 - scaling_ratio
                                 * src_width) / 2));
          dest_y = (gint) (MAX (top_offset,
                                top_offset + 
                                 (area_height
                                 - scaling_ratio
                                 * src_height) / 2));

          gdk_pixbuf_composite (src, dest,
                                dest_x, dest_y,
                                scaling_ratio * src_width,
                                scaling_ratio * src_height,
                                dest_x,
                                dest_y,
                                scaling_ratio, scaling_ratio,
                                GDK_INTERP_NEAREST,
                                HILDON_HOME_IMAGE_ALPHA_FULL);
          break;
      case BACKGROUND_TILED:
          for (dest_x = left_offset;
               dest_x < width - right_offset;
               dest_x += src_width)
            {
              for (dest_y = top_offset;
                   dest_y < height - bottom_offset;
                   dest_y += src_height)
                {
                  gdk_pixbuf_composite (src, dest,
                                        dest_x, dest_y,
                                        MIN (src_width, width - dest_x),
                                        MIN (src_height, height - dest_y),
                                        dest_x, dest_y,
                                        1.0, 1.0,
                                        GDK_INTERP_NEAREST,
                                        HILDON_HOME_IMAGE_ALPHA_FULL);
                }
            }
          break;
      case BACKGROUND_STRETCHED:
          gdk_pixbuf_composite (src, dest,
                                left_offset, top_offset,
                                area_width, area_height,
                                left_offset, top_offset,
                                (gdouble) area_width  / src_width,
                                (gdouble) area_height / src_height,
                                GDK_INTERP_NEAREST,
                                HILDON_HOME_IMAGE_ALPHA_FULL);
          break;
      default:
          g_assert_not_reached ();
          break;
    }

  return dest;
}

/* We create the cached pixbuf compositing the sidebar and the titlebar
 * pixbufs from their relative files; we use a child process to retain
 * some interactivity; we use a pipe to move the error messages from
 * the child process to the background manager. the child process saves
 * the composed image to the cache file and we read it inside the
 * child notification callback
 */
static GdkPixbuf *
composite_background (const GdkPixbuf  *bg_image,
                      const GdkColor   *bg_color,
                      BackgroundMode    mode,
                      const gchar      *sidebar_path,
                      const gchar      *titlebar_path,
                      gint              window_width,
                      gint              window_height,
                      gboolean          cancellable,
                      gint              top_offset,
                      gint              bottom_offset,
                      gint              left_offset,
                      gint              right_offset,
                      GError          **error)
{
  GError *bg_error;
  GdkPixbuf *pixbuf;
  GdkPixbuf *compose;
  gint titlebar_height = 0;

  g_debug ("Compositing background image...");

  bg_error = NULL;

  if (bg_image)
    {
      pixbuf = create_background_from_pixbuf (bg_image,
                                              bg_color,
                                              mode,
                                              window_width,
                                              window_height,
                                              top_offset,
                                              bottom_offset,
                                              left_offset,
                                              right_offset,
                                              &bg_error);
    }
  else
    {
      pixbuf = create_background_from_color (bg_color,
                                             window_width,
                                             window_height
                                            );

      g_return_val_if_fail (pixbuf, NULL);
    }

  if (bg_error)
    {
      g_propagate_error (error, bg_error);

      return NULL;
    }

#if 0
  if (titlebar_path && *titlebar_path)
    {
      compose = load_image_from_file (titlebar_path, cancellable, &bg_error);

      titlebar_height = gdk_pixbuf_get_height (compose);

      if (bg_error)
        {
          g_warning ("Unable to load titlebar pixbuf: %s", bg_error->message);

          g_error_free (bg_error);
          bg_error = NULL;
        }
      else if (!compose)
        {
          g_debug ("Assuming loading of titlebar cancelled");
          if (pixbuf)
            g_object_unref (pixbuf);
          return NULL;
        }
      else
        {
          g_debug ("Compositing titlebar");
          /* Scale it horizontally */
          double x_scale;

          x_scale = (double)(window_width - left_offset - right_offset) /
                    gdk_pixbuf_get_width (compose);

          gdk_pixbuf_composite (compose,
                                pixbuf,
                                left_offset, top_offset,
                                window_width - left_offset - right_offset,
                                gdk_pixbuf_get_height (compose),
                                left_offset, top_offset,
                                x_scale,
                                1.0,
                                GDK_INTERP_NEAREST,
                                HILDON_HOME_IMAGE_ALPHA_FULL);

          g_object_unref (compose);
          compose = NULL;
        }
    }
#endif

  if (sidebar_path && *sidebar_path)
    {

      compose = load_image_from_file (sidebar_path, cancellable, &bg_error);
      if (bg_error)
        {
          g_warning ("Unable to load sidebar pixbuf: %s", bg_error->message);

          g_error_free (bg_error);
          bg_error = NULL;
        }
      else if (!compose)
        {
          g_debug ("Assuming loading of sidebar cancelled");
          if (pixbuf)
            g_object_unref (pixbuf);
          return NULL;
        }
      else
        {
          gint width = gdk_pixbuf_get_width (compose);
          gint height = gdk_pixbuf_get_height (compose);
          gint sidebar_height;

          g_debug ("Compositing sidebar (w:%d, h:%d)",
                   width, height);

          sidebar_height = window_height - titlebar_height;
          if (height != sidebar_height)
            {
              GdkPixbuf *scaled;
              gint i;

              scaled = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                                       width, sidebar_height);
              for (i = 0; i < (sidebar_height - height); i += height)
                {
                  gdk_pixbuf_copy_area (compose,
                                        0, 0,
                                        width, height,
                                        scaled,
                                        0, i + height);
                }

              g_object_unref (compose);
              compose = scaled;
            }

          gdk_pixbuf_composite (compose,
                                pixbuf,
                                left_offset,
                                top_offset + titlebar_height,
                                gdk_pixbuf_get_width (compose),
                                gdk_pixbuf_get_height (compose),
                                left_offset,
                                top_offset,
                                1.0, 1.0,
                                GDK_INTERP_NEAREST,
                                HILDON_HOME_IMAGE_ALPHA_FULL);

          g_object_unref (compose);
          compose = NULL;
        }
    }

  return pixbuf;
}


static void
background_manager_class_init (BackgroundManagerClass *klass)
{
  GObjectClass     *gobject_class = G_OBJECT_CLASS (klass);
  GError           *error = NULL;

  gobject_class->set_property = background_manager_set_property;
  gobject_class->get_property = background_manager_get_property;

  klass->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (error)
    {
      klass->connection = NULL;
      g_warning ("Could not get DBus connection");
      g_error_free (error);
      return;
    }

  dbus_g_object_type_install_info (TYPE_BACKGROUND_MANAGER,
                                   &dbus_glib_background_manager_object_info);
}

static void
background_manager_init (BackgroundManager *manager)
{
  BackgroundManagerClass       *klass;

  klass = BACKGROUND_MANAGER_GET_CLASS (manager);
  if (klass->connection)
    dbus_g_connection_register_g_object (klass->connection,
                                         HILDON_BACKGROUND_MANAGER_OBJECT_PATH,
                                         G_OBJECT (manager));

}

/* RPC method */
gboolean
background_manager_set_background (BackgroundManager   *manager,
                                   gint                 window_xid,
                                   const gchar         *filename,
                                   const gchar         *top_bar,
                                   const gchar         *left_bar,
                                   guint16              red,
                                   guint16              green,
                                   guint16              blue,
                                   BackgroundMode       mode,
                                   gint32               top_offset,
                                   gint32               bottom_offset,
                                   gint32               left_offset,
                                   gint32               right_offset,
                                   gint                *pixmap_xid,
                                   GError             **error)
{
  GdkDisplay               *display;
  GdkColor                  color;
  GdkWindow                *window;
  GdkPixbuf                *image = NULL;
  GdkPixbuf                *background = NULL;
  GdkColormap              *colormap;
  GdkPixmap                *pixmap  = NULL;
/*  GdkBitmap                *bitmask = NULL;*/
  GError                   *local_error = NULL;
  gint                      width, height;
  const gchar              *display_name;
  GdkGC                    *gc;
  /*XImage                   *ximage;*/

  g_debug ("set_background on %s", filename);

  display_name = g_getenv ("DISPLAY");
  if (!display_name)
    {
      g_set_error (error,
                   background_manager_error_quark (),
                   BACKGROUND_MANAGER_ERROR_DISPLAY,
                   "Could not open display");
      return FALSE;
    }

  display = gdk_display_open (display_name);
  if (!display)
    {
      g_set_error (error,
                   background_manager_error_quark (),
                   BACKGROUND_MANAGER_ERROR_DISPLAY,
                   "Could not open display %s",
                   display_name);
      return FALSE;
    }
  
  window = gdk_window_foreign_new_for_display (display, window_xid);

  if (!window)
    {
      g_set_error (error,
                   background_manager_error_quark (),
                   BACKGROUND_MANAGER_ERROR_WINDOW,
                   "Window %x not found",
                   window_xid);
      return FALSE;
    }
  
  color.red   = red;
  color.blue  = blue;
  color.green = green;

  if (filename && filename[0])
    {
      image = load_image_from_uri (filename,
                                   TRUE, 
                                   TRUE,
                                   &local_error);
      if (local_error)
        {
          /* We will try to recover by using a plain color */
          g_warning ("Error when loading uri %s: %s",
                     filename,
                     local_error->message);

          g_clear_error (&local_error);
          image = NULL;
          /*
          g_propagate_error (error, local_error);
          return FALSE;
          */
        }
    }

  gdk_drawable_get_size (GDK_DRAWABLE (window), &width, &height);

  background = composite_background (image,
                                     &color,
                                     mode,
                                     top_bar,
                                     left_bar,
                                     width,
                                     height,
                                     TRUE,
                                     top_offset,
                                     bottom_offset,
                                     left_offset,
                                     right_offset,
                                     &local_error);

  if (image)
    g_object_unref (image);

  if (local_error)
    {
      g_propagate_error (error, local_error);
      return FALSE;
    }
  
  colormap = gdk_drawable_get_colormap (GDK_DRAWABLE (window));

#if 0
  gdk_pixbuf_render_pixmap_and_mask_for_colormap (background,
                                                  colormap,
                                                  &pixmap,
                                                  &bitmask,
                                                  0);
#endif
  gc = gdk_gc_new (GDK_DRAWABLE (window));
  gdk_flush ();
  pixmap = gdk_pixmap_new (GDK_DRAWABLE (window),
                           width,
                           height,
                           -1);
                           /*gdk_colormap_get_visual (colormap)->depth);*/
  gdk_flush ();

#if 0

  g_debug ("Window depth is %i", gdk_drawable_get_depth (GDK_DRAWABLE (window)));
  g_debug ("bixbuf has %i bits per color", gdk_pixbuf_get_bits_per_sample (background);

  ximage = XCreateImage (GDK_DISPLAY_XDISPLAY(display),
                         GDK_VISUAL_XVISUAL (gdk_drawable_get_visual (GDK_DRAWABLE (window))),
                         32,
/*                       gdk_drawable_get_depth (GDK_DRAWABLE (window))*/
                         ZPixmap,
                         0,
                         gdk_pixbuf_get_pixels (background),
                         width,
                         height,
                         32,
                         4*width);

  XPutImage (GDK_DISPLAY_XDISPLAY(display),
             GDK_PIXMAP_XID (pixmap),
             GDK_GC_XGC (gc),
             ximage,
             0,
             0,
             0,
             0,
             width,
             height);

  XFree (ximage);
#else



  gdk_drawable_set_colormap (GDK_DRAWABLE (pixmap), colormap);
  gdk_flush ();
  gdk_draw_pixbuf (pixmap, gc, background, 
                   0, 0, 0, 0,
                   gdk_pixbuf_get_width (background),
                   gdk_pixbuf_get_height (background),
                   GDK_RGB_DITHER_NORMAL,
                   0, 0);
  gdk_flush ();
  g_object_unref (gc);
#endif

  if (pixmap)
    {
      if (pixmap_xid)
        {
          XSetCloseDownMode (GDK_DISPLAY_XDISPLAY(display), RetainTemporary);
          *pixmap_xid = (gint)GDK_PIXMAP_XID (pixmap);
        }
      else
        {
          gdk_window_set_back_pixmap (window, pixmap, FALSE);
          g_object_unref (pixmap);
        }
    }

  gdk_display_close (display);

  if (background)
    g_object_unref (background);

  gdk_window_clear (window);
  gdk_flush ();

  g_idle_add ((GSourceFunc)g_main_loop_quit, main_loop);
  return TRUE;
}
