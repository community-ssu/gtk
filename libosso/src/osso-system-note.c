/**
 * @file osso-system-note.c
 * This file implements DBUS requests for system notification
 * dialogs and infoprints (on statusbar)
 * 
 * Copyright (C) 2005 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License.
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

#include <osso-log.h>
#include "osso-internal.h"

osso_return_t osso_system_note_dialog(osso_context_t *osso,
				      const gchar *message,
				      osso_system_note_type_t type,
				      osso_rpc_t *retval)
{
  gint rpc_retval = OSSO_OK;

  if ( (osso == NULL) || (message == NULL) ) {
    ULOG_ERR_F("Invalid osso context or message");
    return OSSO_INVALID;
  }
  if ( (type != OSSO_GN_WARNING) && (type != OSSO_GN_WAIT) &&
       (type != OSSO_GN_ERROR) && (type != OSSO_GN_NOTICE) ) {
    ULOG_ERR_F("Invalid message type");
    return OSSO_INVALID;
  }
  rpc_retval = osso_rpc_run_with_defaults(osso, OSSO_BUS_STATUSBAR,
					  OSSO_BUS_SYSNOTE,
					  retval, DBUS_TYPE_STRING,
					  message, DBUS_TYPE_INT32, type,
					  DBUS_TYPE_INVALID);
  if (rpc_retval != OSSO_OK) {
    ULOG_ERR_F("RPC call for system note dialog failed");
    return OSSO_ERROR;
  }
  return OSSO_OK;
}

osso_return_t osso_system_note_infoprint(osso_context_t *osso,
					 const char *text,
					 osso_rpc_t *retval)
{
  gint rpc_retval = 0;
  
  if ( (osso == NULL) || (text == NULL) ) {
    ULOG_ERR_F("Invalid osso context or message text");
    return OSSO_INVALID;
  }
    
  rpc_retval = osso_rpc_run_with_defaults(osso, OSSO_BUS_STATUSBAR,
					  OSSO_BUS_INFOPRINT,
					  retval, DBUS_TYPE_STRING, text,
					  DBUS_TYPE_INVALID);

  if (rpc_retval != OSSO_OK) {
    ULOG_ERR_F("RPC call for statusbar infoprint failed");
    return OSSO_ERROR;
  }
  return OSSO_OK;
}
