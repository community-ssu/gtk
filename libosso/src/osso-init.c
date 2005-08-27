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
    if(osso == NULL) return NULL;

    /* Peform statefile clean-up, but do not fail osso_initialize()
       if it does not succeed 100% - just log a warning */
    _cleanup_state_dir(application, version);

#ifdef DEBUG
    /* Redirect all GLib/GTK logging to OSSO logging macros */
      osso->log_handler = g_log_set_handler(NULL,
					   G_LOG_LEVEL_MASK |
					   G_LOG_FLAG_FATAL |
					   G_LOG_FLAG_RECURSION,
					   (GLogFunc)_osso_log_handler,
					   (gpointer)application);
#endif
    dprint("connecting to the session bus");
    osso->conn = _dbus_connect_and_setup(osso,
			activation?DBUS_BUS_ACTIVATION:DBUS_BUS_SESSION,
					 context);
    if(osso->conn == NULL) {
	dprint("connecting to the sessionbus failed");
	_deinit(osso);
	return NULL;
    }
    dprint("connecting to the system bus");
    osso->sys_conn = _dbus_connect_and_setup(osso, DBUS_BUS_SYSTEM, context);
    if(osso->sys_conn == NULL) {
	dprint("connecting to the systembus failed");
    }
    osso->cur_conn = NULL;
    return osso;
}

/************************************************************************/
void osso_deinitialize(osso_context_t *osso)
{
    if(osso == NULL) return;
    
    _dbus_disconnect(osso->conn, osso);
    _dbus_disconnect(osso->sys_conn, osso);
    
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
    if ((strstr(application, "/") != NULL) ||
	(strstr(application, ".") != NULL)) {
	return FALSE;
    }
    if ((strstr(version, "/") != NULL) ||
	(strstr(version, "/") != NULL)) {
	return FALSE;
    }
    return TRUE;
}

/************************************************************************/
static osso_context_t * _init(const gchar *application, const gchar *version)
{
    osso_context_t * osso;
    
    if(!_validate(application, version)) return NULL;

    osso = (osso_context_t *)malloc(sizeof(osso_context_t));
    if(osso == NULL) {
	return NULL;
    }	
    memset(osso, 0, sizeof(osso_context_t));

    osso->application = g_strdup(application);
    if(osso->application == NULL) {
	goto register_error0;
    }
    osso->version = g_strdup(version);
    if(osso->version == NULL) {
	goto register_error1;
    }

    osso->hw = (_osso_hw_cb_t *)malloc(sizeof(_osso_hw_cb_t));
    if(osso->hw == NULL) {
	goto register_error2;
    }
    osso->hw->shutdown_ind.set = FALSE;
    osso->hw->sig_device_mode_ind.set = FALSE;
    
    osso->hw_state = (osso_hw_state_t *)malloc(sizeof(osso_hw_state_t));
    if(osso->hw_state == NULL) {
	goto register_error3;
    }
    memset(osso->hw_state, 0, sizeof(osso_hw_state_t));

    osso->mime = (_osso_mime_t *)malloc(sizeof(_osso_mime_t));

    osso->ifs = g_array_new(FALSE, FALSE, sizeof(_osso_interface_t));
    osso->cp_plugins = g_array_new(FALSE, FALSE, sizeof(_osso_cp_plugin_t));
    osso->rpc_timeout = -1;
    osso->exit.cb = NULL;
    osso->exit.data = NULL;
    return osso;

    /**** ERROR HANDLING ****/
    
    register_error3:
    if(osso->hw != NULL)
	free(osso->hw);
    
    register_error2:
    if (osso->version != NULL) {
      g_free(osso->version);
    }

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
        /* interface members need to be freed separately */
        for (i = 0; i < osso->ifs->len; ++i) {
            elem = &g_array_index(osso->ifs, _osso_interface_t, i);
            if (elem->interface != NULL) {
                g_free(elem->interface);
        	elem->interface = NULL;
            }
        }
        g_array_free(osso->ifs, TRUE);
        osso->ifs = NULL;
    }
    if(osso->autosave != NULL) {
	free(osso->autosave);
	osso->autosave = NULL;
    }
    if(osso->hw != NULL) {
	free(osso->hw);
	osso->hw = NULL;
    }
    if(osso->hw_state != NULL) {
	free(osso->hw_state);
	osso->hw_state = NULL;
    }
    if(osso->mime != NULL) {
	free(osso->mime);
	osso->mime = NULL;
    }
    osso->exit.cb = NULL;
    osso->exit.data = NULL;
    
