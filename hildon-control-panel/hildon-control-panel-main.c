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


#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

/* Osso logging functions */
#include <osso-log.h>


#include <gconf/gconf-client.h>
#include <libosso.h>

#include "hildon-control-panel-main.h"
#include "hildon-cp-applist.h"
#include "hildon-cp-view.h"
#include "hildon-cp-item.h"


/* Delay to wait from callback to actually reading the entries in msecs */
#define DIRREADDELAY 500

#define HILDON_CP_RFS_SCRIPT "/usr/sbin/osso-app-killer-rfs.sh"
#define HILDON_CP_CUD_SCRIPT "/usr/sbin/osso-app-killer-cud.sh"

/* Definitions for mobile operator wizard launch */
#define HILDON_CONTROL_PANEL_DBUS_OPERATOR_WIZARD_SERVICE "operator_wizard"
#define HILDON_CONTROL_PANEL_OPERATOR_WIZARD_LAUNCH       "launch_operator_wizard"

#define HILDON_CONTROL_PANEL_STATEFILE_NO_CONTENT "EOF"

/* Definitions for ugly hacks that have to be used in order to make
 * CPGrid behave in this situation.
 * FIXME: use a widget better suited for this purpose
 */
#define LARGE_ITEM_ROW_HEIGHT 64
#define SMALL_ITEM_ROW_HEIGHT 30
#define GRID_WIDTH            650
#define LARGE_ITEM_PADDING    0 /* Padding is unnecessary as the item  */
#define SMALL_ITEM_PADDING    0 /* category label contains padding too */


/* Sets DNOTIFY callback for the .desktop dir */
static int hcp_init_dnotify (HCP *hcp, 
                             const gchar * path,
                             const gchar * path_additional);
static void hcp_dnotify_callback_f (gchar * path, HCP *hcp);


/* Create the GUI entries */
/*static void hcp_init_gui(void);*/
static GtkWidget *hcp_create_window (HCP *hcp);
static void       hcp_show_window (HCP *hcp);

/* Set the grid to use large/small icons */
static void hcp_large_icons (HCP *hcp);
static void hcp_small_icons (HCP *hcp);

/* Create & free state data */
static void hcp_create_data (HCP *hcp);
static void hcp_destroy_data (HCP *hcp);

/* State saving functions */
static void hcp_save_state (HCP *hcp, gboolean clear_state);
static void hcp_retrieve_state(HCP *hcp);
static void hcp_enforce_state(HCP *hcp);

/* Configuration saving functions */
static void hcp_save_configuration (HCP *hcp);
static void hcp_retrieve_configuration (HCP *hcp);

/* Keylistener */
static gint hcp_cp_keyboard_listener (GtkWidget * widget,
                                      GdkEventKey * keyevent, 
                                      HCP *hcp);

/* Callback for topping */
static void hcp_topmost_status_change (GObject * gobject, 
		                               GParamSpec * arg1,
				                       HCP *hcp);
/* Callback for resetting factory settings */
static gboolean hcp_reset_factory_settings (GtkWidget * widget, 
		                                    HCP *hcp);

/* Callback for clear user data */
static gboolean hcp_clear_user_data (GtkWidget * widget, HCP *hcp);

/* Callback for operator wizard launch */
static void hcp_run_operator_wizard (GtkWidget * widget, HCP *hcp);

/* Callback for help function */
static void hcp_launch_help (GtkWidget * widget, HCP *hcp);

/* Callback for menu item small/large icons */
static void hcp_iconsize (GtkWidget * widget, HCP *hcp);
static void hcp_my_open  (GtkWidget * widget, HCP *hcp);
static void hcp_my_quit  (GtkWidget * widget, HCP *hcp);


/* Callback for handling HW signals */
static void hcp_hw_signal_cb (osso_hw_state_t * state, HCP *hcp);

/* Hack to make CPGrid behave */
static int hcp_calculate_grid_rows (GtkWidget *grid);
    
/* RPC setup */
static void hcp_init_rpc (HCP *hcp);

/* RPC callbacks */
static gint hcp_rpc_handler (const gchar *interface,
                             const gchar *method,
                             GArray *arguments,
                             HCP *hcp,
                             osso_rpc_t *retval);

