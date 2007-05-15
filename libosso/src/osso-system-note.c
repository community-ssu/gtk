/**
 * @file osso-system-note.c
 * This file implements D-Bus requests for system notification
 * dialogs and infoprints
 * 
 * This file is part of Libosso
 *
 * Copyright (C) 2005-2007 Nokia Corporation. All rights reserved.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <osso-log.h>
#include "osso-internal.h"

#define FDO_SERVICE "org.freedesktop.Notifications"
#define FDO_OBJECT_PATH "/org/freedesktop/Notifications"
#define FDO_INTERFACE "org.freedesktop.Notifications"

osso_return_t osso_system_note_dialog(osso_context_t *osso,
				      const gchar *message,
				      osso_system_note_type_t type,
				      osso_rpc_t *retval)
{
  gint rpc_retval = OSSO_OK;

  if (osso == NULL || message == NULL) {
    ULOG_ERR_F("Invalid osso context or message");
    return OSSO_INVALID;
  }
  if ( (type != OSSO_GN_WARNING) && (type != OSSO_GN_WAIT) &&
       (type != OSSO_GN_ERROR) && (type != OSSO_GN_NOTICE) ) {
    ULOG_ERR_F("Invalid message type");
    return OSSO_INVALID;
  }
  rpc_retval = osso_rpc_run(osso, FDO_SERVICE, FDO_OBJECT_PATH,
                            FDO_INTERFACE, "SystemNoteDialog",
                            retval, DBUS_TYPE_STRING, message,
                            DBUS_TYPE_UINT32, type,
                            DBUS_TYPE_STRING, "",
                            DBUS_TYPE_INVALID);

  if (rpc_retval != OSSO_OK) {
    ULOG_ERR_F("osso_rpc_run failed");
    return OSSO_ERROR;
  }
  return OSSO_OK;
}

osso_return_t osso_system_note_infoprint(osso_context_t *osso,
					 const char *text,
					 osso_rpc_t *retval)
{
  gint rpc_retval = 0;
  
  if (osso == NULL || text == NULL) {
    ULOG_ERR_F("Invalid osso context or message text");
    return OSSO_INVALID;
  }
    
  rpc_retval = osso_rpc_run(osso, FDO_SERVICE, FDO_OBJECT_PATH,
                            FDO_INTERFACE, "SystemNoteInfoprint",
                            retval, DBUS_TYPE_STRING, text,
                            DBUS_TYPE_INVALID);

  if (rpc_retval != OSSO_OK) {
    ULOG_ERR_F("osso_rpc_run failed");
    return OSSO_ERROR;
  }
  return OSSO_OK;
}
