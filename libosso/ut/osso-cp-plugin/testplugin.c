/**
 * @file test_lib.c
 * This is just a dummy "plugin" to test the functionality of the ctrlpanel
 * plugin functions.
 *
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
