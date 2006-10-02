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
#include "config.h"
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

typedef struct
{
  gchar *image_uri;
  BackgroundMode mode;
  GdkColor color;

  gchar *cache;

  guint has_cache  : 1;
  guint is_preview : 1;
} BackgroundData;

static BackgroundData *
background_data_new (gboolean is_preview)
{
  BackgroundData *retval;

  retval = g_new (BackgroundData, 1);

  retval->color.red = 0;
  retval->color.green = 0;
  retval->color.blue = 0;
  
  retval->image_uri = NULL;
  
  if (!is_preview)
    {
      retval->image_uri = g_strdup_printf ("file://%s",
					   hildon_home_get_user_bg_file ());
      retval->cache = g_build_filename (hildon_home_get_user_config_dir (),
			                "hildon_home_bg_user.png",
					NULL);
      retval->mode = BACKGROUND_CENTERED;
    }
  else
    {
      retval->image_uri = g_strdup_printf ("file://%s",
		      			   hildon_home_get_user_bg_file ());
      retval->cache = g_build_filename (hildon_home_get_user_config_dir (),
		                        "hildon_home_bg_preview.png",
				        NULL);
      retval->mode = BACKGROUND_CENTERED;
    }

  retval->is_preview = is_preview;
  retval->has_cache = FALSE;

  return retval;
}

static void
background_data_free (BackgroundData *data)
{
  if (G_LIKELY (data != NULL))
    {
      g_free (data->image_uri);
      g_free (data->cache);

      g_free (data);
    }
}

struct _BackgroundManagerPrivate
{
  BackgroundData *normal;
  BackgroundData *preview;
  BackgroundData *current;

  guint is_preview_mode : 1;
  
  /* these are local files */
  gchar *titlebar;
  gchar *sidebar;

  GPid child_pid;

  guint is_screen_singleton : 1;
  GdkScreen *screen;

  GdkWindow *desktop;
  
  guint bg_timeout;
  GtkWidget *loading_note;
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
  PREVIEW,
  LOAD_BEGIN,
  LOAD_COMPLETE,
  LOAD_CANCEL,
  LOAD_ERROR,

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
  BackgroundManager *manager = BACKGROUND_MANAGER (object);
  
  switch (prop_id)
    {
    case PROP_IMAGE_URI:
      background_manager_set_image_uri (manager,
                                        g_value_get_string (value));
      break;
    case PROP_MODE:
      background_manager_set_mode (manager, g_value_get_enum (value));
      break;
    case PROP_COLOR:
      background_manager_set_color (manager, g_value_get_boxed (value));
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
      g_value_set_string (value, background_manager_get_image_uri (manager));
      break;
    case PROP_MODE:
      g_value_set_enum (value, background_manager_get_mode (manager));
      break;
    case PROP_COLOR:
      {
        GdkColor color;

        background_manager_get_color (manager, &color);
        g_value_set_boxed (value, &color);
      }
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

  background_data_free (priv->preview);
  background_data_free (priv->normal);
  priv->current = NULL;

  g_free (priv->titlebar);
  g_free (priv->sidebar);

  if (priv->bg_timeout)
    g_source_remove (priv->bg_timeout);

  if (priv->loading_note)
    gtk_widget_destroy (priv->loading_note);
  
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
  const gchar *config_file;
  gchar *config_backup;

  priv = manager->priv;
  
  /* dump the state of the background to the config file */
  conf = g_key_file_new ();
  
  /* images locations */
  if (priv->current->image_uri)
    g_key_file_set_string (conf, HILDON_HOME_CONF_MAIN_GROUP,
			   HILDON_HOME_CONF_BG_URI_KEY,
			   priv->current->image_uri);
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
		          priv->current->color.red);
  g_key_file_set_integer (conf, HILDON_HOME_CONF_MAIN_GROUP,
		  	  HILDON_HOME_CONF_BG_COLOR_GREEN_KEY,
			  priv->current->color.green);
  g_key_file_set_integer (conf, HILDON_HOME_CONF_MAIN_GROUP,
		          HILDON_HOME_CONF_BG_COLOR_BLUE_KEY,
			  priv->current->color.blue);

  /* background mode, by symbolic name */
  enum_class = g_type_class_ref (TYPE_BACKGROUND_MODE);
  enum_value = g_enum_get_value (enum_class, priv->current->mode);
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
  else
    g_key_file_free (conf);

