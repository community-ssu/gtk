/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of maemo-af-desktop
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
#include <config.h>
#endif

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <libintl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libosso.h>
#include <libmb/mbutil.h>
#include <glob.h>
#include <osso-log.h>
#include <osso-helplib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/*SAW (allocation watchdog facilities)*/
#include <osso-mem.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs.h>

#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-color-button.h>

#include "hildon-home-common.h"
#include "hildon-home-private.h"
#include "background-manager.h"

#define HILDON_HOME_IMAGE_FORMAT           "png"
#define HILDON_HOME_IMAGE_ALPHA_FULL       255
#define HILDON_HOME_GCONF_MMC_COVER_OPEN   "/system/osso/af/mmc-cover-open"
#define HILDON_HOME_WINDOW_WIDTH	   800
#define HILDON_HOME_WINDOW_HEIGHT          480
#define HILDON_HOME_TASKNAV_WIDTH          80
#define HILDON_HOME_TITLEBAR_HEIGHT        60

#define HILDON_BACKGROUND_MANAGER (background_manager_quark ())

static GQuark
background_manager_quark (void)
{
  return g_quark_from_static_string ("hildon-background-manager");
}


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
        { BACKGROUND_TILED, "BACKGROUND_TILED", "tiled" },
        { BACKGROUND_STRETCHED, "BACKGROUND_STRETCHED", "stretched" },
        { 0, NULL, NULL }
      };
      
      etype = g_enum_register_static ("BackgroundMode", values);
    }

  return etype;
}

#define BACKGROUND_MANAGER_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_BACKGROUND_MANAGER, BackgroundManagerPrivate))

struct _BackgroundManagerPrivate
{
  gchar *image_uri;
  BackgroundMode mode;
  GdkColor color;

  /* these are local files */
  gchar *titlebar;
  gchar *sidebar;
  gchar *cache;

  GPid child_pid;

  guint is_screen_singleton : 1;
  GdkScreen *screen;

  guint       bg_timeout;
  GtkWidget * loading_note;
};

enum
{
  PROP_0,

  PROP_IMAGE_URI,
  PROP_MODE,
  PROP_COLOR
};

enum
{
  CHANGED,

  LAST_SIGNAL
};

static guint manager_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BackgroundManager, background_manager, G_TYPE_OBJECT);

