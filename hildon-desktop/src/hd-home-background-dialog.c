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
#include <config.h>
#endif

#ifdef HAVE_LIBHILDONHELP
#include <hildon/hildon-help.h>
#endif

#ifdef HAVE_LIBOSSO
#include <libosso.h>
#endif

#ifdef HAVE_HILDON_FM2
#include <hildon/hildon-file-chooser-dialog.h>
#elif defined HAVE_HILDON_FM
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#else
#include <gtk/gtkfilechooserdialog.h>
#endif

#ifdef HAVE_LIBHILDON
#include <hildon/hildon-color-button.h>
#include <hildon/hildon-caption.h>
#else
#include <hildon-widgets/hildon-color-button.h>
#include <hildon-widgets/hildon-caption.h>
#endif

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>

#include <gtk/gtktreemodel.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcelllayout.h>

#include "hd-home-background-dialog.h"
#include "hd-home-l10n.h"
#include "hd-home-background.h"
#include "background-manager/hildon-background-manager.h"

#define BG_DESKTOP_GROUP           "Desktop Entry"
#define BG_DESKTOP_IMAGE_NAME      "Name"
#define BG_DESKTOP_IMAGE_FILENAME  "File"
#define BG_DESKTOP_IMAGE_PRIORITY  "X-Order"
#define BG_IMG_INFO_FILE_TYPE      "desktop"

#define HOME_BG_IMG_DEFAULT_PRIORITY  15327 /* this is a random number */

#define HILDON_HOME_HC_USER_IMAGE_DIR    "MyDocs/.images"

#define HILDON_HOME_FILE_CHOOSER_ACTION_PROP  "action"
#define HILDON_HOME_FILE_CHOOSER_TITLE_PROP   "title"
#define HILDON_HOME_FILE_CHOOSER_TITLE        _("home_ti_select_image")
#define HILDON_HOME_FILE_CHOOSER_SELECT_PROP  "open-button-text"
#define HILDON_HOME_FILE_CHOOSER_SELECT       _("home_bd_select_image")
#define HILDON_HOME_FILE_CHOOSER_EMPTY_PROP   "empty-text"
#define HILDON_HOME_FILE_CHOOSER_EMPTY        _("home_li_no_images")

#define HILDON_HOME_BACKGROUND_NO_IMAGE  "no_image"


#define HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HD_TYPE_HOME_BACKGROUND_DIALOG, HDHomeBackgroundDialogPrivate))

G_DEFINE_TYPE (HDHomeBackgroundDialog, hd_home_background_dialog, GTK_TYPE_DIALOG);

enum 
{ 	 
  BG_IMAGE_NAME, 	 
  BG_IMAGE_FILENAME, 	 
  BG_IMAGE_PRIORITY
}; 

enum
{
  PROP_0,
  PROP_BACKGROUND_DIR,
  PROP_OSSO_CONTEXT,
  PROP_BACKGROUND
};

struct _HDHomeBackgroundDialogPrivate
{
  gchar            *background_dir;
  GtkListStore     *combobox_contents;
  GtkTreePath      *custom_image;
  GtkWidget        *color_button;
  GtkWidget        *img_combo;
  GtkWidget        *mode_combo;
  gpointer          osso_context;
  
  HDHomeBackground *background;
};

static void
hd_home_background_dialog_set_property (GObject      *gobject,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec);

static void
hd_home_background_dialog_get_property (GObject      *gobject,
                                        guint         prop_id,
                                        GValue *value,
                                        GParamSpec   *pspec);


static void
hd_home_background_dialog_response (GtkDialog *dialog, 
                                    gint       arg);
static void
hd_home_background_dialog_set_background_dir (HDHomeBackgroundDialog *dialog,
                                              const gchar *dir);

static void
hd_home_background_dialog_background_dir_changed 
                                    (HDHomeBackgroundDialog *dialog);

static void
hd_home_background_dialog_set_background (HDHomeBackgroundDialog *dialog,
                                          HDHomeBackground *background);
static void
hd_home_background_dialog_sync_from_background (HDHomeBackgroundDialog *dialog);

static void
hd_home_background_dialog_filename_changed (HDHomeBackgroundDialog *dialog);

static void
hd_home_background_dialog_color_changed (HDHomeBackgroundDialog *dialog);

static void
hd_home_background_dialog_mode_changed (HDHomeBackgroundDialog *dialog);

static void
hd_home_background_dialog_finalize (GObject *object);

