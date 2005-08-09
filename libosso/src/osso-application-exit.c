/**
 * @file osso-application-exit.c
 * This file implements functionality related the exit and exit_if_possible
 * signals.
 * 
 * Copyright (C) 2005 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "libosso.h"
#include "osso-internal.h"

#define EXIT_INTERFACE "com.nokia.osso_app_killer"
#define EXIT_SIGNAL "exit"
#define EXIT_IF_POSSIBLE_SIGNAL "exit_if_possible"
#define MATCH_RULE "type='signal',interface='com.nokia.osso_app_killer'"

static DBusHandlerResult _exit_handler(osso_context_t *osso, DBusMessage *msg,
       gpointer data);

osso_return_t osso_application_set_exit_cb(osso_context_t *osso,
					   osso_application_exit_cb *cb,
					   gpointer data)
{
    if ( (osso == NULL) || (cb == NULL) ) {
	ULOG_ERR_F("error: Null pointer to cb function (%p) or osso (%p) given",
		   cb, osso);
	return OSSO_INVALID;
    }
    if (osso->sys_conn == NULL) {
	ULOG_ERR_F("error: no D-BUS connection!");
	return OSSO_INVALID;
    }
    
    dbus_bus_add_match (osso->conn, MATCH_RULE, NULL);
    dbus_bus_add_match (osso->sys_conn, MATCH_RULE, NULL);
    
    osso->exit.cb = cb;
    osso->exit.data = data;

    _msg_handler_set_cb_f(osso, EXIT_INTERFACE,
			  _exit_handler, NULL, FALSE);

    dbus_connection_flush(osso->conn);
    return OSSO_OK;
}

/************************************************************************/
static DBusHandlerResult _exit_handler(osso_context_t *osso, DBusMessage *msg,
       gpointer data)
{
    DBusHandlerResult ret=DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    if(strcmp(dbus_message_get_interface(msg),EXIT_INTERFACE)==0) {
	
	if( dbus_message_is_signal(msg, EXIT_INTERFACE, EXIT_SIGNAL) == TRUE )
	{
	    if(osso->exit.cb != NULL)
		osso->exit.cb(TRUE, osso->exit.data);
	    ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if( dbus_message_is_signal(msg, EXIT_INTERFACE,
					EXIT_IF_POSSIBLE_SIGNAL) == TRUE )
	{
	    if(osso->exit.cb != NULL)
		osso->exit.cb(FALSE, osso->exit.data);
	    ret =  DBUS_HANDLER_RESULT_HANDLED;
	}
    }
    return ret;
}
