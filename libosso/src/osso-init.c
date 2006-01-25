/**
 * @file osso-init.c
 * This file implements all initialisation and shutdown of the library.
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
#include "osso-init.h"
#include "osso-log.h"

/* for internal use only
 * This function strdups application name and makes it
 * suitable for being part of an object path, or returns NULL.
 * Currently needed for allowing '.' in application name. */
gchar* __attribute__ ((visibility("hidden")))
appname_to_valid_path_component(const gchar *application)
{
    gchar* copy = NULL, *p = NULL;
    g_assert(application != NULL);
    copy = g_strdup(application);
    if (copy == NULL) {
       return NULL;
    }
    for (p = g_strrstr(copy, "."); p != NULL; p = g_strrstr(p + 1, ".")) {
        *p = '/';
    }
    return copy;
}

gboolean __attribute__ ((visibility("hidden")))
validate_appname(const gchar *application)
{
    if (application == NULL || strstr(application, "/") != NULL) {
	return FALSE;
    }
    return TRUE;
}

gboolean __attribute__ ((visibility("hidden")))
validate_osso_context(const osso_context_t * osso)
{
    if (osso == NULL || !validate_appname(osso->application) ||
        osso->version == NULL) {
       return FALSE;
    }
    if (strstr(osso->version, "/") != NULL) {
       return FALSE;
    }
    return TRUE;
}


/************************************************************************/
osso_context_t * osso_initialize(const gchar *application,
				 const gchar *version,
				 gboolean activation,
				 GMainContext *context)
{
    osso_context_t *osso;

    dprint("Initialising application '%s' version '%s' "
	   "%s activation, with context = %p",application, version,
	   activation?"with":"without",context);
    
    osso = _init(application, version);
    if (osso == NULL) {
        ULOG_CRIT_F("initialization failed: out of memory");
        return NULL;
    }

#ifdef LIBOSSO_DEBUG
    /* Redirect all GLib/GTK logging to OSSO logging macros */
      osso->log_handler = g_log_set_handler(NULL,
					   G_LOG_LEVEL_MASK |
					   G_LOG_FLAG_FATAL |
					   G_LOG_FLAG_RECURSION,
					   (GLogFunc)_osso_log_handler,
					   (gpointer)application);
#endif
    if (activation) {
        dprint("connecting to the activation bus");
        ULOG_WARN_F("WARNING: if the system bus activated this program, "
                    "Libosso does not connect to the session bus!");
        dprint("WARNING: if the system bus activated this program, "
               "Libosso does not connect to the session bus!");
        fprintf(stderr, "osso_initialize() WARNING: if the system bus "
          "activated this program, Libosso does not connect"
          " to the session bus!\n");
        osso->conn = _dbus_connect_and_setup(osso, DBUS_BUS_STARTER,
                                             context);
    } else {
        dprint("connecting to the session bus");
        osso->conn = _dbus_connect_and_setup(osso, DBUS_BUS_SESSION,
                                             context);
    }
    if (osso->conn == NULL) {
        if (activation) {
	    ULOG_CRIT_F("connecting to the activation bus failed");
	    dprint("connecting to the activation bus failed");
        } else {
	    ULOG_CRIT_F("connecting to the session bus failed");
	    dprint("connecting to the session bus failed");
        }
	_deinit(osso);
	return NULL;
    }
    dprint("connecting to the system bus");
    osso->sys_conn = _dbus_connect_and_setup(osso, DBUS_BUS_SYSTEM, context);
    if (osso->sys_conn == NULL) {
        ULOG_CRIT_F("connecting to the system bus failed");
        dprint("connecting to the system bus failed");
	_deinit(osso);
        return NULL;
    }
    osso->cur_conn = NULL;
    return osso;
}

/************************************************************************/
void osso_deinitialize(osso_context_t *osso)
{
    if(osso == NULL) return;
    
    _dbus_disconnect(osso, FALSE);
    _dbus_disconnect(osso, TRUE);
    
    _deinit(osso);
    
    return;
}


/************************************************************************/

static gboolean _validate(const gchar *application, const gchar* version)
{
    if ((application  == NULL) ||
	(version == NULL)) {
	return FALSE;
    }
    if (!validate_appname(application)) {
	return FALSE;
    }
    if (strstr(version, "/") != NULL) {
	return FALSE;
    }
    return TRUE;
}

