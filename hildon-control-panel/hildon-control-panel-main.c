/*
 * This file is part of hildon-control-panel
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
 
#include "hildon-control-panel-main.h"

/* For logging functions */
#include <log-functions.h>

#  define dprint(f, a...)

static const gchar *user_home_dir;


typedef struct _AppData AppData;

static GtkCheckMenuItem *cp_view_small_icons;
static GtkCheckMenuItem *cp_view_large_icons;

struct _AppData
{
    HildonApp *app;
    HildonAppView *appview;
    GtkWidget * grid;
    GtkWindow *window;
    osso_context_t *osso;

    /* for state save data */
    gint icon_size;
    gchar * focused_filename;
    gchar * saved_focused_filename;
    gint scroll_value;
    gint execute;
};


AppData state_data;

/* delay to wait from callback to actually reading the entries in msecs*/
#define DIRREADDELAY 500

/* Message bus definitions */
/* Definitions for password validation call */
#define HILDON_CP_DBUS_MCE_SERVICE "com.nokia.mce"
#define HILDON_CP_DBUS_MCE_REQUEST_IF "com.nokia.mce.request"
#define HILDON_CP_DBUS_MCE_REQUEST_PATH "/com/nokia/mce/request"
#define HILDON_CP_MCE_PASSWORD_VALIDATE "validate_devicelock_code"
#define HILDON_CP_DEFAULT_SALT "$1$JE5Gswee$"

/* Definitions for appkiller call */
#define HILDON_CP_APPL_DBUS_NAME "osso_app_killer"
#define HILDON_CP_SVC_NAME "com.nokia." HILDON_CP_APPL_DBUS_NAME
#define HILDON_CP_RFS_SHUTDOWN_OP "/com/nokia/" \
 HILDON_CP_APPL_DBUS_NAME "/rfs_shutdown"
#define HILDON_CP_RFS_SHUTDOWN_IF HILDON_CP_SVC_NAME ".rfs_shutdown"
#define HILDON_CP_RFS_SHUTDOWN_MSG "shutdown"

/* Definitions for mobile operator wizard launch*/
#define HILDON_CONTROL_PANEL_DBUS_OPERATOR_WIZARD_SERVICE "operator_wizard"
#define HILDON_CONTROL_PANEL_OPERATOR_WIZARD_LAUNCH "launch_operator_wizard"

#define HILDON_CONTROL_PANEL_STATEFILE_NO_CONTENT "EOF"

/* Sets DNOTIFY callback for the .desktop dir */
static int _init_dnotify(gchar * path, gchar *path_additional );
static void _dnotify_callback_f(gchar * path, gpointer data);


/* Create the GUI entries */
static void _init_gui (void);
/* Set the grid to use large/small icons */
static void _large_icons(void);
static void _small_icons(void);

/* Make a DBUS system call.  When RETVAL is non-NULL, it needs to be
   freed with osso_rpc_free_val.  */
static osso_return_t _cp_run_rpc_system_request (osso_context_t *osso, 
                                                 const gchar *service, 
                                                 const gchar *object_path,
                                                 const gchar *interface, 
                                                 const gchar *method,
                                                 osso_rpc_t *retval, 
                                                 int first_arg_type,
                                                 ...);

/* When RETVAL is non-NULL, it needs to be freed with
   osso_rpc_free_val.
*/
static osso_return_t _rpc_system_request (osso_context_t *osso, 
                                          DBusConnection *conn,
                                          const gchar *service, 
                                          const gchar *object_path,
                                          const gchar *interface, 
                                          const gchar *method,
                                          osso_rpc_t *retval, 
                                          int first_arg_type,
                                          va_list var_args);
static void _append_args(DBusMessage *msg, int type, va_list var_args);
static void _get_arg(DBusMessageIter *iter, osso_rpc_t *retval);

/* Create & free state data */
static void _create_data(void);
static void _destroy_data( void );
/* State saving functions */
static void _save_state( gboolean clear_state );
static void _retrieve_state(void);
static void _enforce_state(void);

/* Configuration saving functions */
static void _save_configuration( gpointer data );
static void _retrieve_configuration( void );

/* Keylistener */
static gint _cp_keyboard_listener(GtkWidget *widget, 
                                  GdkEventKey * keyevent, gpointer data);

/* Callback for topping */
static void _topmost_status_acquire(void);
/* Callback for saving state */
static void _topmost_status_lose(void);
/* Callback for resetting factory settings */
static gboolean _reset_factory_settings( GtkWidget *widget, gpointer data );
/* Callback for operator wizard launch */
static void _run_operator_wizard( GtkWidget *widget, gpointer data );
/* Callback for help function */
static void _launch_help( GtkWidget *widget, gpointer data );
/* Callback for menu item small/large icons */
static void _iconsize( GtkWidget *widget, gpointer data );
static void _my_open( GtkWidget *widget, gpointer data );
static void _my_quit( GtkWidget *widget, gpointer data );


/* Callback for launching an item */
static void _launch (MBDotDesktop *, gpointer, gboolean);
static void _focus_change (MBDotDesktop *, gpointer);

/* Callback for handling HW signals */
static void _hw_signal_cb( osso_hw_state_t *state, gpointer data );


/**********************************************************************
 * Main program
 **********************************************************************/