static gchar *
imagename_from_filename (const gchar *filename)
{
  gchar *tmp;
  gchar *imagename;

  tmp = g_filename_from_uri (filename, NULL, NULL);
  if (!tmp)
    tmp = g_strdup (filename);

  imagename = g_filename_display_basename (tmp);

  return imagename;
}

static void
hd_home_background_dialog_class_init (HDHomeBackgroundDialogClass *klass)
{
  GParamSpec       *pspec;
  GObjectClass     *object_class;
  GtkDialogClass   *dialog_class;

  object_class = G_OBJECT_CLASS (klass);
  dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->set_property = hd_home_background_dialog_set_property;
  object_class->get_property = hd_home_background_dialog_get_property;
  object_class->finalize = hd_home_background_dialog_finalize;

  dialog_class->response = hd_home_background_dialog_response;

  klass->background_dir_changed = hd_home_background_dialog_background_dir_changed;

  g_signal_new ("background-dir-changed",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HDHomeBackgroundDialogClass,
                                 background_dir_changed),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  pspec = g_param_spec_string ("background-dir",
                               "Background directory",
                               "Directory where to look for default backgrounds",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   PROP_BACKGROUND_DIR,
                                   pspec);
  
  pspec = g_param_spec_pointer ("osso-context",
                                "OSSO Context",
                                "OSSO Context",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   PROP_OSSO_CONTEXT,
                                   pspec);
  
  pspec = g_param_spec_object ("background",
                               "Background",
                               "Current background",
                               HD_TYPE_HOME_BACKGROUND,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   PROP_BACKGROUND,
                                   pspec);


  g_type_class_add_private (klass, sizeof (HDHomeBackgroundDialogPrivate));
}

static void
hd_home_background_dialog_init (HDHomeBackgroundDialog *dialog)
{
  HDHomeBackgroundDialogPrivate    *priv;
  GtkWidget                        *hbox_color;
  GtkWidget                        *hbox_image;
  GtkWidget                        *hbox_mode;
  GtkWidget                        *color_caption;
  GtkWidget                        *image_caption;
  GtkWidget                        *mode_caption;
  GtkCellRenderer                  *renderer;
  GtkSizeGroup                     *group;
  const gchar                      *image_modes[] = { 
    HH_SET_BG_MODE_CENTERED,
    HH_SET_BG_MODE_SCALED,
    HH_SET_BG_MODE_STRETCHED,
    HH_SET_BG_MODE_TILED
  };

  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (dialog);
  
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          HH_SET_BG_OK,
                          GTK_RESPONSE_OK,
                          HH_SET_BG_PREVIEW,
                          HILDON_HOME_SET_BG_RESPONSE_PREVIEW,
                          HH_SET_BG_IMAGE,
                          HILDON_HOME_SET_BG_RESPONSE_IMAGE,
                          HH_SET_BG_CANCEL,
                          GTK_RESPONSE_CANCEL,
                          NULL);

  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
  gtk_window_set_title (GTK_WINDOW (dialog), HH_SET_BG_TITLE);
  
  /* Hildon Caption HBoxes */
  hbox_color = gtk_hbox_new (FALSE, 10);
  hbox_image = gtk_hbox_new (FALSE, 10);
  hbox_mode  = gtk_hbox_new (FALSE, 10);
  
  priv->color_button = hildon_color_button_new ();
  
  g_signal_connect_swapped (priv->color_button, "notify::color",
                            G_CALLBACK (hd_home_background_dialog_color_changed),
                            dialog);
  
  priv->img_combo = gtk_combo_box_new ();

  g_signal_connect_swapped (priv->img_combo, "changed",
                            G_CALLBACK (hd_home_background_dialog_filename_changed),
                            dialog);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->img_combo),
                              renderer, 
                              TRUE);

  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->img_combo),
                                  renderer, "text", 
                                  BG_IMAGE_NAME, NULL);
  
  priv->mode_combo = gtk_combo_box_new_text ();
  
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->mode_combo), 
                             image_modes[BACKGROUND_CENTERED]);
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->mode_combo), 
                             image_modes[BACKGROUND_SCALED]);
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->mode_combo), 
                             image_modes[BACKGROUND_STRETCHED]);
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->mode_combo), 
                             image_modes[BACKGROUND_TILED]);
  
  g_signal_connect_swapped (priv->mode_combo, "changed",
                            G_CALLBACK (hd_home_background_dialog_mode_changed),
                            dialog);

  /* Hildon captions in Set Background Image dialog */
  group = GTK_SIZE_GROUP (gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL));
  color_caption =
      hildon_caption_new (group,
                          HH_SET_BG_COLOR_TITLE,  
                          priv->color_button,
                          NULL,
                          HILDON_CAPTION_OPTIONAL);
  image_caption = 
      hildon_caption_new (group, 
                          HH_SET_BG_IMAGE_TITLE,
                          priv->img_combo, 
                          NULL, 
                          HILDON_CAPTION_OPTIONAL);
  mode_caption =
      hildon_caption_new (group,
                          HH_SET_BG_MODE_TITLE,
                          priv->mode_combo,
                          NULL,
                          HILDON_CAPTION_OPTIONAL);
     
  gtk_box_pack_start (GTK_BOX (hbox_color), color_caption,
                      FALSE, FALSE, 0); 
  gtk_box_pack_start (GTK_BOX (hbox_image), image_caption,
                      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_mode), mode_caption,
                      TRUE, TRUE, 0);

  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 10);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox_color);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox_image);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox_mode);

  gtk_widget_show_all (GTK_DIALOG (dialog)->vbox);

  hildon_caption_set_child_expand (HILDON_CAPTION (image_caption), TRUE);
  hildon_caption_set_child_expand (HILDON_CAPTION (mode_caption), TRUE);
  
  /* Let the WM take care of the positioning */
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_NONE);

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
}