  config_file = hildon_home_get_user_config_file ();

  /* save a backup copy of the current configuration */
  config_backup = g_strconcat (config_file, ".bak", NULL);
  if (g_rename (config_file, config_backup) == -1)
    {
      g_warning ("Unable to save a backup copy of the configuration: %s",
                 g_strerror (errno));
    }
  
  g_free (config_backup);

  /* save the configuration */
  error = NULL;
  g_file_set_contents (config_file, data, len, &error);
  if (error)
    {
      g_warning ("Error while saving configuration to file `%s': %s",
		 hildon_home_get_user_config_file (),
		 error->message);

      g_error_free (error);
    }

  g_free (data);
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
  
  g_free (priv->current->image_uri);
  priv->current->image_uri =
    g_key_file_get_string (conf, HILDON_HOME_CONF_MAIN_GROUP,
				 HILDON_HOME_CONF_BG_URI_KEY,
				 &parse_error);
  if (parse_error)
    {
      priv->current->image_uri =
        g_strconcat ("file://", hildon_home_get_user_bg_file (), NULL);
      
      g_error_free (parse_error);
      parse_error = NULL;
    }
  else if (!strcmp (priv->current->image_uri, HILDON_HOME_BACKGROUND_NO_IMAGE))
    {
      g_free (priv->current->image_uri);
      priv->current->image_uri = NULL;
    }
  
  priv->current->color.red =
    g_key_file_get_integer (conf, HILDON_HOME_CONF_MAIN_GROUP,
				  HILDON_HOME_CONF_BG_COLOR_RED_KEY,
				  &parse_error);
  if (parse_error)
    {
      g_warning ("failed read color.red");
      priv->current->color.red = 0;

      g_clear_error (&parse_error);
    }

  priv->current->color.green =
    g_key_file_get_integer (conf, HILDON_HOME_CONF_MAIN_GROUP,
			          HILDON_HOME_CONF_BG_COLOR_GREEN_KEY,
			          &parse_error);
  if (parse_error)
    {
      g_warning ("failed read color.green");
      priv->current->color.green = 0;

      g_clear_error (&parse_error);
    }

  priv->current->color.blue =
    g_key_file_get_integer (conf, HILDON_HOME_CONF_MAIN_GROUP,
			          HILDON_HOME_CONF_BG_COLOR_BLUE_KEY,
			          &parse_error);
  if (parse_error)
    {
      g_warning ("failed read color.blue");
      priv->current->color.blue = 0;

      g_clear_error (&parse_error);
    }
  
  mode = g_key_file_get_string (conf, HILDON_HOME_CONF_MAIN_GROUP,
				      HILDON_HOME_CONF_BG_MODE_KEY,
				      &parse_error);
  if (parse_error)
    {
      mode = g_strdup ("centered");

      g_clear_error (&parse_error);
    }

  enum_class = g_type_class_ref (TYPE_BACKGROUND_MODE);
  enum_value = g_enum_get_value_by_nick (enum_class, mode);
  priv->current->mode = enum_value->value;
  
  g_type_class_unref (enum_class);
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
                     gboolean      oom_check,
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
                                 hildon_home_get_mmc_mount_point (),
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

  if (!oom_check &&
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

  if (oom_check)
    osso_mem_saw_disable ();

  return retval;
}

static GdkPixbuf *
load_image_from_file (const gchar  *filename,
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
          dest_x = MAX (HILDON_HOME_TASKNAV_WIDTH,
                        (width - HILDON_HOME_TASKNAV_WIDTH - src_width)
                         / 2
                         + HILDON_HOME_TASKNAV_WIDTH);
	}
      else
	{
	  dest_x = MAX (HILDON_HOME_TASKNAV_WIDTH,
                        (width - HILDON_HOME_TASKNAV_WIDTH - src_width)
                        / 2
                        + HILDON_HOME_TASKNAV_WIDTH);
	}

      if (src_height < height)
        {
          dest_y = MAX (0, (height - src_height) / 2);
	}
      else
        {
	  dest_y = MAX (0, (height - src_height) / 2);
	}
      
