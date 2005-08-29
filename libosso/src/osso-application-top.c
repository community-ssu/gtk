/**
 * @file osso-init.c
 * This file implements functionality related to the top_application
 * D-Bus message.
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
#include "log-functions.h"
#include "osso-application-top.h"
#include <stdlib.h>

static void _set_environment(DBusMessageIter *iter);
static void _append_environment(DBusMessage *msg);

osso_return_t osso_application_top(osso_context_t *osso, const gchar *application,
			  const gchar *arguments)
{
    char service[255], path[255], interface[255];
    guint serial;
    DBusMessage *msg;
    if(osso == NULL) return OSSO_INVALID;
    if(application == NULL) return OSSO_INVALID;

    dprint("Topping application (service) '%s' with args %s",
	   application, arguments);
    
    g_snprintf(service, 255, "%s.%s", OSSO_BUS_ROOT, application);

    g_snprintf(path, 255, "%s/%s", OSSO_BUS_ROOT_PATH, application);
	       

    g_snprintf(interface, 255, "%s", service);

    dprint("New method: %s%s::%s:%s",service,path,interface,OSSO_BUS_TOP);
    msg = dbus_message_new_method_call(service, path, interface,
				       OSSO_BUS_TOP);

    if(dbus_message_append_args(msg, DBUS_TYPE_STRING, 
				(arguments == NULL)?"":arguments,
                                DBUS_TYPE_INVALID) != TRUE) {
	dbus_message_unref(msg);
	return OSSO_ERROR;
    }
    
    _append_environment(msg);

    dbus_message_set_no_reply(msg, TRUE);
    dbus_message_set_auto_activation(msg, TRUE);

    if(dbus_connection_send(osso->conn, msg, &serial)==FALSE) {
	dbus_message_unref(msg);
	return OSSO_ERROR;
    }
    else {
	dbus_connection_flush(osso->conn);
	dbus_message_unref(msg);
	return OSSO_OK;
    }


}

/************************************************************************/
extern char **environ;

static void _append_environment(DBusMessage *msg)
{
    char **iter;
    iter = environ;

    for (;*iter;iter++)
    {
        dbus_message_append_args(msg,DBUS_TYPE_STRING,
                            *iter,DBUS_TYPE_INVALID);

        dprint("append %s",*iter);
    }
    dbus_message_append_args(msg,DBUS_TYPE_INT32,0,DBUS_TYPE_INVALID);
    
}

/************************************************************************/
static DBusHandlerResult _top_handler(osso_context_t *osso,
                                      DBusMessage *msg, gpointer data);
static DBusHandlerResult _top_handler(osso_context_t *osso,
                                      DBusMessage *msg, gpointer data)
{
    struct _osso_top *top;
    gchar interface[256] = {0};

    top = (struct _osso_top *)data;

    g_snprintf(interface, 255, "%s.%s", OSSO_BUS_ROOT,
	       osso->application);

    if(dbus_message_is_method_call(msg, interface, OSSO_BUS_TOP) == TRUE)
    {	
	char *arguments = NULL;
	DBusMessageIter iter;
	
	dbus_message_iter_init(msg, &iter);
	if(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
	    arguments = dbus_message_iter_get_string(&iter);
            if (!osso->environment_set)
            {
                _set_environment(&iter);
                osso->environment_set = TRUE;
            }
	}
	else {
	    arguments = "";
	}
dprint("arguments = '%s'",arguments);	
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
    gchar interface[256] = {0};
    if(osso == NULL) return OSSO_INVALID;
    if(cb == NULL) return OSSO_INVALID;
    
    top = (struct _osso_top *) malloc(sizeof(struct _osso_top));
    if(top == NULL) return OSSO_ERROR;
    top->handler = cb;
    top->data = data;

    g_snprintf(interface, 255, "%s.%s", OSSO_BUS_ROOT,
	       osso->application);

    /* register our top_application handler to the main message handler */
    _msg_handler_set_cb_f(osso, interface, _top_handler, top, TRUE);

    return OSSO_OK;
}

/************************************************************************/
osso_return_t osso_application_unset_top_cb(osso_context_t *osso,
					    osso_application_top_cb_f *cb,
					    gpointer data)
{
    struct _osso_top *top;
    gchar interface[256] = {0};
    
    if(osso == NULL) return OSSO_INVALID;
    if(cb == NULL) return OSSO_INVALID;
    
    g_snprintf(interface, 255, "%s.%s", OSSO_BUS_ROOT,
	       osso->application);
    
    top = (struct _osso_top *)_msg_handler_rm_cb_f(osso, interface,
						   _top_handler, TRUE);

    free(top);
    return OSSO_OK;
}

static void _set_environment(DBusMessageIter *iter)
{
    dbus_message_iter_next(iter);
    
    for (;dbus_message_iter_has_next(iter);
            dbus_message_iter_next(iter))
    {
        gchar **splitted;
        if (dbus_message_iter_get_arg_type(iter)
                != DBUS_TYPE_STRING)
            continue;
        
        d_log(LOG_D,"Osso got env: %s",
                    dbus_message_iter_get_string(iter));

        splitted = g_strsplit(dbus_message_iter_get_string(iter),
                            "=",2);
        if (splitted == NULL || splitted[0] == NULL || splitted[1]==NULL)
            continue;
        
        setenv(splitted[0],splitted[1],1);
        
        g_strfreev(splitted);
    
    }
    
}