int 
main(int argc, char ** argv)
{

    setlocale( LC_ALL, "" );
    
    bindtextdomain( PACKAGE, LOCALEDIR );

    bind_textdomain_codeset( PACKAGE, "UTF-8" );

    textdomain( PACKAGE );
     
    _init_dnotify( CONTROLPANEL_ENTRY_DIR, 
                   g_getenv(ADDITIONAL_CP_APPLETS_DIR_ENVIRONMENT) == NULL ?
                   NULL :
                   (char *)(g_getenv(ADDITIONAL_CP_APPLETS_DIR_ENVIRONMENT)));

    /* We can cast this and lose the const keyword because dnotify just
       strdups the string anyway. */

    _create_data();       /* Create state data */

    _retrieve_configuration(); /*configuration from disk */

    _retrieve_state();    /* State data from disk */
    
    gtk_init( &argc, &argv );
    
    _init_gui();  /* Create GUI components */

    _enforce_state();     /* realize the saved state */

    gtk_widget_show_all( GTK_WIDGET( state_data.app ) );

    if(state_data.execute==1) {
        _launch(hildon_cp_applist_get_entry(state_data.focused_filename),
                NULL, FALSE );
    }
    
    gtk_main();


    _destroy_data( );
    
    return 0;
}

/**********************************************************************
 * DNotify stuff
 **********************************************************************/

static int callback_pending = 0;

/* called by dnotify_callback_f after a timeout (to prevent exessive
   callbacks. */

static gboolean _dnotify_reread_desktop_entries()
{
    callback_pending = 0;
    hildon_cp_applist_reread_dot_desktops();
    return FALSE; /* do not have the timeout called again */
}

static void _dnotify_callback_f(char * path,gpointer data)
{
    if(!callback_pending)
    {
        callback_pending = 1;
        g_timeout_add( DIRREADDELAY, 
                       (GSourceFunc)_dnotify_reread_desktop_entries,
                       path );
    }
}

static int _init_dnotify(gchar * path, gchar * path_additional )
{
  hildon_return_t ret;

  ret = hildon_dnotify_handler_init();
  if( ret!=HILDON_OK )
  {
      return ret;
  }
  ret = hildon_dnotify_set_cb( _dnotify_callback_f, path, NULL);
                               
  if( ret!=HILDON_OK )
  {
      return ret;
  }
  if (path_additional !=NULL)
  {
      ret = hildon_dnotify_set_cb (_dnotify_callback_f, path_additional,
                                   NULL);
  }
  return HILDON_OK;
}



/**********************************************************************
 * GUI helper functions
 **********************************************************************/

static void _large_icons(void)
{
    if(state_data.grid)
        hildon_grid_set_style(HILDON_GRID(state_data.grid),
                              "largeicons-cp");
}


static void _small_icons(void)
{
    if(state_data.grid)
        hildon_grid_set_style(HILDON_GRID(state_data.grid),
                              "smallicons-cp");

}

