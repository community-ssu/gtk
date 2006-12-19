/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2006 Nokia Corporation. All rights reserved.
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
#include <stdlib.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-dbus-utils.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib-lowlevel.h>

#include "gnome-vfs-volume-monitor-daemon.h"
#include "gnome-vfs-volume-monitor-private.h"
#include "gnome-vfs-private.h"
#include "dbus-utils.h"
#include "daemon-connection.h"

#define d(x)

static DBusConnection *  daemon_get_connection    (gboolean        create);
static void              daemon_shutdown          (void);
static gboolean          daemon_init              (void);
static void              daemon_unregistered_func (DBusConnection *conn,
						   gpointer        data);
static DBusHandlerResult daemon_message_func      (DBusConnection *conn,
						   DBusMessage    *message,
						   gpointer        data);



static DBusObjectPathVTable daemon_vtable = {
	daemon_unregistered_func,
	daemon_message_func,
	NULL
};

typedef struct {
	gint32      conn_id;
	DBusServer *server;
} ShutdownData;	

static GHashTable *connections;


static DBusConnection *
daemon_get_connection (gboolean create)
{
	static DBusConnection *conn = NULL;
	DBusError              error;
	gint                   ret;

	if (conn) {
		return conn;
	}

	if (!create) {
		return NULL;
	}

	dbus_error_init (&error);

	conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
	if (!conn) {
		g_printerr ("Failed to connect to the D-BUS daemon: %s\n",
			    error.message);

		dbus_error_free (&error);
		return NULL;
	}

	ret = dbus_bus_request_name (conn, DVD_DAEMON_SERVICE, 0, &error);
	if (dbus_error_is_set (&error)) {
		g_warning ("Failed to acquire vfs-daemon service: %s", error.message);
		dbus_error_free (&error);

		dbus_connection_disconnect (conn);
		dbus_connection_unref (conn);
		conn = NULL;

		return NULL;
	}
	
	if (ret == DBUS_REQUEST_NAME_REPLY_EXISTS) {
		g_printerr ("VFS daemon already running, exiting.\n");

		dbus_connection_disconnect (conn);
		dbus_connection_unref (conn);
		conn = NULL;

		return NULL;
	}

	if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		g_printerr ("Not primary owner of the service, exiting.\n");

		dbus_connection_disconnect (conn);
		dbus_connection_unref (conn);
		conn = NULL;

		return NULL;
	}

	if (!dbus_connection_register_object_path (conn,
						   DVD_DAEMON_OBJECT,
						   &daemon_vtable,
						   NULL)) {
		g_printerr ("Failed to register object with D-BUS.\n");

		dbus_connection_disconnect (conn);
		dbus_connection_unref (conn);
		conn = NULL;

		return NULL;
	}

        dbus_connection_setup_with_g_main (conn, NULL);

	return conn;
}

static gboolean
daemon_init (void)
{
	return daemon_get_connection (TRUE) != NULL;
}

static void
daemon_shutdown (void)
{
	DBusConnection *conn;

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}

	dbus_connection_disconnect (conn);
	dbus_connection_unref (conn);
}

static void
daemon_unregistered_func (DBusConnection *conn,
			  gpointer        data)
{
}

static gboolean
daemon_connection_shutdown_idle_cb (ShutdownData *data)
{
	g_hash_table_remove (connections, GINT_TO_POINTER (data->conn_id));

	dbus_server_disconnect (data->server);
	dbus_server_unref (data->server);
	
	g_free (data);
	
	return FALSE;
}

static void
daemon_connection_shutdown_func (DaemonConnection *conn,
				 ShutdownData     *data)
{
	/* We add an idle since the shutdown function is called from the daemon
	 * connection thread. */
	g_idle_add ((GSourceFunc) daemon_connection_shutdown_idle_cb, data);
}

static void
daemon_new_connection_func (DBusServer     *server,
			    DBusConnection *conn,
			    gpointer        user_data)
{
	gint32            conn_id;
	DaemonConnection *daemon_connection;
	ShutdownData     *shutdown_data;

	conn_id = GPOINTER_TO_INT (user_data);

	d(g_print ("Got a new connection, id %d\n", conn_id));

	shutdown_data = g_new0 (ShutdownData, 1);
	shutdown_data->conn_id = conn_id;
	shutdown_data->server = server;

	daemon_connection = daemon_connection_setup (
		conn,
		(DaemonConnectionShutdownFunc) daemon_connection_shutdown_func,
		shutdown_data);

	if (!daemon_connection) {
		g_free (shutdown_data);
		g_hash_table_remove (connections, GINT_TO_POINTER (conn_id));
		g_printerr ("Couldn't setup new connection.");
		return;
	}

	g_hash_table_insert (connections, GINT_TO_POINTER (conn_id),
			     daemon_connection);
}

