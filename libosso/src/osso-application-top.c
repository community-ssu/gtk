/**
 * @file osso-init.c
 * This file implements functionality related to the top_application
 * D-Bus message.
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
#include "log-functions.h"
#include "osso-application-top.h"
#include <stdlib.h>

osso_return_t osso_application_top(osso_context_t *osso,
                                   const gchar *application,
                                   const gchar *arguments)
{
    char service[MAX_SVC_LEN], path[MAX_OP_LEN], interface[MAX_IF_LEN];
    guint serial;
    DBusMessage *msg = NULL;
    gchar* copy = NULL;
    const char *arg = "";

    if (osso == NULL) return OSSO_INVALID;
    if (application == NULL) return OSSO_INVALID;

    dprint("Topping application (service) '%s' with args %s",
	   application, arguments);
    
    g_snprintf(service, MAX_SVC_LEN, OSSO_BUS_ROOT ".%s", application);

    copy = appname_to_valid_path_component(application);
    if (copy == NULL) {
        ULOG_ERR_F("failure in appname_to_valid_path_component");
        return OSSO_ERROR;
    }
    g_snprintf(path, MAX_OP_LEN, OSSO_BUS_ROOT_PATH "/%s", copy);
    g_free(copy); copy = NULL;

    g_snprintf(interface, MAX_IF_LEN, "%s", service);

    dprint("New method: %s:%s:%s:%s",service,path,interface,OSSO_BUS_TOP);
    msg = dbus_message_new_method_call(service, path, interface,
				       OSSO_BUS_TOP);

    if (arguments != NULL) {
        arg = arguments;
    }
    if (!dbus_message_append_args(msg, DBUS_TYPE_STRING, &arg,
                                  DBUS_TYPE_INVALID)) {
	dbus_message_unref(msg);
	return OSSO_ERROR;
    }
    
    dbus_message_set_no_reply(msg, TRUE);
    dbus_message_set_auto_start(msg, TRUE);

    if (!dbus_connection_send(osso->conn, msg, &serial)) {
        ULOG_ERR_F("dbus_connection_send failed");
	dbus_message_unref(msg);
	return OSSO_ERROR;
    }
    else {
	dbus_message_unref(msg);
	return OSSO_OK;
    }
}

/************************************************************************/
static DBusHandlerResult _top_handler(osso_context_t *osso,
                                      DBusMessage *msg, gpointer data);
static DBusHandlerResult _top_handler(osso_context_t *osso,
                                      DBusMessage *msg, gpointer data)
{
    struct _osso_top *top;
    gchar interface[MAX_IF_LEN] = {0};

    top = (struct _osso_top *)data;

    g_snprintf(interface, MAX_IF_LEN, OSSO_BUS_ROOT ".%s",
	       osso->application);

    if(dbus_message_is_method_call(msg, interface, OSSO_BUS_TOP))
    {	
        char *arguments = NULL;
        DBusMessageIter iter;

        if(!dbus_message_iter_init(msg, &iter)) {
            ULOG_ERR_F("Message has no arguments");
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&iter, &arguments);
        } else {
            arguments = "";
        }
        dprint("arguments = '%s'", arguments);

	top->handler(arguments, top->data);
	return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/************************************************************************/
osso_return_t osso_application_set_top_cb(osso_context_t *osso,
					  osso_application_top_cb_f *cb,
					  gpointer data)
{
    struct _osso_top *top;
    gchar interface[MAX_IF_LEN] = {0};
    if(osso == NULL) return OSSO_INVALID;
    if(cb == NULL) return OSSO_INVALID;
    
    top = (struct _osso_top *) calloc(1, sizeof(struct _osso_top));
    if (top == NULL) {
        ULOG_ERR_F("calloc failed");
        return OSSO_ERROR;
    }
    top->handler = cb;
    top->data = data;

    g_snprintf(interface, MAX_IF_LEN, OSSO_BUS_ROOT ".%s",
	       osso->application);

    /* register our top_application handler to the main message handler */
    _msg_handler_set_cb_f_free_data(osso, interface, _top_handler, top, TRUE);

    return OSSO_OK;
}

/************************************************************************/
osso_return_t osso_application_unset_top_cb(osso_context_t *osso,
					    osso_application_top_cb_f *cb,
					    gpointer data)
{
    struct _osso_top *top;
    gchar interface[MAX_IF_LEN] = {0};
    
    if(osso == NULL) return OSSO_INVALID;
    if(cb == NULL) return OSSO_INVALID;
    
    g_snprintf(interface, MAX_IF_LEN, OSSO_BUS_ROOT ".%s",
	       osso->application);
    
    top = (struct _osso_top *)_msg_handler_rm_cb_f(osso, interface,
						   _top_handler, TRUE);

    free(top);
    return OSSO_OK;
}