      off_x = (width - HILDON_HOME_TASKNAV_WIDTH - src_width)
              / 2
              + HILDON_HOME_TASKNAV_WIDTH;
      off_y = (height-src_height) / 2;
      
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
      scaling_ratio = MIN ((gdouble) ((gdouble) (width - HILDON_HOME_TASKNAV_WIDTH) / src_width),
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
	     dest_x < width;
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
			    width - HILDON_HOME_TASKNAV_WIDTH, height,
			    0, 0,
			    (gdouble) (width) / src_width,
			    ((gdouble) height) / src_height,
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
      
      sidebar_height = HILDON_HOME_WINDOW_HEIGHT
	               - HILDON_HOME_TITLEBAR_HEIGHT;
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

static gchar *
pack_gerror (GError *error,
             gsize  *length)
{
  gchar *retval;
  
  g_return_val_if_fail (error != NULL, NULL);

  retval = g_strdup_printf ("%s|%d|%s",
                            g_quark_to_string (error->domain),
                            error->code,
                            error->message);
  

  if (*length)
    *length = strlen (retval);

  return retval;
}

static gboolean
unpack_gerror (GError **dest,
               gchar   *packed_gerror,
               gsize    length)
{
  GError *error;
  gchar **unpacked;
  GQuark domain;
  gint code;
  gchar *message;
  
  g_return_val_if_fail ((dest != NULL) && (*dest == NULL), FALSE);
  g_return_val_if_fail (packed_gerror != NULL, FALSE);

  if (length < 0)
    length = strlen (packed_gerror);

  unpacked = g_strsplit (packed_gerror, "|", 3);
  if (!unpacked)
    return FALSE;

  if (g_strv_length (unpacked) != 3)
    {
      g_strfreev (unpacked);
      return FALSE;
    }

  g_debug ("error: { %s, %s, %s }", unpacked[0], unpacked[1], unpacked[2]);
  
  domain = g_quark_from_string (unpacked[0]);
  code = atoi (unpacked[1]);
  message = unpacked[2];
  
  error = g_error_new_literal (domain, code, message);
  if (!error)
    {
      g_strfreev (unpacked);
      return FALSE;
    }
  else
    g_propagate_error (dest, error);
  
  g_strfreev (unpacked);
  
  return TRUE;
}

static void
child_done_cb (GPid     pid,
	       gint     status,
	       gpointer user_data)
{
  BackgroundManager *manager = user_data;
  
  g_debug ("child (pid:%d) done (status:%d)",
	   (int) pid,
	   WIFEXITED (status) ? WEXITSTATUS (status)
	                      : WIFSIGNALED (status) ? WTERMSIG (status)
			      			     : -1);

  manager->priv->child_pid = 0;
}

static gboolean
read_pipe_from_child (GIOChannel   *source,
		      GIOCondition  condition,
		      gpointer      user_data)
{
  BackgroundManager *manager = BACKGROUND_MANAGER (user_data);
  BackgroundManagerPrivate *priv = manager->priv;
  
  /* something arrived from the child pipe: this means that
   * the child fired an error message and (possibly) died; we
   * relay the error to the console.
   */
  if (condition & G_IO_IN)
    {
      gchar *packed_error = NULL;
      GError *error = NULL;

      g_io_channel_read_line (source, &packed_error, NULL, NULL, NULL);
      
      unpack_gerror (&error, packed_error, strlen (packed_error));
      
      g_warning ("Unable to create background: %s",
                 error->message);

      g_signal_emit (manager, manager_signals[LOAD_ERROR], 0, error);

      g_error_free (error);

      goto finish_up;
    }

  if (!priv->is_preview_mode)
    background_manager_write_configuration (manager);
  
  g_signal_emit (manager, manager_signals[LOAD_COMPLETE], 0);
  
finish_up:
  if (priv->bg_timeout)
    {
      g_source_remove (priv->bg_timeout);
      priv->bg_timeout = 0;
    }

  if (priv->loading_note)
    {
      gtk_widget_destroy (priv->loading_note);
      priv->loading_note = NULL;
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
        {
          kill (manager->priv->child_pid, SIGTERM);

          g_signal_emit (manager, manager_signals[LOAD_CANCEL], 0);
        }
    }
}

static void
show_loading_image_banner (BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv;
  GtkDialog *dialog;
  GtkWidget *label;
  GtkWidget *prog_bar;
  
  priv = manager->priv;
  
  if (priv->loading_note)
    gtk_widget_destroy (priv->loading_note);
  
  priv->loading_note =
    gtk_dialog_new_with_buttons ("",
		 		 NULL,
				 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				 HILDON_HOME_SET_BG_CANCEL, GTK_RESPONSE_CANCEL,
				 NULL);
  dialog = GTK_DIALOG (priv->loading_note);
  
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

  gtk_widget_realize (priv->loading_note);
  gdk_window_set_decorations (priv->loading_note->window,
		  	      GDK_DECOR_BORDER);

  gtk_widget_show_all (priv->loading_note);
}

static gboolean
bg_timeout_cb (gpointer data)
{
  BackgroundManager *manager = data;
  
  show_loading_image_banner (manager);

  /* one-off operation */
  manager->priv->bg_timeout = 0;
  
  return FALSE;
}

gboolean
background_manager_refresh_from_cache (BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv;
  GdkPixbuf *pixbuf;
  GError *load_error = NULL;

  priv = manager->priv;

  pixbuf = gdk_pixbuf_new_from_file (priv->current->cache, &load_error);
  if (load_error)
    {
      g_warning ("Unable to load background from cache: %s",
                 load_error->message);
      g_error_free (load_error);

      return FALSE;
    }
  else
    {
      g_debug ("Background loaded from cache");
      
      g_signal_emit (manager, manager_signals[CHANGED], 0, pixbuf);
      g_object_unref (pixbuf);

      return TRUE;
    }
}

/* background_manager_create_background:
 * @manager: a #BackgroundManager
 * @cancellable: whether the creation should be cancellable or not.
 *
 * this function creates the background by loading the user-defined image,
 * compositing it into a pixbuf with the user-defined background color using
 * the user-defined mode (scaled, stretched, tiled and centered).  then the
 * "changed" signal is emitted, for the main desktop window to catch and
 * update the background pixmap.
 *
 * a cache image is also saved, to be used when booting up; the image saving
 * is the critical section because of the PNG compression and I/O operation;
 * hence, it's done in a child process which relays the eventual errors using
 * a pipe.
 *
 * the save process is cancellable: if @cancellable is %TRUE a modal dialog
 * will appear; if the user presses the Cancel button inside it, the child
 * process is killed and the "load-cancel" signal is emitted.  if the save
 * process terminated successfully, the "load-complete" signal is emitted.
 *
 * in case the BackgroundManager is in preview mode (that is, the function
 * background_manager_push_preview_mode() has been called), the "preview"
 * signal is emitted instead of the "changed" signal.
 */
static void
background_manager_create_background (BackgroundManager *manager,
                                      gboolean           cancellable)
{
  BackgroundManagerPrivate *priv;
  BackgroundData *current;
  GPid pid;
  int parent_exit_notify[2];
  int pipe_from_child[2];
  static gboolean first_run = TRUE;
  GdkPixbuf *image, *pixbuf;
  GError *err;
  
  priv = manager->priv;

  /* upon first load, try to get the cached pixmap instead
   * of generating the background; this speeds up the loading
   * as we're sure that the background did not change behind
   * our backs since last time.  in case loading fails, then
   * we fall through and regenerate the cache.
   */
  if (first_run)
    {
      if (background_manager_refresh_from_cache (manager))
        {
          first_run = FALSE;
          return;
        }
      else
        first_run = FALSE;
    }

  if (cancellable)
    priv->bg_timeout = g_timeout_add (1000, bg_timeout_cb, manager);

  current = priv->current;
  
  g_debug ("Creating background...");

  g_signal_emit (manager, manager_signals[LOAD_BEGIN], 0);

  err = NULL;
  if (current->image_uri)
    image = load_image_from_uri (current->image_uri, TRUE, &err);
  else
    image = NULL;

  if (err && err->message)
    {
      g_warning ("Unable to load background from `%s': %s",
                 current->image_uri,
                 err->message);

      if (cancellable)
        {
          if (priv->bg_timeout)
            g_source_remove (priv->bg_timeout);
          
          if (priv->loading_note)
            gtk_widget_destroy (priv->loading_note);
          
          priv->loading_note = NULL;
        }

      g_signal_emit_by_name (manager, "load-cancel");
      g_signal_emit_by_name (manager, "load-error", err);

      g_error_free (err);

      return;
    }

  pixbuf = composite_background (image, &(current->color),
				 current->mode,
				 priv->sidebar,
				 priv->titlebar,
				 &err);
  if (err && err->message)
    {
      g_warning ("Unable to composite backround: %s",
                 err->message);

      g_error_free (err);
      if (image)
        g_object_unref (image);

      return;
    }
  else if (image)
    {
      g_object_unref (image);
    }

  pipe (parent_exit_notify);
  pipe (pipe_from_child);

  pid = fork ();
  if (pid == 0)
    {
      GError *save_err = NULL;

      g_debug ("Saving background (pid %d)...", getpid ());

      signal (SIGINT, signal_handler);
      signal (SIGTERM, signal_handler);

      close (parent_exit_notify[1]);
      close (pipe_from_child[0]);

      save_image_to_file (pixbuf, current->cache, &save_err);
      if (save_err && save_err->message)
	{
          gchar *save_err_packed;
          gsize len;
          
          save_err_packed = pack_gerror (save_err, &len);
	  write (pipe_from_child[1], save_err_packed, len);

          g_free (save_err_packed);
	  g_error_free (save_err);
	}

      /* we're done, so we close the pipe and let
       * the parent process know that we finished
       */
      close (pipe_from_child[1]);

      _exit (0);
    }
  else if (pid > 0)
    {
      /* parent */
      GIOChannel *channel;

      g_debug ("Child spawned (pid %d)...", pid);
      priv->child_pid = pid;

      close (parent_exit_notify[0]);
      close (pipe_from_child[1]);

      channel = g_io_channel_unix_new (pipe_from_child[0]);
      g_io_add_watch (channel,
		      G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
		      read_pipe_from_child,
		      manager);
      g_io_channel_unref (channel);

      g_child_watch_add (pid, child_done_cb, manager);

      if (!priv->current->is_preview)
	{
	  g_signal_emit (manager, manager_signals[CHANGED], 0, pixbuf);
	}
      else
        {
          g_signal_emit (manager, manager_signals[PREVIEW], 0, pixbuf);
        }

      priv->current->has_cache = TRUE;
      
      g_object_unref (pixbuf);
    }
  else
    {
      g_warning ("Unable to fork: %s", g_strerror (errno));
      g_assert_not_reached ();
    }
}

static void
background_manager_changed (BackgroundManager *manager,
                            GdkPixbuf         *pixbuf)
{
  BackgroundManagerPrivate *priv;
  GdkColormap *colormap;
  GdkPixmap *pixmap;
  GdkBitmap *bitmask;
  
  if (G_UNLIKELY (!pixbuf))
    return;

  priv = manager->priv;
  if (G_UNLIKELY (!priv->desktop))
    {
      g_warning ("Attempting to set the background pixmap, but no desktop "
                 "GdkWindow set. Use background_manager_set_desktop()");
      return;
    }

  g_debug (G_STRLOC ": applying background");

  colormap =
    gdk_drawable_get_colormap (GDK_DRAWABLE (priv->desktop));

  gdk_pixbuf_render_pixmap_and_mask_for_colormap (pixbuf, colormap,
                                                  &pixmap,
                                                  &bitmask,
                                                  0);
  if (pixmap)
    gdk_window_set_back_pixmap (priv->desktop, pixmap, FALSE);

  if (bitmask)
    g_object_unref (bitmask);
}

static void
background_manager_preview (BackgroundManager *manager,
                            GdkPixbuf         *pixbuf)
{
  BackgroundManagerPrivate *priv;
  GdkColormap *colormap;
  GdkPixmap *pixmap;
  GdkBitmap *bitmask;
  
  if (G_UNLIKELY (!pixbuf))
    return;

  priv = manager->priv;
  if (G_UNLIKELY (!priv->desktop))
    {
      g_warning ("Attempting to set the background pixmap, but no desktop "
                 "GdkWindow set. Use background_manager_set_desktop()");
      return;
    }

  g_debug (G_STRLOC ": applying preview background");

  colormap =
    gdk_drawable_get_colormap (GDK_DRAWABLE (priv->desktop));

  gdk_pixbuf_render_pixmap_and_mask_for_colormap (pixbuf, colormap,
                                                  &pixmap,
                                                  &bitmask,
                                                  0);
  if (pixmap)
    gdk_window_set_back_pixmap (priv->desktop, pixmap, FALSE);

  if (bitmask)
    g_object_unref (bitmask);
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
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);
  manager_signals[PREVIEW] =
    g_signal_new ("preview",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BackgroundManagerClass, preview),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);
  manager_signals[LOAD_BEGIN] =
    g_signal_new ("load-begin",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BackgroundManagerClass, load_begin),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  manager_signals[LOAD_COMPLETE] =
    g_signal_new ("load-complete",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BackgroundManagerClass, load_complete),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  manager_signals[LOAD_CANCEL] =
    g_signal_new ("load-cancel",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BackgroundManagerClass, load_cancel),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  manager_signals[LOAD_ERROR] =
    g_signal_new ("load-error",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BackgroundManagerClass, load_error),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE,
          1,
          G_TYPE_POINTER);

  klass->changed = background_manager_changed;
  klass->preview = background_manager_preview;

  g_type_class_add_private (gobject_class, sizeof (BackgroundManagerPrivate));
}

