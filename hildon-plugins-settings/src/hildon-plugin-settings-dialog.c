/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

#include "hildon-plugin-settings-dialog.h"
#include "hildon-plugin-config-parser.h"

#include <gtk/gtktreemodelfilter.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtk.h>

#include <hildon-desktop/hd-config.h>

#define HPSD_OK "Ok"
#define HPSD_CANCEL "Cancel"

#define HPSD_TAB_SB "Statusbar"
#define HPSD_TAB_TN "Task Navigator"

#define HPSD_TITLE "Plugin's Settings"

#ifndef HP_PATH_TN
#define HP_PATH_TN "/usr/share/applications/hildon-navigator"
#endif

#ifndef HP_PATH_SB
#define HP_PATH_SB "/usr/share/applications/hildon-status-bar"
#endif

/* TODO: FIXME: 
 * This should read /etc/hildon-desktop/desktop.conf and look for panels 
 * and create as many pages as necessary */

#define HP_CONFIG_SB "statusbar.conf"
#define HP_CONFIG_TN "tasknavigator.conf"
#define HP_CONFIG_DESKTOP "desktop.conf"

#define HP_CONFIG_DEFAULT_PATH "/etc/hildon-desktop"
#define HP_CONFIG_USER_PATH ".osso/hildon-desktop"

#define HD_DESKTOP_CONFIG_USER_PATH ".osso/hildon-desktop"
#define HD_DESKTOP_CONFIG_PATH "/etc/hildon-desktop"

#define HILDON_PLUGIN_SETTINGS_DIALOG_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_PLUGIN_TYPE_SETTINGS_DIALOG, HildonPluginSettingsDialogPrivate))

G_DEFINE_TYPE (HildonPluginSettingsDialog, hildon_plugin_settings_dialog, GTK_TYPE_DIALOG);

typedef struct 
{
  gchar *name;
  HildonPluginConfigParser *parser;
}
HPSDTab;

struct _HildonPluginSettingsDialogPrivate
{
  GList *tabs;
	
  GtkTreeModelFilter *sb_filter;
  GtkTreeModelFilter *tn_filter;
};

static GObject *hildon_plugin_settings_dialog_constructor (GType gtype,
                                              		   guint n_params,
                                              		   GObjectConstructParam *params);

static void hildon_plugin_settings_dialog_finalize (GObject *object);


static void hildon_plugin_settings_dialog_response (GtkDialog *dialog, gint response_id);

static void hildon_plugin_settings_dialog_fill_treeview (HildonPluginSettingsDialog *settings, GtkTreeView *tw);

static gboolean hildon_plugin_settings_parse_desktop_conf (HildonPluginSettingsDialog *settings);

static void hildon_plugin_settings_dialog_tab_free (HPSDTab *tab);

static void 
hildon_plugin_settings_dialog_init (HildonPluginSettingsDialog *settings)
{
  settings->priv = HILDON_PLUGIN_SETTINGS_DIALOG_GET_PRIVATE (settings);

  settings->sbtm = 
  settings->tntm = NULL;	  
  
  settings->priv->sb_filter =
  settings->priv->tn_filter = NULL;	  

  settings->priv->tabs = NULL;
}

static void 
hildon_plugin_settings_dialog_class_init (HildonPluginSettingsDialogClass *settings_class)
{
  GObjectClass *object_class   = G_OBJECT_CLASS (settings_class);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (settings_class);

  object_class->constructor = hildon_plugin_settings_dialog_constructor;
  object_class->finalize    = hildon_plugin_settings_dialog_finalize;

  dialog_class->response    = hildon_plugin_settings_dialog_response;
  
  g_type_class_add_private (object_class, sizeof (HildonPluginSettingsDialogPrivate));

}