/* Note: Use abstract sockets like dbus does by default on Linux. Abstract
 * sockets are only available on Linux.
 */
static gchar *
generate_address (void)
{
	gint   i;
	gchar  tmp[9];
	gchar *path;

	for (i = 0; i < 8; i++) {
		if (g_random_int_range (0, 2) == 0) {
			tmp[i] = g_random_int_range ('a', 'z' + 1);
		} else {
			tmp[i] = g_random_int_range ('A', 'Z' + 1);
		}
	}
	tmp[8] = '\0';

	path = g_strdup_printf ("unix:abstract=/dbus-vfs-daemon/socket-%s", tmp);

	return path;
}

static void
daemon_handle_get_connection (DBusConnection *conn, DBusMessage *message)
{
	DBusServer    *server;
	DBusError      error;
	DBusMessage   *reply;
	gchar         *address;
	static gint32  conn_id = 0;

	address = generate_address ();

	dbus_error_init (&error);

	server = dbus_server_listen (address, &error);
	if (!server) {
		reply = dbus_message_new_error (message,
						DVD_ERROR_SOCKET_FAILED,
						"Failed to create new socket");
		if (!reply) {
			g_error ("Out of memory");
		}

		dbus_connection_send (conn, reply, NULL);
		dbus_message_unref (reply);

		g_free (address);
		return;
	}

	dbus_server_set_new_connection_function (server,
						 daemon_new_connection_func,
						 GINT_TO_POINTER (++conn_id),
						 NULL);

	dbus_server_setup_with_g_main (server, NULL);

	reply = dbus_message_new_method_return (message);

	dbus_message_append_args (reply,
				  DBUS_TYPE_STRING, &address,
				  DBUS_TYPE_INT32, &conn_id,
				  DBUS_TYPE_INVALID);

	dbus_connection_send (conn, reply, NULL);
	dbus_message_unref (reply);

	g_free (address);
}

static void
daemon_handle_cancel (DBusConnection *conn, DBusMessage *message)
{
	gint32            cancellation_id;
	gint32            conn_id;
	DaemonConnection *daemon_connection;

	if (!dbus_message_get_args (message, NULL,
				    DBUS_TYPE_INT32, &cancellation_id,
				    DBUS_TYPE_INT32, &conn_id,
				    DBUS_TYPE_INVALID)) {
		return;
	}

	d(g_print ("daemon got Cancel (conn id %d, cancel id %d)\n",
		   conn_id, cancellation_id));

	daemon_connection = g_hash_table_lookup (connections,
						 GINT_TO_POINTER (conn_id));

	if (daemon_connection) {
		daemon_connection_cancel (daemon_connection, cancellation_id);
	}
}

static void
daemon_handle_get_volumes (DBusConnection *conn, DBusMessage *message)
{
	GnomeVFSVolumeMonitor *monitor;
	GList                 *volumes;
	DBusMessage           *reply;

	d(g_print ("daemon got GetVolumes\n"));

	monitor = gnome_vfs_get_volume_monitor ();
	volumes = gnome_vfs_volume_monitor_get_mounted_volumes (monitor);

	reply = dbus_message_new_method_return (message);

	dbus_utils_message_append_volume_list (reply, volumes);

	dbus_connection_send (conn, reply, NULL);
	dbus_message_unref (reply);

	g_list_foreach (volumes, (GFunc) gnome_vfs_volume_unref, NULL);
	g_list_free (volumes);
}

static void
daemon_handle_get_drives (DBusConnection *conn, DBusMessage *message)
{
	GnomeVFSVolumeMonitor *monitor;
	GList                 *drives;
	DBusMessage           *reply;

	d(g_print ("daemon got GetDrives\n"));

	monitor = gnome_vfs_get_volume_monitor ();
	drives = gnome_vfs_volume_monitor_get_connected_drives (monitor);

	reply = dbus_message_new_method_return (message);

	dbus_utils_message_append_drive_list (reply, drives);

	dbus_connection_send (conn, reply, NULL);
	dbus_message_unref (reply);

	g_list_foreach (drives, (GFunc) gnome_vfs_drive_unref, NULL);
	g_list_free (drives);
}

