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
#include "hhwsloader.h"
#include "hhws-l10n.h"

#include <libhildondesktop/libhildondesktop.h>

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
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>


#include <osso-ic.h>
#include <libosso.h>
#include <osso-uri.h>


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf-client.h>

#include "hildon-widgets/hildon-file-chooser-dialog.h"
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-defines.h>

#define HHWS_GCONF_IAP          "/apps/osso/apps/hhws/iap"
#define HHWS_GCONF_URI          "/apps/osso/apps/hhws/uri"

#define HILDON_HOME_WS_WIDTH 290
#define HILDON_HOME_WS_HEIGHT 134
#define HILDON_HOME_WS_ENV_HOME "HOME"
#define HILDON_HOME_WS_SYSTEM_DIR ".osso/hildon-home"
#define HILDON_HOME_WS_USER_FILE "hildon_home_wshortcut.png"
#define HILDON_HOME_WS_ORGINAL_FILE "wshortcut_image_filename.txt"
#define HILDON_HOME_WS_URL_FILE "wshortcut_url_path.txt"

#define HILDON_HOME_WS_USER_IMAGE_DIR "MyDocs/.images"
#define HILDON_HOME_WS_DEFAULT_URL "http://www.nokia.com/"

#define HILDON_HOME_WS_NAME "WebShortcut"
#define HILDON_HOME_WS_VERSION "1.0.0"
#define BUF_SIZE 8192

#define HILDON_HOME_WS_DIALOG_PADDING 10
#define HILDON_HOME_WS_GCONF_MMC_COVER_OPEN "/system/osso/af/mmc-cover-open"
#define HILDON_HOME_WS_ENV_MMC_MOUNTPOINT   "MMC_MOUNTPOINT"
#define HILDON_HOME_WS_IMAGE_FORMAT         "png"

#define HILDON_HOME_WS_IMAGE_FILLCOLOR 0xffffffff

struct _HhwsPrivate {
  HhwsLoader       *loader;
  GtkWidget        *applet_widget;
  GtkWidget        *image;
  GtkWidget        *entry_url;
  GtkWidget        *image_cbox;
  GtkWidget        *csm;
  GtkWidget        *home_win;
  gchar            *iap;
  gchar            *uri;
  gchar            *image_uri;
  gchar            *selected_image_uri;
  guint             old_width;
  guint             old_height;
  GConfClient      *gconf_client;
};

HD_DEFINE_PLUGIN_WITH_CODE (Hhws, hhws, HILDON_TYPE_HOME_APPLET, hhws_loader_register_type (module);)

static void
hhws_show_information_note (Hhws *hhws,
                            const gchar *text)
{
  HhwsPrivate  *priv = hhws->priv;
  GtkWidget    *note = NULL;

  note = hildon_note_new_information (GTK_WINDOW (priv->home_win),
                                      text);

  gtk_dialog_run (GTK_DIALOG (note));

  if (note)
    gtk_widget_destroy (GTK_WIDGET (note));
}

static void
hhws_loader_error_cb (HhwsLoader *loader,
                      GError *error,
                      Hhws *hhws)
{
  gchar *text = NULL;
  g_return_if_fail (error);

  g_warning ("Loading of image failed: %s", error->message);

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
          case GDK_PIXBUF_ERROR_UNKNOWN_TYPE:
              text = HHWS_NOT_SUPPORTED;
              break;
          default:
              break;
        }
    }

  if (text)
    hhws_show_information_note (hhws, text);
  
}

