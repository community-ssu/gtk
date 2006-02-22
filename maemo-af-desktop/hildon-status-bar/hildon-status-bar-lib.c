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
    osso_log( LOG_DEBUG, "next_free_index = %d", next_free_index );
    osso_log( LOG_DEBUG, "showing_dialog = %d", showing_dialog );

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

        g_signal_connect( G_OBJECT( dialog_queue[id].widget ), "response",
            G_CALLBACK( hildon_status_bar_lib_destroy_dialog ),
            (gpointer)id );
    }

    if( showing_dialog )
    {
        /* We are currently showing a dialog, don't show it yet. */
        osso_log( LOG_DEBUG, "Some dialog is already shown" );
        return;
    }
    gtk_widget_show( dialog_queue[id].widget );
    showing_dialog = TRUE;
}

static void hildon_status_bar_lib_destroy_dialog( GtkDialog *dialog,
                                                  gint arg1, 
                                                  gpointer data )
{
    gint id = (gint)data;

    g_assert( id < HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS && id >= 0 );
    g_assert( showing_dialog == TRUE );
    if( dialog == NULL )
    {
        osso_log( LOG_ERR, "Invalid arguments" );
        return;
    }
    osso_log( LOG_DEBUG, "destroying dialog %d", id );

    g_assert( GTK_WIDGET( dialog ) == dialog_queue[id].widget );
    gtk_widget_destroy( GTK_WIDGET( dialog ) );
    showing_dialog = FALSE;

    /* inform that the dialog is closed */
    if( dialog_queue[id].cb != NULL )
    {
        dialog_queue[id].cb( dialog_queue[id].int_type,
                             dialog_queue[id].data );
    }

    /* clear current dialog struct (memset sets occupied to FALSE) */
    g_free( dialog_queue[id].msg );
    g_free( dialog_queue[id].icon );
    memset( &dialog_queue[id], 0, sizeof( SystemDialog ) );

    /* show next dialog, if there is one in the queue */
    if( ++id == HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS )
    {
        id = 0;
    }

    if( dialog_queue[id].occupied )
    {
        hildon_status_bar_lib_show_dialog( id );
    }
}

static gint hildon_status_bar_lib_append_to_queue( gint type,
                                                   const gchar *icon,
                                                   const gchar *msg,
                                                   gint int_type,
                                                   dialog_closed_cb cb,
                                                   gpointer data )
{
    gint id = next_free_index;

    if( dialog_queue[id].occupied )
    {
        osso_log( LOG_ERR, "Dialog queue overflow" );
        return -1;
    }

    dialog_queue[id].type = type;
    dialog_queue[id].icon = g_strdup( icon );
    dialog_queue[id].msg = g_strdup( msg );
    dialog_queue[id].int_type = int_type;
    dialog_queue[id].cb = cb;
    dialog_queue[id].data = data;
    dialog_queue[id].occupied = TRUE;
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
                                                cb, data );
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
                                                cb, data );
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
                                                  const gchar *msg )
{
    gint id = -1;

    if( msg == NULL || type > HILDON_STATUS_BAR_MAX_NOTE_TYPE || type < 0 )
    {
        osso_log( LOG_ERR, "Invalid arguments" );
        return -1;
    }

    id = hildon_status_bar_lib_append_to_queue( type, NULL, msg, -1,
                                                NULL, NULL );
    if( id == -1 )
    {
        return -1;
    }

    hildon_status_bar_lib_show_dialog( id );
    return id;
}

void hildon_status_bar_lib_close_closeable_dialog( gint id )
{
    if( id >= HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS || id < 0 )
    {
        osso_log( LOG_ERR, "Invalid dialog id given as argument" );
        return;
    }

    if( !showing_dialog || !dialog_queue[id].occupied )
    {
        osso_log( LOG_DEBUG, "Dialog is already closed" );
        return;
    }

    g_assert( dialog_queue[id].widget != NULL );

    g_signal_emit_by_name( dialog_queue[id].widget, "response" );
}

