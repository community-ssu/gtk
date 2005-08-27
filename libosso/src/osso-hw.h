/**
 * @file osso-hw.c
 * This file is a header for the device state signal handler functions.
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

#ifndef OSSO_HW_H_
# define OSSO_HE_H_

# define STORED_LEN 10
# define STATEPREFIX "/tmp/devicestate"

# define OSSO_DEVSTATE_MODE_FILE STATEPREFIX"/device_mode"

# define _set_state_cb(hwstate, cb_f, data_p) do {\
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

# define _unset_state_cb(hwstate) do {\
    if((state->hwstate) && (osso->hw_cbs.hwstate.set)) { \
	dprint("Unsetting handler for signal %s"\
		   ,#hwstate); \
	osso->hw_cbs.hwstate.cb = NULL; \
	osso->hw_cbs.hwstate.data = NULL; \
	osso->hw_cbs.hwstate.set = FALSE; \
	dbus_bus_remove_match(osso->sys_conn, "type='signal',interface='" \
			      MCE_SIGNAL_IF "',member='"#hwstate"'"\
			      , NULL); \
	dbus_connection_flush(osso->sys_conn); \
    } \
}while(0)

# define _state_is_unset() ( (!osso->hw_cbs.shutdown_ind.set) && \
                             (!osso->hw_cbs.memory_low_ind.set) && \
                             (!osso->hw_cbs.save_unsaved_data_ind.set) && \
				 (!osso->hw_cbs.system_inactivity_ind.set) && \
				 (!osso->hw_cbs.sig_device_mode_ind.set) )

# define _call_state_cb(hwstate) do {\
    if( osso->hw_cbs.hwstate.set && \
	    (dbus_message_is_signal(msg, MCE_SIGNAL_IF, #hwstate) \
		 ==TRUE) ) \
    { \
	dprint("Calling handler for signal %s at %p with data = %p"\
		   , #hwstate , osso->hw_cbs.hwstate.cb, &osso->hw_state); \
	(osso->hw_cbs.hwstate.cb)(&osso->hw_state, osso->hw_cbs.hwstate.data); \
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED; \
    } \
}while(0)

# define _set_state(signal, hwstate, newstate) do {  \
    if(strcmp(signal, #hwstate)==0) {\
	dprint("signal is %s", #hwstate); \
        if (strcmp(#hwstate, "sig_device_mode_ind")==0)\
        {\
        FILE *f;\
          if (strcmp((gchar *)newstate, MCE_NORMAL_MODE) == 0) {\
	    osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_NORMAL;\
	  }\
	  else if (strcmp((gchar *)newstate, MCE_FLIGHT_MODE) == 0) {\
	    osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_FLIGHT;\
	  }\
	  else if (strcmp((gchar *)newstate, MCE_OFFLINE_MODE) == 0) {\
	    osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_OFFLINE;\
	  }\
	  else if (strcmp((gchar *)newstate, MCE_INVALID_MODE) == 0) {\
	    osso->hw_state.sig_device_mode_ind = OSSO_DEVMODE_INVALID;\
	  }\
        f = fopen(OSSO_DEVSTATE_MODE_FILE, "w");\
        if (f != NULL) {\
	  fprintf(f, "%s", (gchar *)newstate); \
          fclose(f); \
        } \
        break;\
       } \
           osso->hw_state.hwstate = (gboolean)newstate;\
       } \
}while(0)

static void _osso_hw_state_get(osso_context_t *osso);
static DBusHandlerResult _hw_handler(osso_context_t *osso,
				     DBusMessage *msg,
				     gpointer data);

#endif /* OSSO_HW_H_ */