static GObject *
hildon_plugin_settings_dialog_constructor (GType gtype,
                                           guint n_params,
                                           GObjectConstructParam *params)
{
  GObject *object;
  GtkDialog *dialog;
  GtkWidget *button_up, 
	    *button_down, 
	    *notebook,
	    *scrolled_window;
  GList *l;

  GError *error = NULL;
  HildonPluginSettingsDialog *settings;

  object = 
    G_OBJECT_CLASS (hildon_plugin_settings_dialog_parent_class)->constructor (gtype,
		    							      n_params,
									      params);
  dialog = GTK_DIALOG (object);
  settings = HILDON_PLUGIN_SETTINGS_DIALOG (object);

  gtk_window_set_title (GTK_WINDOW (dialog), HPSD_TITLE);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  gtk_dialog_set_has_separator (dialog, FALSE);

  gtk_widget_set_size_request (GTK_WIDGET (dialog), -1, 300);

  gtk_widget_push_composite_child ();

  gtk_dialog_add_button (dialog,
		  	 HPSD_OK,
			 GTK_RESPONSE_OK);

  button_down =
    gtk_dialog_add_button (dialog,
	  		   "Up",
	       		   GTK_RESPONSE_APPLY);

  button_up =
    gtk_dialog_add_button (dialog,
	 		   "Down",
	       		   GTK_RESPONSE_APPLY);

  gtk_dialog_add_button (dialog,
			 HPSD_CANCEL,
		       	 GTK_RESPONSE_CANCEL);

  notebook = gtk_notebook_new ();

  hildon_plugin_settings_parse_desktop_conf (settings);

  for (l = settings->priv->tabs; l != NULL; l = g_list_next (l))
  {
    GtkWidget *tw;
    gchar *path_to_save;
    gchar *basename;
	  
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    GTK_POLICY_NEVER,
				    GTK_POLICY_AUTOMATIC);

    hildon_plugin_config_parser_load (((HPSDTab *)l->data)->parser, &error);

    if (error)
    {
      tw = gtk_tree_view_new ();
      g_error_free (error); 
    }
    else
      tw = gtk_tree_view_new_with_model (((HPSDTab *)l->data)->parser->tm);

    hildon_plugin_settings_dialog_fill_treeview (settings, GTK_TREE_VIEW (tw));

    gtk_container_add (GTK_CONTAINER (scrolled_window), tw);

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                              scrolled_window,
                              gtk_label_new (((HPSDTab *)l->data)->name));

    g_object_get (G_OBJECT (((HPSDTab *)l->data)->parser), 
		  "filename", &path_to_save,
		  NULL);

    if (!g_file_test (path_to_save, G_FILE_TEST_EXISTS))
    {
      basename = g_path_get_basename (path_to_save);

      g_free (path_to_save);

      path_to_save = g_build_filename (HD_DESKTOP_CONFIG_PATH,
                                       basename,
                                       NULL);
      g_free (basename);
    }
    
    if (path_to_save)
    {
      GError *error_comparing = NULL;	    
      hildon_plugin_config_parser_compare_with 
        (((HPSDTab *)l->data)->parser, path_to_save, &error_comparing);

      if (error_comparing)
      {
        g_warning ("I couldn't compare files: %s %s", path_to_save, error_comparing->message);	      
	g_error_free (error_comparing);
      }
    }
  }
  
  gtk_container_add (GTK_CONTAINER (dialog->vbox), notebook);
  gtk_widget_show_all (notebook); 

  gtk_widget_pop_composite_child ();
  
  return object;
}

static void 
hildon_plugin_settings_dialog_finalize (GObject *object)
{
  HildonPluginSettingsDialog *settings = 
    HILDON_PLUGIN_SETTINGS_DIALOG (object);

  g_list_foreach (settings->priv->tabs,
		  (GFunc)hildon_plugin_settings_dialog_tab_free,
		  NULL);

  g_list_free (settings->priv->tabs);
}	

static void 
hildon_plugin_settings_dialog_response (GtkDialog *dialog, gint response_id)
{
  GList *l;	
  HildonPluginSettingsDialog *settings =
    HILDON_PLUGIN_SETTINGS_DIALOG (dialog);
	
  GError *error = NULL;
	
  if (response_id == GTK_RESPONSE_OK)
  {
   
    for (l = settings->priv->tabs; l != NULL; l = g_list_next (l))
    {	    
      hildon_plugin_config_parser_save (((HPSDTab *)l->data)->parser, &error);

      if (error)
      {
        g_warning ("Error saving... %s",error->message);
        g_error_free (error);
      }
    }
  }
}