/************************************************************************/
static osso_context_t * _init(const gchar *application, const gchar *version)
{
    osso_context_t * osso;
    
    if(!_validate(application, version)) return NULL;

    osso = (osso_context_t *)calloc(1, sizeof(osso_context_t));
    if(osso == NULL) {
	return NULL;
    }	

    osso->application = g_strdup(application);
    if(osso->application == NULL) {
	goto register_error0;
    }
    osso->version = g_strdup(version);
    if(osso->version == NULL) {
	goto register_error1;
    }

    osso->mime = (_osso_mime_t *)calloc(1, sizeof(_osso_mime_t));

    osso->ifs = g_array_new(FALSE, FALSE, sizeof(_osso_interface_t));
    osso->cp_plugins = g_array_new(FALSE, FALSE, sizeof(_osso_cp_plugin_t));
    osso->rpc_timeout = -1;
    return osso;

    /**** ERROR HANDLING ****/
    
    register_error1:
    if (osso->application != NULL) {
      g_free(osso->application);
    }
    register_error0:

    free(osso);
        
    return NULL;
}

/*************************************************************************/

static void _deinit(osso_context_t *osso)
{
    if(osso == NULL)
	return;
    if (osso->application != NULL) {
	g_free(osso->application);
	osso->application = NULL;
    }
    if (osso->version != NULL) {
	g_free(osso->version);
	osso->version = NULL;
    }
    if (osso->object_path != NULL) {
	g_free(osso->object_path);
	osso->object_path = NULL;
    }
    if(osso->ifs != NULL) {
        int i;
        _osso_interface_t *elem;
        /* some members need to be freed separately */
        for (i = 0; i < osso->ifs->len; ++i) {
            elem = (_osso_interface_t*)
                    &g_array_index(osso->ifs, _osso_interface_t, i);
            if (elem->interface != NULL) {
                g_free(elem->interface);
        	elem->interface = NULL;
            }
            if (elem->can_free_data) {
                free(elem->data);
                elem->data = NULL;
            }
        }
        g_array_free(osso->ifs, TRUE);
        osso->ifs = NULL;
    }
    if(osso->autosave != NULL) {
	free(osso->autosave);
	osso->autosave = NULL;
    }
    if(osso->mime != NULL) {
	free(osso->mime);
	osso->mime = NULL;
    }
    osso->exit.cb = NULL;
    osso->exit.data = NULL;
    
#ifdef LIBOSSO_DEBUG
    g_log_remove_handler(NULL, osso->log_handler);
    osso->log_handler = 0;
#endif
    free(osso);
    osso = NULL;
}

/*************************************************************************/
static DBusConnection * _dbus_connect_and_setup(osso_context_t *osso,
						DBusBusType bus_type,
						GMainContext *context)
{
    DBusConnection *conn;
    DBusError err;
    DBusObjectPathVTable vtable;
    gchar service[255];
    gint i;
    
    dbus_error_init(&err);
    dprint("getting the DBUS");
    conn = dbus_bus_get(bus_type, &err);
    if (conn == NULL) {
        ULOG_ERR_F("Unable to connect to the D-BUS daemon: %s", err.message);
        dprint("Unable to connect to the D-BUS daemon: %s", err.message);
        dbus_error_free(&err);
        return NULL;
    }
    dbus_connection_setup_with_g_main(conn, context);
    
    dprint("connection to the D-BUS daemon was a success");
    
    g_snprintf(service, 255, "%s.%s", OSSO_BUS_ROOT, osso->application);
    dprint("service='%s'",service);

    i = dbus_bus_request_name(conn, service, 0, &err);
    dprint("acquire service returned '%d'", i);
    if (i == -1) {
        ULOG_ERR_F("dbus_bus_request_name failed: %s", err.message);
	dbus_error_free(&err);
	goto dbus_conn_error1;
    }
    
    if (osso->object_path == NULL) {
        char* copy = NULL;
        copy = appname_to_valid_path_component(osso->application);
        osso->object_path = g_strdup_printf("/com/nokia/%s", copy);
        if (osso->object_path == NULL) {
            ULOG_ERR_F("g_strdup_printf failed");
            g_free(copy);
            goto dbus_conn_error1;
        }
        g_free(copy);
    }
    dprint("osso->object_path='%s'", osso->object_path);

    vtable.message_function = _msg_handler;
    vtable.unregister_function = NULL;
    
    if(!dbus_connection_register_object_path(conn, osso->object_path,
					     &vtable, osso)) {
	ULOG_ERR_F("Unable to register object '%s'\n", osso->object_path);
	goto dbus_conn_error2;
    }
   
    dbus_connection_set_exit_on_disconnect(conn, FALSE );

#ifdef LIBOSSO_DEBUG
    dprint("adding Filter function %p",&_debug_filter);
    if(!dbus_connection_add_filter(conn, &_debug_filter, NULL, NULL))
    {
        ULOG_ERR_F("dbus_connection_add_filter failed");
	goto dbus_conn_error3;
    }
#endif
    dprint("adding Filter function %p",&_msg_handler);
    if(!dbus_connection_add_filter(conn, &_msg_handler, osso, NULL))
    {
        ULOG_ERR_F("dbus_connection_add_filter failed");
	goto dbus_conn_error4;
    }
    dprint("My base service is '%s'", dbus_bus_get_unique_name(conn));

    return conn;

    /**** ERROR HANDLING ****/

    dbus_conn_error4:
#ifdef LIBOSSO_DEBUG
    dbus_connection_remove_filter(conn, _debug_filter, NULL);
    dbus_conn_error3:
#endif
    dbus_connection_unregister_object_path(conn, osso->object_path);

    dbus_conn_error2:
    g_free(osso->object_path);
    dbus_conn_error1:
        
    /* no explicit disconnection, because the connections are shared */
    dbus_connection_unref(conn);

    return NULL;
}
/*************************************************************************/

