/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2006 Nokia Corporation.
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
#include <ctype.h>
#include <string.h>
#include <gw-obex.h>
#include <bt-dbus.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib-lowlevel.h>

#include "om-dbus.h"

#define d(x) x

/* Gwcond communication. Talks to gwcond to get the device.
 *
 * This API can be accessed from any thread, since it creates it's own context
 * and uses that for the dbus connection.
 */

typedef struct {
	DBusConnection *dbus_conn;
	GMainContext   *context;
	GMainLoop      *loop;
} Connection;

/* We will try to disconnect busy devices, but only before they are used, in
 * case they were used while the daemon was killed.
 */
G_LOCK_DEFINE (used_devs);
static GHashTable *used_devs = NULL;


static Connection *
get_gwcond_connection (void)
{
	DBusConnection *dbus_conn;
	Connection     *conn;
	DBusError       error;

	/* NOTE: We are using dbus_bus_get_private here, for the reason that
	 * need to get our own private dbus connection to avoid threading
	 * problems with other libraries or applications that use this module
	 * and dbus (the vfs daemon in particular).
	 */
	dbus_error_init (&error);
        dbus_conn = dbus_bus_get_private (DBUS_BUS_SYSTEM, &error);	
	
	if (!dbus_conn) {
		g_printerr ("Failed to connect to the D-BUS daemon: %s", error.message);
		
		dbus_error_free (&error);
		return NULL;
	}

	conn = g_new0 (Connection, 1);
	
	conn->context = g_main_context_new ();
	conn->loop = g_main_loop_new (conn->context, FALSE);

	conn->dbus_conn = dbus_conn;

	dbus_connection_setup_with_g_main (dbus_conn, conn->context);
	
	return conn;
}

static void
connection_free (Connection *conn)
{
	dbus_connection_disconnect (conn->dbus_conn);
	dbus_connection_unref (conn->dbus_conn);
	
	g_main_loop_unref (conn->loop);
	g_main_context_unref (conn->context);
	
	g_free (conn);
}

/* This assumes that the connection is setup and that the bda has been checked
 * for correctness.
 */
static void
send_cancel_connect (Connection  *conn,
		     const gchar *bda,
		     const gchar *profile)
{
	DBusMessage *message;

	d(g_printerr ("obex: Send cancel connect.\n"));
	
	message = dbus_message_new_method_call (BTCOND_SERVICE,
						BTCOND_REQ_PATH,
						BTCOND_REQ_INTERFACE,
						BTCOND_RFCOMM_CANCEL_CONNECT_REQ);
	if (!message) {
		g_error ("Out of memory");
	}

	if (!dbus_message_append_args (message,
				       DBUS_TYPE_STRING, &bda,
				       DBUS_TYPE_STRING, &profile,
				       DBUS_TYPE_INVALID)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->dbus_conn, message, NULL);
	dbus_message_unref (message);
}

static void
send_disconnect (Connection *conn, const gchar *str, const gchar *profile)
{
	DBusMessage *message;
	DBusMessage *reply;

	d(g_printerr ("obex: Send disconnect.\n"));
	
	message = dbus_message_new_method_call (BTCOND_SERVICE,
						BTCOND_REQ_PATH,
						BTCOND_REQ_INTERFACE,
						BTCOND_RFCOMM_DISCONNECT_REQ);
	if (!message) {
		g_error ("Out of memory");
	}
	
	if (!dbus_message_append_args (message,
				       DBUS_TYPE_STRING, &str,
				       DBUS_TYPE_INVALID)) {
		g_error ("Out of memory");
	}

	/* The method support two signatures, one string (dev), or two strings
	 * (bda and profile).
	 */
	if (profile) {
		if (!dbus_message_append_args (message,
					       DBUS_TYPE_STRING, &profile,
					       DBUS_TYPE_INVALID)) {
			g_error ("Out of memory");
		}
	}
	
	reply = dbus_connection_send_with_reply_and_block (conn->dbus_conn,
							   message, -1, NULL);
	
	dbus_message_unref (message);

	if (reply) {
		dbus_message_unref (reply);
	}
}

static gboolean 
send_disconnect_if_first (Connection *conn, const gchar *str, const gchar *profile)
{
	G_LOCK (used_devs);

	if (!used_devs) {
		used_devs = g_hash_table_new (g_str_hash, g_str_equal);
	}

	if (g_hash_table_lookup (used_devs, str)) {
		d(g_printerr ("obex: %s has already been used, don't disconnect.\n", str));
		G_UNLOCK (used_devs);
		return FALSE;
	}

	g_hash_table_insert (used_devs, g_strdup (str), GINT_TO_POINTER (TRUE));

	G_UNLOCK (used_devs);

	d(g_printerr ("obex: %s has not been used yet, disconnect.\n", str));
	
	send_disconnect (conn, str, profile);

	return TRUE;
}

