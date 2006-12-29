/**
 * @file osso-rpc.c
 * This file implements rpc calls from one application to another.
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

#define TASK_NAV_SERVICE "com.nokia.tasknav"
/* NOTICE: Keep these in sync with values in
 * hildon-navigator/windowmanager.c! */
#define APP_LAUNCH_BANNER_METHOD_INTERFACE \
            "com.nokia.tasknav.app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD_PATH \
            "/com/nokia/tasknav/app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD "app_launch_banner"


#include "osso-internal.h"
#include <assert.h>

static void _rpc_handler (osso_context_t * osso,
                          DBusMessage * msg,
                          _osso_callback_data_t *data,
                          muali_bus_type dbus_type);
static void _append_args(DBusMessage *msg, int type, va_list var_args);
static void _append_arg (DBusMessage * msg, osso_rpc_t * arg);
static void _get_arg (DBusMessageIter * iter, osso_rpc_t * retval);
static void _async_return_handler (DBusPendingCall * pending, void *data);
static osso_return_t _rpc_run_with_argfill (osso_context_t *osso,
					    DBusConnection *conn,
					    const gchar *service,
					    const gchar *object_path,
					    const gchar *interface,
					    const gchar *method,
					    osso_rpc_t *retval, 
					    osso_rpc_argfill *argfill,
					    void *argfill_data);

typedef struct {
    osso_rpc_async_f *func;
    gpointer data;
    gchar *interface;
    gchar *method;
}_osso_rpc_async_t;


void osso_rpc_free_val (osso_rpc_t *rpc)
{
  if (rpc->type == DBUS_TYPE_STRING)
	  g_free (rpc->value.s);
  rpc->type = DBUS_TYPE_INVALID;
}

/************************************************************************/
typedef struct {
  int argument_type;
  va_list arg_list;
} fill_from_va_list_data;

static void
fill_from_va_list (DBusMessage *msg, void *raw)
{
  fill_from_va_list_data *data = (fill_from_va_list_data *)raw;

  if (data->argument_type != DBUS_TYPE_INVALID) {
    _append_args(msg, data->argument_type, data->arg_list);
  }
}

osso_return_t osso_rpc_run(osso_context_t *osso, const gchar *service,
			   const gchar *object_path,
			   const gchar *interface,
			   const gchar *method, osso_rpc_t *retval,
			   int argument_type, ...)
{
    osso_return_t ret;
    fill_from_va_list_data data;
    
    if( (osso == NULL) || (service == NULL) || (object_path == NULL) ||
	(interface == NULL) || (method == NULL) || (osso->conn == NULL))
	return OSSO_INVALID;

    data.argument_type = argument_type;
    va_start(data.arg_list, argument_type);
    
    ret = _rpc_run_with_argfill (osso, osso->conn,
				 service, object_path, interface, method,
				 retval,
				 fill_from_va_list, &data);

    va_end(data.arg_list);
    return ret;
}

/************************************************************************/
osso_return_t osso_rpc_run_with_argfill (osso_context_t *osso,
					 const gchar *service,
					 const gchar *object_path,
					 const gchar *interface,
					 const gchar *method,
					 osso_rpc_t *retval,
					 osso_rpc_argfill *argfill,
					 void *argfill_data)
{
    if( (osso == NULL) || (service == NULL) || (object_path == NULL) ||
	(interface == NULL) || (method == NULL) || (osso->conn == NULL))
	return OSSO_INVALID;

    return _rpc_run_with_argfill (osso, osso->conn,
				  service, object_path, interface, method, 
				  retval,
				  argfill, argfill_data);
}

/************************************************************************/
osso_return_t osso_rpc_run_system(osso_context_t *osso, const gchar *service,
			   const gchar *object_path,
			   const gchar *interface,
			   const gchar *method, osso_rpc_t *retval,
			   int argument_type, ...)
{
    osso_return_t ret;
    fill_from_va_list_data data;
    
    if( (osso == NULL) || (service == NULL) || (object_path == NULL) ||
	(interface == NULL) || (method == NULL) || (osso->sys_conn == NULL))
	return OSSO_INVALID;

    data.argument_type = argument_type;
    va_start(data.arg_list, argument_type);
    
    ret = _rpc_run_with_argfill (osso, osso->sys_conn,
				 service, object_path, interface, method,
				 retval,
				 fill_from_va_list, &data);

    va_end(data.arg_list);
    return ret;
}

/************************************************************************/
osso_return_t osso_rpc_run_system_with_argfill (osso_context_t *osso,
						const gchar *service,
						const gchar *object_path,
						const gchar *interface,
						const gchar *method,
						osso_rpc_t *retval,
						osso_rpc_argfill *argfill,
						void *argfill_data)
{
    if( (osso == NULL) || (service == NULL) || (object_path == NULL) ||
	(interface == NULL) || (method == NULL) || (osso->sys_conn == NULL))
	return OSSO_INVALID;

    return _rpc_run_with_argfill (osso, osso->sys_conn,
				  service, object_path, interface, method, 
				  retval,
				  argfill, argfill_data);
}

