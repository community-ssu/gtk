/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
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
 */

/**
 * @file hildon-home-main.c
 *
 */
 

#define USE_AF_DESKTOP_MAIN__
 
 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>
#include <locale.h>
#include <libintl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libosso.h>
#include <libmb/mbutil.h>
#include <glob.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

#include "libmb/mbdotdesktop.h"
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-caption.h>
#include "hildon-home-applet.h"
#include "hildon-home-plugin-loader.h"
#include "hildon-home-main.h"
#include "hildon-home-interface.h"
#include "../kstrace.h"

/* log include */
#include <log-functions.h>  	 

static GtkWidget *window = NULL;
static GtkWidget *home_base_fixed;
static GtkWidget *home_base_eventbox;

static gboolean menu_key_pressed = FALSE;
static gboolean home_is_topmost;

static GtkWidget *home_fixed;
/* Menu and titlebar */
static GtkWidget *titlebar_menu = NULL;
static GtkWidget *menu_item[MAX_APPLETS];
static GtkWidget *shortcut_properties;
static GtkWidget *titlebar_submenu = NULL;

static gboolean shortcut_applet_hideable;     /* FALSE if not allowed */
static gboolean shortcut_properties_editable; /* FALSE if not allowed */
static gboolean background_changable;         /* FALSE if not allowed */

/* applet area */
static GtkWidget *home_area_eventbox;
static GtkWidget *applet_area[MAX_APPLETS];

static gboolean plugin_exists[MAX_APPLETS];

AppletInfo applet_info[MAX_APPLETS];

static HildonHomePluginLoader *plugin[MAX_APPLETS];

static GtkIconTheme *icon_theme;
static osso_context_t *osso_home;

static const gchar *home_user_dir;
static const gchar *home_user_image_dir = NULL;  /*full dir path */

static const gchar *home_factory_bg_image = NULL;/*full file path */
static const gchar *user_original_bg_image = "";/*file having file path*/
static const gchar *home_current_bg_image = NULL;/*full file path */
static gchar *home_bg_image_uri = NULL;/*full file path */
static gchar *home_bg_image_in_use_uri = NULL;/*full file path */
static const gchar *home_bg_image_filename = NULL; /*file name (short)*/
static gint home_bg_combobox_active_item = -1; 
static gboolean home_bg_image_loading_necessary = FALSE;
static gboolean home_bg_image_symlinked = FALSE;

static const gchar *titlebar_original_image_savefile;
static const gchar *sidebar_original_image_savefile;

static GtkWidget *loading_image_note = NULL;
static gboolean loading_image_process_active = FALSE;
static GPid image_loader_pid = -1;

static gint num_applets = NUM_STATIC_APPLETS;
static gchar *user_applets;  

static MBDotDesktop *personalization;
static MBDotDesktop *screen_calibration;

static void create_startup_lock(void);
static gboolean startup_lock_exists(void);

/* --------------------------*/
/* titlebar area starts here */
/* --------------------------*/


/**
 * @titlebar_menu_position_func
 *
 * @param menu
 * @param x
 * @param y
 * @param push_in
 * @param user_data
 * 
 * Handles the positioning of the HildonHome 'titlebar' menu.
 */

static 
void titlebar_menu_position_func(GtkMenu *menu, gint *x, gint *y,
                                 gboolean *push_in,
                                 gpointer user_data)
{
    *x =  HILDON_HOME_TITLEBAR_X_OFFSET_DEFAULT;
    *y =  HILDON_HOME_TITLEBAR_Y_OFFSET_DEFAULT;

    gtk_widget_style_get (GTK_WIDGET (menu), "horizontal-offset", x,
                          "vertical-offset", y, NULL);
    
    *x += HILDON_TASKNAV_WIDTH;
    *y += HILDON_HOME_TITLEBAR_HEIGHT;
}


/**
 * @menu_popup_handler
 *
 * @param titlebar
 * @param event
 * @param user_data
 *
 * Handles the activation of the titlebar menu
 */

static 
gboolean menu_popup_handler(GtkWidget *titlebar,
                            GdkEvent *event, gpointer user_data)
{
    gdouble x_win = 0, y_win;
    gdk_event_get_coords (event, &x_win, &y_win);
    /* Some trickery to only popup the menu at right area of titlebar*/
    if (x_win < HILDON_HOME_MENUAREA_LMARGIN || 
        x_win > HILDON_HOME_MENUAREA_WIDTH + HILDON_HOME_MENUAREA_LMARGIN)
    {
        return FALSE;
    }
    gtk_menu_popup(GTK_MENU(titlebar_menu), NULL, NULL,
                   (GtkMenuPositionFunc)titlebar_menu_position_func,
                   NULL, 1,
                   gtk_get_current_event_time());
    gtk_menu_shell_select_first (GTK_MENU_SHELL(titlebar_menu), TRUE);
    return TRUE;
}


/**
 * @menu_popdown_handler
 *
 * @param titlebar
 * @param event
 * @param user_data
 *
 * Handles the popdown of the menu for the case where button is released.
 * Automatic handling fails when release happens on the Home titlebar
*/

static 
gboolean menu_popdown_handler(GtkWidget *titlebar, GdkEvent *event,
                              gpointer user_data)
{
    gdouble x_win = 0, y_win;
    gdk_event_get_coords (event, &x_win, &y_win);
    if (x_win < HILDON_HOME_MENUAREA_LMARGIN ||
        x_win > HILDON_HOME_MENUAREA_WIDTH + HILDON_HOME_MENUAREA_LMARGIN)
    {
        return TRUE;
    }
    gtk_menu_popdown(GTK_MENU(titlebar_menu));
    return FALSE;
}

/**
 * @toggle_applet_visibility
 *
 * @param menuitem
 * @param user_data
 *
 * If applet is not created and loaded, do before toggle.
 * Toggles the visiblity of the individual applets.
 */

static 
void toggle_applet_visibility(GtkCheckMenuItem *menuitem,
                              gpointer user_data)
{
    gboolean visibility = gtk_check_menu_item_get_active(menuitem);
    gint applet_num = (gint)user_data;

    applet_info[applet_num].status = !(applet_info[applet_num].status);
    if (applet_info[applet_num].status && applet_area[applet_num] == NULL)
    {
        gint statesize = 0;
        void *statedata = NULL;
        gint fd;

        fd = osso_state_open_read(osso_home);

        if (fd != -1)
        {
            read(fd, &statesize, sizeof(int));
            if (statesize > 0)
            {
                if ((statedata = g_malloc0(statesize*sizeof(void))) 
                    == NULL)
                {
                    osso_state_close(osso_home, fd);
                    return;
                }
                read(fd, statedata, statesize);
            }
        }
        applet_area[applet_num] = 
            create_applet(&plugin[applet_num], 
                          applet_info[applet_num].plugin_name,
                          statesize, statedata,
                          applet_num);
        if (statedata != NULL)
        {
            g_free(statedata);
        }

        osso_state_close(osso_home, fd);
        if(plugin_exists[applet_num])
        {
            set_applet_width_and_position(applet_num);
        }
    }
    if(!plugin_exists[applet_num]) 
    {
        applet_info[applet_num].status = FALSE;
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(menu_item[applet_num]),FALSE);
        return;
    }

    if (applet_num == HILDON_HOME_APPLET_WEB_SHORTCUT &&
        shortcut_properties_editable &&
        shortcut_properties != NULL) 
    {
        gtk_widget_set_sensitive(GTK_WIDGET(shortcut_properties), 
                                 applet_info[applet_num].status);
        if(applet_info[applet_num].status)
        {
            hildon_home_plugin_applet_foreground(plugin[applet_num]);
        }
    }
    
    g_object_set(G_OBJECT(applet_area[applet_num]), 
                 "visible", visibility, NULL);

    if (applet_num >= NUM_STATIC_APPLETS)
    {
        gchar *user_applets_new;

        if (applet_info[applet_num].status)
        {
            if (user_applets)
                user_applets_new = g_strconcat(g_strstrip(user_applets),
                    " ", applet_info[applet_num].plugin_name, NULL);
            else
                user_applets_new = g_strdup(applet_info[applet_num].plugin_name);
        } else
        {
            gchar **tokens;

            tokens = g_strsplit(user_applets,
                                applet_info[applet_num].plugin_name,
                                2);
            user_applets_new = g_strconcat(tokens[0], tokens[1], NULL);
            g_strfreev(tokens);
        }
        g_free(user_applets);
        user_applets = user_applets_new;
    }

    hildon_home_save_configure();
    return;
}

/**
 * @construct_titlebar_area
 * 
 * constructs titlebar area with label
 */

static 
void construct_titlebar_area()
{
    GtkWidget *titlebar_eventbox;
    GtkWidget *titlebar_label;

    titlebar_eventbox = gtk_event_box_new();
    gtk_widget_set_size_request( GTK_WIDGET( titlebar_eventbox ), 
                                 HILDON_HOME_TITLEBAR_WIDTH, 
                                 HILDON_HOME_TITLEBAR_HEIGHT);

    gtk_fixed_put(GTK_FIXED( home_fixed ), 
                  GTK_WIDGET( titlebar_eventbox ),  
                  HILDON_HOME_TITLEBAR_X, HILDON_HOME_TITLEBAR_Y );
    gtk_event_box_set_visible_window(GTK_EVENT_BOX( titlebar_eventbox ),
                                     FALSE);

    /* Set handler for clicks to handle the invocation of the menu */

    g_signal_connect(G_OBJECT(titlebar_eventbox), "button-press-event",
                     G_CALLBACK(menu_popup_handler), GTK_WIDGET(window));
    g_signal_connect(G_OBJECT(titlebar_eventbox), "button-release-event",
                     G_CALLBACK(menu_popdown_handler), GTK_WIDGET(window));

    titlebar_label = gtk_label_new(HILDON_HOME_TITLEBAR_MENU_LABEL);
    hildon_gtk_widget_set_logical_font (titlebar_label,
                                        HILDON_HOME_TITLEBAR_MENU_LABEL_FONT);
    hildon_gtk_widget_set_logical_color(titlebar_label,
                                        GTK_RC_FG,
                                        GTK_STATE_NORMAL, 
                                        HILDON_HOME_TITLEBAR_MENU_LABEL_COLOR);

    gtk_fixed_put(GTK_FIXED(home_fixed), 
                  GTK_WIDGET(titlebar_label),
                  HILDON_HOME_TITLEBAR_MENU_LABEL_X,
                  HILDON_HOME_TITLEBAR_MENU_LABEL_Y);
    gtk_widget_show(titlebar_label);
}


/**
 * @construct_titlebar_menu
 *
 * Constructs the titlebar menu and sets the relevant callbacks
 */

