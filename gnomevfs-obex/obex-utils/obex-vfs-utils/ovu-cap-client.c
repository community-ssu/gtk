/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>
#include <glib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib-lowlevel.h>

#include "ovu-caps.h"
#include "ovu-cap-parser.h"
#include "ovu-cap-dbus.h"
#include "ovu-cap-client.h"


#define d(x)
 

static DBusConnection *cap_dbus_get_connection (gboolean create);

static DBusConnection *dbus_conn = NULL;


static DBusConnection *
cap_dbus_get_connection (gboolean create)
{
	DBusError error;
	
	if (dbus_conn) {
		return dbus_conn;
	}

	if (!create) {
		return NULL;
	}
	
	dbus_error_init (&error);
	dbus_conn = dbus_bus_get_private (DBUS_BUS_SESSION, &error);
	if (!dbus_conn) {
		g_warning ("Failed to connect to the D-BUS daemon: %s", error.message);
		
		dbus_error_free (&error);
		return NULL;
	}
	
	dbus_connection_setup_with_g_main (dbus_conn, NULL);
	
	return dbus_conn;
}

OvuCaps *
ovu_cap_client_get_caps (const gchar *uri, GError **gerror)
{
	DBusConnection *dbus_conn;
	DBusMessage    *message;
        DBusMessage    *reply;
        gboolean        ret;
        gchar          *str;
	OvuCaps        *caps;
	DBusError       error;
	
	dbus_conn = cap_dbus_get_connection (TRUE);
	if (!dbus_conn) {
		g_set_error (gerror,
			     OVU_CAP_CLIENT_ERROR,
			     OVU_CAP_CLIENT_ERROR_INTERNAL,
			     "Could not connect to dbus");
		return NULL;
	}
	
        message = dbus_message_new_method_call (CAP_SERVICE_OBEX,
                                                CAP_OBJECT_OBEX_SERVER,
                                                CAP_INTERFACE_CAPABILITY,
                                                CAP_METHOD_GET_CAPABILITIES);
	if (!message) {
		g_error ("Out of memory");
	}

	if (!dbus_message_append_args (message,
				       DBUS_TYPE_STRING, uri,
				       DBUS_TYPE_INVALID)) {
		g_error ("Out of memory");
	}

	dbus_error_init (&error);
	
        reply = dbus_connection_send_with_reply_and_block (dbus_conn,
							   message,
							   -1,
							   &error);

        dbus_message_unref (message);
	
	if (dbus_error_is_set (&error)) {
		g_set_error (gerror,
			     OVU_CAP_CLIENT_ERROR,
			     OVU_CAP_CLIENT_ERROR_INTERNAL,
			     error.message);
		dbus_error_free (&error);
		return NULL;
	}
	
        if (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_ERROR) {
		g_set_error (gerror,
			     OVU_CAP_CLIENT_ERROR,
			     OVU_CAP_CLIENT_ERROR_INTERNAL,
			     "Error: %s",
			     dbus_message_get_error_name (reply));

                dbus_message_unref (reply);
                return NULL;
        }
	
        ret = dbus_message_get_args (reply, NULL,
                                     DBUS_TYPE_STRING, &str,
                                     DBUS_TYPE_INVALID);
                                                                                        
        dbus_message_unref (reply);
	
        if (!ret) {
		g_set_error (gerror,
			     OVU_CAP_CLIENT_ERROR,
			     OVU_CAP_CLIENT_ERROR_INTERNAL,
			     "Couldn't get capablities string");
		
                return NULL;
        }

	caps = ovu_caps_parser_parse (str, -1, gerror);

        dbus_free (str);
                                                                                        
        return caps;
}

GQuark
ovu_cap_client_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0) {
		q = g_quark_from_static_string ("ovu-cap-client-error-quark");
	}
	
	return q;
}