/**********************************************************************
 * Main program
 **********************************************************************/
int main (int argc, char **argv)
{
    HCP *hcp;
    const gchar *additional_cp_applets_dir_environment;
    gboolean dbus_activated;

    setlocale(LC_ALL, "");

    bindtextdomain(PACKAGE, LOCALEDIR);

    bind_textdomain_codeset(PACKAGE, "UTF-8");

    textdomain(PACKAGE);

    /* Set application name to "" as we only need the window title
     * in the title bar */
    g_set_application_name("");
    
    gtk_init(&argc, &argv);

    hcp = g_new0 (HCP, 1);

    hcp_item_init (hcp);
    
    additional_cp_applets_dir_environment = 
        g_getenv (ADDITIONAL_CP_APPLETS_DIR_ENVIRONMENT);

    hcp_init_dnotify (hcp,
                      CONTROLPANEL_ENTRY_DIR,
                      additional_cp_applets_dir_environment);


    hcp_create_data (hcp);             /* Create state data */

    hcp_init_rpc (hcp);
    
    hcp_retrieve_configuration (hcp);  /* configuration from disk */

    hcp_retrieve_state (hcp);          /* State data from disk */

    if (hcp->saved_focused_filename)
        hcp->focused_item = g_hash_table_lookup (hcp->al->applets,
                                                 hcp->saved_focused_filename);

    osso_hw_set_event_cb (hcp->osso,
                          NULL ,
                          (osso_hw_cb_f *)hcp_hw_signal_cb,
                          hcp);


    dbus_activated = g_getenv ("DBUS_STARTER_BUS_TYPE")?TRUE:FALSE;

#if 0
    if (!dbus_activated)
    {
        /* When started from the command line we show the UI as default
         * behavior. When dbus activated, we wait to see if we got
         * top_application or run_applet method call */
        hcp_show_window (hcp);
    }
#endif
    /* Always start the user interface for now */
    hcp_show_window (hcp);
    
    if (hcp->execute == 1 && hcp->focused_item) {
        hcp_item_launch(hcp->focused_item, FALSE);
    }

    gtk_main();

    hcp_destroy_data (hcp);
    g_free (hcp);

    return 0;
}

/**********************************************************************
 * DNotify stuff
 **********************************************************************/

static int callback_pending = 0;

/* called by dnotify_callback_f after a timeout (to prevent exessive
   callbacks. */

static gboolean hcp_dnotify_reread_desktop_entries (HCP *hcp)
{
    gchar * focussed_item = g_strdup (hcp->focused_item->plugin);
    HCPItem *item;
    callback_pending = 0;

    /* Re-read the item list from .desktop files */
    hcp_al_update_list (hcp->al);
    /* Update the view */
    hcp_view_populate (hcp->view, hcp->al);
    gtk_widget_show_all (hcp->view);

    item = g_hash_table_lookup (hcp->al->applets, focussed_item);
    g_free (focussed_item);

    if (item && item->grid_item)
        gtk_widget_grab_focus (item->grid_item);

    hcp_enforce_state (hcp);

    return FALSE;       /* do not have the timeout called again */
}

static void hcp_dnotify_callback_f (char *path, HCP *hcp)
{
    if (!callback_pending) {
        callback_pending = 1;
        g_timeout_add(DIRREADDELAY,
                      (GSourceFunc) hcp_dnotify_reread_desktop_entries, hcp);
    }
}

static int hcp_init_dnotify (HCP * hcp,
                             const gchar * path,
                             const gchar * path_additional)
{
    hildon_return_t ret;

    ret = hildon_dnotify_handler_init ();
    if (ret != HILDON_OK)
    {
        return ret;
    }
    
    ret = hildon_dnotify_set_cb ((hildon_dnotify_cb_f*)hcp_dnotify_callback_f,
                                 (gchar *)path,
                                 hcp);

    if (ret != HILDON_OK)
    {
        return ret;
    }

    if (path_additional)
    {
        ret = hildon_dnotify_set_cb (
                (hildon_dnotify_cb_f*)hcp_dnotify_callback_f,
                (gchar *)path_additional,
                hcp);
    }
    return HILDON_OK;
}


