/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
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
 
#define USE_AF_DESKTOP_MAIN__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
/* Hildon includes */
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>

/* GTK includes */
#include <gtk/gtkwindow.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkarrow.h>
#include <gtk/gtkfixed.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkwidget.h>

/* GDK includes */
#include <gdk/gdkwindow.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

/* GLib */
#include <glib.h>
#include <glib/gprintf.h>

/* X include */
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

/* log include */
#include <osso-log.h>

/* dnotify */
#include "hildon-base-lib/hildon-base-dnotify.h"

#include "hildon-status-bar-lib.h"
#include "hildon-status-bar-item.h"
#include "hildon-status-bar-main.h"
#include "hildon-status-bar-interface.h"
#include "../kstrace.h"


static gint _delayed_infobanner_add(gint32 pid, 
		                    gint32 begin, 
				    gint32 timeout, 
				    const gchar *text);
gboolean _delayed_infobanner_remove(gpointer data);
gboolean _delayed_ib_show(gpointer data);
void _remove_sb_d_ib_item (gpointer key, gpointer value, gpointer user_data);

static void show_infoprint( const gchar *msg );

static gint rpc_cb( const gchar *interface, 
		    const gchar *method, 
		    GArray *arguments, 
		    gpointer data, 
		    osso_rpc_t *retval );

/* Function for creating and initializing statusbar window */
static void init_dock( StatusBar *panel );
/* Callback for handling arrow button togglings */
static void arrow_button_toggled_cb( GtkToggleButton *togglebutton, 
		                     gpointer user_data );
/* Function for closing popup window in clean way */
static void close_popup_window( StatusBar *panel );
/* Callback for listening popup window events */
static gboolean popup_window_event_cb( GtkWidget *widget, 
		                       GdkEvent *event, 
				       gpointer user_data );
/* Function for adding the default plugins */
static void add_prespecified_items( StatusBar *panel );
/* Function for getting item */ 
static HildonStatusBarItem *get_item( StatusBar *panel, const gchar *plugin );
/* Function for setting item */
static HildonStatusBarItem *add_item( StatusBar *panel, 
		                      const gchar *plugin,
				      gboolean mandatory, 
				      gint slot );

/* Function for arranging plugins in the container */
static void arrange_items( StatusBar *panel, gint start );
/* Callback for item destroy */ 
static void destroy_item( GtkObject *object,
                          gpointer user_data );

/* Function for parsing plugins from configuration files */
static GList* get_plugins_from_file( const gchar *file );
/* Function for adding configured plugins */
static gboolean add_configured_plugins( StatusBar *panel );	
/* Function for reloading all configured plugins */
static void reload_configured_plugins( char *applets_path, gpointer user_data );
/* Function for setting listener for user configuration changes */ 
static void add_notify_for_configured_plugins( StatusBar *panel );
/* Callback for updating conditionally shown plugins in panel view */
static void update_conditional_plugin_cb( HildonStatusBarItem *item, 
		                          gboolean conditional_status, 
					  StatusBar *panel );

static void statusbar_send_signal (DBusConnection *conn, 
				    HildonStatusBarItem *item,
				    gboolean status);

	
/* Global variables */		
static GHashTable *delayed_banners;
static gboolean sb_is_sensitive = TRUE;

static gboolean 
force_close_popup_cb (GtkWidget *widget, 
		      GdkEventKey *event, 
		      gpointer data);

static 
void init_dock( StatusBar *panel )
{  	
    Atom atoms[3];
    gint i;
    Display *dpy;
    Window  win;
    GtkWidget *arrow_image;
    GtkIconTheme *icon_theme;
    GdkPixbuf *arrow_pixbuf;
    GError *error = NULL;
    gchar *log_path;
    
    /* Initialize item widgets */
    for ( i = 0; i < HSB_MAX_NO_OF_ITEMS; ++i ) {
        panel->items[i] = panel->buffer_items[i] = NULL;
    }
    panel->item_num = 0;

    /* Initialize Main Window */
    panel->window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_accept_focus( GTK_WINDOW(panel->window), FALSE );
    gtk_container_set_border_width( GTK_CONTAINER(panel->window), 0 );
    gtk_widget_set_name( panel->window, "HildonStatusBar" );
    
    /* Set window type as DOCK */
    gtk_window_set_type_hint( GTK_WINDOW( panel->window ),
		              GDK_WINDOW_TYPE_HINT_DOCK );

    /* GtkWindow must be realized before using XChangeProperty */
    gtk_widget_realize( GTK_WIDGET(panel->window) );
    
    /* We need to set this here, because otherwise the GDK will replace
     * our XProperty settings. The GDK needs to a patch for this, this
     * has nothing to do with the vanilla GDK. */
    g_object_set_data( G_OBJECT(panel->window->window), 
		       "_NET_WM_STATE", (gpointer)PropModeAppend ); 
    dpy = GDK_DISPLAY();
    win = GDK_WINDOW_XID( panel->window->window );
    
    /* Create the atoms we need */
    atoms[0] = XInternAtom(dpy, "_NET_WM_STATE", False);
    atoms[1] = XInternAtom(dpy, "_MB_WM_STATE_DOCK_TITLEBAR", False);
    atoms[2] = XInternAtom(dpy, "_MB_DOCK_TITLEBAR_SHOW_ON_DESKTOP", False);

    /* Set proper properties for the matchbox to show
     * dock as the dock we want. 2 Items to show. */
    XChangeProperty(dpy, win, atoms[0], XA_ATOM, XLIB_FORMAT_32_BIT, 
		    PropModeReplace, (unsigned char *) &atoms[1], 2);
    
    panel->fixed = gtk_fixed_new();
    gtk_container_set_border_width( GTK_CONTAINER(panel->fixed), 0 );
    gtk_container_add( GTK_CONTAINER(panel->window), panel->fixed );
    gtk_widget_show_all( panel->window );
    
    /* Initialize arrow button */
    panel->arrow_button = gtk_toggle_button_new();
    gtk_widget_set_name(panel->arrow_button, "HildonStatusBarItem");
    g_object_ref( panel->arrow_button );
    gtk_object_sink( GTK_OBJECT(panel->arrow_button) );
    gtk_widget_set_size_request(panel->arrow_button, 
		                HSB_ITEM_SIZE, HSB_ITEM_SIZE);
    arrow_image = gtk_image_new();
    icon_theme = gtk_icon_theme_get_default();
    arrow_pixbuf = gtk_icon_theme_load_icon(icon_theme,
                                            HSB_ARROW_ICON_NAME,
                                            HSB_ARROW_ICON_SIZE,
                                            GTK_ICON_LOOKUP_NO_SVG,
                                            &error);
    if( arrow_pixbuf )
    {
        gtk_image_set_from_pixbuf(GTK_IMAGE(arrow_image), arrow_pixbuf);
        gdk_pixbuf_unref(arrow_pixbuf);
    }
    else if( error && error->message )
    {
        g_warning("Could not load statusbar extension icon: %s",
                  error->message);
        g_error_free(error);
    }

    gtk_button_set_image (GTK_BUTTON (panel->arrow_button), arrow_image);
    panel->arrow_button_toggled = FALSE; 
    g_signal_connect(panel->arrow_button, "toggled", 
		     G_CALLBACK(arrow_button_toggled_cb), panel);
   
    log_path = g_strdup_printf("%s%s",(gchar *)getenv("HOME"),HSB_PLUGIN_LOG_FILE);
    
    panel->log = hildon_log_new (log_path);

    /* Initialize positions for fixed window areas */
    for ( i = 0; i < HSB_VISIBLE_ITEMS_IN_ROW; i++ ) {	
	    
	/* FIXME:should really ask sb panel current width, though window 
	 * must finish the drawning process before that can be done, 
	 * which isn't happening here yet! Timing problem */
	    
        /* panel plugin coordinates */
	panel->plugin_pos_x[i] = HSB_PANEL_DEFAULT_WIDTH - 
		                 (i+1)*HSB_ITEM_X_OFFSET;	
        panel->plugin_pos_y[i] = HSB_ITEM_Y;
        
	/* extension plugin coordinates for first row */
	panel->plugin_pos_x[i+HSB_VISIBLE_ITEMS_IN_ROW] = i*HSB_ITEM_X_OFFSET;
        panel->plugin_pos_y[i+HSB_VISIBLE_ITEMS_IN_ROW] = HSB_ITEM_Y;
        /* extension plugin coordinates for second row */
	panel->plugin_pos_x[i+2*HSB_VISIBLE_ITEMS_IN_ROW] = i*HSB_ITEM_X_OFFSET;
        panel->plugin_pos_y[i+2*HSB_VISIBLE_ITEMS_IN_ROW] = HSB_ITEM_Y_OFFSET;		
    }
    
    g_free (log_path);	            
}

