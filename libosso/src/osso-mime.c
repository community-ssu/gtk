/**
 * @file osso-mime.c
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

#include "osso-internal.h"
#include <assert.h>

static void _mime_handler(osso_context_t *osso,
                          DBusMessage *msg,
                          _osso_callback_data_t *mime,
                          muali_bus_type dbus_type);
static char * _get_arg(DBusMessageIter *iter);

osso_return_t osso_mime_set_cb(osso_context_t *osso,
                               osso_mime_cb_f *cb,
			       gpointer data)
{
    _osso_callback_data_t *mime;

    if (osso == NULL || cb == NULL) {
        ULOG_ERR_F("invalid arguments");
	return OSSO_INVALID;
    }

    mime = calloc(1, sizeof(_osso_callback_data_t));
    if (mime == NULL) {
        ULOG_ERR_F("calloc() failed");
        return OSSO_ERROR;
    }

    mime->user_cb = cb;
    mime->user_data = data;
    mime->data = NULL;
    
    _msg_handler_set_cb_f_free_data(osso,
                                    osso->service,
                                    osso->object_path,
                                    osso->interface,
                                    _mime_handler, mime, TRUE);
    return OSSO_OK;
}

osso_return_t osso_mime_unset_cb(osso_context_t *osso)
{
    gboolean ret;

    if (osso == NULL) {
        ULOG_ERR_F("invalid parameters");
	return OSSO_INVALID;
    }

    ret = _msg_handler_rm_cb_f(osso,
                               osso->service,
                               osso->object_path,
                               osso->interface,
                               (const _osso_handler_f*)_mime_handler,
                               NULL, TRUE);
    return ret ? OSSO_OK : OSSO_INVALID;
}

osso_return_t osso_mime_unset_cb_full(osso_context_t *osso,
                                      osso_mime_cb_f *cb,
			              gpointer data)
{
    _osso_callback_data_t mime;
    gboolean ret;

    if (osso == NULL || cb == NULL) {
        ULOG_ERR_F("invalid parameters");
	return OSSO_INVALID;
    }

    mime.user_cb = cb;
    mime.user_data = data;
    mime.data = NULL;

    ret = _msg_handler_rm_cb_f(osso,
                               osso->service,
                               osso->object_path,
                               osso->interface,
                               (const _osso_handler_f*)_mime_handler,
                               &mime, TRUE);
    return ret ? OSSO_OK : OSSO_INVALID;
}

static int get_message_arg_count(DBusMessage *m)
{
    DBusMessageIter iter;
    int count;

    if (!dbus_message_iter_init(m, &iter)) {
        return 0;
    }
    for (count = 1; dbus_message_iter_next(&iter); ) {
        ++count;
    }
    return count;
}

static void _mime_handler(osso_context_t *osso,
                          DBusMessage *msg,
                          _osso_callback_data_t *mime,
                          muali_bus_type dbus_type)
{
    if (dbus_message_is_method_call(msg, osso->interface,
                                    OSSO_BUS_MIMEOPEN)) {
	int argc, idx = 0;
        gchar **argv = NULL;
        gchar *arg = NULL;
	DBusMessageIter iter;
        osso_mime_cb_f *handler;
        DBusMessage *reply = NULL;

        argc = get_message_arg_count(msg);
        if (argc == 0) {
            ULOG_ERR_F("No arguments in message");
            return;
        }
        
        argv = (gchar**)calloc(argc + 1, sizeof(gchar*));
        if (argv == NULL) {
            ULOG_ERR_F("Not enough memory");
            return;
        }
	
        if (!dbus_message_iter_init(msg, &iter)) {
            ULOG_ERR_F("No arguments - should not happen since"
                       " it was already checked");
            free(argv);
            return;
        }
	while ((arg = _get_arg(&iter)) != NULL) {
	    argv[idx++] = arg;
	    dbus_message_iter_next(&iter);
	}
        assert(idx == argc);
        argv[idx] = NULL;
	
        handler = mime->user_cb;
	(*handler)(mime->user_data, argc, argv);
        free(argv);

        /* send an empty reply message */
        reply = dbus_message_new_method_return(msg);
        if (reply != NULL) {
            dbus_connection_send(osso->cur_conn, reply, NULL);
            dbus_message_unref(reply);
        }
    }
}

static char * _get_arg(DBusMessageIter *iter)
{
    int type;

    type = dbus_message_iter_get_arg_type(iter);

    if(type != DBUS_TYPE_STRING) {
	return NULL;
    }
    else {
        char *str = NULL;
        dbus_message_iter_get_basic(iter, &str);
        return str;
    }
}