static gboolean
check_bda (const gchar *bda)
{
	gint len, i;

	if (!bda) {
		return FALSE;
	}

	len = strlen (bda);
	if (len != 17) {
		return FALSE;
	}
	
	for (i = 0; i < 17; i += 3) {
		if (!isxdigit (bda[i])) {
			return FALSE;
		}
		if (!isxdigit (bda[i+1])) {
			return FALSE;
		}
		if (i < 15 && bda[i+2] != ':') {
			return FALSE;
		}
	}
	
	return TRUE;
}

/* Note: This needs to be refactored for the next version, we need our own
 * return value here, which can include invalid profile and already connected as
 * results. We can also move the bda checking to the caller, it only needs
 * checked once.
 */
static gchar *
get_dev (Connection     *conn,
	 const gchar    *bda,
	 const gchar    *profile,
	 GnomeVFSResult *result,
	 gboolean       *invalid_profile,
	 gboolean       *already_connected)
{
	DBusMessage *message;
	DBusError    dbus_error;
	DBusMessage *reply;
	gboolean     ret;
	gchar       *str;
	gchar       *dev;
	gboolean     b;

	*result = GNOME_VFS_OK;
	*invalid_profile = FALSE;
	*already_connected = FALSE;
	
	if (!check_bda (bda)) {
		*result = GNOME_VFS_ERROR_INVALID_URI;
		return NULL;
	}

	message = dbus_message_new_method_call (BTCOND_SERVICE,
						BTCOND_REQ_PATH,
						BTCOND_REQ_INTERFACE,
						BTCOND_RFCOMM_CONNECT_REQ);
	if (!message) {
		g_error ("Out of memory");
	}

	b = FALSE;
	if (!dbus_message_append_args (message,
				       DBUS_TYPE_STRING, &bda,
				       DBUS_TYPE_STRING, &profile,
				       DBUS_TYPE_BOOLEAN, &b,
				       DBUS_TYPE_INVALID)) {
		g_error ("Out of memory");
	}

	d(g_printerr ("obex: Send connect.\n"));

	dbus_error_init (&dbus_error);
	reply = dbus_connection_send_with_reply_and_block (conn->dbus_conn,
							   message, -1,
							   &dbus_error);
	
	dbus_message_unref (message);

	/* The errors according to the documentation of osso-gwconnect 0.67 can be:

	"com.nokia.btcond.error.invalid_dev" 	No such device selected
	"com.nokia.btcond.error.invalid_svc"	No such SDP profile on device
	"com.nokia.btcond.error.no_dev_info"	No info about device
	"com.nokia.btcond.error.connect_failed"	Connection to device failed
	"com.nokia.btcond.error.connected"	Already connected to specified service
	*/

	if (dbus_error_is_set (&dbus_error)) {
		if (strcmp (dbus_error.name, BTCOND_ERROR_INVALID_SVC) == 0) {
			d(g_printerr ("obex: Invalid SDP profile.\n"));
			*result = GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
			*invalid_profile = TRUE;
		}
		else if (strcmp (dbus_error.name, BTCOND_ERROR_INVALID_DEV) == 0) {
			d(g_printerr ("obex: Invalid BDA.\n"));
			*result = GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
		}
		else if (strcmp (dbus_error.name, BTCOND_ERROR_NO_DEV_INFO) == 0) {
			d(g_printerr ("obex: No dev info.\n"));
			*result = GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
		}
		else if (strcmp (dbus_error.name, BTCOND_ERROR_CONNECT_FAILED) == 0) {
			d(g_printerr ("obex: GW connect failed.\n"));
			*result = GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
		}
		else if (strcmp (dbus_error.name, BTCOND_ERROR_CONNECTED) == 0) {
			d(g_printerr ("obex: GW already connected.\n"));
			*result = GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
			*already_connected = TRUE;
		}
		else if (strcmp (dbus_error.name, BTCOND_ERROR_CANCELLED) == 0) {
			d(g_printerr ("obex: GW cancelled.\n"));
			*result = GNOME_VFS_ERROR_INTERRUPTED;
		}
		else if (strcmp (dbus_error.name, DBUS_ERROR_NAME_HAS_NO_OWNER) == 0 ||
			 strcmp (dbus_error.name, DBUS_ERROR_SERVICE_UNKNOWN) == 0) {
			d(g_printerr ("obex: btcond is not running.\n"));
			*result = GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
		}
		else if (strcmp (dbus_error.name, DBUS_ERROR_NO_REPLY) == 0) {
			d(g_printerr ("obex: No reply.\n"));
			/* We get this when btcond times out. Cancel the
			 * connection so that btcond knows that this end will
			 * not be interested in the connection if we time out.
			 */
			send_cancel_connect (conn, bda, profile);
			
			*result = GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
		} else {
			d(g_printerr ("obex: generic '%s'\n", dbus_error.name));
			*result = GNOME_VFS_ERROR_INTERNAL;
		}

		dbus_error_free (&dbus_error);
		return NULL;
	}
	
	if (!reply) {
		*result = GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
		return NULL;
	}

	ret = dbus_message_get_args (reply, NULL,
				     DBUS_TYPE_STRING, &str,
				     DBUS_TYPE_INVALID);

	dbus_message_unref (reply);

	if (!ret) {
		*result = GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
		return NULL;
	}
	
	dev = g_strdup (str);

	*result = GNOME_VFS_OK;
	return dev;
}

