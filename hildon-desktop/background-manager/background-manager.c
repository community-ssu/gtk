/* -*- mode:C; c-file-style:"gnu"; -*- */
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
#include "config.h"
#endif
#include "background-manager.h"
#include "background-manager-glue.h"

#include <gtk/gtkmain.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkx.h>

#include <libhildondesktop/hildon-desktop-picture.h>
#include <X11/extensions/Xrender.h>

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
        { BACKGROUND_CROPPED, "BACKGROUND_CROPPED", "cropped" },
        { 0, NULL, NULL }
      };

      etype = g_enum_register_static ("BackgroundMode", values);
    }

  return etype;
}

G_DEFINE_TYPE (BackgroundManager, background_manager, G_TYPE_OBJECT);

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

static Picture
create_background_from_color (GdkDisplay       *display,
                              const GdkColor   *src,
                              gint              width,
                              gint              height)
{
  Picture                       picture;
  XRenderPictFormat            *format;
  XRenderPictureAttributes      pa = {0};
  XRenderColor                  color = {src->red,
                                         src->green,
                                         src->blue,
                                         0xFFFF};
  Pixmap                        pixmap;
  Display                      *xdisplay = GDK_DISPLAY_XDISPLAY (display);

  pixmap = XCreatePixmap (xdisplay,
                          DefaultRootWindow (xdisplay),
                          width,
                          height,
                          24);

  pa.repeat = True;

  format = XRenderFindStandardFormat (xdisplay, PictStandardRGB24);
  picture = XRenderCreatePicture (xdisplay,
                                  pixmap,
                                  format,
                                  CPRepeat,
                                  &pa);

  XRenderFillRectangle (xdisplay,
                        PictOpSrc,
                        picture,
                        &color,
                        0, 0,
                        width, height);

  XFreePixmap (xdisplay, pixmap);

  return picture;
}

static Picture
create_background_from_pixbuf (GdkDisplay       *display,
                               const GdkPixbuf  *pixbuf,
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
  Display *xdisplay = GDK_DISPLAY_XDISPLAY (display);
  Picture picture = None;
  Picture image_picture = None;
  Picture image_mask = None;
  gint src_width, src_height;
  gint area_width, area_height;
  gint off_x, off_y;
  gint dest_x, dest_y;

  picture = create_background_from_color (display, fill, width, height);

  if (picture == None)
    {
      g_set_error (error, BACKGROUND_MANAGER_ERROR,
		   BACKGROUND_MANAGER_ERROR_SYSTEM_RESOURCES,
		   "Unable to create background color");

      return None;
    }

  hildon_desktop_picture_and_mask_from_pixbuf (display,
                                               pixbuf,
                                               &image_picture,
                                               &image_mask);


  src_width  = gdk_pixbuf_get_width  (pixbuf);
  src_height = gdk_pixbuf_get_height (pixbuf);
  area_width = width - left_offset - right_offset;
  area_height = height - top_offset - bottom_offset;

  if (src_width == area_width &&
      src_height == area_height)
    {
      XRenderComposite (xdisplay,
                        PictOpOver,
                        image_picture,
                        image_mask,
                        picture,
                        0, 0,
                        0, 0,
                        left_offset, top_offset,
                        src_width, src_height);

      if (image_picture != None)
        XRenderFreePicture (xdisplay, image_picture);

      if (image_mask != None)
        XRenderFreePicture (xdisplay, image_mask);
      return picture;
    }

  switch (mode)
    {
      case BACKGROUND_CENTERED:
          off_x = (area_width - src_width) / 2 + left_offset;
          off_y = (area_height - src_height) / 2 + top_offset;

          dest_x = MAX (left_offset, off_x);

          dest_y = MAX (top_offset, off_y);

          XRenderComposite (xdisplay,
                            PictOpOver,
                            image_picture,
                            image_mask,
                            picture,
                            0, 0,
                            0, 0,
                            off_x, off_y,
                            src_width, src_height);

          break;
      case BACKGROUND_SCALED:
        {
          gdouble scaling_ratio = MAX ((gdouble) src_width  / area_width,
                                       (gdouble) src_height / area_height);
          gint dest_width  = (gdouble)src_width  / scaling_ratio;
          gint dest_height = (gdouble)src_height / scaling_ratio;
          XTransform transform = {{{XDoubleToFixed (scaling_ratio), 0, 0 },
                                   {0, XDoubleToFixed (scaling_ratio), 0 },
                                   {0, 0, XDoubleToFixed (1.0)}}};

          dest_x = (gint) (left_offset + (area_width  - dest_width)  / 2);
          dest_y = (gint) (top_offset  + (area_height - dest_height) / 2);

          XRenderSetPictureTransform (xdisplay,
                                      image_picture,
                                      &transform);
          if (image_mask != None)
            XRenderSetPictureTransform (xdisplay,
                                        image_mask,
                                        &transform);

          XRenderComposite (xdisplay,
                            PictOpOver,
                            image_picture,
                            image_mask,
                            picture,
                            0, 0,
                            0, 0,
                            dest_x, dest_y,
                            dest_width, dest_height);
          break;
        }


      case BACKGROUND_TILED:
        {
          XRenderComposite (xdisplay,
                            PictOpOver,
                            image_picture,
                            image_mask,
                            picture,
                            0, 0,
                            0, 0,
                            0, 0,
                            area_width, area_height);
          break;

        }
      case BACKGROUND_STRETCHED:
        {
          XTransform transform =
            {{{XDoubleToFixed ((gdouble)src_width/area_width), 0, 0 },
              {0, XDoubleToFixed ((gdouble)src_height/area_height), 0 },
              {0, 0, XDoubleToFixed (1.0)}}};

          XRenderSetPictureTransform (xdisplay,
                                      image_picture,
                                      &transform);
          if (image_mask != None)
            XRenderSetPictureTransform (xdisplay,
                                        image_mask,
                                        &transform);

          XRenderComposite (xdisplay,
                            PictOpOver,
                            image_picture,
                            image_mask,
                            picture,
                            0, 0,
                            0, 0,
                            0, 0,
                            area_width, area_height);

          break;
        }
      case BACKGROUND_CROPPED:
        {
          gdouble scaling_ratio = (src_width < src_height)?
                                        (gdouble) src_width  / area_width:
                                        (gdouble) src_height / area_height;
          gint dest_width  = (gdouble)src_width  / scaling_ratio;
          gint dest_height = (gdouble)src_height / scaling_ratio;
          XTransform transform = {{{XDoubleToFixed (scaling_ratio), 0, 0 },
                                   {0, XDoubleToFixed (scaling_ratio), 0 },
                                   {0, 0, XDoubleToFixed (1.0)}}};

          dest_x = (gint) (left_offset + (area_width  - dest_width) / 2);
          dest_y = (gint) (top_offset  + (area_height - dest_height) / 2);

          g_debug ("Got dest: %i, %i", dest_x, dest_y);

          XRenderSetPictureTransform (xdisplay,
                                      image_picture,
                                      &transform);
          if (image_mask != None)
            XRenderSetPictureTransform (xdisplay,
                                        image_mask,
                                        &transform);

          XRenderComposite (xdisplay,
                            PictOpOver,
                            image_picture,
                            image_mask,
                            picture,
                            0, 0,
                            0, 0,
                            dest_x, dest_y,
                            dest_width, dest_height);

          break;
        }
    }

  return picture;
}

