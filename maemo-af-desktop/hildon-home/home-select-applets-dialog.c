/* This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
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
 **/

/**
 * @file home-select-applets-dialog.c
 *
 **/

#include <locale.h>
#include <libintl.h> 
#include <string.h>

#include <gdk/gdkkeysyms.h>

#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "home-select-applets-dialog.h"
#include "hildon-home-main.h"
#include "applet-manager.h"
#include "layout-manager.h"

/* log include */
#include <osso-log.h>

#include <osso-helplib.h>

#define DIALOG_WIDTH    300


extern osso_context_t *osso_home;


/* Dialog data contents structure */
static SelectAppletsData data_t;
static GtkWidget *home_applets_scrollwin;


/* Private function declarations */
static
gboolean select_applets_key_pressed_cb(GtkWidget *widget, 
		                       GdkEventKey *event, 
				       gpointer unused_data);
static 
void select_applets_toggled_cb(GtkCellRendererToggle *cell_renderer, 
		               gchar *path, 
			       gpointer user_data);
static
void select_applets_reload_applets(char *applets_path,
		                   gpointer user_data); 


/* Function definitions */

/**
 * @show_select_applets_dialog
 *
 * @param GList*  currently showed applets
 * @param GList** added desktop file strings 
 * @param GList** removed desktop file strings
 * @param osso_context* home_help the home libosso context
 *
 * Select and deselect applets
 **/