static void
background_manager_init (BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv;

  manager->priv = priv = BACKGROUND_MANAGER_GET_PRIVATE (manager);

  priv->screen = NULL;
  priv->is_screen_singleton = FALSE;

  priv->normal = background_data_new (FALSE);
  priv->preview = background_data_new (TRUE);
  
  priv->current = priv->normal;
  priv->is_preview_mode = FALSE;
  
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

/*
 * background_manager_get_default:
 * 
 * Retrieves the #BackgroundManager object for the default screen; in case no
 * #BackgroundManager object has been set, it creates one.
 *
 * You should not use g_object_unref() on the returned instance: the
 * #BackgroundManager object is bound to the lifetime of the default screen
 * (as returned by gdk_screen_get_default()) and will be destroyed when the
 * default screen is closed.
 *
 * Return value: the #BackgroundManager for the default screen.
 */
BackgroundManager *
background_manager_get_default (void)
{
  return background_manager_get_for_screen (gdk_screen_get_default ());
}

G_CONST_RETURN gchar *
background_manager_get_image_uri (BackgroundManager *manager)
{
  g_return_val_if_fail (IS_BACKGROUND_MANAGER (manager), NULL);

  return manager->priv->current->image_uri;
}

static inline void
background_manager_set_image_uri_internal (BackgroundManager *manager,
                                           const gchar       *uri)
{
  BackgroundManagerPrivate *priv = manager->priv;

  g_object_ref (manager);

  g_free (priv->current->image_uri);
  priv->current->image_uri = g_strdup (uri);

  g_debug (G_STRLOC ": image_uri set to [%s]",
           priv->current->image_uri);

  if (!priv->is_preview_mode)
    background_manager_create_background (manager, TRUE);
  else
    priv->current->has_cache = FALSE;
  
  g_object_notify (G_OBJECT (manager), "image-uri");

  g_object_unref (manager);
}

void
background_manager_set_image_uri (BackgroundManager  *manager,
				  const gchar        *uri)
{
  BackgroundManagerPrivate *priv;
  
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));
  g_return_if_fail (uri != NULL);

  priv = manager->priv;

  if (strcmp (uri, HILDON_HOME_BACKGROUND_NO_IMAGE) == 0)
    {
      if (priv->current->image_uri)
        {
          background_manager_set_image_uri_internal (manager, NULL);
        }
    }
  else
    {
      if ((priv->current->image_uri == NULL) ||
          ((priv->current->image_uri != NULL) &&
           (strcmp (priv->current->image_uri, uri))))
        {
          background_manager_set_image_uri_internal (manager, uri);
        }
    }
}

