/*
 * $Id$
 *
 * Copyright (C) 2006 Nokia
 *
 * Author: Guillem Jover <guillem.jover@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#ifndef COMM_DBUS_H
#define COMM_DBUS_H

#define MAEMO_LAUNCHER_PATH			"/org/maemo/launcher"
#define MAEMO_LAUNCHER_IFACE			"org.maemo.launcher"
#define MAEMO_LAUNCHER_SIGNAL_APP_DIED		"ApplicationDied"

void comm_send_app_died(char *filename, int pid, int status);

#endif