static gboolean 
force_close_popup_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  StatusBar *sb_panel = (StatusBar *)data;

  KeyCode keycode = 0;
  
  if (event->keyval == HILDON_HARDKEY_FULLSCREEN || 
      event->keyval == HILDON_HARDKEY_MENU)
  {
    close_popup_window( sb_panel );

    if ((keycode = XKeysymToKeycode (GDK_DISPLAY(), event->keyval)) != 0)
    {
      XTestFakeKeyEvent (GDK_DISPLAY(), keycode, TRUE, CurrentTime);
      XTestFakeKeyEvent (GDK_DISPLAY(), keycode, FALSE, CurrentTime);
    }
  }

  return TRUE;
}

static gboolean 
_sb_grab_pointer_cb (GtkWidget *window, GdkEventExpose *event, StatusBar *sb_panel)
{
  (void)event;

  if ( ( gdk_pointer_grab (window->window, TRUE, 
			   GDK_BUTTON_PRESS_MASK |
	 	           GDK_BUTTON_RELEASE_MASK |
			   GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
			   GDK_POINTER_MOTION_MASK,
			   NULL, NULL,
			   GDK_CURRENT_TIME ) == GDK_GRAB_SUCCESS ) )
  {
     return TRUE;
  }
  
  close_popup_window (sb_panel);

  return TRUE;
}


static 
void arrow_button_toggled_cb( GtkToggleButton *togglebutton,
		              gpointer user_data ) 
{  
    StatusBar *sb_panel = (StatusBar *)user_data;
    gint sb_width = 0, sb_height = 0, sb_x = 0, sb_y = 0;
    gint i, found = 0;

    /* Return if panel fails */ 
    if ( !sb_panel ) return; 
    
    sb_panel->arrow_button_toggled = !sb_panel->arrow_button_toggled;
    
    /* Show extension panel */
    if ( sb_panel->arrow_button_toggled ) {

	/* Initialize popup window */ 
	sb_panel->popup = gtk_window_new( GTK_WINDOW_TOPLEVEL );	
	gtk_widget_set_name(sb_panel->popup, "HildonStatusBarExtension");
	
	gtk_window_set_type_hint( GTK_WINDOW( sb_panel->popup ),
 				  GDK_WINDOW_TYPE_HINT_MENU );
	gtk_window_set_decorated( GTK_WINDOW( sb_panel->popup ), FALSE);

	/* Add popup fixed into window */
	sb_panel->popup_fixed = gtk_fixed_new();
	gtk_container_set_border_width
		( GTK_CONTAINER(sb_panel->popup_fixed), 0 );
	gtk_container_add
		( GTK_CONTAINER(sb_panel->popup), sb_panel->popup_fixed );
	
	/* Add popup window into Home screen */
	GdkScreen *screen = gdk_screen_get_default();
	gtk_window_set_screen(GTK_WINDOW(sb_panel->popup), screen);
	gtk_window_set_modal(GTK_WINDOW(sb_panel->popup), FALSE);

		
    /* Add all yet non-visible plugins into extension panel */	
    for ( i = 0; i < HSB_MAX_NO_OF_ITEMS; i++ ) {

        if ( !sb_panel->items[i] ) { 
            continue;
        }
        if ( !hildon_status_bar_item_get_conditional
                (HILDON_STATUS_BAR_ITEM(sb_panel->items[i])) ) { 
            continue;
        }
        if ( ++found < HSB_VISIBLE_ITEMS_IN_ROW ) {  
            continue;
        }	  

        /* If conditional update leaves only one surplus plugin 
         * for extension return */
        if ( gtk_widget_get_parent(sb_panel->items[i]) 
                == sb_panel->fixed ) {	
            close_popup_window( sb_panel ); 
            return;	
        } 

        /* Add plugin onto right position */
        if ( found > HSB_VISIBLE_ITEMS_IN_1ROW_EXTENSION && 
                found <= HSB_VISIBLE_ITEMS_IN_2ROW_EXTENSION ) {

            gtk_fixed_put( GTK_FIXED(sb_panel->popup_fixed), 
                    sb_panel->items[i], /* 2 empty positions */
                    sb_panel->plugin_pos_x[found+2],  
                    sb_panel->plugin_pos_y[found+2] );

        } else if( found <= HSB_VISIBLE_ITEMS_IN_1ROW_EXTENSION ) {

            /*		g_object_ref( sb_panel->items[i] );	 */
            gtk_fixed_put( GTK_FIXED(sb_panel->popup_fixed), 
                    sb_panel->items[i], /* 1 empty position */
                    sb_panel->plugin_pos_x[found+1],  
                    sb_panel->plugin_pos_y[found+1] );	

        }

        /* Show plugin */	
        gtk_widget_show( sb_panel->items[i] );

    } /* ..for items */  

	/* Define extension panel size and show it */
	gtk_window_get_size(GTK_WINDOW(sb_panel->window), &sb_width, &sb_height);
	
	/* Show 2 row extension */
	if ( found > HSB_VISIBLE_ITEMS_IN_1ROW_EXTENSION ) { 
            gtk_window_set_default_size(GTK_WINDOW(sb_panel->popup), 
			                sb_width, 2*sb_height);
	} else { /* Show 1 row extension */
	    gtk_window_set_default_size(GTK_WINDOW(sb_panel->popup), 
			                sb_width, sb_height);
	}
	
	/* Move extension onto right place */
	gdk_window_get_origin(GDK_WINDOW(sb_panel->window->window), 
			      &sb_x, &sb_y);
	gtk_window_move(GTK_WINDOW(sb_panel->popup), sb_x, 
			sb_height + sb_y + HSB_MARGIN_DEFAULT);

	gtk_widget_realize (sb_panel->popup);

	gdk_window_set_transient_for( sb_panel->popup->window,
  		                      gdk_screen_get_active_window( gtk_widget_get_screen(GTK_WIDGET (sb_panel->arrow_button))));
	
	gtk_widget_show_all(sb_panel->popup);
	
	/* Set button-release listener */
	
	g_signal_connect_after (G_OBJECT (sb_panel->popup), 
				"map-event", 
				G_CALLBACK (_sb_grab_pointer_cb), 
				(gpointer)sb_panel);
	
	g_signal_connect(G_OBJECT(sb_panel->popup), "button-release-event", 
		         G_CALLBACK(popup_window_event_cb), sb_panel);

	g_signal_connect (G_OBJECT (sb_panel->popup),
			  "key-press-event",
			  G_CALLBACK(force_close_popup_cb),
			  (gpointer)sb_panel);

	gtk_grab_add (sb_panel->popup);

    } /* ..if arrow button toggled */ 
 
}


