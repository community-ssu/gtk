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
    HildonProgram *program;
    HildonWindow *window;
    GtkWidget * grid;
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

#define HILDON_CP_RFS_SCRIPT "/usr/sbin/osso-app-killer-rfs.sh"

#define HILDON_CP_CUD_SCRIPT "/usr/sbin/osso-app-killer-cud.sh"

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
static void _topmost_status_change(GObject *gobject, GParamSpec *arg1,
                                   gpointer user_data);;
/* Callback for resetting factory settings */
static gboolean _reset_factory_settings( GtkWidget *widget, gpointer data );
/* Callback for clear user data */
static gboolean _clear_user_data( GtkWidget *widget, gpointer data );
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

    /* Don't display the application name in the title bar */
    g_set_application_name ("");
     
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

    gtk_widget_show_all( GTK_WIDGET( state_data.window ) );

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

    state_data.window = HILDON_WINDOW( hildon_window_new() );
    state_data.program = HILDON_PROGRAM( hildon_program_get_instance() );

    hildon_program_add_window( state_data.program, state_data.window );

    gtk_window_set_title( GTK_WINDOW( state_data.window ),
            HILDON_CONTROL_PANEL_TITLE );
    
    g_signal_connect( G_OBJECT( state_data.window ), "destroy", 
                      G_CALLBACK( _my_quit ),NULL);
    
    g_signal_connect(G_OBJECT(state_data.program), "notify::is-topmost",
                     G_CALLBACK(_topmost_status_change), NULL);

    menu = GTK_MENU( gtk_menu_new() );

    hildon_window_set_menu( state_data.window, menu );
    
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
        ( HILDON_CONTROL_PANEL_MENU_RFS );

    gtk_menu_shell_append( GTK_MENU_SHELL( sub_tools ), mi );
    
    g_signal_connect( G_OBJECT( mi ), "activate", 
                      G_CALLBACK(_reset_factory_settings), NULL);
    
    /* Clean User Data */
    mi = gtk_menu_item_new_with_label
        ( HILDON_CONTROL_PANEL_MENU_CUD );

    gtk_menu_shell_append( GTK_MENU_SHELL( sub_tools ), mi );
    
    g_signal_connect( G_OBJECT( mi ), "activate", 
                      G_CALLBACK(_clear_user_data), NULL);

    
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
                                    GTK_WIDGET( state_data.window ), 
                                    CONTROLPANEL_ENTRY_DIR);

    grid = hildon_cp_applist_get_grid();
    {
        GValue val = {0,};
        g_value_init(&val, G_TYPE_STRING);
        g_value_set_string(&val, "");
        g_object_set_property(G_OBJECT (grid), "empty_label", &val);
	g_value_unset(&val);
    }

    gtk_widget_show_all(GTK_WIDGET(state_data.window));

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
          
          MBDotDesktop* dd = 
              hildon_cp_applist_get_entry( state_data.focused_filename );
          char * lib_file = NULL;
          if(dd && (lib_file = 
               (char *)mb_dotdesktop_get(dd, "X-control-panel-plugin")))
          {
              osso_cp_plugin_save_state(
		 		    state_data.osso,
					lib_file,
					NULL
					);
          }
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
    ssize_t rc;
    gint fd;
    gchar *eq;
    gchar *nl;
    gchar *start;
    gint length;

    fd = osso_state_open_read(state_data.osso);
    if(fd == -1)
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
    if(rc == -1)
    {
        osso_log(LOG_ERR,
                 "Error retrieving state -- error on reading the state file");
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
            state_data.icon_size = 0;
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
                state_data.icon_size = 0;
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

static void _topmost_status_change(GObject *gobject, GParamSpec *arg1,
                                   gpointer user_data)
{
    HildonProgram *program = HILDON_PROGRAM( gobject );

    if( hildon_program_get_is_topmost (program) )
    {
        hildon_program_set_can_hibernate(program, FALSE);
    }
    else
    {
        _save_state(FALSE);
        hildon_program_set_can_hibernate(program, TRUE);
    }

}

/****** Menu callbacks ******/

static void _my_open( GtkWidget *widget, gpointer data )
{
    _launch( hildon_cp_applist_get_entry( state_data.focused_filename ),
             NULL, TRUE );
}

static void _my_quit(GtkWidget *widget, gpointer data)
{
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

    hildon_cp_rfs( state_data.osso, HILDON_CP_RFS_WARNING, HILDON_CP_RFS_WARNING_TITLE, HILDON_CP_RFS_SCRIPT, HILDON_CP_RFS_HELP_TOPIC );

    return TRUE;
}

static gboolean _clear_user_data( GtkWidget *widget, gpointer data )
{

    hildon_cp_rfs( state_data.osso, HILDON_CP_CUD_WARNING, HILDON_CP_CUD_WARNING_TITLE, HILDON_CP_CUD_SCRIPT, HILDON_CP_CUD_HELP_TOPIC );

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
    char * lib_file = NULL;

    if(dd && (lib_file = 
                (char *)mb_dotdesktop_get(dd, "X-control-panel-plugin")))
    {

        state_data.execute=1;
        ret = osso_cp_plugin_execute( state_data.osso, 
                                      lib_file,
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