void show_select_applets_dialog(GList *applets, 
        		        GList **added_list, 
		                GList **removed_list)
{
    /* Dialog widget */
    GtkWidget *dialog = NULL;
    /* Buttons */
    GtkWidget *button_ok;
    GtkWidget *button_cancel;
    
    /* Return value */
    int ret;
    
    /* GtkTreeView widgets */
    GtkTreeView *treeview;
    GtkCellRenderer *cell_renderer;
    
    /* GList containers */
    GList *add_list = NULL;
    GList *remove_list = NULL;      
   
 
    /* TreeView model */
    data_t.model_data = GTK_TREE_MODEL(gtk_list_store_new
		    (APPLETS_LIST_COLUMNS, 
		     G_TYPE_BOOLEAN, /* Checkbox */
	             G_TYPE_STRING,  /* Applet name */
		     G_TYPE_STRING   /* Desktop file */
		    ));
    
    treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(data_t.model_data));
    g_object_set(treeview, "allow-checkbox-mode", FALSE, NULL);

    
    /* Init the SelectAppletsData struct with currently shown applets */
    data_t.list_data = applets;
   
      
    /* TreeView cell renderers */
    
    /*    :toggle cell renderer */
    cell_renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(G_OBJECT(cell_renderer), "toggled", 
		    G_CALLBACK(select_applets_toggled_cb), 
		    data_t.model_data); 
    gtk_tree_view_insert_column_with_attributes(treeview, -1, NULL,
		    cell_renderer, "active", CHECKBOX_COL, NULL);
    g_object_set(cell_renderer, "activatable", TRUE, NULL);
    
    /*    :text cell renderer */
    gtk_tree_view_insert_column_with_attributes(treeview, -1, NULL,
		    gtk_cell_renderer_text_new(), "text", 
		    APPLET_NAME_COL, NULL);

    g_signal_connect(G_OBJECT(treeview), "key-press-event",
		     G_CALLBACK(select_applets_key_pressed_cb), 
		     data_t.model_data);
    
    /* Initialize the dnotify callback for .desktop directory */
    if( hildon_dnotify_set_cb
		    ((hildon_dnotify_cb_f *)select_applets_reload_applets,
		     (char *)HOME_APPLETS_DESKTOP_DIR, 
		     NULL) == HILDON_ERR ) {
        ULOG_WARN("Could not set notify for directory:%s\n", 
		  HOME_APPLETS_DESKTOP_DIR);
    }
    
    /* Create scrolled window */
    home_applets_scrollwin = gtk_scrolled_window_new(NULL, NULL);
    
    /* Set the shadow type */
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(home_applets_scrollwin), 
		                        GTK_SHADOW_ETCHED_IN);
    
    /* Init the vertical scrollbar */   
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(home_applets_scrollwin), 
		                   GTK_POLICY_NEVER, GTK_POLICY_NEVER);
    
    /* Add the treeview to the window */
    gtk_container_add(GTK_CONTAINER(home_applets_scrollwin), GTK_WIDGET(treeview));

    /* Populate the applets list */
    select_applets_reload_applets(HOME_APPLETS_DESKTOP_DIR, NULL);
	
    
    /* Create the dialog */
    dialog = gtk_dialog_new_with_buttons(HOME_APPLETS_SELECT_TITLE, 
		                         NULL, 
		                         GTK_DIALOG_MODAL | 
					 GTK_DIALOG_DESTROY_WITH_PARENT |
					 GTK_DIALOG_NO_SEPARATOR, NULL);

    gtk_widget_set_size_request (dialog, DIALOG_WIDTH, -1);

    /* Add help button to the dialog */
    ossohelp_dialog_help_enable(GTK_DIALOG(dialog),
                                HILDON_HOME_SELECT_APPLETS_HELP_TOPIC,
                                osso_home);
    
    /* Add "OK" button to the dialog */
    button_ok = gtk_dialog_add_button(GTK_DIALOG(dialog), 
		                      HOME_APPLETS_SELECT_OK, 
				      GTK_RESPONSE_OK);
    /* Add "Cancel" button to the dialog */
    button_cancel = gtk_dialog_add_button(GTK_DIALOG(dialog), 
		                          HOME_APPLETS_SELECT_CANCEL, 
					  GTK_RESPONSE_CANCEL);
    
    /* Add the scrolled window to the dialog */
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 
		      home_applets_scrollwin); 
    
    /* Show 'em! */
    gtk_widget_show_all(dialog);

    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    
    /* User clicked OK - Save and/or remove applets */
    if (ret == GTK_RESPONSE_OK) {
    
        GtkTreeIter iter;
	
	if( !gtk_tree_model_get_iter_first(data_t.model_data, &iter) ) {
	    ULOG_WARN("FAILED to initialize GtkTreeIter, "
		      "could not save or remove applets.\n");
	}
        else {   
	    GList *find_list = NULL;
	    gchar *desktop_file = NULL;
	    gboolean enabled;
	      
	    
	    do {
	        gtk_tree_model_get(data_t.model_data, &iter, 
				   CHECKBOX_COL, &enabled, 
				   DESKTOP_FILE_COL, &desktop_file, -1);
		
		/* find the applet in selected applets */
		find_list = g_list_find_custom(data_t.list_data, desktop_file, 
                                               (GCompareFunc)strcmp);
		
		/* if not found but enabled, add it to the added list */
		if ( find_list == NULL && desktop_file && enabled ) {
		    add_list = g_list_append(add_list, desktop_file);
	            ULOG_WARN("\nADDING-APPLET: %s", desktop_file);	    
		}
		/* or if disabled but found from selected list, removed list */
		else if( find_list && desktop_file && !enabled && 
			 find_list->data != NULL &&
                         g_str_equal(find_list->data, desktop_file) ) { 
		    remove_list = g_list_append(remove_list, desktop_file);	
		    ULOG_WARN("\nREMOVING-APPLET: %s", desktop_file);    
		}
		
		find_list = NULL;
		desktop_file = NULL;
	    
	    } while( gtk_tree_model_iter_next(data_t.model_data, &iter) != FALSE );
	    
	    
	    /* Cleanup */
	    if( find_list ) {
                g_list_free(find_list);
            }
        }
   } /* .. if OK */
    
	  
   if( hildon_dnotify_remove_cb
		   (HOME_APPLETS_DESKTOP_DIR) == HILDON_ERR ) {
       ULOG_WARN("FAILED to remove notify for the directory:%s!\n",
                 HOME_APPLETS_DESKTOP_DIR);
   }

   /* Store the return values */
   *added_list = add_list;
   *removed_list = remove_list;

   remove_list = NULL;

   /* Cleanup */			   	 
   gtk_widget_destroy(home_applets_scrollwin);
   gtk_widget_destroy(dialog);

    
   return;
}

/**
 * @select_applets_selected
 *
 * @param widget The parent widget
 * @param data Pointer to the GtkWidget* (in fact container)
 *
 * Calls activation of Select Applets dialog
 **/