static 
gboolean popup_window_event_cb( GtkWidget *widget, 
		                GdkEvent *event, 
				gpointer user_data )
{
    StatusBar *sb_panel = (StatusBar *)user_data;
    
    /* Return if panel fails */
    if ( !event || !sb_panel ) return FALSE;
    
    gboolean in_panel_area = FALSE, in_button_area = FALSE;
    gint x, y;
    
    gtk_widget_get_pointer(widget, &x, &y);
    gint w = widget->allocation.width;
    gint h = widget->allocation.height;
    	 
    /* Pointer on window popup area */
    if ( (x >= 0) && (x <= w) && (y >= 0) && (y <= h) ) {
        in_panel_area = TRUE;
    } 
    else if ( sb_panel->arrow_button_toggled ) {	
	    
        w = (sb_panel->arrow_button)->allocation.width;
	h = (sb_panel->arrow_button)->allocation.height;
	gtk_widget_get_pointer(sb_panel->arrow_button, &x, &y);
	
	/* Pointer on button area  */
	if ( (x >= 0) && (x <= w) && (y >= 0) && (y <= h) ) { 
            in_button_area = TRUE;
	}
    } 
    
    /* Event outside of popup or in button area, close in clean way */    
    if ( !in_panel_area || in_button_area ) {
        close_popup_window( sb_panel );
    }
   
    return TRUE;
}


static 
void close_popup_window( StatusBar *panel )
{ 
    guint i;    

    /* Return if popup doesn't exist */
    if ( !panel || !panel->popup )  return;
    
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    gdk_keyboard_ungrab(GDK_CURRENT_TIME);
    
    g_object_set(panel->arrow_button, "active", FALSE, NULL);
    
    gtk_grab_remove(panel->popup);

    /* Remove extension plugins from the container so they wont 
     * be destroyed upon popup destroy */
    for ( i = 0; i < HSB_MAX_NO_OF_ITEMS; i++ ) {
	    
        if ( !panel->items[i] ) { 
            continue;
	}
	if ( gtk_widget_get_parent(panel->items[i]) == panel->popup_fixed ) {
            gtk_container_remove( GTK_CONTAINER(panel->popup_fixed), 
			          panel->items[i] );
	}
    }
									    
    gtk_widget_destroy( panel->popup_fixed );
    gtk_widget_destroy( panel->popup );
    panel->popup_fixed = NULL;
    panel->popup = NULL;

    panel->arrow_button_toggled = FALSE;
    
}


static 
void add_prespecified_items( StatusBar *panel )
{
    if ( !add_item(panel, "sound", FALSE, HSB_SOUND_SLOT) ) {
        ULOG_ERR("FAILED to add prespecified Statusbar item Sound");
    }
    if ( !add_item(panel, "internet", FALSE, HSB_INTERNET_SLOT) ) {
        ULOG_ERR("FAILED to add prespecified Statusbar item Internet");
    }
    if ( !add_item(panel, "battery", TRUE, HSB_BATTERY_SLOT) ) {
        ULOG_ERR("FAILED to add prespecified Statusbar item Battery");
    }
    if ( !add_item(panel, "display", FALSE, HSB_DISPLAY_SLOT) ) {
        ULOG_ERR("FAILED to add prespecified Statusbar item Display");
    }
    if ( !add_item(panel, "presence", TRUE, HSB_PRESENCE_SLOT) ) { 
       	ULOG_ERR("FAILED to add prespecified Statusbar item Presence");
    }
}


static 
GList* get_plugins_from_file( const gchar *file )
{   	
    GKeyFile *keyfile = g_key_file_new();
    GError *error  = NULL;
    gchar **groups = NULL; 
    StatusBarPlugin *plugin;
    /* Key values */
    gint position; 
    gchar *library;
    gchar *category;
    gboolean mandatory = FALSE;
    /* Plugin list */
    GList *list = NULL;
    gint i;
    
    /* Return if file string does not exist */
    if ( !file )
    {
	g_key_file_free (keyfile);
	return NULL;	
    }
    
    /* Load configuration file */ 
    g_key_file_load_from_file(keyfile, file, G_KEY_FILE_NONE, &error);
   
    /* Could not load the configuration file */
    if ( error ) { 
        g_key_file_free(keyfile);
	
	/* The user wants an empty configuration */
	if ( g_str_has_suffix(file, HSB_PLUGIN_USER_CONFIG_FILE) 
	     && g_file_test(file, G_FILE_TEST_EXISTS) ) {
	     
	    g_error_free(error);
	    plugin = g_new0(StatusBarPlugin, 1);
	    list = g_list_append(list, plugin);
	    /* return an empty plugin */
	    return list;
	}	  	    
	g_error_free(error);
	
	return NULL;
    }
   
    /* Get plugins */
    groups = g_key_file_get_groups(keyfile, NULL);
    
    /* Get plugin data */
    i = -1;	
    while ( groups[++i] != NULL ) {    
       	
	/* get library value */    
        library = g_key_file_get_string(keyfile, groups[i], 
			                HSB_PLUGIN_CONFIG_LIBRARY_KEY, 
					&error);
	
	if ( error != NULL ) { 
	    ULOG_ERR("FAILED to read value key \'%s\' from \'%s\'", 
		     HSB_PLUGIN_CONFIG_LIBRARY_KEY, file);
	    g_error_free(error);
	    error = NULL;
	    g_free(library);
	    
	    continue;
	}

	if (!strcmp (library,HSB_PLUGIN_CONFIG_LIBRARY_VALUE))
	{
	  g_free (library);
	  continue;
	}
	
        /* get position value */
	position = g_key_file_get_integer(keyfile, groups[i], 
			                  HSB_PLUGIN_CONFIG_POSITION_KEY, 
					  &error);
	
	if ( error != NULL ) { 
	    ULOG_ERR("FAILED to read value for key \'%s\' from \'%s\'", 
		     HSB_PLUGIN_CONFIG_POSITION_KEY, file);	     
	    g_error_free(error);
	    error = NULL;
	    
	    continue;
	}
	
	/* get category value */
	category = g_key_file_get_string(keyfile, groups[i], 
			                 HSB_PLUGIN_CONFIG_CATEGORY_KEY, 
					 &error);
        
	if ( error != NULL ) { 
	    ULOG_ERR("FAILED to read value for key \'%s\' from \'%s\'",
		     HSB_PLUGIN_CONFIG_CATEGORY_KEY, file); 
	    g_error_free(error);
	    error = NULL;
	    category = HSB_PLUGIN_CATEGORY_PERMANENT; 
	}
	
	if ( !g_str_equal(category, HSB_PLUGIN_CATEGORY_PERMANENT) && 
	     !g_str_equal(category, HSB_PLUGIN_CATEGORY_CONDITIONAL) && 
	     !g_str_equal(category, HSB_PLUGIN_CATEGORY_TEMPORAL) ) {
		
            category = HSB_PLUGIN_CATEGORY_PERMANENT;	
	}		

	/* get optional mandatory value */

	mandatory = g_key_file_get_boolean (keyfile, groups[i],
					    HSB_PLUGIN_CONFIG_MANDATORY_KEY,&error);
	
	if ( error != NULL ) {
	    g_error_free(error);
	    error = NULL;
	    mandatory = FALSE;
	}			

	/* create new Status Bar plugin */
 	plugin = g_new0(StatusBarPlugin, 1);
        if ( !plugin ) {   
            ULOG_ERR("FAILURE: Memory exhausted when loading plugin structure");
	    g_strfreev (groups);
	    g_key_file_free (keyfile);
	    g_list_free (list);
            return NULL;
	}
	
	plugin->position  = position;
        plugin->library   = g_strdup(library);
	plugin->mandatory = mandatory;
	g_free(library);
	g_free(category);

	list = g_list_append(list, plugin);
		
    } /* ..while groups != NULL */

    
    /* Cleanup */
    g_strfreev(groups);
    g_key_file_free(keyfile);    

    return list;
} 


