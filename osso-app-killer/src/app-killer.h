/**
  @file app-killer.h
  
  Application killer header file.

  Copyright (C) 2004-2006 Nokia Corporation.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.
	   
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
		   
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA
*/

#ifndef APP_KILLER_H_
#define APP_KILLER_H_

#include <stdlib.h>
#include <osso-log.h>
#include <glib-2.0/glib.h>
#include <libosso.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include "exec-func.h"

#define DBUS_API_SUBJECT_TO_CHANGE 1
#include <dbus/dbus.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APPL_NAME "app-killer"
#define APPL_VERSION "1"

#define APPL_DBUS_NAME "osso_app_killer"
#define SVC_NAME "com.nokia." APPL_DBUS_NAME

#define AF_BASE_APPS "/etc/init.d/af-base-apps"
#define MESSAGEBUS_USER "messagebus"
#define ROOT_USER "root"
#define ROOT_UID 0

/* how long to wait for reply (ms) */
#define MSG_TIMEOUT 5000

/* max. line length in locale file */
#define MAX_LINE 100

/* message asking Task Navigator to save application windows */
#define TN_SVC "com.nokia.tasknav"
#define TN_IF "com.nokia.tasknav"
#define TN_OP "/com/nokia/tasknav"
#define TN_SAVE_METHOD "save"
/* message asking Task Navigator to restore appl. windows */
#define TN_RESTORE_METHOD "restore"

/* some script locations */
#define DBUS_SESSIONBUS_SCRIPT "/etc/osso-af-init/dbus-sessionbus.sh"
#define GCONF_DAEMON_SCRIPT "/etc/osso-af-init/gconf-daemon.sh"
#define OSSO_LOCALE_FILE "/etc/osso-af-init/locale"

/* locale change interface */
#define LOCALE_IF SVC_NAME ".locale"
#define LOCALE_OP "/com/nokia/" APPL_DBUS_NAME "/locale"
#define LOCALE_MSG "change"
#define LOCALE_ERROR LOCALE_IF ".error"
#define AK_LOCALE_SCRIPT "/usr/sbin/osso-app-killer-locale.sh"

/* restore factory settings interface */
#define RFS_SHUTDOWN_IF SVC_NAME ".rfs_shutdown"
#define RFS_SHUTDOWN_OP "/com/nokia/" APPL_DBUS_NAME "/rfs_shutdown"
#define RFS_SHUTDOWN_MSG "shutdown"
#define RFS_SHUTDOWN_ERROR RFS_SHUTDOWN_IF ".error"
#define AK_RFS_SCRIPT "/usr/sbin/osso-app-killer-rfs.sh"

/* cleanup user data interface */
#define CUD_IF SVC_NAME ".cud"
#define CUD_OP "/com/nokia/" APPL_DBUS_NAME "/cud"
#define CUD_MSG "cud"
#define CUD_ERROR CUD_IF ".error"
#define AK_CUD_SCRIPT "/usr/sbin/osso-app-killer-cud.sh"

/* restore interface for the Backup application */
#define RESTORE_IF SVC_NAME ".restore"
#define RESTORE_OP "/com/nokia/" APPL_DBUS_NAME "/restore"
#define RESTORE_MSG "restore"
#define RESTORE_ERROR RESTORE_IF ".error"
#define AK_RESTORE_SCRIPT "/usr/sbin/osso-app-killer-restore.sh"

/* Exit signal definitions */
#define AK_BROADCAST_IF SVC_NAME
#define AK_BROADCAST_OP "/com/nokia/" APPL_DBUS_NAME
#define AK_BROADCAST_EXIT "exit"
#define AK_EXIT_MATCH_RULE \
        "type='signal',interface='" AK_BROADCAST_IF "',member='" \
	AK_BROADCAST_EXIT "'"

#ifdef __cplusplus
}
#endif
#endif /* APP_KILLER_H_ */
