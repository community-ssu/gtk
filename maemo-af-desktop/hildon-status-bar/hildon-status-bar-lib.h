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


/**
* @file hildon-status-bar-lib.h
* @brief
* DESCRIPTION: Status bar library functions
*/

#ifndef HILDON_STATUR_BAR_LIB_FUNCTIONS_H
#define HILDON_STATUR_BAR_LIB_FUNCTIONS_H

#include <glib.h>

G_BEGIN_DECLS

#define HILDON_STATUS_BAR_MAX_NOTE_TYPE      4  /* highest system note type */
#define HILDON_STATUS_BAR_MAX_NO_OF_DIALOGS 30  /* maximum number of dialogs */

typedef struct system_dialog_st SystemDialog;

typedef void (*dialog_closed_cb)( gint type, gpointer data );

struct system_dialog_st
{
    gint             type;   /* type of the dialog, -1 for plugins */
    gchar            *icon;  /* icon to be shown */
    gchar            *msg;   /* msg to be shown */
    gint             int_type; /* plugin's internal dialog type */
    dialog_closed_cb cb;     /* callback */
    gpointer         data;   /* data to be passed to cb */
    gboolean         visible; /* is this dialog visible at the moment */
};



/**
 * @hildon_status_bar_lib_prepare_dialog
 *
 * @param *type describing dialog type
 *
 * @param *icon stock icon used
 *
 * @param *msg message to show
 *
 * @param int_type internal type
 *
 * @param cb callback to call after dialog closes
 *
 * @param data data to forward to callback function
 *
 * This function prepares the dialog and shows it
 */

void hildon_status_bar_lib_prepare_dialog( gint type, const gchar *icon, 
                                           const gchar *msg, gint int_type, 
                                           dialog_closed_cb cb, gpointer data );
/**
 * @hildon_status_bar_lib_queue_dialog
 *
 * @param *icon stock icon used
 * 
 * @param *msg  message to show
 * 
 * @param *int_type plugin's internal type
 *
 * @param *cb callback to be called after dialog is closed
 *
 * @param data data passed to callback
 *
 * The dialog will be added to queue and will be shown later.
 */

void hildon_status_bar_lib_queue_dialog( const gchar *icon,  /* stock icon */
                                         const gchar *msg,   /* msg to show */
                                         gint int_type,     /* plugin's internal
                                                               msg type */
                                         dialog_closed_cb cb,/* cb to be called
                                                              * after the dialog
                                                              * is closed */
                                         gpointer data );    /* data passed to 
                                                              * cb */


G_END_DECLS

#endif /* HILDON_STATUR_BAR_LIB_FUNCTIONS_H */