static 
gboolean add_configured_plugins( StatusBar *panel )
{   	
    gchar *user_dir;
    gchar *plugin_path = NULL;
    GList *iter = NULL, *l = NULL;
    GList *bad_plugins = NULL;

    /* Return if panel fails */
    if ( !panel )  return FALSE;

    /* Get user plugins */
    user_dir = g_strdup_printf("%s%s", (gchar *)getenv("HOME"), 
            HSB_PLUGIN_USER_CONFIG_FILE);
    l = get_plugins_from_file(user_dir);
    g_free(user_dir);

    /* Get factory plugins */
    if (!l) {
        l = get_plugins_from_file(HSB_PLUGIN_FACTORY_CONFIG_FILE);	
    }

    /* Could not load plugin configuration file */     
    if (!l) { 
        ULOG_WARN("Could not read configuration file for status bar plugins.");
        return FALSE; 
    }

    bad_plugins = hildon_log_get_incomplete_groups (panel->log,
		    				    HSB_PLUGIN_LOG_KEY_START,
						    HSB_PLUGIN_LOG_KEY_END,NULL);

    for (iter = l; iter ; iter = iter->next) {
        StatusBarPlugin *plugin = (StatusBarPlugin *)iter->data;
	
	plugin_path = NULL;
	
        if ( plugin ) { 
            /* Extract name, cut of lib prefix and .so suffix */	
            if ( plugin->library && 
                    g_str_has_prefix(plugin->library, "lib") && 
                    g_str_has_suffix(plugin->library, ".so") &&
                    g_file_test ( 
                        (plugin_path = g_strdup_printf("/usr/lib/hildon-status-bar/%s",plugin->library)),
                        G_FILE_TEST_EXISTS
                        )) 
            { 
                gchar *stripped = g_strndup(plugin->library + 3, 
                        strlen(plugin->library) - 6);

		if (g_list_find_custom (bad_plugins,stripped,(GCompareFunc)strcmp) == NULL)
                  add_item(panel, stripped, plugin->mandatory, plugin->position);
		
                g_free(stripped);
            }
        }

	g_free (plugin_path);
    }

    /* Cleanup */
    g_list_free (l);
    g_list_foreach (bad_plugins,(GFunc)g_free,NULL);
    g_list_free (bad_plugins);
    hildon_log_remove_file (panel->log);

    return TRUE;
}


static 
void add_notify_for_configured_plugins( StatusBar *panel ) 
{      
    gchar *user_directory = g_strdup_printf
	    ("%s%s", (gchar *)getenv("HOME"), HSB_PLUGIN_USER_CONFIG_DIR);

    /* Make user configuration directory if it doesn't already exist */
    if ( g_mkdir_with_parents(user_directory, 0755) < 0 ) {
        ULOG_DEBUG("Status Bar FAILED to create user configuration directory"
		   " '%s': user is unable to configure Status Bar plugins!", 
		   user_directory);
    }

    /* Set callback notify for user configuration filepath */
    if ( hildon_dnotify_set_cb
		    ((hildon_dnotify_cb_f *) reload_configured_plugins,
		     user_directory, panel) == HILDON_ERR ) { 
        ULOG_WARN("Could not set notify for directory:%s", 
		  HSB_PLUGIN_USER_CONFIG_DIR);
    }
 
    /* Cleanup */
    g_free(user_directory);
    
}

static void
buffer_item ( StatusBar *panel, GtkWidget * item )
{
  gint i;
  g_return_if_fail (panel && item);
	 
  for (i=0; i<HSB_MAX_NO_OF_ITEMS; i++)
    if (panel->buffer_items[i] == NULL) break;
 
  if (i == HSB_MAX_NO_OF_ITEMS)
  {
    ULOG_ERR ("I can t buffer the item. Destroying it");
    gtk_widget_destroy (item);
    panel->item_num++; /* The destroy signal is connected to destroy item
			  and it tries to remove from the panel, but it's
			  not actually in it
			*/
  }
  else
  {
    panel->buffer_items[i] = item;
  }
			
}

static 
void reload_configured_plugins( char *applets_path, gpointer user_data )
{	
    StatusBar *panel = (StatusBar *)user_data;
    gint i = -1;
    gboolean item_is_mandatory = FALSE;
    
    /* Return if panel fails */
    g_return_if_fail (panel);
       
    /* Remove all pre-existing status bar plugins */
    while ( ++i < HSB_MAX_NO_OF_ITEMS ) {
        if( panel->items[i] ) {
            item_is_mandatory =
      	     hildon_status_bar_item_get_mandatory (
                     HILDON_STATUS_BAR_ITEM(panel->items[i]));	

	    if (!item_is_mandatory)
            { 
              GtkWidget *item = panel->items[i];
              gtk_widget_destroy(item);
              /* We need to finalize the plugin, so remove our reference */
              g_object_unref (item);
            }
	    else
	    {
	      buffer_item(panel,panel->items[i]);
	      panel->items[i] = NULL;
	      panel->item_num--;
	    }
        }
    }
    
    /* Add re-configured plugins */	   	    
    if ( !add_configured_plugins( panel ) ) {    
        add_prespecified_items( panel );	
    }
 
    gtk_widget_show_all( panel->window );
    
}


