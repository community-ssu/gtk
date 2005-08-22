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
 
/* Hildon includes */
#include "hildon-status-bar-lib.h"

#include <hildon-widgets/hildon-note.h>

#include <gtk/gtkdialog.h>
#include <glib.h>
#include <libosso.h>

/* log include */
#include <log-functions.h>

static SystemDialog dialogs[HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS];
static gint dialog_index = 0;

static void hildon_status_bar_lib_show_dialog( gint dialog_id );
static void hildon_status_bar_lib_destroy_dialog( GtkDialog *dialog, gint arg1, 
                                                  gpointer data );

static void hildon_status_bar_lib_show_dialog( gint dialog_id )
{
    GtkWidget *note;

    /* These must match with the D-BUS system note types */
    /* osso_system_note_type_t; */
    static gchar *icon[HILDON_STATUS_BAR_MAX_NOTE_TYPE] = {
        "qgn_note_gene_syswarning",         /* OSSO_GN_WARNING */
        "qgn_note_gene_syserror", /* OSSO_GN_ERROR */
        "qgn_note_info",  /* OSSO_GN_NOTICE */
        "qgn_note_gene_wait"  /* OSSO_GN_WAIT */
    };

    /* no type, so it's required by a plugin */
    if( dialogs[dialog_id].type == -1 )
        note = hildon_note_new_information_with_icon_theme( 
            NULL, 
            dialogs[dialog_id].msg, 
            dialogs[dialog_id].icon );
    else
        note = hildon_note_new_information_with_icon_name( 
            NULL, 
            dialogs[dialog_id].msg, 
            icon[dialogs[dialog_id].type] );

    g_signal_connect( G_OBJECT( note ), "response",
                      G_CALLBACK( hildon_status_bar_lib_destroy_dialog ),
		      (gpointer)dialog_id );

    dialogs[dialog_id].visible = TRUE;
    gtk_widget_show( note );
}

static void hildon_status_bar_lib_destroy_dialog( GtkDialog *dialog, gint arg1, 
					   gpointer data )
{
    gint dialog_i = (int)data;
    gint int_type;
    
    g_return_if_fail( dialog );

    gtk_widget_destroy( GTK_WIDGET( dialog ) );

    int_type = dialogs[dialog_i].int_type; 

    /* if requested by a plugin, inform it, that the dialog is closed */
    if( dialogs[dialog_i].cb )
    {
        dialogs[dialog_i].cb( dialogs[dialog_i].int_type,
                              dialogs[dialog_i].data );
        dialogs[dialog_i].cb = NULL;
    }

    /* clear current dialog struct */
    dialogs[dialog_i].visible = FALSE;

    dialogs[dialog_i].type = 0;
    dialogs[dialog_i].int_type = 0;

    g_free( dialogs[dialog_i].msg );
    dialogs[dialog_i].msg = NULL;

    g_free( dialogs[dialog_i].icon );
    dialogs[dialog_i].icon = NULL;
    dialogs[dialog_i].data = NULL;

    /* show next dialog, if one in queue */
    if( ++dialog_i == HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS )
        dialog_i = 0;

    if( dialogs[dialog_i].msg != NULL )
        hildon_status_bar_lib_show_dialog( dialog_i );
}

void hildon_status_bar_lib_queue_dialog( const gchar *icon, const gchar *msg, 
					 gint int_type,
					 dialog_closed_cb cb, gpointer data )
{
    g_return_if_fail( cb );

    /* call the callback immediately if mandatory parameters are not given */
    if( !icon || !msg )
    {
        cb( int_type, data );
        return;
    }

    hildon_status_bar_lib_prepare_dialog( -1, icon, msg, int_type, cb, data );
}

void hildon_status_bar_lib_prepare_dialog( gint type, const gchar *icon, 
					   const gchar *msg, gint int_type,
					   dialog_closed_cb cb, gpointer data )
{
    gboolean queue = TRUE;
    gboolean overwrite = FALSE;
    gint dialog_id = dialog_index;

    /* no messages in queue */
    if( dialogs[dialog_id].msg == NULL )
        queue = FALSE;
    else 
    {
        /* requested by a plugin */
        if( cb )
        {
            int i;
            /* search for the same plugin and same internal type */
            for( i = 0; i < HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS; ++i )
            {
                if( cb == dialogs[i].cb )
                {
                    if( int_type == dialogs[i].int_type )
                    {
                        /* If the dialog is not visible, replace it */
                        if( !dialogs[i].visible )
                        {
                            dialog_id = i;
                            overwrite = TRUE;

                            g_free( dialogs[i].msg );
                            dialogs[i].msg = NULL;

                            g_free( dialogs[i].icon );
                            dialogs[i].icon = NULL;
                            break;
                        }
                    }
                }       
            }
        }
        
        if( !overwrite && ++dialog_id == HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS )
            dialog_id = 0;
    }

    /* too many system dialogs */
    if( dialogs[dialog_id].msg != NULL )
    {
        osso_log( LOG_ERR, "Too many system dialogs. Defined maximum is %d!", 
                  HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS );

        /* tell the plugin, it's dialog is closed */
        if( type == -1 )
            cb( int_type, data );
        return;
    }

    dialogs[dialog_id].type = type;
    dialogs[dialog_id].msg = g_strdup( msg );
    dialogs[dialog_id].icon = g_strdup( icon );
    dialogs[dialog_id].int_type = int_type;
    dialogs[dialog_id].cb = cb;
    dialogs[dialog_id].data = data;
    dialogs[dialog_id].visible = FALSE;
    
    if( !overwrite )
        dialog_index = dialog_id;

    if( !queue )
        hildon_status_bar_lib_show_dialog( dialog_id );

}