void
background_manager_set_components (BackgroundManager *manager,
				   const gchar       *titlebar,
				   const gchar       *sidebar)
{
  BackgroundManagerPrivate *priv;
  
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));

  priv = manager->priv;
  
  if (titlebar && titlebar[0] != '\0')
    {
      g_free (priv->titlebar);
      priv->titlebar = g_strdup (titlebar);
    }

  if (sidebar && sidebar[0] != '\0')
    {
      g_free (priv->sidebar);
      priv->sidebar = g_strdup (sidebar);
    }

  background_manager_create_background (manager, FALSE);
}

G_CONST_RETURN gchar *
background_manager_get_cache (BackgroundManager *manager)
{
  g_return_val_if_fail (IS_BACKGROUND_MANAGER (manager), NULL);

  return manager->priv->current->cache;
}

BackgroundMode
background_manager_get_mode (BackgroundManager *manager)
{
  g_return_val_if_fail (IS_BACKGROUND_MANAGER (manager), BACKGROUND_CENTERED);

  return manager->priv->current->mode;
}

void
background_manager_set_mode (BackgroundManager *manager,
			     BackgroundMode     mode)
{
  BackgroundManagerPrivate *priv;
  
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));

  priv = manager->priv;
 
  if (priv->current->mode != mode)
    {
      g_object_ref (manager);

      priv->current->mode = mode;

      if (!priv->is_preview_mode)
        background_manager_create_background (manager, TRUE);
      else
        priv->current->has_cache = FALSE;

      g_object_notify (G_OBJECT (manager), "mode");
      g_object_unref (manager);
    }
}