/*********************************************************************
 * RPC setup
 *********************************************************************/
static
void hcp_init_rpc (HCP *hcp)
{
    hcp->osso = osso_initialize(APP_NAME, APP_VERSION, TRUE, NULL);
    
    if (!hcp->osso)
    {
        ULOG_ERR("Error initializing osso -- check that D-BUS is running");
        exit(-1);
    }

#if 0
    osso_rpc_set_cb_f (hcp->osso,
                       "com.nokia.controlpanel",
                       HCP_RPC_PATH,
                       HCP_RPC_INTERFACE,
                       (osso_rpc_cb_f *)hcp_rpc_handler,
                       hcp);
#endif
    
    osso_rpc_set_default_cb_f (hcp->osso,
                       (osso_rpc_cb_f *)hcp_rpc_handler,
                       hcp);
                       
}


/**********************************************************************
 * RPC callbacks
 **********************************************************************/
static
gint hcp_rpc_handler (const gchar *interface,
                      const gchar *method,
                      GArray *arguments,
                      HCP *hcp,
                      osso_rpc_t *retval)
{
    g_return_val_if_fail (method, OSSO_ERROR);

    if ((!strcmp (method, HCP_RPC_METHOD_RUN_APPLET)))
    {
        osso_rpc_t applet, user_activated;
        HCPItem *item;
        
        if (arguments->len != 2)
            goto error;

        applet = g_array_index (arguments, osso_rpc_t, 0);
        user_activated = g_array_index (arguments, osso_rpc_t, 1);

        if (applet.type != DBUS_TYPE_STRING)
            goto error;
        
        if (user_activated.type != DBUS_TYPE_BOOLEAN)
            goto error;
        
        item = g_hash_table_lookup (hcp->al->applets, applet.value.s);

        if (item)
        {
            if (!item->running)
            {
                hcp_item_launch (item, user_activated.value.b);

            }

            if(GTK_IS_WINDOW (hcp->window))
                gtk_window_present (GTK_WINDOW (hcp->window));

            retval->type = DBUS_TYPE_INT32;
            retval->value.i = 0;
            return OSSO_OK;
        }

    }

    else if ((!strcmp (method, HCP_RPC_METHOD_TOP_APPLICATION)))
    {
        if (!hcp->window)
            hcp_show_window (hcp);
        else
            gtk_window_present (GTK_WINDOW (hcp->window));

        retval->type = DBUS_TYPE_INT32;
        retval->value.i = 0;
        return OSSO_OK;
    }

error:
    retval->type = DBUS_TYPE_INT32;
    retval->value.i = -1;
    return OSSO_ERROR;


}

/**********************************************************************
 * GUI helper functions
 **********************************************************************/

/* FIXME: use a widget better suited for this situation. Grid wants all the
 * space it can get as it is designed to fill the whole area. 
 * Now we have to restrict it's size in this rather ugly way
 */
static 
void hcp_large_icons (HCP *hcp)
{
    GtkWidget *grid = NULL;
    GList *iter = NULL;
    GList *list = NULL;

    /* if we have grids */
    if (hcp->view)
    {
        list = iter = gtk_container_get_children (
                GTK_CONTAINER (hcp->view));

        /* iterate through all of them and set their mode */
        while (iter != 0)
        {
            if (CP_OBJECT_IS_GRID(iter->data))
            {
                grid = GTK_WIDGET (iter->data);
                cp_grid_set_style(CP_GRID(grid), "largeicons-cp");
            
                guint num_rows = hcp_calculate_grid_rows (grid);
                gtk_widget_set_size_request (grid, GRID_WIDTH,
                        num_rows * LARGE_ITEM_ROW_HEIGHT + LARGE_ITEM_PADDING);
            }

            iter = iter->next;
        }
        
        g_list_free (list);
    }


}

/* FIXME: use a widget better suited for this situation. Grid wants all the
 * space it can get as it is designed to fill the whole area. 
 * Now we have to restrict it's size in this rather ugly way
 */
