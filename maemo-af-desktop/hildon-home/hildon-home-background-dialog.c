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

#include <libosso.h>
#include <osso-helplib.h>
#include <osso-log.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-color-button.h>
#include <hildon-widgets/hildon-caption.h>

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>

#include <libintl.h>

#include "hildon-home-window.h"
#include "hildon-home-background-dialog.h"
#include "hildon-home-main.h"
#include "background-manager.h"

#define BG_DESKTOP_GROUP           "Desktop Entry"
#define BG_DESKTOP_IMAGE_NAME      "Name"
#define BG_DESKTOP_IMAGE_FILENAME  "File"
#define BG_DESKTOP_IMAGE_PRIORITY  "X-Order"
#define BG_IMG_INFO_FILE_TYPE      "desktop"
#define HOME_BG_IMG_DEFAULT_PRIORITY  15327 /* this is a random number */
#define BG_LOADING_PIXBUF_NULL    -526
#define BG_LOADING_OTHER_ERROR    -607
#define BG_LOADING_RENAME_FAILED  -776
#define BG_LOADING_SUCCESS        0

#define HILDON_HOME_FILE_CHOOSER_ACTION_PROP  "action"
#define HILDON_HOME_FILE_CHOOSER_TITLE_PROP   "title"
#define HILDON_HOME_FILE_CHOOSER_TITLE        _("home_ti_select_image")
#define HILDON_HOME_FILE_CHOOSER_SELECT_PROP  "open-button-text"
#define HILDON_HOME_FILE_CHOOSER_SELECT       _("home_bd_select_image")
#define HILDON_HOME_FILE_CHOOSER_EMPTY_PROP   "empty-text"
#define HILDON_HOME_FILE_CHOOSER_EMPTY        _("home_li_no_images")

#define HILDON_HOME_SELECT_IMAGE_HELP_TOPIC   "uiframework_home_select_image"

#define HILDON_HOME_SET_BG_COLOR_TITLE     _("home_fi_set_backgr_color")
#define HILDON_HOME_SET_BG_IMAGE_TITLE     _("home_fi_set_backgr_image")
#define HILDON_HOME_SET_BG_IMAGE_NONE      _("home_va_set_backgr_none")
#define HILDON_HOME_SET_BG_MODE_TITLE      _("home_fi_set_backgr_mode") 
#define HILDON_HOME_SET_BG_MODE_CENTERED   _("home_va_set_backgr_centered")
#define HILDON_HOME_SET_BG_MODE_SCALED     _("home_va_set_backgr_scaled")
#define HILDON_HOME_SET_BG_MODE_STRETCHED  _("home_va_set_backgr_stretched")
#define HILDON_HOME_SET_BG_MODE_TILED      _("home_va_set_backgr_tiled") 

typedef struct 
{
  GtkWidget *dialog;
  
  GtkWidget *parent;
  
  GtkWidget *img_combo;
  GtkWidget *mode_combo;
  GtkWidget *color_button;
  
  guint applied         : 1;
  guint built_in        : 1;
  guint has_preview     : 1;
  guint discard_preview : 1;
  guint apply_preview   : 1;
} ResponseData;

static gchar *
home_bgd_filename_to_uri (const gchar *filename)
{
  GError *err = NULL;
  gchar *retval;
  
  retval = g_filename_to_uri (filename, NULL, &err);
  if (err)
    {
      g_warning ("Unable to transform `%s' into a URI: %s",
                 filename,
                 err->message);

      g_error_free (err);

      return g_strdup (filename);
    }
  
  return retval;
}

static gchar *
home_bgd_filename_from_uri (const char *uri_str)
{
  GnomeVFSURI *uri;
  gchar *retval;
    
  uri = gnome_vfs_uri_new (uri_str);
  retval = gnome_vfs_uri_extract_short_name (uri);
  gnome_vfs_uri_unref (uri);

  return retval;
}

/**
 * @home_bgd_get_priority
 *
 * @param tree GtkTreeModel containing priority in column
 *  BG_IMAGE_PRIORITY of type G_TYPE_INT
 * @param *iter a pointer to iterator of the correct node
 *
 * @return gint the priority fetched
 * 
 * Digs up a gint value from the treemodel at iter
 **/
