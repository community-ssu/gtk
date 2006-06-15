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
#include "ovu-cap-server.h"

#define d(x)


static DBusConnection *  cap_server_get_connection    (void);
static void              cap_server_unregistered_func (DBusConnection *dbus_conn,
						       gpointer        data);
static DBusHandlerResult cap_server_message_func      (DBusConnection *dbus_conn,
						       DBusMessage    *message,
						       gpointer        data);


static DBusObjectPathVTable server_vtable = {
	cap_server_unregistered_func,
	cap_server_message_func,
	NULL
};

static DBusConnection *server_conn = NULL;


static DBusConnection *
cap_server_get_connection (void)
{
	DBusError error;
	
	dbus_error_init (&error);
	server_conn = dbus_bus_get_private (DBUS_BUS_SESSION, &error);
	if (!server_conn) {
		g_warning ("Failed to connect to the D-BUS daemon: %s", error.message);
		
		dbus_error_free (&error);
		return NULL;
	}
	
	return server_conn;
}

static gpointer
main_loop_thread (gpointer data)
{
	GMainLoop *loop = data;

	g_main_loop_run (loop);

	return NULL;
}	

gboolean
ovu_cap_server_init (OvuGetCapsFunc func)
{
	DBusConnection *dbus_conn;
	GMainContext   *context;
	GMainLoop      *loop;

	gnome_vfs_init ();
	
        dbus_conn = cap_server_get_connection ();
        if (!dbus_conn) {
                return FALSE;
        }

	if (!dbus_bus_request_name (dbus_conn, CAP_SERVICE_OBEX, 0, NULL)) {
		g_printerr ("Failed to acquire obex cap service.\n");
		return FALSE;
	}

	if (!dbus_connection_register_object_path (dbus_conn,
						   CAP_OBJECT_OBEX_SERVER,
						   &server_vtable,
						   func)) {
		g_printerr ("Failed to register object with D-BUS.\n");
		return FALSE;
	}
	
	context = g_main_context_new ();
	loop = g_main_loop_new (context, FALSE);
	
	dbus_connection_setup_with_g_main (dbus_conn, context);

	g_thread_create (main_loop_thread, loop, FALSE, NULL);
	
        return TRUE;
}

static void
cap_server_unregistered_func (DBusConnection *dbus_conn,
			      gpointer        data)
{
}

static DBusHandlerResult
cap_server_message_func (DBusConnection *dbus_conn,
			 DBusMessage    *message,
			 gpointer        data)
{
	OvuGetCapsFunc  func = data;
        DBusMessage    *reply;
        gchar          *dev;
        gchar          *str;
	gint            len;
	GnomeVFSURI    *uri;

	if (!dbus_message_is_method_call (message,
					  CAP_INTERFACE_CAPABILITY,
					  CAP_METHOD_GET_CAPABILITIES)){
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	reply = NULL;
	if (dbus_message_get_args (message, NULL,
				   DBUS_TYPE_STRING, &dev,
				   DBUS_TYPE_INVALID)) {
		
		uri = gnome_vfs_uri_new (dev);

		if (func && func (uri, &str, &len)) {
			reply = dbus_message_new_method_return (message);
			if (!reply) {
				g_error ("Out of memory");
			}
			
			if (!dbus_message_append_args (reply, 
						       DBUS_TYPE_STRING, &str,
						       DBUS_TYPE_INVALID)) {
				g_error ("Out of memory");
			}
			
			g_free (str);
		}
		
		gnome_vfs_uri_unref (uri);
	}
	
	if (!reply) { 
		reply = dbus_message_new_error (message,
						CAP_ERROR_FAILED,
						"Could not get capabilities string");
		if (!reply) {
			g_error ("Out of memory");
		}
	}
	
	dbus_connection_send (dbus_conn, reply, NULL);
	dbus_message_unref (reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}
 


