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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "hildon-plugin-settings-dialog.h"
#include "hildon-plugin-config-parser.h"
#include "hildon-plugin-cell-renderer-button.h"
#include "hildon-plugin-module-settings.h"

#include <gtk/gtktreemodelfilter.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <hildon-desktop/hd-config.h>

#define HPSD_OK _("tncpa_bv_tnsb_ok")
#define HPSD_CANCEL _("tncpa_bv_tnsb_cancel")

#define HPSD_TAB_SB "Statusbar"
#define HPSD_TAB_TN "Task Navigator"

#define HPSD_TITLE _("tncpa_ti_tnsb_title") 


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

#define CPA_UP_BUTTON_ICON      "qgn_indi_arrow_up"
#define CPA_DOWN_BUTTON_ICON    "qgn_indi_arrow_down"
#define CPA_ARROW_BUTTON_ICON_SIZE  26

#define HPSD_RESPONSE_UP GTK_RESPONSE_YES
#define HPSD_RESPONSE_DOWN GTK_RESPONSE_NO

#define HILDON_PLUGIN_SETTINGS_DIALOG_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_PLUGIN_TYPE_SETTINGS_DIALOG, HildonPluginSettingsDialogPrivate))


G_DEFINE_TYPE (HildonPluginSettingsDialog, hildon_plugin_settings_dialog, GTK_TYPE_DIALOG);

typedef struct 
{
  gchar *name;
  HildonPluginConfigParser *parser;
  GtkTreeView *tw;
  GtkTreeModel *filter;
  gboolean is_sorted;
  gint limit;
  guint counter_limit;
}
HPSDTab;

struct _HildonPluginSettingsDialogPrivate
{
  GList *tabs;

  GtkWidget *notebook;  

  GtkWidget *button_up;
  GtkWidget *button_down;

  gboolean type;
  gboolean hide_home;
};

enum
{
  PROP_WINDOW_TYPE=1,
  PROP_HIDE_HOME
};


static GObject *hildon_plugin_settings_dialog_constructor (GType gtype,
                                              		   guint n_params,
                                              		   GObjectConstructParam *params);

static void hildon_plugin_settings_dialog_finalize (GObject *object);

static void hildon_plugin_settings_dialog_set_property (GObject *object,
							guint prop_id,
                                            		const GValue *value,
                                            		GParamSpec *pspec);

static void hildon_plugin_settings_dialog_get_property (GObject *object,
							guint prop_id,
							GValue *value,
							GParamSpec *pspec);

static void hildon_plugin_settings_dialog_response (GtkDialog *dialog, gint response_id);

static void hildon_plugin_settings_dialog_fill_treeview (HildonPluginSettingsDialog *settings, GtkTreeView *tw);

static gboolean hildon_plugin_settings_parse_desktop_conf (HildonPluginSettingsDialog *settings);

static void hildon_plugin_settings_dialog_tab_free (HPSDTab *tab);

static void hildon_plugin_settings_dialog_response_cb (GtkDialog *dialog, gint response_id, gpointer data);

static void hildon_plugin_settings_dialog_renderer_toggled_cb (GtkCellRendererToggle *cell_renderer,
		                           		       gchar *path,
					                       GtkTreeView *tw);

static void hildon_desktop_plugin_settings_dialog_switch_nb_cb (GtkNotebook *nb,
								GtkNotebookPage *nb_page,
								guint page,
								HildonPluginSettingsDialog *settings);

static void 
hildon_plugin_settings_dialog_init (HildonPluginSettingsDialog *settings)
{
  settings->priv = HILDON_PLUGIN_SETTINGS_DIALOG_GET_PRIVATE (settings);

  settings->priv->tabs = NULL;

  settings->priv->type = HILDON_PLUGIN_SETTINGS_DIALOG_TYPE_DIALOG;

  settings->priv->hide_home = TRUE;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
}