static void 
hildon_plugin_settings_dialog_fill_treeview (HildonPluginSettingsDialog *settings, GtkTreeView *tw)
{
  GtkCellRenderer *renderer_pixbuf = gtk_cell_renderer_pixbuf_new (),
                  *renderer_toggle = gtk_cell_renderer_toggle_new (),
                  *renderer_text   = gtk_cell_renderer_text_new (),
                  *renderer_button = NULL;

  GtkTreeViewColumn *column_pb,
                    *column_toggle,
                    *column_text,
                    *column_button;

  column_pb =
    gtk_tree_view_column_new_with_attributes
      (NULL, renderer_pixbuf, "pixbuf", 4, NULL);

  column_toggle =
    gtk_tree_view_column_new_with_attributes
      (NULL, renderer_toggle, "active", HP_COL_CHECKBOX, NULL);

  column_text =
    gtk_tree_view_column_new_with_attributes
      (NULL, renderer_text, "text", 3, NULL);

  renderer_button = NULL; /*TODO: NOTE: To make compiler happy */
  column_button = NULL;

  gtk_tree_view_append_column (GTK_TREE_VIEW (tw), column_pb);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tw), column_toggle);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tw), column_text);
  /*gtk_tree_view_append_column (GTK_TREE_VIEW (tw), column_button);*/
}	

static gboolean 
hildon_plugin_settings_parse_desktop_conf (HildonPluginSettingsDialog *settings)
{
  GKeyFile *keyfile;
  gchar *config_file_path;
  gchar **panels;
  gsize n_panels;
  register gint i;

  config_file_path = g_build_filename (g_get_home_dir (),
                                       HD_DESKTOP_CONFIG_USER_PATH,
                                       HP_CONFIG_DESKTOP,
                                       NULL);

  if (!g_file_test (config_file_path, G_FILE_TEST_EXISTS))
  {
    g_free (config_file_path);

    config_file_path = g_build_filename (HD_DESKTOP_CONFIG_PATH,
                                         HP_CONFIG_DESKTOP,
                                         NULL);

    if (!g_file_test (config_file_path, G_FILE_TEST_EXISTS))
    {
      g_free (config_file_path);
      config_file_path = NULL;
    }
  }

  if (!config_file_path)
    return FALSE;

  keyfile = g_key_file_new ();

  if (!g_key_file_load_from_file
        (keyfile,config_file_path,G_KEY_FILE_NONE,NULL))
  {
    g_key_file_free (keyfile);
    return FALSE;
  }

  panels = g_key_file_get_groups (keyfile, &n_panels);

  if (!panels)
  {
    g_key_file_free (keyfile);
    return FALSE;
  }

  for (i=0; i < n_panels; i++)
  {
    HPSDTab *tab;
    gchar *config_file = NULL,
	  *plugin_dir = NULL,
	  *type = NULL;

    plugin_dir = 
      g_key_file_get_string (keyfile,
			     panels[i],
			     HD_DESKTOP_CONFIG_KEY_PLUGIN_DIR,
			     NULL);

    type = 
      g_key_file_get_string (keyfile,
		             panels[i],
			     HD_DESKTOP_CONFIG_KEY_TYPE,
			     NULL);
 
    config_file =
      g_key_file_get_string (keyfile,
                             panels[i],
                             HD_DESKTOP_CONFIG_KEY_CONFIG_FILE,
                             NULL);
     
    if (!plugin_dir || 
	!type || 
	!config_file ||
        g_str_equal (type,HD_CONTAINER_TYPE_HOME))
    {
      goto cleanup;
    }

    tab = g_new0 (HPSDTab,1);

    tab->name = g_strdup (panels[i]);
    tab->parser = 
     HILDON_PLUGIN_CONFIG_PARSER 
      (hildon_plugin_config_parser_new 
         (plugin_dir,g_build_filename (g_get_home_dir (),
				       HD_DESKTOP_CONFIG_USER_PATH,
				       config_file)));

    hildon_plugin_config_parser_set_keys (tab->parser,
					  "Name", G_TYPE_STRING,
					  "Icon", GDK_TYPE_PIXBUF,
					  "Mandatory", G_TYPE_BOOLEAN,
					  NULL);

    settings->priv->tabs = g_list_append (settings->priv->tabs, tab);

cleanup:
    g_free (plugin_dir);
    g_free (type);
    g_free (config_file);
  }

  g_strfreev (panels);
  g_key_file_free (keyfile);

  return TRUE;
}

static void 
hildon_plugin_settings_dialog_tab_free (HPSDTab *tab)
{
  g_free (tab->name);
  g_object_unref (tab->parser);
}

GtkWidget *
hildon_plugin_settings_dialog_new (void)
{
  return 
    GTK_WIDGET 
      (g_object_new (HILDON_PLUGIN_TYPE_SETTINGS_DIALOG,NULL));
}