static void 
hcp_small_icons (HCP *hcp)
{
    GtkWidget *grid = NULL;
    GList *iter = NULL;
    GList *list = NULL;

    /* If we have grids */
    if (hcp->view)
    {
        list = iter = gtk_container_get_children (
                GTK_CONTAINER(hcp->view));

        /* Iterate through all of them and set their mode */
        while (iter != 0)
        {
            if (CP_OBJECT_IS_GRID(iter->data))
            {
                grid = GTK_WIDGET (iter->data);
                cp_grid_set_style(CP_GRID(grid), "smallicons-cp");

                guint num_rows = hcp_calculate_grid_rows (grid);
                gtk_widget_set_size_request (grid, GRID_WIDTH,
                        num_rows * SMALL_ITEM_ROW_HEIGHT + SMALL_ITEM_PADDING);
            }

            iter = iter->next;
        }

        g_list_free (list);
    }
}


/************************************************************************
 * Application state data handling
 ************************************************************************/

/* Create a struct for the application state data */
static void 
hcp_create_data (HCP *hcp)
{

    hcp->icon_size = 1;
    hcp->focused_item = NULL;
    hcp->saved_focused_filename = NULL;
    hcp->execute = 0;
    hcp->scroll_value = 0;
    hcp->al = hcp_al_new ();
    hcp_al_update_list (hcp->al);
}

/* Clean the application state data struct */
static void 
hcp_destroy_data (HCP *hcp)
{
    if (hcp->osso)
        osso_deinitialize (hcp->osso);
}

/* Save state data on disk */
static void 
hcp_save_state (HCP *hcp, gboolean clear_state)
{
    osso_state_t state = { 0, };
    GKeyFile *keyfile = NULL;
    osso_return_t ret;
    GError *error = NULL;

    if (clear_state)
    {
        state.state_data = "";
        state.state_size = 1;
        ret = osso_state_write (hcp->osso, &state);
        if (ret != OSSO_OK)
            ULOG_ERR ("An error occured when clearing application state");

        goto cleanup;
    }

    keyfile = g_key_file_new ();

    g_key_file_set_string (keyfile,
                           HCP_STATE_GROUP,
                           HCP_STATE_FOCUSSED,
                           hcp->focused_item?  hcp->focused_item->plugin:"");
    
    g_key_file_set_integer (keyfile,
                            HCP_STATE_GROUP,
                            HCP_STATE_SCROLL_VALUE,
                            hcp->scroll_value);

    g_key_file_set_boolean (keyfile,
                            HCP_STATE_GROUP,
                            HCP_STATE_EXECUTE,
                            hcp->execute);

    state.state_data = g_key_file_to_data (keyfile,
                                           &state.state_size,
                                           &error);

    if (error)
        goto cleanup;

    ret = osso_state_write (hcp->osso, &state);

    if (ret != OSSO_OK)
    {
        ULOG_ERR ("An error occured when reading application state");
    }


cleanup:
    if (error)
    {
        ULOG_ERR ("An error occured when reading application state: %s",
                  error->message);
        g_error_free (error);
    }

    g_free (state.state_data);
    if (keyfile)
        g_key_file_free (keyfile);
}

