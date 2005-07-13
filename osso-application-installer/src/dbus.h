/**
   @file dbus.h
   
   Header for DBUS handling functions
   <p>
*/

/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */


#ifndef AI_DBUS_H
#define AI_DBUS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <libosso.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <osso-log.h>

#include "appdata.h"

/* Application Installer definitions */
#define OSSO_AI_SERVICE "com.nokia.osso_application_installer"
#define OSSO_AI_INTERFACE "com.nokia.osso_application_installer"
#define OSSO_AI_OBJECT_PATH "/com/nokia/osso_application_installer"


/* Mime open defines */
#define DBUS_METHOD_MIME_OPEN "mime_open"


/* Define d-bus messages, to which application have to react */
/* For instance game application needs methods to start, continue, restart, 
   close and pause the game */

#define OSSO_AI_RUN_METHOD "osso-application-installer-run"
#define OSSO_AI_CONTINUE_METHOD "osso-application-installer-continue"
#define OSSO_AI_RESTART_METHOD "osso-application-installer-restart"
#define OSSO_AI_CLOSE_METHOD "osso-application-installer-close"
#define OSSO_AI_PAUSE_METHOD "osso-application-installer-pause"

/* Testing d-bus messaging with displaying infoprint */
#define OSSO_AI_DISPLAY_INFOPRINT "osso_application_installer_display_infoprint"

gboolean set_dbus_handler(osso_rpc_cb_f *func);

/**
 Send DBUS message

 @param app Application 
 @param method Method to be called.
 @param args A GArray of osso_rpc_t structures.
 @param retval The return value of the method.
 @param app_data Application specific data
*/
osso_return_t send_dbus_message(const gchar *app,
                                 const gchar *method,
				                             GArray *args,
				                             osso_rpc_t *retval, 
                                 AppData *app_data);


/** 
 Receive D-BUS messages and handles them

 @param interface The interface of the called method.
 @param method The method that was called.
 @param arguments A GArray of osso_rpc_t_structures.
 @param data An application specific pointer.
 @param retval The return value of the method.
 @returns gint value
*/
gint dbus_req_handler(const gchar *interface, 
		                     const gchar *method,
		                     GArray *arguments, 
                       gpointer app_data, 
		                     osso_rpc_t *retval);


/* Handles incoming D-BUS message

 @param method The method that was called.
 @param arguments A GArray of osso_rpc_t structures.
 @param data An application specific pointer.
 @param retval The return value of the method.
 @returns gint value
*/
gint dbus_message_handler(const gchar *method, 
		                      	  GArray *arguments,
			                        gpointer app_data, 
			                        osso_rpc_t *retval);


/**
 Set hardware event handler

 @param state Signals that are of interest
 @param cb Callback function.
 @param data Application specific data.
*/
gboolean set_hw_event_handler(osso_hw_state_t *state, 
			                            osso_hw_cb_f *cb,
			                            gpointer data);


/** 
 Handles hardware events.

 @todo Just a skeleton
 @param state State occured.
 @param data Application specific data.
*/
void hw_event_handler(osso_hw_state_t *state, gpointer data);

/**
 Osso topping callback
 
 @param arguments Extra parameters
 @param app_data Application specific data
*/
void osso_top_callback(const gchar *arguments, AppData *app_data);

/**
 Initialize osso

 @param app Application specific data
 @returns TRUE on success, FALSE otherwise
*/
gboolean init_osso( AppData *app_data);

/**
 Deinitialize osso

 @returns TRUE on success, FALSE otherwise
*/
gboolean deinit_osso(AppData *app_data);

#endif /* AI_DBUS_H */