static void
hd_home_background_dialog_set_property (GObject      *gobject,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  HDHomeBackgroundDialogPrivate    *priv;

  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (gobject);

  switch (prop_id)
  {
    case PROP_BACKGROUND_DIR:
        hd_home_background_dialog_set_background_dir (
                          HD_HOME_BACKGROUND_DIALOG (gobject),
                          g_value_get_string (value));
        break;

    case PROP_OSSO_CONTEXT:
        priv->osso_context = g_value_get_pointer (value);

#ifdef HAVE_LIBHILDONHELP
        /* Add help button */
        if (priv->osso_context)
          hildon_help_dialog_help_enable (GTK_DIALOG (gobject),
                                          HH_HELP_SET_BACKGROUND,
                                          priv->osso_context);
#endif
        break;
        
    case PROP_BACKGROUND:
        hd_home_background_dialog_set_background (
                           HD_HOME_BACKGROUND_DIALOG (gobject),
                           g_value_get_object (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
        break;
  }
}

static void
hd_home_background_dialog_get_property (GObject      *gobject,
                                        guint         prop_id,
                                        GValue       *value,
                                        GParamSpec   *pspec)
{
  HDHomeBackgroundDialogPrivate    *priv;

  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (gobject);

  switch (prop_id)
  {
    case PROP_BACKGROUND_DIR:
        g_value_set_string (value, priv->background_dir);
        break;

#ifdef HAVE_LIBHILDONHELP
    case PROP_OSSO_CONTEXT:
        g_value_set_pointer (value, priv->osso_context);
        break;
#endif

    case PROP_BACKGROUND:
        g_value_set_object (value, priv->background);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
        break;
  }
}

static void
hd_home_background_dialog_finalize (GObject *object)
{
  HDHomeBackgroundDialogPrivate    *priv;

  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (object);

  if (priv->background)
    {
      g_object_unref (priv->background);
      priv->background = NULL;
    }

  if (priv->custom_image)
    {
      gtk_tree_path_free (priv->custom_image);
      priv->custom_image = NULL;
    }

  g_free (priv->background_dir);
  priv->background_dir = NULL;

}

static void
hd_home_background_dialog_file_select (HDHomeBackgroundDialog *dialog)
{
  HDHomeBackgroundDialogPrivate    *priv;
  GtkWidget                        *fdialog;
  GtkFileFilter                    *mime_type_filter;
  gint                              response;
  gchar                            *image_dir;

  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (dialog);

#if defined HAVE_HILDON_FM  || defined HAVE_HILDON_FM2
  fdialog = hildon_file_chooser_dialog_new_with_properties (
					  GTK_WINDOW (dialog),
					  HILDON_HOME_FILE_CHOOSER_ACTION_PROP,
					  GTK_FILE_CHOOSER_ACTION_OPEN,
					  HILDON_HOME_FILE_CHOOSER_TITLE_PROP,
					  HILDON_HOME_FILE_CHOOSER_TITLE,
					  HILDON_HOME_FILE_CHOOSER_SELECT_PROP,
					  HILDON_HOME_FILE_CHOOSER_SELECT,
					  HILDON_HOME_FILE_CHOOSER_EMPTY_PROP,
					  HILDON_HOME_FILE_CHOOSER_EMPTY,
					  NULL);
#else
  fdialog = gtk_file_chooser_dialog_new (HILDON_HOME_FILE_CHOOSER_TITLE,
					 GTK_WINDOW (dialog),
					 GTK_FILE_CHOOSER_ACTION_OPEN,
                                         HILDON_HOME_FILE_CHOOSER_SELECT, GTK_RESPONSE_OK, 
                                         HILDON_HOME_FILE_CHOOSER_EMPTY, GTK_RESPONSE_CANCEL, 
					 NULL);
#endif
  
#ifdef HAVE_LIBHILDONHELP
  if (priv->osso_context)
    hildon_help_dialog_help_enable (GTK_DIALOG(fdialog), 
                                    HH_HELP_SELECT_IMAGE,
                                    priv->osso_context);
#endif
        
  mime_type_filter = gtk_file_filter_new();
  gtk_file_filter_add_mime_type (mime_type_filter, "image/jpeg");
  gtk_file_filter_add_mime_type (mime_type_filter, "image/gif");
  gtk_file_filter_add_mime_type (mime_type_filter, "image/png");
  gtk_file_filter_add_mime_type (mime_type_filter, "image/bmp");
  gtk_file_filter_add_mime_type (mime_type_filter, "image/tiff");
  gtk_file_filter_add_mime_type (mime_type_filter, "sketch/png");
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (fdialog),
                               mime_type_filter);

  image_dir = g_build_path(G_DIR_SEPARATOR_S,
                           g_get_home_dir (),
                           HILDON_HOME_HC_USER_IMAGE_DIR,
                           NULL);

  if (!gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (fdialog),
                                            image_dir))
    g_warning ("Couldn't set default image dir for dialog %s", 
               image_dir);

  g_free (image_dir);
  
  response = gtk_dialog_run (GTK_DIALOG (fdialog));
    
  if (response == GTK_RESPONSE_OK) 
    {
      const gchar  *filename;

      filename = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (fdialog));

      if (filename) 
        {
          GtkTreeModel *model;
          GtkTreeIter   iter;            
          gchar        *name;

          name = imagename_from_filename (filename);

          model = GTK_TREE_MODEL (priv->combobox_contents);

          if (priv->custom_image)
            {
              gtk_tree_model_get_iter (model,
                                       &iter,
                                       priv->custom_image);
              gtk_list_store_remove (priv->combobox_contents,
                                     &iter);
              gtk_tree_path_free (priv->custom_image);
            }

          gtk_list_store_append (priv->combobox_contents,
                                 &iter);
          
          priv->custom_image = gtk_tree_model_get_path (model,
                                                        &iter);

          gtk_list_store_set (priv->combobox_contents,
                              &iter,
                              BG_IMAGE_NAME, name?name:filename,
                              BG_IMAGE_FILENAME, filename,
                              BG_IMAGE_PRIORITY, G_MAXINT,
                              -1);

          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->img_combo),
                                         &iter);
          g_free (name);
        }

    }  

  gtk_widget_destroy (fdialog);

}

