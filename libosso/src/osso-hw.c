/**
 * @file osso-hw.c
 * This file implements the device signal handler functions.
 * 
 * This file is part of libosso
 *
 * Copyright (C) 2005-2006 Nokia Corporation. All rights reserved.
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

#include "osso-internal.h"
#include "osso-hw.h"
#include "osso-mem.h"
#include "muali.h"
#include <assert.h>

#define MCE_SERVICE			"com.nokia.mce"
#define MCE_REQUEST_PATH		"/com/nokia/mce/request"
#define MCE_REQUEST_IF			"com.nokia.mce.request"
#define MCE_SIGNAL_PATH			"/com/nokia/mce/signal"
#define MCE_SIGNAL_IF			"com.nokia.mce.signal"
#define MCE_SIGNAL_SVC		        MCE_SERVICE	
#define MCE_DISPLAY_ON_REQ		"req_display_state_on"
#define MCE_PREVENT_BLANK_REQ		"req_display_blanking_pause"

#define DEVICE_MODE_SIG	                "sig_device_mode_ind"
#define INACTIVITY_SIG                  "system_inactivity_ind"
#define SHUTDOWN_SIG                    "shutdown_ind"
#define SAVE_UNSAVED_SIG                "save_unsaved_data_ind"

#define NORMAL_MODE                     "normal"
#define FLIGHT_MODE                     "flight"
#define OFFLINE_MODE                    "offline"
#define INVALID_MODE                    "invalid"

/* user lowmem signal */
#define USER_LOWMEM_OFF_SIGNAL_SVC "com.nokia.ke_recv"
#define USER_LOWMEM_OFF_SIGNAL_OP "/com/nokia/ke_recv/user_lowmem_off"
#define USER_LOWMEM_OFF_SIGNAL_IF "com.nokia.ke_recv.user_lowmem_off"
#define USER_LOWMEM_OFF_SIGNAL_NAME "user_lowmem_off"
#define USER_LOWMEM_ON_SIGNAL_SVC "com.nokia.ke_recv"
#define USER_LOWMEM_ON_SIGNAL_OP "/com/nokia/ke_recv/user_lowmem_on"
#define USER_LOWMEM_ON_SIGNAL_IF "com.nokia.ke_recv.user_lowmem_on"
#define USER_LOWMEM_ON_SIGNAL_NAME "user_lowmem_on"

#define MAX_CACHE_FILE_NAME 100
static char cache_file_name[MAX_CACHE_FILE_NAME];
static gboolean first_hw_set_cb_call = TRUE;

static DBusHandlerResult lowmem_signal_handler(osso_context_t *osso,
                                               DBusMessage *msg,
                                               _osso_callback_data_t *data);

static DBusHandlerResult signal_handler(osso_context_t *osso,
                                        DBusMessage *msg,
                                        _osso_callback_data_t *data);