static 
void show_infoprint( const gchar *msg )
{     		
    if ( !msg ) return;

    hildon_banner_show_information( NULL, NULL, msg );
}

static void statusbar_insensitive_cb( GtkWidget *widget, gpointer data)
{

    gtk_widget_set_sensitive(widget, FALSE);

    if (GTK_IS_CONTAINER(widget))
    {
    gtk_container_foreach(GTK_CONTAINER(widget),
                  (GtkCallback)(statusbar_insensitive_cb),
                  NULL);
    }
}
static void statusbar_sensitive_cb( GtkWidget *widget, gpointer data)
{

    gtk_widget_set_sensitive(widget, TRUE);

    if (GTK_IS_CONTAINER(widget))
    {
    gtk_container_foreach(GTK_CONTAINER(widget),
                  (GtkCallback)(statusbar_sensitive_cb),
                  NULL);
    }
}

static 
gint rpc_cb( const gchar *interface, 
             const gchar *method,
             GArray *arguments, 
             gpointer data, 
             osso_rpc_t *retval )
{
    StatusBar *panel;
    osso_rpc_t *val[4];
    gint i;
    
    /* These must be specified */
    if ( !interface || !method || !arguments || !data ) {
        retval->value.s = "NULL parameter";
        ULOG_WARN("%s", retval->value.s);
	
        return OSSO_ERROR;
    }

    panel = (StatusBar*)data;
	    
    /* Get the parameters */
    for ( i = 0; i < arguments->len; ++i ) { 
        val[i] = &g_array_index( arguments, osso_rpc_t, i );
    }
  
    /* Check the method infoprint */
    if ( g_str_equal("system_note_infoprint", method) )
    {
        /* Check that the parameter is of right type */
        if ( arguments->len < 1 ||
             val[0]->type != DBUS_TYPE_STRING ) {
		
            if( arguments->len < 1 ) {
                retval->value.s = g_strdup("Not enough args to infoprint");
            } else {
                g_sprintf(retval->value.s, 
			  "Wrong type param to infoprint (%d)", val[0]->type);
            }
	    
            if ( retval->value.s ) {
                ULOG_ERR( retval->value.s );
            }

            return OSSO_ERROR;       
        }
        show_infoprint( val[0]->value.s );
    }
    
    /* Check the method dialog */
    else if( g_str_equal( "system_note_dialog", method ) )
    {
        /* check arguments */
        if( arguments->len < 2 || 
            val[0]->type != DBUS_TYPE_STRING || 
            val[1]->type != DBUS_TYPE_INT32 ) {
		
            if( arguments->len < 2 ) {
                retval->value.s = "Not enough args to dialog";
            } else {
                retval->value.s = "Wrong type of arguments to dialog";
            }
            ULOG_ERR( retval->value.s );
            return OSSO_ERROR;       
        }

        hildon_status_bar_lib_prepare_dialog( val[1]->value.i, NULL, 
                                              val[0]->value.s, 0, NULL, NULL );
    }
    /* open closeable system dialog */
    else if( g_str_equal( "open_closeable_system_dialog", method ) )
    {
        gint id;
        const gchar *btext = NULL;
        /* check arguments */
        if( arguments->len < 4 ||
            val[0]->type != DBUS_TYPE_STRING ||
            val[1]->type != DBUS_TYPE_INT32 ||
            val[2]->type != DBUS_TYPE_STRING ||
            val[3]->type != DBUS_TYPE_BOOLEAN )
        {
            retval->type = DBUS_TYPE_STRING;
            if( arguments->len < 4 ) {
                retval->value.s = g_strdup( "Not enough args to dialog" );
            } else {
                retval->value.s = g_strdup( "Wrong type of arguments to dialog" );
            }
            ULOG_ERR( retval->value.s );
            return OSSO_ERROR;
        }

        if( (val[2]->value.s)[0] != '\0' )
        {
            btext = val[2]->value.s;
        }

        id = hildon_status_bar_lib_open_closeable_dialog( val[1]->value.i,
                 val[0]->value.s, btext, val[0]->value.b );
        /* return id of the dialog to the caller */
        retval->type = DBUS_TYPE_INT32;
        retval->value.i = id;
    }
    /* close closeable system dialog */
    else if( g_str_equal( "close_closeable_system_dialog", method ) )
    {
        /* the id of the dialog is given as argument */
        if( arguments->len < 1 || val[0]->type != DBUS_TYPE_INT32 )
        {
            retval->type = DBUS_TYPE_STRING;
            if( arguments->len < 1 ) {
                retval->value.s = g_strdup( "Not enough args to dialog" );
            } else {
                retval->value.s = g_strdup( "Argument has invalid type" );
            }
            ULOG_ERR( retval->value.s );
            return OSSO_ERROR;
        }

        hildon_status_bar_lib_close_closeable_dialog( val[0]->value.i );
    }
    /* get response value of system dialog */
    else if( g_str_equal( "get_system_dialog_response", method ) )
    {
        gint response = -1;
        /* the id of the dialog is given as argument */
        if( arguments->len < 1 || val[0]->type != DBUS_TYPE_INT32 )
        {
            retval->type = DBUS_TYPE_STRING;
            if( arguments->len < 1 ) {
                retval->value.s = g_strdup( "Not enough args to dialog" );
            } else {
                retval->value.s = g_strdup( "Argument has invalid type" );
            }
            ULOG_ERR( retval->value.s );
            return OSSO_ERROR;
        }

        response = hildon_status_bar_lib_get_dialog_response(
                        val[0]->value.i );
        retval->type = DBUS_TYPE_INT32;
        retval->value.i = response;
    }
    /* plugin */
    else if( g_str_equal( "event", method ) )
    {
        HildonStatusBarItem *item;
        /* check arguments */
        if( arguments->len < 4 || 
            val[0]->type != DBUS_TYPE_STRING ||
            val[1]->type != DBUS_TYPE_INT32 ||
            val[2]->type != DBUS_TYPE_INT32 ||
            val[3]->type != DBUS_TYPE_STRING ) {
            if( arguments->len < 4 ) {
                retval->value.s = "Not enough arguments to plugin";
            } else {
                g_sprintf(retval->value.s, "Wrong type of arguments to a plugin "
			  "(%d,%d,%d,%d); was expecting (%d, %d, %d ,%d)",
			  val[0]->type, val[1]->type, 
			  val[2]->type, val[3]->type,
			  DBUS_TYPE_STRING, DBUS_TYPE_INT32,
			  DBUS_TYPE_INT32, DBUS_TYPE_STRING);
            }
            ULOG_ERR( retval->value.s );
	    
            return OSSO_ERROR;
        }

        if (!panel)
            return OSSO_ERROR;

        item = get_item( panel, val[0]->value.s );
        
        /* Plugin loading failed */
        if ( !item ) {
	    
            return OSSO_ERROR;
        }

        hildon_status_bar_item_update( item, 
                                       val[1]->value.i, 
                                       val[2]->value.i, 
                                       val[3]->value.s );
    }	
    /* Check the method delayed infobanner */    
    else if( g_str_equal( "delayed_infobanner", method ) )
    {
        if( arguments->len < 4 ||
	    arguments->len > 5 ||
            val[0]->type != DBUS_TYPE_INT32 ||
            val[1]->type != DBUS_TYPE_INT32 ||
            val[2]->type != DBUS_TYPE_INT32 ||
            val[3]->type != DBUS_TYPE_STRING ) 
            /* (arguments->len == 5 && val[5]->type != DBUS_TYPE_NIL) )*/
	  {
            if( arguments->len < 4 ) {
                retval->value.s = "Not enough arguments.";
            } else {
                g_sprintf(retval->value.s, "Wrong type of arguments: "
			  "(%d,%d,%d,%d); was expecting (%d, %d, %d and %d)",
			  val[0]->type, val[1]->type, 
			  val[2]->type, val[3]->type,
			  DBUS_TYPE_INT32, DBUS_TYPE_INT32, 
			  DBUS_TYPE_INT32, DBUS_TYPE_STRING);
            }
	    ULOG_ERR( retval->value.s );
	    
	    return OSSO_ERROR;
	  }
	
	return _delayed_infobanner_add(val[0]->value.i,
				       val[1]->value.i,
				       val[2]->value.i,
				       val[3]->value.s
				       );
    }
    /* Check the method cancel delayed infobanner */
    else if( g_str_equal( "cancel_delayed_infobanner", method ) )
    {
        if( arguments->len > 1 ||
                val[0]->type != DBUS_TYPE_INT32 ) {
            retval->type = DBUS_TYPE_STRING;

            if( arguments->len > 1 ) {
                retval->value.s = "Too many args.";
            } else {
                g_sprintf(retval->value.s, "Wrong type of arguments: "
                        "(%d), was expecting int (%d)",
                        val[0]->type, DBUS_TYPE_INT32);
            }
            ULOG_ERR( retval->value.s );

            return OSSO_ERROR;   
        }	

        _delayed_infobanner_remove(GINT_TO_POINTER(val[0]->value.i));
        /* this function returns boolean for the timeout functions, *
         * we don't care about that here. It's false always, anyway */

        return OSSO_OK;

    }
    else if( g_str_equal( "statusbar_insensitive", method ) )
    {
        sb_is_sensitive = FALSE;
        gtk_container_foreach(GTK_CONTAINER(panel->fixed),
                (GtkCallback)(statusbar_insensitive_cb),
                NULL);

  	gtk_container_foreach(GTK_CONTAINER(panel->arrow_button),
                (GtkCallback)(statusbar_insensitive_cb),
                NULL);

        return OSSO_OK;
    }
    else if( g_str_equal( "statusbar_sensitive", method ) )
    {
        sb_is_sensitive = TRUE;
        gtk_container_foreach(GTK_CONTAINER(panel->fixed),
                (GtkCallback)(statusbar_sensitive_cb),
                NULL);

	gtk_container_foreach(GTK_CONTAINER(panel->arrow_button),
                (GtkCallback)(statusbar_sensitive_cb),
                NULL);

        return OSSO_OK;
    }
    else if( g_str_equal( "statusbar_get_conditional", method ) )
    {
	int i;

	for (i=0;i<HSB_MAX_NO_OF_ITEMS;i++) /* can we break earlier? */
	{
	   if (panel->items[i])
	   {
	     statusbar_send_signal (osso_get_dbus_connection (panel->osso),
	                            HILDON_STATUS_BAR_ITEM (panel->items[i]),
				    hildon_status_bar_item_get_conditional
				     (HILDON_STATUS_BAR_ITEM (panel->items[i])));
	   }
	}   
   	g_debug ("Conditional check called");
	return OSSO_OK;
    }
    /* Unknown method */
    else
    {
        ULOG_ERR( "Unknown SB RPC method" );
        
        return OSSO_ERROR;
    }

    return OSSO_OK;
}


