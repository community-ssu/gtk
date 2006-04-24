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

#include <hildon-widgets/hildon-code-dialog.h>
#include <libosso.h>

#include <log-functions.h>


static gboolean hildon_cp_rfs_display_warning( const gchar * warning,
                                               const gchar *title,
                                               const gchar *help_topic,
       	                                       osso_context_t *osso );
static gboolean hildon_cp_rfs_check_lock_code_dialog( osso_context_t * osso );
static gint hildon_cp_rfs_check_lock_code( const gchar *, osso_context_t * );
static void hildon_cp_rfs_launch_script( const gchar * );


gboolean hildon_cp_rfs( osso_context_t * osso, 
                        const gchar *warning,
                        const gchar *title,
                        const gchar *script,
                        const gchar *help_topic )
{
    if( warning )
    {
        if( !hildon_cp_rfs_display_warning( warning, title, help_topic, osso ) )
        {
            /* User canceled, return */
            return TRUE;
        }
    }
            
    if( hildon_cp_rfs_check_lock_code_dialog( osso ) )
    {
        /* Password is correct, proceed */
        hildon_cp_rfs_launch_script( script );
        return FALSE;
    }
    else
    {
            /* User cancelled or an error occured. Exit */
            return TRUE;
    }
}



/*
 * Asks the user for confirmation, returns TRUE if confirmed
 */
static gboolean hildon_cp_rfs_display_warning( const gchar *warning,
                                               const gchar *title,
                                               const gchar *help_topic,
       	                                       osso_context_t *osso ) 
{
    GtkWidget *confirm_dialog;
    GtkWidget *label;
    gint ret;

    confirm_dialog = gtk_dialog_new_with_buttons (
        title,
        NULL,
        GTK_DIALOG_MODAL, 
        RESET_FACTORY_SETTINGS_INFOBANNER_OK, GTK_RESPONSE_OK,
        RESET_FACTORY_SETTINGS_INFOBANNER_CANCEL, GTK_RESPONSE_CANCEL,
        NULL
        );

    ossohelp_dialog_help_enable( GTK_DIALOG( confirm_dialog ),
                                 help_topic,
                                 osso );

    gtk_dialog_set_has_separator( GTK_DIALOG( confirm_dialog ), FALSE );

    label = gtk_label_new( warning );
    gtk_label_set_line_wrap( GTK_LABEL( label ), TRUE );

    gtk_container_add( GTK_CONTAINER( GTK_DIALOG( confirm_dialog )->vbox ), 
                       label );


    gtk_widget_show_all( confirm_dialog );
    ret = gtk_dialog_run( GTK_DIALOG( confirm_dialog ) );

    gtk_widget_destroy( GTK_WIDGET(confirm_dialog) );

    if( ret == GTK_RESPONSE_CANCEL ||
            ret == GTK_RESPONSE_DELETE_EVENT ) {
        return FALSE;
    }

    return TRUE;
}

/*
 * Prompts the user for the lock password.
 * Returns TRUE if correct password, FALSE if cancelled
 */
static gboolean hildon_cp_rfs_check_lock_code_dialog( osso_context_t * osso )
{
    GtkWidget *dialog;
    gint ret;
    gint password_correct = FALSE;

    dialog = hildon_code_dialog_new();
    
    ossohelp_dialog_help_enable( GTK_DIALOG( dialog ),
                                 HILDON_CP_CODE_DIALOG_HELP_TOPIC,
                                 osso );

    gtk_widget_show_all (dialog);

    while( !password_correct )
    {
        gtk_widget_set_sensitive( dialog, TRUE );
        
        ret = gtk_dialog_run( GTK_DIALOG( dialog ) );

        gtk_widget_set_sensitive( dialog, FALSE );

        if (ret == GTK_RESPONSE_CANCEL ||
                ret == GTK_RESPONSE_DELETE_EVENT) {
            gtk_widget_destroy( dialog );
            
            return FALSE;
        }

        password_correct = hildon_cp_rfs_check_lock_code( 
                hildon_code_dialog_get_code( HILDON_CODE_DIALOG( dialog ) ),
                osso );

        if( !password_correct )
        {
            gtk_infoprint (NULL,
                           RESET_FACTORY_SETTINGS_IB_WRONG_LOCKCODE );
            hildon_code_dialog_clear_code( HILDON_CODE_DIALOG( dialog ) );
        }
    }

    gtk_widget_destroy( dialog );

    if( password_correct == -1 )
    {
        /* An error occured in the lock code verification query */
        return FALSE;
    }

    return TRUE;
}

/*
 * Sends dbus message to MCE to check the lock code
 * Returns 0 if not correct, 1 if correct, -1 if an error occured
 */
static gint hildon_cp_rfs_check_lock_code( const gchar * code,
                                           osso_context_t *osso )
{
    gchar * crypted_code;
    osso_return_t ret;
    osso_rpc_t returnvalue;
    gint result;

    crypted_code = crypt( code, HILDON_CP_DEFAULT_SALT );
    crypted_code = rindex( crypted_code, '$' ) + 1;

    ret = osso_rpc_run_system( osso, 
                        HILDON_CP_DBUS_MCE_SERVICE,
                        HILDON_CP_DBUS_MCE_REQUEST_PATH,
                        HILDON_CP_DBUS_MCE_REQUEST_IF,
                        HILDON_CP_MCE_PASSWORD_VALIDATE,
                        &returnvalue,
                        DBUS_TYPE_STRING,
                        crypted_code,
                        DBUS_TYPE_STRING,
                        HILDON_CP_DEFAULT_SALT,
                        DBUS_TYPE_INVALID );

    switch( ret )
    {
        case OSSO_INVALID:
            osso_log( LOG_ERR, "Lockcode query call failed: Invalid "
                               "parameter\n" );
            osso_rpc_free_val( &returnvalue );
            return -1;
        case OSSO_RPC_ERROR:
        case OSSO_ERROR:
        case OSSO_ERROR_NAME:
        case OSSO_ERROR_NO_STATE:
        case OSSO_ERROR_STATE_SIZE:
            if( returnvalue.type == DBUS_TYPE_STRING )
            {
                osso_log( LOG_ERR, "Lockcode query call failed: %s\n",
                          returnvalue.value.s );
            }
            else
            {
                osso_log( LOG_ERR,
                          "Lockcode query call failed: unspecified" );
            }
            osso_rpc_free_val( &returnvalue );
            return -1;

        case OSSO_OK:
            break;
        default:
            osso_log( LOG_ERR, "Lockcode query call failed: unknown"
                               " error type %d", ret );
            osso_rpc_free_val( &returnvalue );
            return -1;
    }
        
    if( returnvalue.type != DBUS_TYPE_BOOLEAN )
    {
        osso_log( LOG_ERR, "Lockcode query call failed: unexpected return "
                "value type %d", returnvalue.type );

        osso_rpc_free_val( &returnvalue );
        return -1;
    }

    result = (gint)returnvalue.value.b;
    osso_rpc_free_val( &returnvalue );
    return result;
}

static void hildon_cp_rfs_launch_script( const gchar * script )
{
    GError *error = NULL;

    if (!g_spawn_command_line_async (script,
                &error))
    {
        osso_log (LOG_ERR, "Call to RFS or CUD script failed");
        if (error)
        {
            osso_log (LOG_ERR, error->message);
            g_error_free (error);
        }
    }
}