/************************************************************************/
static osso_return_t
_rpc_run_with_argfill (osso_context_t *osso, DBusConnection *conn,
		       const gchar *service, const gchar *object_path,
		       const gchar *interface, const gchar *method,
		       osso_rpc_t *retval, 
		       osso_rpc_argfill *argfill,
		       void *argfill_data)
{
    DBusMessage *msg;
    dbus_bool_t b;
    
    dprint("");
    if (conn == NULL) {
        ULOG_ERR_F("invalid arguments");
	return OSSO_INVALID;
    }
    
    dprint("New method: %s:%s:%s:%s",service,object_path,interface,method);
    msg = dbus_message_new_method_call(service, object_path,
				       interface, method);
    if (msg == NULL) {
        ULOG_ERR_F("dbus_message_new_method_call failed");
	return OSSO_ERROR;
    }

    argfill (msg, argfill_data);

    dbus_message_set_auto_start(msg, TRUE);
    
    if (retval == NULL) {
	dbus_message_set_no_reply(msg, TRUE);
	b = dbus_connection_send(conn, msg, NULL);
        dbus_message_unref(msg);
        if (b) {

	    /* Tell TaskNavigator to show "launch banner" */
	    msg = dbus_message_new_method_call(TASK_NAV_SERVICE,
			    APP_LAUNCH_BANNER_METHOD_PATH,
			    APP_LAUNCH_BANNER_METHOD_INTERFACE,
			    APP_LAUNCH_BANNER_METHOD );
   
	    if (msg != NULL) {
                dbus_message_append_args(msg, DBUS_TYPE_STRING, &service,
                                         DBUS_TYPE_INVALID);

                b = dbus_connection_send(conn, msg, NULL);
                if (!b) {
                    ULOG_WARN_F("dbus_connection_send failed");
                }
                dbus_message_unref(msg);
	    }
            else {
                ULOG_WARN_F("dbus_message_new_method_call failed");
            }
            return OSSO_OK;
        }
        else {
            ULOG_ERR_F("dbus_connection_send failed");
            return OSSO_ERROR;
        }
    }
    else {
        DBusError err;
        DBusMessage *reply = NULL;
        dbus_error_init(&err);
	reply = dbus_connection_send_with_reply_and_block(conn, msg, 
							osso->rpc_timeout,
							&err);
        dbus_message_unref(msg);

        if (reply == NULL) {
            ULOG_ERR_F("dbus_connection_send_with_reply_and_block error: %s",
                       err.message);
            retval->type = DBUS_TYPE_STRING;
            retval->value.s = g_strdup(err.message);
            dbus_error_free(&err);
            return OSSO_RPC_ERROR;
        }
        else {
	    DBusMessageIter iter;

	    switch (dbus_message_get_type(reply)) {
	      case DBUS_MESSAGE_TYPE_ERROR:
		dbus_set_error_from_message(&err, reply);
		retval->type = DBUS_TYPE_STRING;
		retval->value.s = g_strdup(err.message);
		dbus_error_free(&err);
		dbus_message_unref(reply);
		return OSSO_RPC_ERROR;
	      case DBUS_MESSAGE_TYPE_METHOD_RETURN:
		dprint("message return");
		dbus_message_iter_init(reply, &iter);
	    
		_get_arg(&iter, retval);
	    
		dbus_message_unref(reply);
	
		/* Tell TaskNavigator to show "launch banner" */
		msg = dbus_message_new_method_call(TASK_NAV_SERVICE,
				APP_LAUNCH_BANNER_METHOD_PATH,
				APP_LAUNCH_BANNER_METHOD_INTERFACE,
				APP_LAUNCH_BANNER_METHOD );

		if (msg != NULL) {
                    dbus_message_append_args(msg, DBUS_TYPE_STRING,
                                             &service, DBUS_TYPE_INVALID);

                    b = dbus_connection_send(conn, msg, NULL);

                    if (!b) {
                        ULOG_WARN_F("dbus_connection_send failed");
                    }
                    dbus_message_unref(msg);
		}
                else {
                    ULOG_WARN_F("dbus_message_new_method_call failed");
                }
                return OSSO_OK;
	      default:
                ULOG_WARN_F("got unknown message type as reply");
		retval->type = DBUS_TYPE_STRING;
		retval->value.s = g_strdup("Invalid return value");
		dbus_message_unref(reply);
		return OSSO_RPC_ERROR;
	    }
        }
    }
}

/************************************************************************/

osso_return_t osso_rpc_run_with_defaults(osso_context_t *osso,
					 const gchar *application,
					 const gchar *method, 
					 osso_rpc_t *retval,
					 int argument_type,
					 ...)
{
    char service[MAX_SVC_LEN], path[MAX_OP_LEN], interface[MAX_IF_LEN];
    fill_from_va_list_data data;
    osso_return_t ret;

    if( (osso == NULL) || (application == NULL) || (method == NULL)
        || (osso->conn == NULL) )
	return OSSO_INVALID;

    make_default_service(application, service);
    make_default_object_path(application, path);
    make_default_interface(application, interface);
    
    data.argument_type = argument_type;
    va_start(data.arg_list, argument_type);

    ret = _rpc_run_with_argfill (osso, osso->conn, service,
                                 path, interface,
                                 method, retval,
				 fill_from_va_list, &data);
    
    va_end(data.arg_list);

    return ret;
}