static gint
home_bgd_get_priority (GtkTreeModel *tree, GtkTreeIter *iter)
{
  gint priority = -1;
  GValue value = { 0, };
  GValue value_priority = { 0, };        

  gtk_tree_model_get_value(tree, iter, BG_IMAGE_PRIORITY, &value);    
  g_value_init(&value_priority, G_TYPE_INT);
    
  if (g_value_transform (&value, &value_priority))
    {
      priority = g_value_get_int(&value_priority);
    }

  return priority;
}

/**
 * @home_bgd_select_file_dialog
 *
 * @param parent pointer to parent dialog
 * 
 * @param box pointer to the bacground combo box
 *
 * @param built_in indicates whether current BG image is built in or
 * or custom
 *
 * @return TRUE
 * 
 * 'Select image file' dialog 
 * 
 **/
static gboolean
home_bgd_select_file_dialog (GtkWidget   * parent,
			     GtkComboBox * box,
			     gboolean      built_in)
{
  GtkTreeModel *tree = gtk_combo_box_get_model (box);
  GtkWidget *dialog;
  GtkFileFilter *mime_type_filter;
  gint response;
  gchar *name_str, *temp_mime, *dot;
  gchar *chooser_name;
  gchar *image_dir;
  
  dialog =
    hildon_file_chooser_dialog_new_with_properties(
					  GTK_WINDOW(parent),
					  HILDON_HOME_FILE_CHOOSER_ACTION_PROP,
					  GTK_FILE_CHOOSER_ACTION_OPEN,
					  HILDON_HOME_FILE_CHOOSER_TITLE_PROP,
					  HILDON_HOME_FILE_CHOOSER_TITLE,
					  HILDON_HOME_FILE_CHOOSER_SELECT_PROP,
					  HILDON_HOME_FILE_CHOOSER_SELECT,
					  HILDON_HOME_FILE_CHOOSER_EMPTY_PROP,
					  HILDON_HOME_FILE_CHOOSER_EMPTY,
					  NULL);

  /* Add help button */
  ossohelp_dialog_help_enable(GTK_DIALOG(dialog), 
			      HILDON_HOME_SELECT_IMAGE_HELP_TOPIC,
			      home_get_osso_context ());
        
  mime_type_filter = gtk_file_filter_new();
  gtk_file_filter_add_mime_type (mime_type_filter, "image/jpeg");
  gtk_file_filter_add_mime_type (mime_type_filter, "image/gif");
  gtk_file_filter_add_mime_type (mime_type_filter, "image/png");
  gtk_file_filter_add_mime_type (mime_type_filter, "image/bmp");
  gtk_file_filter_add_mime_type (mime_type_filter, "image/tiff");
  gtk_file_filter_add_mime_type (mime_type_filter, "sketch/png");

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog),
			       mime_type_filter);

  image_dir = g_build_path("/",
			   g_getenv( HILDON_HOME_ENV_HOME ),
			   HILDON_HOME_HC_USER_IMAGE_DIR,
			   NULL);

  if(!gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (dialog),
					  image_dir))
    {
      ULOG_ERR( "Couldn't set default image dir for dialog %s", 
		image_dir);
    }

  g_free (image_dir);
  
  response = gtk_dialog_run(GTK_DIALOG(dialog));
    
  if (response == GTK_RESPONSE_OK) 
    {
      GnomeVFSURI *uri_tmp;

      chooser_name = 
	gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
        
      uri_tmp  = gnome_vfs_uri_new (chooser_name);
      name_str = gnome_vfs_uri_extract_short_name(uri_tmp);
      temp_mime = 
	(gchar *)gnome_vfs_get_mime_type(gnome_vfs_uri_get_path(uri_tmp));

      gnome_vfs_uri_unref (uri_tmp);

      /* remove extension */
      if ((dot = g_strrstr (name_str, ".")) && dot != name_str)
	  *dot = '\0';
        
      if (name_str && *name_str) 
        {
	  GtkTreeIter iter;            
	  GtkTreePath *path;
	  gint priority = 0;
	  gtk_list_store_append(GTK_LIST_STORE(tree), &iter);

#if 1
	  /* this was in the original dialog -- is this in the spec ? */
	  
	  /* remove previously selected user image from the image combo */
	  if( built_in )
            {
	      gchar *remove_name = NULL;
	      GtkTreeIter remove_iter;
	      GtkTreePath *remove_path;

	      remove_path = gtk_tree_model_get_path(tree, &iter);
	      gtk_tree_path_prev(remove_path);
	      gtk_tree_model_get_iter(tree, &remove_iter, remove_path);
	      gtk_tree_model_get(tree, &remove_iter,
				 BG_IMAGE_NAME, &remove_name, -1);

	      BackgroundManager * bm = background_manager_get_default ();
	      
	      if( remove_name && 
		  g_str_equal(background_manager_get_image_uri (bm),
			      remove_name) )
                {
		  gtk_list_store_remove(GTK_LIST_STORE(tree), &remove_iter);
                }

	      g_free(remove_name);
	      gtk_tree_path_free(remove_path);
            }
#endif
	  
	  gtk_list_store_set(GTK_LIST_STORE(tree),
			     &iter,
			     BG_IMAGE_NAME, name_str,
			     BG_IMAGE_FILENAME, chooser_name,
			     BG_IMAGE_PRIORITY, G_MAXINT, -1);

	  gtk_combo_box_set_active_iter(box, &iter);

	  path = gtk_tree_model_get_path(tree, &iter);
	  gtk_tree_path_next(path);

	  if (gtk_tree_model_get_iter(tree, &iter, path)) 
            {
	      priority = home_bgd_get_priority (tree, &iter);
	      if (priority == G_MAXINT) 
                {
		  gtk_list_store_remove(GTK_LIST_STORE(tree), &iter);
                    
                }
            }
	  gtk_tree_path_free(path);
        }
	
      /* Clean up */
      g_free(name_str);
      gtk_widget_destroy(dialog);    
        
      return TRUE;
    }  
    
  gtk_widget_destroy(dialog);
    
  return FALSE;
}