static 
void construct_titlebar_menu()
{    
    GtkWidget *separator_item;
    GtkWidget *screen_item;
    GtkWidget *set_bg_image_item;
    GtkWidget *personalisation_item;
    GtkWidget *calibration_item;
    gint applet_num;

    titlebar_menu = gtk_menu_new();
    
    gtk_widget_set_name(titlebar_menu, HILDON_HOME_TITLEBAR_MENU_NAME); 

    for(applet_num=0;applet_num<NUM_STATIC_APPLETS;applet_num++)
    {
        if(applet_num != HILDON_HOME_APPLET_WEB_SHORTCUT || 
           shortcut_applet_hideable) 
        {
            menu_item[applet_num] = 
                gtk_check_menu_item_new_with_label(
                    applet_info[applet_num].label);
            gtk_widget_show(menu_item[applet_num]);
            gtk_menu_append(GTK_MENU(titlebar_menu), 
                            menu_item[applet_num]);
            if (applet_info[applet_num].status)
            {
                if(plugin_exists[applet_num])
                {
                gtk_check_menu_item_set_active(
                    GTK_CHECK_MENU_ITEM(menu_item[applet_num]),TRUE);                
                    hildon_home_plugin_applet_foreground(
                        plugin[applet_num]);
                    g_object_set(G_OBJECT(applet_area[applet_num]), 
                                 "visible", TRUE, NULL);
                }
            }
            g_signal_connect(G_OBJECT(menu_item[applet_num]), 
                             "toggled",
                             G_CALLBACK(toggle_applet_visibility), 
                             (gint *)applet_num);
        }
    }

    separator_item = gtk_separator_menu_item_new();
    gtk_widget_show(separator_item);
    gtk_menu_append(GTK_MENU(titlebar_menu), separator_item);

    if(shortcut_properties_editable && 
       plugin_exists[HILDON_HOME_APPLET_WEB_SHORTCUT])
    {
        shortcut_properties =
            hildon_home_plugin_applet_properties(
                plugin[HILDON_HOME_APPLET_WEB_SHORTCUT],
                GTK_WINDOW(window));
        if(shortcut_properties != NULL)
        {
            gtk_widget_show(shortcut_properties);
            gtk_menu_append(GTK_MENU(titlebar_menu), shortcut_properties);
            gtk_widget_set_sensitive(
                GTK_WIDGET(shortcut_properties), 
                applet_info[HILDON_HOME_APPLET_WEB_SHORTCUT].status);
        }
    }


    /* Sub menu */
    titlebar_submenu = gtk_menu_new();

    screen_item =
        gtk_menu_item_new_with_label(HILDON_HOME_TITLEBAR_MENU_SCREEN);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(screen_item), titlebar_submenu);
    gtk_menu_append(GTK_MENU(titlebar_menu), screen_item);
    gtk_widget_show(screen_item);    
    if(background_changable)
    {
        set_bg_image_item =
            gtk_menu_item_new_with_label(HILDON_HOME_TITLEBAR_SUB_SET_BG);
        gtk_widget_show(set_bg_image_item);
        gtk_menu_append(GTK_MENU(titlebar_submenu), set_bg_image_item);
        g_signal_connect(G_OBJECT(set_bg_image_item), "activate",
                         G_CALLBACK(set_background_dialog_selected), NULL);
    }
    
    personalisation_item =
        gtk_menu_item_new_with_label(HILDON_HOME_TITLEBAR_SUB_PERSONALISATION);
    gtk_widget_show(personalisation_item);
    gtk_menu_append(GTK_MENU(titlebar_submenu), personalisation_item);
    if(personalization != NULL)
    {
        g_signal_connect(G_OBJECT(personalisation_item), "activate",
                         G_CALLBACK(personalisation_selected), NULL);
    }

    calibration_item =
        gtk_menu_item_new_with_label(HILDON_HOME_TITLEBAR_SUB_CALIBRATION);
    gtk_widget_show(calibration_item);
    gtk_menu_append(GTK_MENU(titlebar_submenu), calibration_item);
    if(screen_calibration != NULL)
    {
        g_signal_connect(G_OBJECT(calibration_item), "activate",
                         G_CALLBACK(screen_calibration_selected), NULL);
    }
}

/* ------------------------*/
/* titlebar area ends here */
/* ------------------------*/

/* ------------------------*/
/* appler area starts here */
/* ------------------------*/

/**
 * @get_filename_from_treemodel
 *       
 * @param tree GtkTreeModel containing data in colums of         
 *  G_TYPE_STRING, G_TYPE_STRING and G_TYPE_INT          
 *       
 * @param index index of the treemodel where the required data   
 * is to be dug out of   
 *       
 * @return *gchar the filename   
 *       
 *       
 * Digs up a path from the treemodel at index.   
 */      
         
static 
gchar *get_filename_from_treemodel(GtkComboBox *box, gint index)
{        
    gchar *name = NULL;          
    GtkTreeIter iter;    
    GtkTreePath *walkthrough;    
    GtkTreeModel *tree = gtk_combo_box_get_model(box);   
    
    walkthrough = gtk_tree_path_new_from_indices(index, -1);
    
    gtk_tree_model_get_iter(tree, &iter, walkthrough);   
   
    gtk_tree_model_get (tree, &iter,
                        BG_IMAGE_FILENAME, &name,
			-1);

    if (walkthrough != NULL) 
    {
        gtk_tree_path_free(walkthrough);     
    }    

    return name;         
}        
 

/**
 * @get_priority_from_treemodel
 *
 * @param tree GtkTreeModel containing priority in column
 *  BG_IMAGE_PRIORITY of type G_TYPE_INT
 *
 * @param *iter a pointer to iterator of the correct node
 *
 * @return gint the priority fetched
 * 
 * Digs up a gint value from the treemodel at iter
 **/
static 
gint get_priority_from_treemodel(GtkTreeModel *tree, GtkTreeIter *iter)
{
    gint priority = -1;
    GValue value = { 0, };
    GValue value_priority = { 0, };        
    
    gtk_tree_model_get_value(tree, iter, BG_IMAGE_PRIORITY, &value);
    
    g_value_init(&value_priority, G_TYPE_INT);
    
    if (g_value_transform(&value, &value_priority)) {
        priority = g_value_get_int(&value_priority);
    }

    return priority;
}

/**
 * @personalisation_selected
 *
 * @param widget The parent widget
 * @param event The event that caused this callback
 * @param data Pointer to the data (not actually used by this function)
 * 
 * @return TRUE (keeps cb alive)
 * 
 * Calls activation of personalisation applet
 **/
static 
gboolean personalisation_selected(GtkWidget *widget, 
                                  GdkEvent *event,
                                  gpointer data)
{ 
    int ret;
    ret = osso_cp_plugin_execute(osso_home,
                                 (char *)mb_dotdesktop_get(
                                     personalization, 
                                     "X-control-panel-plugin"),
                                 NULL,
                                 TRUE);

    switch(ret) 
    {
    case OSSO_OK:
        break;
    case OSSO_ERROR:
        osso_log(LOG_ERR,"OSSO_ERROR (No help for such topic ID)\n");
        break;
    case OSSO_RPC_ERROR:
        osso_log(LOG_ERR,"OSSO_RPC_ERROR (RPC failed for Personalization)\n");
        break;
    case OSSO_INVALID:
        osso_log(LOG_ERR,"OSSO_INVALID (invalid argument)\n");
        break;
    default:
        osso_log(LOG_ERR,"Unknown error!\n");
        break;
    }

    return TRUE;
}

/**
 * @screen_calibration_selected
 *
 * @param widget The parent widget
 * @param event The event that caused this callback
 * @param data Pointer to the data (not actually used by this function)
 * 
 * @return TRUE (keeps cb alive)
 * 
 * Calls activation of screen calibration applet
 **/
static 
gboolean screen_calibration_selected(GtkWidget *widget, 
                                     GdkEvent *event,
                                     gpointer data)
{ 
    int ret;

    ret = osso_cp_plugin_execute(osso_home,
                                 (char *)mb_dotdesktop_get(
                                     screen_calibration, 
                                     "X-control-panel-plugin"),
                                 NULL,
                                 TRUE);

    switch(ret) 
    {
    case OSSO_OK:
        break;
    case OSSO_ERROR:
        osso_log(LOG_ERR,"OSSO_ERROR (No help for such topic ID)\n");
        break;
    case OSSO_RPC_ERROR:
        osso_log(LOG_ERR,"OSSO_RPC_ERROR (RPC failed for Screen calibration)\n");
        break;
    case OSSO_INVALID:
        osso_log(LOG_ERR,"OSSO_INVALID (invalid argument)\n");
        break;
    default:
        osso_log(LOG_ERR,"Unknown error!\n");
        break;
    }

    return TRUE;
}

/**
 * @load_original_bg_image_uri
 * 
 * Sets original image uri to global variable from save file. If not     
 * set uses hc default. Also sets the shortened filename without         
 * path to a global variable.    
 */
static 
void load_original_bg_image_uri()
{
    FILE *fp;
    gchar bg_uri[HILDON_HOME_PATH_STR_LENGTH] = {0};
    GnomeVFSURI *uri;

    fp = fopen (user_original_bg_image, "r");
    if(fp == NULL) 
    {
        osso_log(LOG_ERR, "Couldn't open file %s for reading image name", 
                 user_original_bg_image);
        home_bg_image_uri = "";

        home_bg_image_filename = "";

        return;
    }
    if(fscanf(fp, HILDON_HOME_BG_FILENAME_FORMAT_SAVE, bg_uri) < 0)
    {
        osso_log(LOG_ERR, "Couldn't load image uri from file %s", 
                 user_original_bg_image);
    }
    if(fp != NULL && fclose(fp) != 0) 
    {
        osso_log(LOG_ERR, "Couldn't close file %s", 
                 user_original_bg_image);
    }
    home_bg_image_uri = g_strdup_printf("%s", bg_uri);
    home_bg_image_in_use_uri = g_strdup(home_bg_image_uri);

    uri = gnome_vfs_uri_new (home_bg_image_uri);
    home_bg_image_filename = gnome_vfs_uri_extract_short_name(uri);
    gnome_vfs_uri_unref (uri);
}


/**
 * @set_background_select_file_dialog
 * 
 * @param tree Pointer to GtkTreeModel containing the 
 *        contents of image select combobox in 'set
 *        background' dialog (HOM-DIA002)
 *
 * @return TRUE
 * 
 * 'Select image file' dialog (WID-DIA003). Updates combobox
 * content in 'set background' dialog (HOM-DIA002).
 * 
 **/
static 
gboolean set_background_select_file_dialog(GtkComboBox *box)
{
    GtkTreeModel *tree = gtk_combo_box_get_model(box);
    GtkWidget *dialog;
    GtkFileFilter *mime_type_filter;
    gint response;
    gchar *name_str, *temp_name, *dot;
    gchar *chooser_name;


    dialog = hildon_file_chooser_dialog_new_with_properties(
        GTK_WINDOW(window),
        HILDON_HOME_FILE_CHOOSER_ACTION_PROP,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        HILDON_HOME_FILE_CHOOSER_TITLE_PROP,
        HILDON_HOME_FILE_CHOOSER_TITLE,
        HILDON_HOME_FILE_CHOOSER_SELECT_PROP,
        HILDON_HOME_FILE_CHOOSER_SELECT,
        HILDON_HOME_FILE_CHOOSER_EMPTY_PROP,
        HILDON_HOME_FILE_CHOOSER_EMPTY,
        NULL);
        
    mime_type_filter = gtk_file_filter_new();
    gtk_file_filter_add_mime_type (mime_type_filter, "image/jpeg");
    gtk_file_filter_add_mime_type (mime_type_filter, "image/gif");
    gtk_file_filter_add_mime_type (mime_type_filter, "image/png");
    gtk_file_filter_add_mime_type (mime_type_filter, "image/bmp");
    gtk_file_filter_add_mime_type (mime_type_filter, "image/tiff");
    gtk_file_filter_add_pattern (mime_type_filter, "*.png");    

    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog),
                                 mime_type_filter);
    if(!gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (dialog),
                                            home_user_image_dir))
    {
        osso_log(LOG_ERR, "Couldn't set default image dir for dialog %s", 
                 home_user_image_dir);
    }

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_OK) 
    {
	GnomeVFSURI *uri_tmp;

        chooser_name = 
            gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
        
	uri_tmp = gnome_vfs_uri_new (chooser_name);
        name_str = gnome_vfs_uri_extract_short_name(uri_tmp);
	gnome_vfs_uri_unref (uri_tmp);

        temp_name = g_strdup(name_str);
        dot = g_strrstr(temp_name, ".");
        if (dot && dot != temp_name) {
            *dot = '\0';
            g_free((gchar *)name_str);
            name_str = g_strdup(temp_name);
        }
        g_free(temp_name);

        
        if(name_str != NULL && name_str[0] != '\0') 
        {
            GtkTreeIter iter;            
            GtkTreePath *path;
            gint priority = 0;
            gtk_list_store_append(GTK_LIST_STORE(tree), &iter);
            gtk_list_store_set(GTK_LIST_STORE(tree),
                               &iter,
                               BG_IMAGE_NAME, name_str,
                               BG_IMAGE_FILENAME, chooser_name,
                               BG_IMAGE_PRIORITY, G_MAXINT, -1);

            gtk_combo_box_set_active_iter(box, &iter);
            home_bg_combobox_active_item = 
                gtk_combo_box_get_active(GTK_COMBO_BOX(box));

            path = gtk_tree_model_get_path(tree, &iter);

            gtk_tree_path_next(path);

            if (gtk_tree_model_get_iter(tree, &iter, path)) 
            {
                priority = get_priority_from_treemodel(tree, &iter);
                if (priority == G_MAXINT) 
                {
                    gtk_list_store_remove(GTK_LIST_STORE(tree), &iter);
                    
                }
            }
            gtk_tree_path_free(path);
        }
        g_free(name_str);
        gtk_widget_destroy(dialog);    
        return TRUE;
    }  
    
    gtk_widget_destroy(dialog);
    return FALSE;
}