/* Read the saved state from disk */
static void 
hcp_retrieve_state (HCP *hcp)
{
    osso_state_t state = { 0, };
    GKeyFile *keyfile = NULL;
    osso_return_t ret;
    GError *error = NULL;
    gchar *focussed = NULL;
    gint scroll_value;
    gboolean execute;

    ret = osso_state_read (hcp->osso, &state);

    if (ret != OSSO_OK)
    {
        ULOG_ERR ("An error occured when reading application state");
        return;
    }

    if (state.state_size == 1)
    {
        /* Clean state, return */
        goto cleanup;
    }

    keyfile = g_key_file_new ();

    g_key_file_load_from_data (keyfile,
                               state.state_data,
                               state.state_size,
                               G_KEY_FILE_NONE,
                               &error);

    if (error)
    {
        ULOG_ERR ("An error occured when reading application state: %s",
                  error->message);
        goto cleanup;
    }

    focussed = g_key_file_get_string (keyfile,
                                      HCP_STATE_GROUP,
                                      HCP_STATE_FOCUSSED,
                                      &error);

    if (error)
    {
        ULOG_ERR ("An error occured when reading application state: %s",
                  error->message);
        goto cleanup;
    }

    hcp->saved_focused_filename = focussed;

    scroll_value = g_key_file_get_integer (keyfile,
                                           HCP_STATE_GROUP,
                                           HCP_STATE_SCROLL_VALUE,
                                           &error);
    if (error)
    {
        ULOG_ERR ("An error occured when reading application state: %s",
                  error->message);
        goto cleanup;
    }
    
    hcp->scroll_value = scroll_value;
    
    execute = g_key_file_get_boolean (keyfile,
                                      HCP_STATE_GROUP,
                                      HCP_STATE_EXECUTE,
                                      &error);
    if (error)
    {
        ULOG_ERR ("An error occured when reading application state: %s",
                  error->message);
        goto cleanup;
    }
    
    hcp->execute = execute;

cleanup:
    if (error)
        g_error_free (error);

    g_free (state.state_data);
    if (keyfile)
        g_key_file_free (keyfile);
    
}

static void 
hcp_enforce_state (HCP *hcp)
{
    /* Actually enforce the saved state */

    if (hcp->icon_size == 0)
        hcp_small_icons (hcp);
    else if (hcp->icon_size == 1)
        hcp_large_icons (hcp);
    else 
        ULOG_ERR("Unknown iconsize");

    if (hcp->saved_focused_filename)
    {
        hildon_cp_applist_focus_item (hcp->al, 
                                      hcp->saved_focused_filename);

        g_free (hcp->saved_focused_filename);
        hcp->saved_focused_filename = NULL;
    }

    /* main() will start the possible plugin in hcp->execute */
}


/* Save the configuration file (large/small icons)  */
static void 
hcp_save_configuration (HCP *hcp)
{
    GConfClient *client = NULL;
    GError *error = NULL;
    gboolean icon_size;

    client = gconf_client_get_default ();

    g_return_if_fail (client);

    icon_size = hcp->icon_size?TRUE:FALSE;

    gconf_client_set_bool (client,
                           HCP_GCONF_ICON_SIZE,
                           icon_size,
                           &error);

    if (error)
    {
        ULOG_ERR (error->message);
        g_error_free (error);
    }

    g_object_unref (client);

}

static void 
hcp_retrieve_configuration (HCP *hcp)
{
    GConfClient *client = NULL;
    GError *error = NULL;
    gboolean icon_size;

    client = gconf_client_get_default ();

    g_return_if_fail (client);

    icon_size = hcp->icon_size?TRUE:FALSE;

    icon_size = gconf_client_get_bool (client,
                                       HCP_GCONF_ICON_SIZE,
                                       &error);

    if (error)
    {
        ULOG_ERR (error->message);
        g_error_free (error);
    }
    else
    {
        hcp->icon_size = icon_size?1:0;
    }

    g_object_unref (client);

}

/***********************************************************************
 * Callbacks
 ***********************************************************************/

/***** Keyboard listener *******/
static gint 
hcp_cp_keyboard_listener (GtkWidget * widget,
                          GdkEventKey * keyevent, 
		                  HCP *hcp)
{
    if (keyevent->type == GDK_KEY_RELEASE) {
        if (keyevent->keyval == HILDON_HARDKEY_INCREASE) {
            if (hcp->icon_size != 1) {
                hcp->icon_size = 1;
                gtk_check_menu_item_set_active (
                        GTK_CHECK_MENU_ITEM (hcp->large_icons_menu_item),
                        TRUE);
                hcp_large_icons (hcp);
                hcp_save_configuration (hcp);
                return TRUE;
            }

        } else if (keyevent->keyval == HILDON_HARDKEY_DECREASE) {
            if (hcp->icon_size != 0) {
                hcp->icon_size = 0;
                gtk_check_menu_item_set_active (
                        GTK_CHECK_MENU_ITEM (hcp->small_icons_menu_item),
                        TRUE);
                hcp_small_icons (hcp);
                hcp_save_configuration (hcp);
                return TRUE;
            }
        }
    }
    
    return FALSE;

}

