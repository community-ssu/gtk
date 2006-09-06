/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005-2006 Nokia Corporation.
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


/**
* @file hildon-status-bar-lib.h
* @brief
* DESCRIPTION: Status bar library functions
*/

#ifndef HILDON_STATUR_BAR_LIB_FUNCTIONS_H
#define HILDON_STATUR_BAR_LIB_FUNCTIONS_H

#include <glib.h>
#include <gtk/gtkdialog.h>

G_BEGIN_DECLS

#define HILDON_STATUS_BAR_MAX_NOTE_TYPE      3  /* highest system note type */
#define HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS 30  /* maximum number of dialogs */

typedef struct system_dialog_st SystemDialog;

typedef void (*dialog_closed_cb)( gint type, gpointer data );

struct system_dialog_st
{
    gint             type;      /* type of the dialog, -1 for plugins */
    gchar            *icon;     /* icon to be shown */
    gchar            *msg;      /* msg to be shown */
    gchar            *button;   /* text to be shown in the button */
    gint             int_type;  /* first parameter to cb */
    dialog_closed_cb cb;        /* callback */
    gpointer         data;      /* second parameter to cb */
    gboolean         occupied;  /* is this slot free */
    gboolean         save_result;  /* should the response be saved */
    gboolean         result_ready;  /* is the response ready */
    gboolean         do_not_show;  /* true, if dialog should not be showed
                                      ('closed' before it was shown) */
    gboolean         is_showing;  /* is the dialog showing now */
    gint             result;    /* response to the dialog */
    GtkWidget        *widget;   /* the dialog widget (for closing it) */
};


/**
 * @hildon_status_bar_lib_prepare_dialog
 *
 * @param *type Dialog type, must be valid osso_system_note_type_t
 * (defined in libosso.h).
 *
 * @param *icon Stock icon to show on the dialog. This parameter is
 * ignored, the type determines the icon to show.
 *
 * @param *msg Message string to show.
 *
 * @param int_type First parameter to the callback, ignored by this
 * function.
 *
 * @param cb Callback to call after dialog closes, or when there was
 * an error.
 *
 * @param data Second parameter to the callback, ignored by this
 * function.
 *
 * This function shows a system modal dialog. If there is already a
 * dialog shown, then the dialog is queued and shown later.
 */

void hildon_status_bar_lib_prepare_dialog( gint type,
                                           const gchar *icon, 
                                           const gchar *msg,
                                           gint int_type, 
                                           dialog_closed_cb cb,
                                           gpointer data );
/**
 * @hildon_status_bar_lib_queue_dialog
 *
 * @param *icon Stock icon to show on the dialog.
 * 
 * @param *msg Message string to show.
 * 
 * @param int_type First parameter to the callback, ignored by this
 * function.
 *
 * @param *cb Callback to be called after dialog is closed. It is also
 * called if there is an error.
 *
 * @param data Second parameter to the callback, ignored by this
 * function.
 *
 * This function shows a system modal dialog. If there is already a
 * dialog shown, then the dialog is queued and shown later. This is
 * almost identical to hildon_status_bar_lib_prepare_dialog but meant
 * for plugins.
 */

void hildon_status_bar_lib_queue_dialog( const gchar *icon,
                                         const gchar *msg,
                                         gint int_type,
                                         dialog_closed_cb cb,
                                         gpointer data );

/**
 * @hildon_status_bar_lib_open_closeable_dialog
 *
 * @param *type Dialog type, must be valid osso_system_note_type_t
 * (defined in libosso.h).
 * @param *msg Message string to show.
 * @param *btext Text shown on the button, or NULL for default text.
 * @param save_result Whether the response value should be saved. If this
 * is TRUE, the caller must call hildon_status_bar_lib_get_dialog_response
 * to get the value, even if the value is not interesting anymore.
 * @return Id of the dialog or -1 on error. This id can be used to close
 * the dialog with hildon_status_bar_lib_close_closeable_dialog.
 * 
 * This function shows a system modal dialog. If there is already a
 * dialog shown, then the dialog is queued and shown later.
 * The dialog can be closed by calling
 * hildon_status_bar_lib_close_closeable_dialog (if it was not already
 * closed by the user).
 *
 * NOTICE: This API is not stable.
 */

gint hildon_status_bar_lib_open_closeable_dialog( gint type,
                                                  const gchar *msg,
                                                  const gchar *btext,
                                                  gboolean save_result );

/**
 * @hildon_status_bar_lib_close_closeable_dialog
 *
 * @param id Dialog to close.
 * @return 
 * 
 * This function closes a system modal dialog opened with
 * hildon_status_bar_lib_open_closeable_dialog, if it was not closed
 * by the user already.
 *
 * NOTICE: This API is not stable.
 */

void hildon_status_bar_lib_close_closeable_dialog( gint id );

/**
 * @hildon_status_bar_lib_get_dialog_response
 *
 * @param id Dialog id.
 * @return Response value (<0), or 2 if the dialog is not closed, or 1 if
 * the id is invalid (e.g. it was not saved for the dialog, or the value
 * has been asked already).
 * 
 * This function returns the response value of the dialog (value telling
 * what button was pressed). The value can be asked only once.
 *
 * NOTICE: This API is not stable.
 */

gint hildon_status_bar_lib_get_dialog_response( gint id );

G_END_DECLS

#endif /* HILDON_STATUR_BAR_LIB_FUNCTIONS_H */