/**
 * @combobox_active_tracer
 *
 * @param combobox the emitting combobox
 * @param data Pointer to GtkDialog containing the combobox
 *
 * Handles changed - signals, recording the active combobox item
 * to global variable so as to be accesible in other functions.
 **/


static void combobox_active_tracer(GtkWidget *combobox,
                                   gpointer data)
{
    gint active_index = -1;
    GtkDialog *dialog = GTK_DIALOG(data);

    active_index = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    home_bg_combobox_active_item = active_index; 

    if (active_index != -1) 
    {       
        gchar *image_name = get_filename_from_treemodel
            (GTK_COMBO_BOX(combobox), 
             active_index);
        if (home_bg_image_in_use_uri != NULL &&
            image_name != NULL &&
            g_str_equal(home_bg_image_in_use_uri, image_name))
        {
            home_bg_image_loading_necessary = FALSE;
            gtk_dialog_set_response_sensitive(dialog,
                                              GTK_RESPONSE_APPLY , FALSE);
        }
        else
        {
            home_bg_image_loading_necessary = TRUE;
            if (home_bg_image_uri) {
                g_free(home_bg_image_uri);
            }
            home_bg_image_uri = image_name;
            gtk_dialog_set_response_sensitive(dialog,
                                              GTK_RESPONSE_APPLY, TRUE);
        }
    }
} 

/**
 * @set_background_response_handler
 *
 * @param dialog The parent widget
 * @param arg The event that caused this handler's call
 * @param data Pointer to label telling user image name in 
 *        'set background' dialog (HOM-DIA002)
 * 
 * Handles response signals and in case of browsing activates
 * new dialog (WID-DIA003) for file retrieving
 **/

static 
void set_background_response_handler(GtkWidget *dialog, 
                                     gint arg, gpointer data)
{
    GtkComboBox *box = GTK_COMBO_BOX(data);
    gchar *image_name;
    gint active_index = -1;
    
    switch (arg) 
    {
    case GTK_RESPONSE_YES:
        g_signal_stop_emission_by_name(dialog, "response");
        if (set_background_select_file_dialog(box))
        {
            active_index = gtk_combo_box_get_active(box);
            image_name = get_filename_from_treemodel(box, active_index);
            if (home_bg_image_in_use_uri != NULL &&
                image_name != NULL &&
                g_str_equal(home_bg_image_in_use_uri, image_name))
            {
                
                gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                                  GTK_RESPONSE_APPLY , FALSE);
                while(gtk_main_iteration());
            } else
            {
                home_bg_image_loading_necessary = TRUE;
                gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                                  GTK_RESPONSE_APPLY , TRUE);
            }
        }
        break;
    case GTK_RESPONSE_APPLY:
        g_signal_stop_emission_by_name(dialog, "response");
        home_bg_image_loading_necessary = FALSE;

        if (home_bg_combobox_active_item != -1) 
        {
            construct_background_image_with_uri(
                get_filename_from_treemodel(box, 
                                            home_bg_combobox_active_item),
                TRUE);
            gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                              GTK_RESPONSE_APPLY , FALSE);
        }
        break;
    default:
        break;
    }
}

/**
 * @set_background_dialog_selected
 *
 * @param widget The parent widget
 * @param event The event that caused this callback
 * @param data Pointer to the data (not actually used by this function)
 * 
 * @return TRUE (keeps cb alive)
 * 
 * Set background image dialog (HOM-DIA002).
 **/
static 
gboolean set_background_dialog_selected(GtkWidget *widget, 
                                        GdkEvent *event,
                                        gpointer data)
{ 
    GtkWidget *dialog;
    GtkWidget *hbox_label;

    GtkWidget *combobox_image_select;
    GtkWidget *image_caption;
    GtkSizeGroup *group;

    GtkCellRenderer *renderer; 
    GtkTreeModel *combobox_contents;
    GtkTreeIter iterator;
    GDir *bg_image_desc_base_dir;
    MBDotDesktop *file_contents;
    GError *error = NULL;
    GtkTreeModel * model;

    gchar *dot;
    gchar *temp_name;
    gchar *image_name = NULL;
    gchar *image_path = NULL;
    gint image_order;
    const gchar *image_desc_file;
    gboolean bg_image_is_default = FALSE;
    gint combobox_active = -1;
    gint active_count = -1;
    gint width;
    
    bg_image_desc_base_dir = g_dir_open(
         HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR, 0, &error);

    combobox_contents = 
        GTK_TREE_MODEL (
            gtk_list_store_new (
                3,               /* number of parameters */
                G_TYPE_STRING,   /* image localised descriptive name */
                G_TYPE_STRING,   /* image file path &name */
                G_TYPE_INT       /* Image priority */
                )
            );

    load_original_bg_image_uri();

    if (bg_image_desc_base_dir == NULL) 
    {
        osso_log(LOG_ERR, "Failed to open path: %s", error->message);
        if(error != NULL)
        {
            g_error_free(error);
        }
        image_desc_file = NULL;
    } else 
    {
        image_desc_file = g_dir_read_name(bg_image_desc_base_dir);        
    }
    
    
    while (image_desc_file != NULL) 
    {
        if (g_str_has_suffix(image_desc_file, 
                             BG_IMG_INFO_FILE_TYPE)) 
        {
            gchar *filename;
            filename = g_build_filename(
                HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR,
                image_desc_file, NULL);
            file_contents = mb_dotdesktop_new_from_file(filename);
            g_free(filename);
            
            if (file_contents == NULL) 
            {
                osso_log(LOG_ERR, "Failed to read file: %s\n", 
                         image_desc_file);
            } else 
            {
                gchar *maybe_int;
                image_name = g_strdup(mb_dotdesktop_get(
                                          file_contents,
                                          BG_DESKTOP_IMAGE_NAME));
                
                image_path = g_strdup(mb_dotdesktop_get(
                                          file_contents,
                                          BG_DESKTOP_IMAGE_FILENAME));
                
                maybe_int = mb_dotdesktop_get(file_contents,
                                              BG_DESKTOP_IMAGE_PRIORITY);
                
                
                if (maybe_int != NULL && g_ascii_isdigit(*maybe_int)) 
                {
                    image_order = (gint)atoi(maybe_int);
                } else 
                {
                    image_order = HOME_BG_IMG_DEFAULT_PRIORITY;
                }
                
                
                if (image_name != NULL && image_path != NULL) 
                {
                    active_count++;
                    
                    if (image_path != NULL && 
                        home_bg_image_uri != NULL &&
                        g_str_equal(image_path, home_bg_image_uri)) 
                    {
                        bg_image_is_default = TRUE;
                        /* This is calculated after the
                           combobox_contents is sorted 
                           combobox_active = active_count;
                        */
                    }   
                    
                    gtk_list_store_append(GTK_LIST_STORE
                                         (combobox_contents),
                                         &iterator);
                   
                   gtk_list_store_set(GTK_LIST_STORE(combobox_contents),
                                      &iterator,
                                      BG_IMAGE_NAME, _(image_name),
                                      BG_IMAGE_FILENAME, image_path,
                                      BG_IMAGE_PRIORITY, image_order, -1);
               } else 
               {
                   osso_log(LOG_ERR, "Desktop file malformed: %s", 
                            image_desc_file);
               }

               mb_dotdesktop_free(file_contents);
               
               g_free(image_name);
               g_free(image_path);
            }            
        } else /*suffix test*/ 
        {
            osso_log(LOG_ERR, "Unexpected file type: %s ",image_desc_file);
        }
        
        image_desc_file = g_dir_read_name(bg_image_desc_base_dir);

    } /*while*/

    if (bg_image_desc_base_dir != NULL) 
    {
        g_dir_close(bg_image_desc_base_dir);        
        
    }

    gtk_tree_sortable_set_sort_column_id(
        GTK_TREE_SORTABLE(combobox_contents),
        BG_IMAGE_PRIORITY, 
        GTK_SORT_ASCENDING);
    
    
    if (bg_image_is_default == FALSE)
    {
        if(home_bg_image_symlinked)
        {
            /* default in use, default is first always */
            combobox_active = 0;
        } else
        {

        gtk_list_store_append(GTK_LIST_STORE
                              (combobox_contents), 
                              &iterator);
        
        temp_name = g_strdup(home_bg_image_filename);
        dot = g_strrstr(temp_name, ".");
        if (dot && dot != temp_name) 
        {
            *dot = '\0';
            g_free((gchar *)home_bg_image_filename);
            home_bg_image_filename = g_strdup(temp_name);
        }
        g_free(temp_name);
        

        gtk_list_store_set(GTK_LIST_STORE(combobox_contents),
                           &iterator,
                           BG_IMAGE_NAME, home_bg_image_filename,
                           BG_IMAGE_FILENAME, home_bg_image_uri,
                           BG_IMAGE_PRIORITY, G_MAXINT, -1);

        active_count++;
        
        combobox_active = active_count;
        }
    } else 
    {
        /*
         * We have found an already selected file but we need to figure
         * out where it is in the new sorted list.
         */
        gboolean is_node;
        gint ac = 0;
        model = GTK_TREE_MODEL(combobox_contents);

        is_node = gtk_tree_model_get_iter_first(model, &iterator);
        while (is_node) 
        {
            gchar * filename;
            gtk_tree_model_get(model, &iterator, 1, &filename, -1);
            if (filename != NULL &&
                home_bg_image_uri != NULL &&
                g_str_equal(filename, home_bg_image_uri)) 
            {
                combobox_active = ac;
                g_free(filename);
                break;
            }
                
            is_node = gtk_tree_model_iter_next(model, &iterator);
            ac++;
            g_free(filename);
        }
    }
    
    dialog = 
        gtk_dialog_new_with_buttons(HILDON_HOME_SET_BG_TITLE,
                                    GTK_WINDOW(window),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    HILDON_HOME_SET_BG_OK,
                                    GTK_RESPONSE_OK,
                                    HILDON_HOME_SET_BG_BROWSE,
                                    GTK_RESPONSE_YES,
                                    HILDON_HOME_SET_BG_APPLY,
                                    GTK_RESPONSE_APPLY,
                                    HILDON_HOME_SET_BG_CANCEL,
                                    GTK_RESPONSE_CANCEL,
                                    NULL);
    
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
    gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                      GTK_RESPONSE_APPLY , FALSE);
    hbox_label = gtk_hbox_new( FALSE, 10);
    combobox_image_select = 
        gtk_combo_box_new_with_model(combobox_contents);

    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox_image_select),
                             combobox_active);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox_image_select),
                               renderer, 
                               TRUE);

    
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combobox_image_select),
                                   renderer, "text", 
                                   BG_IMAGE_NAME, NULL);

    group= GTK_SIZE_GROUP( gtk_size_group_new( GTK_SIZE_GROUP_HORIZONTAL ) );
    image_caption = 
        hildon_caption_new(group, 
                           HILDON_HOME_SET_BG_IMAGE,
                           combobox_image_select, 
                           NULL, 
                           HILDON_CAPTION_OPTIONAL);

    gtk_window_get_size (GTK_WINDOW (dialog), &width, NULL);
    gtk_widget_set_size_request (combobox_image_select, width, -1);
    
    gtk_box_pack_start( GTK_BOX(hbox_label), 
                        image_caption,
                        FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),hbox_label);

    gtk_widget_show_all(dialog);

    g_signal_connect(G_OBJECT(combobox_image_select), "changed", 
                     G_CALLBACK(combobox_active_tracer), 
                     dialog);

    g_signal_connect(G_OBJECT(dialog), "response", 
                     G_CALLBACK(set_background_response_handler), 
                     combobox_image_select);


    switch (gtk_dialog_run(GTK_DIALOG(dialog)))
    {
    case GTK_RESPONSE_OK:
        if(home_bg_image_uri != NULL && 
           home_bg_combobox_active_item != -1 &&
           home_bg_image_loading_necessary) 
        {              
            construct_background_image_with_uri(home_bg_image_uri, TRUE);
        }
        break;
    default:
        break;
    }

    gtk_widget_destroy(dialog);
    return TRUE;
} 

