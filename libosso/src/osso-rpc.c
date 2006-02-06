/**
 * @file osso-rpc.c
 * This file implements rpc calls from one application to another.
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
 */

#define TASK_NAV_SERVICE                    "com.nokia.tasknav"
/* NOTICE: Keep these in sync with values in hildon-navigator/windowmanager.c! */
#define APP_LAUNCH_BANNER_METHOD_INTERFACE  "com.nokia.tasknav.app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD_PATH       "/com/nokia/tasknav/app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD            "app_launch_banner"


#include "osso-internal.h"

static void _rm_cb_f (osso_context_t * osso, const gchar * interface,
                      osso_rpc_cb_f * cb, gpointer data);
static DBusHandlerResult _rpc_handler (osso_context_t * osso,
                                       DBusMessage * msg, gpointer data);
static void _append_args(DBusMessage *msg, int type, va_list var_args);
static void _append_arg (DBusMessage * msg, osso_rpc_t * arg);
static void _get_arg (DBusMessageIter * iter, osso_rpc_t * retval);
static void _async_return_handler (DBusPendingCall * pending, void *data);

typedef struct {
    osso_rpc_cb_f *func;
    osso_rpc_retval_free_f *retval_free;
    gpointer data;
}_osso_rpc_t;

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
osso_return_t osso_rpc_run(osso_context_t *osso, const gchar *service,
			   const gchar *object_path,
			   const gchar *interface,
			   const gchar *method, osso_rpc_t *retval,
			   int argument_type, ...)
{
    osso_return_t ret;
    va_list arg_list;
    
    if( (osso == NULL) || (service == NULL) || (object_path == NULL) ||
	(interface == NULL) || (method == NULL))
	return OSSO_INVALID;

    va_start(arg_list, argument_type);
    
    ret = _rpc_run(osso, osso->conn, service, object_path, interface,
		   method, retval, argument_type, arg_list);
    va_end(arg_list);
    return ret;
}


/************************************************************************/
osso_return_t __attribute__ ((visibility("hidden")))
_rpc_run(osso_context_t *osso, DBusConnection *conn,
         const gchar *service, const gchar *object_path,
         const gchar *interface, const gchar *method,
         osso_rpc_t *retval, int first_arg_type,
         va_list var_args)
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

    if (first_arg_type != DBUS_TYPE_INVALID) {
	_append_args(msg, first_arg_type, var_args);
    }

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
    gchar service[256] = {0}, object_path[256] = {0}, interface[256] = {0};
    va_list arg_list;
    osso_return_t ret;
    gchar* copy = NULL;

    dprint("");
    if( (osso == NULL) || (application == NULL) || (method == NULL) )
	return OSSO_INVALID;
    
    g_snprintf(service, 255, "%s.%s", OSSO_BUS_ROOT, application);
    copy = appname_to_valid_path_component(application);
    if (copy == NULL) {
	ULOG_ERR_F("failure in appname_to_valid_path_component");
	return OSSO_ERROR;
    }
    g_snprintf(object_path, 255, "%s/%s", OSSO_BUS_ROOT_PATH, copy);
    g_free(copy); copy = NULL;
    g_snprintf(interface, 255, "%s", service);

    va_start(arg_list, argument_type);

    ret = _rpc_run(osso, osso->conn, service, object_path, interface,
		   method, retval, argument_type, arg_list);
    
    va_end(arg_list);

    return ret;
}