/**
 * @home_bgd_get_filename
 *       
 * @param box GtkComboBox listing background images
 * 
 * @param index index of the box entry that is selected   
 *       
 * @return *gchar the filename   
 *       
 *       
 * Gets file path from the background image combo box.   
 */               
static gchar *
home_bgd_get_filename (GtkComboBox *box,
                       gint         index)
{        
    gchar *name = NULL;          
    GtkTreeIter iter;    
    GtkTreePath *walkthrough;    
    GtkTreeModel *tree = gtk_combo_box_get_model (box);   
    
    walkthrough = gtk_tree_path_new_from_indices (index, -1);
    
    gtk_tree_model_get_iter (tree, &iter, walkthrough);   
   
    gtk_tree_model_get (tree, &iter,
                        BG_IMAGE_FILENAME, &name,
                        -1);

    if (walkthrough) 
      gtk_tree_path_free (walkthrough);    

    return name;
}        

static void
home_bgd_apply (ResponseData *resp_data)
{
  GtkWidget *img_combo = resp_data->img_combo;
  GtkWidget *mode_combo = resp_data->mode_combo;
  GtkWidget *color_button = resp_data->color_button;
  BackgroundManager *manager = background_manager_get_default ();
  
  if (GTK_IS_COMBO_BOX (img_combo))
    {
      gint active = gtk_combo_box_get_active (GTK_COMBO_BOX (img_combo));
      
      if (active != -1)
        {
          gchar *filename = home_bgd_get_filename (GTK_COMBO_BOX (img_combo),
                                                   active);
          gchar *uri;
          
          if (filename)
            uri = home_bgd_filename_to_uri (filename);
          else
            uri = g_strdup (HILDON_HOME_BACKGROUND_NO_IMAGE);

          g_debug ("selected index %d, [%s]", active, uri);

          background_manager_set_image_uri (manager, uri);

          g_free (uri);
          g_free (filename);
        }
    }

  if (GTK_IS_COMBO_BOX (mode_combo))
    {
      gint active = gtk_combo_box_get_active (GTK_COMBO_BOX (mode_combo));

      if (active < 0)
        {
          active = BACKGROUND_CENTERED;
        }

      background_manager_set_mode (manager, (BackgroundMode) active);
    }

  if (HILDON_IS_COLOR_BUTTON (color_button))
    {
      GdkColor *color;

      color =
        hildon_color_button_get_color (HILDON_COLOR_BUTTON (color_button));

      background_manager_set_color (manager, color);
    }

  if (!resp_data->has_preview)
    background_manager_update_preview (manager);
  else
    background_manager_refresh_from_cache (manager);
}

