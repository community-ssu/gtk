/**
 * @file osso-mime.c
 * This file implements the function to set the mime callback function
 * which is called whenever a D-BUS message idicates that a file should
 * be opened.
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

#include "osso-internal.h"

static DBusHandlerResult _mime_handler(osso_context_t *osso,
				       DBusMessage *msg,
				       gpointer data);
static gchar * _get_arg(DBusMessageIter *iter);
static void _freeargv(int argc, gchar **argv);

osso_return_t osso_mime_set_cb(osso_context_t *osso, osso_mime_cb_f *cb,
			       gpointer data)
{
    gchar interface[256];
    if (osso == NULL || cb == NULL)
	return OSSO_INVALID;
    if(osso->mime == NULL) {
	osso->mime = (_osso_mime_t *)calloc(1, sizeof(_osso_mime_t));
	if(osso->mime == NULL) {
            ULOG_ERR_F("calloc failed");
	    return OSSO_ERROR;
	}
    }

    g_snprintf(interface, 255, "%s.%s", OSSO_BUS_ROOT, osso->application);

    osso->mime->func = cb;
    osso->mime->data = data;
    
    _msg_handler_set_cb_f(osso, interface, _mime_handler, NULL, TRUE);
    return OSSO_OK;
}

osso_return_t osso_mime_unset_cb(osso_context_t *osso)
{
    gchar interface[256];
    if (osso == NULL)
	return OSSO_INVALID;
    if(osso->mime == NULL) {
	return OSSO_INVALID;
    }

    g_snprintf(interface, 255, "%s.%s", OSSO_BUS_ROOT, osso->application);

    osso->mime->func = NULL;
    osso->mime->data = NULL;
    
    _msg_handler_rm_cb_f(osso, interface, _mime_handler, TRUE);
    return OSSO_OK;
}

static DBusHandlerResult _mime_handler(osso_context_t *osso,
				       DBusMessage *msg,
				       gpointer data)
{
    gchar interface[256] = {0};

    g_snprintf(interface, 255, "%s.%s", OSSO_BUS_ROOT,
	       osso->application);

    if(dbus_message_is_method_call(msg, interface, OSSO_BUS_MIMEOPEN) == TRUE)
    {	
	int argc=0;
	gchar **argv=NULL;
	DBusMessageIter iter;
	
	dbus_message_iter_init(msg, &iter);
	while(TRUE) {
	    gchar *arg, **pp;
	    
	    arg = _get_arg(&iter);
	    if(arg == NULL) {
		_freeargv(argc, argv);
		return DBUS_HANDLER_RESULT_HANDLED;
	    }
	    
	    argc++;
	    pp = realloc(argv, argc*sizeof(gchar*));
	    if(pp == NULL) {
		_freeargv(argc, argv);
		return DBUS_HANDLER_RESULT_HANDLED;
	    }
	    else {
		argv = pp;
	    }
	    argv[argc-1] = arg;
	
	    if(dbus_message_iter_has_next(&iter))
		dbus_message_iter_next(&iter);
	    else
		break;
	}
	
	(osso->mime->func)(osso->mime->data, argc, argv);
    
	return DBUS_HANDLER_RESULT_HANDLED;
    }
    else {
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
}

static gchar * _get_arg(DBusMessageIter *iter)
{
    gint type;
    
    dprint("");

    type = dbus_message_iter_get_arg_type(iter);

    if(type != DBUS_TYPE_STRING) {
	return NULL;
    }
    else {
	return g_strdup(dbus_message_iter_get_string(iter));
    }
}

static void _freeargv(int argc, gchar **argv)
{
    if(argv!=NULL) {
	int i;
	for(i=0;i<argc;i++) {
	    if(argv[i] != NULL) {
		free(argv[i]);
	    }
	}
	free(argv);
    }
    else {
	return;
    }
}