void
background_manager_get_color (BackgroundManager *manager,
			      GdkColor          *color)
{
  BackgroundManagerPrivate *priv;
  
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));

  priv = manager->priv;
  
  color->red = priv->current->color.red;
  color->green = priv->current->color.green;
  color->blue = priv->current->color.blue;
}

void
background_manager_set_color (BackgroundManager *manager,
			      const GdkColor    *color)
{
  BackgroundManagerPrivate *priv;
  
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));
  g_return_if_fail (color != NULL);

  priv = manager->priv;
  
  if ((priv->current->color.red != color->red) ||
      (priv->current->color.green != color->green) ||
      (priv->current->color.blue != color->blue))
    {
      g_object_ref (manager);

      priv->current->color.red = color->red;
      priv->current->color.green = color->green;
      priv->current->color.blue = color->blue;

      if (!priv->is_preview_mode)
        background_manager_create_background (manager, TRUE);
      else
        priv->current->has_cache = FALSE;

      g_object_notify (G_OBJECT (manager), "color");
      g_object_unref (manager);
    }
}

/*
 * background_manager_push_preview_mode:
 * @manager: a #BackgroundManager
 *
 * Enables the "preview mode" of @manager.  A #BackgroundManager in preview
 * will not save the settings and will emit a "preview" signal instead of a
 * "changed" signal when the background is changed.  You should use this
 * function when showing a preview of the new settings of the background.
 */