#ifdef DEBUG
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
    gchar service[255], *error=NULL;
    gint i;
    
    dbus_error_init(&err);
    dprint("getting the DBUS");
    conn = dbus_bus_get(bus_type, &err);
    if(conn == NULL) {
	ULOG_ERR_F("Unable to connect to the D-BUS daemon: %s", err.message);
	dprint("Unable to connect to the D-BUS daemon: %s", err.message);
	return NULL;
    }
    dbus_connection_setup_with_g_main(conn, context);
    
    dprint("connection to the D-BUS daemon was a success");
    
    g_snprintf(service, 255, "%s.%s", OSSO_BUS_ROOT, osso->application);
    dprint("service='%s'",service);

    i = dbus_bus_acquire_service(conn, service, 
				 0, /* undocumented parameter, flags */
				 &err);
    dprint("acquire service returned '%d'",i);
    if(i <= 0) {
	if(i == 0) {
	    ULOG_ERR_F("Unable to acquire service '%s': Invalid parameters\n", 
		     service);
	}
	else {
	    ULOG_ERR_F("Unable to acquire service '%s': %s\n",
		     service, err.message);
	}
	error = "Error: Not enough memory";
	dbus_error_free(&err);
	goto dbus_conn_error1;
    }
    
    if (osso->object_path == NULL) {
        osso->object_path = g_strdup_printf("/com/nokia/%s",
                                            osso->application);
        if (osso->object_path == NULL)
            goto dbus_conn_error1;
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

#ifdef DEBUG
    dprint("adding Filter function %p",&_debug_filter);
    if(dbus_connection_add_filter(conn, &_debug_filter, NULL, NULL)
       != TRUE)
    {
	error = "Error: unable to add debug filter";
	goto dbus_conn_error3;
    }
#endif
    dprint("adding Filter function %p",&_msg_handler);
    if(dbus_connection_add_filter(conn, &_msg_handler, osso, NULL)
       != TRUE)
    {
	error = "Error: unable to add _msg_handler as a filter";
	goto dbus_conn_error4;
    }
    dprint("My base service is '%s'",
	   dbus_bus_get_base_service(conn));

    dbus_error_free(&err);

    return conn;

    /**** ERROR HANDLING ****/

/*    dbus_connection_remove_filter(conn, _msg_handler, osso); */
    dbus_conn_error4:
#ifdef DEBUG
    dbus_connection_remove_filter(conn, _debug_filter, NULL);
    dbus_conn_error3:
#endif
    dbus_connection_unregister_object_path(conn, osso->object_path);

    dbus_conn_error2:
    g_free(osso->object_path);
    dbus_conn_error1:
        
    dbus_connection_disconnect(conn);
    dbus_connection_unref(conn);
    if(error != NULL) {
	ULOG_ERR_F("%s", error);
    }

    return NULL;
}
/*************************************************************************/

static void _dbus_disconnect(DBusConnection *conn, osso_context_t *osso)
{
    if( (conn == NULL) || (osso == NULL) )
	return;
    dbus_connection_remove_filter(conn, _msg_handler, osso);
#ifdef DEBUG
    dbus_connection_remove_filter(conn, _debug_filter, NULL);
#endif
    dbus_connection_unregister_object_path(conn, osso->object_path);

    dbus_connection_disconnect(conn);
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
    
    dprint("intf.handler = %p",intf.handler);
    dprint("intf.interface = '%s'",intf.interface);
    dprint("intf.data = %p", intf.data);
    dprint("intf.method = %s", intf.method?"TRUE":"FALSE");
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

/*************************************************************/

static gint _cleanup_state_dir(const gchar *application,
			       const gchar *version)
{
  gchar *dirname = NULL;
  gchar *filename = NULL;
  struct passwd *pwdstruct = NULL;
  DIR *directory;
  struct stat buf;
  struct dirent *entry = NULL;

  if (application == NULL || version == NULL) {
    return OSSO_ERROR;
  }
  
  pwdstruct = getpwuid(geteuid());
  if (pwdstruct == NULL)
    {
      osso_log(LOG_ERR, "Unknown user!");
      return OSSO_ERROR;
    }
  dirname = g_strconcat(pwdstruct->pw_dir, STATEDIRS,
			  application, NULL);
  if (dirname == NULL) {
    return OSSO_ERROR;
  }
  directory = opendir(dirname);
  if (directory == NULL) {
    g_free(dirname);
    return OSSO_ERROR;
  }

  /* Walk through the directory, remove everything but the current
     version and directories */
  
  while ( (entry = readdir(directory)) ) {
    if (strcmp(entry->d_name, version) != 0) {
      filename = g_strconcat(dirname, "/", entry->d_name, NULL);
      if (filename == NULL) {
	continue;
      }
      if (stat(filename, &buf) != 0) {
	g_free(filename);
	continue;
      }
      if (S_ISREG(buf.st_mode)) {
	unlink(filename);
      }
      g_free(filename);
    }
  }
  g_free(dirname);
  g_free(directory);
  return OSSO_OK;
}

#ifdef DEBUG
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


#endif /* DEBUG */

gpointer osso_get_dbus_connection(osso_context_t *osso)
{
    return (gpointer) osso->conn;
}


gpointer osso_get_sys_dbus_connection(osso_context_t *osso)
{
  return (gpointer) osso->sys_conn;
}