static void _init_gui(void)
{
    GtkMenu *menu=NULL;
    GtkWidget *sub_view=NULL;
    GtkWidget *sub_tools=NULL;
    GtkWidget *mi = NULL;
    GSList *menugroup = NULL;
    GtkWidget *grid = NULL;

    state_data.app = HILDON_APP( hildon_app_new() );

    hildon_app_set_title( state_data.app, HILDON_CONTROL_PANEL_TITLE );

    hildon_app_set_two_part_title( state_data.app, FALSE );

    g_signal_connect( G_OBJECT( state_data.app ), "destroy", 
                      G_CALLBACK( _my_quit ),NULL);

    state_data.appview = HILDON_APPVIEW( hildon_appview_new(
                                             "MainView" ) );

    hildon_app_set_appview( state_data.app, state_data.appview );
    
    g_signal_connect(G_OBJECT(state_data.app), "topmost_status_lose",
                     G_CALLBACK(_topmost_status_lose), NULL);
    g_signal_connect(G_OBJECT(state_data.app), "topmost_status_acquire",
                     G_CALLBACK(_topmost_status_acquire), NULL);
    
    /* Menu */
    menu = GTK_MENU( hildon_appview_get_menu( state_data.appview ) );

    mi = gtk_menu_item_new_with_label( HILDON_CONTROL_PANEL_MENU_OPEN );
    
    g_signal_connect( G_OBJECT( mi ), "activate", 
                      G_CALLBACK( _my_open ), NULL ); 

    gtk_menu_shell_append( GTK_MENU_SHELL( menu ), mi ); 
    
    
    /* View submenu */
    sub_view = gtk_menu_new();

    mi = gtk_menu_item_new_with_label( HILDON_CONTROL_PANEL_MENU_SUB_VIEW );
    
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), sub_view);

    gtk_menu_shell_append( GTK_MENU_SHELL( menu ), mi );
        
    /* Small icon size */
    mi = gtk_radio_menu_item_new_with_label
        ( menugroup, HILDON_CONTROL_PANEL_MENU_SMALL_ITEMS );
    menugroup = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM (mi) );
    cp_view_small_icons = GTK_CHECK_MENU_ITEM(mi);

    if(state_data.icon_size == 0) {
        gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(mi), TRUE);
    }


    gtk_menu_shell_append( GTK_MENU_SHELL( sub_view ), mi );

    
    g_signal_connect( G_OBJECT( mi ), "activate", G_CALLBACK(_iconsize),
                      "small");

    /* Large icon size */
    mi = gtk_radio_menu_item_new_with_label
        ( menugroup,
          HILDON_CONTROL_PANEL_MENU_LARGE_ITEMS );
    cp_view_large_icons = GTK_CHECK_MENU_ITEM(mi);

    if (state_data.icon_size == 1) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), TRUE);
    }
    menugroup = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM (mi) );
   
    gtk_menu_shell_append( GTK_MENU_SHELL( sub_view ), mi );
    
    g_signal_connect( G_OBJECT( mi ), "activate", G_CALLBACK(_iconsize),
                      "large");



    /* Tools submenu*/
    sub_tools = gtk_menu_new();

    mi = gtk_menu_item_new_with_label( HILDON_CONTROL_PANEL_MENU_SUB_TOOLS);
    
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), sub_tools);

    gtk_menu_shell_append( GTK_MENU_SHELL( menu ), mi );


    /* Run operator wizard */
    mi = gtk_menu_item_new_with_label
        ( HILDON_CONTROL_PANEL_MENU_SETUP_WIZARD);

    gtk_menu_shell_append( GTK_MENU_SHELL( sub_tools ), mi );
    
    g_signal_connect( G_OBJECT( mi ), "activate", 
                      G_CALLBACK(_run_operator_wizard), NULL);

    /* Reset Factory Settings */
    mi = gtk_menu_item_new_with_label
        ( HILDON_CONTROL_PANEL_MENU_RESET_SETTINGS);

    gtk_menu_shell_append( GTK_MENU_SHELL( sub_tools ), mi );
    
    g_signal_connect( G_OBJECT( mi ), "activate", 
                      G_CALLBACK(_reset_factory_settings), NULL);

    
    /* Help! */

    mi = gtk_menu_item_new_with_label
        ( HILDON_CONTROL_PANEL_MENU_HELP);

    gtk_menu_shell_append( GTK_MENU_SHELL( sub_tools ), mi );
    
    g_signal_connect( G_OBJECT( mi ), "activate", 
                      G_CALLBACK(_launch_help), NULL);

    /* close */
    mi = gtk_menu_item_new_with_label( HILDON_CONTROL_PANEL_MENU_CLOSE);
    
    g_signal_connect(GTK_OBJECT(mi), "activate",
                     G_CALLBACK(_my_quit),
                     NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    
    gtk_widget_show_all( GTK_WIDGET( menu ));
    
    hildon_cp_applist_initialize( _launch, NULL,
                                    _focus_change, NULL,
                                    GTK_WIDGET( state_data.appview ), 
                                    CONTROLPANEL_ENTRY_DIR);

    grid = hildon_cp_applist_get_grid();
    {
        GValue val = {0,};
        g_value_init(&val, G_TYPE_STRING);
        g_value_set_string(&val, "");
        g_object_set_property(G_OBJECT (grid), "empty_label", &val);
	g_value_unset(&val);
    }

    gtk_widget_show_all(GTK_WIDGET(state_data.app));

    state_data.window = GTK_WINDOW( gtk_widget_get_toplevel(
                                        GTK_WIDGET( state_data.app ) ) );

    state_data.grid = hildon_cp_applist_get_grid();

    /* Set the callback function for HW signals */
    osso_hw_set_event_cb(state_data.osso, NULL , _hw_signal_cb, NULL );

    /* Set the keyboard listening callback */

    gtk_widget_add_events(GTK_WIDGET(state_data.window), 
                          GDK_BUTTON_RELEASE_MASK);

    g_signal_connect(G_OBJECT(state_data.window), "key_release_event",
                     G_CALLBACK(_cp_keyboard_listener), NULL);



}


/************************************************************************
 * Application state data handling
 ************************************************************************/

/* Create a struct for the application state data */

static void _create_data(void)
{
    state_data.osso = osso_initialize(APP_NAME, APP_VERSION, TRUE, NULL);
    if(!state_data.osso)
    {
        osso_log(LOG_ERR,
                "Error initializing osso -- check that D-BUS is running");
        exit(-1);
    }
    
    g_assert(state_data.osso);
    state_data.icon_size=1;
    state_data.focused_filename=NULL;
    state_data.saved_focused_filename=NULL;
    state_data.execute = 0;
    state_data.scroll_value = 0;
}

/* Clean the application state data struct */

static void _destroy_data( void )
{
    if( state_data.osso )      
        osso_deinitialize(state_data.osso);
    if( state_data.focused_filename )   
        g_free( state_data.focused_filename );
    if( state_data.saved_focused_filename )  
        g_free( state_data.saved_focused_filename );
}

/* save state data on disk */