static void 
hildon_plugin_settings_dialog_class_init (HildonPluginSettingsDialogClass *settings_class)
{
  GObjectClass *object_class   = G_OBJECT_CLASS (settings_class);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (settings_class);

  object_class->constructor = hildon_plugin_settings_dialog_constructor;
  object_class->finalize    = hildon_plugin_settings_dialog_finalize;
  
  object_class->set_property = hildon_plugin_settings_dialog_set_property;
  object_class->get_property = hildon_plugin_settings_dialog_get_property;

  dialog_class->response    = hildon_plugin_settings_dialog_response;

  g_type_class_add_private (object_class, sizeof (HildonPluginSettingsDialogPrivate));

  g_object_class_install_property (object_class,
                                   PROP_WINDOW_TYPE,
                                   g_param_spec_boolean ("window-type",
                                                         "window-type",
                                                         "Type of the settings dialog window",
                                                         HILDON_PLUGIN_SETTINGS_DIALOG_TYPE_DIALOG,
                                                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HIDE_HOME,
                                   g_param_spec_boolean ("hide-home",
                                                         "hide-home",
                                                         "Hide our beloved home",
                                                         TRUE,
                                                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

}

static void 
hildon_plugin_settings_dialog_check_limits (GtkTreeView *tw, HPSDTab *tab)
{
  gboolean selected;	
  GtkTreeIter iter;	
  GtkTreeModel *tm; /* We shouldn't care about whether we use a filter or not
		       we want to check the limits according what user sees
		       */
  
  tm = gtk_tree_view_get_model (tw);

  gtk_tree_model_get_iter_first (tm, &iter);

  do
  {
    gtk_tree_model_get (tm, &iter,
		        HP_COL_CHECKBOX, &selected,
			-1);

    if (selected)
      tab->counter_limit++;	    
  }
  while (gtk_tree_model_iter_next (tm, &iter));  

  if (tab->counter_limit > 0 && tab->counter_limit > tab->limit)
    tab->limit = tab->counter_limit;	  
}

static void 
hildon_plugin_settings_dialog_change_drag_icon_cb (GtkWidget *widget,
		                    		   GdkDragContext *context,
				                   gpointer data)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GdkPixbuf *pixbuf = NULL;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

  if (!gtk_tree_selection_get_selected(selection, &model, &iter))
    return;

  gtk_tree_model_get (model,&iter,
		      HP_COL_PIXBUF, &pixbuf,
		      -1);

  if (!pixbuf)
    return;	  

  gtk_drag_set_icon_pixbuf (context, pixbuf, 0, 0 );
  g_object_unref(pixbuf);

  g_signal_stop_emission_by_name(widget, "drag-begin");
}

static void 
hildon_plugin_settings_dialog_row_inserted_cb (GtkTreeModel *tm,
		                    	       GtkTreePath  *path,
					       GtkTreeIter  *iter,
				               GtkTreeView  *tw)
{
  gtk_tree_view_set_cursor (tw, path, NULL, FALSE);
}

static void 
hildon_plugin_settings_dialog_selection_changed_cb (GtkTreeSelection *selection,
		                                    HildonPluginSettingsDialog *settings)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean up_active = FALSE, down_active = FALSE;

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
  {
    gchar *path_str;
    path_str = gtk_tree_model_get_string_from_iter (model, &iter);

     /* Start with all sensitive and disable if needed. */
    up_active = TRUE;
    down_active = TRUE;

    if (g_str_equal(path_str, "0"))
      up_active = FALSE;

    g_free (path_str);

    if (!gtk_tree_model_iter_next (model, &iter))
      down_active = FALSE;
 }

 gtk_widget_set_sensitive (settings->priv->button_up, up_active);
 gtk_widget_set_sensitive (settings->priv->button_down, down_active);

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
	    *scrolled_window;
  GdkPixbuf *pb_up = NULL,
	    *pb_down = NULL;
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
  GList *l;

  GError *error = NULL;
  HildonPluginSettingsDialog *settings;

  object = 
    G_OBJECT_CLASS (hildon_plugin_settings_dialog_parent_class)->constructor (gtype,
		    							      n_params,
									      params);
  dialog = GTK_DIALOG (object);
  settings = HILDON_PLUGIN_SETTINGS_DIALOG (object);

  if (settings->priv->type == HILDON_PLUGIN_SETTINGS_DIALOG_TYPE_WINDOW)
  {	  
    gtk_window_set_type_hint (GTK_WINDOW (dialog), GDK_WINDOW_TYPE_HINT_NORMAL);
  }
  else 
  {
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  }
  
  gtk_window_set_title (GTK_WINDOW (dialog), HPSD_TITLE);
  gtk_dialog_set_has_separator (dialog, FALSE);

  gtk_widget_set_size_request (GTK_WIDGET (dialog), -1, 300);

  gtk_widget_push_composite_child ();

  settings->button_ok =
    gtk_dialog_add_button (dialog,
		  	 HPSD_OK,
			 GTK_RESPONSE_OK);

  button_up =
    gtk_dialog_add_button (dialog,
	  		   "Up",
	       		   HPSD_RESPONSE_DOWN);

  button_down =
    gtk_dialog_add_button (dialog,
	 		   "Down",
	       		   HPSD_RESPONSE_UP);

  settings->priv->button_up   = button_up;
  settings->priv->button_down = button_down;

  pb_down = gtk_icon_theme_load_icon
	        (icon_theme,
		 CPA_DOWN_BUTTON_ICON, CPA_ARROW_BUTTON_ICON_SIZE,
                 GTK_ICON_LOOKUP_NO_SVG, 
		 &error);

  if (!error)
  {	  
    gtk_container_remove (GTK_CONTAINER (button_down), GTK_BIN (button_down)->child);	  
    gtk_container_add (GTK_CONTAINER (button_down), 
		       gtk_image_new_from_pixbuf (pb_down));
    gtk_widget_show (GTK_BIN (button_down)->child);
    gdk_pixbuf_unref (pb_down);

  }
  else
  {	  
    g_error_free (error);	  
    error = NULL;
  }

  pb_up = gtk_icon_theme_load_icon
            (icon_theme,
  	     CPA_UP_BUTTON_ICON, CPA_ARROW_BUTTON_ICON_SIZE,
             GTK_ICON_LOOKUP_NO_SVG, 
	     &error);

  if (!error)
  {
    gtk_container_remove (GTK_CONTAINER (button_up), GTK_BIN (button_up)->child);	  
    gtk_container_add (GTK_CONTAINER (button_up), 
		       gtk_image_new_from_pixbuf (pb_up));
    gtk_widget_show (GTK_BIN (button_up)->child);
    gdk_pixbuf_unref (pb_up);

  }
  else
  {	  
    g_error_free (error);	  
    error = NULL;
  }
  
  settings->button_cancel =
    gtk_dialog_add_button (dialog,
			   HPSD_CANCEL,
		       	   GTK_RESPONSE_CANCEL);

  g_signal_connect (dialog, 
		    "response",
                    G_CALLBACK (hildon_plugin_settings_dialog_response_cb),
		    NULL);

  settings->priv->notebook = gtk_notebook_new ();

  g_signal_connect (settings->priv->notebook,
		    "switch-page",
		    G_CALLBACK (hildon_desktop_plugin_settings_dialog_switch_nb_cb),
		    (gpointer)settings);

  hildon_plugin_settings_parse_desktop_conf (settings);

  for (l = settings->priv->tabs; l != NULL; l = g_list_next (l))
  {
    GtkWidget *tw;
    gchar *path_to_save;
    gchar *basename;
    HPSDTab *tab = (HPSDTab *)l->data;
	  
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
      tw = gtk_tree_view_new_with_model (tab->parser->tm);

    g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (tw)),
		      "changed",
                      G_CALLBACK (hildon_plugin_settings_dialog_selection_changed_cb),
		      (gpointer)settings);

    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (tw), TRUE);

    g_signal_connect (tw, 
		      "drag-begin",
		      G_CALLBACK (hildon_plugin_settings_dialog_change_drag_icon_cb), 
		      NULL);

    g_signal_connect (tab->parser->tm,
		      "row-inserted",
		      G_CALLBACK (hildon_plugin_settings_dialog_row_inserted_cb),
		      tw);

    tab->tw = GTK_TREE_VIEW (tw);

    g_object_set_data (G_OBJECT (tw), "tab-data", tab);

    if (tab->limit != HPSD_NO_LIMIT)
      hildon_plugin_settings_dialog_check_limits (GTK_TREE_VIEW (tw), tab);

    hildon_plugin_settings_dialog_fill_treeview (settings, GTK_TREE_VIEW (tw));

    gtk_container_add (GTK_CONTAINER (scrolled_window), tw);

    gtk_notebook_append_page (GTK_NOTEBOOK (settings->priv->notebook),
                              scrolled_window,
                              gtk_label_new (tab->name));

    g_object_get (G_OBJECT (tab->parser), 
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
        (tab->parser, path_to_save, &error_comparing);

      if (error_comparing)
      {
        g_warning ("I couldn't compare files: %s %s", path_to_save, error_comparing->message);	      
	g_error_free (error_comparing);
      }
    }
  }
  
  gtk_container_add (GTK_CONTAINER (dialog->vbox), settings->priv->notebook);
  gtk_widget_show_all (settings->priv->notebook); 

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

  G_OBJECT_CLASS (hildon_plugin_settings_dialog_parent_class)->finalize (object);
}	

