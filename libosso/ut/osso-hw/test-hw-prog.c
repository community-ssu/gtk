/**
 * This file is part of libosso
 *
 * Copyright (C) 2005-2006 Nokia Corporation. All rights reserved.
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

#include <unistd.h>

#define APP_NAME "test_hw"
#define APP_VER "0.0.1"
#define TESTFILE "/tmp/hwsignal"

void hw_cb(osso_hw_state_t *state, gpointer data);

int main(int argc, char **argv)
{
    GMainLoop *loop;
    osso_context_t *osso;

    loop = g_main_loop_new(NULL, FALSE);

    osso = osso_initialize(APP_NAME, APP_VER, TRUE, NULL);
    if(osso == NULL) {
	dprint("no D-BUS found!!\n");
	return 1;
    }

    osso_hw_set_event_cb(osso, NULL, hw_cb, loop);

    g_main_loop_run(loop);

    osso_hw_unset_event_cb(osso, NULL);
    osso_deinitialize(osso);
    return 0;
}


void hw_cb(osso_hw_state_t *state, gpointer data)
{
    GMainLoop *loop;
    FILE *f;
    
    dprint("got a signal");
    loop = (GMainLoop *)data;

    f = fopen(TESTFILE, "w");
    if(f == NULL) {
	dprint("unable to open file %s", TESTFILE);
    }
    else {
	if(state->shutdown_ind) {
	    fprintf(f,"reboot\n");
	    dprint("reboot");
	}
	if(state->memory_low_ind) {
	    fprintf(f,"memlow\n");
	    dprint("memlow");
	}
	if(state->save_unsaved_data_ind) {
	    fprintf(f,"batlow\n");
	    dprint("batlow");
	}
	if(state->system_inactivity_ind) {
	    fprintf(f,"minact\n");
	    dprint("minact");
	}
	if(state->sig_device_mode_ind) {
	    fprintf(f,"flightmode\n");
	    dprint("flightmode");
	}
	fclose(f);
	fflush(f);
	sync();
    }
    g_main_loop_quit((GMainLoop *)data);
    return;
}
