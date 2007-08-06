/*
 * This file is part of hildon-home-webshortcut
 *
 * Copyright (C) 2004, 2005, 2006, 2007 Nokia Corporation.
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

#include "hildon-home-webshortcut.h"
#include "hhws-l10n.h"
#include "hhws-background.h"

#include <libhildondesktop/libhildondesktop.h>
#include <libhildondesktop/hildon-desktop-picture.h>

#include <gtk/gtkmain.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkmenuitem.h>

#include <libosso.h>
#include <hildon-uri.h>
#include <conicconnection.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf-client.h>

#include <dbus/dbus-glib.h>

#include <hildon/hildon-file-chooser-dialog.h>
#include <hildon/hildon-caption.h>
#include <hildon/hildon-defines.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-note.h>

#include <string.h> /* strlen */

#define HHWS_GCONF_IAP          "/apps/osso/apps/hhws/iap"
#define HHWS_GCONF_URI          "/apps/osso/apps/hhws/uri"
#define HHWS_GCONF_IMAGE_URI    "/apps/osso/apps/hhws/image_uri"

#define HILDON_HOME_WS_WIDTH            300
#define HILDON_HOME_WS_HEIGHT           100
#define HILDON_HOME_WS_MINIMUM_WIDTH    120
#define HILDON_HOME_WS_MINIMUM_HEIGHT   60

#define HILDON_HOME_WS_USER_IMAGE_DIR "MyDocs/.images"


struct _HhwsPrivate {
  Picture           picture;
  GtkWidget        *applet_widget;
  GtkWidget        *entry_url;
  GtkWidget        *image_cbox;
  GtkWidget        *csm;
  GtkWidget        *home_win;
  gchar            *iap;
  gchar            *uri;
  gchar            *image;
  gchar            *default_image;
  gchar            *selected_image_uri;
  guint             old_width;
  guint             old_height;
  GConfClient      *gconf_client;
  gdouble           click_x, click_y;
};

HD_DEFINE_PLUGIN_WITH_CODE (Hhws, hhws, HILDON_DESKTOP_TYPE_HOME_ITEM, hhws_background_register_type (module);)

static void
hhws_show_information_note (Hhws *hhws,
                            const gchar *text)
{
  HhwsPrivate  *priv = hhws->priv;
  GtkWidget    *note = NULL;

  note = hildon_note_new_information (priv->home_win?
                                      GTK_WINDOW (priv->home_win):NULL,
                                      text);

  gtk_dialog_run (GTK_DIALOG (note));

  if (note)
    gtk_widget_destroy (GTK_WIDGET (note));
}

static gchar *
hhws_file_to_name (const gchar *filename)
{
  gchar *tmp;
  gchar *imagename;
  gchar *last_dot, *c;

  tmp = g_filename_from_uri (filename, NULL, NULL);
  if (!tmp)
    tmp = g_strdup (filename);

  imagename = g_filename_display_basename (tmp);

  c = imagename;
  last_dot = NULL;

  while (*c)
  {
    if (*c == '.')
    {
      last_dot = c;
    }

    c++;
  }

  if(last_dot)
    *last_dot = '\0';

  /* We need a special case for our wonderful sketch program */
  if (g_str_has_suffix (imagename, ".sketch"))
  {
    tmp = imagename;
    imagename = g_strndup (tmp, strlen (tmp) - 7);
    g_free (tmp);
  }

  return imagename;
}

