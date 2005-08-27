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

/* Ideally, these definitions should come from elsewhere,
   but it is not possible at the moment... (bug #10866) */

#define MCE_SERVICE			"com.nokia.mce"
#define MCE_REQUEST_PATH		"/com/nokia/mce/request"
#define MCE_REQUEST_IF			"com.nokia.mce.request"
#define MCE_SIGNAL_PATH			"/com/nokia/mce/signal"
#define MCE_SIGNAL_IF			"com.nokia.mce.signal"
#define MCE_DISPLAY_ON_REQ		"req_display_state_on"
#define MCE_PREVENT_BLANK_REQ		"req_display_blanking_pause"
#define MCE_DEVICE_MODE_SIG		"sig_device_mode_ind"

#define MCE_INACTIVITY_SIG              "system_inactivity_ind"

#define MCE_NORMAL_MODE		"normal"	/* normal mode */
#define MCE_FLIGHT_MODE		"flight"	/* flight mode */
#define MCE_OFFLINE_MODE	"offline"	/* offline mode; unsupported! */
#define MCE_INVALID_MODE	"invalid"	/* should *never* occur! */


#include "osso-internal.h"
#include "osso-hw.h"

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
#if 0
#define _set_state_cb(hwstate, cb_f, data_p) do {\
        if(state->hwstate) { \
          osso->hw->hwstate.cb = cb_f; \
          osso->hw->hwstate.data = data_p; \
          osso->hw->hwstate.set = TRUE; \
          if (!osso->hw->hwstate.set && osso->hw_state->hwstate) { \
            (cb)(&temp, data); \
          } \
          dbus_bus_add_match(osso->sys_conn, "type='signal',interface='" \
          MCE_SIGNAL_IF "',member='"#hwstate"'" \
                  , NULL); \
         dbus_connection_flush(osso->sys_conn); \
        } \
}while(0)
static void _set_state_cb(osso_context_t *osso, _osso_hw_cb_data_t *field,
                          gboolean set, gboolean curr_state,
                          const osso_hw_cb_f *cb,
                          const gpointer data)
{
    g_assert(osso != NULL);
    if (set) {
        field->cb = cb;
        field->data = data;
        field->set = TRUE;
        if (curr_state) {
            (*cb)(TODO, data);
        } 
        dbus_bus_add_match(osso->sys_conn, "type='signal',interface='"
                           MCE_SIGNAL_IF "',member='"#hwstate"'", NULL);
         
    }
}
#endif

osso_return_t osso_hw_set_event_cb(osso_context_t *osso,
				   osso_hw_state_t *state,
				   osso_hw_cb_f *cb, gpointer data)
{
    static const osso_hw_state_t default_mask = {TRUE,TRUE,TRUE,TRUE,1};
    if (osso == NULL || cb == NULL) {
	ULOG_ERR_F("invalid parameters");
	return OSSO_INVALID;
    }
    if (osso->sys_conn == NULL) {
	ULOG_ERR_F("error: no system bus connection");
	return OSSO_INVALID;
    }

    if (state == NULL) {
	state = &default_mask;
    }

    _osso_hw_state_get(osso);

    /* Now set the HW state / add monitoring if requested */
    
    if (state->shutdown_ind) {
        osso->hw_cbs.shutdown_ind.cb = cb;
        osso->hw_cbs.shutdown_ind.data = data;
        if (!osso->hw_cbs.shutdown_ind.set) {
            /* if callback was not previously registered, add match */
            dbus_bus_add_match(osso->sys_conn, "type='signal',interface='"
                MCE_SIGNAL_IF "',member='shutdown_ind'", NULL);
        }
        osso->hw_cbs.shutdown_ind.set = TRUE;
    }

        /*
    _set_state_cb(memory_low_ind, cb, data);
    _set_state_cb(save_unsaved_data_ind, cb, data);
    _set_state_cb(system_inactivity_ind, cb, data);
    _set_state_cb(sig_device_mode_ind, cb, data);
    */
    _msg_handler_set_cb_f(osso, MCE_SIGNAL_IF, _hw_handler, state,
			  FALSE);
    /* TODO: kutsu callbackia */
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

    if(_state_is_unset())
	_msg_handler_rm_cb_f(osso, MCE_SIGNAL_IF,
			     _hw_handler, FALSE);
    
    return OSSO_OK;    
}

static void _osso_hw_state_get(osso_context_t *osso)
{
    FILE *f;
    gchar storedstate[STORED_LEN];
    
    g_assert(osso != NULL);
    
    f = fopen(OSSO_DEVSTATE_MODE_FILE, "r");
    if(f != NULL) {
      if (fgets(storedstate, STORED_LEN, f) != NULL) {
	
	  if (strcmp(storedstate, MCE_NORMAL_MODE) == 0) {
	    osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_NORMAL;
	  }
	  else if (strcmp(storedstate, MCE_FLIGHT_MODE) == 0) {
	    osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_FLIGHT;
	  }
	  else if (strcmp(storedstate, MCE_OFFLINE_MODE) == 0) {
	    osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_OFFLINE;
	  }
	  else if (strcmp(storedstate, MCE_INVALID_MODE) == 0) {
	    osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_INVALID;
	  }
      }
      fclose(f);
    }
    else {
      ULOG_ERR_F("no device state file found, assuming NORMAL device state");
      osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_NORMAL;
    }
}

static DBusHandlerResult _hw_handler(osso_context_t *osso,
                                     DBusMessage *msg,
                                     gpointer data)
{
    const gchar *signal = NULL, *mode_str = NULL;
    gboolean old_inactivity_state = osso->hw_state->system_inactivity_ind;
    gboolean inactivity_state;
    DBusMessageIter iter;
    int type;
    
    dprint("");
    signal = dbus_message_get_member(msg);
    dbus_message_iter_init(msg, &iter);

    type = dbus_message_iter_get_arg_type(&iter);

    if (signal == NULL)
      {
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
      }
    
    dprint("argument type = '%c'",type);

    /* We need to get the argument type for mode
       signals, others have no arguments specified */

    if (strcmp(signal, MCE_INACTIVITY_SIG) == 0 &&
	type != DBUS_TYPE_BOOLEAN)
      {
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
      }

    if (strcmp(signal, MCE_INACTIVITY_SIG) == 0)
      {
	inactivity_state = dbus_message_iter_get_boolean(&iter);
      }

    if (strcmp(signal, MCE_DEVICE_MODE_SIG) == 0 &&
	     type != DBUS_TYPE_STRING)
      {
	ULOG_WARN_F("warning: invalid arguments in HW signal '%s'", signal);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
      }

    if (strcmp(signal, MCE_DEVICE_MODE_SIG) == 0)
      {
	mode_str = dbus_message_iter_get_string(&iter);
	if ( (strcmp(mode_str, MCE_NORMAL_MODE) != 0) &&
	     (strcmp(mode_str, MCE_FLIGHT_MODE) != 0) &&
	     (strcmp(mode_str, MCE_OFFLINE_MODE) != 0) &&
	     (strcmp(mode_str, MCE_INVALID_MODE) != 0) )
	  {
	    ULOG_WARN_F("warning: invalid device mode '%s'", mode_str);
	    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	  }
      }

     /* Unsetting the non-stateful signals before possible call should
	keep the backward compability without side effects. Shutdown is
        initialized by checking it from disk elsewhere. */

     osso->hw_state.shutdown_ind = FALSE;
     osso->hw_state.memory_low_ind = FALSE;
     osso->hw_state.save_unsaved_data_ind = FALSE;
     /*     osso->hw_state->system_inactivity_ind = FALSE; */

     _set_state(signal, shutdown_ind, TRUE);

     /* To keep things single for identification of signals that do not
	have a state on the callback function, just set their state to TRUE.
     */
     _set_state(signal, shutdown_ind, TRUE);
     _set_state(signal, memory_low_ind, TRUE);
     _set_state(signal, save_unsaved_data_ind, TRUE);

     _set_state(signal, sig_device_mode_ind, mode_str);

     _call_state_cb(shutdown_ind);
     _call_state_cb(memory_low_ind);
     _call_state_cb(save_unsaved_data_ind);
     
     /* Do not trigger system inactivity callback if it's not changed */

     if (old_inactivity_state != inactivity_state)
       {
	 _set_state(signal, system_inactivity_ind, inactivity_state);
	 _call_state_cb(system_inactivity_ind);
       }

     _call_state_cb(sig_device_mode_ind);

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
