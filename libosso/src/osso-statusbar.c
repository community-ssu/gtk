/**
 * @file osso-statusbar.c
 * This file implements statusbar functions
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

#include "osso-internal.h"

#define STATUSBAR_SERVICE "com.nokia.statusbar"
#define STATUSBAR_OBJECT_PATH "/com/nokia/statusbar"
#define STATUSBAR_INTERFACE "com.nokia.statusbar"

osso_return_t osso_statusbar_send_event(osso_context_t *osso,
					const gchar *name,
					gint argument1, gint argument2,
					const gchar *argument3,
					osso_rpc_t *retval)
{
    gint r;
    int dbus_type_string = DBUS_TYPE_STRING;
    

    if( (osso == NULL) || (name == NULL) )
	return OSSO_INVALID;
    

    r = osso_rpc_run_system (osso,
			     STATUSBAR_SERVICE, 
			     STATUSBAR_OBJECT_PATH,
			     STATUSBAR_INTERFACE,
			     "event", retval,
			     DBUS_TYPE_STRING, name,
			     DBUS_TYPE_INT32, argument1,
			     DBUS_TYPE_INT32, argument2,
			     dbus_type_string, argument3,
			     DBUS_TYPE_INVALID);
    return r;
}
