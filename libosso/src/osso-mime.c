/**
 * @file osso-mime.c
 * This file implements the function to set the mime callback function
 * which is called whenever a D-BUS message idicates that a file should
 * be opened.
 * 
 * Copyright (C) 2005-2006 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License.
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
#include <assert.h>

static DBusHandlerResult _mime_handler(osso_context_t *osso,
				       DBusMessage *msg,
				       gpointer data);
static char * _get_arg(DBusMessageIter *iter);

osso_return_t osso_mime_set_cb(osso_context_t *osso,
                               osso_mime_cb_f *cb,
			       gpointer data)
{
    _osso_mime_t *mime;

    if (osso == NULL || cb == NULL) {
        ULOG_ERR_F("invalid arguments");
	return OSSO_INVALID;
    }

    mime = calloc(1, sizeof(_osso_mime_t));
    if (mime == NULL) {
        ULOG_ERR_F("calloc() failed");
        return OSSO_ERROR;
    }

    mime->func = cb;
    mime->data = data;
    
    _msg_handler_set_cb_f_free_data(osso,
                                    osso->service,
                                    osso->object_path,
                                    osso->interface,
                                    _mime_handler, mime, TRUE);
    return OSSO_OK;
}

static DBusHandlerResult match_cb(osso_context_t *osso,
                                  DBusMessage *msg,
                                  gpointer data)
{
    /* we just match all because the API does not allow
     * us to do more */
    return DBUS_HANDLER_RESULT_HANDLED;
}

osso_return_t osso_mime_unset_cb(osso_context_t *osso)
{
    if (osso == NULL) {
        ULOG_ERR_F("invalid parameters");
	return OSSO_INVALID;
    }

    _msg_handler_rm_cb_f(osso,
                         osso->service,
                         osso->object_path,
                         osso->interface,
                         match_cb, TRUE,
                         RM_CB_IS_MATCH_FUNCTION, NULL);
    return OSSO_OK;
}

static DBusHandlerResult match_cb_full(osso_context_t *osso,
                                       DBusMessage *msg,
                                       gpointer data)
{
    _osso_rm_cb_match_t *match_data = data;
    _osso_mime_t *mime;
    _osso_handler_t *handler;

    mime = match_data->data;
    handler = match_data->handler;

    if ((unsigned)mime->func == (unsigned)handler->handler
        && mime->data == handler->data) {
        return DBUS_HANDLER_RESULT_HANDLED;
    } else {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
}

osso_return_t osso_mime_unset_cb_full(osso_context_t *osso,
                                      osso_mime_cb_f *cb,
			              gpointer data)
{
    _osso_mime_t *mime;

    if (osso == NULL || cb == NULL) {
        ULOG_ERR_F("invalid parameters");
	return OSSO_INVALID;
    }

    mime = calloc(1, sizeof(_osso_mime_t));
    if (mime == NULL) {
        ULOG_ERR_F("calloc() failed");
        return OSSO_ERROR;
    }

    mime->func = cb;
    mime->data = data;

    _msg_handler_rm_cb_f(osso,
                         osso->service,
                         osso->object_path,
                         osso->interface,
                         match_cb_full, TRUE,
                         RM_CB_IS_MATCH_FUNCTION, mime);
    free(mime);
    return OSSO_OK;
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

static DBusHandlerResult _mime_handler(osso_context_t *osso,
				       DBusMessage *msg,
				       gpointer data)
{
    assert(osso != NULL);

    if (dbus_message_is_method_call(msg, osso->interface,
                                    OSSO_BUS_MIMEOPEN)) {
	int argc, idx = 0;
        gchar **argv = NULL;
        gchar *arg = NULL;
	DBusMessageIter iter;
        _osso_mime_t *mime = data;

        assert(mime != NULL);
        assert(mime->func != NULL);

        argc = get_message_arg_count(msg);
        if (argc == 0) {
            ULOG_ERR_F("No arguments in message");
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        
        argv = (gchar**)calloc(argc + 1, sizeof(gchar*));
        if (argv == NULL) {
            ULOG_ERR_F("Not enough memory");
            return DBUS_HANDLER_RESULT_HANDLED;
        }
	
        if (!dbus_message_iter_init(msg, &iter)) {
            ULOG_ERR_F("No arguments - should not happen since"
                       " it was already checked");
            free(argv);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
	while ((arg = _get_arg(&iter)) != NULL) {
	    argv[idx++] = arg;
	    dbus_message_iter_next(&iter);
	}
        assert(idx == argc);
        argv[idx] = NULL;
	
	(mime->func)(mime->data, argc, argv);
        free(argv);
    }
    /* have to return this, because there could be more
     * MIME callbacks */
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
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