static 
HildonStatusBarItem *get_item( StatusBar *panel, const gchar *plugin )
{
    gint i;

    /* Return if panel or plugin name fails */
    if ( !panel || !plugin )  return NULL;

    /* Go through the array and check if the plugin match */
    for( i = 0; i < HSB_MAX_NO_OF_ITEMS; ++i ) {
	    
        if( panel->items[i] != NULL &&
            g_str_equal( hildon_status_bar_item_get_name(
                         HILDON_STATUS_BAR_ITEM( panel->items[i] ) ), 
                         plugin ) )	
            return HILDON_STATUS_BAR_ITEM( panel->items[i] );
    }
    
    /* No match found */
    return NULL;
    
}

static void 
logging_start (HildonStatusBarItem *item, gpointer data)
{
  StatusBar *panel = (StatusBar *) data;
  const gchar *name = hildon_status_bar_item_get_name (item);

  hildon_log_add_group   (panel->log, name);
  hildon_log_add_message (panel->log,HSB_PLUGIN_LOG_KEY_START,"1");

}

static void 
logging_end (HildonStatusBarItem *item, gpointer data)
{
  StatusBar *panel = (StatusBar *) data;

  hildon_log_add_message (panel->log,HSB_PLUGIN_LOG_KEY_END,"1");

}

static 
HildonStatusBarItem *add_item( StatusBar *panel, 
		               const gchar *plugin,
			       gboolean mandatory, 
			       gint slot )
{
    HildonStatusBarItem *item;
    gint i=0;
    gboolean already_loaded=FALSE;

    /* Return if panel or plugin name fails */
    g_return_val_if_fail (panel && plugin, NULL);
    /* Check if there is already a plugin mandatory and with the same name */

    for (i=0; i<HSB_MAX_NO_OF_ITEMS; i++)
    {
        if (panel->buffer_items[i] != NULL)
        {
            if ( hildon_status_bar_item_get_mandatory ( 
                        HILDON_STATUS_BAR_ITEM (panel->buffer_items[i])) &&
                    
                  !strcmp (plugin, hildon_status_bar_item_get_name ( 
                          HILDON_STATUS_BAR_ITEM (panel->buffer_items[i]))) )
            {
                already_loaded = TRUE;
                break;
            }
        }
    }
				
    if (!already_loaded)
    {
      item = hildon_status_bar_item_new( plugin, mandatory );

      g_return_val_if_fail (item,NULL);

      if( panel->item_num >= HSB_MAX_NO_OF_ITEMS-1 && item )
      {
        gtk_widget_destroy( GTK_WIDGET( item ));
        return NULL;
      }

      /* ref the item, so it is not destroyed when removed temporarily
       * from container, or destroyed too early when the plugin is removed
       * permanently */
      g_object_ref( item );
      gtk_object_sink( GTK_OBJECT( item ) );

      g_signal_connect (G_OBJECT (item),
                        "hildon_status_bar_log_start",
                        G_CALLBACK (logging_start),
                        (gpointer)panel);

      g_signal_connect (G_OBJECT (item),
                        "hildon_status_bar_log_end",
                        G_CALLBACK (logging_end),
                        (gpointer)panel);

      hildon_status_bar_item_initialize (item);
    }
    else
    {
      item = HILDON_STATUS_BAR_ITEM (panel->buffer_items[i]);
      panel->buffer_items[i] = NULL;
    }
   
    /* The item is either configured or prespecified */
    if( slot != -1 ) { 	    
	hildon_status_bar_item_set_position(item, slot);
	panel->items[slot] = GTK_WIDGET( item );
    } else { /* Unknown plugin */
       	hildon_status_bar_item_set_position(item, panel->item_num);
	panel->items[panel->item_num] = GTK_WIDGET( item );	
    }
   
    if (!already_loaded) 
    {	    
      /* Catch the event of plugin destroying itself */
      g_signal_connect( G_OBJECT( item ), "destroy", 
		        G_CALLBACK( destroy_item ), panel );
      /* Catch the event of plugin updating self conditional status */
      g_signal_connect( G_OBJECT( item ), "hildon_status_bar_update_conditional",
		        G_CALLBACK(update_conditional_plugin_cb), panel );
    }
    /* Increase number of total added items */
    panel->item_num++;
    
    /* Arrange */
    arrange_items( panel, panel->item_num-1 );
    
    return item;
}