static Picture
composite_background (GdkDisplay       *display,
                      const GdkPixbuf  *bg_image,
                      const GdkColor   *bg_color,
                      BackgroundMode    mode,
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
  Picture       picture;

  bg_error = NULL;

  if (bg_image)
    {
      picture = create_background_from_pixbuf (display,
                                               bg_image,
                                               bg_color,
                                               mode,
                                               window_width,
                                               window_height,
                                               0, 0, 0, 0,
                                               /*
                                                  top_offset,
                                                  bottom_offset,
                                                  left_offset,
                                                  right_offset,
                                                  */
                                               &bg_error);
    }
  else
    {
      picture = create_background_from_color (display,
                                              bg_color,
                                              window_width,
                                              window_height
                                             );

    }

  if (bg_error)
    {
      g_propagate_error (error, bg_error);

      return None;
    }

  return picture;
}


static void
background_manager_class_init (BackgroundManagerClass *klass)
{
  GError           *error = NULL;

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
                                   guint16              red,
                                   guint16              green,
                                   guint16              blue,
                                   BackgroundMode       mode,
                                   gint32               top_offset,
                                   gint32               bottom_offset,
                                   gint32               left_offset,
                                   gint32               right_offset,
                                   gint                *picture_xid,
                                   GError             **error)
{
  GdkDisplay               *display;
  GdkColor                  color;
  GdkWindow                *window;
  GdkPixbuf                *image = NULL;
  GError                   *local_error = NULL;
  gint                      width, height;
  const gchar              *display_name;

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
          g_propagate_error (error, local_error);
          return FALSE;
        }
    }

  gdk_drawable_get_size (GDK_DRAWABLE (window), &width, &height);

  *picture_xid = composite_background (display,
                                       image,
                                       &color,
                                       mode,
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

  XSetCloseDownMode (GDK_DISPLAY_XDISPLAY(display), RetainTemporary);
  gdk_flush ();

  gdk_display_close (display);

  g_idle_add ((GSourceFunc)g_main_loop_quit, main_loop);
  return TRUE;
}
