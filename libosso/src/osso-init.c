/**
 * @file osso-init.c
 * This file implements all initialisation and shutdown of the library.
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
#include "osso-init.h"
#include "osso-log.h"
#include <assert.h>
#include "muali.h"

/*  Muali filter is disabled temporarily
static DBusHandlerResult
_muali_filter(DBusConnection *conn, DBusMessage *msg, void *data);
*/

/* for internal use only
 * This function strdups application name and makes it
 * suitable for being part of an object path, or returns NULL.
 * Currently needed for allowing '.' in application name. */
static gchar* appname_to_valid_path_component(const gchar *application)
{
    gchar *copy, *p;

    assert(application != NULL);

    copy = g_strdup(application);
    if (copy == NULL) {
        return NULL;
    }

    for (p = copy; *p != '\0'; ++p) {
         if (*p == '.') {
             *p = '/';
         }
    }
    return copy;
}

static void
compose_hash_key(const char *service, const char *object_path,
                 const char *interface, char *key)
{
    key[0] = '\0';
    strncat(key, interface, MAX_IF_LEN);
    strncat(key, object_path, MAX_OP_LEN);
    if (service != NULL) {
        strncat(key, service, MAX_SVC_LEN);
    }
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
        osso->version[0] == '\0') {
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
    ULOG_DEBUG_F("application '%s', version '%s'", application, version);

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
        ULOG_WARN_F("connecting to both D-BUS busses, 'activation' "
                    "argument does not have any effect");
    }
    dprint("connecting to the session bus");
    osso->conn = _dbus_connect_and_setup(osso, DBUS_BUS_SESSION, context);
    if (osso->conn == NULL) {
        ULOG_CRIT_F("connecting to the session bus failed");
        dprint("connecting to the session bus failed");
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
    if (application == NULL || version == NULL) {
	return FALSE;
    }
    if (!validate_appname(application)) {
	return FALSE;
    }
    if (strchr(version, '/') != NULL) {
        ULOG_ERR_F("invalid version string '%s'", version);
	return FALSE;
    }
    return TRUE;
}

void __attribute__ ((visibility("hidden")))
make_default_interface(const char *application, char *interface)
{
    assert(application != NULL);
    assert(interface != NULL);

    if (g_strrstr(application, ".") != NULL) {
        g_snprintf(interface, MAX_IF_LEN, "%s", application);
    } else {
        g_snprintf(interface, MAX_IF_LEN, OSSO_BUS_ROOT ".%s",
                   application);
    }
}

void __attribute__ ((visibility("hidden")))
make_default_service(const char *application, char *service)
{
    assert(application != NULL);
    assert(service != NULL);

    if (g_strrstr(application, ".") != NULL) {
        g_snprintf(service, MAX_SVC_LEN, "%s", application);
    } else {
        g_snprintf(service, MAX_SVC_LEN, OSSO_BUS_ROOT ".%s",
                   application);
    }
}

gboolean __attribute__ ((visibility("hidden")))
make_default_object_path(const char *application, char *path)
{
    char *copy;

    assert(application != NULL);
    assert(path != NULL);

    copy = appname_to_valid_path_component(application);
    if (copy == NULL) {
        return FALSE;
    }
    if (g_strrstr(application, ".") != NULL) {
        g_snprintf(path, MAX_OP_LEN, "/%s", copy);
    } else {
        g_snprintf(path, MAX_OP_LEN, OSSO_BUS_ROOT_PATH "/%s", copy);
    }
    g_free(copy);
    return TRUE;
}

/************************************************************************/

static void free_handler(gpointer data, gpointer user_data)
{
    _osso_handler_t *h = data;

    if (h != NULL) {
        if (h->can_free_data) {
            free(h->data);
            h->data = NULL;
        }
        free(h);
    }
}

static void free_uniq_hash_value(gpointer data)
{
    _osso_hash_value_t *elem = data;

    if (elem != NULL) {
        if (elem->handlers != NULL) {
            g_list_foreach(elem->handlers, free_handler, NULL);
            g_list_free(elem->handlers);
            elem->handlers = NULL;
        }
        free(elem);
    }
}