void select_applets_selected(GtkEventBox *home_event_box,
	                     GtkFixed *home_fixed,
                             GtkWidget *titlebar_label)
{	
   GList *added_applets;
   GList *removed_applets, *iter_removed_applets;  
	
   /* Return if no home available */
   g_return_if_fail(home_event_box && home_fixed);
   
   /* Get currently showed applets from the applet manager */
   applet_manager_t *applet_manager_instance = 
	   applet_manager_singleton_get_instance();
   /* Return if no applet manager available */
   g_return_if_fail(applet_manager_instance); 
   GList *showed_list = applet_manager_get_identifier_all
	   (applet_manager_instance);

   /* Call Select Applets -dialog */
   show_select_applets_dialog(showed_list, &added_applets, &removed_applets);
   
   /* If applets to be added, call layout manager */
   if ( added_applets ) {   
       ULOG_ERR("\nHOME-SELECT-APPLET-DIALOG - "
                "adding applets now calling layout mode\n");
	   
       layout_mode_begin(home_event_box, home_fixed,
                         added_applets, removed_applets,
                         titlebar_label);
       
       g_list_foreach(added_applets, (GFunc)g_free, NULL);
       g_list_free(added_applets);
   }
   /* If applets are to be removed */
   else if ( removed_applets ) {
       iter_removed_applets = g_list_first(removed_applets);
       
       while( iter_removed_applets ) {
   	       
           /* Deinitialize removed applets */
	   GtkEventBox *removed_event = applet_manager_get_eventbox
		   (applet_manager_instance, (gchar *)iter_removed_applets->data);
           applet_manager_deinitialize(applet_manager_instance, 
			               (gchar *)iter_removed_applets->data);
	   gtk_container_remove(GTK_CONTAINER(home_fixed), 
			        GTK_WIDGET(removed_event));
	   removed_event = NULL;
	   
           iter_removed_applets = iter_removed_applets->next;
       }	  
       applet_manager_configure_save_all(applet_manager_instance);   
   }

   /* Cleanup */
   if( removed_applets ) {
       g_list_foreach(removed_applets, (GFunc)g_free, NULL);
       g_list_free(removed_applets);
   }
    
   
   return;
}


/**
 * @select_applets_key_pressed_cb
 *
 * @param GtkWidget* : The parent, GtkTreeView widget
 * @param GdkEventKey* : Key event 
 * @param gpointer : not used user data 
 *
 * Calls item (de)activation if Enter key was pressed
 **/
static 
gboolean select_applets_key_pressed_cb(GtkWidget *widget, 
		                       GdkEventKey *event, 
				       gpointer unused_data)
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    gboolean active;
	    
    if ( !widget ) return FALSE; 

    
    switch( event->keyval ) {
	    
    case GDK_Return:
    case GDK_KP_Enter:

        /* Get selected list item */
	if ( !(model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget))) ) {
	    ULOG_WARN("Could not get the Home applets dialog plugin list.");
	    return FALSE;
	}
	if ( !(selection = gtk_tree_view_get_selection
				(GTK_TREE_VIEW(widget))) ) {
	    ULOG_WARN("Could not get selected Home applets dialog applet.");
	    return FALSE;
	}
	if ( !(gtk_tree_selection_get_selected(selection, &model, &iter)) ) {
	    /* No item selected, return */ return FALSE;
	}
	
	/* Get boolean value */
	gtk_tree_model_get(model, &iter, CHECKBOX_COL, &active, -1);
	/* Toggle item selected value on the TreeModel */
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
			   CHECKBOX_COL, !active, -1);    	
        return TRUE;
    default:
	return FALSE;
    }
	
    return FALSE;
}
	    

/**
 * 
 * @select_applets_toggled_cb
 *
 * @param GtkCellRendererToggle callback caller object
 * @param gchar* the toggled Treeview path string
 * @param data Pointer to the GtkTreeModel data
 *
 * Changes the toggle_button state on the TreeModel
 **/
static 
void select_applets_toggled_cb(GtkCellRendererToggle *cell_renderer,
		               gchar *path, 
			       gpointer user_data)
{
    GtkTreeIter iter;
    gboolean active;
    
    /* Get the GtkTreeModel iter */
    GtkTreeModel *model = GTK_TREE_MODEL(user_data);

    if ( !gtk_tree_model_get_iter_from_string(model, &iter, path) ) {
        ULOG_ERR("FAILED to find Tree path, not toggled!");
	return;
    }

    /* Get boolean value */
    gtk_tree_model_get(model, &iter, CHECKBOX_COL, &active, -1);
    
    /* Change the iter value on the TreeModel */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		       CHECKBOX_COL, !active, -1);
}