/************************************************************************/
static osso_return_t _rpc_async_run(osso_context_t *osso,
			     const gchar *service,
			     const gchar *object_path,
			     const gchar *interface,
			     const gchar *method,
			     osso_rpc_async_f *async_cb,
			     gpointer data, int first_arg_type,
			     va_list var_args)
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

    dprint("New method: %s%s::%s:%s",service,object_path,interface,method);
    msg = dbus_message_new_method_call(service, object_path,
				       interface, method);
    if (msg == NULL) {
        ULOG_ERR_F("dbus_message_new_method_call failed");
        return OSSO_ERROR;
    }

    dbus_message_set_auto_start(msg, TRUE);
    
    if (first_arg_type != DBUS_TYPE_INVALID) {
        _append_args(msg, first_arg_type, var_args);
    }

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
    va_list arg_list;
    osso_return_t ret;
   
	
    if( (osso == NULL) || (service == NULL) || (object_path == NULL) ||
	(interface == NULL) || (method == NULL))
	return OSSO_INVALID;

    va_start(arg_list, argument_type);

    ret = _rpc_async_run(osso, service, object_path, interface, method,
			 async_cb, data, argument_type, arg_list);
    dprint("_rpc_async_run returned");
    
    va_end(arg_list);
    
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
    va_list arg_list;
    osso_return_t ret;
    gchar service[256] = {0}, object_path[256] = {0}, interface[256] = {0};
    gchar* copy = NULL;

    if( (osso == NULL) || (application == NULL) || (method == NULL) )
	return OSSO_INVALID;
    
    g_snprintf(service, 255, "%s.%s", OSSO_BUS_ROOT, application);
    copy = appname_to_valid_path_component(application);
    if (copy == NULL) {
        ULOG_ERR_F("failure in appname_to_valid_path_component");
	return OSSO_ERROR;
    }
    g_snprintf(object_path, 255, "%s/%s", OSSO_BUS_ROOT_PATH, copy);
    g_free(copy); copy = NULL;
    g_snprintf(interface, 255, "%s", service);

    va_start(arg_list, argument_type);

    ret = _rpc_async_run(osso, service, object_path, interface, method,
			 async_cb, data, argument_type, arg_list);
    dprint("_rpc_async_run returned");
    
    va_end(arg_list);

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
    DBusError err;
    struct DBusObjectPathVTable vt;
    _osso_rpc_t *rpc;
    int ret;
    dbus_bool_t bret;

    ULOG_DEBUG_F("registering method service '%s' at '%s' on "
        "interface '%s', on the %s bus", service, object_path, interface,
	use_system_bus ? "system" : "session");
    
    rpc = (_osso_rpc_t *)calloc(1, sizeof(_osso_rpc_t));
    if (rpc == NULL) {
        ULOG_ERR_F("calloc failed");
        return OSSO_ERROR;
    }
    
    dbus_error_init(&err);

    dprint("requesting service '%s'", service);
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
        return OSSO_ERROR;
    } else if (ret == -1) {
        ULOG_ERR_F("dbus_bus_request_name for '%s' failed: %s",
                   service, err.message);
	dbus_error_free(&err);
        return OSSO_ERROR;
    }

    vt.message_function = _msg_handler;
    vt.unregister_function = NULL;
    
    dprint("registering object_path: '%s'", object_path);
    bret = dbus_connection_register_object_path(use_system_bus ?
               osso->sys_conn : osso->conn, object_path, &vt,
               (void *)osso);
    if (!bret) {
        ULOG_ERR_F("dbus_connection_register_object_path failed");
	/* error ignored because we cannot distinguish OOM and already
	 * registered object path */
    }

    rpc->func = cb;
    rpc->retval_free = retval_free;
    rpc->data = data;

    _msg_handler_set_cb_f(osso, interface, _rpc_handler,
                          (gpointer)rpc, TRUE);
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
osso_return_t osso_rpc_set_cb_f_with_free (osso_context_t *osso, const gchar *service,
					   const gchar *object_path, const gchar *interface,
					   osso_rpc_cb_f *cb, gpointer data,
					   osso_rpc_retval_free_f *retval_free)
{
    if( (osso == NULL) || (service == NULL) || (object_path == NULL) ||
	(interface == NULL) || (cb == NULL))
	return OSSO_INVALID;
    return _rpc_set_cb_f(osso, service, object_path,
			 interface,cb, data, retval_free, FALSE);
}

osso_return_t osso_rpc_set_cb_f (osso_context_t *osso, const gchar *service,
				 const gchar *object_path, const gchar *interface,
				 osso_rpc_cb_f *cb, gpointer data)
{
    return osso_rpc_set_cb_f_with_free (osso, service, object_path, interface, cb, data, NULL);
}