static void
home_bgd_preview (ResponseData *resp_data)
{
  GtkWidget *img_combo = resp_data->img_combo;
  GtkWidget *mode_combo = resp_data->mode_combo;
  GtkWidget *color_button = resp_data->color_button;
  BackgroundManager *manager = background_manager_get_default ();
  
  if (GTK_IS_COMBO_BOX (img_combo))
    {
      gint active = gtk_combo_box_get_active (GTK_COMBO_BOX (img_combo));
      
      if (active != -1)
        {
          gchar *filename = home_bgd_get_filename (GTK_COMBO_BOX (img_combo),
                                                   active);
          gchar *uri;
          
          if (filename)
            uri = home_bgd_filename_to_uri (filename);
          else
            uri = g_strdup (HILDON_HOME_BACKGROUND_NO_IMAGE);

          g_debug ("selected index %d, [%s]", active, uri);

          background_manager_set_image_uri (manager, uri);

          g_free (uri);
          g_free (filename);
        }
    }

  if (GTK_IS_COMBO_BOX (mode_combo))
    {
      gint active = gtk_combo_box_get_active (GTK_COMBO_BOX (mode_combo));

      if (active < 0)
        {
          active = BACKGROUND_CENTERED;
        }

      background_manager_set_mode (manager, (BackgroundMode) active);
    }

  if (HILDON_IS_COLOR_BUTTON (color_button))
    {
      GdkColor *color;

      color =
        hildon_color_button_get_color (HILDON_COLOR_BUTTON (color_button));

      background_manager_set_color (manager, color);
    }
  
  resp_data->has_preview = FALSE;
  background_manager_update_preview (manager);
}

static void
home_bgd_cancel (GtkWidget *widget)
{
  g_debug ("cancel background selection");

  background_manager_discard_preview (background_manager_get_default (), TRUE);
  gtk_widget_destroy (widget);
}

static void
home_bgd_load_error_cb (BackgroundManager *manager,
                        const GError      *error,
                        gpointer           data)
{
  ResponseData *resp_data = data;
  if (!resp_data)
    return;
  
  g_debug (G_STRLOC ": load error: %s", error->message);

  if (resp_data->dialog && !GTK_WIDGET_VISIBLE (resp_data->dialog))
    {
      resp_data->discard_preview = TRUE;
      
      gtk_widget_destroy (resp_data->dialog);
    }
  else
    {
      background_manager_discard_preview (manager, TRUE);
      
      resp_data->has_preview = FALSE;
    }
}

static void
home_bgd_load_cancel_cb (BackgroundManager *manager,
                         gpointer           data)
{
  ResponseData *resp_data = data;
  if (!resp_data)
    return;
  
  g_debug (G_STRLOC ": load cancelled");

  if (resp_data->dialog && !GTK_WIDGET_VISIBLE (resp_data->dialog))
    {
      resp_data->discard_preview = TRUE;
      
      gtk_widget_destroy (resp_data->dialog);
    }
  else
    {
      background_manager_discard_preview (manager, TRUE);
      
      resp_data->has_preview = FALSE;
    }
}

static void
home_bgd_load_complete_cb (BackgroundManager *manager,
                           gpointer           data)
{
  ResponseData *resp_data = data;
  if (!resp_data)
    return;
  
  g_debug (G_STRLOC ": load complete");

  /* if the dialog is not visible, then the user has pressed 'okay'
   * so we are applying the background the user has chosen.
   */
  if (resp_data->dialog && !GTK_WIDGET_VISIBLE (resp_data->dialog))
    {
      resp_data->apply_preview = TRUE;
      
      gtk_widget_destroy (resp_data->dialog);
    }
  else
    resp_data->has_preview = TRUE;
}

static void
home_bgd_destroy_cb (GtkWidget *widget,
                     gpointer   data)
{
  BackgroundManager *manager;
  ResponseData *resp_data = data;
  if (!resp_data)
    return;

  g_debug (G_STRLOC ": background selection dialog destroyed");

  manager = background_manager_get_default ();

  g_signal_handlers_disconnect_by_func (manager,
                                        G_CALLBACK (home_bgd_load_complete_cb),
                                        resp_data);
  g_signal_handlers_disconnect_by_func (manager,
                                        G_CALLBACK (home_bgd_load_cancel_cb),
                                        resp_data);
  g_signal_handlers_disconnect_by_func (manager,
                                        G_CALLBACK (home_bgd_load_error_cb),
                                        resp_data);
  
  if (resp_data->discard_preview)
    background_manager_discard_preview (manager, TRUE);

  if (resp_data->apply_preview)
    background_manager_apply_preview (manager);
  
  background_manager_pop_preview_mode (manager);

  g_free (resp_data);
}

