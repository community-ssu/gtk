/*
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

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libosso.h>

#include "osso-internal.h"

void _top_cb_f(const gchar *arguments, gpointer data);

#define APP_NAME "test_top_msg"
#define APP_VER "0.0.1"
#define TESTFILE "/tmp/"APP_NAME".tmp"

int run = 1;
    
int main(int nargs, char *argv[])
{
    FILE *f;
    GMainLoop *loop;
    osso_context_t *osso;

    f = fopen("/tmp/foo.bar", "w");

    fprintf(f, "launched\n");
    fclose(f);
    loop = g_main_loop_new(NULL, FALSE);

    osso = osso_initialize(APP_NAME, APP_VER, TRUE, NULL);
    if(osso == NULL)
	return 1;

    osso_application_set_top_cb(osso,
        (osso_application_top_cb_f*)_top_cb_f, (gpointer)loop);

    g_main_loop_run(loop);

    return 0;
}

void _top_cb_f(const gchar *arguments, gpointer data)
{
    FILE *f;
    GMainLoop *loop;
    
    loop = (GMainLoop *)data;
    
    if(strncmp(arguments,"print", 5) == 0) {    
	f = fopen(TESTFILE, "w");

	fprintf(f, "%s\n", arguments);
	fflush(f);
	fclose(f);
    }
    else if(strcmp(arguments,"exit") == 0) {
	g_main_loop_quit(loop);
    }
    else {    
	f = fopen(TESTFILE, "w");

	fprintf(f, "%s\n", arguments);
	fflush(f);
	fclose(f);
	g_main_loop_quit(loop);
    }
    
    return;
}