static void
hildon_plugin_settings_dialog_set_property (GObject *object,
                                            guint prop_id,
                                            const GValue *value,
                                            GParamSpec *pspec)
{
  HildonPluginSettingsDialog *settings;

  settings = HILDON_PLUGIN_SETTINGS_DIALOG (object);

  switch (prop_id)
  {
    case PROP_WINDOW_TYPE:
      settings->priv->type = g_value_get_boolean (value);
      break;

    case PROP_HIDE_HOME:
      settings->priv->hide_home = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
hildon_plugin_settings_dialog_get_property (GObject *object,
                                            guint prop_id,
                                            GValue *value,
                                            GParamSpec *pspec)
{
  HildonPluginSettingsDialog *settings;

  settings = HILDON_PLUGIN_SETTINGS_DIALOG (object);

  switch (prop_id)
  {
    case PROP_WINDOW_TYPE:
      g_value_set_boolean (value, settings->priv->type);
      break;

    case PROP_HIDE_HOME:
      g_value_set_boolean (value, settings->priv->hide_home);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void 
hildon_plugin_settings_dialog_response_cb (GtkDialog *dialog, gint response_id, gpointer data)
{
  HildonPluginSettingsDialog *settings =
    HILDON_PLUGIN_SETTINGS_DIALOG (dialog);

  if (response_id == HPSD_RESPONSE_UP || response_id == HPSD_RESPONSE_DOWN)
  {
    g_signal_stop_emission_by_name(dialog, "response");

    GtkWidget *sw =
      gtk_notebook_get_nth_page (GTK_NOTEBOOK (settings->priv->notebook),
                                 gtk_notebook_get_current_page (GTK_NOTEBOOK (settings->priv->notebook)));

    if (GTK_IS_SCROLLED_WINDOW (sw))
    {
      GtkTreeView *tw = GTK_TREE_VIEW (GTK_BIN (sw)->child);
      GtkTreeModel *model;
      GtkTreeSelection *selection;
      GtkTreePath *path;
      GtkTreeIter iter,_iter;
      GtkTreeIter next,_next;

      model = gtk_tree_view_get_model (tw);

      selection = gtk_tree_view_get_selection (tw);

      if (!selection)
        return;

      if (!gtk_tree_selection_get_selected (selection,&model,&iter))
        return;

      path = gtk_tree_model_get_path (model,&iter);

      if (response_id == HPSD_RESPONSE_DOWN)
        gtk_tree_path_prev (path);
      else
        gtk_tree_path_next (path);

      if (gtk_tree_model_get_iter (model, &next, path))
      {
        if (GTK_IS_TREE_MODEL_FILTER (model))
        {
          gtk_tree_model_filter_convert_iter_to_child_iter
            (GTK_TREE_MODEL_FILTER (model), &_iter, &iter);

          gtk_tree_model_filter_convert_iter_to_child_iter
            (GTK_TREE_MODEL_FILTER (model), &_next,&next);

          gtk_list_store_swap
            (GTK_LIST_STORE (gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model))),
             &_next, &_iter);
        }
        else
          gtk_list_store_swap (GTK_LIST_STORE (model), &next, &iter);

        gtk_tree_selection_unselect_all (selection);
        gtk_tree_selection_select_path (selection, path);
	gtk_tree_view_scroll_to_cell (tw, path, NULL, FALSE, 1.0, 0);
      }

      gtk_tree_path_free(path);
    }
  }
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
                  *renderer_button = hildon_plugin_cell_renderer_button_new ();

  GtkTreeViewColumn *column_pb,
                    *column_toggle,
                    *column_text,
                    *column_button;

  column_pb =
    gtk_tree_view_column_new_with_attributes
      (NULL, renderer_pixbuf, "pixbuf", HP_COL_PIXBUF, NULL);

  column_toggle =
    gtk_tree_view_column_new_with_attributes
      (NULL, renderer_toggle, "active", HP_COL_CHECKBOX, NULL);

  column_text =
    gtk_tree_view_column_new_with_attributes
      (NULL, renderer_text, "text", HP_COL_NAME, NULL);

  column_button = 
    gtk_tree_view_column_new_with_attributes
      (NULL, renderer_button, "plugin-module", HP_COL_SETTINGS, NULL);    

  gtk_tree_view_append_column (GTK_TREE_VIEW (tw), column_pb);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tw), column_text);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tw), column_button);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tw), column_toggle);

  g_signal_connect (G_OBJECT(renderer_toggle), 
		    "toggled",
		    G_CALLBACK (hildon_plugin_settings_dialog_renderer_toggled_cb),
		    (gpointer)tw);
}	