static void _save_state( gboolean clear_state ) {
    gint scrollvalue = hildon_grid_get_scrollbar_pos(HILDON_GRID(
                                                        state_data.grid));
    gchar contents[HILDON_CONTROL_PANEL_STATEFILE_MAX];
    gint fd, ret;

    if (clear_state) {
        ret=g_sprintf(contents, "%s\n",
                       HILDON_CONTROL_PANEL_STATEFILE_NO_CONTENT);
    }
    else 
    {
        if (state_data.focused_filename != NULL) 
	  {
  	      osso_cp_plugin_save_state(
		 		        state_data.osso,
					state_data.focused_filename,
					NULL
					);
	  }
        ret = g_snprintf(contents, HILDON_CONTROL_PANEL_STATEFILE_MAX,
                         "focused=%s\n"
                         "scroll=%d\n"
                         "execute=%d\n",
                         state_data.focused_filename != NULL ?
                         state_data.focused_filename : "",
                         scrollvalue,
                         state_data.execute);
    }
    if(ret > HILDON_CONTROL_PANEL_STATEFILE_MAX)
    {
        osso_log(LOG_ERR, "Error saving state -- too long state");
        return;
    }   

    fd = osso_state_open_write(state_data.osso);
    if( fd == -1)
    {
        osso_log(LOG_ERR, "Opening state file for writing failed");
        return;
    }

    ret = write(fd, contents, strlen(contents));
    if(ret != strlen(contents))
    {
        osso_log(LOG_ERR, 
      "Writing state save file failed. Tried to write %d bytes, wrote %d",
                 strlen(contents), ret);
    }
    osso_state_close(state_data.osso, fd);
}

/* Read the saved state from disk */

static void _retrieve_state( void ) {
    gchar buff[HILDON_CONTROL_PANEL_STATEFILE_MAX];
    size_t rc;
    gint fd;
    gchar *eq;
    gchar *nl;
    gchar *start;
    gint length;

    fd = osso_state_open_read(state_data.osso);
    if( fd == -1)
    {
        return;
    }

    rc = read(fd, buff, HILDON_CONTROL_PANEL_STATEFILE_MAX);
    osso_state_close(state_data.osso, fd);

    if(rc >= HILDON_CONTROL_PANEL_STATEFILE_MAX)
    {
        osso_log(LOG_ERR,
                 "Error retrieving state -- too long state file %d", rc);
        return;
    }   
   
    start = buff;
    length = rc;
    while( ((nl = g_strstr_len(start, length, "\n")) != NULL) &&
           ((eq = g_strstr_len(start, length, "=")) != NULL)) {
        *nl = '\0';
        *eq = '\0';
        
        if(strcmp(start, "focused")==0) {
            state_data.saved_focused_filename = g_strdup(eq+1);
        } else if(strcmp(start, "scroll")==0) {
            sscanf(eq+1, "%d", &state_data.scroll_value);
        } else if(strcmp(start, "execute")==0) {
            sscanf(eq+1, "%d", &state_data.execute);
        } else if (strcmp(start, HILDON_CONTROL_PANEL_STATEFILE_NO_CONTENT)==0) {
            return;
        } else {
            osso_log(LOG_ERR, 
                     "Error retrieving state -- unknown field %s", start);
        }
        
        length = length - (nl - start + 1);
        start=nl+1;
    }
}

static void _enforce_state(void)
{
    /* Actually enforce the saved state */
    
    if (state_data.icon_size == 0) 
    {
        _small_icons();
    }
    else if (state_data.icon_size == 1)
    {
        _large_icons();
    } 
    else 
    {
        osso_log(LOG_ERR, "Unknown iconsize");
    }

    if(state_data.saved_focused_filename)
    {
        hildon_cp_applist_focus_item( state_data.saved_focused_filename );
 
        g_free(state_data.saved_focused_filename);
        state_data.saved_focused_filename = NULL;
    }

    hildon_grid_set_scrollbar_pos(HILDON_GRID(state_data.grid),
                                  state_data.scroll_value);
    /* main() will start the possible plugin in state_data.execute*/
}


/* Save the configuration file (large/small icons)  */