static void
hhws_process_error (Hhws *hhws,
                    GError *error)
{
  gchar *text = NULL;

#if 0
  if (error->domain == hhws_loader_error_quark ())
    {
      switch (error->code)
        {
          case HHWS_LOADER_ERROR_MMC_OPEN:
              text = HHWS_MMC_NOT_OPEN_TEXT;
              break;
          case HHWS_LOADER_ERROR_CORRUPTED:
              text = HHWS_CORRUPTED_FILE;
              break;
#if 0
          case HHWS_LOADER_ERROR_MEMORY:
              text = 
#endif
          case HHWS_LOADER_ERROR_OPEN_FAILED:
              text = HHWS_COULD_NOT_OPEN;
              break;
#if 0
          case HHWS_LOADER_ERROR_IO:
              text = 
#endif
          default:
              break;

        }
    }

  else if (error->domain == G_FILE_ERROR)
    {
      switch (error->code)
        {
          case G_FILE_ERROR_NOSPC:
              text = HHWS_FLASH_FULL;
              break;
          default:
              break;
        }
    }

  else if (error->domain == GDK_PIXBUF_ERROR)
    {
      switch (error->code)
        {
          case GDK_PIXBUF_ERROR_CORRUPT_IMAGE:
              text = HHWS_CORRUPTED_FILE;
              break;
          case GDK_PIXBUF_ERROR_FAILED: /* seems to happen with some jpg */
              text = HHWS_CORRUPTED_FILE;
              break;
          case GDK_PIXBUF_ERROR_UNKNOWN_TYPE:
              text = HHWS_NOT_SUPPORTED;
              break;
          default:
              break;
        }
    }
#endif

  dbus_g_error_get_name (error);

  if (dbus_g_error_has_name (error,
                             "org.freedesktop.DBus.GLib.UnmappedError.BackgroundManagerErrorQuark.Code2"))
    text = HHWS_CORRUPTED_TEXT;
  else if ((dbus_g_error_has_name (error,
                                   "org.freedesktop.DBus.GLib.UnmappedError.BackgroundManagerErrorQuark.Code0")))
    text = HHWS_LOW_MEMORY_TEXT;
  else if ((dbus_g_error_has_name (error,
                                   "org.freedesktop.DBus.GLib.UnmappedError.BackgroundManagerErrorQuark.Code1")))
    text = HHWS_NO_CONNECTION_TEXT;
  else if ((dbus_g_error_has_name (error,
                                   "org.freedesktop.DBus.GLib.UnmappedError.BackgroundManagerErrorQuark.Code6")))
    text = HHWS_FLASH_FULL_TEXT;
  else if ((dbus_g_error_has_name (error,
                                   "org.freedesktop.DBus.GLib.UnmappedError.BackgroundManagerErrorQuark.Code7")))
    text = HHWS_NO_CONNECTION_TEXT;
  else if ((dbus_g_error_has_name (error,
                                   "org.freedesktop.DBus.GLib.UnmappedError.GdkPixbufErrorQuark.Code1")))
    text = HHWS_LOW_MEMORY_TEXT;
  else if ((dbus_g_error_has_name (error,
                                   "org.freedesktop.DBus.GLib.UnmappedError.GdkPixbufErrorQuark.Code3")))
    text = HHWS_CORRUPTED_TEXT;
  else if ((dbus_g_error_has_name (error,
                                   "org.freedesktop.DBus.GLib.UnmappedError.GdkPixbufErrorQuark.Code5")))
    text = HHWS_CORRUPTED_TEXT;


  if (text)
    hhws_show_information_note (hhws, text);

}

static void
hhws_apply_background_callback (HildonDesktopBackground        *background,
                                Picture                         picture,
                                GError                         *error,
                                Hhws                           *hhws)
{
  HhwsPrivate  *priv = hhws->priv;

  g_debug ("webshortcut background applied");

  if (error)
  {
    g_debug ("Error occurred: %s", error->message);
    hhws_process_error (hhws, error);
    return;
  }

  if (priv->picture)
    XRenderFreePicture (GDK_DISPLAY (), priv->picture);

  priv->picture = picture;

  gtk_widget_queue_draw (GTK_WIDGET (hhws));

}

static void
hhws_apply_and_save_background_callback (HildonDesktopBackground *background,
                                         Picture                  picture,
                                         GError                  *error,
                                         Hhws                    *hhws)
{
  HhwsPrivate  *priv = hhws->priv;
  GError       *save_error = NULL;

  hhws_apply_background_callback (background, picture, error, hhws);

  if (error) return;

  hildon_desktop_background_save (background,
                                  NULL,
                                  &save_error);

  priv->image = g_strdup (hildon_desktop_background_get_filename (background));

  if (save_error)
  {
    g_warning ("Error when saving background: %s", save_error->message);
    g_error_free (save_error);
  }

}

static void
hhws_apply_and_save_background (Hhws *hhws, HildonDesktopBackground *background)
{
  if (!GTK_WIDGET_REALIZED (hhws))
    return;

  hildon_desktop_background_apply_async (background,
                                         GTK_WIDGET (hhws)->window,
                                         (HildonDesktopBackgroundApplyCallback)
                                         hhws_apply_and_save_background_callback,
                                         hhws);

}