static void
home_bgd_show_cb (GtkWidget *widget,
                  gpointer   data)
{
  BackgroundManager *manager;

  g_debug (G_STRLOC ": background selection dialog shown");
  
  manager = background_manager_get_default ();
  background_manager_push_preview_mode (manager);
}

/**
 * @home_bgd_response_cb
 *
 * @param dialog parent widget
 * @param arg event that caused the call
 * @param data pointer to ResponseData struct
 * 
 * "response" signal callback
 **/

static void
home_bgd_response_cb (GtkWidget *dialog, 
		      gint       arg,
		      gpointer   data)
{
  ResponseData *d = data;
  GtkComboBox *img_combo;

#if 0
  g_debug ("response_data = {\n"
           "  img_combo    .= %s\n"
           "  mode_combo   .= %s\n"
           "  color_button .= %s\n"
           "  applied      .= %s\n"
           "  built_in     .= %s\n"
           "  has_preview  .= %s\n"
           "}",
           g_type_name (G_OBJECT_TYPE (d->img_combo)),
           g_type_name (G_OBJECT_TYPE (d->mode_combo)),
           g_type_name (G_OBJECT_TYPE (d->color_button)),
           d->applied ? "<true>" : "<false>",
           d->built_in ? "<true>" : "<false>",
           d->has_preview ? "<true>" : "<false>");
#endif

  switch (arg) 
    {
    case HILDON_HOME_SET_BG_RESPONSE_IMAGE:
      g_signal_stop_emission_by_name (dialog, "response");
      img_combo = GTK_COMBO_BOX (d->img_combo);
      home_bgd_select_file_dialog (dialog, img_combo, d->built_in);
      break;
    case HILDON_HOME_SET_BG_RESPONSE_PREVIEW:
      g_signal_stop_emission_by_name (dialog, "response");
      home_bgd_preview (d);
      break;
    case GTK_RESPONSE_OK:
      gtk_widget_hide (dialog);
      home_bgd_apply (d);
      break;
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_DELETE_EVENT:
      gtk_widget_hide (dialog);
      home_bgd_cancel (dialog);
      break;
    default:
      break;
    }
}

static void
home_bgd_settings_changed (GtkWidget    *widget,
                           ResponseData *data)
{
  data->has_preview = FALSE;
}

static void
home_bgd_color_changed (GObject      *gobject,
                        GParamSpec   *pspec,
                        ResponseData *data)
{
  data->has_preview = FALSE;
}

/**
 * @home_bgd_dialog_run
 *
 * @param parent The parent window
 * 
 * @return TRUE (keeps cb alive)
 * 
 * Set background image dialog.
 **/