/**
 * @show_no_memory_note
 *
 * Shows information note for lack of memory.
 *
 */

static
void show_no_memory_note(void)
{
    GtkWidget *no_memory_note = NULL;

    no_memory_note =
        hildon_note_new_information(GTK_WINDOW(window), 
                                    HILDON_HOME_NO_MEMORY_TEXT);
    
    hildon_note_set_button_text(HILDON_NOTE(no_memory_note),
                                HILDON_HOME_NO_MEMORY_OK);
    gtk_dialog_run (GTK_DIALOG (no_memory_note));
    if(no_memory_note != NULL) 
    {
        gtk_widget_destroy (GTK_WIDGET (no_memory_note));
    }
}

/**
 * @show_connectivity_broke_note
 *
 * Shows information note for connection breakage
 *
 */

static
void show_connectivity_broke_note(void)
{
    GtkWidget *connectivity_broke_note = NULL;

    connectivity_broke_note =
        hildon_note_new_information(GTK_WINDOW(window), 
                                    HILDON_HOME_CONNECTIVITY_TEXT);
    
    hildon_note_set_button_text(HILDON_NOTE(connectivity_broke_note),
                                HILDON_HOME_CONNECTIVITY_OK);
    gtk_dialog_run (GTK_DIALOG (connectivity_broke_note));
    if(connectivity_broke_note != NULL) 
    {
        gtk_widget_destroy (GTK_WIDGET (connectivity_broke_note));
    }
}

/**
 * @show_system_resource_note
 *
 * Shows information note for lack of system resources
 *
 */

static
void show_system_resource_note(void)
{
    GtkWidget *system_resource_note = NULL;

    system_resource_note =
        hildon_note_new_information(GTK_WINDOW(window), 
                                    HILDON_HOME_SYSTEM_RESOURCE_TEXT);
    
    hildon_note_set_button_text(HILDON_NOTE(system_resource_note),
                                HILDON_HOME_SYSTEM_RESOURCE_OK);
    gtk_dialog_run (GTK_DIALOG (system_resource_note));
    if(system_resource_note != NULL) 
    {
        gtk_widget_destroy (GTK_WIDGET (system_resource_note));
    }
}

/**
 * @show_file_corrupt_note
 *
 * Shows information note for file corruption
 *
 */

static
void show_file_corrupt_note(void)
{
    GtkWidget *file_corrupt_note = NULL;

    file_corrupt_note =
        hildon_note_new_information(GTK_WINDOW(window), 
                                    HILDON_HOME_FILE_CORRUPT_TEXT);
    
    hildon_note_set_button_text(HILDON_NOTE(file_corrupt_note),
                                HILDON_HOME_FILE_CORRUPT_OK);
    gtk_dialog_run (GTK_DIALOG (file_corrupt_note));
    if(file_corrupt_note != NULL) 
    {
         gtk_widget_destroy (GTK_WIDGET (file_corrupt_note));
    }
}
/**
 * @show_file_unreadable_note
 *
 * Shows information note for file opning failure
 *
 */

static
void show_file_unreadable_note(void)
{
    GtkWidget *file_unreadable_note = NULL;

    file_unreadable_note =
        hildon_note_new_information(GTK_WINDOW(window), 
                                    HILDON_HOME_FILE_UNREADABLE_TEXT);
    
    hildon_note_set_button_text(HILDON_NOTE(file_unreadable_note),
                                HILDON_HOME_FILE_UNREADABLE_OK);
    gtk_dialog_run (GTK_DIALOG (file_unreadable_note));
    if(file_unreadable_note != NULL) 
    {
         gtk_widget_destroy (GTK_WIDGET (file_unreadable_note));
    }
}

/**
 * @show_mmc_cover_open_note
 *
 * Shows information note when called.
 *
 */

static
void show_mmc_cover_open_note(void)
{
    GtkWidget *cover_open_note = NULL;

    cover_open_note =
        hildon_note_new_information(GTK_WINDOW(window), 
                                    HILDON_HOME_MMC_NOT_OPEN_TEXT);
    
    hildon_note_set_button_text(HILDON_NOTE(cover_open_note),
                                HILDON_HOME_MMC_NOT_OPEN_CLOSE);
    gtk_dialog_run (GTK_DIALOG (cover_open_note));
    if(cover_open_note != NULL) 
    {
         gtk_widget_destroy (GTK_WIDGET (cover_open_note));
    }
}


/**
 * @construct_background_image
 * 
 * @argument_list: commandline argumentlist for systemcall
 * @loading_image_note_allowed: whatever if it is nesessary for user to be able
 * cancel image loading
 * 
 * Calls image loader with argument list and waits loader to save
 * results appointed place(s)
 */
static
void construct_background_image(char *argument_list[], 
                                gboolean loading_image_note_allowed)
{
    GError *error = NULL;
    guint image_loader_callback_id;

    loading_image_process_active = TRUE;

    if(image_loader_pid != -1)
    {
        g_spawn_close_pid(image_loader_pid);
        kill(image_loader_pid, SIGTERM);
    }

    if(!g_spawn_async(
           /* child's current working directory,
            * or NULL to inherit parent's */
           NULL,
           /* child's argument vector. [0] is the path of the
            * program to execute */
           argument_list,
           /* child's environment, or NULL to inherit
            * parent's */
           NULL,
           /* flags from GSpawnFlags */
           G_SPAWN_DO_NOT_REAP_CHILD,
           /* function to run in the child just before
            * exec() */
           NULL,
           /* user data for child_setup */
           NULL,
           /* return location for child process ID or NULL */
           &image_loader_pid,
           /* return location for error */
           &error)
        )
    {
        osso_log(LOG_ERR, "construct_background_image():%s", 
                 error->message);
        if(error != NULL)
        {
            g_error_free(error);
        }
        image_loader_pid = -1;
        return;
    }

    image_loader_callback_id = 
        g_child_watch_add(image_loader_pid, image_loader_callback, NULL);

    if(loading_image_note_allowed)
    {
        show_loading_image_note();
    }
}

/**
 * @image_loader_callback
 *
 * @pid Pid of image loader child process
 * @child_exit_status exit status of image loader process
 * @user_data cancel note id
 *
 * Removes child processes and calls notification functions if needed
 */
static 
void image_loader_callback(GPid pid, gint child_exit_status, gpointer data)
{    
    /* image loader process is done and succesful operation cannot be
       canceled */
    loading_image_process_active = FALSE;

    if(loading_image_note != NULL && GTK_IS_WIDGET(loading_image_note))
    {
        gtk_widget_destroy(loading_image_note);
    }

    loading_image_note = NULL;

    child_exit_status = child_exit_status >> 8;
    switch(child_exit_status)
    {
    case HILDON_HOME_IMAGE_LOADER_OK:
        home_bg_image_symlinked = FALSE;
        if (home_bg_image_in_use_uri) 
        {
            g_free(home_bg_image_in_use_uri);
        }
        
        home_bg_image_in_use_uri = g_strdup(home_bg_image_uri);
        refresh_background_image();
        break;
    case HILDON_HOME_IMAGE_LOADER_ERROR_MEMORY:
        show_no_memory_note();
        break;
    case HILDON_HOME_IMAGE_LOADER_ERROR_CONNECTIVITY:
        show_connectivity_broke_note();
        break;
    case HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE:
        show_system_resource_note();
        break;
    case HILDON_HOME_IMAGE_LOADER_ERROR_FILE_CORRUPT:
        show_file_corrupt_note();
        break;
    case HILDON_HOME_IMAGE_LOADER_ERROR_FILE_UNREADABLE:
        show_file_unreadable_note();
        break;
    case HILDON_HOME_IMAGE_LOADER_ERROR_MMC_OPEN:
        show_mmc_cover_open_note();
        break;
    default:
        osso_log(LOG_ERR, 
                 "image_loader_callback() child_exit_status %d NOT_HANDLED",
                 child_exit_status);
        break;
    }
    image_loader_pid = -1;
    if(plugin_exists[HILDON_HOME_APPLET_WEB_SHORTCUT] &&
       applet_info[HILDON_HOME_APPLET_WEB_SHORTCUT].status)
    {
        hildon_home_plugin_applet_foreground(
            plugin[HILDON_HOME_APPLET_WEB_SHORTCUT]);
    }
}

/**
 * @show_loading_image_note
 *
 * Shows cancel note during background image loading
 *
 */
static
void show_loading_image_note()
{
    GtkWidget *label;
    GtkIconTheme *theme;
    GtkIconInfo *info;
    GtkWidget *animation = NULL;

    if(loading_image_note != NULL && GTK_IS_WIDGET(loading_image_note))
    {
        gtk_widget_destroy(loading_image_note);
    }
    loading_image_note = NULL;
    if(loading_image_process_active == FALSE)
    {
        return;
    }

    loading_image_note =
        gtk_dialog_new_with_buttons(
            "",
            GTK_WINDOW(window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            HILDON_HOME_LOADING_IMAGE_BUTTON,
            GTK_RESPONSE_CANCEL,
            NULL);
    
    label = gtk_label_new(HILDON_HOME_LOADING_IMAGE_TEXT);
    
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(loading_image_note)->vbox),
                      label);
    
    theme = gtk_icon_theme_get_default();
    info = gtk_icon_theme_lookup_icon(theme, HILDON_HOME_LOADING_IMAGE_ANI,
                                      HILDON_ICON_SIZE_NOTE, 0);
    if (info)
    {
        const gchar *filename = gtk_icon_info_get_filename(info);
        animation = gtk_image_new_from_file(filename);
        gtk_icon_info_free(info);
    }
    if(animation)
    {
        gtk_container_add(GTK_CONTAINER(GTK_DIALOG(loading_image_note)->vbox),
                          animation);
    }
    gtk_dialog_set_has_separator(GTK_DIALOG(loading_image_note), FALSE);
    gtk_window_set_modal(GTK_WINDOW(loading_image_note), TRUE);
    
    g_signal_connect(G_OBJECT(loading_image_note), "response",
                     G_CALLBACK(loading_image_note_handler), NULL);

    gtk_widget_realize (GTK_WIDGET (loading_image_note));
    gdk_window_set_decorations(loading_image_note->window,
                               GDK_DECOR_BORDER);
    
    gtk_widget_show_all (GTK_WIDGET (loading_image_note));
}


/**
 * @loading_image_note_handler
 *
 * @param loading_image_note
 * @param event
 * @param user_data
 *
 * Handles the cancel signal from note
 */
static 
gboolean loading_image_note_handler(GtkWidget *loading_cancel_note,
                                    GdkEvent *event, gpointer user_data)
{
     loading_image_process_active = FALSE;
     if(image_loader_pid != -1)
     {
    
         g_spawn_close_pid(image_loader_pid);
         kill(image_loader_pid, SIGTERM);
     }
     
     if(loading_image_note != NULL && 
        GTK_IS_WIDGET(loading_image_note))
     {
         gtk_widget_destroy(loading_image_note);
     }
     loading_image_note = NULL;
     return TRUE;
}

/**
 * @construct_background_image_with_uri
 * 
 * @uri: uri to location of image file
 * @new: TRUE if user selected. FALSE when loading system default.
 * 
 * Generates argument list for image loader to change original
 * background image from background. If construction was successfull,
 * saves uri it to designstated place for user background image.
 */