static void
hhws_apply_background (Hhws *hhws, HildonDesktopBackground *background)
{
  if (!GTK_WIDGET_REALIZED (hhws))
    return;

  hildon_desktop_background_apply_async (background,
                                         GTK_WIDGET (hhws)->window,
                                         (HildonDesktopBackgroundApplyCallback)
                                         hhws_apply_background_callback,
                                         hhws);

}

static void
hhws_set_uri (Hhws *hhws, const gchar *uri)
{
  HhwsPrivate  *priv = hhws->priv;
  GSList       *actions         = NULL;
  GError       *error           = NULL;

  if (!g_strrstr (uri, ":"))
  {
    /* No : in the URL, adding http:// */
    gchar *http_url = g_strconcat("http://", uri, NULL);

    g_free (priv->uri);
    priv->uri = http_url;

    return;
  }

  actions = hildon_uri_get_actions_by_uri (uri,
                                           HILDON_URI_ACTION_NEUTRAL,
                                           &error);

  if (error)
  {
    hhws_show_information_note (hhws, HHWS_INVALID_URL);
    g_error_free (error);
  }
  else
  {
    g_free (priv->uri);
    priv->uri = g_strdup (uri);
  }

  if (actions)
    hildon_uri_free_actions (actions);

}

static void
hhws_load_configuration (Hhws *hhws)
{
  HhwsPrivate  *priv = hhws->priv;
  GError       *error = NULL;
  GConfValue   *gconfval=NULL;

  g_return_if_fail (priv && priv->gconf_client);

  priv->uri = gconf_client_get_string (priv->gconf_client,
                                       HHWS_GCONF_URI,
                                       &error);

  if (error) goto error;

  priv->iap = gconf_client_get_string (priv->gconf_client,
                                       HHWS_GCONF_IAP,
                                       &error);

  if (error) goto error;

  gconfval = gconf_client_get_default_from_schema (priv->gconf_client,
                                                   HHWS_GCONF_IMAGE_URI,
                                                   &error);
  if (error) goto error;
  if (gconfval)
  {
    priv->default_image = g_strdup (gconf_value_get_string (gconfval));
    gconf_value_free (gconfval);
  }

error:
  if (error)
    {
      g_critical ("Error when retrieving gconf configuration: %s",
                  error->message);
      g_error_free (error);
      g_free (priv->iap);
      priv->iap = NULL;
      g_free (priv->uri);
      priv->uri = NULL;
    }
}

static void
hhws_save_configuration (Hhws *hhws)
{
  HhwsPrivate  *priv = hhws->priv;
  GError       *error = NULL;

  g_return_if_fail (priv->gconf_client);

  if (priv->uri)
    gconf_client_set_string (priv->gconf_client,
                             HHWS_GCONF_URI,
                             priv->uri,
                             &error);

  if (error)
    {
      g_critical ("Error when saving URI to gconf: %s",
                  error->message);
      g_error_free (error);
    }
}

static gchar *
hhws_get_user_image_dir()
{
  gchar *image_dir;

  image_dir = g_build_path("/",
                           g_getenv ("HOME"),
                           HILDON_HOME_WS_USER_IMAGE_DIR,
                           NULL);

  return image_dir;
}

static gboolean
hhws_select_file_dialog (Hhws *hhws)
{
  HhwsPrivate      *priv = hhws->priv;
  GtkWidget        *dialog = NULL;
  GtkWidget        *combo_box = priv->image_cbox;
  GtkFileFilter    *mime_type_filter;
  gint              response;
  gchar            *label_str = NULL;
  gchar            *dir = NULL;

  dialog = hildon_file_chooser_dialog_new (GTK_WINDOW (priv->home_win),
                                           GTK_FILE_CHOOSER_ACTION_OPEN);

  gtk_window_set_title (GTK_WINDOW (dialog),
                        HHWS_FILE_CHOOSER_TITLE);

  mime_type_filter = gtk_file_filter_new();
  gtk_file_filter_add_mime_type (mime_type_filter, "image/jpeg");
  gtk_file_filter_add_mime_type (mime_type_filter, "image/gif");
  gtk_file_filter_add_mime_type (mime_type_filter, "image/png");
  gtk_file_filter_add_mime_type (mime_type_filter, "image/bmp");
  gtk_file_filter_add_mime_type (mime_type_filter, "image/tiff");
  gtk_file_filter_add_mime_type (mime_type_filter, "sketch/png");
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog),
                               mime_type_filter);

  dir = hhws_get_user_image_dir ();
  if (!gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), dir))
    g_warning ("Could not set folder %s", dir);

  g_free (dir);

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  if (response == GTK_RESPONSE_OK)
  {
    g_free (priv->selected_image_uri);
    priv->selected_image_uri =
        gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));

    label_str = hhws_file_to_name (priv->selected_image_uri);

    if (label_str && label_str[0] != '\0')
    {
      gtk_combo_box_remove_text(GTK_COMBO_BOX(combo_box), 1);
      gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box), label_str);
      gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 1);
    }
    g_free (label_str);
  }

  gtk_widget_destroy (dialog);
  return TRUE;
}


