/**
 * @file osso-email.c
 * This file implements autosave feature.
 * 
 * This file is part of libosso
 *
 * Copyright (C) 2005 Nokia Corporation. All rights reserved.
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
#include "osso-internal.h"

#define AUTOSAVE_TIMEOUT 120000 /* 2 mins */

static gboolean _autosave_timeout(gpointer data);

osso_return_t osso_application_set_autosave_cb(osso_context_t *osso,
				      osso_application_autosave_cb_f *cb,
				      gpointer data)
{
    if ( (osso == NULL) || (cb == NULL) ) {
        ULOG_ERR_F("Invalid arguments");
	return OSSO_INVALID;
    }
    if (osso->autosave.func != NULL) {
	ULOG_ERR_F("autosave callback already set!");
	return OSSO_ERROR;
    }

    osso->autosave.func = cb;
    osso->autosave.data = data;
    osso->autosave.id = 0;

    return OSSO_OK;
}

osso_return_t osso_application_unset_autosave_cb(osso_context_t *osso,
					osso_application_autosave_cb_f *cb,
					gpointer data)
{
    if ( (osso == NULL) || (cb == NULL) ) {
        ULOG_ERR_F("Invalid arguments");
	return OSSO_INVALID;
    }
    if (osso->autosave.func == NULL) {
	ULOG_WARN_F("no autosave callback set");
    }
    else {
	if(osso->autosave.id != 0) {
	    g_source_remove(osso->autosave.id);
	    osso->autosave.id = 0;
	}
	osso->autosave.func = NULL;
	osso->autosave.data = NULL;
    }
    return OSSO_OK;
}

osso_return_t osso_application_userdata_changed(osso_context_t *osso)
{
    if (osso == NULL) {
        ULOG_ERR_F("Invalid argument");
	return OSSO_INVALID;
    }
    if (osso->autosave.func == NULL) {
	ULOG_ERR_F("no autosave callback set!");
	return OSSO_ERROR;
    }

    osso->autosave.id = g_timeout_add(AUTOSAVE_TIMEOUT,
				       _autosave_timeout, osso);
    
    return OSSO_OK;
}

osso_return_t osso_application_autosave_force(osso_context_t *osso)
{
    if (osso == NULL) {
	return OSSO_INVALID;
    }
    if (osso->autosave.func == NULL) {
	ULOG_ERR_F("no autosave callback set!");
	return OSSO_ERROR;
    }

    if(osso->autosave.id != 0) {
	g_source_remove(osso->autosave.id);
	osso->autosave.id = 0;
    }

    (osso->autosave.func)(osso->autosave.data);

    return OSSO_OK;
}

static gboolean _autosave_timeout(gpointer data)
{
    osso_context_t *osso = data;

    (osso->autosave.func)(osso->autosave.data);
    osso->autosave.id = 0;

    return FALSE;
}