static
void construct_background_image_with_uri(const gchar *uri, gboolean new)
{
    char *width = g_strdup_printf("%d", HILDON_HOME_AREA_WIDTH);
    char *height = g_strdup_printf("%d", HILDON_HOME_AREA_HEIGHT);
    char *titlebar_image = get_titlebar_image_to_blend();
    char *sidebar_image = get_sidebar_image_to_blend();
    char *argument_list[] = {
        HILDON_HOME_IMAGE_LOADER_PATH, 
        HILDON_HOME_IMAGE_LOADER_IMAGE,
        (char *)home_current_bg_image, 
	width,
	height,
        (char *)uri, 
        (char *)user_original_bg_image,
	titlebar_image,
        (char *)titlebar_original_image_savefile,
        HILDON_HOME_TITLEBAR_LEFT_X,
        HILDON_HOME_TITLEBAR_TOP_Y,
        sidebar_image,
        (char *)sidebar_original_image_savefile,
        HILDON_HOME_SIDEBAR_LEFT_X,
        HILDON_HOME_SIDEBAR_TOP_Y,
        NULL };

    construct_background_image(argument_list, new);

    g_free (width);
    g_free (height);
    g_free (titlebar_image);
    g_free (sidebar_image);
}

/**
 * @construct_background_image_with_new_skin
 * 
 * @param titlebar
 * @param old_style
 * @param user_data
 *
 * Generates argument list for image loader to change skin elements
 * from background. 
 */
static
void construct_background_image_with_new_skin(GtkWidget *widget, 
                                              GtkStyle *old_style,
                                              gpointer user_data)
{
    gchar *width = g_strdup_printf("%d", HILDON_HOME_AREA_WIDTH);
    gchar *height = g_strdup_printf("%d", HILDON_HOME_AREA_HEIGHT);
    gchar *titlebar_image = get_titlebar_image_to_blend();
    gchar *sidebar_image = get_sidebar_image_to_blend();
    char *argument_list[] = {
        HILDON_HOME_IMAGE_LOADER_PATH, 
        HILDON_HOME_IMAGE_LOADER_SKIN,
        (char *)home_current_bg_image, 
	width,
	height,
	titlebar_image,
        (char *)titlebar_original_image_savefile,
        HILDON_HOME_TITLEBAR_LEFT_X,
        HILDON_HOME_TITLEBAR_TOP_Y,
	sidebar_image,
        (char *)sidebar_original_image_savefile,
        HILDON_HOME_SIDEBAR_LEFT_X,
        HILDON_HOME_SIDEBAR_TOP_Y,
        NULL };

    construct_background_image(argument_list, FALSE);

    g_free (width);
    g_free (height);
    g_free (titlebar_image);
    g_free (sidebar_image);
}


static
char *get_sidebar_image_to_blend()
{
    GtkStyle *style;
    gchar *image_name = NULL;
    
    style = gtk_rc_get_style_by_paths(gtk_widget_get_settings(window),
                                      HILDON_HOME_BLEND_IMAGE_SIDEBAR_NAME,
                                      NULL,
                                      G_TYPE_NONE);

    if(style != NULL && style->rc_style->bg_pixmap_name[0] != NULL)
    {
        image_name = g_strdup(style->rc_style->bg_pixmap_name[0]);
        g_object_ref(style);
    }
    return image_name;
}

/**
 * @get_titlebar_image_to_blend
 * 
 * @return path to temporary file of blendable skin image
 *
 * Gets pixbuf image from theme and saves it to temporary file.
 */

static
char *get_titlebar_image_to_blend()
{
    GtkStyle *style;
    char *image_name = NULL;
    
    style = gtk_rc_get_style_by_paths(gtk_widget_get_settings(window),
                                      HILDON_HOME_BLEND_IMAGE_TITLEBAR_NAME,
                                      NULL,
                                      G_TYPE_NONE);

    if(style != NULL && style->rc_style->bg_pixmap_name[0] != NULL)
    {
        image_name = g_strdup(style->rc_style->bg_pixmap_name[0]);
        g_object_ref(style);
    }
    return image_name;
}
 
/**
 * @get_background_image
 * 
 * @return background image in pixbuf format
 *
 * Gets user's saved background image in pixbuf.
 */
 
static 
GdkPixbuf *get_background_image()
{
    GdkPixbuf *pixbuf_bg;
    GError *error = NULL;

    pixbuf_bg = 
        gdk_pixbuf_new_from_file(home_current_bg_image, &error);
    
    if (error != NULL)
    {
        osso_log(LOG_ERR, "get_background_image():%s", error->message);
        g_error_free(error);
        return NULL;
    }

    return pixbuf_bg;
}

/**
 * @get_factory_default_background_image
 * 
 * @returns factory default background image pixbuf
 * 
 * Reads image from place designstated by factory configure. If cannot be
 * found, returns NULL.
 */

static
GdkPixbuf *get_factory_default_background_image()
{
    GdkPixbuf *pixbuf_bg;
    GError *error = NULL;
    struct stat buf;     
    
    if(home_factory_bg_image == NULL ||          
       stat(home_factory_bg_image, &buf) == -1)          
    {    
        return NULL;     
    }
    
    pixbuf_bg = 
        gdk_pixbuf_new_from_file(home_factory_bg_image, &error);
    
    if (error != NULL)
    {
        osso_log(LOG_ERR, "get_factory_default_background_image():%s", 
                 error->message);
        g_error_free(error);
        error = NULL;
        return NULL;
    }
    return pixbuf_bg;
}

/**
 * @set_default_background_image
 * 
 * Set default theme background image 
 */

static
void set_default_background_image()
{
    GDir *bg_image_desc_base_dir;
    GError *error = NULL;
    const gchar *image_desc_file;
    GtkTreeModel *combobox_contents;
    MBDotDesktop *file_contents;

    gchar *image_path = NULL;
    gint priority = -1; 
    gchar *image_file;
    struct stat buf;

    bg_image_desc_base_dir = g_dir_open(
         HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR, 0, &error);

    if (bg_image_desc_base_dir == NULL) 
    {
        osso_log(LOG_ERR, "Failed to open path: %s", error->message);
        if(error != NULL)
        {
            g_error_free(error);
        }
        return;
    } else 
    {
        image_desc_file = g_dir_read_name(bg_image_desc_base_dir);        
    }
    
    combobox_contents = 
        GTK_TREE_MODEL (
            gtk_list_store_new (
                3,               /* number of parameters */
                G_TYPE_STRING,   /* image localised descriptive name */
                G_TYPE_STRING,   /* image file path &name */
                G_TYPE_INT       /* Image priority */
                )
            );
    
    while (image_desc_file != NULL) 
    {
        if (g_str_has_suffix(image_desc_file, BG_IMG_INFO_FILE_TYPE))
        {

            gchar *filename;
            filename = g_build_filename(
                HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR,
                image_desc_file, NULL);
            file_contents = mb_dotdesktop_new_from_file(filename);
            g_free(filename);
            
            if (file_contents != NULL) 
            {
                gchar *priority_temp;
                
                priority_temp = 
                    mb_dotdesktop_get(file_contents,
                                      BG_DESKTOP_IMAGE_PRIORITY);

                if(priority_temp != NULL && g_ascii_isdigit(*priority_temp) &&
                   (priority == -1 || (gint)atoi(priority_temp) < priority))
                {
                    priority = (gint)atoi(priority_temp);
                    
                    image_path = g_strdup(
                        mb_dotdesktop_get(file_contents,
                                          BG_DESKTOP_IMAGE_FILENAME));
                } else 
                {
                    osso_log(LOG_ERR, "Desktop file malformed: %s", 
                             image_desc_file);
                }
                
                mb_dotdesktop_free(file_contents);
                
            }            

        }
        
        image_desc_file = g_dir_read_name(bg_image_desc_base_dir);
    } /*while*/

    if (bg_image_desc_base_dir != NULL) 
    {
        g_dir_close(bg_image_desc_base_dir);        
        
    }

    if (image_path) 
    {
        construct_background_image_with_uri(image_path, FALSE);
        g_free(image_path);

        /* remove old files if nesessary and create symlinks */
        if(stat(home_current_bg_image, &buf) != -1)
        {
            unlink(home_current_bg_image);
        }
        
        image_file = g_build_path("/", HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR,
                                  HILDON_HOME_BG_USER_FILENAME, NULL);
        
        symlink(image_file, home_current_bg_image);
        g_free(image_file);
        
        
        if(stat(titlebar_original_image_savefile, &buf) != -1)
        {
            unlink(titlebar_original_image_savefile);
        }
        
        image_file = g_build_path("/", HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR,
                                  HILDON_HOME_ORIGINAL_IMAGE_TITLEBAR, NULL);
        
        symlink(image_file, titlebar_original_image_savefile);
        g_free(image_file);
        
        
        if(stat(sidebar_original_image_savefile, &buf) != -1)
        {
            unlink(sidebar_original_image_savefile);
        }
        
        image_file = g_build_path("/", HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR,
                                  HILDON_HOME_ORIGINAL_IMAGE_SIDEBAR, NULL);
        
        symlink(image_file, sidebar_original_image_savefile);
        g_free(image_file);
        
        if(stat(user_original_bg_image, &buf) != -1)          
        {    
            unlink(user_original_bg_image);
        }

        image_file = g_build_path("/", HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR, 
                                  HILDON_HOME_CONF_USER_ORIGINAL_FILENAME,
                                  NULL);

        symlink(image_file, user_original_bg_image);
        g_free(image_file);

        home_bg_image_symlinked = TRUE;
    }
}

/**
 * @refresh_background_image
 * 
 * Tries to get current background image first. If not found tries system 
 * default background image. If found but wrong sized, image is cropped 
 * to right size.
 */

static 
void refresh_background_image()
{
    GdkPixbuf *pixbuf_bg = NULL;
    GdkPixmap *pixmap_bg;
    GdkBitmap *bit_mask;
    struct stat buf;

    /* if not save picture, then go to default */
    if (stat(home_current_bg_image, &buf) != -1) 
    {    
        pixbuf_bg = get_background_image();
    }

    if (pixbuf_bg == NULL)
    {    
        pixbuf_bg = get_factory_default_background_image();

        if (pixbuf_bg == NULL)
        {
            set_default_background_image();
            if (stat(home_current_bg_image, &buf) != -1) 
            {    
                pixbuf_bg = get_background_image();
            }
        }
    }

    if (pixbuf_bg != NULL)
    {
        GdkColormap *colormap = NULL;
        
        gtk_widget_set_app_paintable (GTK_WIDGET(home_base_eventbox),
                                      TRUE);
        gtk_widget_realize(GTK_WIDGET( home_base_eventbox));

        colormap = gdk_drawable_get_colormap(
            GDK_DRAWABLE(GTK_WIDGET(home_base_eventbox)->window));
        
        g_assert(colormap);
        
        gdk_pixbuf_render_pixmap_and_mask_for_colormap(pixbuf_bg, 
                                                       colormap,
                                                       &pixmap_bg, 
                                                       &bit_mask, 0);

        g_object_unref(pixbuf_bg);

        if (pixmap_bg != NULL) 
        {
            gdk_window_set_back_pixmap(
                GTK_WIDGET(home_base_eventbox)->window, 
                pixmap_bg, FALSE);
        }
        if (bit_mask != NULL) 
        {
            g_object_unref(bit_mask);
        }

    } else
    {
        osso_log(LOG_ERR, 
                 "refresh_background_image(): failed to retrieve any image");
    }
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(home_base_eventbox),
                                     TRUE); 

    gtk_widget_queue_draw(home_base_eventbox);
    if(pixmap_bg != NULL) 
    {
        g_object_unref(pixmap_bg);
    }
}

/**
 * @create_applet
 *
 * @param plugin pointer to plugin loader 
 * @param plugin_name name of plugin
 * @param statesize size of state save data
 * @param statedata state save data
 * @param applet_num applets number in home
 *
 * @returns A new applet with child applet
 *
 * creates applet area widget
 */
static 
GtkWidget *create_applet(HildonHomePluginLoader **plugin,
                         gchar *plugin_name, gint statesize,
                         void *statedata, gint applet_num)
{
    GtkWidget *applet = NULL;
    GtkWidget *parent_applet = NULL;

    *plugin = hildon_home_plugin_loader_new(plugin_name,
                                            statedata, &statesize, 
                                            &applet);

    if(applet != NULL) 
    {
        if (applet_num < NUM_STATIC_APPLETS)
        {
            parent_applet = hildon_home_applet_new();
            gtk_container_add(GTK_CONTAINER(parent_applet), applet);
        }
        plugin_exists[applet_num] = TRUE;
    } else 
    {
        plugin_exists[applet_num] = FALSE;
    }

    if (parent_applet)
        return parent_applet;
    else
        return applet;
}