static void _save_configuration(gpointer data)
{

    FILE *fp;
    gchar *configure_file;

    if (data == NULL)
        return;

    g_return_if_fail (user_home_dir != NULL);

    /* user_home_dir is fetched from environment variables in 
       _retrieve_configuration, which is executed during cp init */

    configure_file = 
        g_build_path("/", user_home_dir, HILDON_CP_SYSTEM_DIR,
                     HILDON_CP_CONF_USER_FILENAME, NULL);
        
    fp = g_fopen (configure_file, "w");
    if(fp == NULL) 
    {
        osso_log(LOG_ERR, "Couldn't open configure file %s", 
                 configure_file);
    } else 
    {
        if(fprintf(fp, "iconsize=%s",
                   (gchar *)data)
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

static void _retrieve_configuration()
{
    FILE *fp;
    gchar *configure_file;    
    gchar *system_dir;
    gchar iconsize_data[HILDON_CP_MAX_FILE_LENGTH];
    gboolean nonexistent = TRUE;
    struct stat buf;
 
    iconsize_data[0] = 0;
    iconsize_data[HILDON_CP_MAX_FILE_LENGTH-1]= 0;

    user_home_dir = g_getenv(HILDON_ENVIRONMENT_USER_HOME_DIR);

    system_dir = 
        g_build_path("/", user_home_dir, HILDON_CP_SYSTEM_DIR, NULL);

    g_return_if_fail (system_dir != NULL);

    if (stat(system_dir, &buf) == 0) 
    {
        if(S_ISDIR(buf.st_mode))
        {
            nonexistent = FALSE;
        }
    }

    if(nonexistent)
    {
        if(mkdir (system_dir, HILDON_CP_SYSTEM_DIR_ACCESS) != 0) 
        {
            osso_log(LOG_ERR, "Couldn't create directory %s", system_dir);
        }
        g_free(system_dir);
        return;
    }

    g_free(system_dir);
    
    configure_file = 
        g_build_path("/", user_home_dir, HILDON_CP_SYSTEM_DIR,
                     HILDON_CP_CONF_USER_FILENAME, NULL);

    fp = fopen (configure_file, "r");

    if(fp == NULL) 
    {
        osso_log(LOG_ERR, "Couldn't open configure file %s", 
                 configure_file);
    } else 
    {
        if(fscanf(fp,
                  HILDON_CP_CONF_FILE_FORMAT,
                  iconsize_data) 
           <0)
        {
            osso_log(LOG_ERR, 
                     "Couldn't load statusinfo from configure file");
        }
        else {
            g_return_if_fail (iconsize_data != NULL);

            if( strcmp((gchar *)iconsize_data, "large") == 0)
            {
                state_data.icon_size = 1;
            }
            else if( strcmp((gchar *)iconsize_data, "small") == 0)
            {
                state_data.icon_size = 0;
            }
            else {
                state_data.icon_size = 1;
            }
        }
    }
    
    if(fp != NULL && fclose(fp) != 0) 
    {
        osso_log(LOG_ERR, "Couldn't close configure file %s", 
                 configure_file);
    }

    g_free(configure_file);       

}

/***********************************************************************
 * Callbacks
 ***********************************************************************/

/***** Keyboard listener *******/

static gint _cp_keyboard_listener( GtkWidget * widget, 
                                   GdkEventKey * keyevent,
                                   gpointer data)
{
    if (keyevent->type==GDK_KEY_RELEASE) 
    {
        if (keyevent->keyval == HILDON_HARDKEY_INCREASE) {
            if (state_data.icon_size != 1) {
                state_data.icon_size = 1;
                gtk_check_menu_item_set_active 
                    (cp_view_large_icons, TRUE);
                _large_icons();
                _save_configuration ((gpointer)"large");
                return TRUE;
            }

        }
        else if (keyevent->keyval == HILDON_HARDKEY_DECREASE) {
            if (state_data.icon_size != 0) {
                state_data.icon_size = 0;
                gtk_check_menu_item_set_active 
                    (cp_view_small_icons, TRUE);
                _small_icons();
                _save_configuration((gpointer)"small");
                return TRUE;
            }
        }
    }
        return FALSE;

}

/***** Top/Untop callbacks *****/

/* save state when we get untopped*/
static void _topmost_status_lose(void)
{
    _save_state(FALSE);
    hildon_app_set_killable(state_data.app, TRUE);
}

/* Remove the "save to OOM kill" hint when topped */

static void _topmost_status_acquire(void)
{
    mb_util_window_activate(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(
                                GTK_WIDGET(state_data.app)->window));
    hildon_app_set_killable(state_data.app, FALSE);
}


/****** Menu callbacks ******/

static void _my_open( GtkWidget *widget, gpointer data )
{
    _launch( hildon_cp_applist_get_entry( state_data.focused_filename ),
             NULL, TRUE );
}

static void _my_quit(GtkWidget *widget, gpointer data)
{
    hildon_app_set_killable(state_data.app, FALSE);
    _save_state( TRUE );
    gtk_main_quit();
}

static void _iconsize( GtkWidget *widget, gpointer data )
{
    if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
        return;
    
    /* Avoid instant segfault, but can stuff be left uninitialized?
       Hope not. */
    if (data == NULL)
        return;

    _save_configuration(data);
    
    if( strcmp((gchar *)data, "large") == 0)
    {
        _large_icons();
        state_data.icon_size = 1;
    }
    else if( strcmp((gchar *)data, "small") == 0)
    {
        _small_icons();
        state_data.icon_size = 0;
    }
}

static void _run_operator_wizard( GtkWidget *widget, gpointer data)
{

    osso_rpc_t returnvalues;
    osso_return_t returnstatus;
    returnstatus = osso_rpc_run_with_defaults
        (state_data.osso, HILDON_CONTROL_PANEL_DBUS_OPERATOR_WIZARD_SERVICE,
         HILDON_CONTROL_PANEL_OPERATOR_WIZARD_LAUNCH,
         &returnvalues, 
         DBUS_TYPE_INVALID);    

    
    switch (returnstatus)
    {
        case OSSO_OK:
            break;
        case OSSO_INVALID:
            osso_log(LOG_ERR, 
                     "Invalid parameter in operator_wizard launch");
            break;
        case OSSO_RPC_ERROR:
        case OSSO_ERROR:
        case OSSO_ERROR_NAME:
        case OSSO_ERROR_NO_STATE:
        case OSSO_ERROR_STATE_SIZE:
            if (returnvalues.type == DBUS_TYPE_STRING) {
                osso_log(LOG_ERR, "Operator wizard launch failed: %s\n", 
                         returnvalues.value.s);
            }
            else {
                osso_log(LOG_ERR, 
                         "Operator wizard launch failed, unspecified");
            }
            break;            
        default:
            osso_log(LOG_ERR, "Unknown error type %d", returnstatus);
    }
    
    osso_rpc_free_val (&returnvalues);
}

static gboolean _reset_factory_settings( GtkWidget *widget, gpointer data )
{

    gint i, retry;
    gchar *password;
    gchar *crypted_password;
    GtkWidget *confirm_dialog;
    gchar *dollarsign = "$";
    osso_rpc_t returnvalues;
    osso_return_t returnstatus;

    retry=1;

    if ( data == NULL ) { /* In retries, we don't confirm again  */
        
        confirm_dialog = hildon_note_new_confirmation
            (GTK_WINDOW(state_data.window), 
             RESET_FACTORY_SETTINGS_INFOBANNER);
        
        hildon_note_set_button_texts
            (HILDON_NOTE(confirm_dialog),
             RESET_FACTORY_SETTINGS_INFOBANNER_CLOSE_ALL,
             RESET_FACTORY_SETTINGS_INFOBANNER_CANCEL);
        
        gtk_widget_show_all(confirm_dialog);
        i = gtk_dialog_run(GTK_DIALOG(confirm_dialog));

        gtk_widget_destroy(GTK_WIDGET(confirm_dialog));

        if (i == GTK_RESPONSE_CANCEL ||
            i == GTK_RESPONSE_DELETE_EVENT) {
            return TRUE;    
        }

    }
    else {
        retry = GPOINTER_TO_INT(data) +1;
    }
    
    GtkWidget *pw_dialog = (hildon_get_password_dialog_new(
                                GTK_WINDOW(state_data.window), FALSE));

    hildon_get_password_dialog_set_title
        (HILDON_GET_PASSWORD_DIALOG(pw_dialog), 
         RESET_FACTORY_SETTINGS_PASSWORD_DIALOG_TITLE);

    hildon_get_password_dialog_set_caption
        (HILDON_GET_PASSWORD_DIALOG(pw_dialog), 
         RESET_FACTORY_SETTINGS_PASSWORD_DIALOG_CAPTION);
/*    
    hildon_get_password_dialog_set_max_characters
        (HILDON_GET_PASSWORD_DIALOG(pw_dialog),
         HILDON_CP_ASSUMED_LOCKCODE_MAXLENGTH);

         open issue: widget spec just dropped max characters altogether.
         retaining code, but as the logical string no longer exits...
*/
    g_object_set( G_OBJECT(pw_dialog),
                  "numbers_only", NULL, NULL );
    

    gtk_widget_show (pw_dialog);
    
    switch(gtk_dialog_run (GTK_DIALOG(pw_dialog)))
    {
      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_DELETE_EVENT:

        if (pw_dialog !=NULL)
            gtk_widget_destroy(GTK_WIDGET(pw_dialog));        
        return TRUE;
    }  

    password = strdup(hildon_get_password_dialog_get_password
                      (HILDON_GET_PASSWORD_DIALOG(pw_dialog)));
    
    if (password == NULL || strcmp(password,"") == 0) {        

        gtk_infoprint (GTK_WINDOW(state_data.window), 
                       RESET_FACTORY_SETTINGS_IB_NO_LOCKCODE );
        if (pw_dialog)
        {            
            gtk_widget_destroy(GTK_WIDGET(pw_dialog));
        }
        if (password != NULL) 
        {
            g_free (password);
        }
        _reset_factory_settings(widget, GINT_TO_POINTER(retry));
        return TRUE;
    }

    crypted_password = crypt(password, HILDON_CP_DEFAULT_SALT);
    g_free(password);

    gint c = (gint)dollarsign[0];

    password = rindex (crypted_password, c)+1;

    returnstatus = _cp_run_rpc_system_request
        (state_data.osso, 
         HILDON_CP_DBUS_MCE_SERVICE, 
         HILDON_CP_DBUS_MCE_REQUEST_PATH,
         HILDON_CP_DBUS_MCE_REQUEST_IF,
         HILDON_CP_MCE_PASSWORD_VALIDATE,
         &returnvalues, 
         DBUS_TYPE_STRING,
         password,
         DBUS_TYPE_STRING,
         HILDON_CP_DEFAULT_SALT,
         DBUS_TYPE_INVALID);
     
    password = NULL; /* password pointed to middle of crypted password */

    switch (returnstatus)
    {
    case OSSO_INVALID:
        osso_log(LOG_ERR, "Invalid parameter\n");
        fprintf(stderr, "got invalid");
        break;
    case OSSO_RPC_ERROR:
    case OSSO_ERROR:
    case OSSO_ERROR_NAME:
    case OSSO_ERROR_NO_STATE:
    case OSSO_ERROR_STATE_SIZE:
        if (returnvalues.type == DBUS_TYPE_STRING) {
            osso_log(LOG_ERR, "Lockcode query call failed: %s\n", 
                     returnvalues.value.s);
        }
        else {
            osso_log(LOG_ERR, 
                     "Lockcode query call failed, unspecified");
        }
        break;
        
    case OSSO_OK:
        break;
    default:
        osso_log(LOG_ERR, "Unknown error type %d", returnstatus);
    }
    
    
    if (returnvalues.type == DBUS_TYPE_BOOLEAN &&
        returnvalues.value.b == TRUE) { 

        osso_rpc_free_val (&returnvalues);
        returnstatus = osso_rpc_run(state_data.osso, 
                                    HILDON_CP_SVC_NAME, 
                                    HILDON_CP_RFS_SHUTDOWN_OP,
                                    HILDON_CP_RFS_SHUTDOWN_IF, 
                                    HILDON_CP_RFS_SHUTDOWN_MSG,
                                    &returnvalues, 
                                    DBUS_TYPE_INVALID);
        
        

        switch (returnstatus)
        {
        case OSSO_INVALID:
            osso_log(LOG_ERR, "Invalid parameter\n");
            break;
        case OSSO_RPC_ERROR:
        case OSSO_ERROR:
        case OSSO_ERROR_NAME:
        case OSSO_ERROR_NO_STATE:
        case OSSO_ERROR_STATE_SIZE:
            if (returnvalues.type == DBUS_TYPE_STRING) {
                osso_log(LOG_ERR, "Appkiller call failed: %s\n", 
                         returnvalues.value.s);
            }
            else {
                osso_log(LOG_ERR, 
                         "Appkiller call failed, unspecified");
            }
            break;
            
        case OSSO_OK:
	    osso_rpc_free_val (&returnvalues);
            return TRUE;
            /* There is no real need for case OSSO_OK: we'll be dead
               ... if everyhting goes as planned, anyway! */
        default:
            osso_log(LOG_ERR, "Unknown error type %d", returnstatus);
        }

    }
    else if (returnvalues.type == DBUS_TYPE_BOOLEAN &&
             returnvalues.value.b == FALSE) { 
        
        gtk_infoprint (GTK_WINDOW(state_data.window), 
                       RESET_FACTORY_SETTINGS_IB_WRONG_LOCKCODE );
        gtk_widget_destroy(GTK_WIDGET(pw_dialog));
        _reset_factory_settings(widget, GINT_TO_POINTER(retry));
	osso_rpc_free_val (&returnvalues);
        return TRUE;
    }

    if (pw_dialog)
        gtk_widget_destroy(GTK_WIDGET(pw_dialog));

    osso_rpc_free_val (&returnvalues);
    return TRUE;
}

static void _launch_help( GtkWidget *widget, gpointer data)
{
    

    osso_return_t help_ret;
    
    help_ret = ossohelp_show(state_data.osso, OSSO_HELP_ID_CONTROL_PANEL, 0);



    switch (help_ret)
     {
         case OSSO_OK:
             break;
         case OSSO_ERROR:
             g_warning("HELP: ERROR (No help for such topic ID)\n");
             break;
         case OSSO_RPC_ERROR:
             g_warning("HELP: RPC ERROR (RPC failed for HelpApp/Browser)\n");
             break;
         case OSSO_INVALID:
             g_warning("HELP: INVALID (invalid argument)\n");
             break;
         default:
             g_warning("HELP: Unknown error!\n");
             break;
     }

}

static void _hw_signal_cb( osso_hw_state_t *state, gpointer data )
{
	if ( state != NULL ) {
		
		/* Shutdown */
		if ( state->shutdown_ind ) {
			/* Save state */
			_save_state(FALSE);
			/* Quit */
			_my_quit( NULL, NULL );
		}
		
	} else {
		/* Do we care about this? */
	}

	return;
}


/***** Applist callbacks *****/

static void _launch(MBDotDesktop* dd, gpointer data, gboolean user_activated )
{
    int ret;

    if(dd) {
        state_data.execute=1;
        /* FIXME: What if mb_dotdesktop_get returns NULL? --> Segfault */
        ret = osso_cp_plugin_execute( state_data.osso, 
                                      (char *)mb_dotdesktop_get(
                                          dd, "X-control-panel-plugin"),
                                      (gpointer)state_data.window,
				      user_activated );
        state_data.execute = 0;
    }
}

static void _focus_change(MBDotDesktop* dd, gpointer data )
{
    if(dd)
    {
        if(state_data.focused_filename)
            g_free(state_data.focused_filename);
        state_data.focused_filename = g_strdup(
            mb_dotdesktop_get_filename(dd));
    }
}

/***** DBUS system calls *****/



static osso_return_t _cp_run_rpc_system_request (osso_context_t *osso, 
                                                 const gchar *service, 
                                                 const gchar *object_path,
                                                 const gchar *interface, 
                                                 const gchar *method,
                                                 osso_rpc_t *retval, 
                                                 int first_arg_type, ...)
{    
    osso_return_t ret;
    va_list arg_list;
    
    if( (osso == NULL) || (service == NULL) || (object_path == NULL) ||
	(interface == NULL) || (method == NULL))
	return OSSO_INVALID;
    
    va_start(arg_list, first_arg_type);
    
    ret = _rpc_system_request(osso, 
                              (DBusConnection*)osso_get_sys_dbus_connection
                              (osso), 
                              service, 
                              object_path, interface,
                              method, retval, first_arg_type, arg_list);
    va_end(arg_list);
    return ret;
}

static osso_return_t _rpc_system_request (osso_context_t *osso, 
                                          DBusConnection *conn,
                                          const gchar *service, 
                                          const gchar *object_path,
                                          const gchar *interface, 
                                          const gchar *method,
                                          osso_rpc_t *retval, 
                                          int first_arg_type,
                                          va_list var_args)
{
    gint timeout;
    DBusMessage *msg;
    DBusError err;
    DBusMessage *ret;
    osso_return_t timeout_retval;

    if(conn == NULL)
	return OSSO_INVALID;
    
    dprint("New method: %s%s::%s:%s",service,object_path,interface,method);
    msg = dbus_message_new_method_call(service, object_path,
				       interface, method);
    if(msg == NULL)
    {
	return OSSO_ERROR;
    }


    if(first_arg_type != DBUS_TYPE_INVALID)
	_append_args(msg, first_arg_type, var_args);
    
    dbus_message_set_auto_start(msg, FALSE);

    dbus_error_init(&err);
    timeout_retval = osso_rpc_get_timeout(osso, &timeout);
    ret = dbus_connection_send_with_reply_and_block(conn, msg, 
                                                    timeout,
                                                    &err);
    if (!ret) {
        dprint("Error return: %s",err.message);
        retval->type = DBUS_TYPE_STRING;
        retval->value.s = g_strdup(err.message);
        dbus_error_free(&err);
        return OSSO_RPC_ERROR;
    }
    else {
        DBusMessageIter iter;
        DBusError err;

        switch(dbus_message_get_type(ret)) {
        case DBUS_MESSAGE_TYPE_ERROR:
            dbus_set_error_from_message(&err, ret);
            retval->type = DBUS_TYPE_STRING;
            retval->value.s = g_strdup(err.message);
            dbus_error_free(&err);
            return OSSO_RPC_ERROR;
        case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            dprint("message return");
            dbus_message_iter_init(ret, &iter);
	    
            _get_arg(&iter, retval);
            
            dbus_message_unref(ret);
            
            return OSSO_OK;
        default:
            retval->type = DBUS_TYPE_STRING;
            retval->value.s = g_strdup("Invalid return value");
            return OSSO_RPC_ERROR;
        }
    }
}    


static void _append_args(DBusMessage *msg, int type, va_list var_args)
{
    while(type != DBUS_TYPE_INVALID) {
	gchar *s;
	gint32 i;
	guint32 u;
	gdouble d;
	gboolean b;
	
	switch(type) {
	  case DBUS_TYPE_INT32:
	    i = va_arg(var_args, gint32);
	    dbus_message_append_args(msg, type, i, DBUS_TYPE_INVALID);
	    break;
	  case DBUS_TYPE_UINT32:
	    u = va_arg(var_args, guint32);
	    dbus_message_append_args(msg, type, u, DBUS_TYPE_INVALID);
	    break;
	  case DBUS_TYPE_BOOLEAN:
	    b = va_arg(var_args, gboolean);
	    dbus_message_append_args(msg, type, b, DBUS_TYPE_INVALID);
	    break;
	  case DBUS_TYPE_DOUBLE:
	    d = va_arg(var_args, gdouble);
	    dbus_message_append_args(msg, type, d, DBUS_TYPE_INVALID);
	    break;
	  case DBUS_TYPE_STRING:
	    s = va_arg(var_args, gchar *);
#if 0
	    if(s == NULL)
		dbus_message_append_args(msg, DBUS_TYPE_NIL,
					 DBUS_TYPE_INVALID);
	    else
#endif
		dbus_message_append_args(msg, type, s, DBUS_TYPE_INVALID);
	    break;
#if 0
	  case DBUS_TYPE_NIL:	    
		dbus_message_append_args(msg, DBUS_TYPE_NIL,
					 DBUS_TYPE_INVALID);
#endif
	    break;
	  default:
	    break;	    
	}
	
	type = va_arg(var_args, int);
    }
    return;
}

static void _get_arg(DBusMessageIter *iter, osso_rpc_t *retval)
{
    dprint("");

    retval->type = dbus_message_iter_get_arg_type(iter);
    switch(retval->type) {
      case DBUS_TYPE_INT32:
	dbus_message_iter_get_basic(iter,&retval->value.i);
	dprint("got INT32:%d",retval->value.i);
	break;
      case DBUS_TYPE_UINT32:
    dbus_message_iter_get_basic(iter,&retval->value.u);
	dprint("got UINT32:%u",retval->value.u);
	break;
      case DBUS_TYPE_BOOLEAN:
	dbus_message_iter_get_basic(iter,&retval->value.b);
	dprint("got BOOLEAN:%s",retval->value.s?"TRUE":"FALSE");
	break;
      case DBUS_TYPE_DOUBLE:
	dbus_message_iter_get_basic(iter,&retval->value.d);
	dprint("got DOUBLE:%f",retval->value.d);
	break;
      case DBUS_TYPE_STRING:
	{
	  gchar *dbus_string;
      dbus_message_iter_get_basic(iter,&dbus_string);
	  retval->value.s = g_strdup (dbus_string);
	  dbus_free (dbus_string);
	  if(retval->value.s == NULL) {
	    retval->type = DBUS_TYPE_INVALID;
	  }
	  dprint("got STRING:'%s'",retval->value.s);
	}
	break;
#if 0
      case DBUS_TYPE_NIL:
	retval->value.s = NULL;	    
	dprint("got NIL");
	break;
#endif
      default:
	retval->type = DBUS_TYPE_INVALID;
	retval->value.i = 0;	    
	dprint("got unknown type:'%c'(%d)",retval->type,retval->type);
	break;	
    }
}