/**
 * @select_applets_reload_applets
 *
 * @param char* monitored directory string
 * @param gpointer user_data (not used)
 *
 * Reload the applets list from monitored .desktop directory path
 **/
static
void select_applets_reload_applets(char *applets_path, 
                                   gpointer user_data)	
{
    GDir *applet_desktop_base_dir;
    GError *error = NULL;
    /* Key parser file object */
    GKeyFile *desktop;
    const gchar *entry = NULL;
    gchar *applet_name = NULL;

    int rows = 0;
    GtkTreeIter iter;

    
    /* Open the .desktop directory */
    if( (applet_desktop_base_dir = g_dir_open(HOME_APPLETS_DESKTOP_DIR,
				              0, &error)) == NULL ) {
        ULOG_WARN("FAILED to open path: %s", error->message);
	g_error_free(error);
	
	return;
    }

    /* Clear old data */
    gtk_list_store_clear(GTK_LIST_STORE(data_t.model_data));
		
    /* Read all .desktop files from directory */
    entry = g_dir_read_name(applet_desktop_base_dir);

    while (entry != NULL) {
        gchar *indexfile = g_strconcat(HOME_APPLETS_DESKTOP_DIR,
			               entry, NULL);

	/* Read only .desktop file entry */
	if ( !g_str_has_suffix(entry, HOME_APPLETS_DESKTOP_SUFFIX) ) {
	    entry = g_dir_read_name(applet_desktop_base_dir); 
	    continue; 
	}
	
	/* The .desktop path entry */
	if (g_file_test(indexfile, G_FILE_TEST_EXISTS) == TRUE) {
		
	    desktop = g_key_file_new();
	    
	    if ( !g_key_file_load_from_file(desktop, indexfile,
				            G_KEY_FILE_NONE, &error) ) {
	        ULOG_WARN("FAILED to open applet .desktop file: %s\n",
			  error->message);
		g_error_free(error);
	    }
	    else {
                /* get the applet name key value from .desktop file */
	       if ( (applet_name = g_key_file_get_string
				       (desktop, 
					HOME_APPLETS_DESKTOP_GROUP, 
					HOME_APPLETS_DESKTOP_NAME_KEY, 
					&error)) == NULL) {
                   ULOG_WARN("FAILED to read desktop file Icon key value: %s\n",
	                     error->message);
                   g_error_free(error);
                   error = NULL;
                }
                /* and restore it to TreeModel */
	        else {
		    gtk_list_store_append(GTK_LIST_STORE(data_t.model_data), &iter);
	            gtk_list_store_set(GTK_LIST_STORE(data_t.model_data), &iter,
				       CHECKBOX_COL, FALSE,
				       APPLET_NAME_COL, _(applet_name),
				       DESKTOP_FILE_COL, indexfile, -1);

		    rows++;
		    
                    /* if the applet already exists in applet manager */
                    if( (g_list_find_custom(data_t.list_data, indexfile, 
				            (GCompareFunc)strcmp)) != NULL ) {
                        gtk_list_store_set(GTK_LIST_STORE(data_t.model_data), &iter,
	                                   CHECKBOX_COL, TRUE, -1);
                    }
		}
            }

            /* Cleanup */
            g_key_file_free(desktop);
	    g_free(applet_name);
        }

        entry = g_dir_read_name(applet_desktop_base_dir);
        g_free(indexfile);
	
    } /* ..while( entry ) */

    g_dir_close(applet_desktop_base_dir);


    /* Sort by the user visible name */
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(data_t.model_data),
	                                 APPLET_NAME_COL, GTK_SORT_ASCENDING);

    /* Update dialog height */
    if ( rows > HOME_APPLETS_MAXIMUM_VISIBLE_ROWS ) {
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(home_applets_scrollwin), 
			GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_widget_set_size_request(home_applets_scrollwin, -1, 
			HOME_APPLETS_DIALOG_HEIGHT);	
    } else  {
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(home_applets_scrollwin), 
			GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	gtk_widget_set_size_request(home_applets_scrollwin, -1, 
			(gint)((HOME_APPLETS_DIALOG_HEIGHT*rows)/HOME_APPLETS_MAXIMUM_VISIBLE_ROWS));
    }
    
    gtk_widget_queue_draw ( home_applets_scrollwin ); 

}