static void
hd_home_background_dialog_response (GtkDialog *dialog, 
                                    gint       arg)
{
  HDHomeBackgroundDialog        *background_dialog;

  background_dialog = HD_HOME_BACKGROUND_DIALOG (dialog);

  if (arg == HILDON_HOME_SET_BG_RESPONSE_IMAGE)
    hd_home_background_dialog_file_select (background_dialog);
}

static void
hd_home_background_dialog_filename_changed (HDHomeBackgroundDialog *dialog)
{
  HDHomeBackgroundDialogPrivate    *priv;
  GtkTreeIter                       iter;
  gchar                            *filename;
  
  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (dialog);

  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->img_combo),
                                 &iter);

  gtk_tree_model_get (GTK_TREE_MODEL (priv->combobox_contents),
                      &iter,
                      BG_IMAGE_FILENAME, &filename,
                      -1);

  if (filename)
    g_object_set (G_OBJECT (priv->background),
                  "filename", filename,
                  NULL);
  else
    g_object_set (G_OBJECT (priv->background),
                  "filename", HD_HOME_BACKGROUND_NO_IMAGE,
                  NULL);

  g_free (filename);
}

static void
hd_home_background_dialog_color_changed (HDHomeBackgroundDialog *dialog)
{
  HDHomeBackgroundDialogPrivate    *priv;
#ifdef HAVE_LIBHILDON
  GdkColor                          color;
#else
  GdkColor                         *color;
#endif

  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (dialog);

  if (!priv->background)
    return;

#ifdef HAVE_LIBHILDON
   hildon_color_button_get_color (HILDON_COLOR_BUTTON (priv->color_button),
                                  &color);
  
   g_object_set (G_OBJECT (priv->background),
                "color", &color,
                NULL);
#else
  color =
   hildon_color_button_get_color (HILDON_COLOR_BUTTON (priv->color_button));

  g_object_set (G_OBJECT (priv->background),
                "color", color,
                NULL);
#endif
}