static gboolean 
hildon_plugin_settings_parse_desktop_conf (HildonPluginSettingsDialog *settings)
{
  GKeyFile *keyfile;
  gchar *config_file_path;
  gchar **panels;
  const gchar *home = g_get_home_dir ();
  gsize n_panels;
  register gint i;

  config_file_path = g_build_filename (home,
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
    GError *error = NULL;
    HPSDTab *tab;
    gchar *config_file = NULL,
	  *plugin_dir = NULL,
	  *type = NULL,
	  *path_to_save = NULL;
    gboolean is_sorted = TRUE;

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

    is_sorted =
      g_key_file_get_boolean (keyfile,
		      	      panels[i],
	  		      HD_DESKTOP_CONFIG_KEY_IS_ORDERED,
			      &error);
    if (error)
    {
      is_sorted = TRUE;
      g_error_free (error);
    }      
    
    if (!plugin_dir || 
	!type || 
	!config_file ||
        (g_str_equal (type,HD_CONTAINER_TYPE_HOME) && settings->priv->hide_home))
    {
      goto cleanup;
    }

    path_to_save = g_build_filename (home,
		    		     HD_DESKTOP_CONFIG_USER_PATH,
				     config_file,
				     NULL);

    tab = g_new0 (HPSDTab,1);
    
    tab->name = g_strdup (panels[i]);
    tab->parser = 
     HILDON_PLUGIN_CONFIG_PARSER 
      (hildon_plugin_config_parser_new (plugin_dir,path_to_save));
	 
    tab->tw = NULL;
    tab->filter = NULL;
    tab->is_sorted = is_sorted;
    tab->limit = HPSD_NO_LIMIT;
    tab->counter_limit = 0; /*Note to myself: create the damned _new function!!! */

    g_free (path_to_save);

    hildon_plugin_config_parser_set_keys (tab->parser,
					  "Name", G_TYPE_STRING,
					  "Icon", GDK_TYPE_PIXBUF,
					  "Mandatory", G_TYPE_BOOLEAN,
					  "X-Settings", HILDON_PLUGIN_TYPE_MODULE_SETTINGS,
					  "Category", G_TYPE_STRING,
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

static gint 
hildon_plugin_settings_dialog_compare_tab (gconstpointer a,
					   gconstpointer b)
{
  const HPSDTab *tab1;
  const gchar *name;

  tab1 = (const HPSDTab *)a;
  name = (const gchar *)b;  

  if (g_str_equal (tab1->name, name))
    return 0;
   
  return 1;
}

static gint 
hildon_plugin_settings_dialog_compare_tab_tw (gconstpointer a,
					      gconstpointer b)
{
  const HPSDTab *tab;
  const GtkTreeView *tw;

  tab = (const HPSDTab *)a;
  tw  = (const GtkTreeView *)b;
  
  if (tab->tw == tw)
    return 0;
   
  return 1;
}

static void 
hildon_plugin_settings_dialog_renderer_toggled_cb (GtkCellRendererToggle *cell_renderer,
                           		           gchar *path,
					           GtkTreeView *tw)
{
  GtkTreeIter iter;
  gboolean selected;
  GtkTreeModel *tm = gtk_tree_view_get_model (tw);
  HPSDTab *tab = NULL;
  
  if (!gtk_tree_model_get_iter_from_string (tm, &iter, path))
    return;
  
  gtk_tree_model_get (tm, &iter, 
		      HP_COL_CHECKBOX, &selected, 
		      -1);

  tab = g_object_get_data (G_OBJECT (tw),"tab-data");

  if (tab && tab->limit != HPSD_NO_LIMIT)
  {
    if (!selected && tab->counter_limit >= tab->limit)
      return;
    else
    if (tab->counter_limit <= tab->limit)
      tab->counter_limit = (!selected) ? tab->counter_limit + 1: tab->counter_limit - 1;	    

    if (tab->counter_limit < 0)
      tab->counter_limit = 0;	    
  }	  
  
  if (GTK_IS_TREE_MODEL_FILTER (tm))
  {
    GtkTreeIter real_iter;
    
    gtk_tree_model_filter_convert_iter_to_child_iter
        (GTK_TREE_MODEL_FILTER (tm),&real_iter,&iter);

    gtk_list_store_set 
      (GTK_LIST_STORE (gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER(tm))), &real_iter,
       HP_COL_CHECKBOX, !selected,
       -1);
  }
  else
  {  
    gtk_list_store_set (GTK_LIST_STORE (tm), &iter,
                        HP_COL_CHECKBOX, !selected,
		        -1);
  }
}

static void 
hildon_desktop_plugin_settings_dialog_switch_nb_cb (GtkNotebook *nb,
						    GtkNotebookPage *nb_page,
						    guint page,
						    HildonPluginSettingsDialog *settings)
{
  GtkTreeIter iter;	
  GtkWidget *sw =
      gtk_notebook_get_nth_page (GTK_NOTEBOOK (settings->priv->notebook),page);
                                
  if (GTK_IS_SCROLLED_WINDOW (sw))
  {
    GtkTreeView *tw = GTK_TREE_VIEW (GTK_BIN (sw)->child);
    GList *container_tab = NULL;

    gtk_tree_model_get_iter_first (gtk_tree_view_get_model (tw), &iter);

    gtk_tree_selection_select_iter (gtk_tree_view_get_selection (tw), &iter);

    gtk_widget_set_sensitive (settings->priv->button_up, FALSE);
    
    container_tab = 
      g_list_find_custom (settings->priv->tabs,
			  tw,
			  (GCompareFunc)hildon_plugin_settings_dialog_compare_tab_tw);

    if (!container_tab)
      return; 
			  
    HPSDTab *tab = (HPSDTab *)container_tab->data;

    gtk_widget_set_sensitive (settings->priv->button_down, tab->is_sorted);
  }
}

GtkWidget *
hildon_plugin_settings_dialog_new (GtkWindow *parent)
{
  GtkWidget *dialog =  
    GTK_WIDGET 
      (g_object_new (HILDON_PLUGIN_TYPE_SETTINGS_DIALOG,NULL));

  if (parent && GTK_IS_WINDOW (parent))
    gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);	    

  return dialog;
}

GList *
hildon_plugin_settings_dialog_get_container_names (HildonPluginSettingsDialog *settings)
{
  GList *names = NULL, *l;
  
  for (l = settings->priv->tabs; l != NULL ; l = g_list_next (l))
     names = g_list_append (names, ((HPSDTab *)l->data)->name);

  return names;
}

GtkTreeModel *
hildon_plugin_settings_dialog_get_model_by_name (HildonPluginSettingsDialog *settings,
						 const gchar *name,
						 gboolean filter)
{
  GList *container_tab = NULL;


  container_tab =
    g_list_find_custom (settings->priv->tabs,
                        name,
                        (GCompareFunc)hildon_plugin_settings_dialog_compare_tab);

  if (!container_tab)
    return NULL;

  HPSDTab *tab = (HPSDTab *)container_tab->data;

  if (filter && tab->filter)
    return tab->filter;
  else
  if (tab->filter)
    return gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (tab->filter));
  else
    return gtk_tree_view_get_model (GTK_TREE_VIEW (tab->tw));	  
}
	