void
background_manager_push_preview_mode (BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv;
  
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));

  priv = manager->priv;
  if (!priv->is_preview_mode)
    {
      g_debug (G_STRLOC ": in preview mode - image: %s",
               priv->current->image_uri);
      
      background_manager_discard_preview (manager, FALSE);

      priv->current = priv->preview;
      priv->is_preview_mode = TRUE;
    }
}

/*
 * background_manager_pop_preview_mode:
 * @manager: a #BackgroundManager
 *
 * Disables the "preview mode" of @manager.  All the settings changed during a
 * preview mode are discarded unless background_manager_apply_preview() is
 * called before calling this function.
 */
void
background_manager_pop_preview_mode (BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv;
  
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));

  priv = manager->priv;
  if (priv->is_preview_mode)
    {
      priv->current = priv->normal;
      priv->is_preview_mode = FALSE;

      g_debug ("%s: out of preview mode - image: %s",
               G_STRLOC,
               priv->current->image_uri);
    }
}

static inline void
background_manager_apply_image_uri_internal (BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv = manager->priv;
  BackgroundData *preview = priv->preview;
  BackgroundData *normal = priv->normal;

  g_free (normal->image_uri);
  normal->image_uri = g_strdup (preview->image_uri);

  g_object_notify (G_OBJECT (manager), "image-uri");

  g_debug (G_STRLOC ": image_uri changed");
}

/*
 * background_manager_apply_preview:
 * @manager: a #BackgroundManager
 *
 * Applies all the changes done in preview mode.  In case the preview mode
 * created a cached image of the new background, the preview cache is renamed
 * to be the main cache; otherwise, a new background with the new setting is
 * created.
 */
