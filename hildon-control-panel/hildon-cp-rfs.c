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

#include <codelockui.h>
#include <log-functions.h>


static gboolean hildon_cp_rfs_display_warning( const char * warning );
static gboolean hildon_cp_rfs_check_lock_code( osso_context_t * osso );
static void hildon_cp_rfs_launch_script( const gchar * );


gboolean hildon_cp_rfs( osso_context_t * osso, 
                        const gchar *warning,
                        const gchar * script )
{
    if( warning )
    {
        if( !hildon_cp_rfs_display_warning( warning ) )
        {
            /* User canceled, return */
            return TRUE;
        }
    }
            
    if( hildon_cp_rfs_check_lock_code( osso ) )
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
static gboolean hildon_cp_rfs_display_warning( const gchar *warning )
{
    GtkWidget *confirm_dialog;
    gint ret;
    
    confirm_dialog = hildon_note_new_confirmation(
        NULL,
        /*(GTK_WINDOW(state_data.window),*/
         warning);

    hildon_note_set_button_texts
        ( HILDON_NOTE(confirm_dialog),
          RESET_FACTORY_SETTINGS_INFOBANNER_OK,
          RESET_FACTORY_SETTINGS_INFOBANNER_CANCEL );

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
static gboolean hildon_cp_rfs_check_lock_code( osso_context_t * osso )
{
    CodeLockUI *lock_ui = g_malloc0( sizeof( CodeLockUI ) );
    GtkWidget *dialog;
    gint ret;
    gboolean password_correct = FALSE;

    codelockui_init(osso);

    dialog = codelock_create_dialog( lock_ui, TIMEOUT_FOOBAR );

    while( !password_correct )
    {
        gtk_widget_set_sensitive( dialog, TRUE );
        
        ret = gtk_dialog_run( GTK_DIALOG( dialog ) );

        gtk_widget_set_sensitive( dialog, FALSE );

        if (ret == GTK_RESPONSE_NO /* XXX cancel triggers NO, not CANCEL */ ||
                ret == GTK_RESPONSE_DELETE_EVENT) {
            codelock_destroy_dialog( lock_ui ); 
            g_free( lock_ui );
            
            codelockui_deinit();
            return FALSE;
        }

        password_correct = codelock_is_passwd_correct( gtk_entry_get_text(                     GTK_ENTRY( lock_ui->entry1 ) ) );

        if( !password_correct )
        {
            gtk_infoprint (NULL,
                           RESET_FACTORY_SETTINGS_IB_WRONG_LOCKCODE );
            gtk_entry_set_text( GTK_ENTRY( lock_ui->entry1 ), "" );
        }
    }

    codelock_destroy_dialog( lock_ui ); 
    g_free( lock_ui );
    
    codelockui_deinit();

    return TRUE;
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