static void free_if_hash_value(gpointer data)
{
    _osso_hash_value_t *elem = data;

    if (elem != NULL) {
        if (elem->handlers != NULL) {
            g_list_free(elem->handlers);
            elem->handlers = NULL;
        }
        free(elem);
    }
}

static osso_context_t *_init(const gchar *application, const gchar *version)
{
    osso_context_t *osso;
    
    if (!_validate(application, version)) {
	ULOG_ERR_F("invalid arguments");
	return NULL;
    }

    osso = calloc(1, sizeof(osso_context_t));
    if (osso == NULL) {
	ULOG_ERR_F("calloc failed");
	return NULL;
    }	

    g_snprintf(osso->application, MAX_APP_NAME_LEN, "%s", application);
    g_snprintf(osso->version, MAX_VERSION_LEN, "%s", version);
    make_default_interface((const char*)application, osso->interface);
    make_default_service((const char*)application, osso->service);

    if (!make_default_object_path((const char*)application,
        osso->object_path)) {
        ULOG_ERR_F("make_default_object_path() failed");
        free(osso);
        return NULL;
    }

    osso->uniq_hash = g_hash_table_new_full(g_str_hash, g_str_equal,
                                            free, free_uniq_hash_value);
    osso->if_hash = g_hash_table_new_full(g_str_hash, g_str_equal,
                                          free, free_if_hash_value);
    osso->id_hash = g_hash_table_new_full(g_int_hash, g_int_equal,
                                          NULL, free_uniq_hash_value);
    if (osso->uniq_hash == NULL || osso->if_hash == NULL
        || osso->id_hash == NULL) {
        ULOG_ERR_F("g_hash_table_new_full failed");
        free(osso);
        return NULL;
    }
    osso->cp_plugins = g_array_new(FALSE, FALSE, sizeof(_osso_cp_plugin_t));
    osso->rpc_timeout = -1;
    return osso;
}

/*************************************************************************/

static void _deinit(osso_context_t *osso)
{
    if (osso == NULL) {
	return;
    }
    if (osso->uniq_hash != NULL) {
        g_hash_table_destroy(osso->uniq_hash);
    }
    if (osso->if_hash != NULL) {
        g_hash_table_destroy(osso->if_hash);
    }
    if (osso->id_hash != NULL) {
        g_hash_table_destroy(osso->id_hash);
    }
    if (osso->cp_plugins != NULL) {
        g_array_free(osso->cp_plugins, TRUE);
    }
    
#ifdef LIBOSSO_DEBUG
    g_log_remove_handler(NULL, osso->log_handler);
    osso->log_handler = NULL;
#endif
    memset(osso, 0, sizeof(osso_context_t));
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
    int ret;
    
    dbus_error_init(&err);
    dprint("getting the DBUS");
    conn = dbus_bus_get(bus_type, &err);
    if (conn == NULL) {
        ULOG_ERR_F("Unable to connect to the D-BUS daemon: %s", err.message);
        dbus_error_free(&err);
        return NULL;
    }
    dbus_connection_setup_with_g_main(conn, context);
    
    dprint("connection to the D-BUS daemon was a success");
    dprint("service='%s'", osso->service);

    ret = dbus_bus_request_name(conn, osso->service,
                                DBUS_NAME_FLAG_ALLOW_REPLACEMENT, &err);
    if (ret == -1) {
        ULOG_ERR_F("dbus_bus_request_name failed: %s", err.message);
	dbus_error_free(&err);
	goto dbus_conn_error1;
    }
    
    dprint("osso->object_path='%s'", osso->object_path);

    dbus_connection_set_exit_on_disconnect(conn, FALSE);

#ifdef LIBOSSO_DEBUG
    dprint("adding debug filter function %p", _debug_filter);
    if(!dbus_connection_add_filter(conn, &_debug_filter, NULL, NULL))
    {
        ULOG_ERR_F("dbus_connection_add_filter failed");
	goto dbus_conn_error3;
    }
#endif

    if (!dbus_connection_add_filter(conn, _msg_handler, osso, NULL))
    {
        ULOG_ERR_F("dbus_connection_add_filter failed");
	goto dbus_conn_error4;
    }
    /* FIXME: there are two filters because semantics in the new
     * muali API are slightly different (stricter matching) */
    /*
    if (!dbus_connection_add_filter(conn, _muali_filter, osso, NULL))
    {
        ULOG_ERR_F("dbus_connection_add_filter failed");
	goto dbus_conn_error4;
    }
    */
    dprint("My base service is '%s'", dbus_bus_get_unique_name(conn));

    return conn;

    /**** ERROR HANDLING ****/

    dbus_conn_error4:
#ifdef LIBOSSO_DEBUG
    dbus_connection_remove_filter(conn, _debug_filter, NULL);
    dbus_conn_error3:
#endif

    dbus_conn_error1:
        
    /* no explicit disconnection, because the connections are shared */
    /* no unref either, because it makes assert() fail in DBus */

    return NULL;
}
/*************************************************************************/