static void
hd_home_background_dialog_mode_changed (HDHomeBackgroundDialog *dialog)
{
  HDHomeBackgroundDialogPrivate    *priv;
  BackgroundMode                    mode;

  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (dialog);

  if (!priv->background)
    return;

  mode = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->mode_combo));

  g_object_set (G_OBJECT (priv->background),
                "mode", mode,
                NULL);
}

static void
hd_home_background_dialog_background_dir_changed 
                                    (HDHomeBackgroundDialog *dialog)
{
  HDHomeBackgroundDialogPrivate *priv;
  GDir                          *dir = NULL;
  GError                        *error = NULL;
  GtkTreeIter                    iterator;
  const gchar                   *image_desc_file = NULL;
  GKeyFile                      *kfile;

  g_return_if_fail (HD_IS_HOME_BACKGROUND_DIALOG (dialog));
  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (dialog);

  if (priv->combobox_contents)
    gtk_list_store_clear (priv->combobox_contents);
  else
    {
      priv->combobox_contents =
          gtk_list_store_new (3,
                              G_TYPE_STRING, /*localised descriptive name */
                              G_TYPE_STRING, /* image file path & name */
                              G_TYPE_INT     /* image priority */);
      gtk_combo_box_set_model (GTK_COMBO_BOX (priv->img_combo),
                               GTK_TREE_MODEL (priv->combobox_contents));
    }

  /* No background file option */
  gtk_list_store_append (priv->combobox_contents, &iterator);
  gtk_list_store_set (priv->combobox_contents,
                      &iterator,
                      BG_IMAGE_NAME, HH_SET_BG_IMAGE_NONE,
                      BG_IMAGE_FILENAME, NULL,
                      BG_IMAGE_PRIORITY, 0, -1);

  /* It's the default */
  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->img_combo),
                                 &iterator);

  if (!priv->background_dir)
    return;

  dir = g_dir_open (priv->background_dir,
                    0,
                    &error);

  if (error)
  {
    g_warning ("Could not open background directory %s: %s",
               priv->background_dir,
               error->message);
    g_error_free (error);
    return;
  }

  kfile = g_key_file_new ();

  while ((image_desc_file = g_dir_read_name (dir)))
    {
      gchar *image_name = NULL;
      gchar *image_path = NULL;
      gchar *filename   = NULL;
      gint   image_order = 0;
      
      if (!g_str_has_suffix (image_desc_file, BG_IMG_INFO_FILE_TYPE)) 
        continue;

      error = NULL;

      filename = g_build_filename (priv->background_dir,
                                   image_desc_file,
                                   NULL);

      if (!g_key_file_load_from_file (kfile,
                                      filename,
                                      G_KEY_FILE_NONE,
                                      &error))
        goto end_of_loop;
      

      image_name = g_key_file_get_string (kfile,
                                          BG_DESKTOP_GROUP,
                                          BG_DESKTOP_IMAGE_NAME,
                                          &error);

      if (!image_name || error) goto end_of_loop;

      image_path = g_key_file_get_string (kfile,
                                          BG_DESKTOP_GROUP,
                                          BG_DESKTOP_IMAGE_FILENAME,
                                          &error);

      if (!image_path || error) goto end_of_loop;

      image_order = g_key_file_get_integer (kfile,
                                            BG_DESKTOP_GROUP,
                                            BG_DESKTOP_IMAGE_PRIORITY,
                                            &error);

      if (error) goto end_of_loop;

      gtk_list_store_append (priv->combobox_contents, &iterator);

      gtk_list_store_set (priv->combobox_contents,
                          &iterator,
                          BG_IMAGE_NAME,
                          /* work around a strange behavior of gettext for
                           * empty strings */
                          ((image_name && *image_name) ? _(image_name) : image_path),
                          BG_IMAGE_FILENAME, image_path,
                          BG_IMAGE_PRIORITY, image_order,
                          -1);

end_of_loop:
      if (error)
        g_warning ("Error when reading background descriptions: %s",
                   error->message);
      g_clear_error (&error);

      g_free (filename);
      g_free (image_name);
      g_free (image_path);
    } 

  if (dir) 
    g_dir_close (dir);             
      
  g_key_file_free (kfile);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->combobox_contents),
                                        BG_IMAGE_PRIORITY, 
                                        GTK_SORT_ASCENDING);
}