/**
 * @set_applet_width_and_position
 *
 * @param applet_num number of applet to added
 *
 * Sets size and position of the applet
 */
static
void set_applet_width_and_position(gint applet_num)
{
    gtk_widget_set_size_request(
        GTK_WIDGET(applet_area[applet_num]),
        applet_info[applet_num].width,
        applet_info[applet_num].height);
    
    gtk_fixed_put(GTK_FIXED(home_fixed),
                  GTK_WIDGET(applet_area[applet_num]),
                  applet_info[applet_num].x,
                  applet_info[applet_num].y);
    gtk_widget_show_all(applet_area[applet_num]);
    gtk_container_set_border_width(GTK_CONTAINER(applet_area[applet_num]),
                                   HILDON_HOME_APPLET_BORDER_WIDTH);
    hildon_home_plugin_applet_foreground(plugin[applet_num]);
}

/**
 * @construct_applets
 *
 * Constructs applets, which are visible except webshortcut is needed always
 */

static
void construct_applets()
{

    gint applet_num, fd;
    
    fd = osso_state_open_read(osso_home);

    for(applet_num= 0;applet_num<NUM_STATIC_APPLETS;applet_num++)
    {
        gint statesize = 0;
        void *statedata = NULL;

        if(!applet_info[applet_num].status && 
            applet_num != HILDON_HOME_APPLET_WEB_SHORTCUT)
        {
            applet_area[applet_num] = NULL;
            continue;
        }

        if (fd != -1)
        {
            read(fd, &statesize, sizeof(int));
            if (statesize > 0)
            {
                if ((statedata = g_malloc0(statesize*sizeof(void))) 
                    == NULL)
                {
                    continue;
                }
                read(fd, statedata, statesize);
            }
        }
        applet_area[applet_num] = 
            create_applet(&plugin[applet_num], 
                          applet_info[applet_num].plugin_name,
                          statesize, statedata,
                          applet_num);

        g_free(statedata);

        if(plugin_exists[applet_num]) 
        {
            set_applet_width_and_position(applet_num);
        }
        if (applet_num == HILDON_HOME_APPLET_WEB_SHORTCUT &&
            shortcut_properties_editable &&
            shortcut_properties != NULL) 
        {
            gtk_widget_set_sensitive(GTK_WIDGET(shortcut_properties), 
                                     applet_info[applet_num].status);
        }
    }

    g_object_set(G_OBJECT(applet_area[HILDON_HOME_APPLET_WEB_SHORTCUT]), 
                 "visible", 
                 applet_info[HILDON_HOME_APPLET_WEB_SHORTCUT].status, NULL);
    osso_state_close(osso_home, fd);
}

/**
 * @construct_user_applets
 *
 * Constructs third party applets
 */
static
void construct_user_applets(void)
{
    gchar *pattern;
    glob_t globbuf;
    FILE *fp;
    gint applet_num;
    AppletInfo *info;
    GtkWidget *applet;
    char buf[512];
    gchar **tokens;
    int i;
    
    pattern = g_strconcat(HILDON_HOME_USER_PLUGIN_PATH, "*conf", NULL);
    
    glob(pattern, 0, NULL, &globbuf);
        
    for (i = 0; i < globbuf.gl_pathc && num_applets < MAX_APPLETS; i++)
    {
        applet_num = num_applets;

        info = &applet_info[applet_num];
        info->label = malloc(HILDON_HOME_PATH_STR_LENGTH);
        info->plugin_name = malloc(HILDON_HOME_PATH_STR_LENGTH);
	
        fp = fopen(globbuf.gl_pathv[i], "r");
        if (fp != NULL)
        {
            /* Extract the applet name */
            if (fgets(buf, sizeof(buf), fp))
            {
                tokens = g_strsplit(buf, "=", 2);
                info->label = g_strdup(g_strstrip(tokens[1]));
                g_strfreev(tokens);
            }
            /* Exract the rest of the applet parameters */
            if (fscanf(fp,
                       HILDON_HOME_USER_PLUGIN_CONF_FORMAT,
                       info->plugin_name,
                       &info->width,
                       &info->height,
                       &info->x,
                       &info->y)
                < 0) {
                 osso_log(LOG_ERR, "Couldn't load plugin conf file %s\n",
                          globbuf.gl_pathv[i]);
            } else
            {
                if (user_applets && strstr(user_applets, info->plugin_name))
                    info->status = TRUE;
                else
                    info->status = FALSE;

                num_applets++;

                menu_item[applet_num] = 
                    gtk_check_menu_item_new_with_label(
                        info->label);
                gtk_widget_show(menu_item[applet_num]);
                gtk_menu_insert(GTK_MENU(titlebar_menu), 
                                menu_item[applet_num], applet_num);
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM (menu_item[applet_num]),
                                               info->status);
                g_signal_connect(G_OBJECT(menu_item[applet_num]), 
                                 "toggled",
                                 G_CALLBACK(toggle_applet_visibility), 
                                 (gint *)applet_num);

                if (info->status)
                {
                    applet = create_applet(&plugin[applet_num], 
                                           info->plugin_name,
                                           0, NULL,
                                           applet_num);
                    if (applet)
                    {
                        applet_area[applet_num] = applet;
                        set_applet_width_and_position(applet_num);
                    }
                }
            }
            /* Added checking of NULL to prevent crashes Karoliina Salminen 13092005 */
            if(fp!=NULL){
                fclose(fp);
            } 
            /* End of fix 13092005 */
        }
    }

    g_free(pattern);
}

/* ----------------------*/
/* applet area ends here */
/* ----------------------*/

/**
 * @construct_home_area
 *
 * Constructs home background.
 */

static
void construct_home_area()
{
    home_base_fixed = gtk_fixed_new();
    gtk_container_add( GTK_CONTAINER( window ), home_base_fixed );

    home_base_eventbox = gtk_event_box_new();
    gtk_widget_set_size_request( GTK_WIDGET( home_base_eventbox ),
                                 HILDON_HOME_WIDTH,
                                 HILDON_HOME_HEIGHT);

    gtk_fixed_put( GTK_FIXED( home_base_fixed ),
                   GTK_WIDGET( home_base_eventbox ),
                   HILDON_HOME_X, HILDON_HOME_Y );

    refresh_background_image();

    home_fixed = gtk_fixed_new();
    gtk_container_add( GTK_CONTAINER(home_base_eventbox), home_fixed );

    home_area_eventbox = gtk_event_box_new();
    gtk_widget_set_size_request( GTK_WIDGET( home_area_eventbox ),
                                 HILDON_HOME_AREA_WIDTH,
                                 HILDON_HOME_AREA_HEIGHT);

    gtk_event_box_set_visible_window(GTK_EVENT_BOX(home_area_eventbox),
                                     FALSE);

    gtk_fixed_put( GTK_FIXED( home_fixed ),
                   GTK_WIDGET( home_area_eventbox ),
                   HILDON_HOME_AREA_X, HILDON_HOME_AREA_Y );

}


/**
 * @hildon_home_display_base
 * 
 * Displays background and titlebar
 */
static 
void hildon_home_display_base(void)
{
    gtk_widget_realize( GTK_WIDGET( window ) );
    gdk_window_set_type_hint( GTK_WIDGET( window )->window, 
                              GDK_WINDOW_TYPE_HINT_DESKTOP );
    gtk_widget_show_all( GTK_WIDGET(window) );
}


/**
 * @hildon_home_initiliaze
 * 
 * initialize home variables
 */

static 
void hildon_home_initiliaze()
{
    FILE *fp;
    gchar *configure_file;
    gchar bg_orig_filename[HILDON_HOME_PATH_STR_LENGTH] = {0};
    gchar image_dir[HILDON_HOME_PATH_STR_LENGTH] = {0};
    gint applet_num;
    gboolean applet_statusinfo[MAX_APPLETS_HC];
    char buf[2048];
    gchar **tokens;

    hildon_home_get_enviroment_variables();
    hildon_home_set_hardcode_values();
    hildon_home_get_factory_settings();

    /* If the lock file exists, we probably crashed during startup */
    if (startup_lock_exists())
    {
        hildon_home_create_configure(); /* Disable all applets */
    } else
    {
        create_startup_lock();
    }

    titlebar_original_image_savefile = 
        g_build_path("/", home_user_dir, HILDON_HOME_SYSTEM_DIR,
                     HILDON_HOME_ORIGINAL_IMAGE_TITLEBAR, NULL);


    sidebar_original_image_savefile = 
        g_build_path("/", home_user_dir, HILDON_HOME_SYSTEM_DIR,
                     HILDON_HOME_ORIGINAL_IMAGE_SIDEBAR, NULL);

    home_current_bg_image = 
        g_build_path("/", home_user_dir, HILDON_HOME_SYSTEM_DIR,
                     HILDON_HOME_BG_USER_FILENAME, NULL);

    user_original_bg_image =
        g_build_path("/", home_user_dir, HILDON_HOME_SYSTEM_DIR,
                     HILDON_HOME_CONF_USER_ORIGINAL_FILENAME, NULL);

    hildon_home_construct_user_system_dir();

    configure_file = 
        g_build_path("/", home_user_dir, HILDON_HOME_SYSTEM_DIR,
                     HILDON_HOME_CONF_USER_FILENAME, NULL);

    fp = fopen (configure_file, "r");

    if(fp == NULL) 
    {
        osso_log(LOG_ERR, "Couldn't open configure file %s", 
                 configure_file);
    } else 
    {
        if(fscanf(fp, 
                  HILDON_HOME_CONF_USER_FORMAT_SAVE,
                  bg_orig_filename, image_dir, 
                  &applet_statusinfo[0],
                  &applet_statusinfo[1],
                  &applet_statusinfo[2],
                  &applet_statusinfo[3]) 
           < 0)
        {
            osso_log(LOG_ERR, 
                     "Couldn't load statusinfo from configure file %s", 
                     user_original_bg_image);            
        }
        if (fgets(buf, sizeof(buf), fp))
        {
            tokens = g_strsplit(buf, ":", 2);
            if (tokens)
            {
                if (tokens[1])
                    user_applets = g_strdup(g_strstrip(tokens[1]));
                else
                    user_applets = "";

                g_strfreev(tokens);
            }
        }
        
        for(applet_num= 0;applet_num<NUM_STATIC_APPLETS;applet_num++)
        {
            applet_info[applet_num].status =
                applet_statusinfo[applet_num];
        }
    }
    if(fp != NULL && fclose(fp) != 0) 
    {
        osso_log(LOG_ERR, "Couldn't close configure file %s", 
                 configure_file);
    }

    if(bg_orig_filename != NULL && user_original_bg_image == NULL) 
    {
        user_original_bg_image = 
            g_build_path("/", home_user_dir, HILDON_HOME_SYSTEM_DIR, 
                         bg_orig_filename, NULL);
    }

    if(image_dir != NULL) 
    {
        home_user_image_dir = 
            g_build_path("/", home_user_dir, image_dir, NULL);
    } else
    {
        home_user_image_dir = 
            g_build_path("/", home_user_dir, 
                         HILDON_HOME_HC_USER_IMAGE_DIR, NULL);
    }

    g_free(configure_file);

    hildon_home_cp_read_desktop_entries();
}

/**
 * @hildon_home_set_hardcode_values();
 * 
 * Sets hard code values for applets
 */