static 
void update_conditional_plugin_cb( HildonStatusBarItem *item,  
			           gboolean conditional_status,
			           StatusBar *panel ) 
{	
    /* Return if plugin item or panel fails */
    if ( !panel || !item )  return;     	

    int slot = hildon_status_bar_item_get_position(item);

    /* Remove item from the panel container */
    if( !conditional_status ) {

        if ( gtk_widget_get_parent( panel->items[slot] ) == panel->fixed ) {
            gtk_container_remove( GTK_CONTAINER(panel->fixed), 
                    panel->items[slot] );
        }

    }

    statusbar_send_signal ( osso_get_dbus_connection (panel->osso),
 			    HILDON_STATUS_BAR_ITEM (panel->items[slot]),
			    conditional_status);

    g_debug ("signal sended %d",conditional_status);

    /* Arrange items on visible panel */  
    arrange_items( panel, slot );

    /* If the extension panel was opened update */
    if ( panel->popup ) {
        close_popup_window( panel );
        g_signal_emit_by_name(panel->arrow_button, "toggled", panel);
    }
			        
}


static 
void arrange_items( StatusBar *panel, gint start )
{ 
    gint i, place_here, prev_place;


    /* Return if panel fails */
    if ( !panel )  return;

    place_here = 0; 
    prev_place = HSB_MAX_NO_OF_ITEMS;

    /* Go through all dynamic items */
    for ( i = 0; i < HSB_MAX_NO_OF_ITEMS; ++i ) {

        if ( !panel->items[i] ) {	
            continue;
        }


        if ( !hildon_status_bar_item_get_conditional(
                    HILDON_STATUS_BAR_ITEM(panel->items[i])) ) {
            continue;	
        }
        if ( i < start-1 ) {	
            place_here++;
            continue;
        }

        if ( place_here > HSB_VISIBLE_ITEMS_IN_ROW )
            break;

        /* Remove arrow if exists */
        if ( gtk_widget_get_parent( panel->arrow_button ) ) {	    
            gtk_container_remove( GTK_CONTAINER(panel->fixed), 
                    panel->arrow_button );
        }

        /* If the extension was opened, remove item from it */	
        if ( panel->popup_fixed && gtk_widget_get_parent
                (panel->items[i]) == panel->popup_fixed ) {

            gtk_container_remove( GTK_CONTAINER(panel->popup_fixed),
                    panel->items[i] );
        }

        /* Save last shown items place */
        if ( place_here == HSB_VISIBLE_ITEMS_IN_ROW - 1 ) {
            prev_place = i;
        }

        /* If arrow button is needed add it */
        if ( place_here == HSB_VISIBLE_ITEMS_IN_ROW ) {
            if ( gtk_widget_get_parent
                    ( panel->items[prev_place] ) == panel->fixed ) { 
                gtk_container_remove( GTK_CONTAINER(panel->fixed), 
                        panel->items[prev_place] ); 
            }
            if ( gtk_widget_get_parent
                    ( panel->items[i] ) == panel->fixed ) {
                gtk_container_remove( GTK_CONTAINER(panel->fixed), 
                        panel->items[i] );
            }

	    if (sb_is_sensitive)
      	      gtk_widget_set_sensitive (panel->arrow_button,TRUE);
    	    else
	      gtk_widget_set_sensitive (panel->arrow_button,FALSE);
	    
            gtk_fixed_put( GTK_FIXED( panel->fixed ), 
                    panel->arrow_button, 
                    panel->plugin_pos_x[place_here-1], 
                    panel->plugin_pos_y[place_here-1] );
	
            break;	     
        }

        /* Add or move item */
        if ( gtk_widget_get_parent( panel->items[i] ) == panel->fixed ) {
            gtk_fixed_move( GTK_FIXED( panel->fixed ), 
                    panel->items[i],
                    panel->plugin_pos_x[place_here],
                    panel->plugin_pos_y[place_here] ); 
        } else if ( !gtk_widget_get_parent( panel->items[i] ) ) {
            gtk_fixed_put( GTK_FIXED( panel->fixed ), 
                       panel->items[i], 
                       panel->plugin_pos_x[place_here],
                       panel->plugin_pos_y[place_here] );

	    if (sb_is_sensitive)
      	      gtk_widget_set_sensitive (GTK_WIDGET (panel->items[i]),TRUE);
    	    else
              gtk_widget_set_sensitive (GTK_WIDGET (panel->items[i]),FALSE);
        }

        place_here++;
    }

    gtk_widget_show_all( panel->window );
    
}


/* Callback for item destroy */
static 
void destroy_item( GtkObject *object, gpointer user_data )
{   
    gint i;
    StatusBar *panel;
    GtkWidget *item;
   
    /* Return if item object or panel fails */
    if ( !object || !user_data )  return;

    panel = (StatusBar*)user_data;
    item  = (GtkWidget *)object;

    /* Find the item and destroy it */
    for( i = 0; i < HSB_MAX_NO_OF_ITEMS; ++i ) {
	    
        if( panel->items[i] == item ) {	
#if 0
            gtk_widget_destroy( GTK_WIDGET(item) );
#endif
            panel->items[i] = NULL;
            panel->item_num--; 
            arrange_items( panel, i );
            break;
        }
    }
    
}


/* Task delayed info banner, added 27092005 */
static 
gint _delayed_infobanner_add(gint32 pid, 
		             gint32 begin, 
			     gint32 timeout, 
			     const gchar *text)
{
    SBDelayedInfobanner *data;
  
    if (g_hash_table_lookup(delayed_banners, GINT_TO_POINTER(pid)) != NULL)
    {
      _remove_sb_d_ib_item(GINT_TO_POINTER(pid),
             g_hash_table_lookup(delayed_banners, 
                         GINT_TO_POINTER(pid)),
             NULL);
    }
    data = g_new0(SBDelayedInfobanner, 1);

    data->displaytime = timeout;
    data->text = g_strdup (text);
    data->timeout_onscreen_id = 0;
    data->timeout_to_show_id = g_timeout_add( (guint) begin,
                        _delayed_ib_show,
                        GINT_TO_POINTER(pid));
    
    g_hash_table_insert(delayed_banners, GINT_TO_POINTER(pid), data);
    
    
    return OSSO_OK;
}
 