static void
daemon_handle_force_probe (DBusConnection *conn, DBusMessage *message)
{
	GnomeVFSVolumeMonitor *monitor;
	DBusMessage           *reply;

	d(g_print ("daemon got ForceProbe\n"));

	monitor = gnome_vfs_get_volume_monitor ();

	gnome_vfs_volume_monitor_daemon_force_probe (monitor);

	reply = dbus_message_new_method_return (message);

	dbus_connection_send (conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
daemon_handle_emit_pre_unmount_volume (DBusConnection *conn, DBusMessage *message)
{
	GnomeVFSVolumeMonitor *monitor;
	dbus_int32_t           id;
	GnomeVFSVolume        *volume;
	DBusMessage           *reply;

	d(g_print ("daemon got EmitPreUnmountVolume\n"));

	monitor = gnome_vfs_get_volume_monitor ();

	if (dbus_message_get_args (message, NULL,
				   DBUS_TYPE_INT32, &id,
				   DBUS_TYPE_INVALID)) {
		volume = gnome_vfs_volume_monitor_get_volume_by_id (monitor, id);
		if (volume != NULL) {
			gnome_vfs_volume_monitor_emit_pre_unmount (monitor,
								   volume);
			gnome_vfs_volume_unref (volume);
		}
	}

	reply = dbus_message_new_method_return (message);

	dbus_connection_send (conn, reply, NULL);
	dbus_message_unref (reply);
}

static DBusHandlerResult
daemon_message_func (DBusConnection *conn,
		     DBusMessage    *message,
		     gpointer        data)
{
	if (dbus_message_is_method_call (message,
					 DVD_DAEMON_INTERFACE,
					 DVD_DAEMON_METHOD_GET_CONNECTION)) {
		daemon_handle_get_connection (conn, message);
	}
	else if (dbus_message_is_method_call (message,
					      DVD_DAEMON_INTERFACE,
					      DVD_DAEMON_METHOD_CANCEL)) {
		daemon_handle_cancel (conn, message);
	}
	else if (dbus_message_is_method_call (message,
					      DVD_DAEMON_INTERFACE,
					      DVD_DAEMON_METHOD_GET_VOLUMES)) {
		daemon_handle_get_volumes (conn, message);
	}
	else if (dbus_message_is_method_call (message,
					      DVD_DAEMON_INTERFACE,
					      DVD_DAEMON_METHOD_GET_DRIVES)) {
		daemon_handle_get_drives (conn, message);
	}
	else if (dbus_message_is_method_call (message,
					      DVD_DAEMON_INTERFACE,
					      DVD_DAEMON_METHOD_FORCE_PROBE)) {
		daemon_handle_force_probe (conn, message);
	}
	else if (dbus_message_is_method_call (message,
					      DVD_DAEMON_INTERFACE,
					      DVD_DAEMON_METHOD_EMIT_PRE_UNMOUNT_VOLUME)) {
		daemon_handle_emit_pre_unmount_volume (conn, message);
	} else {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static void
monitor_volume_mounted_cb (GnomeVFSVolumeMonitor *monitor,
			   GnomeVFSVolume        *volume,
			   gpointer               user_data)
{
	DBusConnection *conn;
	DBusMessage    *signal;

	d(g_print ("daemon got volume_mounted\n"));

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}

	signal = dbus_message_new_signal (DVD_DAEMON_OBJECT,
					  DVD_DAEMON_INTERFACE,
					  DVD_DAEMON_VOLUME_MOUNTED_SIGNAL);

	dbus_utils_message_append_volume (signal, volume);

	dbus_connection_send (conn, signal, NULL);
	dbus_message_unref (signal);
}

static void
monitor_volume_unmounted_cb (GnomeVFSVolumeMonitor *monitor,
			     GnomeVFSVolume        *volume,
			     gpointer               user_data)
{
	DBusConnection *conn;
	DBusMessage    *signal;
	gint32          id;

	d(g_print ("daemon got volume_unmounted\n"));

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}

	signal = dbus_message_new_signal (DVD_DAEMON_OBJECT,
					  DVD_DAEMON_INTERFACE,
					  DVD_DAEMON_VOLUME_UNMOUNTED_SIGNAL);

	id = gnome_vfs_volume_get_id (volume);

	dbus_message_append_args (signal,
				  DBUS_TYPE_INT32, &id,
				  DBUS_TYPE_INVALID);

	dbus_connection_send (conn, signal, NULL);
	dbus_message_unref (signal);
}

static void
monitor_volume_pre_unmount_cb (GnomeVFSVolumeMonitor *monitor,
			       GnomeVFSVolume        *volume,
			       gpointer               user_data)
{
	DBusConnection *conn;
	DBusMessage    *signal;
	gint32          id;

	d(g_print ("daemon got volume_pre_unmount\n"));

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}

	signal = dbus_message_new_signal (DVD_DAEMON_OBJECT,
					  DVD_DAEMON_INTERFACE,
					  DVD_DAEMON_VOLUME_PRE_UNMOUNT_SIGNAL);

	id = gnome_vfs_volume_get_id (volume);

	dbus_message_append_args (signal,
				  DBUS_TYPE_INT32, &id,
				  DBUS_TYPE_INVALID);

	dbus_connection_send (conn, signal, NULL);
	dbus_message_unref (signal);
}

static void
monitor_drive_connected_cb (GnomeVFSVolumeMonitor *monitor,
			    GnomeVFSDrive         *drive,
			    gpointer               user_data)
{
	DBusConnection *conn;
	DBusMessage    *signal;

	d(g_print ("daemon got drive_connected\n"));

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}

	signal = dbus_message_new_signal (DVD_DAEMON_OBJECT,
					  DVD_DAEMON_INTERFACE,
					  DVD_DAEMON_DRIVE_CONNECTED_SIGNAL);

	dbus_utils_message_append_drive (signal, drive);

	dbus_connection_send (conn, signal, NULL);
	dbus_message_unref (signal);
}

