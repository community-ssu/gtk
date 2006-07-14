/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005-2006 Nokia Corporation.
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
 
#include <string.h> /* for memset */
/* Hildon includes */
#include "hildon-status-bar-lib.h"

#include <hildon-widgets/hildon-note.h>

#include <gtk/gtkdialog.h>
#include <glib.h>
#include <libosso.h>

/* log include */
#include <log-functions.h>

/* Queue for dialogs is a rotating buffer.
 * Dialogs are shown in the FIFO order. */
static SystemDialog dialog_queue[HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS];
static gint next_free_index = 0;
static gboolean showing_dialog = FALSE;

static void hildon_status_bar_lib_show_dialog( gint dialog_id );
static void hildon_status_bar_lib_destroy_dialog( GtkDialog *dialog,
                                                  gint arg1, 
                                                  gpointer data );
static gint hildon_status_bar_lib_append_to_queue( gint type,
                                                   const gchar *icon,
                                                   const gchar *msg,
                                                   gint int_type,
                                                   dialog_closed_cb cb,
                                                   const gchar *btext,
                                                   gboolean save_result,
                                                   gpointer data );


static void hildon_status_bar_lib_show_dialog( gint id )
{
    /* These must match with the D-BUS system note types */
    /* osso_system_note_type_t (defined in libosso.h) */
    static const gchar *icon[HILDON_STATUS_BAR_MAX_NOTE_TYPE + 1] = {
        "qgn_note_gene_syswarning",   /* OSSO_GN_WARNING */
        "qgn_note_gene_syserror",     /* OSSO_GN_ERROR */
        "qgn_note_info",              /* OSSO_GN_NOTICE */
        "qgn_note_gene_wait"          /* OSSO_GN_WAIT */
    };

    g_assert( id < HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS && id >= 0 );
    g_assert( dialog_queue[id].occupied );

    if( dialog_queue[id].is_showing )
    {
        osso_log( LOG_DEBUG, "Tried to show dialog that is already shown" );
        return;
    }

    if( dialog_queue[id].do_not_show )
    {
        osso_log( LOG_DEBUG, "Tried to show a do_not_show dialog" );
        return;
    }

    /* create the widget if it does not exist */
    if( dialog_queue[id].widget == NULL )
    {
        if( dialog_queue[id].type == -1 )
        {
            /* no type, a plugin is showing the dialog */
            dialog_queue[id].widget =
                hildon_note_new_information_with_icon_theme( NULL, 
                    dialog_queue[id].msg, dialog_queue[id].icon );
        }
        else
        {
            dialog_queue[id].widget =
                hildon_note_new_information_with_icon_name( NULL, 
                    dialog_queue[id].msg, icon[dialog_queue[id].type] );
        }

        if( dialog_queue[id].button != NULL )
        {
            hildon_note_set_button_text( (HildonNote*)
                                         dialog_queue[id].widget,
                                         dialog_queue[id].button );
        }

        g_signal_connect( G_OBJECT( dialog_queue[id].widget ), "response",
            G_CALLBACK( hildon_status_bar_lib_destroy_dialog ),
            (gpointer)id );
    }

    if( showing_dialog )
    {
        /* We are currently showing a dialog, don't show it yet. */
        osso_log( LOG_DEBUG, "Some dialog is already shown" );
    }
    else
    {
        gtk_widget_show( dialog_queue[id].widget );
        showing_dialog = TRUE;
        dialog_queue[id].is_showing = TRUE;
    }
}

#define NEXT_ID if (++id == HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS) { \
                    id = 0; \
                }

static void hildon_status_bar_lib_finalize_dialog( gint id,
                                                   gint response )
{
    g_assert( id < HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS && id >= 0 );
    g_assert( dialog_queue[id].occupied );

    osso_log( LOG_DEBUG, "finalizing dialog %d", id );
    
    if( dialog_queue[id].widget != NULL )
    {
        gtk_widget_destroy( GTK_WIDGET( dialog_queue[id].widget ) );
        dialog_queue[id].widget = NULL;
    }
    g_free( dialog_queue[id].msg );
    dialog_queue[id].msg = NULL;
    g_free( dialog_queue[id].icon );
    dialog_queue[id].icon = NULL;
    g_free( dialog_queue[id].button );
    dialog_queue[id].button = NULL;

    dialog_queue[id].do_not_show = TRUE;

    /* it could be created but not showing */
    if( dialog_queue[id].is_showing )
    {
        g_assert( showing_dialog );
        showing_dialog = FALSE;
        dialog_queue[id].is_showing = FALSE;
    }

    /* inform that the dialog is closed (or 'closed' before shown) */
    if( dialog_queue[id].cb != NULL )
    {
        /* TODO: What if these dialog functions are called from
         * this callback? */
        dialog_queue[id].cb( dialog_queue[id].int_type,
                             dialog_queue[id].data );
    }

    if( dialog_queue[id].save_result )
    {
        /* don't clear yet, the result is collected later */
        dialog_queue[id].result = response;
        dialog_queue[id].result_ready = TRUE;
    }
    else
    {
        /* clear current dialog struct (sets occupied to FALSE) */
        memset( &dialog_queue[id], 0, sizeof( SystemDialog ) );
    }

    if( !showing_dialog )
    {
        /* show next dialog, if there is one in the queue */
        NEXT_ID
        while( dialog_queue[id].occupied )
        {
            /* it's possible that dialog is 'closed' before it's shown */
            if( !dialog_queue[id].do_not_show )
            {
                hildon_status_bar_lib_show_dialog( id );
                break;
            }
            NEXT_ID
        }
    }
}