static void
hhws_settings_dialog_response (GtkWidget *dialog,
                               gint arg,
                               Hhws *hhws)
{
  HhwsPrivate  *priv = hhws->priv;
  const gchar  *new_url_str;

  switch (arg)
  {
      case GTK_RESPONSE_OK:
          new_url_str = gtk_entry_get_text (GTK_ENTRY (priv->entry_url));

          if (g_str_equal(new_url_str, ""))
          {
            hildon_banner_show_information (priv->home_win,
                                            NULL,
                                            HHWS_URL_REQ);

            g_signal_stop_emission_by_name (dialog, "response");

          }
          else
          {
            const gchar              *new_image_path = NULL;
            HildonDesktopBackground  *background;

            if (gtk_combo_box_get_active (GTK_COMBO_BOX (priv->image_cbox)) == 0)
            {
              /* Use the original image */
              new_image_path = priv->default_image;
            }
            else
            {
              /* Use user selected one */
              if (priv->selected_image_uri)
                new_image_path = priv->selected_image_uri;
              else
                new_image_path = priv->image;
            }

            /* DO THE MAGIC HERE */
            background = g_object_new (HHWS_TYPE_BACKGROUND,
                                       "filename", new_image_path,
                                       NULL);

            hhws_apply_and_save_background (hhws, background);
            g_object_unref (background);

            g_free (priv->selected_image_uri);
            priv->selected_image_uri = NULL;

            hhws_set_uri (hhws, new_url_str);
            hhws_save_configuration (hhws);

            gtk_widget_destroy (GTK_WIDGET (dialog));
          }
          break;

      case GTK_RESPONSE_APPLY:
          g_signal_stop_emission_by_name (dialog, "response");
          hhws_select_file_dialog (hhws);
          break;

      case GTK_RESPONSE_CANCEL:
          gtk_widget_destroy (GTK_WIDGET (dialog));
      default:
          break;
  }
}

static void
hhws_settings_dialog (Hhws *hhws)
{
  HhwsPrivate  *priv = hhws->priv;
  GtkWidget    *dialog = NULL;

  GtkSizeGroup *group;
  GtkWidget    *caption_image;

  GtkWidget    *caption_url;

  dialog = gtk_dialog_new_with_buttons (HHWS_SET_TITLE,
                                        GTK_WINDOW (priv->home_win),
                                        GTK_DIALOG_MODAL |
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                        HHWS_SET_OK,
                                        GTK_RESPONSE_OK,
                                        HHWS_SET_BROWSE,
                                        GTK_RESPONSE_APPLY,
                                        HHWS_SET_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);

  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

  group = GTK_SIZE_GROUP (gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL));

  priv->image_cbox = gtk_combo_box_new_text ();

  if (priv->default_image)
  {
    gchar *name = hhws_file_to_name (priv->default_image);

    gtk_combo_box_append_text (GTK_COMBO_BOX(priv->image_cbox),
                               name);

    g_free (name);
  }

  if (priv->image && !(priv->default_image &&
                       g_str_equal (priv->default_image, priv->image)))
  {
    gchar *name = hhws_file_to_name (priv->image);

    gtk_combo_box_append_text (GTK_COMBO_BOX(priv->image_cbox),
                               name);
    gtk_combo_box_set_active(GTK_COMBO_BOX(priv->image_cbox), 1);
    g_free (name);
  }
  else
    gtk_combo_box_set_active (GTK_COMBO_BOX(priv->image_cbox), 0);

  caption_image = hildon_caption_new (group,
                                      HHWS_SET_IMAGE,
                                      priv->image_cbox,
                                      NULL,
                                      HILDON_CAPTION_OPTIONAL);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
                     caption_image);

  priv->entry_url = gtk_entry_new ();
  g_object_set (G_OBJECT (priv->entry_url),
                "hildon-input-mode",
                HILDON_GTK_INPUT_MODE_ALPHA|HILDON_GTK_INPUT_MODE_NUMERIC|
                HILDON_GTK_INPUT_MODE_SPECIAL);

  if (priv->uri)
    gtk_entry_set_text (GTK_ENTRY (priv->entry_url),
                        priv->uri);

  caption_url = hildon_caption_new (group,
                                    HHWS_SET_URL,
                                    priv->entry_url,
                                    NULL,
                                    HILDON_CAPTION_OPTIONAL);


  gtk_container_add (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),
                     caption_url);

  gtk_widget_show_all (dialog);

  g_signal_connect (G_OBJECT(dialog), "response",
                    G_CALLBACK (hhws_settings_dialog_response),
                    hhws);

  g_object_unref (G_OBJECT (group));

}

