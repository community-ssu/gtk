/**
 * Copyright (C) 2005  Nokia
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
 */               
#include "osso-internal.h"

#include <unistd.h>

#define APP_NAME "test_hw"
#define APP_VER "0.0.1"
#define TESTFILE "/tmp/testossoexit"

void exit_cb(gboolean die_now, gpointer data);

int main(int argc, char **argv)
{
    GMainLoop *loop;
    osso_context_t *osso;

    ULOG_OPEN("testexit");
    loop = g_main_loop_new(NULL, FALSE);

    ULOG_INFO_F("Initializing osso");
    osso = osso_initialize(APP_NAME, APP_VER, TRUE, NULL);
    if(osso == NULL) {
	dprint("no D-BUS found!!\n");
	return 1;
    }

    osso_application_set_exit_cb(osso, exit_cb, loop);

    g_main_loop_run(loop);

    osso_hw_unset_event_cb(osso, NULL);
    osso_deinitialize(osso);
    LOG_CLOSE();
    return 0;
}


void exit_cb(gboolean die_now, gpointer data)
{
    FILE *f;
    
    ULOG_INFO_F("got an exit signal");
    
    f = fopen(TESTFILE, "w");
    if(f == NULL) {
	ULOG_INFO_F("unable to open file %s", TESTFILE);
    }
    else {
	if(die_now) {
	    ULOG_INFO_F("got signal 'exit'");
	    fprintf(f, "exit\n");
	}
	else {
	    ULOG_INFO_F("got signal 'exit_if_possible'");
	    fprintf(f, "exit_if_possible\n");
	}
    }
    fclose(f);
    fflush(f);
    sync();
    g_main_loop_quit((GMainLoop *)data);
    return;
}

