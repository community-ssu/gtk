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

static DBusHandlerResult _mime_handler(osso_context_t *osso,
				       DBusMessage *msg,
				       gpointer data);
static char * _get_arg(DBusMessageIter *iter);

osso_return_t osso_mime_set_cb(osso_context_t *osso, osso_mime_cb_f *cb,
			       gpointer data)
{
    char interface[MAX_IF_LEN + 1];
    if (osso == NULL || cb == NULL) {
        ULOG_ERR_F("invalid arguments");
	return OSSO_INVALID;
    }

    g_snprintf(interface, MAX_IF_LEN, OSSO_BUS_ROOT ".%s",
               osso->application);

    osso->mime.func = cb;
    osso->mime.data = data;
    
    _msg_handler_set_cb_f(osso, interface, _mime_handler, NULL, TRUE);
    return OSSO_OK;
}

osso_return_t osso_mime_unset_cb(osso_context_t *osso)
{
    char interface[MAX_IF_LEN + 1];
    if (osso == NULL) {
        ULOG_ERR_F("osso context is NULL");
	return OSSO_INVALID;
    }
    if (osso->mime.func == NULL) {
        ULOG_ERR_F("MIME callback is NULL");
	return OSSO_INVALID;
    }

    g_snprintf(interface, MAX_IF_LEN, OSSO_BUS_ROOT ".%s",
               osso->application);

    osso->mime.func = NULL;
    osso->mime.data = NULL;
    
    _msg_handler_rm_cb_f(osso, interface, _mime_handler, TRUE);
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
    char interface[MAX_IF_LEN + 1] = {0};
    g_assert(osso != NULL);
    g_snprintf(interface, MAX_IF_LEN, OSSO_BUS_ROOT ".%s",
               osso->application);

    if (dbus_message_is_method_call(msg, interface, OSSO_BUS_MIMEOPEN)) {
	int argc, idx = 0;
        gchar **argv = NULL;
        gchar *arg = NULL;
	DBusMessageIter iter;

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
        g_assert(idx == argc);
        argv[idx] = NULL;
	
	(osso->mime.func)(osso->mime.data, argc, argv);
        free(argv);

	return DBUS_HANDLER_RESULT_HANDLED;
    }
    else {
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
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
