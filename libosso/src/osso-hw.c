/**
 * @file osso-hw.c
 * This file implements the device signal handler functions.
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

#include "osso-internal.h"
#include "osso-hw.h"

#define MCE_SERVICE			"com.nokia.mce"
#define MCE_REQUEST_PATH		"/com/nokia/mce/request"
#define MCE_REQUEST_IF			"com.nokia.mce.request"
#define MCE_SIGNAL_PATH			"/com/nokia/mce/signal"
#define MCE_SIGNAL_IF			"com.nokia.mce.signal"
#define MCE_DISPLAY_ON_REQ		"req_display_state_on"
#define MCE_PREVENT_BLANK_REQ		"req_display_blanking_pause"

#define DEVICE_MODE_SIG	                "sig_device_mode_ind"
#define INACTIVITY_SIG                  "system_inactivity_ind"
#define SHUTDOWN_SIG                    "shutdown_ind"
#define MEMORY_LOW_SIG                  "memory_low_ind"
#define SAVE_UNSAVED_SIG                "save_unsaved_data_ind"

#define NORMAL_MODE                     "normal"
#define FLIGHT_MODE                     "flight"
#define OFFLINE_MODE                    "offline"
#define INVALID_MODE                    "invalid"

osso_return_t osso_display_state_on(osso_context_t *osso)
{
  DBusMessage *msg = NULL;
  dbus_bool_t b;
  
  if (osso->sys_conn == NULL) {
    ULOG_ERR_F("error: no system bus connection");
    return OSSO_ERROR;
  }
  msg = dbus_message_new_method_call(MCE_SERVICE, MCE_REQUEST_PATH,
				     MCE_REQUEST_IF, MCE_DISPLAY_ON_REQ);
  if (msg == NULL) {
    ULOG_ERR_F("could not create message");
    return OSSO_ERROR;
  }

  b = dbus_connection_send(osso->sys_conn, msg, NULL);
  if (!b) {
    ULOG_ERR_F("dbus_connection_send() failed");
    dbus_message_unref(msg);
    return OSSO_ERROR;
  }
  dbus_connection_flush(osso->sys_conn);
  dbus_message_unref(msg);
  return OSSO_OK;
}

osso_return_t osso_display_blanking_pause(osso_context_t *osso)
{
  DBusMessage *msg = NULL;
  dbus_bool_t b;
  
  if (osso->sys_conn == NULL) {
    ULOG_ERR_F("error: no sys D-BUS connection!");
    return OSSO_ERROR;
  }
  msg = dbus_message_new_method_call(MCE_SERVICE, MCE_REQUEST_PATH,
				     MCE_REQUEST_IF, MCE_PREVENT_BLANK_REQ);
  if (msg == NULL) {
    ULOG_ERR_F("could not create message");
    return OSSO_ERROR;
  }

  b = dbus_connection_send(osso->sys_conn, msg, NULL);
  if (!b) {
    ULOG_ERR_F("dbus_connection_send() failed");
    dbus_message_unref(msg);
    return OSSO_ERROR;
  }
  dbus_connection_flush(osso->sys_conn);
  dbus_message_unref(msg);
  return OSSO_OK;
}

osso_return_t osso_hw_set_event_cb(osso_context_t *osso,
				   osso_hw_state_t *state,
				   osso_hw_cb_f *cb, gpointer data)
{
    static const osso_hw_state_t default_mask = {TRUE,TRUE,TRUE,TRUE,1};
    gboolean call_cb = FALSE;

    if (osso == NULL || cb == NULL) {
	ULOG_ERR_F("invalid parameters");
	return OSSO_INVALID;
    }
    if (osso->sys_conn == NULL) {
	ULOG_ERR_F("error: no system bus connection");
	return OSSO_INVALID;
    }

    if (state == NULL) {
	state = (osso_hw_state_t*) &default_mask;
    }

    read_device_state_from_file(osso);

    if (state->shutdown_ind) {
        osso->hw_cbs.shutdown_ind.cb = cb;
        osso->hw_cbs.shutdown_ind.data = data;
        if (!osso->hw_cbs.shutdown_ind.set) {
            /* if callback was not previously registered, add match */
            dbus_bus_add_match(osso->sys_conn, "type='signal',interface='"
                MCE_SIGNAL_IF "',member='" SHUTDOWN_SIG "'", NULL);
        }
	if (osso->hw_state.shutdown_ind) {
            call_cb = TRUE;
        } 
        osso->hw_cbs.shutdown_ind.set = TRUE;
    }
    if (state->memory_low_ind) {
        osso->hw_cbs.memory_low_ind.cb = cb;
        osso->hw_cbs.memory_low_ind.data = data;
        if (!osso->hw_cbs.memory_low_ind.set) {
		/* FIXME: not correct signal */
            dbus_bus_add_match(osso->sys_conn, "type='signal',interface='"
                MCE_SIGNAL_IF "',member='" MEMORY_LOW_SIG "'", NULL);
        }
	if (osso->hw_state.memory_low_ind) {
            call_cb = TRUE;
        } 
        osso->hw_cbs.memory_low_ind.set = TRUE;
    }
    if (state->save_unsaved_data_ind) {
        osso->hw_cbs.save_unsaved_data_ind.cb = cb;
        osso->hw_cbs.save_unsaved_data_ind.data = data;
        if (!osso->hw_cbs.save_unsaved_data_ind.set) {
            dbus_bus_add_match(osso->sys_conn, "type='signal',interface='"
                MCE_SIGNAL_IF "',member='" SAVE_UNSAVED_SIG "'", NULL);
        }
	if (osso->hw_state.save_unsaved_data_ind) {
            call_cb = TRUE;
        } 
        osso->hw_cbs.save_unsaved_data_ind.set = TRUE;
    }
    if (state->system_inactivity_ind) {
        osso->hw_cbs.system_inactivity_ind.cb = cb;
        osso->hw_cbs.system_inactivity_ind.data = data;
        if (!osso->hw_cbs.system_inactivity_ind.set) {
            dbus_bus_add_match(osso->sys_conn, "type='signal',interface='"
                MCE_SIGNAL_IF "',member='" INACTIVITY_SIG "'", NULL);
        }
	if (osso->hw_state.system_inactivity_ind) {
            call_cb = TRUE;
        } 
        osso->hw_cbs.system_inactivity_ind.set = TRUE;
    }
    if (state->sig_device_mode_ind) {
        osso->hw_cbs.sig_device_mode_ind.cb = cb;
        osso->hw_cbs.sig_device_mode_ind.data = data;
        if (!osso->hw_cbs.sig_device_mode_ind.set) {
            dbus_bus_add_match(osso->sys_conn, "type='signal',interface='"
                MCE_SIGNAL_IF "',member='" DEVICE_MODE_SIG "'", NULL);
        }
	if (osso->hw_state.sig_device_mode_ind != OSSO_DEVMODE_NORMAL) {
            call_cb = TRUE;
        } 
        osso->hw_cbs.sig_device_mode_ind.set = TRUE;
    }

    _msg_handler_set_cb_f(osso, MCE_SIGNAL_IF, signal_handler, state, FALSE);
    if (call_cb) {
        (*cb)(&osso->hw_state, data);
    }
    dbus_connection_flush(osso->sys_conn);

    return OSSO_OK;    
}

