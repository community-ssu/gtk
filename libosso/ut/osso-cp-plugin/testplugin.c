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

#define TESTPLUGIN "unit_test"

struct plugin_data {
    gint action;
};

osso_return_t execute(osso_context_t *osso, gpointer data,
		      gboolean user_activated);
gchar * get_service_name(osso_context_t *osso, gpointer data);

osso_return_t execute(osso_context_t *osso, gpointer data,
		      gboolean user_activated)
{
    printf("This is execute\n");
    fflush(stdout);

    return OSSO_OK;
}

gchar * get_service_name(osso_context_t *osso, gpointer data)
{
    return TESTPLUGIN;
}