static void
hhws_launch_uri (Hhws *hhws)
{
  HhwsPrivate      *priv = hhws->priv;
  GError           *error = NULL;

  if (!priv->uri)
    return;

  if (!hildon_uri_open (priv->uri, NULL /* default action */, &error ))
  {
    g_warning ("Could not launch URI %s: %s", priv->uri, error->message);
    g_error_free (error);
  }
}

static gboolean
hhws_button_press (GtkWidget           *widget,
                   GdkEventButton      *event)
{
  Hhws             *hhws = HHWS (widget);
  HhwsPrivate      *priv = hhws->priv;

  priv->click_x = event->x_root;
  priv->click_y = event->y_root;

  return GTK_WIDGET_CLASS (hhws_parent_class)->button_press_event (widget,
                                                                   event);
}

static gboolean
hhws_button_release (GtkWidget         *widget,
                     GdkEventButton    *event)
{
  Hhws             *hhws = HHWS (widget);
  HhwsPrivate      *priv = hhws->priv;

  if ((ABS (priv->click_x - event->x_root)  > 15.0 )  ||
      (ABS (priv->click_y - event->y_root)) > 15.0)
  {
    return GTK_WIDGET_CLASS (hhws_parent_class)->button_release_event (widget,
                                                                       event);
  }

  if (priv->iap)
  {
    ConIcConnection  *connection;

    connection = con_ic_connection_new ();

    if (!con_ic_connection_connect_by_id (connection,
                                          priv->iap,
                                          CON_IC_CONNECT_FLAG_NONE))
      g_critical ("con_ic_connection_connect_by_id failed");

    g_object_unref (connection);
  }

  hhws_launch_uri (hhws);

  return GTK_WIDGET_CLASS (hhws_parent_class)->button_release_event (widget,
                                                                     event);
}

static void
hhws_realize (GtkWidget *widget)
{
  HildonDesktopBackground      *background;
  HhwsPrivate                  *priv = HHWS (widget)->priv;

  GTK_WIDGET_CLASS (hhws_parent_class)->realize (widget);

  background = g_object_new (HHWS_TYPE_BACKGROUND,
                             NULL);

  hildon_desktop_background_load (background, NULL, NULL);
  priv->image = g_strdup (hildon_desktop_background_get_filename (background));
  hhws_apply_background (HHWS (widget), background);

  g_object_unref (background);

}

static gboolean
hhws_expose (GtkWidget         *widget,
             GdkEventExpose    *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
  {
    HhwsPrivate      *priv = HHWS (widget)->priv;
    GdkDrawable      *drawable;
    gint              x_offset, y_offset;

    gdk_window_get_internal_paint_info (widget->window,
                                        &drawable,
                                        &x_offset, &y_offset);

    gtk_paint_box (widget->style,
                   widget->window,
                   GTK_STATE_NORMAL,
                   GTK_SHADOW_NONE,
                   &event->area,
                   widget,
                   NULL,
                   0,
                   0,
                   widget->allocation.width,
                   widget->allocation.height
                  );

    if (priv->picture != None)
    {
      Picture           widget_picture;

      widget_picture = hildon_desktop_picture_from_drawable (drawable);

      if (widget_picture == None)
        return FALSE;

      XRenderComposite (GDK_DISPLAY (),
                        PictOpSrc,
                        priv->picture,
                        None,
                        widget_picture,
                        15, 15,
                        15, 15,
                        15 - x_offset, 15 - y_offset,
                        widget->allocation.width  - 30,
                        widget->allocation.height - 30);

    }

    return GTK_WIDGET_CLASS (hhws_parent_class)->expose_event (widget,
                                                               event);
  }

  return FALSE;
}