gboolean
background_manager_apply_preview (BackgroundManager *manager)
{
  BackgroundManagerPrivate *priv;
  BackgroundData *preview, *normal;
  gboolean changed = FALSE;
  
  g_return_val_if_fail (IS_BACKGROUND_MANAGER (manager), FALSE);

  priv = manager->priv;
  
  if (!priv->is_preview_mode)
    {
      g_warning ("Attempting to save the preview settings but we are not in "
                 "preview mode.  Use background_manager_push_preview_mode() "
                 "to enable preview mode.");
      return FALSE;
    }
  
  g_object_ref (manager);

  preview = priv->preview;
  normal = priv->normal;

  g_debug (G_STRLOC ": preview: [%s] normal: [%s]",
           preview->image_uri ? preview->image_uri : HILDON_HOME_BACKGROUND_NO_IMAGE,
           normal->image_uri  ? normal->image_uri  : HILDON_HOME_BACKGROUND_NO_IMAGE);
  
  if (((preview->image_uri == NULL) && (normal->image_uri != NULL)) ||
      ((preview->image_uri != NULL) && (normal->image_uri == NULL)))
    {
      background_manager_apply_image_uri_internal (manager);
      changed = TRUE;
    }
  else if (((preview->image_uri != NULL) && (normal->image_uri != NULL)) &&
           (strcmp (preview->image_uri, normal->image_uri) != 0))
    {
      background_manager_apply_image_uri_internal (manager);
      changed = TRUE;
    }

  if (preview->mode != normal->mode)
    {
      normal->mode = preview->mode;
      
      g_object_notify (G_OBJECT (manager), "mode");
      
      g_debug (G_STRLOC ": mode changed");
      changed = TRUE;
    }

  if ((preview->color.red != normal->color.red) ||
      (preview->color.green != normal->color.green) ||
      (preview->color.blue != normal->color.blue))
    {
      normal->color.red = preview->color.red;
      normal->color.green = preview->color.green;
      normal->color.blue = preview->color.blue;

      g_object_notify (G_OBJECT (manager), "color");

      g_debug (G_STRLOC ": color changed");
      changed = TRUE;
    }

  if (changed)
    {
      priv->current = priv->normal;
      
      if (!preview->has_cache)
        {
          g_debug (G_STRLOC ": no preview cache found, generating");

          background_manager_create_background (manager, TRUE);
        }
      else
        {
          if (g_unlink (normal->cache) == -1)
            {
              if (errno != ENOENT)
                {
                  g_warning ("Unable to delete the cached background image: %s",
                             g_strerror (errno));
                }
            }

          if (g_rename (preview->cache, normal->cache) == -1)
            {
              if (errno != ENOENT)
                {
                  g_warning ("Unable to move the cached preview image: %s",
                             g_strerror (errno));
                }
            }

         g_debug (G_STRLOC ": renamed the preview cache");
       }

      background_manager_write_configuration (manager);
    }

  g_object_unref (manager);

  return changed;
}

/*
 * background_manager_update_preview:
 * @manager: a #BackgroundManager
 *
 * Forces the creation of a new background image with the current preview
 * settings.
 */
void
background_manager_update_preview (BackgroundManager *manager)
{
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));

  if (!manager->priv->is_preview_mode)
    {
      g_warning ("Attempting to show the preview background but we are not "
                 "in preview mode. Use background_manager_push_preview_mode() "
                 "before showing the preview.");
      return;
    }

  background_manager_create_background (manager, TRUE);
}

/*
 * background_manager_discard_preview:
 * @manager: a #BackgroundManager
 * @reload: whether to reload the current background
 *
 * Forces the reset of the preview settings to the currently saved settings.
 * If @reload is %TRUE it also reloads the background from cache and emits the
 * "changed" signal.
 */
void
background_manager_discard_preview (BackgroundManager *manager,
                                    gboolean           reload)
{
  BackgroundManagerPrivate *priv;
  BackgroundData *preview, *normal;
  
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));

  priv = manager->priv;

  preview = priv->preview;
  normal = priv->normal;
  
  g_free (preview->image_uri);
  preview->image_uri = g_strdup (normal->image_uri);

  preview->mode = normal->mode;

  preview->color.red = normal->color.red;
  preview->color.green = normal->color.green;
  preview->color.blue = normal->color.blue;

  /* reload the old background if we had generated a preview
   * pixmap at least once.
   */
  if (reload && preview->has_cache)
    {
      GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (normal->cache, NULL);
      if (pixbuf)
        {
          g_debug (G_STRLOC ": reloading old background");

          g_signal_emit (manager, manager_signals[CHANGED], 0, pixbuf);

          g_object_unref (pixbuf);
        }
  
      preview->has_cache = FALSE;
    }
}

GdkWindow *
background_manager_get_desktop (BackgroundManager *manager)
{
  g_return_val_if_fail (IS_BACKGROUND_MANAGER (manager), NULL);

  return manager->priv->desktop;
}

void
background_manager_set_desktop (BackgroundManager *manager,
                                GdkWindow         *window)
{
  g_return_if_fail (IS_BACKGROUND_MANAGER (manager));
  g_return_if_fail (window == NULL || GDK_IS_WINDOW (window));

  manager->priv->desktop = window;

  background_manager_create_background (manager, FALSE);
}