osso_return_t osso_display_state_on(osso_context_t *osso)
{
  DBusMessage *msg = NULL;
  dbus_bool_t b;

  if (osso == NULL) {
    return OSSO_INVALID;
  }
  
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

  if (osso == NULL) {
    return OSSO_INVALID;
  }
  
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
                                   osso_hw_cb_f *cb,
                                   gpointer data)
{
    static const osso_hw_state_t default_mask = {TRUE,TRUE,TRUE,TRUE,1};
    gboolean call_cb = FALSE;
    gboolean install_mce_handler = FALSE;
    DBusError error;

    ULOG_DEBUG_F("entered");
    if (osso == NULL || cb == NULL) {
	ULOG_ERR_F("invalid parameters");
	return OSSO_INVALID;
    }
    if (osso->sys_conn == NULL) {
	ULOG_ERR_F("error: no system bus connection");
	return OSSO_INVALID;
    }

    if (first_hw_set_cb_call) {
        /* uid is used for naming the device mode cache file, avoiding
         * permission issues */
        int ret = snprintf(cache_file_name, MAX_CACHE_FILE_NAME, "%s-%d",
                           OSSO_DEVSTATE_MODE_FILE, geteuid());
        if (ret < 0) {
            ULOG_ERR_F("snprintf failed");
            return OSSO_ERROR;
        }
        read_device_state_from_file(osso);
    }

    if (state == NULL) {
	state = (osso_hw_state_t*) &default_mask;
    }

    dbus_error_init(&error);

    if (state->shutdown_ind) {
        osso->hw_cbs.shutdown_ind.cb = cb;
        osso->hw_cbs.shutdown_ind.data = data;
        if (!osso->hw_cbs.shutdown_ind.set) {
            /* if callback was not previously registered, add match */
            dbus_bus_add_match(osso->sys_conn, "type='signal',interface='"
                MCE_SIGNAL_IF "',member='" SHUTDOWN_SIG "'", &error);
            if (dbus_error_is_set(&error)) {
                ULOG_ERR_F("dbus_bus_add_match failed: %s", error.message);
                dbus_error_free(&error);
                return OSSO_ERROR;
            }
            install_mce_handler = TRUE;
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
            dbus_bus_add_match(osso->sys_conn, "type='signal',interface='"
                USER_LOWMEM_OFF_SIGNAL_IF "'", &error);
            if (dbus_error_is_set(&error)) {
                ULOG_ERR_F("dbus_bus_add_match failed: %s", error.message);
                dbus_error_free(&error);
                return OSSO_ERROR;
            }
            dbus_bus_add_match(osso->sys_conn, "type='signal',interface='"
                USER_LOWMEM_ON_SIGNAL_IF "'", &error);
            if (dbus_error_is_set(&error)) {
                ULOG_ERR_F("dbus_bus_add_match failed: %s", error.message);
                dbus_error_free(&error);
                return OSSO_ERROR;
            }
            _msg_handler_set_cb_f(osso, USER_LOWMEM_OFF_SIGNAL_SVC,
                                  USER_LOWMEM_OFF_SIGNAL_OP,
                                  USER_LOWMEM_OFF_SIGNAL_IF,
                                  lowmem_signal_handler, NULL, FALSE);
            _msg_handler_set_cb_f(osso, USER_LOWMEM_ON_SIGNAL_SVC,
                                  USER_LOWMEM_ON_SIGNAL_OP,
                                  USER_LOWMEM_ON_SIGNAL_IF,
                                  lowmem_signal_handler, NULL, FALSE);
        }
        osso->hw_cbs.memory_low_ind.set = TRUE;

        osso->hw_state.memory_low_ind = osso_mem_in_lowmem_state();
        call_cb = TRUE;
    }
    if (state->save_unsaved_data_ind) {
        osso->hw_cbs.save_unsaved_data_ind.cb = cb;
        osso->hw_cbs.save_unsaved_data_ind.data = data;
        if (!osso->hw_cbs.save_unsaved_data_ind.set) {
            dbus_bus_add_match(osso->sys_conn, "type='signal',interface='"
                MCE_SIGNAL_IF "',member='" SAVE_UNSAVED_SIG "'", &error);
            if (dbus_error_is_set(&error)) {
                ULOG_ERR_F("dbus_bus_add_match failed: %s", error.message);
                dbus_error_free(&error);
                return OSSO_ERROR;
            }
            install_mce_handler = TRUE;
        }
	/* This signal is stateless: It makes no sense to call the
	 * application callback */
        osso->hw_cbs.save_unsaved_data_ind.set = TRUE;
    }
    if (state->system_inactivity_ind) {
        osso->hw_cbs.system_inactivity_ind.cb = cb;
        osso->hw_cbs.system_inactivity_ind.data = data;
        if (!osso->hw_cbs.system_inactivity_ind.set) {
            dbus_bus_add_match(osso->sys_conn, "type='signal',interface='"
                MCE_SIGNAL_IF "',member='" INACTIVITY_SIG "'", &error);
            if (dbus_error_is_set(&error)) {
                ULOG_ERR_F("dbus_bus_add_match failed: %s", error.message);
                dbus_error_free(&error);
                return OSSO_ERROR;
            }
            install_mce_handler = TRUE;
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
                MCE_SIGNAL_IF "',member='" DEVICE_MODE_SIG "'", &error);
            if (dbus_error_is_set(&error)) {
                ULOG_ERR_F("dbus_bus_add_match failed: %s", error.message);
                dbus_error_free(&error);
                return OSSO_ERROR;
            }
            install_mce_handler = TRUE;
        }
	if (osso->hw_state.sig_device_mode_ind != OSSO_DEVMODE_NORMAL) {
            call_cb = TRUE;
        } 
        osso->hw_cbs.sig_device_mode_ind.set = TRUE;
    }

    if (install_mce_handler) {
        _msg_handler_set_cb_f(osso,
                              MCE_SIGNAL_SVC,
                              MCE_SIGNAL_PATH,
                              MCE_SIGNAL_IF,
                              signal_handler,
                              NULL, FALSE);
    }
    first_hw_set_cb_call = FALSE;  /* set before calling callbacks */
    if (call_cb) {
        ULOG_DEBUG_F("calling application callback");
        (*cb)(&osso->hw_state, data);
    }

    return OSSO_OK;    
}