osso_return_t osso_hw_unset_event_cb(osso_context_t *osso,
				     osso_hw_state_t *state)
{
    osso_hw_state_t temp = {TRUE,TRUE,TRUE,TRUE,1};
    if (osso == NULL) {
	ULOG_ERR_F("invalid parameters");
	return OSSO_INVALID;
    }
    if (osso->sys_conn == NULL) {
	ULOG_ERR_F("error: no system bus connection");
	return OSSO_INVALID;
    }

    dprint("state = %p", state);
    if(state == NULL)
	state = &temp;
    dprint("state = %p", state);

    _unset_state_cb(shutdown_ind);
    _unset_state_cb(memory_low_ind);
    _unset_state_cb(save_unsaved_data_ind);
    _unset_state_cb(system_inactivity_ind);
    _unset_state_cb(sig_device_mode_ind);

    if (_state_is_unset()) {
	_msg_handler_rm_cb_f(osso, MCE_SIGNAL_IF, signal_handler, FALSE);
    }
    return OSSO_OK;    
}

static void read_device_state_from_file(osso_context_t *osso)
{
    FILE *f = NULL;
    gchar s[STORED_LEN];
    g_assert(osso != NULL);

    f = fopen(OSSO_DEVSTATE_MODE_FILE, "r");
    if (f != NULL) {
        if (fgets(s, STORED_LEN, f) != NULL) {
            if (strncmp(s, NORMAL_MODE, strlen(NORMAL_MODE)) == 0) {
                osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_NORMAL;
            } else if (strncmp(s, FLIGHT_MODE, strlen(FLIGHT_MODE)) == 0) {
                osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_FLIGHT;
            } else if (strncmp(s, OFFLINE_MODE, strlen(OFFLINE_MODE)) == 0) {
                osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_OFFLINE;
            } else if (strncmp(s, INVALID_MODE, strlen(INVALID_MODE)) == 0) {
                osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_INVALID;
            } else {
                ULOG_WARN_F("invalid device mode '%s'", s);
            }
        }
        fclose(f);
    } else {
        ULOG_ERR_F("could not open device state file '%s', "
            "assuming OSSO_DEVMODE_NORMAL", OSSO_DEVSTATE_MODE_FILE);
        osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_NORMAL;
    }
}