gboolean
home_bgd_dialog_run (GtkWindow * parent)
{
  /* GtkDialog layout Widgets */ 
  GtkWidget       * dialog;
  GtkWidget       * hbox_image;
  GtkWidget       * hbox_color;
  GtkWidget       * hbox_mode;

  /* HildonCaption Widgets */
  GtkWidget       * combobox_image_select;
  GtkWidget       * combobox_mode_select;
  GtkWidget       * color_caption;
  GtkWidget       * image_caption;
  GtkWidget       * mode_caption; 
  GtkSizeGroup    * group = NULL;
  GtkWidget       * color_button;
  
  /* Image Combobox Widgets */
  GtkCellRenderer * renderer; 
  GtkListStore    * combobox_contents;
  GtkTreeIter       iterator;
  GDir            * bg_image_desc_base_dir;
  GError          * error = NULL;
  GtkTreeModel    * model;

  /* Misc */
  gchar           * dot;
  gint              image_order;
  const gchar     * image_desc_file;
  const gchar     * image_modes[] = { 
    HILDON_HOME_SET_BG_MODE_CENTERED,
    HILDON_HOME_SET_BG_MODE_SCALED,
    HILDON_HOME_SET_BG_MODE_STRETCHED,
    HILDON_HOME_SET_BG_MODE_TILED
  };
  
  ResponseData    * resp_data;
  gint              combobox_active = -1;
  gint              active_count = 0;
  GdkColor          current_color;
  
  BackgroundManager *bm = background_manager_get_default ();

  resp_data = g_new (ResponseData, 1);

  /* data for the response callback */
  resp_data->applied = FALSE;
  resp_data->built_in = FALSE;
  resp_data->has_preview = FALSE;
  resp_data->discard_preview = FALSE;
  resp_data->apply_preview = FALSE;
  
  bg_image_desc_base_dir = g_dir_open (HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR,
				       0,
				       &error);

  combobox_contents =
    gtk_list_store_new (3,
		        G_TYPE_STRING, /*localised descriptive name */
			G_TYPE_STRING, /* image file path & name */
			G_TYPE_INT     /* image priority */);

  /* No background file option */
  gtk_list_store_append (combobox_contents, &iterator);
  gtk_list_store_set (combobox_contents,
		      &iterator,
		      BG_IMAGE_NAME, HILDON_HOME_SET_BG_IMAGE_NONE,
		      BG_IMAGE_FILENAME, NULL,
		      BG_IMAGE_PRIORITY, 0, -1);
  
  while (bg_image_desc_base_dir && 
	 (image_desc_file = g_dir_read_name (bg_image_desc_base_dir)) )
    {
      if (g_str_has_suffix (image_desc_file, BG_IMG_INFO_FILE_TYPE)) 
        {
	  gchar *image_name = NULL;
	  gchar *image_path = NULL;
	  gchar *filename   = NULL;
	  const gchar *current_uri;
          GKeyFile *kfile;
                
	  error = NULL;

	  filename = g_build_filename (HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR,
				       image_desc_file,
				       NULL);

	  g_debug ("loading image descrition file [%s]", filename);
	  
	  kfile = g_key_file_new ();

	  if (!g_key_file_load_from_file (kfile,
					  filename,
					  G_KEY_FILE_NONE,
					  &error))
            {
	      g_debug ("Failed to read file: %s: %s\n",
                       image_desc_file,
                       error->message ? error->message : "Unknown");

	      if (error)
		g_error_free (error);

	      g_key_file_free (kfile);
	      g_free (filename);

	      continue;
            }
            
	  g_free (filename);

	  image_name = g_key_file_get_string (kfile,
					      BG_DESKTOP_GROUP,
					      BG_DESKTOP_IMAGE_NAME,
					      &error);

	  g_debug ("image name [%s]", image_name ? image_name : "None");
	  
	  if (!image_name || error)
            {
	      if (error)
                {
                  g_warning ("Unable to read image name: %s",
                             error->message);
                  g_error_free (error);
                }

	      g_key_file_free (kfile);

	      continue;
            }

	  image_path = g_key_file_get_string (kfile,
					      BG_DESKTOP_GROUP,
					      BG_DESKTOP_IMAGE_FILENAME,
					      &error);

	  g_debug ("image uri [%s]", image_path);
	  
	  if (!image_path || error)
            {
	      if (error)
                {
                  g_warning ("Unable to read image filename: %s",
                             error->message);
		  g_error_free (error);
                }
              
	      g_free (image_name);
	      g_key_file_free (kfile);

	      continue;
            }

	  image_order = g_key_file_get_integer (kfile,
						BG_DESKTOP_GROUP,
						BG_DESKTOP_IMAGE_PRIORITY,
						&error);
            
	  if (error)
            {
              g_warning ("Unable to read image priority: %s",
                         error->message);
              
	      g_error_free (error);
	      
              error = NULL;
	      
              image_order = HOME_BG_IMG_DEFAULT_PRIORITY;
            }
          
	  active_count++;

	  current_uri = background_manager_get_image_uri (bm);

	  if (current_uri)
	    {
	      gchar *current_path;
              GError *error = NULL;

              current_path = g_filename_from_uri (current_uri, NULL, &error);
              if (error)
                {
                  g_warning ("Unable to get the filename from uri `%s': %s",
                             current_uri,
                             error->message);
                  
                  g_error_free (error);

                  /* we let the background manager deal with it */
                  current_path = g_strdup (current_uri);
                }
	      
	      g_debug ("current filename [%s], processing [%s]",
		       current_path, image_path);
	  
	      if (image_path &&
		  g_str_equal(image_path, current_path)) 
		{
		  g_debug ("current BG uses built in image");
		  resp_data->built_in = TRUE;
		}

              g_free (current_path);
	    }
	  
	  gtk_list_store_append (combobox_contents, &iterator);

	  gtk_list_store_set (combobox_contents, &iterator,
			      BG_IMAGE_NAME,
			      /* work around a strange behavior of gettext for
			       * empty strings */
			      ((image_name && *image_name) ? _(image_name) : image_path),
			      BG_IMAGE_FILENAME, image_path,
			      BG_IMAGE_PRIORITY, image_order,
                              -1);
            
	  g_key_file_free (kfile);
	  g_free (image_name);
	  g_free (image_path);
        } 
      else /* suffix test */ 
        {
	  g_debug ("Skipping non-.desktop file: %s",
                   image_desc_file);
        }

    } /* while */

  if (bg_image_desc_base_dir) 
    {
      g_dir_close (bg_image_desc_base_dir);             
    }
	      
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (combobox_contents),
				        BG_IMAGE_PRIORITY, 
				        GTK_SORT_ASCENDING);
    
  if (resp_data->built_in == FALSE)
    {
      /* custom image is used for background; we need to add it to the
       * list; this includes the 'no-background option'
       */
      const gchar *uri = background_manager_get_image_uri (bm);

      if ( uri == NULL ||
	   g_str_equal (uri, HILDON_HOME_BACKGROUND_NO_IMAGE)) 
	{
	  g_debug ("no background");
	  combobox_active = 0;
	}
      else 
	{
	  gchar *image_filename = home_bgd_filename_from_uri (uri);
	  g_debug ("current background uses custom image");
					 
	  gtk_list_store_append (combobox_contents, &iterator);
            
	  if ((dot = g_strrstr (image_filename, ".")) &&
	      dot != image_filename) 
	    {
	      *dot = '\0';
	    }

	  g_debug ("setting [%s] [%s]", image_filename, uri);
	  
	  gtk_list_store_set (combobox_contents, &iterator,
			      BG_IMAGE_NAME, image_filename,
			      BG_IMAGE_FILENAME, uri,
			      BG_IMAGE_PRIORITY, G_MAXINT, -1);

	  g_free (image_filename);
	  active_count++;
            
	  combobox_active = active_count;
	}
    }
  else 
    {
      /*
       * the current background is already on the list but we need to figure
       * out where.
       */
      gboolean is_node;
      gint     ac = 0;
      
      model = GTK_TREE_MODEL (combobox_contents);

      g_debug ("using built-in image");
      
      is_node = gtk_tree_model_get_iter_first (model, &iterator);
      while (is_node) 
	{
	  gchar *filename;
	  const gchar *uri = background_manager_get_image_uri (bm);
	  const gchar *uri_filename = uri;

	  if (uri && g_str_has_prefix (uri, "file://"))
	    uri_filename += 7;
	  
	  gtk_tree_model_get (model, &iterator, 1, &filename, -1);

	  g_debug ("filename [%s], uri [%s]", filename, uri_filename);
	  
	  if (filename && uri && g_str_equal(filename, uri_filename)) 
	    {
	      g_debug ("current image at index %d", ac);
	      combobox_active = ac;
	      
	      g_free (filename);
	      break;
	    }
                
	  is_node = gtk_tree_model_iter_next (model, &iterator);
	  ac++;
	  g_free (filename);
	}
    }
   
  /* Create Set Background Image dialog */ 
  dialog = 
    gtk_dialog_new_with_buttons (HILDON_HOME_SET_BG_TITLE,
				 NULL,
				 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				 HILDON_HOME_SET_BG_OK,
				 GTK_RESPONSE_OK,
				 HILDON_HOME_SET_BG_PREVIEW,
				 HILDON_HOME_SET_BG_RESPONSE_PREVIEW,
				 HILDON_HOME_SET_BG_IMAGE,
				 HILDON_HOME_SET_BG_RESPONSE_IMAGE,
				 HILDON_HOME_SET_BG_CANCEL,
				 GTK_RESPONSE_CANCEL,
				 NULL);
  
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

  /* Add help button */
  ossohelp_dialog_help_enable (GTK_DIALOG (dialog),
			       HILDON_HOME_SET_BACKGROUND_HELP_TOPIC,
			       home_get_osso_context ());

  /* Hildon Caption HBoxes */
  hbox_color = gtk_hbox_new (FALSE, 10);
  hbox_image = gtk_hbox_new (FALSE, 10);
  hbox_mode  = gtk_hbox_new (FALSE, 10);
     
  /* Widgets for Hildon Captions in Set Background Image dialog */
  background_manager_get_color (bm, &current_color);
  
  color_button = hildon_color_button_new_with_color (&current_color);
  resp_data->color_button = color_button;

  /* HildonColorButton should really have a "changed" signal */
  g_signal_connect (color_button, "notify::color",
                    G_CALLBACK (home_bgd_color_changed),
                    resp_data);
  
  combobox_image_select =
    gtk_combo_box_new_with_model (GTK_TREE_MODEL (combobox_contents));
  resp_data->img_combo = combobox_image_select;
  
  if(!background_manager_get_image_uri (bm) ||
     g_str_equal(background_manager_get_image_uri (bm),
		 HILDON_HOME_BACKGROUND_NO_IMAGE))
    {
      combobox_active = 0;
    }
  
  gtk_combo_box_set_active (GTK_COMBO_BOX (combobox_image_select),
			    combobox_active);
  
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combobox_image_select),
			      renderer, 
			      TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox_image_select),
				  renderer, "text", 
				  BG_IMAGE_NAME, NULL);
  
  g_signal_connect (combobox_image_select, "changed",
                    G_CALLBACK (home_bgd_settings_changed),
                    resp_data);
  
  combobox_mode_select = gtk_combo_box_new_text ();
  resp_data->mode_combo = combobox_mode_select;
  
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox_mode_select), 
			     image_modes[BACKGROUND_CENTERED]);
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox_mode_select), 
			     image_modes[BACKGROUND_SCALED]);
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox_mode_select), 
			     image_modes[BACKGROUND_STRETCHED]);
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox_mode_select), 
			     image_modes[BACKGROUND_TILED]);
  
  /* Centered is a default scaling value */
  gtk_combo_box_set_active (GTK_COMBO_BOX (combobox_mode_select), 
			    background_manager_get_mode (bm));

  g_signal_connect (combobox_mode_select, "changed",
                    G_CALLBACK (home_bgd_settings_changed),
                    resp_data);

  /* Hildon captions in Set Background Image dialog */
  group = GTK_SIZE_GROUP (gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL));
  color_caption =
    hildon_caption_new (group,
			HILDON_HOME_SET_BG_COLOR_TITLE,  
			color_button,
			NULL,
			HILDON_CAPTION_OPTIONAL);
  image_caption = 
    hildon_caption_new (group, 
			HILDON_HOME_SET_BG_IMAGE_TITLE,
			combobox_image_select, 
			NULL, 
			HILDON_CAPTION_OPTIONAL);
  mode_caption =
    hildon_caption_new (group,
			HILDON_HOME_SET_BG_MODE_TITLE,
			combobox_mode_select,
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

  hildon_caption_set_child_expand (HILDON_CAPTION (image_caption), TRUE);
  hildon_caption_set_child_expand (HILDON_CAPTION (mode_caption), TRUE);

  resp_data->dialog = dialog;
  resp_data->parent = GTK_WIDGET (parent);
  
  g_signal_connect (bm, "load-complete",
                    G_CALLBACK (home_bgd_load_complete_cb),
                    resp_data);
  g_signal_connect (bm, "load-cancel",
                    G_CALLBACK (home_bgd_load_cancel_cb),
                    resp_data);
  g_signal_connect (bm, "load-error",
                    G_CALLBACK (home_bgd_load_error_cb),
                    resp_data);

  g_signal_connect (dialog, "show",
                    G_CALLBACK (home_bgd_show_cb),
                    resp_data);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (home_bgd_destroy_cb),
                    resp_data);
  g_signal_connect (dialog, "response", 
		    G_CALLBACK (home_bgd_response_cb), 
		    resp_data);
  
  /* Let matchbox take care of the positioning */
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_NONE);
  gtk_widget_show_all (dialog);

  return TRUE;
} 