/* TODO: needs additional function that allows several handlers
 * for the same signal */
osso_return_t osso_hw_unset_event_cb(osso_context_t *osso,
				     osso_hw_state_t *state)
{
    static const osso_hw_state_t default_mask = {TRUE,TRUE,TRUE,TRUE,1};
    ULOG_DEBUG_F("entered");
    if (osso == NULL) {
	ULOG_ERR_F("invalid parameters");
	return OSSO_INVALID;
    }
    if (osso->sys_conn == NULL) {
	ULOG_ERR_F("error: no system bus connection");
	return OSSO_INVALID;
    }
    if (first_hw_set_cb_call) {
        ULOG_WARN_F("called without calling osso_hw_set_event_cb first");
        return OSSO_OK;
    }

    if (state == NULL) {
	state = (osso_hw_state_t*) &default_mask;
    }

    _unset_state_cb(shutdown_ind);
    if (state->memory_low_ind && osso->hw_cbs.memory_low_ind.set) {
        osso->hw_cbs.memory_low_ind.cb = NULL;
        osso->hw_cbs.memory_low_ind.data = NULL;
        osso->hw_cbs.memory_low_ind.set = FALSE;
        dbus_bus_remove_match(osso->sys_conn, "type='signal',interface='"
                USER_LOWMEM_OFF_SIGNAL_IF "'", NULL);
        dbus_bus_remove_match(osso->sys_conn, "type='signal',interface='"
                USER_LOWMEM_ON_SIGNAL_IF "'", NULL);
        _msg_handler_rm_cb_f(osso, USER_LOWMEM_OFF_SIGNAL_SVC,
                             USER_LOWMEM_OFF_SIGNAL_OP,
                             USER_LOWMEM_OFF_SIGNAL_IF,
                             lowmem_signal_handler, NULL, FALSE);
        _msg_handler_rm_cb_f(osso, USER_LOWMEM_ON_SIGNAL_SVC,
                             USER_LOWMEM_ON_SIGNAL_OP,
                             USER_LOWMEM_ON_SIGNAL_IF,
                             lowmem_signal_handler, NULL, FALSE);
    }
    _unset_state_cb(save_unsaved_data_ind);
    _unset_state_cb(system_inactivity_ind);
    _unset_state_cb(sig_device_mode_ind);

    if (_state_is_unset()) {
	_msg_handler_rm_cb_f(osso, MCE_SIGNAL_SVC,
                             MCE_SIGNAL_PATH, MCE_SIGNAL_IF,
                             signal_handler, NULL, FALSE);
    }
    return OSSO_OK;    
}


static void write_device_state_to_file(const char* s)
{
    char buf[STORED_LEN];
    int ret;
    assert(cache_file_name[0] != 0);
    ret = readlink(cache_file_name, buf, STORED_LEN - 1);
    if (errno == ENOENT) {
        /* does not exist, create it */
        ret = symlink(s, cache_file_name);
        if (ret == -1) {
            ULOG_ERR_F("failed to create symlink '%s': %s",
                       cache_file_name, strerror(errno));
        }
    } else if (ret >= 0) {
        buf[ret] = '\0';
        if (strncmp(s, buf, strlen(buf)) == 0) {
            /* the desired value is already there */
            return;
        }
        /* change the value: unlink + symlink */
        ret = unlink(cache_file_name);
        if (ret == -1) {
            ULOG_ERR_F("failed to unlink symlink '%s': %s",
                       cache_file_name, strerror(errno));
            return;
        }
        ret = symlink(s, cache_file_name);
        if (ret == -1) {
            ULOG_ERR_F("failed to create symlink '%s': %s",
                       cache_file_name, strerror(errno));
        }
    }
}

