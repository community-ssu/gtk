/**
 * @file test_lib.c
 * This is just a dummy "plugin" to test the functionality of the ctrlpanel
 * plugin functions.
 *
 *
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
 *               
 */

#include <stdio.h>

/* function prototypes */
#include <libosso.h>

#define TESTPLUGIN "testplugin2"

struct plugin_data {
    gint action;
};

struct state_data{
    gint32 a;
    gint32 b;
    gint32 c;
    gint32 d;
}plugin_state;

osso_return_t execute(osso_context_t *osso, gpointer data,
		      gboolean user_activated);

osso_return_t execute(osso_context_t *osso, gpointer data,
		      gboolean user_activated)
{    
    struct state_data *sd, *rd;

    rd = (struct state_data *)data;
    if(user_activated) { /*FIXME do some real testing here*/
	sd = rd;
	
	printf("I got this state data:\n");
	printf("a=%d, b=%d, c=%d, d=%d\n", sd->a, sd->b, sd->c, sd->d);
	fflush(stderr);
	fflush(stdout);
	if(rd != NULL) {
	    rd->a = sd->a;
	    rd->b = sd->b;
	    rd->c = sd->c;
	    rd->d = sd->d;
	}
    }
    else {
	rd->a = 1;
	rd->b = 2;
	rd->c = 3;
	rd->d = 4;
    }

    return OSSO_OK;
}

osso_return_t save_state(osso_context_t *osso, gboolean user_activated, 
			 gpointer data)
{
    plugin_state.a = 11;
    plugin_state.b = 2;
    plugin_state.c = 19;
    plugin_state.d = 77;
    
    
    return OSSO_OK;
}
gchar * get_service_name(osso_context_t *osso, gpointer data)
{
    return TESTPLUGIN;
}