static void
background_manager_set_property (GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_IMAGE_URI:
      break;
    case PROP_MODE:
      break;
    case PROP_COLOR:
      break;
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
  BackgroundManager *manager = BACKGROUND_MANAGER (object);

  switch (prop_id)
    {
    case PROP_IMAGE_URI:
      g_value_set_string (value, manager->priv->image_uri);
      break;
    case PROP_MODE:
      g_value_set_enum (value, manager->priv->mode);
      break;
    case PROP_COLOR:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
background_manager_finalize (GObject *object)
{
  BackgroundManager *manager = BACKGROUND_MANAGER (object);
  BackgroundManagerPrivate *priv = manager->priv;

  g_free (priv->image_uri);
  
  g_free (priv->cache);
  g_free (priv->titlebar);
  g_free (priv->sidebar);
  
  G_OBJECT_CLASS (background_manager_parent_class)->finalize (object);
}

static void
background_manager_write_configuration (BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv;
  GKeyFile *conf;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  gchar *data;
  gsize len;
  GError *error;

  priv = manager->priv;
  
  /* dump the state of the background to the config file */
  conf = g_key_file_new ();
  
  /* images locations */
  if (priv->image_uri)
    g_key_file_set_string (conf, HILDON_HOME_CONF_MAIN_GROUP,
			   HILDON_HOME_CONF_BG_URI_KEY,
			   priv->image_uri);
  else
    g_key_file_set_string (conf, HILDON_HOME_CONF_MAIN_GROUP,
			   HILDON_HOME_CONF_BG_URI_KEY,
			   HILDON_HOME_BACKGROUND_NO_IMAGE);
    
  g_key_file_set_string (conf, HILDON_HOME_CONF_MAIN_GROUP,
		         HILDON_HOME_CONF_SIDEBAR_KEY,
			 priv->sidebar);
  g_key_file_set_string (conf, HILDON_HOME_CONF_MAIN_GROUP,
		         HILDON_HOME_CONF_TITLEBAR_KEY,
			 priv->titlebar);
  
  /* background color, by component */
  g_key_file_set_integer (conf, HILDON_HOME_CONF_MAIN_GROUP,
		          HILDON_HOME_CONF_BG_COLOR_RED_KEY,
		          priv->color.red);
  g_key_file_set_integer (conf, HILDON_HOME_CONF_MAIN_GROUP,
		  	  HILDON_HOME_CONF_BG_COLOR_GREEN_KEY,
			  priv->color.green);
  g_key_file_set_integer (conf, HILDON_HOME_CONF_MAIN_GROUP,
		          HILDON_HOME_CONF_BG_COLOR_BLUE_KEY,
			  priv->color.blue);

  /* background mode, by symbolic name */
  enum_class = g_type_class_ref (TYPE_BACKGROUND_MODE);
  enum_value = g_enum_get_value (enum_class, priv->mode);
  g_key_file_set_string (conf, HILDON_HOME_CONF_MAIN_GROUP,
		         HILDON_HOME_CONF_BG_MODE_KEY,
			 enum_value->value_nick);
  g_type_class_unref (enum_class);

  error = NULL;
  data = g_key_file_to_data (conf, &len, &error);
  if (error)
    {
      g_warning ("Error while saving configuration: %s",
		 error->message);
      
      g_error_free (error);
      g_key_file_free (conf);

      return;
    }

  error = NULL;
  g_file_set_contents (hildon_home_get_user_config_file (),
		       data,
		       len,
		       &error);
  if (error)
    {
      g_warning ("Error while saving configuration to file `%s': %s",
		 hildon_home_get_user_config_file (),
		 error->message);

      g_error_free (error);
      g_key_file_free (conf);
      g_free (data);

      return;
    }

  g_free (data);
  g_key_file_free (conf);
}

static void
background_manager_read_configuration (BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv = manager->priv;
  GKeyFile *conf;
  GError *parse_error;
  gchar *mode;
  GEnumValue *enum_value;
  GEnumClass *enum_class;
  
  conf = g_key_file_new ();

  parse_error = NULL;
  g_key_file_load_from_file (conf,
		             hildon_home_get_user_config_file (),
		  	     0,
			     &parse_error);
  if (parse_error)
    {
      g_warning ("Unable to read configuration file at `%s': %s",
		 hildon_home_get_user_config_file (),
		 parse_error->message);

      g_error_free (parse_error);
      g_key_file_free (conf);

      return;
    }

  parse_error = NULL;
  
  g_free (priv->image_uri);
  priv->image_uri = g_key_file_get_string (conf,
		  			   HILDON_HOME_CONF_MAIN_GROUP,
					   HILDON_HOME_CONF_BG_URI_KEY,
					   &parse_error);
  if (parse_error)
    {
      priv->image_uri = g_strdup_printf ("file://%s",
		                         hildon_home_get_user_bg_file ());
      
      g_error_free (parse_error);
      parse_error = NULL;
    }
  else if (g_str_equal (priv->image_uri, HILDON_HOME_BACKGROUND_NO_IMAGE))
    {
      g_free (priv->image_uri);
      priv->image_uri = NULL;
    }
  
  priv->color.red = g_key_file_get_integer (conf,
					    HILDON_HOME_CONF_MAIN_GROUP,
					    HILDON_HOME_CONF_BG_COLOR_RED_KEY,
					    &parse_error);
  if (parse_error)
    {
      g_warning ("failed read color.red");
      priv->color.red = 0;

      g_error_free (parse_error);
      parse_error = NULL;
    }

  priv->color.green =
    g_key_file_get_integer (conf,
			    HILDON_HOME_CONF_MAIN_GROUP,
			    HILDON_HOME_CONF_BG_COLOR_GREEN_KEY,
			    &parse_error);
  if (parse_error)
    {
      g_warning ("failed read color.green");
      priv->color.green = 0;

      g_error_free (parse_error);
      parse_error = NULL;
    }

  priv->color.blue =
    g_key_file_get_integer (conf,
			    HILDON_HOME_CONF_MAIN_GROUP,
			    HILDON_HOME_CONF_BG_COLOR_BLUE_KEY,
			    &parse_error);
  if (parse_error)
    {
      g_warning ("failed read color.blue");
      priv->color.blue = 0;

      g_error_free (parse_error);
      parse_error = NULL;
    }
  
  mode = g_key_file_get_string (conf,
		  		HILDON_HOME_CONF_MAIN_GROUP,
				HILDON_HOME_CONF_BG_MODE_KEY,
				&parse_error);
  if (parse_error)
    {
      mode = g_strdup ("centered");

      g_error_free (parse_error);
      parse_error = NULL;
    }

  enum_class = g_type_class_ref (TYPE_BACKGROUND_MODE);
  enum_value = g_enum_get_value_by_nick (enum_class, mode);
  priv->mode = enum_value->value;

  g_free (mode);

  g_key_file_free (conf);
}

static void
load_image_oom_cb (size_t  current_size,
		   size_t  max_size,
		   void   *context)
{
  *(gboolean *) context = TRUE;
}

#define BUFFER_SIZE	8192

static GdkPixbuf *
load_image_from_uri (const gchar  *uri,
		     gboolean      post_install,
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

  mmc_mount_point = g_strdup_printf ("file://%s",
		                     g_getenv (HILDON_HOME_ENV_MMC_MOUNTPOINT));
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

  if (!post_install &&
      osso_mem_saw_enable (3 << 20, 32767, load_image_oom_cb, (void *) &oom))
    {
      oom = TRUE;
    }

  loader = gdk_pixbuf_loader_new ();

  result = GNOME_VFS_OK;
  while (!oom && (result == GNOME_VFS_OK) && (!image_from_mmc || !mmc_cover_open))
    {
      GnomeVFSFileSize bytes_read;

      result = gnome_vfs_read (handle, buffer, BUFFER_SIZE, &bytes_read);
      if ((result == GNOME_VFS_ERROR_EOF) || (bytes_read == 0))
        {
          g_warning ("Reached EOF of `%s', building the pixbuf", uri);
	  
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
	      gchar * name
		= gdk_pixbuf_format_get_name (
				      gdk_pixbuf_loader_get_format (loader));
	      
	      g_warning ("we got the pixbuf (%d x %d), type: %s",
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

  if (post_install)
    osso_mem_saw_disable ();

  return retval;
}

static GdkPixbuf *
load_image_from_file (const gchar  *filename,
		      GError      **error)
{
  gchar *filename_uri;
  GdkPixbuf *retval;
  GError *load_error;

  filename_uri = g_filename_to_uri (filename, NULL, NULL);
  if (!filename_uri)
    {
      g_set_error (error, BACKGROUND_MANAGER_ERROR,
		   BACKGROUND_MANAGER_ERROR_UNKNOWN,
		   "Unable to convert `%s' to a valid URI",
		   filename);

      return NULL;
    }

  load_error = NULL;
  retval = load_image_from_uri (filename_uri, TRUE, &load_error);
  if (load_error)
    {
      g_propagate_error (error, load_error);
    }

  g_free (filename_uri);

  return retval;
}

static gboolean
save_image_to_file (GdkPixbuf    *src,
		    const gchar  *filename,
		    GError      **error)
{
  GError *save_error;
  gchar *data;
  gsize length;

  g_return_val_if_fail (src != NULL, FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  
  save_error = NULL;
  gdk_pixbuf_save_to_buffer (src, &data, &length,
		             HILDON_HOME_IMAGE_FORMAT,
			     &save_error,
			     NULL);
  if (save_error)
    {
      g_propagate_error (error, save_error);

      return FALSE;
    }

  save_error = NULL;
  g_file_set_contents (filename, data, length, &save_error);
  if (save_error)
    {
      if (save_error->domain == G_FILE_ERROR &&
          save_error->code == G_FILE_ERROR_NOSPC)
        {
	  g_set_error (error, BACKGROUND_MANAGER_ERROR,
		       BACKGROUND_MANAGER_ERROR_FLASH_FULL,
		       "Flash storage is full");
	}
      else
        g_propagate_error (error, save_error);

      g_free (data);

      return FALSE;
    }

  g_free (data);

  return TRUE;
}

static GdkPixbuf *
create_background_from_color (const GdkColor  *src,
			      gint             width,
			      gint             height)
{
  GdkPixbuf *dest;
  guint32 color = 0;

  g_return_val_if_fail (src != NULL, NULL);

  if (width < 0)
    width = HILDON_HOME_WINDOW_WIDTH;

  if (height < 0)
    height = HILDON_HOME_WINDOW_HEIGHT;

  color = (guint8) (src->red >> 8) << 24 |
	  (guint8) (src->green >> 8) << 16 |
	  (guint8) (src->blue >> 8) << 8 |
	  0xff;

  dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
  gdk_pixbuf_fill (dest, color);

  return dest;
}

/* this gets rather convoluted because we want to create a background
 * pixbuf of the same size of the screen and we must account the white
 * space left for the task navigator; so, we must shift the image of
 * HILDON_HOME_TASKNAV_WIDTH pixels on the x axis.
 */
static GdkPixbuf *
create_background_from_pixbuf (const GdkPixbuf  *src,
			       const GdkColor   *fill,
			       BackgroundMode    mode,
			       gint              width,
			       gint              height,
			       GError          **error)
{
  GdkPixbuf *dest = NULL;
  gint src_width, src_height;
  gint dest_x, dest_y;
  gdouble scaling_ratio;
  gdouble off_x, off_y;

  g_return_val_if_fail (src != NULL, NULL);
  g_return_val_if_fail (fill != NULL, NULL);

  if (width < 0)
    width = HILDON_HOME_WINDOW_WIDTH;

  if (height < 0)
    height = HILDON_HOME_WINDOW_HEIGHT;

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

  g_debug ("*** background: (w:%d, h:%d), mode: %d",
	   src_width,
	   src_height,
	   mode);

  if (src_width == (width - HILDON_HOME_TASKNAV_WIDTH) &&
      src_height == height)
    {
      gdk_pixbuf_composite (src,
		            dest,
			    0, 0,
			    width, height,
			    HILDON_HOME_TASKNAV_WIDTH, 0,
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
      if (src_width < (width - HILDON_HOME_TASKNAV_WIDTH))
        {
          dest_x = MAX (HILDON_HOME_TASKNAV_WIDTH, (width - src_width) / 2);
          off_x = MAX (0.0, (width - src_width) / 2);
	}
      else
	{
	  dest_x = MAX (HILDON_HOME_TASKNAV_WIDTH, (width - src_width) / 2);
          off_x = MAX (0.0, (src_width - width) / 2);
	}

      if (src_height < height)
        {
          dest_y = MAX (0, (height - src_height) / 2);
          off_y = MAX (0.0, (height - src_height) / 2);
	}
      else
        {
	  dest_y = MAX (0, (height - src_height) / 2);
	  off_y = MAX (0.0, (src_height - height) / 2);
	}
      
      gdk_pixbuf_composite (src, dest,
		            dest_x, dest_y,
		            MIN (src_width, width - HILDON_HOME_TASKNAV_WIDTH),
			    MIN (src_height, height),
			    off_x, off_y,
			    1.0, 1.0,
			    GDK_INTERP_NEAREST,
			    HILDON_HOME_IMAGE_ALPHA_FULL);
      break;
    case BACKGROUND_SCALED:
      scaling_ratio = MIN ((gdouble) ((width - HILDON_HOME_TASKNAV_WIDTH) / src_width),
		           (gdouble) height / src_height);
      dest_x = (gint) (MAX (HILDON_HOME_TASKNAV_WIDTH,
			    (width
			     - HILDON_HOME_TASKNAV_WIDTH
			     - scaling_ratio
			     * src_width) / 2));
      dest_y = (gint) (MAX (0,
			    (height
			     - scaling_ratio
			     * src_height) / 2));

      gdk_pixbuf_composite (src, dest,
			    dest_x, dest_y,
		            scaling_ratio * src_width,
			    scaling_ratio * src_height,
			    MAX (HILDON_HOME_TASKNAV_WIDTH,
				 (width
				  - HILDON_HOME_TASKNAV_WIDTH
				  - scaling_ratio
				  * src_width) / 2),
			    MAX (0, 
				 (height
				  - scaling_ratio
				  * src_height) / 2),
			    scaling_ratio, scaling_ratio,
			    GDK_INTERP_NEAREST,
			    HILDON_HOME_IMAGE_ALPHA_FULL);
      break;
    case BACKGROUND_TILED:
        for (dest_x = HILDON_HOME_TASKNAV_WIDTH;
	     dest_x < (width - HILDON_HOME_TASKNAV_WIDTH);
	     dest_x += src_width)
          {
            for (dest_y = 0;
	         dest_y < height;
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
	      		    HILDON_HOME_TASKNAV_WIDTH, 0,
			    width, height,
			    0, 0,
			    (gdouble) (width - HILDON_HOME_TASKNAV_WIDTH) / src_width,
			    (gdouble) height / src_height,
			    GDK_INTERP_NEAREST,
			    HILDON_HOME_IMAGE_ALPHA_FULL);
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  return dest;
}

#if 0
static GdkPixbuf *
create_background_from_pixbuf_area (const GdkPixbuf  *src,
				    gint              src_x,
				    gint              src_y,
				    gint              width,
				    gint              height,
				    GError          **error)
{
  GdkPixbuf *dest;
  
  g_return_val_if_fail (src != NULL, NULL);

  dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
  if (!dest)
    {
      g_set_error (error, BACKGROUND_MANAGER_ERROR,
		   BACKGROUND_MANAGER_ERROR_SYSTEM_RESOURCES,
		   "Unable to create a new pixbuf");

      return NULL;
    }

  gdk_pixbuf_copy_area (src, src_x, src_y, width, height, dest, 0, 0);

  return dest;
}
#endif

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
		      GError          **error)
{
  GError *bg_error;
  GdkPixbuf *pixbuf;
  GdkPixbuf *compose;

  g_debug ("Compositing background image...");

  bg_error = NULL;

  if (bg_image)
    {
      pixbuf = create_background_from_pixbuf (bg_image,
					      bg_color,
					      mode,
					      HILDON_HOME_WINDOW_WIDTH,
					      HILDON_HOME_WINDOW_HEIGHT,
					      &bg_error);
    }
  else
    {
      pixbuf = create_background_from_color (bg_color,
					     HILDON_HOME_WINDOW_WIDTH,
					     HILDON_HOME_WINDOW_HEIGHT);

      g_return_val_if_fail (pixbuf, NULL);
    }
  
  if (bg_error)
    {
      g_propagate_error (error, bg_error);

      return NULL;
    }

  compose = load_image_from_file (titlebar_path, &bg_error);
  if (bg_error)
    {
      g_warning ("Unable to load titlebar pixbuf: %s", bg_error->message);
      
      g_error_free (bg_error);
      bg_error = NULL;
    }
  else
    {
      g_debug ("Compositing titlebar");
      
      gdk_pixbuf_composite (compose,
		            pixbuf,
			    0, 0,
			    gdk_pixbuf_get_width (compose),
			    gdk_pixbuf_get_height (compose),
			    HILDON_HOME_TASKNAV_WIDTH, 0,
			    1.0, 1.0,
			    GDK_INTERP_NEAREST,
			    HILDON_HOME_IMAGE_ALPHA_FULL);

      g_object_unref (compose);
      compose = NULL;
    }
 
  compose = load_image_from_file (sidebar_path, &bg_error);
  if (bg_error)
    {
      g_warning ("Unable to load sidebar pixbuf: %s", bg_error->message);
      
      g_error_free (bg_error);
      bg_error = NULL;
    }
  else
    {
      gint width = gdk_pixbuf_get_width (compose);
      gint height = gdk_pixbuf_get_height (compose);
      gint sidebar_height;
      
      g_debug ("Compositing sidebar (w:%d, h:%d)",
	       width, height);
      
      sidebar_height = HILDON_HOME_WINDOW_HEIGHT - HILDON_HOME_TITLEBAR_HEIGHT;

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
			    HILDON_HOME_TASKNAV_WIDTH,
			    HILDON_HOME_TITLEBAR_HEIGHT,
			    gdk_pixbuf_get_width (compose),
			    gdk_pixbuf_get_height (compose),
			    HILDON_HOME_TASKNAV_WIDTH,
			    0,
			    1.0, 1.0,
			    GDK_INTERP_NEAREST,
			    HILDON_HOME_IMAGE_ALPHA_FULL);

      g_object_unref (compose);
      compose = NULL;
    }
  
  return pixbuf;
}

static void
child_done_cb (GPid     pid,
	       gint     status,
	       gpointer user_data)
{
  BackgroundManager *manager;
  
  g_warning ("child done");

  manager = BACKGROUND_MANAGER (user_data);
  manager->priv->child_pid = 0;
}

static gboolean
read_pipe_from_child (GIOChannel   *source,
		      GIOCondition  condition,
		      gpointer      user_data)
{
  BackgroundManager *manager = BACKGROUND_MANAGER (user_data);
  GError *load_error;
  GdkPixbuf *pixbuf;
  
  /* something arrived from the child pipe: this means that
   * the child fired an error message and (possibly) died; we
   * relay the error to the console.
   */
  if (condition & G_IO_IN)
    {
      gchar *error = NULL;

      g_io_channel_read_line (source, &error, NULL, NULL, NULL);
      
      g_warning ("Unable to create background: %s", error);

      goto finish_up;
    }

  /* at this point we should be done with the child,
   * so it means that the background creation went fine
   * and we can load the background from the cache,
   * save the configuration and emit the changed signal
   * for the users to update themselves
   */
  
  load_error = NULL;
  pixbuf = gdk_pixbuf_new_from_file (manager->priv->cache,
		                     &load_error);
  if (load_error)
    {
      g_warning ("Unable to load background: %s", load_error->message);
      g_error_free (load_error);
    }
  else
    {
      background_manager_write_configuration (manager);

      g_signal_emit (manager, manager_signals[CHANGED], 0, pixbuf);
      g_object_unref (pixbuf);
    }

 finish_up:
  if (manager->priv->bg_timeout)
    {
      g_source_remove (manager->priv->bg_timeout);
      manager->priv->bg_timeout = 0;
    }

  if (manager->priv->loading_note)
    {
      gtk_widget_destroy(manager->priv->loading_note);
      manager->priv->loading_note = NULL;
    }

  return FALSE;
}

static void
signal_handler (int sig)
{
  signal (sig, SIG_DFL);
  kill (getpid (), sig);
}

static gboolean
loading_image_banner_update_progress (gpointer data)
{
  GtkProgressBar *pbar;
  
  if (!data || !GTK_IS_PROGRESS_BAR (data))
    return FALSE;
  
  pbar = GTK_PROGRESS_BAR (data);

  gtk_progress_bar_pulse (pbar);
  return TRUE;
}

static void
loading_image_banner_response (GtkDialog *dialog,
			       gint       response,
			       gpointer   user_data)
{
  if (response == GTK_RESPONSE_CANCEL)
    {
      BackgroundManager *manager = BACKGROUND_MANAGER (user_data);
      
      if (manager->priv->child_pid != 0)
        kill (manager->priv->child_pid, SIGTERM);
    }
}

static void
show_loading_image_banner (BackgroundManager *manager)
{
  GtkDialog *dialog;
  GtkWidget *label;
  GtkWidget *prog_bar;
  
  if (manager->priv->loading_note)
    gtk_widget_destroy (manager->priv->loading_note);
  
  manager->priv->loading_note =
    gtk_dialog_new_with_buttons ("",
		 		 NULL,
				 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				 HILDON_HOME_SET_BG_CANCEL,
				 GTK_RESPONSE_CANCEL,
				 NULL);
  dialog = GTK_DIALOG (manager->priv->loading_note);
  
  label = gtk_label_new (HILDON_HOME_LOADING_IMAGE_TEXT);
  gtk_container_add (GTK_CONTAINER (dialog->vbox), label);

  prog_bar = gtk_progress_bar_new ();
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (prog_bar), 0.2);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (prog_bar), 0.2);
  gtk_container_add (GTK_CONTAINER (dialog->vbox), prog_bar);

  g_timeout_add (500, loading_image_banner_update_progress, prog_bar);

  gtk_dialog_set_has_separator (dialog, FALSE);
  
  g_signal_connect (dialog, "response",
		    G_CALLBACK (loading_image_banner_response),
		    manager);

  gtk_widget_realize (manager->priv->loading_note);
  gdk_window_set_decorations (manager->priv->loading_note->window,
		  	      GDK_DECOR_BORDER);

  gtk_widget_show_all (manager->priv->loading_note);
}

static gboolean
bg_timeout_cb (BackgroundManager * manager)
{
  show_loading_image_banner (manager);

  /* one-off operation */
  manager->priv->bg_timeout = 0;
  return FALSE;
}

static void
background_manager_create_background (BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv;
  GPid pid;
  int parent_exit_notify[2];
  int pipe_from_child[2];
  static gboolean first_run = TRUE;

  priv = manager->priv;

  /* upon first load, try to get the cached pixmap instead
   * of generating the background; this speeds up the loading
   * as it's sure that the background did not change behind
   * our backs since last time.  in case loading fails, then
   * we fall through and regenerate the cache.
   */
  if (first_run)
    {
      GdkPixbuf *pixbuf;
      GError *load_error = NULL;

      pixbuf = gdk_pixbuf_new_from_file (priv->cache, &load_error);
      if (load_error)
        {
          g_warning ("Unable to load background from cache: %s",
		     load_error->message);
          g_error_free (load_error);
        }
      else
        {
          g_debug ("Background loaded from cache");

          g_signal_emit (manager, manager_signals[CHANGED], 0, pixbuf);
          g_object_unref (pixbuf);
          first_run = FALSE;

          return;
        }
    }

  if (first_run)
    first_run = FALSE;
  else
    priv->bg_timeout = g_timeout_add (1000, (GSourceFunc)bg_timeout_cb, manager);
  
  g_debug ("unlink the cache image file");
  if (g_unlink (priv->cache) == -1)
    {
      if (errno != ENOENT)
        {
	  g_warning ("Unable to remove the cache file: %s",
	             g_strerror (errno));
	}
    }

  pipe (parent_exit_notify);
  pipe (pipe_from_child);

  pid = fork ();
  if (pid == 0)
    {
      GError *err = NULL;
      GdkPixbuf *image = NULL;
      GdkPixbuf *pixbuf;

      g_debug ("Creating background (pid %d)...", getpid ());

      signal (SIGINT, signal_handler);
      signal (SIGTERM, signal_handler);

      close (parent_exit_notify[1]);
      close (pipe_from_child[0]);

      if (priv->image_uri)
	image = load_image_from_uri (priv->image_uri, TRUE, &err);
	  
      if (err && err->message)
	{
	  write (pipe_from_child[1],
		 err->message,
		 strlen (err->message));
	  
	  g_error_free (err);

	  goto close_channel;
	}

      pixbuf = composite_background (image, &(priv->color),
				     priv->mode,
				     priv->sidebar,
				     priv->titlebar,
				     &err);
      if (err && err->message)
	{
	  write (pipe_from_child[1],
		 err->message,
		 strlen (err->message));

	  g_error_free (err);

	  if (image)
	    g_object_unref (image);

	  goto close_channel;
	}
      else if (image)
	{
	  g_object_unref (image);
	}

      g_debug ("saving composited background");
      
      /* FIXME: the actual saving of the file to the flash storage is
       * slow (depending on the size of the image; for the default bacgrounds
       * it is from 9 to 25s)
       *
       * Perhaps we could use the pipe to send the pixbuf data from the child
       * to the parent and so make it possible to load the background before
       * while the child is working on the save.
       */
      save_image_to_file (pixbuf, priv->cache, &err);

      if (err && err->message)
	{
	  write (pipe_from_child[1],
		 err->message,
		 strlen (err->message));

	  g_error_free (err);
	  g_object_unref (pixbuf);
	}
      else
	{
	  g_object_unref (pixbuf);
	}

    close_channel:
      /* we're done, so we close the pipe and let
       * the parent process know that we finished
       */
      close (pipe_from_child[1]);

      _exit (0);
    }
  else if (pid > 0)
    {
      GIOChannel *channel;

      g_debug ("Child spawned (pid %d)...", pid);
      manager->priv->child_pid = pid;

      close (parent_exit_notify[0]);
      close (pipe_from_child[1]);

      channel = g_io_channel_unix_new (pipe_from_child[0]);
      g_io_add_watch (channel,
		      G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
		      read_pipe_from_child,
		      manager);
      g_io_channel_unref (channel);

      g_child_watch_add (pid, child_done_cb, manager);
    }
  else
    {
      g_assert_not_reached ();
    }
}

static void
background_manager_class_init (BackgroundManagerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = background_manager_set_property;
  gobject_class->get_property = background_manager_get_property;
  gobject_class->finalize = background_manager_finalize;

  g_object_class_install_property (gobject_class,
		  		   PROP_IMAGE_URI,
				   g_param_spec_string ("image-uri",
					   		"Image URI",
							"The URI of the background image",
							NULL,
							(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  g_object_class_install_property (gobject_class,
		  		   PROP_MODE,
				   g_param_spec_enum ("mode",
					   	      "Mode",
						      "The mode to be used when applying the image",
						      TYPE_BACKGROUND_MODE,
						      BACKGROUND_CENTERED,
						      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
  g_object_class_install_property (gobject_class,
		  		   PROP_COLOR,
				   g_param_spec_boxed ("color",
					   	       "Color",
						       "The color to be merged to the background",
						       GDK_TYPE_COLOR,
						       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
  
  manager_signals[CHANGED] =
    g_signal_new ("changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BackgroundManagerClass, changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE,
		  1, G_TYPE_POINTER);

  g_type_class_add_private (gobject_class, sizeof (BackgroundManagerPrivate));
}

static void
background_manager_init (BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv;

  manager->priv = priv = BACKGROUND_MANAGER_GET_PRIVATE (manager);

  priv->screen = NULL;
  priv->is_screen_singleton = FALSE;

  priv->color.red = 0;
  priv->color.green = 0;
  priv->color.blue = 0;

  priv->image_uri = g_strdup_printf ("file://%s",
		                     hildon_home_get_user_bg_file ());
  priv->cache = g_build_filename (hildon_home_get_user_config_dir (),
		                  "hildon_home_bg_user.png",
				  NULL);
  priv->mode = BACKGROUND_CENTERED;

  background_manager_read_configuration (manager);
}

/* We bind the background manager to the GdkScreen object, like
 * GTK does for the icon theme, so that we don't have to handle
 * the lifetime of the background manager and we can have a per
 * screen singleton that is destroyed as soon as the display is
 * closed.  Also, it makes perfectly sense to bind a per screen
 * object to the GdkScreen.
 */

static void background_manager_set_screen (BackgroundManager *manager,
					   GdkScreen         *screen);

static void
display_closed (GdkDisplay        *display,
		gboolean           is_error,
		BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv = manager->priv;
  GdkScreen *screen = priv->screen;
  gboolean was_screen_singleton = priv->is_screen_singleton;

  if (was_screen_singleton)
    {
      g_object_set_qdata (G_OBJECT (screen), HILDON_BACKGROUND_MANAGER, NULL);
      priv->is_screen_singleton = FALSE;
    }

  background_manager_set_screen (manager, NULL);

  if (was_screen_singleton)
    g_object_unref (manager);
}

static void
unset_screen (BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv = manager->priv;
  GdkDisplay *display;

  if (priv->screen)
    {
      display = gdk_screen_get_display (priv->screen);

      g_signal_handlers_disconnect_by_func (display,
		      			    (gpointer) display_closed,
					    manager);

      priv->screen = NULL;
    }
}

static void
background_manager_set_screen (BackgroundManager *manager,
			       GdkScreen         *screen)
{
  BackgroundManagerPrivate *priv;
  GdkDisplay *display;
  
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));
  g_return_if_fail (screen == NULL || GDK_IS_SCREEN (screen));
  
  priv = manager->priv;

  unset_screen (manager);

  if (screen)
    {
      display = gdk_screen_get_display (screen);
      
      g_signal_connect (display, "closed",
		        G_CALLBACK (display_closed), manager);

      priv->screen = screen;
    }
}

static BackgroundManager *
background_manager_get_for_screen (GdkScreen *screen)
{
  BackgroundManager *manager;
  
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
  g_return_val_if_fail (!screen->closed, NULL);

  manager = g_object_get_qdata (G_OBJECT (screen), HILDON_BACKGROUND_MANAGER);
  if (!manager)
    {
      manager = g_object_new (TYPE_BACKGROUND_MANAGER, NULL);
      background_manager_set_screen (manager, screen);

      manager->priv->is_screen_singleton = TRUE;

      g_object_set_qdata (G_OBJECT (screen),
		          HILDON_BACKGROUND_MANAGER,
			  manager);
    }

  return manager;
}

BackgroundManager *
background_manager_get_default (void)
{
  return background_manager_get_for_screen (gdk_screen_get_default ());
}

G_CONST_RETURN gchar *
background_manager_get_image_uri (BackgroundManager *manager)
{
  g_return_val_if_fail (IS_BACKGROUND_MANAGER (manager), NULL);

  return manager->priv->image_uri;
}

void
background_manager_set_image_uri (BackgroundManager  *manager,
				  const gchar        *uri)
{
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));

  g_object_ref (manager);

  if (background_manager_preset_image_uri (manager, uri))
    {
      g_object_notify (G_OBJECT (manager), "image-uri");
      background_manager_create_background (manager);
      background_manager_write_configuration (manager);
    }
  
  g_object_unref (manager);
}

gboolean
background_manager_preset_image_uri (BackgroundManager  *manager,
				     const gchar        *uri)
{
  BackgroundManagerPrivate *priv;
  gboolean retval = FALSE;
  
  g_return_val_if_fail (IS_BACKGROUND_MANAGER (manager), FALSE);

  g_object_ref (manager);

  priv = manager->priv;

  if (uri && g_str_equal (uri, HILDON_HOME_BACKGROUND_NO_IMAGE))
    uri = NULL;
  
  if ((!priv->image_uri && uri)  ||
      (priv->image_uri && ! uri) ||
      (priv->image_uri && uri && !g_str_equal (uri, priv->image_uri)))
    {
      g_free (priv->image_uri);

      if (uri)
	priv->image_uri = g_strdup (uri);
      else
	priv->image_uri = NULL;

      retval = TRUE;
    }
  
  g_object_unref (manager);

  return retval;
}

void
background_manager_set_components (BackgroundManager *manager,
				   const gchar       *titlebar,
				   const gchar       *sidebar)
{
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));

  if (titlebar && titlebar[0] != '\0')
    {
      g_free (manager->priv->titlebar);
      manager->priv->titlebar = g_strdup (titlebar);
    }

  if (sidebar && sidebar[0] != '\0')
    {
      g_free (manager->priv->sidebar);
      manager->priv->sidebar = g_strdup (sidebar);
    }

  background_manager_create_background (manager);
}

G_CONST_RETURN gchar *
background_manager_get_cache (BackgroundManager *manager)
{
  g_return_val_if_fail (IS_BACKGROUND_MANAGER (manager), NULL);

  return manager->priv->cache;
}

BackgroundMode
background_manager_get_mode (BackgroundManager *manager)
{
  g_return_val_if_fail (IS_BACKGROUND_MANAGER (manager), BACKGROUND_CENTERED);

  return manager->priv->mode;
}

void
background_manager_set_mode (BackgroundManager *manager,
			     BackgroundMode     mode)
{
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));
  
  g_object_ref (manager);

  if (background_manager_preset_mode (manager, mode))
    {
      g_object_notify (G_OBJECT (manager), "mode");
      background_manager_create_background (manager);
      background_manager_write_configuration (manager);
    }
  
  g_object_unref (manager);
}

gboolean
background_manager_preset_mode (BackgroundManager *manager,
				BackgroundMode     mode)
{
  BackgroundManagerPrivate *priv;
  gboolean retval = FALSE;
  
  g_return_val_if_fail (IS_BACKGROUND_MANAGER (manager), FALSE);
  
  priv = manager->priv;
  
  if (priv->mode != mode)
    {
      g_object_ref (manager);
      
      priv->mode = mode;

      g_object_unref (manager);
      
      retval = TRUE;
    }

  return retval;
}

void
background_manager_get_color (BackgroundManager *manager,
			      GdkColor          *color)
{
  BackgroundManagerPrivate *priv;
  
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));

  priv = manager->priv;
  
  color->red = priv->color.red;
  color->green = priv->color.green;
  color->blue = priv->color.blue;
}

void
background_manager_set_color (BackgroundManager *manager,
			      const GdkColor    *color)
{
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));
  g_return_if_fail (color != NULL);

  g_object_ref (manager);

  if (background_manager_preset_color (manager, color))
    {
      g_object_notify (G_OBJECT (manager), "color");
      background_manager_create_background (manager);
      background_manager_write_configuration (manager);
    }
  
  g_object_unref (manager);
}

gboolean
background_manager_preset_color (BackgroundManager *manager,
				 const GdkColor    *color)
{
  BackgroundManagerPrivate *priv;
  gboolean retval = FALSE;
  
  g_return_val_if_fail (IS_BACKGROUND_MANAGER (manager), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  priv = manager->priv;

  g_object_ref (manager);

  if ( priv->color.red != color->red     || 
       priv->color.green != color->green ||
       priv->color.blue != color->blue)
    {
      priv->color.red = color->red;
      priv->color.green = color->green;
      priv->color.blue = color->blue;

      retval = TRUE;
    }
  
  g_object_unref (manager);

  return retval;
}

void
background_manager_apply_preset (BackgroundManager * manager)
{
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));

  g_object_ref (manager);

  g_object_notify (G_OBJECT (manager), "color");
  g_object_notify (G_OBJECT (manager), "mode");
  g_object_notify (G_OBJECT (manager), "image-uri");
  
  background_manager_create_background (manager);
  background_manager_write_configuration (manager);
  
  g_object_unref (manager);
}