static void read_device_state_from_file(osso_context_t *osso)
{
    int ret;
    char s[STORED_LEN];

    ULOG_DEBUG_F("entered");
    assert(osso != NULL);
    assert(cache_file_name[0] != 0);

    ret = readlink(cache_file_name, s, STORED_LEN - 1);
    if (ret >= 0) {
        s[ret] = '\0';
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
    } else {
        ULOG_ERR_F("readlink of '%s' failed: '%s', "
            "assuming OSSO_DEVMODE_NORMAL", cache_file_name,
            strerror(errno));
        osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_NORMAL;
        ret = symlink(NORMAL_MODE, cache_file_name);
        if (ret == -1) {
            ULOG_ERR_F("failed to create symlink '%s': %s",
                       cache_file_name, strerror(errno));
        }
    }
}

static DBusHandlerResult lowmem_signal_handler(osso_context_t *osso,
                                               DBusMessage *msg,
                                               _osso_callback_data_t *data)
{
    ULOG_DEBUG_F("entered");
    assert(osso != NULL);

    if (dbus_message_is_signal(msg, USER_LOWMEM_ON_SIGNAL_IF,
                               USER_LOWMEM_ON_SIGNAL_NAME)) {
        osso->hw_state.memory_low_ind = TRUE;
        if (osso->hw_cbs.memory_low_ind.set) {
            (osso->hw_cbs.memory_low_ind.cb)(&osso->hw_state,
                osso->hw_cbs.memory_low_ind.data);
        } 
    } else if (dbus_message_is_signal(msg, USER_LOWMEM_OFF_SIGNAL_IF,
                               USER_LOWMEM_OFF_SIGNAL_NAME)) {
        osso->hw_state.memory_low_ind = FALSE;
        if (osso->hw_cbs.memory_low_ind.set) {
            (osso->hw_cbs.memory_low_ind.cb)(&osso->hw_state,
                osso->hw_cbs.memory_low_ind.data);
        } 
    } else {
        ULOG_WARN_F("unknown signal received");
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult signal_handler(osso_context_t *osso,
                                        DBusMessage *msg,
                                        _osso_callback_data_t *data)
{
    ULOG_DEBUG_F("entered");
    assert(osso != NULL);

    if (dbus_message_is_signal(msg, MCE_SIGNAL_IF, SHUTDOWN_SIG)) {
        osso->hw_state.shutdown_ind = TRUE;
        if (osso->hw_cbs.shutdown_ind.set) {
            (osso->hw_cbs.shutdown_ind.cb)(&osso->hw_state,
                osso->hw_cbs.shutdown_ind.data);
        } 
    } else if (dbus_message_is_signal(msg, MCE_SIGNAL_IF, SAVE_UNSAVED_SIG)) {
        if (osso->hw_cbs.save_unsaved_data_ind.set) {
            /* stateless signal, the value only tells the signal came */
            osso->hw_state.save_unsaved_data_ind = TRUE;
            (osso->hw_cbs.save_unsaved_data_ind.cb)(&osso->hw_state,
                osso->hw_cbs.save_unsaved_data_ind.data);
            osso->hw_state.save_unsaved_data_ind = FALSE;
            if (osso->autosave.func != NULL) {
	        /* call force autosave function */
	        osso_application_autosave_force(osso);
            }
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

        dbus_message_iter_get_basic (&i, &new_state);
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

        dbus_message_iter_get_basic (&i, &s);
        if (strncmp(s, NORMAL_MODE, strlen(NORMAL_MODE)) == 0) {
            osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_NORMAL;
            write_device_state_to_file(NORMAL_MODE);
        } else if (strncmp(s, FLIGHT_MODE, strlen(FLIGHT_MODE)) == 0) {
            osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_FLIGHT;
            write_device_state_to_file(FLIGHT_MODE);
        } else if (strncmp(s, OFFLINE_MODE, strlen(OFFLINE_MODE)) == 0) {
            osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_OFFLINE;
            write_device_state_to_file(OFFLINE_MODE);
        } else if (strncmp(s, INVALID_MODE, strlen(INVALID_MODE)) == 0) {
            osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_INVALID;
            write_device_state_to_file(INVALID_MODE);
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

/******************************************************
 * NEW API DEVELOPMENT - THESE ARE SUBJECT TO CHANGE!
 * muali = maemo user application library
 ******************************************************/

inline static muali_error_t _set_handler(muali_context_t *context,
                                         const char *service,
                                         const char *object_path,
                                         const char *interface,
                                         const char *match,
                                         _osso_handler_f *event_cb,
                                         int event_type,
                                         muali_handler_t *user_handler,
                                         const void *user_data)
{
        DBusError error;
        _osso_callback_data_t *cb_data;

        cb_data = calloc(1, sizeof(_osso_callback_data_t));
        if (cb_data == NULL) {
                ULOG_ERR_F("calloc failed");
                return MUALI_ERROR_OOM;
        }

        cb_data->user_cb = user_handler;
        cb_data->user_data = user_data;
        cb_data->match_rule = match;
        cb_data->event_type = event_type;
        cb_data->data = cb_data;

        dbus_error_init(&error);
        dbus_bus_add_match(context->sys_conn, match, &error);

        if (dbus_error_is_set(&error)) {
                ULOG_ERR_F("dbus_bus_add_match failed: %s", error.message);
                dbus_error_free(&error);
                free(cb_data);
                return MUALI_ERROR;
        }

        _msg_handler_set_cb_f(context,
                              service,
                              object_path,
                              interface,
                              event_cb,
                              cb_data, FALSE);

        return MUALI_ERROR_SUCCESS;
}

muali_error_t muali_set_event_handler(muali_context_t *context,
                                      int event_type,
                                      muali_handler_t *handler,
                                      const void *user_data)
{
        muali_error_t error = MUALI_ERROR_SUCCESS;

        ULOG_DEBUG_F("entered");

        switch (event_type) {
                _osso_handler_f *event_cb = NULL;
                const char *service = NULL, *object_path = NULL,
                           *interface = NULL;

                case MUALI_EVENT_LOWMEM_BOTH:
                case MUALI_EVENT_LOWMEM_OFF:
                        event_cb = lowmem_signal_handler;
                        service = USER_LOWMEM_OFF_SIGNAL_SVC;
                        object_path = USER_LOWMEM_OFF_SIGNAL_OP;
                        interface = USER_LOWMEM_OFF_SIGNAL_IF;
                        match = "type='signal',interface='"
                                USER_LOWMEM_OFF_SIGNAL_IF "'";
                        error = _set_handler(context,
                                             service,
                                             object_path,
                                             interface,
                                             match,
                                             event_cb,
                                             event_type,
                                             handler,
                                             user_data);
                        if (event_type == MUALI_EVENT_LOWMEM_BOTH
                            && error == MUALI_ERROR_SUCCESS) {
                                error = muali_set_event_handler(context,
                                                MUALI_EVENT_LOWMEM_ON,
                                                handler,
                                                user_data);
                        }
                        break;
                case MUALI_EVENT_LOWMEM_ON:
                        event_cb = lowmem_signal_handler;
                        service = USER_LOWMEM_ON_SIGNAL_SVC;
                        object_path = USER_LOWMEM_ON_SIGNAL_OP;
                        interface = USER_LOWMEM_ON_SIGNAL_IF;
                        match = "type='signal',interface='"
                                USER_LOWMEM_ON_SIGNAL_IF "'";
                        error = _set_handler(context,
                                             service,
                                             object_path,
                                             interface,
                                             match,
                                             event_cb,
                                             event_type,
                                             handler,
                                             user_data);
                        break;
                default:
                        ULOG_ERR_F("unknown event type %d", event_type);
                        error = MUALI_ERROR_INVALID;
        }
        return error;
}