static void
monitor_drive_disconnected_cb (GnomeVFSVolumeMonitor *monitor,
			       GnomeVFSDrive         *drive,
			       gpointer               user_data)
{
	DBusConnection *conn;
	DBusMessage    *signal;
	gint32          id;

	d(g_print ("daemon got drive_disconnected\n"));

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}

	signal = dbus_message_new_signal (DVD_DAEMON_OBJECT,
					  DVD_DAEMON_INTERFACE,
					  DVD_DAEMON_DRIVE_DISCONNECTED_SIGNAL);

	id = gnome_vfs_drive_get_id (drive);
	dbus_message_append_args (signal,
				  DBUS_TYPE_INT32, &id,
				  DBUS_TYPE_INVALID);

	dbus_connection_send (conn, signal, NULL);
	dbus_message_unref (signal);
}

int
main (int argc, char *argv[])
{
	GMainLoop             *loop;
	GnomeVFSVolumeMonitor *monitor;

	d(g_print ("Starting daemon.\n"));

	g_type_init ();

	gnome_vfs_set_is_daemon (GNOME_VFS_TYPE_VOLUME_MONITOR_DAEMON,
				 gnome_vfs_volume_monitor_daemon_force_probe);

	if (!gnome_vfs_init ()) {
		g_printerr ("Could not initialize gnome vfs");
		return 1;
	}

	connections = g_hash_table_new (g_direct_hash, g_direct_equal);

	loop = g_main_loop_new (NULL, FALSE);

	if (!daemon_init ()) {
		d(g_print ("Couldn't init daemon, exiting.\n"));
		return 1;
	}

	/* Init the volume monitor. */
	monitor = gnome_vfs_get_volume_monitor ();

	g_signal_connect (monitor,
			  "volume_mounted",
			  G_CALLBACK (monitor_volume_mounted_cb),
			  NULL);
	g_signal_connect (monitor,
			  "volume_unmounted",
			  G_CALLBACK (monitor_volume_unmounted_cb),
			  NULL);
	g_signal_connect (monitor,
			  "volume_pre_unmount",
			  G_CALLBACK (monitor_volume_pre_unmount_cb),
			  NULL);

	g_signal_connect (monitor,
			  "drive_connected",
			  G_CALLBACK (monitor_drive_connected_cb),
			  NULL);
	g_signal_connect (monitor,
			  "drive_disconnected",
			  G_CALLBACK (monitor_drive_disconnected_cb),
			  NULL);

	g_main_loop_run (loop);

	d(g_print ("Shutting down.\n"));

	daemon_shutdown ();

	g_hash_table_destroy (connections);

	return 0;
}