static void free_osso_rpc_async_t(_osso_rpc_async_t *rpc)
{
    if (rpc != NULL) {
        if (rpc->interface != NULL) {
            g_free(rpc->interface);
            rpc->interface = NULL;
        }
        if (rpc->method != NULL) {
            g_free(rpc->method);
            rpc->method = NULL;
        }
        free(rpc); /* free() because calloc() was used */
    }
}

/************************************************************************/
osso_return_t osso_rpc_async_run_with_argfill (osso_context_t *osso,
						  const gchar *service,
						  const gchar *object_path,
						  const gchar *interface,
						  const gchar *method,
						  osso_rpc_async_f *async_cb,
						  gpointer data,
						  osso_rpc_argfill *argfill,
						  void *argfill_data)
{
    DBusMessage *msg = NULL;
    DBusPendingCall *pending = NULL;
    dbus_bool_t succ = FALSE;
    _osso_rpc_async_t *rpc = NULL;
   
    if (osso == NULL || service == NULL || object_path == NULL ||
        interface == NULL || method == NULL) {
        ULOG_ERR_F("invalid arguments");
        return OSSO_INVALID;
    }

    dprint("New method: %s:%s:%s:%s",service,object_path,interface,method);
    msg = dbus_message_new_method_call(service, object_path,
				       interface, method);
    if (msg == NULL) {
        ULOG_ERR_F("dbus_message_new_method_call failed");
        return OSSO_ERROR;
    }

    dbus_message_set_auto_start(msg, TRUE);
    
    argfill (msg, argfill_data);

    if (async_cb == NULL) {
	dprint("no reply wanted");
	dbus_message_set_no_reply(msg, TRUE);
	succ = dbus_connection_send(osso->conn, msg, NULL);
    }
    else {
	dprint("caller wants reply");
	rpc = (_osso_rpc_async_t *)calloc(1, sizeof(_osso_rpc_async_t));
	if (rpc == NULL) {
            ULOG_ERR_F("calloc failed");
            return OSSO_ERROR;
        }
	
	rpc->func = async_cb;
	rpc->data = data;
	rpc->interface = g_strdup(interface);
	rpc->method = g_strdup(method);
	dprint("rpc = %p",rpc);
	dprint("rpc->func = %p",rpc->func);
	dprint("rpc->func = %p",rpc->data);
	dprint("rpc->interface = '%s'",rpc->interface);
	dprint("rpc->method = '%s'",rpc->method);

	succ = dbus_connection_send_with_reply(osso->conn, msg, 
					    &pending, osso->rpc_timeout);
    }

    if (succ) {
	dprint("succ is true");
	if (async_cb != NULL) {
	    dbus_pending_call_set_notify(pending,
					 _async_return_handler,
					 rpc, NULL);
	}
        else if (rpc != NULL) {
            free_osso_rpc_async_t(rpc);
            rpc = NULL;
        }
	dbus_message_unref(msg);

	/* Tell TaskNavigator to show "launch banner" */
	msg = dbus_message_new_method_call(TASK_NAV_SERVICE,
                        APP_LAUNCH_BANNER_METHOD_PATH,
                        APP_LAUNCH_BANNER_METHOD_INTERFACE,
                        APP_LAUNCH_BANNER_METHOD );

	if (msg != NULL) {
            dbus_message_append_args(msg, DBUS_TYPE_STRING, &service,
                                     DBUS_TYPE_INVALID);

            succ = dbus_connection_send(osso->conn, msg, NULL);

            if (!succ) {
                ULOG_WARN_F("dbus_connection_send failed");
            }
            dbus_message_unref(msg);
	}
        else {
            ULOG_WARN_F("dbus_message_new_method_call failed");
        }

	return OSSO_OK;
    }
    else {
        ULOG_ERR_F("dbus_connection_send(_with_reply) failed");
        dbus_message_unref(msg);
        if (rpc != NULL) {
            free_osso_rpc_async_t(rpc);
            rpc = NULL;
        }
        return OSSO_ERROR;
    }
}

/************************************************************************/
osso_return_t osso_rpc_async_run(osso_context_t *osso,
				 const gchar *service,
				 const gchar *object_path,
				 const gchar *interface,
				 const gchar *method,
				 osso_rpc_async_f *async_cb,
				 gpointer data,
				 int argument_type, ...)
{
    fill_from_va_list_data argfill_data;
    osso_return_t ret;
   
	
    if( (osso == NULL) || (service == NULL) || (object_path == NULL) ||
	(interface == NULL) || (method == NULL))
	return OSSO_INVALID;

    argfill_data.argument_type = argument_type;
    va_start(argfill_data.arg_list, argument_type);

    ret = osso_rpc_async_run_with_argfill (osso,
				       service, object_path, interface, method,
				       async_cb, data,
				       fill_from_va_list, &argfill_data);
    dprint("osso_rpc_async_run_with_argfill returned");
    
    va_end(argfill_data.arg_list);
    
    return ret;
}