static void hildon_status_bar_lib_destroy_dialog( GtkDialog *dialog,
                                                  gint arg1, 
                                                  gpointer data )
{
    gint id = (gint)data;

    g_assert( dialog != NULL );
    g_assert( id < HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS && id >= 0 );
    g_assert( dialog_queue[id].occupied );
    g_assert( dialog_queue[id].widget != NULL );
    g_assert( GTK_WIDGET( dialog ) == dialog_queue[id].widget );

    hildon_status_bar_lib_finalize_dialog( id, arg1 );
}

static gint hildon_status_bar_lib_append_to_queue( gint type,
                                                   const gchar *icon,
                                                   const gchar *msg,
                                                   gint int_type,
                                                   dialog_closed_cb cb,
                                                   const gchar *btext,
                                                   gboolean save_result,
                                                   gpointer data )
{
    gint id = next_free_index;

    if( dialog_queue[id].occupied )
    {
        osso_log( LOG_ERR, "Dialog queue overflow" );
        return -1;
    }

    memset( &dialog_queue[id], 0, sizeof( SystemDialog ) );
    dialog_queue[id].type = type;
    dialog_queue[id].icon = icon != NULL ? g_strdup( icon ) : NULL;
    dialog_queue[id].msg = msg != NULL ? g_strdup( msg ) : NULL;
    dialog_queue[id].button = btext != NULL ? g_strdup( btext ) : NULL;
    dialog_queue[id].int_type = int_type;
    dialog_queue[id].cb = cb;
    dialog_queue[id].data = data;
    dialog_queue[id].occupied = TRUE;
    dialog_queue[id].save_result = save_result;
    dialog_queue[id].result_ready = FALSE;
    dialog_queue[id].do_not_show = FALSE;
    dialog_queue[id].is_showing = FALSE;
    dialog_queue[id].result = -1;
    dialog_queue[id].widget = NULL;

    if( ++next_free_index == HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS )
    {
        next_free_index = 0;
    }
    return id;
}

void hildon_status_bar_lib_queue_dialog( const gchar *icon,
                                         const gchar *msg, gint int_type,
                                         dialog_closed_cb cb,
                                         gpointer data )
{
    gint id = -1;

    if( icon == NULL || msg == NULL || cb == NULL )
    {
        osso_log( LOG_ERR, "Invalid arguments" );
        if( cb != NULL )
        {
            cb( int_type, data );
        }
        return;
    }

    id = hildon_status_bar_lib_append_to_queue( -1, icon, msg, int_type,
                                                cb, NULL, FALSE, data );
    if( id == -1 )
    {
        cb( int_type, data );
        return;
    }

    hildon_status_bar_lib_show_dialog( id );
}

void hildon_status_bar_lib_prepare_dialog( gint type, const gchar *icon, 
                                           const gchar *msg, gint int_type,
                                           dialog_closed_cb cb,
                                           gpointer data )
{
    gint id = -1;

    if( msg == NULL || type > HILDON_STATUS_BAR_MAX_NOTE_TYPE || type < 0 )
    {
        osso_log( LOG_ERR, "Invalid arguments" );
        if( cb != NULL )
        {
            cb( int_type, data );
        }
        return;
    }

    id = hildon_status_bar_lib_append_to_queue( type, icon, msg, int_type,
                                                cb, NULL, FALSE, data );
    if( id == -1 )
    {
        if( cb != NULL )
        {
            cb( int_type, data );
        }
        return;
    }

    hildon_status_bar_lib_show_dialog( id );
}

gint hildon_status_bar_lib_open_closeable_dialog( gint type,
                                                  const gchar *msg,
                                                  const gchar *btext,
                                                  gboolean save_result )
{
    gint id = -1;

    if( msg == NULL || type > HILDON_STATUS_BAR_MAX_NOTE_TYPE || type < 0 )
    {
        osso_log( LOG_ERR, "Invalid arguments" );
        return -1;
    }

    id = hildon_status_bar_lib_append_to_queue( type, NULL, msg, -1,
                                                NULL, btext, save_result,
                                                NULL );
    if( id == -1 )
    {
        return -1;
    }

    hildon_status_bar_lib_show_dialog( id );
    return id;
}

void hildon_status_bar_lib_close_closeable_dialog( gint id )
{
    if( id >= HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS || id < 0 ||
        !dialog_queue[id].occupied )
    {
        osso_log( LOG_ERR, "Invalid dialog id %d given as argument", id );
        return;
    }

    if( dialog_queue[id].do_not_show )
    {
        osso_log( LOG_DEBUG, "Dialog %d is already closed", id );
    }
    else
    {
        hildon_status_bar_lib_finalize_dialog( id, GTK_RESPONSE_CLOSE );
    }
}

gint hildon_status_bar_lib_get_dialog_response( gint id )
{
    if( id >= HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS || id < 0 ||
        !dialog_queue[id].occupied )
    {
        osso_log( LOG_ERR, "Invalid dialog id %d given as argument", id );
        return 1;
    }

    if( dialog_queue[id].save_result )
    {
        if( dialog_queue[id].result_ready )
        {
            gint result = dialog_queue[id].result;
            /* clear current dialog struct (sets occupied to FALSE) */
            memset( &dialog_queue[id], 0, sizeof( SystemDialog ) );
            return result;
        }
        else
        {
            osso_log( LOG_DEBUG, "Dialog %d is not closed yet", id );
            return 2;
        } 
    }
    else
    {
        osso_log( LOG_ERR, "Dialog %d response is not saved", id );
        return 1;
    }
}