gchar *
om_dbus_get_dev (const gchar *bda, GnomeVFSResult *result)
{
	Connection  *conn;
	const gchar *profile;
	gchar       *dev;
	gboolean     invalid_profile;
	gboolean     already_connected;

	conn = get_gwcond_connection ();
	if (!conn) {
		*result = GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
		return NULL;
	}
	
	/* Try NFTP first, which appears to be some vendor specific profile. If
	 * it's not available, fallback to FTP.
	 */

	profile = "NFTP";
	dev = get_dev (conn, bda, profile, result,
		       &invalid_profile, &already_connected);
	if (!dev && invalid_profile) {
		profile = "FTP";
		dev = get_dev (conn, bda, profile, result,
			       &invalid_profile, &already_connected);
	}

	/* We only try to disconnect once, when first starting, in case there
	 * are old stale connections.
	 */
	if (!dev && already_connected) {
		if (send_disconnect_if_first (conn, bda, profile)) {
			dev = get_dev (conn, bda, profile, result,
				       &invalid_profile, &already_connected);
		}
	}
	
	connection_free (conn);
	
	return dev;
}

void
om_dbus_disconnect_dev (const gchar *dev)
{
	Connection *conn;

	conn = get_gwcond_connection ();
	if (!conn) {
		return;
	}
	
	send_disconnect (conn, dev, NULL);

	connection_free (conn);
}


#ifdef OBEX_PROGRESS

#define NOTIFY_SIGNAL_RULE \
	"type='signal',interface='com.nokia.ObexProgress',member='Cancel'"

static DBusHandlerResult
notify_message_filter (DBusConnection *dbus_conn,
		       DBusMessage    *message,
		       gpointer        user_data)
{
	CancelNotifyFunc  func;
	gchar            *str;
	GnomeVFSURI      *uri;

	func = (CancelNotifyFunc) user_data;

	if (dbus_message_is_signal (message,
				    "com.nokia.ObexProgress",
				    "Cancel")) {
		if (!dbus_message_get_args (message, NULL,
					    DBUS_TYPE_STRING, &str,
					    DBUS_TYPE_INVALID)) {
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}
		
		uri = gnome_vfs_uri_new (str);

		if (!uri) {
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}

		/* This is called from the main thread. */
		
		(* func) (uri);
		gnome_vfs_uri_unref (uri);
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean
_om_dbus_init_cancel_monitor (CancelNotifyFunc cancel_func)
{
	DBusConnection *conn;
	DBusError       error;

	/* Get our own connection to make threads work. */
	dbus_error_init (&error);
        conn = dbus_bus_get_private (DBUS_BUS_SESSION, &error);	
	if (!conn) {
		g_printerr ("Failed to connect to the D-BUS daemon: %s", error.message);
		
		dbus_error_free (&error);
		return FALSE;
	}

	dbus_bus_add_match (conn, NOTIFY_SIGNAL_RULE, NULL);
	dbus_connection_add_filter (conn, notify_message_filter, 
				    cancel_func, NULL);

	dbus_connection_setup_with_g_main (conn, NULL);

	return FALSE;
}

void
om_dbus_init_cancel_monitor (CancelNotifyFunc cancel_func)
{
	/* This is called from any thread so we need to set up the connection in
	 * an idle.
	 */
	g_idle_add ((GSourceFunc) _om_dbus_init_cancel_monitor, cancel_func);
}
#endif