/***** Top/Untop callbacks *****/
static void 
hcp_topmost_status_change (GObject * gobject, 
		                   GParamSpec * arg1,
			               HCP *hcp)
{
    HildonProgram *program = HILDON_PROGRAM (gobject);

    if (hildon_program_get_is_topmost (program)) {
        hildon_program_set_can_hibernate (program, FALSE);
    } else {
        hcp_save_state (hcp, FALSE);
        hildon_program_set_can_hibernate (program, TRUE);
    }

}

/****** Menu callbacks ******/

static void hcp_my_open (GtkWidget *widget, HCP *hcp)
{
    hcp_item_launch (hcp->focused_item, TRUE);
}

static void hcp_my_quit (GtkWidget *widget, HCP *hcp)
{
    hcp_save_state (hcp, TRUE);
    gtk_main_quit ();
}

static void hcp_iconsize (GtkWidget *widget, HCP *hcp)
{
    if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget)))
        return;
    
    if (widget == hcp->large_icons_menu_item)
    {
        hcp_large_icons (hcp);
        hcp->icon_size = 1;
    }
    else if (widget == hcp->small_icons_menu_item)
    {
        hcp_small_icons (hcp);
        hcp->icon_size = 0;
    }
    
    hcp_save_configuration (hcp);
}

static void hcp_run_operator_wizard (GtkWidget *widget, HCP *hcp)
{

    osso_rpc_t returnvalues;
    osso_return_t returnstatus;
    returnstatus = osso_rpc_run_with_defaults
        (hcp->osso, HILDON_CONTROL_PANEL_DBUS_OPERATOR_WIZARD_SERVICE,
         HILDON_CONTROL_PANEL_OPERATOR_WIZARD_LAUNCH,
         &returnvalues, 
         DBUS_TYPE_INVALID);    

    
    switch (returnstatus)
    {
        case OSSO_OK:
            break;
        case OSSO_INVALID:
        
            ULOG_ERR("Invalid parameter in operator_wizard launch");

            break;
        case OSSO_RPC_ERROR:
        case OSSO_ERROR:
        case OSSO_ERROR_NAME:
        case OSSO_ERROR_NO_STATE:
        case OSSO_ERROR_STATE_SIZE:
            if (returnvalues.type == DBUS_TYPE_STRING) {
                
                ULOG_ERR("Operator wizard launch failed: %s\n",returnvalues.value.s);
            }
            else {
                ULOG_ERR("Operator wizard launch failed, unspecified");
            }
            break;            
        default:
            ULOG_ERR("Unknown error type %d", returnstatus);
    }
    
    osso_rpc_free_val (&returnvalues);
}

static gboolean hcp_reset_factory_settings (GtkWidget *widget, HCP *hcp)
{

    hildon_cp_rfs( hcp->osso, HILDON_CP_RFS_WARNING, HILDON_CP_RFS_WARNING_TITLE, HILDON_CP_RFS_SCRIPT, HILDON_CP_RFS_HELP_TOPIC );

    return TRUE;
}

static gboolean hcp_clear_user_data (GtkWidget *widget, HCP *hcp)
{

    hildon_cp_rfs (hcp->osso,
                   HILDON_CP_CUD_WARNING,
                   HILDON_CP_CUD_WARNING_TITLE,
                   HILDON_CP_CUD_SCRIPT,
                   HILDON_CP_CUD_HELP_TOPIC );

    return TRUE;
}

static void hcp_launch_help (GtkWidget *widget, HCP *hcp)
{
    osso_return_t help_ret;
    
    help_ret = ossohelp_show (hcp->osso, OSSO_HELP_ID_CONTROL_PANEL, 0);

    switch (help_ret)
     {
         case OSSO_OK:
             break;
         case OSSO_ERROR:
             ULOG_WARN ("HELP: ERROR (No help for such topic ID)\n");
             break;
         case OSSO_RPC_ERROR:
             ULOG_WARN ("HELP: RPC ERROR (RPC failed for HelpApp/Browser)\n");
             break;
         case OSSO_INVALID:
             ULOG_WARN ("HELP: INVALID (invalid argument)\n");
             break;
         default:
             ULOG_WARN ("HELP: Unknown error!\n");
             break;
     }

}