void
hildon_plugin_settings_dialog_set_choosing_limit (HildonPluginSettingsDialog *settings,
                                                  const gchar *container_name,
                                                  gint limit)
{
  GList *container_tab = NULL;


  container_tab = 
    g_list_find_custom (settings->priv->tabs,
   		        container_name,
			(GCompareFunc)hildon_plugin_settings_dialog_compare_tab);

  if (!container_tab)
    return;

  HPSDTab *tab = (HPSDTab *)container_tab->data;

  tab->limit = limit;

  hildon_plugin_settings_dialog_check_limits (tab->tw, tab);
}

GtkTreeModel *
hildon_plugin_settings_dialog_set_visibility_filter (HildonPluginSettingsDialog *settings,
					             const gchar *container_name,
					             GtkTreeModelFilterVisibleFunc visible_func,
					             gpointer data,
						     GtkDestroyNotify destroy)
{
  GtkTreeModel *filter;
  GList *container_tab = NULL;


  container_tab = 
    g_list_find_custom (settings->priv->tabs,
   		        container_name,
			(GCompareFunc)hildon_plugin_settings_dialog_compare_tab);

  if (!container_tab)
    return NULL;

  HPSDTab *tab = (HPSDTab *)container_tab->data;

  if (!tab->filter)
  {	  
    filter = gtk_tree_model_filter_new (tab->parser->tm, NULL);
    tab->filter = filter;
  }
  else
    filter = tab->filter;	  

  gtk_tree_model_filter_set_visible_func 
    (GTK_TREE_MODEL_FILTER (filter),
     visible_func,
     data,
     destroy);

  gtk_tree_view_set_model (tab->tw, filter);
     
  return filter; 
}