static DBusHandlerResult signal_handler(osso_context_t *osso,
                                        DBusMessage *msg, gpointer data)
{
    ULOG_DEBUG_F("entered");

    if (dbus_message_is_signal(msg, MCE_SIGNAL_IF, SHUTDOWN_SIG)) {
        osso->hw_state.shutdown_ind = TRUE;
        if (osso->hw_cbs.shutdown_ind.set) {
            (osso->hw_cbs.shutdown_ind.cb)(&osso->hw_state,
                osso->hw_cbs.shutdown_ind.data);
        } 
    } else if (dbus_message_is_signal(msg, MCE_SIGNAL_IF, MEMORY_LOW_SIG)) {
        osso->hw_state.memory_low_ind = TRUE;
        /* FIXME: wrong signal */
        if (osso->hw_cbs.memory_low_ind.set) {
            (osso->hw_cbs.memory_low_ind.cb)(&osso->hw_state,
                osso->hw_cbs.memory_low_ind.data);
        }
    } else if (dbus_message_is_signal(msg, MCE_SIGNAL_IF, SAVE_UNSAVED_SIG)) {
        osso->hw_state.save_unsaved_data_ind = TRUE;
        if (osso->hw_cbs.save_unsaved_data_ind.set) {
            (osso->hw_cbs.save_unsaved_data_ind.cb)(&osso->hw_state,
                osso->hw_cbs.save_unsaved_data_ind.data);
        }
    } else if (dbus_message_is_signal(msg, MCE_SIGNAL_IF, INACTIVITY_SIG)) {
        int type;
        DBusMessageIter i;
        gboolean new_state;

        dbus_message_iter_init(msg, &i);
        type = dbus_message_iter_get_arg_type(&i);
        if (type != DBUS_TYPE_BOOLEAN) {
            ULOG_ERR_F("invalid argument in '" INACTIVITY_SIG "' signal");
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        new_state = dbus_message_iter_get_boolean(&i);
        if (osso->hw_state.system_inactivity_ind != new_state) {
            osso->hw_state.system_inactivity_ind = new_state;
            if (osso->hw_cbs.system_inactivity_ind.set) {
                (osso->hw_cbs.system_inactivity_ind.cb)(&osso->hw_state,
                    osso->hw_cbs.system_inactivity_ind.data);
            }
        }
    } else if (dbus_message_is_signal(msg, MCE_SIGNAL_IF, DEVICE_MODE_SIG)) {
        int type;
        DBusMessageIter i;
	char* s = NULL;

        dbus_message_iter_init(msg, &i);
        type = dbus_message_iter_get_arg_type(&i);
        if (type != DBUS_TYPE_STRING) {
            ULOG_ERR_F("invalid argument in '" DEVICE_MODE_SIG "' signal");
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        s = dbus_message_iter_get_string(&i);
        if (strncmp(s, NORMAL_MODE, strlen(NORMAL_MODE)) == 0) {
            osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_NORMAL;
        } else if (strncmp(s, FLIGHT_MODE, strlen(FLIGHT_MODE)) == 0) {
            osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_FLIGHT;
        } else if (strncmp(s, OFFLINE_MODE, strlen(OFFLINE_MODE)) == 0) {
            osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_OFFLINE;
        } else if (strncmp(s, INVALID_MODE, strlen(INVALID_MODE)) == 0) {
            osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_INVALID;
        } else {
            ULOG_WARN_F("invalid device mode '%s'", s);
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        if (osso->hw_cbs.sig_device_mode_ind.set) {
            (osso->hw_cbs.sig_device_mode_ind.cb)(&osso->hw_state,
                osso->hw_cbs.sig_device_mode_ind.data);
        }
    } else {
        ULOG_WARN_F("received unknown signal");
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