static void hcp_hw_signal_cb (osso_hw_state_t *state, HCP *hcp)
{
	if (state != NULL) {
		
		/* Shutdown */
		if (state->shutdown_ind)
        {
			/* Save state */
			hcp_save_state (hcp, FALSE);
			/* Quit */
			hcp_my_quit (NULL, hcp);
		}
		
	}

	return;
}



static int hcp_calculate_grid_rows (GtkWidget *grid)
{
    guint num_columns = 0, num_rows = 0, num_items = 0;
    GValue val = { 0, };

    g_value_init(&val, G_TYPE_UINT);
    g_object_get_property(G_OBJECT(grid), "n_items", &val);
    num_items = g_value_get_uint(&val);
    g_value_unset(&val);

    gtk_widget_style_get (grid, "n_columns", &num_columns, NULL);

    num_rows = (num_items / num_columns) + (num_items % num_columns ? 1 : 0);

    return num_rows;
}


static void
hcp_show_window (HCP *hcp)
{
    hcp->view = hcp_view_new ();
    hcp_view_populate (hcp->view, hcp->al);
    hcp_create_window (hcp);
    
    gtk_widget_show_all (GTK_WIDGET(hcp->window));
    hcp_enforce_state (hcp);           /* realize the saved state */
}

static GtkWidget *
hcp_create_window (HCP *hcp)
{
    GtkMenu *menu = NULL;
    GtkWidget *sub_view = NULL;
    GtkWidget *sub_tools = NULL;
    GtkWidget *mi = NULL;
    GtkWidget *scrolled_window = NULL;
    GSList *menugroup = NULL;

    /* Why is this not read from the gtkrc?? -- Jobi */
    /* Control Panel Grid */
    gtk_rc_parse_string ("  style \"hildon-control-panel-grid\" {"
                "    CPGrid::n_columns = 2"
            "    CPGrid::label_pos = 1"
            "  }"
            " widget \"*.hildon-control-panel-grid\" "
            "    style \"hildon-control-panel-grid\"");

    /* Separators style */
    gtk_rc_parse_string ("  style \"hildon-control-panel-separator\" {"
            "    GtkSeparator::hildonlike-drawing = 1"
                        "  }"
            " widget \"*.hildon-control-panel-separator\" "
                        "    style \"hildon-control-panel-separator\"");

    hcp->window = HILDON_WINDOW (hildon_window_new ());
    hcp->program = HILDON_PROGRAM (hildon_program_get_instance ());

    hildon_program_add_window (hcp->program, hcp->window);

    gtk_window_set_title (GTK_WINDOW (hcp->window),
                          HILDON_CONTROL_PANEL_TITLE);

    g_signal_connect (G_OBJECT (hcp->window), "destroy",
                      G_CALLBACK (hcp_my_quit), hcp);

    g_signal_connect(G_OBJECT(hcp->program), "notify::is-topmost",
                     G_CALLBACK(hcp_topmost_status_change), hcp);

    menu = GTK_MENU (gtk_menu_new ());

    hildon_window_set_menu (hcp->window, menu);

    mi = gtk_menu_item_new_with_label (HILDON_CONTROL_PANEL_MENU_OPEN);

    g_signal_connect (G_OBJECT(mi), "activate",
                      G_CALLBACK(hcp_my_open), hcp);

    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);


    /* View submenu */
    sub_view = gtk_menu_new ();

    mi = gtk_menu_item_new_with_label (HILDON_CONTROL_PANEL_MENU_SUB_VIEW);

    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), sub_view);

    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    /* Small icon size */
    mi = gtk_radio_menu_item_new_with_label
        (menugroup, HILDON_CONTROL_PANEL_MENU_SMALL_ITEMS);
    menugroup = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (mi));
    hcp->small_icons_menu_item = mi;

    if (hcp->icon_size == 0) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(mi), TRUE);
    }


    gtk_menu_shell_append (GTK_MENU_SHELL(sub_view), mi);


    g_signal_connect (G_OBJECT (mi), "activate",
                      G_CALLBACK (hcp_iconsize), hcp);

    /* Large icon size */
    mi = gtk_radio_menu_item_new_with_label
        (menugroup, HILDON_CONTROL_PANEL_MENU_LARGE_ITEMS);
    hcp->large_icons_menu_item = mi;

    if (hcp->icon_size == 1) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), TRUE);
    }
    menugroup = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (mi));

    gtk_menu_shell_append (GTK_MENU_SHELL (sub_view), mi);

    g_signal_connect (G_OBJECT (mi), "activate", 
		              G_CALLBACK (hcp_iconsize), hcp);


    /* Tools submenu */
    sub_tools = gtk_menu_new ();

    mi = gtk_menu_item_new_with_label (HILDON_CONTROL_PANEL_MENU_SUB_TOOLS);

    gtk_menu_item_set_submenu (GTK_MENU_ITEM(mi), sub_tools);

    gtk_menu_shell_append (GTK_MENU_SHELL(menu), mi);


    /* Run operator wizard */
    mi = gtk_menu_item_new_with_label
        (HILDON_CONTROL_PANEL_MENU_SETUP_WIZARD);

    gtk_menu_shell_append (GTK_MENU_SHELL(sub_tools), mi);

    g_signal_connect (G_OBJECT (mi), "activate",
                      G_CALLBACK (hcp_run_operator_wizard), hcp);

    /* Reset Factory Settings */
    mi = gtk_menu_item_new_with_label (HILDON_CONTROL_PANEL_MENU_RFS);

    gtk_menu_shell_append (GTK_MENU_SHELL (sub_tools), mi);

    g_signal_connect (G_OBJECT (mi), "activate",
                      G_CALLBACK (hcp_reset_factory_settings), hcp);

    /* Clean User Data */
    mi = gtk_menu_item_new_with_label (HILDON_CONTROL_PANEL_MENU_CUD);

    gtk_menu_shell_append (GTK_MENU_SHELL (sub_tools), mi);

    g_signal_connect (G_OBJECT (mi), "activate",
                      G_CALLBACK (hcp_clear_user_data), hcp);


    /* Help! */
    mi = gtk_menu_item_new_with_label (HILDON_CONTROL_PANEL_MENU_HELP);

    gtk_menu_shell_append (GTK_MENU_SHELL (sub_tools), mi);

    g_signal_connect(G_OBJECT (mi), "activate",
                     G_CALLBACK (hcp_launch_help), hcp);

    /* Close */
    mi = gtk_menu_item_new_with_label (HILDON_CONTROL_PANEL_MENU_CLOSE);

    g_signal_connect (GTK_OBJECT(mi), "activate",
                      G_CALLBACK(hcp_my_quit), hcp);

    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);


    gtk_widget_show_all (GTK_WIDGET (menu));

    /* What is this for? -- Jobi */
#if 0 
  

    hcp->grids = hildon_cp_applist_get_grids();
    grids = hcp->grids;
    
    while (grids != 0)
    {
        grid = grids->data;
        {
            GValue val = { 0, };
            g_value_init(&val, G_TYPE_STRING);
            g_value_set_string(&val, "");
            g_object_set_property(G_OBJECT(grid), "empty_label", &val);
            g_value_unset(&val);
        }

        grids = grids->next;
    }
#endif

    /* Set the keyboard listening callback */

    gtk_widget_add_events (GTK_WIDGET(hcp->window),
                           GDK_BUTTON_RELEASE_MASK);

    g_signal_connect (G_OBJECT(hcp->window), "key_release_event",
                      G_CALLBACK(hcp_cp_keyboard_listener), hcp);
    
    scrolled_window = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                                    "vscrollbar-policy",
                                    GTK_POLICY_ALWAYS,
                                    "hscrollbar-policy",
                                    GTK_POLICY_NEVER,
                                    NULL);

    gtk_container_add (GTK_CONTAINER (hcp->window), scrolled_window);

    gtk_scrolled_window_add_with_viewport (
            GTK_SCROLLED_WINDOW (scrolled_window),
            hcp->view);

    return GTK_WIDGET (hcp->window);
}
