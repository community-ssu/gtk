/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2005 Nokia Corporation. All rights reserved.
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

#ifndef __DAEMON_CONNECTION_H__
#define __DAEMON_CONNECTION_H__

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>

typedef struct _DaemonConnection DaemonConnection;

typedef void (*DaemonConnectionShutdownFunc) (DaemonConnection *conn,
					      gpointer          data);

DaemonConnection *daemon_connection_setup  (DBusConnection               *dbus_conn,
					    DaemonConnectionShutdownFunc  shutdown_func,
					    gpointer                      shutdown_data);
void              daemon_connection_cancel (DaemonConnection             *conn,
					    gint32                        cancellation_id);

#endif /* __DAEMON_CONNECTION_H__ */
