/**
 * @file osso-init.h
 * This file includes definitions for osso-init static funtions.
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

#ifndef OSSO_INIT_H
#define OSSO_INIT_H

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>



/**
 * This internal function performs a simple validation for the application
 * and version information of the osso_context regarding their validity
 * as components of the filesystem (no slashes, value not NULL etc)
 * @param application The application name
 * @param verson The application version.
 * @return TRUE if the context passes the validation, FALSE otherwise.
 */
static gboolean _validate(const gchar *application, const gchar* version);

/**
 * This function allocates and initializes the #osso_type_t structure
 * @param application The application name
 * @param verson The application version.
 * @return the newly allocated #osso_context_t structure, or NULL if there
 * is an error.
 */
static osso_context_t * _init(const gchar *application, const gchar *version);

/**
 * This function deinitializes the #osso_type_t  structure, but does not touch
 * and D-BUS element. As a final thing it will destroy the osso_context_t
 * structure.
 * @param osso the disconnecte osso_context_t structure to destroy.
 */
static void _deinit(osso_context_t *osso);

/**
 * This function connect to the given D-BUS bus and registers itself with the
 * D-BUS daemon.
 * @param osso The #osso_context_t for this connection.
 * @param bus_type. The DBusType to use.
 * @param context The GLib main-loop context to connect to, use NULL for the
 * default context.
 * @return The DBusConnection for the connection.
 */
static DBusConnection * _dbus_connect_and_setup(osso_context_t *osso,
						DBusBusType bus_type,
						GMainContext *context);

/**
 * This function deregisters with the D-BUS daemon, and unreferences
 * the given dbus connection.
 * @param osso The #osso_context_t for this connection.
 * @param sys TRUE for system bus, FALSE for session bus.
 */
static void _dbus_disconnect(osso_context_t *osso, gboolean sys);

#ifdef LIBOSSO_DEBUG

/**
 * This function will redirect the GLib/GTK log messages to the
 * OSSO logging macros.
 * @param log_domain NULL in this case, see GLib documentation
 * @param log_level Level of the log event, see GLib documentation
 * @param message The log message string
 * @param user_data The name of the application that sent the message
 */

static GLogFunc _osso_log_handler(const gchar *log_domain,
				 GLogLevelFlags log_level,
				 const gchar *message,
				 gpointer user_data);

/**
 * This debug function will return the type of the message as a string.
 * @param message_type The message type to convert.
 * @param return A string representing the message type.
 * @note This function is copied in verbatim from dbus 0.20 file
 * tools/dbus-print-message.c
 */
static char* type_to_name (int message_type);



/**
 * This debug function will print out the 
 * connection.
 * @param conn The connection to monitor.
 * @param message The message that was received.
 * @param data NULL.
 * @note This function is copied in verbatim from dbus 0.20 file
 * tools/dbus-print-message.c
 */

static DBusHandlerResult _debug_filter(DBusConnection *conn,
				       DBusMessage *message,
				       void *data);
#endif

#endif /* OSSO_INIT_H */