GtkTreeModel *
hildon_plugin_settings_dialog_set_modify_filter (HildonPluginSettingsDialog *settings,
					         const gchar *container_name,
						 gint n_columns,
						 GType *types,
					         GtkTreeModelFilterModifyFunc modify_func,
						 gpointer data,
						 GtkDestroyNotify destroy)
{
  GtkTreeModel *filter;
  GList *container_tab = NULL;


  container_tab = 
    g_list_find_custom (settings->priv->tabs,
   		        container_name,
			(GCompareFunc)hildon_plugin_settings_dialog_compare_tab);

  if (!container_tab)
    return NULL;

  HPSDTab *tab = (HPSDTab *)container_tab->data;

  if (!tab->filter)
  {	  
    filter = gtk_tree_model_filter_new (tab->parser->tm, NULL);
    tab->filter = filter;
  }
  else
    filter = tab->filter;	  

  gtk_tree_model_filter_set_modify_func 
    (GTK_TREE_MODEL_FILTER (filter),
     n_columns,
     types,
     modify_func,
     data,
     destroy);

  gtk_tree_view_set_model (tab->tw, filter);
     
  return filter; 
}

void
hildon_plugin_settings_dialog_set_cell_data_func (HildonPluginSettingsDialog *settings,
						  HildonPluginSettingsDialogColumn column,
						  const gchar *container_name,
                                                  GtkTreeCellDataFunc func,
                                                  gpointer func_data,
                                                  GtkDestroyNotify destroy)
{
  GList *container_tab = NULL;

  container_tab =
    g_list_find_custom (settings->priv->tabs,
                        container_name,
                        (GCompareFunc)hildon_plugin_settings_dialog_compare_tab);
  
  if (!container_tab)
    return;

  HPSDTab *tab = (HPSDTab *)container_tab->data;

  GtkTreeViewColumn *twcolumn = gtk_tree_view_get_column (tab->tw, column);

  if (!twcolumn)
    return;
  
  GList *renderers = gtk_tree_view_column_get_cell_renderers (twcolumn);

  /* We only have a cell renderer per column */

  if (!renderers)
    return;

  gtk_tree_view_column_set_cell_data_func (twcolumn,
					   GTK_CELL_RENDERER (renderers->data),
					   func,
					   func_data,
					   destroy);

  g_list_free (renderers);
}

void 
hildon_plugin_settings_dialog_rename_tab (HildonPluginSettingsDialog *settings,
					  const gchar *container_name,
					  const gchar *new_name)
{
  GList *container_tab = NULL;

  if (!container_name || !new_name)
    return;	  

  container_tab =
    g_list_find_custom (settings->priv->tabs,
                        container_name,
                        (GCompareFunc)hildon_plugin_settings_dialog_compare_tab);
  
  if (!container_tab)
    return;

  HPSDTab *tab = (HPSDTab *)container_tab->data;

  if (tab->tw)
  {
    GtkWidget *page = gtk_widget_get_parent (GTK_WIDGET (tab->tw));

    gtk_notebook_set_tab_label (GTK_NOTEBOOK (settings->priv->notebook),
		    		page,
				gtk_label_new (_(new_name)));
  }
}

