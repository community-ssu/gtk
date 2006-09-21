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
 *
 * Author: Richard Hult <richard@imendio.com>
 */

#ifndef GNOME_VFS_DBUS_UTILS_H
#define GNOME_VFS_DBUS_UTILS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib-lowlevel.h>

G_BEGIN_DECLS

#define DVD_DAEMON_SERVICE                          "org.gnome.GnomeVFS.Daemon"
#define DVD_DAEMON_OBJECT                           "/org/gnome/GnomeVFS/Daemon"
#define DVD_DAEMON_INTERFACE                        "org.gnome.GnomeVFS.Daemon"

/* File monitoring signal. */
#define DVD_DAEMON_MONITOR_SIGNAL                   "MonitorSignal"

#define DVD_DAEMON_METHOD_GET_CONNECTION            "GetConnection"

/* File ops methods. */
#define DVD_DAEMON_METHOD_OPEN                      "Open"
#define DVD_DAEMON_METHOD_CREATE                    "Create"
#define DVD_DAEMON_METHOD_READ                      "Read"
#define DVD_DAEMON_METHOD_WRITE                     "Write"
#define DVD_DAEMON_METHOD_CLOSE                     "Close"

#define DVD_DAEMON_METHOD_SEEK                      "Seek"
#define DVD_DAEMON_METHOD_TELL                      "Tell"

#define DVD_DAEMON_METHOD_TRUNCATE_HANDLE           "TruncateHandle"

#define DVD_DAEMON_METHOD_OPEN_DIRECTORY            "OpenDirectory"
#define DVD_DAEMON_METHOD_CLOSE_DIRECTORY           "CloseDirectory"
#define DVD_DAEMON_METHOD_READ_DIRECTORY            "ReadDirectory"

#define DVD_DAEMON_METHOD_GET_FILE_INFO             "GetFileInfo"
#define DVD_DAEMON_METHOD_GET_FILE_INFO_FROM_HANDLE "GetFileInfoFromHandle"

#define DVD_DAEMON_METHOD_IS_LOCAL                  "IsLocal"
#define DVD_DAEMON_METHOD_MAKE_DIRECTORY            "MakeDirectory"
#define DVD_DAEMON_METHOD_REMOVE_DIRECTORY          "RemoveDirectory"

#define DVD_DAEMON_METHOD_MOVE                      "Move"
#define DVD_DAEMON_METHOD_UNLINK                    "Unlink"
#define DVD_DAEMON_METHOD_CHECK_SAME_FS             "CheckSameFs"
#define DVD_DAEMON_METHOD_SET_FILE_INFO             "SetFileInfo"
#define DVD_DAEMON_METHOD_TRUNCATE                  "Truncate"
#define DVD_DAEMON_METHOD_FIND_DIRECTORY            "FindDirectory"
#define DVD_DAEMON_METHOD_CREATE_SYMBOLIC_LINK      "CreateSymbolicLink"
#define DVD_DAEMON_METHOD_FORGET_CACHE              "ForgetCache"
#define DVD_DAEMON_METHOD_GET_VOLUME_FREE_SPACE     "GetVolumeFreeSpace"

#define DVD_DAEMON_METHOD_MONITOR_ADD               "MonitorAdd"
#define DVD_DAEMON_METHOD_MONITOR_CANCEL            "MonitorCancel"

#define DVD_DAEMON_METHOD_CANCEL                    "Cancel"

/* Volume monitor methods. */
#define DVD_DAEMON_METHOD_REGISTER_VOLUME_MONITOR   "RegisterVolumeMonitor"
#define DVD_DAEMON_METHOD_DEREGISTER_VOLUME_MONITOR "DeregisterVolumeMonitor"
#define DVD_DAEMON_METHOD_GET_VOLUMES               "GetVolumes"
#define DVD_DAEMON_METHOD_GET_DRIVES                "GetDrives"
#define DVD_DAEMON_METHOD_EMIT_PRE_UNMOUNT_VOLUME   "EmitPreUnmountVolume"
#define DVD_DAEMON_METHOD_FORCE_PROBE               "ForceProbe"

/* Volume monitor signals. */
#define DVD_DAEMON_VOLUME_MOUNTED_SIGNAL            "VolumeMountedSignal"
#define DVD_DAEMON_VOLUME_UNMOUNTED_SIGNAL          "VolumeUnmountedSignal"
#define DVD_DAEMON_VOLUME_PRE_UNMOUNT_SIGNAL        "VolumePreUnmountSignal"
#define DVD_DAEMON_DRIVE_CONNECTED_SIGNAL           "DriveConnectedSignal"
#define DVD_DAEMON_DRIVE_DISCONNECTED_SIGNAL        "DriveDisconnectedSignal"

/* Errors. */
#define DVD_ERROR_FAILED            "org.gnome.GnomeVFS.Daemon.Error.Failed"
#define DVD_ERROR_SOCKET_FAILED     "org.gnome.GnomeVFS.Error.SocketFailed"

G_END_DECLS

#endif /* GNOME_VFS_DBUS_UTILS_H */