static void _dbus_disconnect(osso_context_t *osso, gboolean sys)
{
    DBusConnection *conn = NULL;
    assert(osso != NULL);
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
    /* no explicit disconnection, because the connections are shared */
    /* no unref either, because it makes assert() fail in DBus */
    return;
}

/*************************************************************************/

DBusHandlerResult __attribute__ ((visibility("hidden")))
_msg_handler(DBusConnection *conn, DBusMessage *msg, void *data)
{
    osso_context_t *osso;
    _osso_hash_value_t *elem;
    gboolean is_method;
    const char *interface;
#ifdef OSSOLOG_COMPILE
    gboolean found = FALSE;
#endif

    osso = data;

    assert(osso != NULL);

    if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_METHOD_CALL)
	is_method = TRUE;
    else if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL)
	is_method = FALSE;
    else
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    /* FIXME: this is kind of brain-damaged: only interface is considered
     * (would require new API to fix, to keep backwards compatibility) */
    interface = dbus_message_get_interface(msg);

    if (interface == NULL) {
        ULOG_DEBUG_F("interface of the message was NULL");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    ULOG_DEBUG_F("key = '%s'", interface);
    elem = g_hash_table_lookup(osso->if_hash, interface);

    if (elem != NULL) {
        GList *list;

        osso->cur_conn = conn;

        list = g_list_first(elem->handlers);
        while (list != NULL) {
            _osso_handler_t *handler;
            DBusHandlerResult ret;

            handler = list->data;

            if (handler->method == is_method) {
                ULOG_DEBUG_F("before calling the handler");
                ULOG_DEBUG_F(" handler = %p", handler->handler);
                ULOG_DEBUG_F(" data = %p", handler->data);
                ret = (*handler->handler)(osso, msg, handler->data);
                ULOG_DEBUG_F("after calling the handler");
                if (ret == DBUS_HANDLER_RESULT_HANDLED) {
                    return ret;
                }
#ifdef OSSOLOG_COMPILE
                found = TRUE;
#endif
            }

            list = g_list_next(list);
        }
    } 
#ifdef OSSOLOG_COMPILE
    if (!found) {
        ULOG_DEBUG_F("suitable handler not found from the hash table");
    }
#endif

#if 0
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
#endif

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

inline static int muali_convert_msgtype(int t)
{
    switch (t) {
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
                    return MUALI_EVENT_MESSAGE;
            case DBUS_MESSAGE_TYPE_SIGNAL:
                    return MUALI_EVENT_SIGNAL;
            case DBUS_MESSAGE_TYPE_ERROR:
                    return MUALI_EVENT_ERROR;
            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
                    return MUALI_EVENT_REPLY;
            default:
                    ULOG_ERR_F("unknown message type %d", t);
                    return MUALI_EVENT_NONE;
    }
}

inline static int types_match(int a, int b)
{
    if (a == b) return 1;

    if (a == MUALI_EVENT_MESSAGE_OR_SIGNAL &&
        (b == MUALI_EVENT_MESSAGE || b == MUALI_EVENT_SIGNAL))
            return 1;

    if (a == MUALI_EVENT_REPLY_OR_ERROR &&
        (b == MUALI_EVENT_REPLY || b == MUALI_EVENT_ERROR))
            return 1;

    if (b == MUALI_EVENT_MESSAGE_OR_SIGNAL &&
        (a == MUALI_EVENT_MESSAGE || a == MUALI_EVENT_SIGNAL))
            return 1;

    if (b == MUALI_EVENT_REPLY_OR_ERROR &&
        (a == MUALI_EVENT_REPLY || a == MUALI_EVENT_ERROR))
            return 1;

    return 0;
}

inline static int str_match(const char *a, const char *b)
{
    if (a == NULL || b == NULL) return 1;

    if (strcmp(a, b) == 0) return 1;

    return 0;
}

#if 0
/* filter function for muali API */
static DBusHandlerResult
_muali_filter(DBusConnection *conn, DBusMessage *msg, void *data)
{
    muali_context_t *muali;
    _osso_hash_value_t *elem;
    int msgtype;
    const char *interface;
#ifdef OSSOLOG_COMPILE
    gboolean found = FALSE;
#endif

    muali = data;

    assert(muali != NULL);

    msgtype = muali_convert_msgtype(dbus_message_get_type(msg));
    if (msgtype == MUALI_EVENT_NONE) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    /* interface is used as hash key to limit number of initial matches */
    interface = dbus_message_get_interface(msg);

    if (interface == NULL) {
        ULOG_DEBUG_F("interface of the message was NULL");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    ULOG_DEBUG_F("key = '%s'", interface);
    elem = g_hash_table_lookup(muali->if_hash, interface);

    if (elem != NULL) {
        GList *list;

        muali->cur_conn = conn;

        list = g_list_first(elem->handlers);
        while (list != NULL) {
            _osso_handler_t *handler;
            DBusHandlerResult ret;

            handler = list->data;

            if (types_match(handler->data->event_type, msgtype) &&
                str_match(handler->data->service,
                          dbus_message_get_sender(msg)) &&
                str_match(handler->data->path,
                          dbus_message_get_path(msg)) &&
                /* interface has matched already */
                str_match(handler->data->name,
                          dbus_message_get_member(msg))) {

                ULOG_DEBUG_F("before calling the handler");
                ULOG_DEBUG_F(" handler = %p", handler->handler);
                ULOG_DEBUG_F(" data = %p", handler->data);
                ret = (*handler->handler)(muali, msg, handler->data);
                ULOG_DEBUG_F("after calling the handler");
                if (ret == DBUS_HANDLER_RESULT_HANDLED) {
                    return ret;
                }
#ifdef OSSOLOG_COMPILE
                found = TRUE;
#endif
            }

            list = g_list_next(list);
        }
    } 
#ifdef OSSOLOG_COMPILE
    if (!found) {
        ULOG_DEBUG_F("suitable handler not found from the hash table");
    }
#endif

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
#endif

/************************************************************************/

static gboolean add_to_if_hash(osso_context_t *osso,
                               const _osso_handler_t *handler,
                               const char *interface)
{
    _osso_hash_value_t *old;

    old = g_hash_table_lookup(osso->if_hash, interface);
    if (old != NULL) {
        old->handlers = g_list_append(old->handlers, (_osso_handler_t*)handler);
    } else {
        _osso_hash_value_t *new_elem;
        char *new_key;

        /* we need to allocate a new hash table element */
        new_elem = calloc(1, sizeof(_osso_hash_value_t));
        if (new_elem == NULL) {
            ULOG_ERR_F("calloc() failed");
            return FALSE;
        }

        new_key = strdup(interface);
        if (new_elem == NULL) {
            ULOG_ERR_F("calloc() failed");
            free(new_elem);
            return FALSE;
        }

        new_elem->handlers = g_list_append(NULL, (_osso_handler_t*)handler);

        g_hash_table_insert(osso->if_hash, new_key, new_elem);
    }
    return TRUE;
}

static gboolean set_handler_helper(osso_context_t *osso,
                                   const char *service,
                                   const char *object_path,
                                   const char *interface,
                                   _osso_handler_f *cb,
                                   _osso_callback_data_t *data, 
                                   gboolean method,
                                   gboolean can_free_data)
{
    char uniq_key[MAX_HASH_KEY_LEN + 1];
    _osso_hash_value_t *old;
    _osso_handler_t *handler;

    compose_hash_key(service, object_path, interface, uniq_key);

    handler = calloc(1, sizeof(_osso_handler_t));
    if (handler == NULL) {
        ULOG_ERR_F("calloc() failed");
        return FALSE;
    }

    handler->handler = cb;
    handler->data = data;
    handler->method = method;
    handler->can_free_data = can_free_data;

    /* warn about the old element if it exists */
    old = g_hash_table_lookup(osso->uniq_hash, uniq_key);
    if (old != NULL) {
        ULOG_WARN_F("Libosso WARNING: yet another handler"
                    " registered for:");
        ULOG_WARN_F(" service: %s", service);
        ULOG_WARN_F(" obj. path: %s", object_path);
        ULOG_WARN_F(" interface: %s", interface);
        fprintf(stderr, "\nLibosso WARNING: yet another "
                        "handler registered for:\n");
        fprintf(stderr, " service: %s\n", service);
        fprintf(stderr, " obj. path: %s\n", object_path);
        fprintf(stderr, " interface: %s\n", interface);

        /* add it to the list of handlers */
        old->handlers = g_list_append(old->handlers, handler);

    } else {
        _osso_hash_value_t *new_elem;
        char *new_key;

        /* we need to allocate a new hash table element */
        new_elem = calloc(1, sizeof(_osso_hash_value_t));
        if (new_elem == NULL) {
            ULOG_ERR_F("calloc() failed");
            free(handler);
            return FALSE;
        }

        new_key = strdup(uniq_key);
        if (new_key == NULL) {
            ULOG_ERR_F("strdup() failed");
            free(handler);
            free(new_elem);
            return FALSE;
        }

        new_elem->handlers = g_list_append(NULL, handler);

        g_hash_table_insert(osso->uniq_hash, new_key, new_elem);
    }
    return add_to_if_hash(osso, handler, interface);
}

void __attribute__ ((visibility("hidden")))
_msg_handler_set_cb_f(osso_context_t *osso,
                      const char *service,
                      const char *object_path,
                      const char *interface,
                      _osso_handler_f *cb,
                      _osso_callback_data_t *data, 
                      gboolean method)
{   
    if (osso == NULL || object_path == NULL
        || interface == NULL || cb == NULL) {
        ULOG_ERR_F("invalid parameters");
	return;
    }

    set_handler_helper(osso, service, object_path, interface,
                       cb, data, method, FALSE);
}

gboolean __attribute__ ((visibility("hidden")))
_muali_set_handler(_muali_context_t *context,
                   _osso_handler_f *handler,
                   _osso_callback_data_t *data, 
                   int handler_id)
{   
    _osso_hash_value_t *old;
    _osso_handler_t *elem;

    assert(context != NULL && handler != NULL && data != NULL);
    assert(handler_id != 0);

    elem = calloc(1, sizeof(_osso_handler_t));
    if (elem == NULL) {
        ULOG_ERR_F("calloc() failed");
        return FALSE;
    }

    elem->handler = handler;
    elem->data = data;
    elem->handler_id = handler_id;
    /* other members are not used and left zero */

    old = g_hash_table_lookup(context->id_hash, &handler_id);
    if (old != NULL) {
        ULOG_DEBUG_F("registering another handler for id %d", handler_id);

        /* add it to the list of handlers */
        old->handlers = g_list_append(old->handlers, elem);

    } else {
        _osso_hash_value_t *new_elem;

        ULOG_DEBUG_F("registering first handler for id %d", handler_id);

        /* we need to allocate a new hash table element */
        new_elem = calloc(1, sizeof(_osso_hash_value_t));
        if (new_elem == NULL) {
            ULOG_ERR_F("calloc() failed");
            free(elem);
            return FALSE;
        }

        new_elem->handlers = g_list_append(NULL, elem);

        g_hash_table_insert(context->id_hash, &handler_id, new_elem);
    }

    return add_to_if_hash(context, elem, data->interface);
}

void __attribute__ ((visibility("hidden")))
_msg_handler_set_cb_f_free_data(osso_context_t *osso,
                                const gchar *service,
                                const gchar *object_path,
                                const gchar *interface,
                                _osso_handler_f *cb,
                                _osso_callback_data_t *data, 
                                gboolean method)
{   
    if (osso == NULL || object_path == NULL
        || interface == NULL || cb == NULL) {
        ULOG_ERR_F("invalid parameters");
	return;
    }

    set_handler_helper(osso, service, object_path, interface,
                       cb, data, method, TRUE);
}

static inline gboolean data_matches(const _osso_handler_t *handler,
                                    const _osso_callback_data_t *data)
{
    if (data == NULL) {
        return TRUE;
    } else {
        const _osso_callback_data_t *cb_data;
        cb_data = handler->data;

        if (cb_data->user_cb == data->user_cb
            && cb_data->user_data == data->user_data) {
            return TRUE;
        }
    }
    return FALSE;
}

/************************************************************************/
gboolean __attribute__ ((visibility("hidden")))
_msg_handler_rm_cb_f(osso_context_t *osso,
                     const gchar *service,
                     const gchar *object_path,
                     const gchar *interface,
                     const _osso_handler_f *cb,
                     const _osso_callback_data_t *data,
                     gboolean method)
{   
    char uniq_key[MAX_HASH_KEY_LEN + 1];
    _osso_hash_value_t *elem;
    const _osso_handler_t *matched_handler = NULL;
    gboolean ret = FALSE;

    if (osso == NULL || object_path == NULL || interface == NULL
        || cb == NULL) {
        ULOG_DEBUG_F("invalid parameters");
	return FALSE;
    }

    compose_hash_key(service, object_path, interface, uniq_key);

    elem = g_hash_table_lookup(osso->uniq_hash, uniq_key);
    if (elem != NULL) {
        GList *list;

        list = g_list_first(elem->handlers);
        while (list != NULL) {
            _osso_handler_t *handler;

            handler = list->data;

            if (handler->method == method && handler->handler == cb) {
                if (data_matches(handler, data)) {
                    ULOG_DEBUG_F("found from uniq_hash");
                    ret = TRUE;

                    elem->handlers = g_list_remove_link(elem->handlers,
                                                        list);
                    free_handler(handler, NULL);
                    g_list_free(list); /* free the removed link */

                    /* if this was the last element in the list, free the
                     * list and the hash element */
                    if (g_list_length(elem->handlers) == 0) {
                        g_hash_table_remove(osso->uniq_hash, interface);
                    }
                    matched_handler = handler;
                    break;
                }
            }
            list = g_list_next(list);
        }
    }

    if (matched_handler != NULL) {
        elem = g_hash_table_lookup(osso->if_hash, interface);
        if (elem != NULL) {
            GList *list;

            list = g_list_first(elem->handlers);
            while (list != NULL) {
                if (list->data == matched_handler) {
                    ULOG_DEBUG_F("found from if_hash");
                    elem->handlers = g_list_remove_link(elem->handlers,
                                                        list);
                    g_list_free(list); /* free the removed link */

                    /* if this was the last element in the list, free the
                     * list and the hash element */
                    if (g_list_length(elem->handlers) == 0) {
                        g_hash_table_remove(osso->if_hash, interface);
                    }
                    return TRUE;
                }
                list = g_list_next(list);
            }
        }
    }

#if 0
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
#endif

    ULOG_DEBUG_F("WARNING: tried to remove non-existent handler for:");
    ULOG_DEBUG_F(" service: %s", service);
    ULOG_DEBUG_F(" obj. path: %s", object_path);
    ULOG_DEBUG_F(" interface: %s", interface);
    return ret;
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