static void
hhws_load_configuration (Hhws *hhws)
{
  HhwsPrivate  *priv = hhws->priv;
  GError       *error = NULL;

  g_return_if_fail (priv && priv->gconf_client);

  priv->uri = gconf_client_get_string (priv->gconf_client,
                                       HHWS_GCONF_URI,
                                       &error);

  if (error) goto error;

  priv->iap = gconf_client_get_string (priv->gconf_client,
                                       HHWS_GCONF_IAP,
                                       &error);

  if (error) goto error;

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

      label_str = hhws_url_to_filename (priv->selected_image_uri);

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
              const gchar  *new_image_path = NULL;
              gulong        error_handler = 0;

              if (gtk_combo_box_get_active (GTK_COMBO_BOX (priv->image_cbox)) == 0)
                {
                  /* Use the original image */
                  new_image_path = hhws_loader_get_default_uri (priv->loader);
                }
              else
                {
                  /* Use user selected one */
                  if (priv->selected_image_uri)
                    new_image_path = priv->selected_image_uri;
                  else
                    new_image_path = hhws_loader_get_uri (priv->loader);
                }

              g_debug ("User selected: %s", new_image_path);
              error_handler = g_signal_connect (priv->loader,
                                                "loading-failed",
                                                G_CALLBACK (hhws_loader_error_cb),
                                                priv);

              hhws_loader_set_uri (priv->loader, new_image_path);
              g_signal_handler_disconnect (priv->loader, error_handler);

              g_free (priv->uri);

              if (!g_strrstr (new_url_str, ":"))
                {
                  /* No : in the URL, adding http:// */
                  gchar *http_url = g_strconcat("http://", new_url_str, NULL);

                  priv->uri = http_url;
                }
              else
                priv->uri = g_strdup (new_url_str);

              hhws_save_configuration (hhws);

              g_free (priv->selected_image_uri);
              priv->selected_image_uri = NULL;
              
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

  const gchar  *default_image_name;
  const gchar  *image_name;

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

  g_return_if_fail (dialog);

  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

  group = GTK_SIZE_GROUP (gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL));

  priv->image_cbox = gtk_combo_box_new_text ();

  default_image_name = hhws_loader_get_default_image_name (priv->loader);
  image_name = hhws_loader_get_image_name (priv->loader);

  if (default_image_name)
    gtk_combo_box_append_text (GTK_COMBO_BOX(priv->image_cbox),
                               default_image_name);

  if (image_name && !g_str_equal (default_image_name, image_name))
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX(priv->image_cbox),
                                 image_name);
      gtk_combo_box_set_active(GTK_COMBO_BOX(priv->image_cbox), 1);
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

  if (!osso_uri_open (priv->uri, NULL /* default action */, &error ))
    {
      g_warning ("Could not launch URI %s: %s", priv->uri, error->message);
      g_error_free (error);
    }
}


static gboolean
hhws_button_release (GtkWidget         *widget,
                     GdkEventButton    *event)
{
  Hhws             *hhws = HHWS (widget);
  HhwsPrivate      *priv = hhws->priv;
  gint              applet_x, applet_y;

  /* Get the widget's root coordinates */
  gdk_window_get_origin (widget->window, &applet_x, &applet_y);
  if (GTK_WIDGET_NO_WINDOW (widget))
    {
      applet_x += widget->allocation.x;
      applet_y += widget->allocation.y;
    }

  if((gint)event->x_root < applet_x || 
     (gint)event->x_root > applet_x + widget->allocation.width ||
     (gint)event->y_root < applet_y ||
     (gint)event->y_root > applet_y + widget->allocation.height)
    {
      /* Released outside the applet */
      return GTK_WIDGET_CLASS (hhws_parent_class)->button_release_event (widget,
                                                                         event);
    }

  if (priv->iap) 
    {
      if (osso_iap_connect (priv->iap,
                            OSSO_IAP_REQUESTED_CONNECT,
                            NULL) != OSSO_OK)
        g_critical ("osso_iap_connect != OSSO_OK");
    } 
  else
    hhws_launch_uri (hhws);

  return GTK_WIDGET_CLASS (hhws_parent_class)->button_release_event (widget,
                                                                     event);
}

static void
hhws_reload_pixbuf (Hhws *hhws)
{
  HhwsPrivate      *priv = hhws->priv;
  GdkPixbuf        *pixbuf;

  pixbuf = hhws_loader_request_pixbuf (priv->loader,
                                       priv->image->allocation.width,
                                       priv->image->allocation.height);

  if (pixbuf)
    {
      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), pixbuf);
      g_object_unref (pixbuf);
    }
}