static void
hd_home_background_dialog_sync_from_background (HDHomeBackgroundDialog *dialog)
{
  HDHomeBackgroundDialogPrivate    *priv;
  gchar                            *filename;
  GdkColor                         *color;
  BackgroundMode                    mode;

  g_return_if_fail (HD_IS_HOME_BACKGROUND_DIALOG (dialog));
  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (dialog);

  if (!priv->background)
    return;
  
  g_object_get (G_OBJECT (priv->background),
                "color", &color,
                "filename", &filename,
                "mode", &mode,
                NULL);

  if (filename && *filename && 
      !g_str_equal (filename, HD_HOME_BACKGROUND_NO_IMAGE))
    {
      gboolean      valid;
      GtkTreeIter   iter;
      GtkTreeModel *model;

      model = GTK_TREE_MODEL (priv->combobox_contents);

      valid = gtk_tree_model_get_iter_first (model, &iter);

      while (valid)
        {
          gchar *existing_filename = NULL;

          gtk_tree_model_get (model,
                              &iter,
                              BG_IMAGE_FILENAME, &existing_filename,
                              -1);

          if (existing_filename && g_str_equal (existing_filename, filename))
            break;

          valid = gtk_tree_model_iter_next (model, &iter);
        }

      if (!valid)
        {
          gchar *image_name = imagename_from_filename (filename);

          gtk_list_store_append (priv->combobox_contents, &iter);

          gtk_list_store_set (priv->combobox_contents,
                              &iter,
                              BG_IMAGE_FILENAME, filename,
                              BG_IMAGE_NAME, image_name,
                              BG_IMAGE_PRIORITY, G_MAXINT,
                              -1);

          if (priv->custom_image)
            gtk_tree_path_free (priv->custom_image);

          priv->custom_image = gtk_tree_model_get_path (model, &iter);

          g_free (image_name);
        }

      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->img_combo),
                                     &iter);

      g_free (filename);
    }

  if (color)
    hildon_color_button_set_color (HILDON_COLOR_BUTTON (priv->color_button),
                                   color);

  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->mode_combo), mode);
}

static void
hd_home_background_dialog_set_background_dir (HDHomeBackgroundDialog *dialog,
                                              const gchar *dir)
{
  HDHomeBackgroundDialogPrivate *priv;

  g_return_if_fail (HD_IS_HOME_BACKGROUND_DIALOG (dialog) && dir);
  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (dialog);

  if (!priv->background_dir 
      || !g_str_equal (dir, priv->background_dir))
  {
    g_free (priv->background_dir);

    priv->background_dir = g_strdup (dir);
    g_object_notify (G_OBJECT (dialog), "background-dir");
    g_signal_emit_by_name (dialog, "background-dir-changed");
  }
}

static void
hd_home_background_dialog_set_background (HDHomeBackgroundDialog *dialog,
                                          HDHomeBackground *background)
{
  HDHomeBackgroundDialogPrivate *priv;
  
  g_return_if_fail (HD_IS_HOME_BACKGROUND_DIALOG (dialog) &&
                    HD_IS_HOME_BACKGROUND (background));

  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (dialog);

  if (priv->background)
    g_object_unref (priv->background);

  priv->background = hd_home_background_copy (background);

  g_object_notify (G_OBJECT (dialog), "background");
  hd_home_background_dialog_sync_from_background (dialog);

}

HDHomeBackground *
hd_home_background_dialog_get_background (HDHomeBackgroundDialog *dialog)
{
  HDHomeBackgroundDialogPrivate *priv;

  g_return_val_if_fail (HD_IS_HOME_BACKGROUND_DIALOG (dialog), NULL);
  priv = HD_HOME_BACKGROUND_DIALOG_GET_PRIVATE (dialog);

  return g_object_ref (priv->background);
}