static
void hildon_home_set_hardcode_values( void)
{
    gint applet_num;
    gchar *label;
    gchar *plugin_name;
    gint width;
    gint height;
    gint x;
    gint y;

    for(applet_num= 0;applet_num<NUM_STATIC_APPLETS;applet_num++)
    {
        switch(applet_num) {
        case 0:
            label =       HILDON_HOME_APPLET_0_LABEL;
            plugin_name = HILDON_HOME_PLUGIN_0_NAME;
            width =       HILDON_HOME_APPLET_0_WIDTH;
            height =      HILDON_HOME_APPLET_0_HEIGHT;
            x =           HILDON_HOME_APPLET_0_X;
            y =           HILDON_HOME_APPLET_0_Y;
            break;
        case 1:
            label =       HILDON_HOME_APPLET_1_LABEL;
            plugin_name = HILDON_HOME_PLUGIN_1_NAME;
            width =       HILDON_HOME_APPLET_1_WIDTH;
            height =      HILDON_HOME_APPLET_1_HEIGHT;
            x =           HILDON_HOME_APPLET_1_X;
            y =           HILDON_HOME_APPLET_1_Y;
            break;
        case 2:
            label =       HILDON_HOME_APPLET_2_LABEL;
            plugin_name = HILDON_HOME_PLUGIN_2_NAME;
            width =       HILDON_HOME_APPLET_2_WIDTH;
            height =      HILDON_HOME_APPLET_2_HEIGHT;
            x =           HILDON_HOME_APPLET_2_X;
            y =           HILDON_HOME_APPLET_2_Y;
            break;
        case 3:
            label =       HILDON_HOME_APPLET_3_LABEL;
            plugin_name = HILDON_HOME_PLUGIN_3_NAME;
            width =       HILDON_HOME_APPLET_3_WIDTH;
            height =      HILDON_HOME_APPLET_3_HEIGHT;
            x =           HILDON_HOME_APPLET_3_X;
            y =           HILDON_HOME_APPLET_3_Y;
            break;
        default:
            label = "";
            plugin_name = "";
            width = 0;
            height = 0;
            x = 0;
            y = 0;
            break;
        }
        applet_info[applet_num].label = label;
        applet_info[applet_num].plugin_name = plugin_name;
        applet_info[applet_num].width = width;
        applet_info[applet_num].height = height;
        applet_info[applet_num].x = x;
        applet_info[applet_num].y = y;
        applet_info[applet_num].status = HILDON_HOME_CONF_USER_APPLET_STATUS_DEFAULT;
    }
}

/**
 * @hildon_home_get_factory_settings
 * 
 * Gets factory settings
 */

static
void hildon_home_get_factory_settings( void )
{
    FILE *fp;

    gint ws_hide = 1;
    gint ws_properties = 1;
    gint bg_change = 1;
    gchar bg_fact_filename[HILDON_HOME_PATH_STR_LENGTH] = {0};

    fp = fopen (HILDON_HOME_FACTORY_FILENAME, "r");
    if(fp == NULL) 
    {
        osso_log(LOG_ERR, "Couldn't open file %s for reading image name", 
                 user_original_bg_image);
    } else
    {
        if(fscanf(fp, HILDON_HOME_FACTORY_FORMAT, 
                  &ws_hide, &ws_properties, &bg_change, bg_fact_filename)
           < 0) 
        {
            osso_log(LOG_ERR, 
                     "Couldn't load factory settings from file %s", 
                     user_original_bg_image);
        }
        if(fp != NULL && fclose(fp) != 0) 
        {
            osso_log(LOG_ERR, "Couldn't close file %s", 
                     user_original_bg_image);
        }
    }
    if(HILDON_HOME_APPLET_WEB_SHORTCUT < NUM_STATIC_APPLETS) 
    {
        shortcut_applet_hideable = ws_hide;
        shortcut_properties_editable = ws_properties;
    } else 
    {
        shortcut_applet_hideable = FALSE;
        shortcut_properties_editable = FALSE;
    }
    background_changable = bg_change;
    if(bg_fact_filename != NULL) 
    {
        home_factory_bg_image = g_strdup_printf("%s", bg_fact_filename);
    }
    return;
}

/**
 * @hildon_home_get_enviroment_variables
 * 
 * Gets and sets user's home directory from $ENV location
 */

static
void hildon_home_get_enviroment_variables( void )
{
    home_user_dir = g_getenv( HILDON_HOME_ENV_HOME );
}

/**
 * @hildon_home_construct_user_system_dir
 * 
 * constructs user's system directory
 */

static 
void hildon_home_construct_user_system_dir()
{
    gchar *image_file;
    gchar *system_dir;
    gboolean nonexistent = TRUE;
    struct stat buf;

    system_dir = 
        g_build_path("/", home_user_dir, HILDON_HOME_SYSTEM_DIR, NULL);

    if (stat(system_dir, &buf) == 0) 
    {
        if(S_ISDIR(buf.st_mode))
        {
            nonexistent = FALSE;
        }
    }

    if(nonexistent)
    {
        if(mkdir (system_dir, HILDON_HOME_SYSTEM_DIR_ACCESS) != 0) 
        {
            osso_log(LOG_ERR, "Couldn't creat directory %s", system_dir);
        }
        
        hildon_home_create_configure();
    }

    /* create symlinks if files are missing */
    if(stat(home_current_bg_image, &buf) == -1)          
    {    
        image_file = g_build_path("/", HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR, 
                                  HILDON_HOME_BG_USER_FILENAME, NULL);

        symlink(image_file, home_current_bg_image);
        g_free(image_file);
    }
    
    if(stat(titlebar_original_image_savefile, &buf) == -1)          
    {    
        image_file = g_build_path("/", HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR, 
                                  HILDON_HOME_ORIGINAL_IMAGE_TITLEBAR, NULL);
        
        symlink(image_file, titlebar_original_image_savefile);
        g_free(image_file);
    }
    
    if(stat(sidebar_original_image_savefile, &buf) == -1)          
    {    
        image_file = g_build_path("/", HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR, 
                                  HILDON_HOME_ORIGINAL_IMAGE_SIDEBAR, NULL);

        symlink(image_file, sidebar_original_image_savefile);
        g_free(image_file);
    }    

    if(stat(user_original_bg_image, &buf) == -1)          
    {    
        image_file = g_build_path("/", HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR, 
                                  HILDON_HOME_CONF_USER_ORIGINAL_FILENAME,
                                  NULL);

        symlink(image_file, user_original_bg_image);
        g_free(image_file);
        home_bg_image_symlinked = TRUE;
    }    

    g_free(system_dir);
}

/**
 * @hildon_home_create_configure
 * 
 * create user configure files using hard coded default values
 */

static 
void hildon_home_create_configure()
{
    FILE *fp;
    gchar *configure_file;
    gboolean applet_statusinfo[MAX_APPLETS_HC];
    gint applet_num;

    configure_file = 
        g_build_path("/", home_user_dir, HILDON_HOME_SYSTEM_DIR,
                     HILDON_HOME_CONF_USER_FILENAME, NULL);
    
    fp = fopen (configure_file, "w");
    if(fp == NULL) 
    {
        osso_log(LOG_ERR, "Couldn't open configure file %s", 
                 configure_file);
    } else 
    {
        for(applet_num=0;applet_num<MAX_APPLETS_HC;applet_num++)
        {
            applet_statusinfo[applet_num] = 
                HILDON_HOME_CONF_USER_APPLET_STATUS_DEFAULT;
        }
        if(fprintf(fp, 
                   HILDON_HOME_CONF_USER_FORMAT,
                   HILDON_HOME_CONF_USER_ORIGINAL_FILENAME,
                   HILDON_HOME_CONF_USER_IMAGE_DIR,
                   applet_statusinfo[0], 
                   applet_statusinfo[1], 
                   applet_statusinfo[2], 
                   applet_statusinfo[3],
                   user_applets)
           < 0)
        {
            osso_log(LOG_ERR, 
                     "Couldn't write default values to configure file %s", 
                     configure_file);
        }
    }
    if(fp != NULL && fclose(fp) != 0) 
    {
        osso_log(LOG_ERR, "Couldn't close file %s", 
                 configure_file);
    }
    
    g_free(configure_file);
}


/**
 * @hildon_home_save_configure
 * 
 * saves user applet statues to configure file
 */

static 
void hildon_home_save_configure()
{
    FILE *fp;
    gchar *configure_file;
    gint applet_num;
    gboolean applet_statusinfo[MAX_APPLETS_HC];

    configure_file = 
        g_build_path("/", home_user_dir, HILDON_HOME_SYSTEM_DIR,
                     HILDON_HOME_CONF_USER_FILENAME, NULL);
        
    fp = fopen (configure_file, "w");
    if(fp == NULL) 
    {
        osso_log(LOG_ERR, "Couldn't open configure file %s", 
                 configure_file);
    } else 
    {
        for(applet_num= 0;applet_num<MAX_APPLETS_HC;applet_num++)
        {
            if(applet_num < NUM_STATIC_APPLETS && plugin_exists[applet_num])
            {
                applet_statusinfo[applet_num] = 
                    applet_info[applet_num].status;
                    
            } else
            {
                applet_statusinfo[applet_num] = FALSE;
            }
        }
        if(fprintf(fp, 
                   HILDON_HOME_CONF_USER_FORMAT,
                   HILDON_HOME_CONF_USER_ORIGINAL_FILENAME,
                   HILDON_HOME_CONF_USER_IMAGE_DIR,
                   applet_statusinfo[0], 
                   applet_statusinfo[1], 
                   applet_statusinfo[2], 
                   applet_statusinfo[3],
                   user_applets)
           < 0)
        {
           osso_log(LOG_ERR, 
                     "Couldn't write values to configure file %s", 
                     configure_file);
        }
        

    }
    if(fp != NULL && fclose(fp) != 0) 
    {
        osso_log(LOG_ERR, "Couldn't close file %s", configure_file);
    }
    
    g_free(configure_file);

}

/**
 * hildon_home_cp_read_desktop_entry:
 * @dir_path: path to directory where dotdesktop files are
 *
 * Reads desktop entries
 *
 **/

static void hildon_home_cp_read_desktop_entries(void)
{
    GDir *dir;
    GError * error = NULL;
    gchar *dir_path = CONTROLPANEL_ENTRY_DIR;
    gchar *path=NULL;
    
    g_return_if_fail(dir_path);
    dir = g_dir_open(dir_path, 0, &error);
    if(!dir)
    {
        osso_log( LOG_ERR, error->message);
        if(error != NULL)
        {
            g_error_free(error);
        }
        personalization = NULL;
        screen_calibration = NULL;
        
        return;
    }
    if(error != NULL)
    {
        g_error_free(error);
    }
    /* personalization plugin */
    path = g_strconcat(dir_path, G_DIR_SEPARATOR_S, 
                       HILDON_CP_PLUGIN_PERSONALISATION, NULL);

    if (path != NULL)
    {
        personalization = mb_dotdesktop_new_from_file(path);
        if(!personalization)
        {
            osso_log( LOG_WARNING, "Error reading entry file %s\n", path );
            personalization = NULL;
        }       
        g_free(path);
    } else 
    {
        personalization = NULL;
    }

    /* screen calibration plugin */
    path = g_strconcat(dir_path, G_DIR_SEPARATOR_S, 
                       HILDON_CP_PLUGIN_CALIBRATION, NULL);

    if (path != NULL)
    {
        screen_calibration = mb_dotdesktop_new_from_file(path);
        if(!screen_calibration)
        {
            osso_log( LOG_WARNING, "Error reading entry file %s\n", path );
            screen_calibration = NULL;
        }       
        g_free(path);
    } else 
    {
        screen_calibration = NULL;
    }

    g_dir_close(dir);
}

/**
 * @hildon_home_key_press_listener
 * 
 * @param widget
 * @param keyevent
 * @param data
 * 
 * @return 
 *
 * Handles the keyboard press events for the Home
 */

static
gint hildon_home_key_press_listener (GtkWidget * widget,
                              GdkEventKey * keyevent,
                              gpointer data)
{
    if (keyevent->keyval == HILDON_MENU_KEY)
    {
        if (!menu_key_pressed)
        {
            menu_key_pressed = TRUE;
            gtk_menu_popup(GTK_MENU(titlebar_menu), NULL, NULL,
                           (GtkMenuPositionFunc)
                           titlebar_menu_position_func,
                           NULL, 0, gtk_get_current_event_time ());
            gtk_menu_shell_select_first (GTK_MENU_SHELL(titlebar_menu), TRUE);

            /* we'll have to grab this widget so we can catch the key
               release signal, which is needed to prevent menu opening/closing
               repeatedly when menu key is being pressed for a long time */
            gtk_grab_add(widget);
        }
    }
    return FALSE;
}

