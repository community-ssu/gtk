/**
 * @file osso-hw.h
 * This file is a header for the device state signal handler functions.
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

#ifndef OSSO_HW_H_
#define OSSO_HW_H_

#include <unistd.h>
#include <errno.h>

#define STORED_LEN 10
#define OSSO_DEVSTATE_MODE_FILE "/tmp/.libosso_device_mode_cache"

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
    } \
}while(0)

#define _state_is_unset() ( (!osso->hw_cbs.shutdown_ind.set) && \
                            (!osso->hw_cbs.save_unsaved_data_ind.set) && \
                            (!osso->hw_cbs.system_inactivity_ind.set) && \
                            (!osso->hw_cbs.sig_device_mode_ind.set) )

static void read_device_state_from_file(osso_context_t *osso);

#endif /* OSSO_HW_H_ */
