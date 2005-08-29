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
#define OSSO_HW_H_

#define STORED_LEN 10
#define STATEPREFIX "/tmp/devicestate"

#define OSSO_DEVSTATE_MODE_FILE STATEPREFIX"/device_mode"

#define _unset_state_cb(hwstate) do {\
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

#define _state_is_unset() ( (!osso->hw_cbs.shutdown_ind.set) && \
                            (!osso->hw_cbs.memory_low_ind.set) && \
                            (!osso->hw_cbs.save_unsaved_data_ind.set) && \
                            (!osso->hw_cbs.system_inactivity_ind.set) && \
                            (!osso->hw_cbs.sig_device_mode_ind.set) )

static void read_device_state_from_file(osso_context_t *osso);
static DBusHandlerResult signal_handler(osso_context_t *osso,
                                        DBusMessage *msg, gpointer data);

#endif /* OSSO_HW_H_ */