/************************************************************************/
osso_return_t osso_rpc_async_run_with_defaults(osso_context_t *osso,
					       const gchar *application,
					       const gchar *method,
					       osso_rpc_async_f *async_cb,
					       gpointer data,
					       int argument_type, ...)
{
    char service[MAX_SVC_LEN], path[MAX_OP_LEN], interface[MAX_IF_LEN];
    fill_from_va_list_data argfill_data;
    osso_return_t ret;

    if( (osso == NULL) || (application == NULL) || (method == NULL) )
	return OSSO_INVALID;

    make_default_service(application, service);
    make_default_object_path(application, path);
    make_default_interface(application, interface);
    
    argfill_data.argument_type = argument_type;
    va_start(argfill_data.arg_list, argument_type);

    ret = osso_rpc_async_run_with_argfill (osso, service, path,
                                           interface, method,
				           async_cb, data,
				           fill_from_va_list, &argfill_data);
    dprint("osso_rpc_async_run_with_argfill returned");
    
    va_end(argfill_data.arg_list);

    return ret;
}

/************************************************************************/
static osso_return_t _rpc_set_cb_f(osso_context_t *osso,
                                   const gchar *service,
                                   const gchar *object_path,
                                   const gchar *interface,
                                   osso_rpc_cb_f *cb, gpointer data,
                                   osso_rpc_retval_free_f *retval_free,
                                   gboolean use_system_bus)
{
    _osso_callback_data_t *rpc;
    int ret;

    ULOG_DEBUG_F("s '%s' o '%s' i '%s', %s bus",
                 service, object_path, interface,
                 use_system_bus ? "system" : "session");
    
    rpc = calloc(1, sizeof(_osso_callback_data_t));
    if (rpc == NULL) {
        ULOG_ERR_F("calloc failed");
        return OSSO_ERROR;
    }
    
    if (strcmp(service, osso->service) != 0
        || (use_system_bus && !osso->systembus_service_registered)
        || (!use_system_bus && !osso->sessionbus_service_registered)) {
        DBusError err;

        dbus_error_init(&err);

        ULOG_DEBUG_F("requesting service '%s'", service);
        ret = dbus_bus_request_name(use_system_bus ? osso->sys_conn :
                  osso->conn, service, DBUS_NAME_FLAG_ALLOW_REPLACEMENT |
                  DBUS_NAME_FLAG_REPLACE_EXISTING |
                  DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
        if (ret == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER ||
            ret == DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER) {
            /* success */ 
        } else if (ret == DBUS_REQUEST_NAME_REPLY_IN_QUEUE ||
                   ret == DBUS_REQUEST_NAME_REPLY_EXISTS) {
            /* this should be impossible */
            ULOG_ERR_F("dbus_bus_request_name is broken");
            free(rpc);
            return OSSO_ERROR;
        } else if (ret == -1) {
            ULOG_ERR_F("dbus_bus_request_name for '%s' failed: %s",
                       service, err.message);
            dbus_error_free(&err);
            free(rpc);
            return OSSO_ERROR;
        }

        if (strcmp(service, osso->service) == 0) {
            if (use_system_bus) {
                osso->systembus_service_registered = TRUE;
            } else {
                osso->sessionbus_service_registered = TRUE;
            }
        }
    }

    rpc->user_cb = cb;
    rpc->user_data = data;
    rpc->data = retval_free;

    _msg_handler_set_cb_f(osso,
                          service,
                          object_path,
                          interface,
                          _rpc_handler,
                          rpc, TRUE);
    return OSSO_OK;
}

/************************************************************************/
osso_return_t _test_rpc_set_cb_f(osso_context_t *osso, const gchar *service,
                                 const gchar *object_path,
                                 const gchar *interface,
                                 osso_rpc_cb_f *cb, gpointer data,
                                 gboolean use_system_bus)
{
    return _rpc_set_cb_f(osso, service, object_path, interface,
                         cb, data, NULL, use_system_bus);
}

/************************************************************************/
osso_return_t osso_rpc_set_cb_f_with_free (osso_context_t *osso,
                                           const gchar *service,
                                           const gchar *object_path,
                                           const gchar *interface,
                                           osso_rpc_cb_f *cb, gpointer data,
                                           osso_rpc_retval_free_f *retval_free)
{
    if( (osso == NULL) || (service == NULL) || (object_path == NULL) ||
	(interface == NULL) || (cb == NULL))
	return OSSO_INVALID;
    return _rpc_set_cb_f(osso, service, object_path,
			 interface,cb, data, retval_free, FALSE);
}

osso_return_t osso_rpc_set_cb_f (osso_context_t *osso,
                                 const gchar *service,
                                 const gchar *object_path,
                                 const gchar *interface,
                                 osso_rpc_cb_f *cb, gpointer data)
{
    return osso_rpc_set_cb_f_with_free (osso, service, object_path,
                                        interface, cb, data, NULL);
}

/************************************************************************/
osso_return_t osso_rpc_set_default_cb_f_with_free (osso_context_t *osso,
						   osso_rpc_cb_f *cb,
						   gpointer data,
						   osso_rpc_retval_free_f *retval_free)
{
    _osso_callback_data_t *rpc;
    
    if (osso == NULL || cb == NULL)
	return OSSO_INVALID;

    rpc = calloc(1, sizeof(_osso_callback_data_t));
    if (rpc == NULL) {
        ULOG_ERR_F("calloc failed");
	return OSSO_ERROR;
    }
    
    rpc->user_cb = cb;
    rpc->user_data = data;
    rpc->data = retval_free;
    
    _msg_handler_set_cb_f_free_data(osso,
                                    osso->service,
                                    osso->object_path,
                                    osso->interface,
                                    _rpc_handler,
                                    rpc, TRUE);
    return OSSO_OK;
}

osso_return_t osso_rpc_set_default_cb_f (osso_context_t *osso,
					 osso_rpc_cb_f *cb,
					 gpointer data)
{
    return osso_rpc_set_default_cb_f_with_free (osso, cb, data, NULL);
}

/************************************************************************/
osso_return_t osso_rpc_unset_cb_f(osso_context_t *osso,
				  const gchar *service,
				  const gchar *object_path,
				  const gchar *interface,
				  osso_rpc_cb_f *cb, gpointer data)
{
    _osso_callback_data_t user_data;

    if (osso == NULL || service == NULL || object_path == NULL
	|| interface == NULL || cb == NULL || osso->conn == NULL) {
        ULOG_ERR_F("invalid parameters");
	return OSSO_INVALID;
    }

    user_data.user_cb = cb;
    user_data.user_data = data;
    user_data.data = NULL;

    _msg_handler_rm_cb_f(osso, service, object_path, interface,
                         (const _osso_handler_f*)_rpc_handler,
                         &user_data, TRUE);

    return OSSO_OK;
}

/************************************************************************/

osso_return_t osso_rpc_unset_default_cb_f(osso_context_t *osso,
					  osso_rpc_cb_f *cb, gpointer data)
{
    _osso_callback_data_t user_data;

    if (osso == NULL || cb == NULL) {
        ULOG_ERR_F("invalid parameters");
        return OSSO_INVALID;
    }

    user_data.user_cb = cb;
    user_data.user_data = data;
    user_data.data = NULL;
    
    _msg_handler_rm_cb_f(osso, osso->service, osso->object_path,
                         osso->interface,
                         (const _osso_handler_f*)_rpc_handler,
                         &user_data, TRUE);

    return OSSO_OK;
}


/************************************************************************/
osso_return_t osso_rpc_get_timeout(osso_context_t *osso, gint *timeout)
{
    if(osso == NULL) return OSSO_INVALID;
    *timeout = osso->rpc_timeout;
    return OSSO_OK;
}

/************************************************************************/
osso_return_t osso_rpc_set_timeout(osso_context_t *osso, gint timeout)
{
    if(osso == NULL) return OSSO_INVALID;
    osso->rpc_timeout = timeout;
    return OSSO_OK;
}

/************************************************************************/

static void _rpc_handler(osso_context_t *osso, DBusMessage *msg,
                         _osso_callback_data_t *rpc,
                         muali_bus_type dbus_type)
{
    DBusMessageIter iter;
    GArray *arguments;
    osso_rpc_t retval;
    gint ret;
    int i;
    osso_rpc_cb_f *handler;
    osso_rpc_retval_free_f *retval_free;

    arguments = g_array_new(FALSE, FALSE, sizeof(osso_rpc_t));

    if (dbus_message_iter_init(msg, &iter)) {
        while(TRUE) {
	    osso_rpc_t arg;

	    _get_arg(&iter, &arg);
	
	    dprint("appending value");
	    g_array_append_val(arguments, arg);
	
	    if(dbus_message_iter_has_next(&iter))
	         dbus_message_iter_next(&iter);
	    else
	         break;
        }
    }

    handler = rpc->user_cb;
    retval_free = rpc->data;

    dprint("calling handler at %p", handler);
    retval.type = DBUS_TYPE_INVALID;
    ret = (*handler)(dbus_message_get_interface(msg),
        	     dbus_message_get_member(msg), arguments,
                     rpc->user_data, &retval);
    if (retval.type == DBUS_TYPE_STRING) {
       dprint("handler returned string '%s'", retval.value.s);
    }

    for (i = 0; i < arguments->len; i++)
        osso_rpc_free_val (&g_array_index (arguments, osso_rpc_t, i));
    g_array_free(arguments, TRUE);

    if(ret == OSSO_INVALID) {
	if (retval_free != NULL)
	    (*retval_free)(&retval);
	return;
    }
    
    if(!dbus_message_get_no_reply(msg)) {
	DBusMessage *reply = NULL;
	if(ret == OSSO_OK) {
	    dprint("Callback returnded OSSO_OK");
	    dprint("callback set retval.type to %d",retval.type);
	    reply = dbus_message_new_method_return(msg);
	    if(reply != NULL) {
                if (retval.type == DBUS_TYPE_STRING) {
                    dprint("string is still '%s'", retval.value.s);
                }
		_append_arg(reply, &retval);
	    }
	}
	else {
	    gchar err_name[256];
	    g_snprintf(err_name, 255, "%s.error.%s",
		       dbus_message_get_destination(msg),
		       dbus_message_get_member(msg));
	    reply = dbus_message_new_error(msg, err_name, retval.value.s);
	}
	if(reply != NULL) {
	    dbus_uint32_t serial;
	    dprint("sending message to '%s'",
		   dbus_message_get_destination(reply));
	    
	    dbus_connection_send(osso->cur_conn, reply, &serial);
	    dbus_connection_flush(osso->cur_conn);
	    dbus_message_unref(reply);
	}
    }
    if (retval_free)
        (*retval_free)(&retval);
}

/************************************************************************/
static void _append_args(DBusMessage *msg, int type, va_list var_args)
{
    dprint("");
    while(type != DBUS_TYPE_INVALID) {
	gchar *s;
	const char *empty_str = "";
	gint32 i;
	guint32 u;
	gdouble d;
	gboolean b;
	
	switch(type) {
	  case DBUS_TYPE_INT32:
	    i = va_arg(var_args, gint32);
	    dprint("appending DBUS_TYPE_INT32 %d", i);
	    dbus_message_append_args(msg, type, &i, DBUS_TYPE_INVALID);
	    break;
	  case DBUS_TYPE_UINT32:
	    u = va_arg(var_args, guint32);
	    dbus_message_append_args(msg, type, &u, DBUS_TYPE_INVALID);
	    break;
	  case DBUS_TYPE_BOOLEAN:
	    b = va_arg(var_args, gboolean);
	    dbus_message_append_args(msg, type, &b, DBUS_TYPE_INVALID);
	    break;
	  case DBUS_TYPE_DOUBLE:
	    d = va_arg(var_args, gdouble);
	    dbus_message_append_args(msg, type, &d, DBUS_TYPE_INVALID);
	    break;
	  case DBUS_TYPE_STRING:
	    dprint("DBUS_TYPE_STRING");
	    s = va_arg(var_args, gchar *);
	    if(s == NULL)
		dbus_message_append_args(msg, type, &empty_str,
					 DBUS_TYPE_INVALID);
	    else
		dbus_message_append_args(msg, type, &s, DBUS_TYPE_INVALID);
	    break;
	  default:
	    break;	    
	}
	
	type = va_arg(var_args, int);
    }
    return;
}

/************************************************************************/

static void _append_arg(DBusMessage *msg, osso_rpc_t *arg)
{
    const char* empty_str = "";
    if(arg == NULL) return;
    switch(arg->type) {
      case DBUS_TYPE_INVALID:
	break;
      case DBUS_TYPE_INT32:
	dprint("Appending INT32:%d",arg->value.i);
	dbus_message_append_args(msg, DBUS_TYPE_INT32, &arg->value.i,
				 DBUS_TYPE_INVALID);
	break;
      case DBUS_TYPE_UINT32:
	dprint("Appending UINT32:%d",arg->value.u);
	dbus_message_append_args(msg, DBUS_TYPE_UINT32, &arg->value.u,
				 DBUS_TYPE_INVALID);
	break;
      case DBUS_TYPE_BOOLEAN:
	dprint("Appending BOOL:%s",arg->value.b?"TRUE":"FALSE");
	dbus_message_append_args(msg, DBUS_TYPE_BOOLEAN, &arg->value.b,
				 DBUS_TYPE_INVALID);
	break;
      case DBUS_TYPE_DOUBLE:
	dprint("Appending DOUBLE:%f",arg->value.d);
	dbus_message_append_args(msg, DBUS_TYPE_DOUBLE, &arg->value.d,
				 DBUS_TYPE_INVALID);
	break;
      case DBUS_TYPE_STRING:
	dprint("Appending STR:'%s'",arg->value.s);
	if(arg->value.s == NULL)
	    dbus_message_append_args(msg, DBUS_TYPE_STRING, &empty_str,
                                     DBUS_TYPE_INVALID);
	else
	    dbus_message_append_args(msg, DBUS_TYPE_STRING, &arg->value.s,
				     DBUS_TYPE_INVALID);
	break;
      default:
	break;	    
    }
}

/************************************************************************/

static void _get_arg(DBusMessageIter *iter, osso_rpc_t *retval)
{
    char *str;

    dprint("");

    retval->type = dbus_message_iter_get_arg_type(iter);
    dprint("before switch");
    switch(retval->type) {
      case DBUS_TYPE_INT32:
        dprint("DBUS_TYPE_INT32");
	dbus_message_iter_get_basic (iter, &retval->value.i);
	dprint("got INT32:%d",retval->value.i);
	break;
      case DBUS_TYPE_UINT32:
	dbus_message_iter_get_basic (iter, &retval->value.u);
	dprint("got UINT32:%u",retval->value.u);
	break;
      case DBUS_TYPE_BOOLEAN:
	dbus_message_iter_get_basic (iter, &retval->value.b);
	dprint("got BOOLEAN:%s",retval->value.s?"TRUE":"FALSE");
	break;
      case DBUS_TYPE_DOUBLE:
	dbus_message_iter_get_basic (iter, &retval->value.d);
	dprint("got DOUBLE:%f",retval->value.d);
	break;
      case DBUS_TYPE_STRING:
        dprint("DBUS_TYPE_STRING");
	dbus_message_iter_get_basic (iter, &str);
	retval->value.s = g_strdup (str);
	if(retval->value.s == NULL) {
	    retval->type = DBUS_TYPE_INVALID;
	    retval->value.i = 0;	    
	}
	dprint("got STRING:'%s'",retval->value.s);
	break;
      default:
	retval->type = DBUS_TYPE_INVALID;
	retval->value.i = 0;	    
	dprint("got unknown type:'%c'(%d)",retval->type,retval->type);
	break;	
    }
}

/***********************************************************************/
static void _async_return_handler(DBusPendingCall *pending, void *data)
{
    DBusMessage *msg;
    _osso_rpc_async_t *rpc;
    osso_rpc_t retval;
    int type;
    rpc = (_osso_rpc_async_t *)data;
    ULOG_INFO_F("At msg return handler");
    msg = dbus_pending_call_steal_reply(pending);
    if(msg == NULL) {
        free_osso_rpc_async_t(rpc);
	return;
    }
    type = dbus_message_get_type(msg);
    if(type == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
	DBusMessageIter iter;

	dprint("message return");
	dbus_message_iter_init(msg, &iter);

	_get_arg(&iter, &retval);
	dprint("message return");
	(rpc->func)(rpc->interface, rpc->method, &retval,
			  rpc->data);
	dprint("done calling cb");
	osso_rpc_free_val(&retval);
    }
    else if(type == DBUS_MESSAGE_TYPE_ERROR) {
	DBusError err;
	dbus_error_init(&err);
	dbus_set_error_from_message(&err, msg);
        dprint("Error return: %s", err.message);
	retval.type = DBUS_TYPE_STRING;
	retval.value.s = g_strdup(err.message);
	dbus_error_free(&err);
	dprint("calling cb with the error string");
	(rpc->func)(rpc->interface, rpc->method, &retval,
			  rpc->data);
	dprint("done calling cb");
	osso_rpc_free_val(&retval);
    }

    dbus_message_unref(msg);
    dprint("rpc = %p",rpc);
    dprint("rpc->func = %p",rpc->func);
    dprint("rpc->func = %p",rpc->data);
    dprint("rpc->interface = '%s'",rpc->interface);
    dprint("rpc->method = '%s'",rpc->method);
    free_osso_rpc_async_t(rpc);
    return;
}


/******************************************************
 * NEW API DEVELOPMENT - THESE ARE SUBJECT TO CHANGE!
 * muali = maemo user application library
 ******************************************************/


static void muali_reply_handler(osso_context_t *osso,
                                DBusMessage *msg,
                                _osso_callback_data_t *cb_data,
                                muali_bus_type dbus_type)
{
        muali_event_info_t info;
        DBusMessageIter iter;
        muali_handler_t *cb;
        int msgtype;

        ULOG_DEBUG_F("entered");

        assert(cb_data->message_id == dbus_message_get_reply_serial(msg));
        assert(cb_data->bus_type == dbus_type);
        assert(cb_data->data == osso);

        /* create a dummy reply message for later use (workaround for
         * D-Bus library API) */
        if (osso->reply_dummy == NULL) {
                DBusMessage *reply;
                reply = dbus_message_new_method_return(msg);
                if (reply == NULL) {
                        ULOG_WARN_F("could not create reply_dummy");
                } else {
                        osso->reply_dummy = reply;
                }
        }

        memset(&info, 0, sizeof(info));

        info.service = dbus_message_get_sender(msg);
        info.path = dbus_message_get_path(msg);
        info.interface = dbus_message_get_interface(msg);
        info.name = dbus_message_get_member(msg);
        info.bus_type = dbus_type;
        info.message_id = dbus_message_get_serial(msg);

        msgtype = dbus_message_get_type(msg);
        info.event_type = muali_convert_msgtype(msgtype);

        if (msgtype == DBUS_MESSAGE_TYPE_ERROR) {
                info.error = dbus_message_get_error_name(msg);
        }

        if (dbus_message_iter_init(msg, &iter)) {
                info.args = _get_muali_args(&iter);
        }

        cb = cb_data->user_cb;
        assert(cb != NULL);

        (*cb)((muali_context_t*)osso, &info, cb_data->user_data);

        if (info.args != NULL) {
                free(info.args);
                info.args = NULL;
        }
}

muali_error_t muali_send_string(muali_context_t *context,
                                muali_handler_t *reply_handler,
                                const void *user_data,
                                muali_bus_type bus_type,
                                const char *destination,
                                const char *message_name,
                                const char *string_to_send,
                                long *message_id)
{
        char service[MAX_SVC_LEN + 1], path[MAX_OP_LEN + 1],
             interface[MAX_IF_LEN + 1];
        _osso_callback_data_t *cb_data;
        DBusMessage *msg;
        DBusConnection *conn;
        int msg_serial = 0, handler_id;

        if (context == NULL) {
                return MUALI_ERROR_INVALID;
        }

        if (destination == NULL || message_name == NULL) {
                ULOG_ERR_F("destination and message_name must be provided");
                return MUALI_ERROR_INVALID;
        }

        if (bus_type != MUALI_BUS_SESSION && bus_type != MUALI_BUS_SYSTEM) {
                ULOG_ERR_F("bus_type must be either MUALI_BUS_SESSION "
                           "or MUALI_BUS_SYSTEM");
                return MUALI_ERROR_INVALID;
        }

        if (reply_handler == NULL && (user_data != NULL
                                      || message_id != NULL)) {
                ULOG_WARN_F("reply_handler == NULL: ignoring user_data and "
                            "message_id arguments");
                fprintf(stderr, "Warning: reply_handler is NULL: ignoring"
                        " user_data and message_id arguments");
        }

        if (bus_type == MUALI_BUS_SESSION) {
                conn = context->conn;
        } else {
                conn = context->sys_conn;
        }

        if (!dbus_connection_get_is_connected(conn)) {
                ULOG_ERR_F("connection %p is not open", conn);
                return MUALI_ERROR_INVALID;
        }

        make_default_service(destination, service);
        make_default_object_path(destination, path);
        make_default_interface(destination, interface);

        msg = dbus_message_new_method_call(service, path, interface,
                                           message_name);
        if (msg == NULL) {
                ULOG_ERR_F("dbus_message_new_method_call failed");
	        goto _muali_send_oom;
        }

        dbus_message_append_args(msg, DBUS_TYPE_STRING, &string_to_send,
                                 DBUS_TYPE_INVALID);

        if (reply_handler == NULL) {
                dbus_message_set_no_reply(msg, TRUE);
        }

        if (!dbus_connection_send(conn, msg, &msg_serial)) {
                ULOG_ERR_F("dbus_connection_send failed");
                dbus_message_unref(msg);
	        goto _muali_send_oom;
        }
        dbus_message_unref(msg);

        if (reply_handler == NULL) {
                /* the caller does not want to handle the reply, so
                 * let's not put any handler for it */
                return MUALI_ERROR_SUCCESS;
        }

        cb_data = calloc(1, sizeof(_osso_callback_data_t));
        if (cb_data == NULL) {
                ULOG_ERR_F("calloc failed");
                goto _muali_send_oom;
        }
        /* this is used to recognise the reply for this message */
        cb_data->message_id = (long)msg_serial;

        cb_data->user_cb = reply_handler;
        cb_data->user_data = user_data;
        cb_data->match_rule = NULL;
        cb_data->event_type = 0;
        cb_data->bus_type = bus_type;
        cb_data->data = context;

        cb_data->service = NULL;
        cb_data->path = MUALI_PATH_MATCH_ALL;
        cb_data->interface = NULL;
        cb_data->name = MUALI_MEMBER_MATCH_ALL;

        /* (muali_filter removes the handler, otherwise this would need
         * to be saved somewhere) */
        handler_id = context->next_handler_id++;

        if (_muali_set_handler((_muali_context_t*)context,
                               muali_reply_handler, cb_data, handler_id,
                               FALSE)) {

                if (message_id != NULL) {
                        /* the caller can use message_id to track
                         * the reply */
                        *message_id = (long)msg_serial;
                }

                return MUALI_ERROR_SUCCESS;
        } else {
                ULOG_ERR_F("_muali_set_handler failed");
                free(cb_data);
                return MUALI_ERROR;
        }

_muali_send_oom:

        return MUALI_ERROR_OOM;
}

muali_error_t muali_reply_string(muali_context_t *context,
                                 muali_bus_type bus_type,
                                 const char *dest,
                                 long message_id,
                                 const char *string)
{
        DBusConnection *conn;
        DBusMessage *reply;
        dbus_bool_t ret;

        if (context == NULL || string == NULL || dest == NULL) {
                return MUALI_ERROR_INVALID;
        }
        if (context->reply_dummy == NULL) {
                /* OOM has happened earlier in the handler, or
                 * the context is invalid */
                ULOG_ERR_F("reply_dummy has not been created");
                return MUALI_ERROR;
        }
        if (bus_type != MUALI_BUS_SYSTEM && bus_type != MUALI_BUS_SESSION) {
                ULOG_ERR_F("bus type must be either MUALI_BUS_SYSTEM or"
                           " MUALI_BUS_SESSION");
                return MUALI_ERROR_INVALID;
        }
        if (bus_type == MUALI_BUS_SYSTEM) {
                conn = context->sys_conn;
        } else {
                conn = context->conn;
        }
        if (!dbus_connection_get_is_connected(conn)) {
                ULOG_ERR_F("connection %p is not open", conn);
                return MUALI_ERROR_INVALID;
        }

        /* make a copy of the reply_dummy */
        reply = dbus_message_copy(context->reply_dummy);
        if (reply == NULL) {
                ULOG_ERR_F("dbus_message_copy failed");
	        return MUALI_ERROR_OOM;
        }
        if (!dbus_message_set_destination(reply, dest)) {
                ULOG_ERR_F("dbus_message_set_destination failed");
                goto unref_and_exit;
        }
        if (!dbus_message_set_reply_serial(reply,
                                           (dbus_uint32_t)message_id)) {
                ULOG_ERR_F("dbus_message_set_reply_serial failed");
                goto unref_and_exit;
        }

        ret = dbus_message_append_args(reply, DBUS_TYPE_STRING, &string,
                                       DBUS_TYPE_INVALID);
        if (!ret) {
                ULOG_ERR_F("dbus_message_append_args failed");
                goto unref_and_exit;
        }

        if (!dbus_connection_send(conn, reply, NULL)) {
                ULOG_ERR_F("dbus_connection_send failed");
                goto unref_and_exit;
        }
        dbus_message_unref(reply);
        return MUALI_ERROR_SUCCESS;

unref_and_exit:
        dbus_message_unref(reply);
        return MUALI_ERROR_OOM;
}