/************************************************************************/
osso_return_t osso_rpc_set_default_cb_f_with_free (osso_context_t *osso,
						   osso_rpc_cb_f *cb,
						   gpointer data,
						   osso_rpc_retval_free_f *retval_free)
{
    gchar interface[256] = {0};
    _osso_rpc_t *rpc;
    
    if( (osso == NULL) || (cb == NULL) )
	return OSSO_INVALID;

    rpc = (_osso_rpc_t *)calloc(1, sizeof(_osso_rpc_t));
    if (rpc == NULL) {
        ULOG_ERR_F("calloc failed");
	return OSSO_ERROR;
    }
    
    g_snprintf(interface, 255, "%s.%s", OSSO_BUS_ROOT, osso->application);    
    rpc->func = cb;
    rpc->retval_free = retval_free;
    rpc->data = data;
    
    _msg_handler_set_cb_f_free_data(osso, interface, _rpc_handler,
			  (gpointer)rpc, TRUE);
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
    if( (osso == NULL) || (service == NULL) || (object_path == NULL) ||
	(interface == NULL) || (cb == NULL))
	return OSSO_INVALID;

    dbus_connection_unregister_object_path(osso->conn, object_path);

    _rm_cb_f(osso, interface, cb, data);

    return OSSO_OK;
}

/************************************************************************/

osso_return_t osso_rpc_unset_default_cb_f(osso_context_t *osso,
					  osso_rpc_cb_f *cb, gpointer data)
{
    gchar interface[256] = {0};
    
    if(osso == NULL) return OSSO_INVALID;
    if(cb == NULL) return OSSO_INVALID;
    
    g_snprintf(interface, 255, "%s.%s", OSSO_BUS_ROOT,
	       osso->application);

/*    dbus_connection_unregister_object_path(osso->conn, osso->object_path); */
    
    _rm_cb_f(osso, interface, cb, data);

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
static void _rm_cb_f(osso_context_t *osso, const gchar *interface,
		     osso_rpc_cb_f *cb, gpointer data)
{   
    guint i=0;
    if( (osso == NULL) || (interface == NULL) || (cb == NULL) )
	return;

    for(i = 0; i < osso->ifs->len; i++) {
	_osso_interface_t *intf;
	intf = &g_array_index(osso->ifs, _osso_interface_t, i);
	if( (intf->handler == &_rpc_handler) &&
	    (strcmp(intf->interface, interface)==0) &&
	    (intf->method) ) {
	    _osso_rpc_t *rpc;
	    rpc = (_osso_rpc_t *)intf->data;
	    if( (rpc->func == cb) && (rpc->data == data) ) {
		g_free(intf->interface);
		g_array_remove_index_fast(osso->ifs, i);
		free(rpc);
	    }
	    return;
	}
    }

    return;
}

/************************************************************************/

static DBusHandlerResult _rpc_handler(osso_context_t *osso, DBusMessage *msg,
			       gpointer data)
{
    _osso_rpc_t *rpc;
    DBusMessageIter iter;
    GArray *arguments;
    osso_rpc_t retval;
    gint ret;
    int i;

    rpc = (_osso_rpc_t *)data;
    dprint("rpc->func = %p",rpc->func);
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
    dprint("calling handler at %p",rpc->func);
    retval.type = DBUS_TYPE_INVALID;
    ret = (rpc->func)(dbus_message_get_interface(msg),
		      dbus_message_get_member(msg), arguments, rpc->data,
		      &retval);
    if (retval.type == DBUS_TYPE_STRING) {
       dprint("handler returned string '%s'", retval.value.s);
    }

    for (i = 0; i < arguments->len; i++)
        osso_rpc_free_val (&g_array_index (arguments, osso_rpc_t, i));
    g_array_free(arguments, TRUE);

    if(ret == OSSO_INVALID) {
	if (rpc->retval_free)
	    rpc->retval_free (&retval);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
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
    if (rpc->retval_free)
        rpc->retval_free (&retval);
    return DBUS_HANDLER_RESULT_HANDLED;
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
	g_free(rpc->interface);
	g_free(rpc->method);
	g_free(rpc);
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
    fflush(stderr);
    g_free(rpc->interface);
    g_free(rpc->method);
    g_free(rpc);
    return;
}

