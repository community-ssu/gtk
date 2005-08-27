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
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libosso.h>

#include "osso-internal.h"

#define APP_NAME "test_osso_sbevent"
#define APP_VER "0.0.1"
#define LOGFILE "/tmp/"APP_NAME".log"
#define TESTFILE "/tmp/sbevent.tmp"

gint cb(const gchar *interface, const gchar *method,
	GArray *arguments, gpointer data, osso_rpc_t *retval);
void _top_cb_f(const gchar *arguments, gpointer data);

int main(int nargs, char *argv[])
{
    FILE *f;
    GMainLoop *loop;
    osso_context_t *osso;
    
    f = fopen(LOGFILE, "a");
    fprintf(f, "launched\n");
    fclose(f);
    loop = g_main_loop_new(NULL, FALSE);

    osso = osso_initialize(APP_NAME, APP_VER, TRUE, NULL);
    if(osso == NULL)
	return 1;
    
    osso_application_set_top_cb(osso, _top_cb_f, (gpointer)loop);
    osso_rpc_set_cb_f(osso, "com.nokia.statusbar", "/com/nokia/statusbar",
                      "com.nokia.statusbar", cb, (gpointer)loop, TRUE);

    g_main_loop_run(loop);

    return 0;
}

gint cb(const gchar *interface, const gchar *method,
	GArray *arguments, gpointer data, osso_rpc_t *retval)
{
    gboolean stop = FALSE;
    gint i;
    GMainLoop *loop;
    FILE *f;
    
    loop = (GMainLoop *)data;
    f = fopen(TESTFILE, "w");

    dprint("");
    if(f == NULL) {
	retval->type = DBUS_TYPE_INT32;
	retval->value.i = OSSO_ERROR;
	return OSSO_OK;
    }
	
    for(i=0; i < arguments->len; i++) {
	osso_rpc_t *arg;
	arg = &g_array_index(arguments, osso_rpc_t, i);
	switch(arg->type) {
	  case DBUS_TYPE_INVALID:
	    break;
	  case DBUS_TYPE_INT32:
	    fprintf(f,"%d",arg->value.i);
	    if(arg->value.i == 1)
		stop = TRUE;
	    break;
	  case DBUS_TYPE_UINT32:
	    fprintf(f,"%d",arg->value.u);
	    break;
	  case DBUS_TYPE_BOOLEAN:
	    fprintf(f,"%s",arg->value.b?"TRUE":"FALSE");
	    break;
	  case DBUS_TYPE_DOUBLE:
	    fprintf(f,"%f",arg->value.d);
	    break;
	  case DBUS_TYPE_STRING:
	    fprintf(f,"%s",arg->value.s);
	    break;
	  default:
	    fprintf(f,"%x",arg->value.u);
	    break;	    
	}
	if(i < (arguments->len-1))
	    fprintf(f," ");
    }
    fprintf(f,"\n");
    fclose(f);
    retval->type = DBUS_TYPE_INT32;
    retval->value.i = OSSO_OK;
    
    if(stop)
	g_main_loop_quit(loop);
	
    return OSSO_OK;
}

void _top_cb_f(const gchar *arguments, gpointer data)
{
    return;
}