static GtkWidget *
hhws_create_csm (Hhws *hhws)
{
  GtkWidget *menu;
  GtkWidget *item;

  menu = gtk_menu_new ();

  item = gtk_menu_item_new_with_label (HHWS_SETTINGS);
  g_signal_connect_swapped (G_OBJECT (item), "activate",
                            G_CALLBACK (hhws_settings_dialog),
                            hhws);

  gtk_container_add (GTK_CONTAINER (menu), item);

  item = gtk_separator_menu_item_new ();
  gtk_container_add (GTK_CONTAINER (menu), item);

  item = gtk_menu_item_new_with_label (HHWS_CLOSE);
  g_signal_connect_swapped (G_OBJECT (item), "activate",
                            G_CALLBACK (gtk_widget_destroy),
                            hhws);

  gtk_container_add (GTK_CONTAINER (menu), item);

  gtk_widget_show_all (menu);

  return menu;
}

static void
hhws_destroy (GtkObject *object)
{
  Hhws             *hhws = HHWS (object);
  HhwsPrivate      *priv = hhws->priv;

  g_free (priv->iap);
  priv->iap = NULL;

  g_free (priv->uri);
  priv->uri = NULL;

  g_free (priv->selected_image_uri);
  priv->selected_image_uri = NULL;

  if (priv->picture != None)
  {
    XRenderFreePicture (GDK_DISPLAY (), priv->picture);
    priv->picture = None;
  }

  if (priv->gconf_client)
  {
    g_object_unref (priv->gconf_client);
    priv->gconf_client = NULL;
  }
}

static GtkWidget *
hhws_settings (HildonDesktopHomeItem   *applet,
               GtkWidget               *parent)
{
  Hhws         *hhws = HHWS (applet);
  HhwsPrivate  *priv = hhws->priv;
  GtkWidget    *item;

  priv->home_win = parent;
  item = gtk_menu_item_new_with_label (HHWS_TITLEBAR_MENU);

  g_signal_connect_swapped (G_OBJECT (item), "activate",
                            G_CALLBACK (hhws_settings_dialog),
                            hhws);

  return item;
}

static void
hhws_init (Hhws *hhws)
{
  HhwsPrivate  *priv;

  priv = hhws->priv =
      G_TYPE_INSTANCE_GET_PRIVATE ((hhws), HILDON_TYPE_HHWS, HhwsPrivate);

  priv->gconf_client = gconf_client_get_default ();
  hhws_load_configuration (hhws);

  gtk_widget_set_size_request (GTK_WIDGET (hhws),
                               HILDON_HOME_WS_WIDTH,
                               HILDON_HOME_WS_HEIGHT);

  priv->csm = hhws_create_csm (hhws);

  gtk_widget_tap_and_hold_setup (GTK_WIDGET (hhws),
                                 priv->csm,
                                 NULL /* position function */,
                                 0 /* flags, deprecated */);

  g_object_set (hhws,
                "resize-type",          HILDON_DESKTOP_HOME_ITEM_RESIZE_NONE,
#if 0
                "minimum-width",        HILDON_HOME_WS_MINIMUM_WIDTH,
                "minimum-height",       HILDON_HOME_WS_MINIMUM_HEIGHT,
#endif
                NULL);
}

static void
hhws_class_init (HhwsClass *klass)
{
  GtkWidgetClass                       *widget_class;
  GtkObjectClass                       *object_class;
  HildonDesktopHomeItemClass           *applet_class;

  widget_class = GTK_WIDGET_CLASS (klass);
  applet_class = HILDON_DESKTOP_HOME_ITEM_CLASS (klass);
  object_class = GTK_OBJECT_CLASS (klass);

  applet_class->settings = hhws_settings;

  widget_class->button_press_event   = hhws_button_press;
  widget_class->button_release_event = hhws_button_release;
  widget_class->expose_event         = hhws_expose;
  widget_class->realize              = hhws_realize;

  object_class->destroy = hhws_destroy;

  g_type_class_add_private (klass, sizeof (HhwsPrivate));
}