/**
 * @hildon_home_key_release_listener
 * 
 * @param widget
 * @param keyevent
 * @param data
 * 
 * @return 
 *
 * Handles the keyboard release events for the Home
 */

static
gint hildon_home_key_release_listener (GtkWidget * widget,
                                       GdkEventKey * keyevent,
                                       gpointer data)
{
    if (keyevent->keyval == HILDON_MENU_KEY)
    {
        gtk_grab_remove(widget);
        menu_key_pressed = FALSE;
    }
    return FALSE;
}


/**
 * @set_focus_to_widget_cb
 * 
 * @param window
 * @param widget
 * @param user_data
 * 
 * Handles the keyboard events for the Home
 */

static void
set_focus_to_widget_cb(GtkWindow *window,
                       GtkWidget *widget,
                       gpointer user_data)
{
    GtkWidget *parent = NULL;
    gboolean applet_selected = FALSE;

    if (GTK_IS_WIDGET(widget))
    {
        parent = gtk_widget_get_parent(widget);
    }

    while (parent)
    {
        if (HILDON_IS_HOME_APPLET(parent))
        {
            gint applet_num;

            for(applet_num=0;applet_num<num_applets;applet_num++)
            {
                if(plugin_exists[applet_num]) 
                {                
                    if (parent == applet_area[applet_num])
                    {
                        hildon_home_applet_has_focus(
                            HILDON_HOME_APPLET(applet_area[applet_num]), 
                            TRUE);
                        applet_selected = TRUE;
                    } else
                    {
                        hildon_home_applet_has_focus(
                            HILDON_HOME_APPLET(applet_area[applet_num]), 
                            FALSE);
                    }
                }
            }

            if (applet_selected)
            {
                return;
            }
        }

        parent = gtk_widget_get_parent(parent);
    }

    /* None applet selected */
    if (!applet_selected)
    {
        gint applet_num;

        for(applet_num=0;applet_num<num_applets;applet_num++)
        {
            if(plugin_exists[applet_num]) 
            {
                hildon_home_applet_has_focus(
                    HILDON_HOME_APPLET(applet_area[applet_num]), FALSE);
            }
        }
    }
}

/**
 * @hildon_home_deinitiliaze
 * 
 * deinitialize home variables freeing memory
 */

static 
void hildon_home_deinitiliaze()
{
    gint applet_num;

    if(home_user_image_dir != NULL)
    {
        g_free((gchar *)home_user_image_dir);
    } 
    if(home_current_bg_image != NULL)
    {
        g_free((gchar *)home_current_bg_image);
    } 
    if(user_original_bg_image != NULL)
    {
        g_free((gchar *)user_original_bg_image);
    }
    if(home_factory_bg_image != NULL)
    {
        g_free((gchar *)home_factory_bg_image);
    }
    if(titlebar_original_image_savefile != NULL)
    {
        g_free((gchar *)titlebar_original_image_savefile);
    }
    if(sidebar_original_image_savefile != NULL)
    {
        g_free((gchar *)sidebar_original_image_savefile);
    }

    for(applet_num=0;applet_num<num_applets;applet_num++)
    {
        if(plugin_exists[applet_num])
        {
            hildon_home_plugin_deinitialize(plugin[applet_num]);
        }
    }
}
/**
 * @get_active_main_window
 *
 * @param window
 *
 * @returns actual main window
 *
 * Let's search a actual main window using tranciency hints.
 * Note that there can be several levels of menus/dialogs above
 * the actual main window. 
 */

static Window get_active_main_window(Window window)
{
    Window parent_window;
    gint limit = 0;
    
    gdk_error_trap_push();
    while (XGetTransientForHint(GDK_DISPLAY(), window, &parent_window))
    {
        /* The limit > TRANSIENCY_MAXITER ensures that we can't be stuck
           here forever if we have circular transiencies for some reason.
           Use of _MB_CURRENT_APP_WINDOW might be more elegant... */

        /* FIXME: You really *do* want to use _MB_CURRENT_APP_WINDOW  
         *        it could potentially avoid a numer of round trips.
         *        Its not just elegance but efficiency too.
        */
        if (!parent_window || parent_window == GDK_ROOT_WINDOW() ||
            parent_window == window || limit > TRANSIENCY_MAXITER)
        {
            break;
        }
        
        limit++;
        window = parent_window;
    }
    gdk_error_trap_pop();
    return window;
}
/**
 * @hildon_home_event_filter
 * 
 * @param xevent 
 * @param event
 * @param data
 * 
 * @returs GDK_FILTER_CONTINUE
 *
 * State saves when home is not topmost
 */


static 
GdkFilterReturn hildon_home_event_filter (GdkXEvent *xevent, 
                                          GdkEvent *event, 
                                          gpointer data)
{
    static Atom     active_window_atom = 0;
    XPropertyEvent *prop = NULL;
    
    /* If this isn't a property chnage event ignore ASAP */
    if (((XEvent*)xevent)->type != PropertyNotify)
        return GDK_FILTER_CONTINUE;

    if (active_window_atom == 0)
    {
        /* Only ever create the the atom _once_. It costs
         * a roundtrip to create an atom so you really dont
         * want to do this all the time. The atom value will
         * not change.
        */
        active_window_atom = XInternAtom (GDK_DISPLAY(),
                                          "_NET_ACTIVE_WINDOW",
                                          False);
    }
    
    prop = (XPropertyEvent*)xevent;

    /* Only process is the event is for root win and active atom */
    if (prop->window == GDK_ROOT_WINDOW () && prop->atom == active_window_atom)
    { 
        Atom realtype;
        gint applet_num = 0;
        gint error_trap = 0;
        int format;
        int status;
        unsigned long n;
        unsigned long extra;
        Window my_window;
        union
        {
            Window *win;
            unsigned char *char_pointer;
        } win;
        win.win = NULL;
        gdk_error_trap_push();
        status = XGetWindowProperty (GDK_DISPLAY (),
                                     GDK_ROOT_WINDOW (),
                                     active_window_atom, 0L, 16L,
                                     0, XA_WINDOW, &realtype,
                                     &format, &n,
                                     &extra,
                                     (unsigned char **)
                                     &win.char_pointer);
        gdk_error_trap_pop();
        if (!( status == Success && realtype == XA_WINDOW &&
               format == 32 && n == 1 && win.win != NULL &&
               error_trap == 0))
        {
            return GDK_FILTER_CONTINUE;
        }

        my_window = GDK_WINDOW_XID(window->window);
        if (win.win[0] != my_window && 
            get_active_main_window(win.win[0]) != my_window &&
            home_is_topmost == TRUE)
        {
            int fd = 0, retval = 0;
            gchar *statedata = NULL;
            gint statesize = 0;
            /* We're no longer topmost, so we will statesave
               the applet states */

            home_is_topmost = FALSE;
            fd = osso_state_open_write(osso_home);

            if (fd == -1)
            {
                return GDK_FILTER_CONTINUE;
            }
            for (applet_num = 0; applet_num < num_applets; applet_num++)
            {
                if (plugin_exists[applet_num])
                {
                    hildon_home_plugin_applet_background(plugin[applet_num]);
                    retval = 
                        hildon_home_plugin_applet_save_state(
                            plugin[applet_num], 
                            (void *)&statedata, &statesize);
                    if (retval != 1 || statesize == 0)
                    {
                        /* Indicate that this plugin has no saved state */
                        write(fd, '\0', 1);
                        continue;
                    }
                    write(fd, &statesize, sizeof(int));
                    write(fd, statedata, statesize);
                }
            }
            fsync(fd);
            osso_state_close(osso_home, fd);
            g_free(statedata);
        } else if (win.win[0] == my_window &&
                   home_is_topmost == FALSE)
        {
            home_is_topmost = TRUE;
            for (applet_num = 0; applet_num < num_applets; applet_num++)
            {
                if (plugin_exists[applet_num] && applet_info[applet_num].status)
                {
                    hildon_home_plugin_applet_foreground(plugin[applet_num]);
                }
            }
        }

        if (win.win != NULL)
        {
            XFree(win.win);
        }
    }
    return GDK_FILTER_CONTINUE;
}

void home_deinitialize(gint keysnooper_id)
{
    /* keysnooper_id is no longer used, and the keyboard listener needs not
       to be detached. The api between maemo and home mains for now
       remains unchanged */
    hildon_home_save_configure();
    gdk_window_remove_filter(GDK_WINDOW(window->window),
                             hildon_home_event_filter, NULL);
    hildon_home_deinitiliaze();

    osso_deinitialize(osso_home);
}

static
void create_startup_lock(void)
{
     FILE *f;
     
     f = fopen(STARTUP_LOCK_FILE, "w");
     fclose(f);
}
 
static
gboolean startup_lock_exists(void)
{
     return g_file_test(STARTUP_LOCK_FILE, G_FILE_TEST_EXISTS);
}
 
static
gboolean remove_startup_lock_timeout(gpointer data)
{
     unlink(STARTUP_LOCK_FILE);
     return FALSE;
}
 

int hildon_home_main(void)
{
    gint window_width;
    gint window_height;
    
    hildon_home_initiliaze();
    
    icon_theme = gtk_icon_theme_get_default();
    
    window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    
    home_is_topmost = TRUE;    

    g_signal_connect_after(window, "set-focus", 
                           G_CALLBACK(set_focus_to_widget_cb), NULL);
    
    gtk_window_set_has_frame( GTK_WINDOW( window ), FALSE );
    
    gdk_window_get_geometry(gdk_xid_table_lookup(GDK_ROOT_WINDOW ()),
                            NULL, NULL, 
                            &window_width, &window_height, NULL);
    
    /* Osso needs to be initialized before creation of applets */

    osso_home = osso_initialize(HILDON_HOME_NAME, HILDON_HOME_VERSION, 
                           FALSE, NULL);
    if( !osso_home )
    {
        return 1;
    }

    construct_home_area();
    
    construct_titlebar_area();
    
    hildon_home_display_base();
    
    construct_applets();
    
    construct_titlebar_menu();
    
    /* added for constructing user installable applets, courtesy Tomas Junnonen */ 
    construct_user_applets();
 
    /* Remove the lock to mark a successfull startup */
    g_timeout_add(STARTUP_LOCK_TIME, remove_startup_lock_timeout, NULL);


    /* Install key listeners to handle menu toggling */

    g_signal_connect(G_OBJECT(window), "key_press_event",
                     G_CALLBACK(hildon_home_key_press_listener), NULL);
    g_signal_connect(G_OBJECT(window), "key_release_event",
                     G_CALLBACK(hildon_home_key_release_listener), NULL);

    /*gdk_window_add_filter(GDK_WINDOW(window->window),
                          hildon_home_event_filter, NULL);*/
    
    /* Hand applied Mathew's patch here, Karoliina Salminen 07092005 bugfixcamp */
    /* Set up the filter so we _only_ get notified for propertys
     * changing on the root window. That is enough to know if we
     * are currently at the top of the stack
     * Ensure that we do not overwrite existing event mask, but only add
       to it (important as long as statusbar, home and TN are one process) */

    gdk_window_set_events(gdk_get_default_root_window(),
			  gdk_window_get_events(gdk_get_default_root_window())
			  | GDK_PROPERTY_CHANGE_MASK);
    gdk_window_add_filter(gdk_get_default_root_window(),
                           hildon_home_event_filter, NULL);

    
    g_signal_connect(window, "style-set", 
                     G_CALLBACK(construct_background_image_with_new_skin), 
                     NULL);
    
    hildon_home_save_configure();
    
    return 0;
}

#ifndef USE_AF_DESKTOP_MAIN__

int main(int argc, char **argv)
{
    gint window_width;
    gint window_height;
    gint keysnooper_id = 0;
    signal(SIGINT, exit);
    signal(SIGTERM, exit);
    signal(SIGHUP, exit);
    
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    textdomain(PACKAGE);
    
    gtk_init(&argc, &argv);
    
    gnome_vfs_init();
    
    keysnooper_id=hildon_home_main();
    
    gtk_main();
    hildon_home_deinitiliaze(keysnooper_id);
}

#endif