static void _dbus_disconnect(osso_context_t *osso, gboolean sys)
{
    DBusConnection *conn = NULL;
    g_assert(osso != NULL);
    if (sys) {
        conn = osso->sys_conn;
        osso->sys_conn = NULL;
    } else {
        conn = osso->conn;
        osso->conn = NULL;
    }
    dbus_connection_remove_filter(conn, _msg_handler, osso);
#ifdef LIBOSSO_DEBUG
    dbus_connection_remove_filter(conn, _debug_filter, NULL);
#endif
    dbus_connection_unregister_object_path(conn, osso->object_path);
    /* no explicit disconnection, because the connections are shared */
    dbus_connection_unref(conn);
    return;
}

/*************************************************************************/
DBusHandlerResult __attribute__ ((visibility("hidden")))
_msg_handler(DBusConnection *conn, DBusMessage *msg, void *data)
{
    osso_context_t *osso;
    const gchar *interface;
    gboolean is_method = FALSE;
    guint i;

    osso = (osso_context_t *)data;
    dprint("message!");
    
    if(dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_METHOD_CALL)
	is_method = TRUE;
    else if(dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL)
	is_method = FALSE;
    else
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    dprint("");
    interface = dbus_message_get_interface(msg);
    
    dprint("Got a %s %s interface '%s'",
	   type_to_name(dbus_message_get_type(msg)), 
	   is_method?"to":"from", interface);

    osso->cur_conn = conn;

    for(i=0; i<osso->ifs->len; i++) {
	_osso_interface_t *intf;
	DBusHandlerResult r;
	intf = &g_array_index(osso->ifs, _osso_interface_t, i);
	if(intf->method == is_method) {
	    dprint("comparing '%s' to '%s'",interface, intf->interface);
	    if(strcmp(interface, intf->interface) == 0) {
		dprint("match!, now calling callback at %p", intf->handler);
		r = (intf->handler)(osso, msg, intf->data);
		if(r == DBUS_HANDLER_RESULT_HANDLED) {
		    return r;
		}
	    }
	}
    }	

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


/************************************************************************/
void __attribute__ ((visibility("hidden")))
_msg_handler_set_cb_f(osso_context_t *osso, const gchar *interface,
                      _osso_interface_cb_f *cb, gpointer data, 
                      gboolean method)
{   
    _osso_interface_t intf;
    if( (osso == NULL) || (interface == NULL) || (cb == NULL) )
	return;
    
    intf.handler = cb;
    intf.interface = g_strdup(interface);
    intf.data = data;
    intf.method = method;
    intf.can_free_data = FALSE;
    
    dprint("intf.handler = %p",intf.handler);
    dprint("intf.interface = '%s'",intf.interface);
    dprint("intf.data = %p", intf.data);
    dprint("intf.method = %s", intf.method?"TRUE":"FALSE");
    g_array_append_val(osso->ifs, intf);

    return;
}

void __attribute__ ((visibility("hidden")))
_msg_handler_set_cb_f_free_data(osso_context_t *osso, const gchar *interface,
                      _osso_interface_cb_f *cb, gpointer data, 
                      gboolean method)
{   
    _osso_interface_t intf;
    if( (osso == NULL) || (interface == NULL) || (cb == NULL) )
	return;
    
    intf.handler = cb;
    intf.interface = g_strdup(interface);
    intf.data = data;
    intf.method = method;
    intf.can_free_data = TRUE;
    
    g_array_append_val(osso->ifs, intf);

    return;
}

/************************************************************************/
gpointer __attribute__ ((visibility("hidden")))
_msg_handler_rm_cb_f(osso_context_t *osso, const gchar *interface,
                     _osso_interface_cb_f *cb, gboolean method)
{   
    guint i=0;
    if( (osso == NULL) || (interface == NULL) || (cb == NULL) )
	return NULL;

    for(i = 0; i < osso->ifs->len; i++) {
	_osso_interface_t *intf;
	intf = &g_array_index(osso->ifs, _osso_interface_t, i);
	if( (intf->handler == cb) &&
	    (strcmp(intf->interface, interface)==0) &&
	    (intf->method == method) ) {
	    gpointer ret;
	    ret = intf->data;
	    g_free(intf->interface);
	    g_array_remove_index_fast(osso->ifs, i);
	    return ret;
	}
    }

    return NULL;
}


#ifdef LIBOSSO_DEBUG
/*************************************************************/
static GLogFunc _osso_log_handler(const gchar *log_domain,
				 GLogLevelFlags log_level,
				 const gchar *message,
				 gpointer user_data)
{
  switch (log_level) {
  case G_LOG_LEVEL_ERROR:
    ULOG_ERR("%s", message);
    break;
  case G_LOG_LEVEL_CRITICAL:
    ULOG_CRIT("%s", message);
    break;
  case G_LOG_LEVEL_WARNING:
    ULOG_WARN("%s", message);
    break;
  case G_LOG_LEVEL_MESSAGE:
    ULOG_INFO("%s", message);
    break;
  case G_LOG_LEVEL_INFO:
    ULOG_INFO("%s", message);
    break;
  case G_LOG_LEVEL_DEBUG:
    ULOG_DEBUG("%s", message);
    break;
  default:
    ULOG_INFO("%s", message);
    break;
  }
  return 0; /* Does not really matter what we return here... */
}


/*************************************************************/
/* This function is copied in verbatim
 from dbus 0.20 file tools/dbus-print-message.c
 */
static char* type_to_name (int message_type)
{
    switch (message_type)
    {
      case DBUS_MESSAGE_TYPE_SIGNAL:
	return "signal";
      case DBUS_MESSAGE_TYPE_METHOD_CALL:
	return "method call";
      case DBUS_MESSAGE_TYPE_METHOD_RETURN:
	return "method return";
      case DBUS_MESSAGE_TYPE_ERROR:
	return "error";
      default:
	return "(unknown message type)";
    }
}

/* The content of this  function is copied in verbatim
 from dbus 0.20 file tools/dbus-print-message.c
 */
static DBusHandlerResult 
    _debug_filter(DBusConnection *conn, DBusMessage *message, void *data)
{
    const char *sender;
    int message_type;
    
    message_type = dbus_message_get_type (message);
    sender = dbus_message_get_sender (message); 
    
    switch (message_type)
    {
      case DBUS_MESSAGE_TYPE_METHOD_CALL:
      case DBUS_MESSAGE_TYPE_SIGNAL:
	dprint ("%s interface=%s; member=%s; sender=%s\n",
		type_to_name (message_type),
		dbus_message_get_interface (message),
		dbus_message_get_member (message),
		sender ? sender : "(no sender)");
	break;      
      case DBUS_MESSAGE_TYPE_METHOD_RETURN:
	dprint ("%s; sender=%s\n",
		type_to_name (message_type),
		sender ? sender : "(no sender)");
	break;
	
      case DBUS_MESSAGE_TYPE_ERROR:
	dprint ("%s name=%s; sender=%s\n",
		type_to_name (message_type),
		dbus_message_get_error_name (message),
		sender ? sender : "(no sender)");
	break;
	
      default:
	dprint ("Message of unknown type %d received\n",
		message_type);
	break;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


#endif /* LIBOSSO_DEBUG */

gpointer osso_get_dbus_connection(osso_context_t *osso)
{
    return (gpointer) osso->conn;
}


gpointer osso_get_sys_dbus_connection(osso_context_t *osso)
{
  return (gpointer) osso->sys_conn;
}