gboolean 
_delayed_infobanner_remove(gpointer data)
{
    gpointer hashvalue;
    hashvalue = g_hash_table_lookup(delayed_banners, (gconstpointer)data);
    if (hashvalue == NULL)
    {
        return FALSE;
    }
    _remove_sb_d_ib_item(data, hashvalue, NULL);
    
    return FALSE;
}


gboolean 
_delayed_ib_show(gpointer data)
{ 
    SBDelayedInfobanner *info;
     
    info = g_hash_table_lookup(delayed_banners, data);
    if (info == NULL) {
        return FALSE; /* False destroys timeout */
    }  

    g_source_remove(info->timeout_to_show_id);
 
    info->timeout_to_show_id = 0;
     
    info->banner = hildon_banner_show_animation( NULL, NULL, info->text );
    info->timeout_onscreen_id = g_timeout_add(
                       (guint)info->displaytime,
                       _delayed_infobanner_remove,
                       data);

    return FALSE;
}
 

void 
_remove_sb_d_ib_item(gpointer key, gpointer value, gpointer user_data)
{
    SBDelayedInfobanner *data;    
 
    data = (SBDelayedInfobanner*)value;
    if ( data->timeout_to_show_id != 0 ) {
        g_source_remove(data->timeout_to_show_id);
    }
    else if ( data->timeout_onscreen_id != 0 ) {
        g_source_remove(data->timeout_onscreen_id);
    }
    
    if ( data->banner )
    {
        gtk_widget_destroy( data->banner );
        data->banner = NULL;
    }
     
    g_hash_table_remove(delayed_banners, (gconstpointer)key);
    g_free(data->text);
    g_free(value);
  
}

static void 
statusbar_send_signal (DBusConnection *conn, 
			HildonStatusBarItem *item,
			gboolean status)
{
  DBusMessage *message;
  gchar *item_name = NULL;
  const gchar *short_name;

  message = dbus_message_new_signal ("/com/nokia/statusbar",
                                     "com.nokia.statusbar.conditional", 
				     "update_status");

  short_name = hildon_status_bar_item_get_name (item);

  if (short_name)  
    item_name = g_strdup_printf ("lib%s.so",hildon_status_bar_item_get_name (item));
  else
    goto out;
  
  if (item_name)
  { 
    dbus_message_append_args (message,
                              DBUS_TYPE_STRING, 
  			      &item_name,
			      DBUS_TYPE_BOOLEAN,
			      &status,
                              DBUS_TYPE_INVALID);
    
    dbus_connection_send (conn, message, NULL);
  }
  
out:
  
  dbus_message_unref (message);
  g_free (item_name);
}

int 
status_bar_main( osso_context_t *osso, StatusBar **panel )
{
    StatusBar *sb_panel = NULL;
    
    TRACE(TDEBUG,"status_bar_main: 1 g_new0");
    sb_panel = g_new0(StatusBar, 1);

    sb_panel->osso = osso;

    TRACE(TDEBUG,"status_bar_main: 2 check panel ptr");
    if ( !sb_panel ) {  
        ULOG_ERR("g_new0 failed! Exiting.." );
        osso_deinitialize( osso );
	
        return OSSO_ERROR;
    }
    TRACE(TDEBUG,"status_bar_main: 3: init dock");  
    
    /* Initialize panel */
    init_dock( sb_panel );

    /* Load plugins */
    if ( !add_configured_plugins( sb_panel ) ) {
       /* If unsucceeded load permanent plugins */
        TRACE(TDEBUG,"status_bar_main: 4 not found configured "
		     "items, adding prespecified items");
        add_prespecified_items( sb_panel );
    } else {
        TRACE(TDEBUG,"status_bar_main: 4 add configured items");
    }
    
    TRACE(TDEBUG,"status_bar_main: 5 add notify for conditional items update");
    add_notify_for_configured_plugins( sb_panel );
    
    TRACE(TDEBUG,"status_bar_main: 6 gtk widget show all");
    gtk_widget_show_all( sb_panel->window );
    TRACE(TDEBUG,"status_bar_main: 7 if rpc...");
    
    /* Set RPC cb */
    if ( osso_rpc_set_default_cb_f( osso, rpc_cb, sb_panel ) != OSSO_OK ) {
        ULOG_ERR("osso_rpc_set_default_cb_f() failed");
    }
    
    delayed_banners = g_hash_table_new(g_direct_hash, g_direct_equal);
    
    TRACE(TDEBUG,"status_bar_main: 8, status bar initialized successfully");
    *panel = sb_panel;
    

    return OSSO_OK;
}


void 
status_bar_deinitialize( osso_context_t *osso, StatusBar **panel )
{
    gint i=0;
  
    StatusBar *sb_panel = (StatusBar *)(*panel);

    TRACE(TDEBUG,"status_bar_deinitialize: 1");
    for( i = 0; i < HSB_MAX_NO_OF_ITEMS; ++i ) {
        if( sb_panel->items[i] != NULL ) {
            TRACE(TDEBUG,"status_bar_deinitialize: 2 gtk widget destroy");
            gtk_widget_destroy( sb_panel->items[i] );
	}
    }
    sb_panel->item_num = 0;
  
    if( osso_rpc_unset_default_cb_f( osso, rpc_cb, sb_panel ) != OSSO_OK ) {
        ULOG_ERR("osso_rpc_unset_default_cb_f() failed");
    }
    
    TRACE(TDEBUG,"status_bar_deinitialize: 3"); 
    g_object_unref( sb_panel->log );
    gtk_widget_destroy( sb_panel->window );
    g_object_unref( sb_panel->arrow_button );
    gtk_widget_destroy( sb_panel->arrow_button );
    if ( sb_panel->popup )
        gtk_widget_destroy( sb_panel->window );
    if ( sb_panel->popup_fixed )
        gtk_widget_destroy( sb_panel->popup_fixed );
    
    TRACE(TDEBUG,"status_bar_deinitialize: 4");
    g_free( *panel );
    g_hash_table_foreach(delayed_banners, _remove_sb_d_ib_item, NULL);
    g_hash_table_destroy(delayed_banners);
    
    /* Remove notify from configuration directory */
    if ( hildon_dnotify_remove_cb(HSB_PLUGIN_USER_CONFIG_DIR) 
		    == HILDON_ERR ) {
        ULOG_WARN("FAILED to remove notify from %s.", 
		  HSB_PLUGIN_USER_CONFIG_DIR);
    }   
}


#ifndef USE_AF_DESKTOP_MAIN__

int main( int argc, char *argv[] )
{
    StatusBar *panel=NULL;
    osso_context_t *osso;
  
    TRACE(TDEBUG,"Status bar: main - this code is executed only when "
	         "Status Bar is run standalone.");
    
    gtk_init( &argc, &argv );
    
    /* Initialize osso */
    osso = osso_initialize( HILDON_STATUS_BAR_NAME, 
                            HILDON_STATUS_BAR_VERSION, FALSE, NULL );
    
    if ( !osso ) {  
        ULOG_ERR("osso_initialize failed! Exiting..");
        return OSSO_ERROR;
    }
  
    status_bar_main(osso, &panel); 
    gtk_main();
    status_bar_deinitialize(osso, &panel);  
    osso_deinitialize( osso );
  
    return OSSO_OK;
}

#endif