static void
hhws_image_size_allocate (Hhws             *hhws,
                          GtkAllocation    *allocation)
{
  HhwsPrivate  *priv = hhws->priv;

  if (priv->old_width  != allocation->width ||
      priv->old_height != allocation->height)
    {
      priv->old_width  = allocation->width;
      priv->old_height = allocation->height;
      hhws_reload_pixbuf (hhws);
    }
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
hhws_launch_iap_cb (struct iap_event_t *event,
                    Hhws *hhws)
{    
  HhwsPrivate  *priv = hhws->priv;

  switch(event->type)
    {
      case OSSO_IAP_CONNECTED:
          hhws_launch_uri (hhws);
          break;
      case OSSO_IAP_ERROR:
          hildon_banner_show_information (priv->home_win,
                                          NULL,
                                          HHWS_IAP_SCAN_FAIL);
          break;
      default:
          break;
    }
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

  if (priv->loader)
    {
      g_object_unref (priv->loader);
      priv->loader = NULL;
    }

  if (priv->gconf_client)
    {
      g_object_unref (priv->gconf_client);
      priv->gconf_client = NULL;
    }
}

static GtkWidget *
hhws_settings (HildonHomeApplet    *applet,
               GtkWidget           *parent)
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
  GtkWidget    *frame;
  GtkWidget    *alignment;
  gchar        *cache_file = NULL;

  cache_file = g_build_path (G_DIR_SEPARATOR_S,
                             g_getenv ("HOME"),
                             HILDON_HOME_WS_SYSTEM_DIR,
                             HILDON_HOME_WS_USER_FILE,
                             NULL);

  priv = hhws->priv =
      G_TYPE_INSTANCE_GET_PRIVATE ((hhws), HILDON_TYPE_HHWS, HhwsPrivate);

  priv->loader = g_object_new (HHWS_TYPE_LOADER,
                               "cache-file", cache_file, 
                               NULL);

  g_free (cache_file);

  g_signal_connect_swapped (priv->loader, "pixbuf-changed",
                            G_CALLBACK (hhws_reload_pixbuf),
                            hhws);

  priv->gconf_client = gconf_client_get_default ();
  hhws_load_configuration (hhws);

  if (priv->iap != NULL) 
    {
      gint iap_ok = osso_iap_cb ((osso_iap_cb_t)hhws_launch_iap_cb);

      if(iap_ok != OSSO_OK)
        {
          g_free (priv->iap);
          priv->iap = NULL;
        }
    }

  gtk_widget_set_size_request (GTK_WIDGET (hhws), 
                               HILDON_HOME_WS_WIDTH, 
                               HILDON_HOME_WS_HEIGHT);

  priv->image = gtk_image_new();
  
  g_signal_connect_swapped (priv->image, "size-allocate",
                            G_CALLBACK (hhws_image_size_allocate),
                            hhws);

  frame = gtk_frame_new (NULL/*label*/);
  gtk_widget_set_name (frame, "osso-speeddial"/*FIXME give it its own name*/);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  alignment = gtk_alignment_new (0.5,
                                 0.5,
                                 1.0,
                                 1.0);

  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 15, 15, 15, 15);

  gtk_container_add (GTK_CONTAINER (alignment), priv->image);
  gtk_container_add (GTK_CONTAINER (frame), alignment);
  gtk_container_add (GTK_CONTAINER (hhws), frame);

  gtk_widget_show (priv->image);
  gtk_widget_show (frame);

  priv->csm = hhws_create_csm (hhws);

  gtk_widget_tap_and_hold_setup (GTK_WIDGET (hhws),
                                 priv->csm,
                                 NULL /* position function */,
                                 0 /* flags, deprecated */);
}

static void
hhws_class_init (HhwsClass *klass)
{
  GtkWidgetClass           *widget_class;
  GtkObjectClass           *object_class;
  HildonHomeAppletClass    *applet_class;

  widget_class = GTK_WIDGET_CLASS (klass);
  applet_class = HILDON_HOME_APPLET_CLASS (klass);
  object_class = GTK_OBJECT_CLASS (klass);

  applet_class->settings = hhws_settings;

  widget_class->button_release_event = hhws_button_release;

  object_class->destroy = hhws_destroy;

  g_type_class_add_private (klass, sizeof (HhwsPrivate));
}
