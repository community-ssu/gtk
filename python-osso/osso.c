/**
 * osso.c
 * Python bindings for libosso components.
 *
 * Copyright (C) 2005-2006 Nokia Corporation.
 *
 * Contact: Osvaldo Santana Neto <osvaldo.santana@indt.org.br>
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

#include <Python.h>
#include <time.h>
#include <glib.h>
#include <libosso.h>

static PyObject *OssoException;
static PyObject *OssoRPCException;
static PyObject *OssoInvalidException;
static PyObject *OssoNameException;
static PyObject *OssoNoStateException;
static PyObject *OssoStateSizeException;

typedef struct {
	PyObject *func;
	PyObject *user_data;
} Callback_wrapper;

/*****************************************************************************/
/* WARNING! You'll see a big hack here...                                    */
/*                                                                           */
/* This hack is here because is very difficult to treat with variable        */
/* argument functions in C. There is no easy, or portable, way to generate   */
/* a va_list structure from a Python tuple.                                  */
/*                                                                           */
/* I'll suggest the functions below to be added in next libosso releases.    */
/*****************************************************************************/

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

#define OSSO_BUS_ROOT	   "com.nokia"
#define OSSO_BUS_ROOT_PATH  "/com/nokia"
#define TASK_NAV_SERVICE					"com.nokia.tasknav"
#define APP_LAUNCH_BANNER_METHOD_INTERFACE  "com.nokia.tasknav.app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD_PATH	   "/com/nokia/tasknav/app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD			"app_launch_banner"

#define MAX_IF_LEN 255
#define MAX_SVC_LEN 255
#define MAX_OP_LEN 255

typedef struct {
	osso_rpc_async_f *func;
	gpointer data;
	gchar *interface;
	gchar *method;
} _osso_rpc_async_t;

static gchar *
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

static void
_get_arg(DBusMessageIter *iter, osso_rpc_t *retval)
{
	char *str;
	retval->type = dbus_message_iter_get_arg_type(iter);
	switch (retval->type) {
		case DBUS_TYPE_INT32:
			retval->value.i = dbus_message_iter_get_int32(iter);
			break;
		case DBUS_TYPE_UINT32:
			retval->value.u = dbus_message_iter_get_uint32(iter);
			break;
		case DBUS_TYPE_BOOLEAN:
			retval->value.b = dbus_message_iter_get_boolean(iter);
			break;
		case DBUS_TYPE_DOUBLE:
			retval->value.d = dbus_message_iter_get_double(iter);
			break;
		case DBUS_TYPE_STRING:
			str = dbus_message_iter_get_string(iter);
			retval->value.s = g_strdup(str);
			dbus_free (str);
			if (retval->value.s == NULL) {
				retval->type = DBUS_TYPE_INVALID;
			}
			break;
		case DBUS_TYPE_NIL:
			retval->value.s = NULL;		
			break;
		default:
			retval->type = DBUS_TYPE_INVALID;
			retval->value.i = 0;		
			break;	
	}
}

osso_return_t
_osso_rpc_run(osso_context_t *osso,
		const gchar *service, const gchar *object_path, const gchar *interface, const gchar *method,
		osso_rpc_t *retval, GArray *rpc_args)
{
	DBusMessage *msg;
	DBusConnection *conn = NULL;
	gint timeout;
	int i;
	osso_rpc_t *rpc_arg;

	/* validate parameters */
	if ((osso == NULL) || (service == NULL) || (object_path == NULL) ||
			(interface == NULL) || (method == NULL)) {
		return OSSO_INVALID;
	}
	
	/* get connection */
	conn = (DBusConnection *)osso_get_dbus_connection(osso);
	if (conn == NULL) {
		return OSSO_INVALID;
	}

	/* get rpc timeout */
	if (osso_rpc_get_timeout(osso, &timeout) != OSSO_OK) {
		return OSSO_INVALID;
	}

	/* get message object */
	msg = dbus_message_new_method_call(service, object_path, interface, method);
	if (msg == NULL) {
		return OSSO_ERROR;
	}

	/* add arguments to call if exists */
	if (rpc_args != NULL) {
		for (i = 0; i < rpc_args->len; i++) {
			rpc_arg = g_array_index(rpc_args, osso_rpc_t *, i);
			dbus_message_append_args(msg, rpc_arg->type, &rpc_arg->value, DBUS_TYPE_INVALID);
		}
	}

	dbus_message_set_auto_activation(msg, TRUE);
	
	if (retval == NULL) {

		dbus_message_set_no_reply(msg, TRUE);
		if (dbus_connection_send(conn, msg, NULL)) {
			dbus_connection_flush(conn);
			dbus_message_unref(msg);
	
			/* Tell TaskNavigator to show "launch banner" */
			msg = dbus_message_new_method_call(
					TASK_NAV_SERVICE,
					APP_LAUNCH_BANNER_METHOD_PATH,
					APP_LAUNCH_BANNER_METHOD_INTERFACE,
					APP_LAUNCH_BANNER_METHOD);
   
			if (msg != NULL) {
				dbus_message_append_args(msg, DBUS_TYPE_STRING, service, DBUS_TYPE_INVALID);
				if (dbus_connection_send(conn, msg, NULL)) {
					dbus_connection_flush(conn);
				}
				dbus_message_unref(msg);
			}
			return OSSO_OK;
		} else {
			dbus_message_unref(msg);
			return OSSO_ERROR;
		}

	} else {

		DBusError err;
		DBusMessage *ret;
		dbus_error_init(&err);
		ret = dbus_connection_send_with_reply_and_block(conn, msg, timeout, &err);
		dbus_message_unref(msg);
		if (!ret) {
			retval->type = DBUS_TYPE_STRING;
			retval->value.s = g_strdup(err.message);
			dbus_error_free(&err);
			return OSSO_RPC_ERROR;
		} else {
			DBusMessageIter iter;
			DBusError err;
			switch(dbus_message_get_type(ret)) {
				case DBUS_MESSAGE_TYPE_ERROR:
					dbus_set_error_from_message(&err, ret);
					retval->type = DBUS_TYPE_STRING;
					retval->value.s = g_strdup(err.message);
					dbus_error_free(&err);
					return OSSO_RPC_ERROR;
				case DBUS_MESSAGE_TYPE_METHOD_RETURN:
					dbus_message_iter_init(ret, &iter);
					_get_arg(&iter, retval);
					dbus_message_unref(ret);
	
					/* Tell TaskNavigator to show "launch banner" */
					msg = dbus_message_new_method_call(
							TASK_NAV_SERVICE,
							APP_LAUNCH_BANNER_METHOD_PATH,
							APP_LAUNCH_BANNER_METHOD_INTERFACE,
							APP_LAUNCH_BANNER_METHOD
						);

					if (msg != NULL) {
						dbus_message_append_args(msg, DBUS_TYPE_STRING, service, DBUS_TYPE_INVALID);
						if (dbus_connection_send(conn, msg, NULL)) {
							dbus_connection_flush(conn);
						}
						dbus_message_unref(msg);
					}
					return OSSO_OK;
				default:
					retval->type = DBUS_TYPE_STRING;
					retval->value.s = g_strdup("Invalid return value");
					return OSSO_RPC_ERROR;
			}
		}
	}
}

static void
_async_return_handler(DBusPendingCall *pending, void *data)
{
	DBusMessage *msg;
	_osso_rpc_async_t *rpc;
	osso_rpc_t retval;
	int type;
	
	rpc = (_osso_rpc_async_t *)data;

	msg = dbus_pending_call_get_reply(pending);
	if (msg == NULL) {
		g_free(rpc->interface);
		g_free(rpc->method);
		g_free(rpc);
		return;
	}

	type = dbus_message_get_type(msg);
	if (type == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
		DBusMessageIter iter;
		dbus_message_iter_init(msg, &iter);
		_get_arg(&iter, &retval);
		(rpc->func)(rpc->interface, rpc->method, &retval, rpc->data);
	} else if (type == DBUS_MESSAGE_TYPE_ERROR) {
		DBusError err;
		dbus_error_init(&err);
		dbus_set_error_from_message(&err, msg);
		dbus_error_free(&err);
	}

	dbus_message_unref(msg);
	g_free(rpc->interface);
	g_free(rpc->method);
	g_free(rpc);
	return;
}

static osso_return_t 
_osso_rpc_async_run(osso_context_t *osso,
		const gchar *service, const gchar *object_path, const gchar *interface, const gchar *method,
		osso_rpc_async_f *async_cb, gpointer data,
		osso_rpc_t *retval, GArray *rpc_args)
{
	DBusMessage *msg;
	DBusPendingCall *pending;
	DBusConnection *conn;
	dbus_bool_t b;
	gint timeout;
	_osso_rpc_async_t *rpc = NULL;

	int i;
	osso_rpc_t *rpc_arg;
   
	if ((osso == NULL) || (service == NULL) || (object_path == NULL) ||
			(interface == NULL) || (method == NULL)) {
		return OSSO_INVALID;
	}

	/* get connection */
	conn = (DBusConnection *)osso_get_dbus_connection(osso);
	if (conn == NULL) {
		return OSSO_INVALID;
	}

	/* get rpc timeout */
	if (osso_rpc_get_timeout(osso, &timeout) != OSSO_OK) {
		return OSSO_INVALID;
	}


	msg = dbus_message_new_method_call(service, object_path, interface, method);
	if (msg == NULL) {
		return OSSO_ERROR;
	}

	dbus_message_set_auto_activation(msg, TRUE);
	
	/* add arguments to call if exists */
	if (rpc_args != NULL) {
		for (i = 0; i < rpc_args->len; i++) {
			rpc_arg = g_array_index(rpc_args, osso_rpc_t *, i);
			dbus_message_append_args(msg, rpc_arg->type, &rpc_arg->value, DBUS_TYPE_INVALID);
		}
	}

	if (async_cb == NULL) {
		dbus_message_set_no_reply(msg, TRUE);
		b = dbus_connection_send(conn, msg, NULL);
	} else {
		rpc = (_osso_rpc_async_t *)calloc(1, sizeof(_osso_rpc_async_t));
		if (rpc == NULL) {
			return OSSO_ERROR;
		} 
		rpc->func = async_cb;
		rpc->data = data;
		rpc->interface = g_strdup(interface);
		rpc->method = g_strdup(method);
		b = dbus_connection_send_with_reply(conn, msg, &pending, timeout);
	}

	if (b) {
		if (async_cb != NULL) {
			dbus_pending_call_set_notify(pending, _async_return_handler, rpc, NULL);
		}
		dbus_connection_flush(conn);
		dbus_message_unref(msg);

		/* Tell TaskNavigator to show "launch banner" */
		msg = dbus_message_new_method_call(
				TASK_NAV_SERVICE,
				APP_LAUNCH_BANNER_METHOD_PATH,
				APP_LAUNCH_BANNER_METHOD_INTERFACE,
				APP_LAUNCH_BANNER_METHOD
			);

		if (msg != NULL) {
			dbus_message_append_args(msg, DBUS_TYPE_STRING, service, DBUS_TYPE_INVALID);
			b = dbus_connection_send(conn, msg, NULL);
			if (b) {
				dbus_connection_flush(conn);
			}
			dbus_message_unref(msg);
		}
		return OSSO_OK;
	} else {
		dbus_message_unref(msg);
		return OSSO_ERROR;
	}
}
/****************************************************************************/
/* Ugly hack finished!                                                      */
/****************************************************************************/

/* helper functions */
static void
_set_exception(osso_return_t err)
{
	switch (err) {
		case OSSO_ERROR:
			PyErr_SetString(OssoException, "OSSO error.");
			break;
		case OSSO_INVALID:
			PyErr_SetString(OssoInvalidException, "Invalid parameter.");
			break;
		case OSSO_RPC_ERROR:
			PyErr_SetString(OssoRPCException, "Error in RPC method call.");
			break;
		case OSSO_ERROR_NAME:
			PyErr_SetString(OssoNameException, "Invalid name.");
			break;
		case OSSO_ERROR_NO_STATE:
			PyErr_SetString(OssoNoStateException, "No state file found to read.");
			break;
		case OSSO_ERROR_STATE_SIZE:
			PyErr_SetString(OssoStateSizeException, "Inconsistent state file size.");
			break;
		default:
			PyErr_SetString(OssoException, "OSSO error."); break;
	}
	return;
}

static PyObject *
_rpc_t_to_python(const osso_rpc_t *arg)
{
	PyObject *py_arg;

	switch (arg->type) {
		case DBUS_TYPE_BOOLEAN:
			py_arg = Py_BuildValue("b", arg->value.b);
			break;
		case DBUS_TYPE_DOUBLE:
			py_arg = Py_BuildValue("d", arg->value.d);
			break;
		case DBUS_TYPE_INT32:
			py_arg = Py_BuildValue("i", arg->value.i);
			break;
		case DBUS_TYPE_UINT32:
			py_arg = Py_BuildValue("i", arg->value.u);
			break;
		case DBUS_TYPE_STRING:
			if (arg->value.s == NULL) {
				py_arg = Py_None;
			} else {
				py_arg = Py_BuildValue("s", arg->value.s);
			}
			break;
		default:
			py_arg = Py_None;
	}
	Py_INCREF(py_arg);
	return py_arg;
}

static void
_python_to_rpc_t(PyObject *py_arg, osso_rpc_t *rpc_arg)
{
	long long_arg;

	if (PyString_CheckExact(py_arg)) {
		rpc_arg->type = DBUS_TYPE_STRING;
		rpc_arg->value.s = PyString_AsString(py_arg);
	} else if (PyInt_CheckExact(py_arg)) {
		long_arg = PyInt_AsLong(py_arg);
		if (long_arg < 0) {
			rpc_arg->type = DBUS_TYPE_UINT32;
			rpc_arg->value.u = long_arg;
		} else {
			rpc_arg->type = DBUS_TYPE_INT32;
			rpc_arg->value.i = long_arg;
		}
	} else if (PyBool_Check(py_arg)) { 
		rpc_arg->type = DBUS_TYPE_BOOLEAN;
		rpc_arg->value.b = PyObject_IsTrue(py_arg);
	} else if (PyFloat_CheckExact(py_arg)) { 
		rpc_arg->type = DBUS_TYPE_DOUBLE;
		rpc_arg->value.d = PyFloat_AsDouble(py_arg);
	} else if (py_arg == Py_None) {
		rpc_arg->type = DBUS_TYPE_STRING;
		rpc_arg->value.s = NULL;
	} else {
		rpc_arg->type = DBUS_TYPE_INVALID;
		rpc_arg->value.i = 0;
	}
}

static GArray *
_rpc_args_py_to_c(PyObject *py_args)
{
	int i;
	int size;
	GArray *rpc_args;
	PyObject *py_arg;
	osso_rpc_t *rpc_arg;

	if (!PyTuple_Check(py_args)) {
		return NULL;
	}

	size = PyTuple_Size(py_args);
	rpc_args = g_array_sized_new(FALSE, FALSE, sizeof(osso_rpc_t *), size);

	for (i = 0; i < size; i++) {
		rpc_arg = (osso_rpc_t *)g_malloc(sizeof(osso_rpc_t));
		py_arg = PyTuple_GetItem(py_args, i);

		_python_to_rpc_t(py_arg, rpc_arg);

		g_array_append_val(rpc_args, rpc_arg);
	}
	return rpc_args;
}

static PyObject *
_rpc_args_c_to_py(GArray *args) {
	int i;
	int size;
	PyObject *ret;
	osso_rpc_t *arg;
	PyObject *py_arg;
	
	size = args->len;

	ret = PyTuple_New(size);
	if (ret == NULL) {
		return NULL;
	}

	for (i = 0; i < size; i++) {
		arg = g_array_index(args, osso_rpc_t *, i);
		py_arg = _rpc_t_to_python(arg);
		PyTuple_SetItem(ret, i, py_arg);
	}

	return ret;
}

static void _release_rpc_args(GArray *rpc_args)
{
	int i;

	for (i = 0; i < rpc_args->len; i++) {
		g_free(g_array_index(rpc_args, osso_rpc_t *, i));
	}
	g_array_free(rpc_args, TRUE);

	return;
}

static char _check_context(osso_context_t *context)
{
	if (context == NULL) {
		PyErr_SetString(OssoException, "Context invalid. Not initialized.");
		return FALSE;
	}
	return TRUE;
}
/* / helper functions */

/* Type: Context */
typedef struct {
	PyObject_HEAD
	osso_context_t *context;
} Context;


/* Context method */
static void
Context_dealloc(Context *self)
{
	osso_deinitialize(self->context);
	return;
}


static PyObject *
Context_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Context *self;

	self = (Context *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->context = NULL;
	}

	return (PyObject *)self;
}


static int
Context_init(Context *self, PyObject *args, PyObject *kwds)
{
	char *application;
	char *version;
	char activation;

	static char *kwlist[] = { "application", "version", "activation", 0 };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "ssi", kwlist,
				&application, &version, &activation)) {
		return -1;
	}
	
	self->context = osso_initialize(application, version, activation, 0);

	if (self->context == NULL) {
		PyErr_SetString(OssoException, "Cannot initialize context.");
		return -1;
	}

	return 0;
}


static PyObject *
Context_get_name(Context *self)
{
	const char *name;

	if (!_check_context(self->context)) return 0;

	name = osso_application_name_get(self->context);
	return Py_BuildValue("s", name);
}

static PyObject *
Context_get_version(Context *self)
{
	const char *version;

	if (!_check_context(self->context)) return 0;

	version = osso_application_version_get(self->context);
	return Py_BuildValue("s", version);
}


static PyObject *
Context_statusbar_send_event(Context *self, PyObject *args, PyObject *kwds)
{
	const gchar *name;
	gint arg1;
	gint arg2;
	const gchar *arg3;
	osso_return_t ret;
	osso_rpc_t retval;

	static char *kwlist[] = {"name", "arg1", "arg2", "arg3", 0};

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "siis:Context.statusbar_send_event", kwlist, &name, &arg1, &arg2, &arg3)) {
		return NULL;
	}

	ret = osso_statusbar_send_event(self->context, name, arg1, arg2, arg3, &retval);
	if (ret != OSSO_OK) {
		_set_exception(ret);
		return NULL;
	}

	return _rpc_t_to_python(&retval);
}


static PyObject *
Context_rpc_run(Context *self, PyObject *args, PyObject *kwds)
{
	char *service = NULL;
	char *object_path = NULL;
	char *interface = NULL;
	char *method = NULL;
	char wait_reply = FALSE;
	PyObject *py_rpc_args = NULL;
	GArray *rpc_args;
	osso_rpc_t retval;
	osso_return_t ret;

	static char *kwlist[] = { "service", "object_path", "interface", "method", "rpc_args", "wait_reply", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "ssss|Ob:Context.rpc_run", kwlist,
				&service, &object_path, &interface, &method, &py_rpc_args, &wait_reply)) {
		return NULL;
	}

	/* rpc_args */
	if (py_rpc_args != NULL) {
		if (!PyTuple_Check(py_rpc_args)) {
			PyErr_SetString(PyExc_TypeError, "RPC arguments must be in a tuple.");
			return NULL;
		}
	} else {
		py_rpc_args = PyTuple_New(0);
	}
	rpc_args = _rpc_args_py_to_c(py_rpc_args);

	if (wait_reply) {
		ret = _osso_rpc_run(self->context, service, object_path, interface, method, &retval, rpc_args);
	} else {
		ret = _osso_rpc_run(self->context, service, object_path, interface, method, NULL, rpc_args);
	}
	_release_rpc_args(rpc_args);

	if (ret != OSSO_OK) {
		_set_exception(ret);
		return NULL;
	}

	if (wait_reply) {
		return _rpc_t_to_python(&retval);
	}
	Py_RETURN_NONE;
}


static PyObject *
Context_rpc_run_with_defaults(Context *self, PyObject *args, PyObject *kwds)
{
	char *application;
	char *method;
    char service[MAX_SVC_LEN] = {0};
	char object_path[MAX_OP_LEN] = {0};
	char interface[MAX_IF_LEN] = {0};
	char wait_reply = FALSE;
	char *copy = NULL;
	PyObject *py_rpc_args = NULL;
	GArray *rpc_args;
	osso_rpc_t retval;
	osso_return_t ret;

	static char *kwlist[] = { "application", "method", "rpc_args", "wait_reply", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "ss|Ob:Context.run_with_defaults", kwlist,
				&application, &method, &py_rpc_args, &wait_reply)) {
		return NULL;
	}

	/* rpc_args */
	if (py_rpc_args != NULL) {
		if (!PyTuple_Check(py_rpc_args)) {
			PyErr_SetString(PyExc_TypeError, "RPC arguments must be in a tuple.");
			return NULL;
		}
	} else {
		py_rpc_args = PyTuple_New(0);
	}
	rpc_args = _rpc_args_py_to_c(py_rpc_args);

	/* make default arguments */
    g_snprintf(service, MAX_SVC_LEN, OSSO_BUS_ROOT ".%s", application);
    copy = appname_to_valid_path_component(application);
    if (copy == NULL) {
		 PyErr_SetString(OssoException, "Invalid application.");
		 return NULL;
    }
    g_snprintf(object_path, MAX_OP_LEN, OSSO_BUS_ROOT_PATH "/%s", copy);
    g_free(copy);
	copy = NULL;
    g_snprintf(interface, MAX_IF_LEN, "%s", service);

	if (wait_reply) {
		ret = _osso_rpc_run(self->context, service, object_path, interface, method, &retval, rpc_args);
	} else {
		ret = _osso_rpc_run(self->context, service, object_path, interface, method, NULL, rpc_args);
	}
	_release_rpc_args(rpc_args);

	if (ret != OSSO_OK) {
		_set_exception(ret);
		return NULL;
	}

	if (wait_reply) {
		return _rpc_t_to_python(&retval);
	}
	Py_RETURN_NONE;
}


static void
_wrap_rpc_async_handler(const gchar *interface, const gchar *method, osso_rpc_t *retval, gpointer data)
{
	PyObject *py_interface;
	PyObject *py_method;
	PyObject *py_ret;
	Callback_wrapper *handler = data;
	PyGILState_STATE state;

	state = PyGILState_Ensure();

	if (handler->func == NULL) {
		return;
	}
	
	py_interface = Py_BuildValue("s", interface);
	py_method = Py_BuildValue("s", method);

	if (handler->user_data) {
		py_ret = PyEval_CallFunction(handler->func, "(ssO)", py_interface, py_method, handler->user_data);
	} else {
		py_ret = PyEval_CallFunction(handler->func, "(ss)", py_interface, py_method);
	}

	if (py_ret == NULL || py_ret == Py_None) {
		PyGILState_Release(state);
		return;
	}

	_python_to_rpc_t(py_ret, retval);

	PyGILState_Release(state);
	return;
}


static PyObject *
Context_rpc_async_run(Context *self, PyObject *args, PyObject *kwds)
{
	char *service;
	char *object_path;
	char *interface;
	char *method;
	PyObject *py_func;
	PyObject *py_data;
	PyObject *py_rpc_args = NULL;

	Callback_wrapper callback;
	GArray *rpc_args;
	osso_rpc_t retval;
	osso_return_t ret;

	static char *kwlist[] = { "service", "object_path", "interface", "method", "callback", "user_data", "rpc_args", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "ssssO|OO:Context.rpc_async_run", kwlist,
				&service, &object_path, &interface, &method, &py_func, &py_data, &py_rpc_args)) {
		return NULL;
	}

	/* py_func / py_data */
	callback.func = py_func;
	callback.user_data = py_data;

	/* py_rpc_args */
	if (py_rpc_args != NULL) {
		if (!PyTuple_Check(py_rpc_args)) {
			PyErr_SetString(PyExc_TypeError, "RPC arguments must be in a tuple.");
			return NULL;
		}
	} else {
		py_rpc_args = PyTuple_New(0);
	}
	rpc_args = _rpc_args_py_to_c(py_rpc_args);

	ret = _osso_rpc_async_run(self->context, service, object_path, interface, method, _wrap_rpc_async_handler, &callback, &retval, rpc_args);
	_release_rpc_args(rpc_args);

	if (ret != OSSO_OK) {
		_set_exception(ret);
		return NULL;
	}

	return _rpc_t_to_python(&retval);
}


static PyObject *
Context_rpc_async_run_with_defaults(Context *self, PyObject *args, PyObject *kwds)
{
	char *application;
	char *method;
	PyObject *py_func;
	PyObject *py_data;
	PyObject *py_rpc_args = NULL;

    char service[MAX_SVC_LEN] = {0};
	char object_path[MAX_OP_LEN] = {0};
	char interface[MAX_IF_LEN] = {0};
	char *copy = NULL;
	Callback_wrapper callback;
	GArray *rpc_args;
	osso_rpc_t retval;
	osso_return_t ret;

	static char *kwlist[] = { "application", "method", "callback", "user_data", "rpc_args", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "ssO|OO:Context.run_async_with_defaults", kwlist,
				&application, &method, &py_func, &py_data, &py_rpc_args)) {
		return NULL;
	}

	/* py_func / py_data */
	callback.func = py_func;
	callback.user_data = py_data;

	if (py_rpc_args != NULL) {
		if (!PyTuple_Check(py_rpc_args)) {
			PyErr_SetString(PyExc_TypeError, "RPC arguments must be in a tuple.");
			return NULL;
		}
	} else {
		py_rpc_args = PyTuple_New(0);
	}
	rpc_args = _rpc_args_py_to_c(py_rpc_args);

	/* make default arguments */
    g_snprintf(service, MAX_SVC_LEN, OSSO_BUS_ROOT ".%s", application);
    copy = appname_to_valid_path_component(application);
    if (copy == NULL) {
		 PyErr_SetString(OssoException, "Invalid application.");
		 return NULL;
    }
    g_snprintf(object_path, MAX_OP_LEN, OSSO_BUS_ROOT_PATH "/%s", copy);
    g_free(copy);
	copy = NULL;
    g_snprintf(interface, MAX_IF_LEN, "%s", service);

	ret = _osso_rpc_async_run(self->context, service, object_path, interface, method, _wrap_rpc_async_handler, &callback, &retval, rpc_args);
	_release_rpc_args(rpc_args);

	if (ret != OSSO_OK) {
		_set_exception(ret);
		return NULL;
	}

	return _rpc_t_to_python(&retval);
}


static gint
_wrap_rpc_callback_handler(const gchar *interface, const gchar *method, GArray *arguments, gpointer data, osso_rpc_t *retval)
{
	PyObject *py_interface;
	PyObject *py_method;
	PyObject *py_arguments;

	Callback_wrapper *handler = data;

	PyGILState_STATE state;
	PyObject *py_ret;

	state = PyGILState_Ensure();

	if (handler->func == NULL) {
		return OSSO_ERROR;
	}
	
	py_interface = Py_BuildValue("s", interface);
	py_method = Py_BuildValue("s", method);
	py_arguments = _rpc_args_c_to_py(arguments);

	if (handler->user_data) {
		py_ret = PyEval_CallFunction(handler->func, "(ssOO)", py_interface, py_method, py_arguments, handler->user_data);
	} else {
		py_ret = PyEval_CallFunction(handler->func, "(ssO)", py_interface, py_method, py_arguments);
	}

	if (py_ret == NULL) {
		retval->type = DBUS_TYPE_STRING;
		retval->value.s = "There is some exceptions in callback.";
		PyGILState_Release(state);
		return OSSO_ERROR;
	}

	_python_to_rpc_t(py_ret, retval);

	PyGILState_Release(state);
	return OSSO_OK;
}


static PyObject *
Context_set_rpc_callback(Context *self, PyObject *args, PyObject *kwds)
{
	const gchar *service;
	const gchar *object_path;
	const gchar *interface;
	PyObject *py_func;
	PyObject *py_data;

	Callback_wrapper callback;
	osso_return_t ret;

	static char *kwlist[] = { "service", "object_path", "interface", "callback", "user_data", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "sssO|O:Context.set_rpc_callback", kwlist, &service, &object_path, &interface, &py_func, &py_data)) {
		return NULL;
	}

	/* py_func / py_data */
	callback.func = py_func;
	callback.user_data = py_data;

	ret = osso_rpc_set_cb_f(self->context, service, object_path, interface, _wrap_rpc_callback_handler, &callback);

	if (ret != OSSO_OK) {
		_set_exception(ret);
		return NULL;
	}

	Py_RETURN_NONE;
}


static PyObject *
Context_close(Context *self)
{
	if (!_check_context(self->context)) return 0;

	Context_dealloc(self);

	Py_RETURN_NONE;
}

static struct PyMethodDef Context_methods[] = {
	{"get_name", (PyCFunction)Context_get_name, METH_NOARGS, "c.get_name() -> string\n\nReturn the application name.\n"},
	{"get_version", (PyCFunction)Context_get_version, METH_NOARGS, "c.get_name() -> string\n\nReturn the application version.\n"},
	{"rpc_run", (PyCFunction)Context_rpc_run, METH_VARARGS | METH_KEYWORDS, \
		"c.rpc_run(service, object_path, interface, method[, rpc_args, wait_reply]) -> object\n"
		"\n"
		"Run a RPC method with arguments inside rpc_args tuple.\n"
		"\n"
		"Usage example:\n\n"
		"\tc.rpc_run('com.nokia.backup', '/com/nokia/backup', 'com.nokia.backup', 'backup_finish', True)\n"},
	{"rpc_run_with_defaults", (PyCFunction)Context_rpc_run_with_defaults, METH_VARARGS | METH_KEYWORDS, \
		"c.rpc_run_with_defaults(application, method[, rpc_args, wait_reply]) -> object\n"
		"\n"
		"Run a RPC method using default parameters.\n"
		"\n"
		"Usage example:\n\n"
		"\tc.rpc_run_with_defaults('tn_mail', 'send_recv_button_focus', (status,)) # status is a int var\n"},
	{"rpc_async_run", (PyCFunction)Context_rpc_async_run, METH_VARARGS | METH_KEYWORDS, \
		"c.rpc_async_run(service, object_path, interface, method, callback, user_data, rpc_args)\n"
		"\n"
		"Run a RPC method and call 'callback' after finished.\n"
		"\n"
		"Usage example:\n\n"
		"\tdef my_func(interface, method, user_data=None):\n"
		"\t\tpass\n\n"
		"\tc.rpc_async_run('com.nokia.backup', '/com/nokia/backup', 'com.nokia.backup', 'backup_finish', my_func, 'hello!', (True,))\n"},
	{"rpc_async_run_with_defaults", (PyCFunction)Context_rpc_async_run_with_defaults, METH_VARARGS | METH_KEYWORDS, \
		"c.rpc_async_run_with_defaults(application, method, callback, user_data, rpc_args)\n"
		"\n"
		"Run a RPC method using default parameters and call 'callback' after finished.\n"
		"\n"
		"Usage example:\n\n"
		"\tdef my_func(interface, method, user_data=None):\n"
		"\t\tpass\n\n"
		"\tc.rpc_async_run_with_defaults('tn_mail', 'send_recv_button_focus', my_func, 'hello!', (True,))\n"},
	{"set_rpc_callback", (PyCFunction)Context_set_rpc_callback, METH_VARARGS | METH_KEYWORDS,
		"This function registers a callback function for handling RPC calls to a given object of a service.\n"},
/*	{"set_rpc_default_callback", (PyCFunction)Context_set_rpc_default_callback, METH_VARARGS | METH_KEYWORDS,
		"This function registers a callback function for handling RPC calls to the\n"
		"default service of the application. The default service is 'com.nokia.A',\n"
		"where A is the application's name as given to osso_initialize.\n"},*/
	{"statusbar_send_event", (PyCFunction)Context_statusbar_send_event, METH_VARARGS | METH_KEYWORDS, "Send an event to statusbar."},
	{"close", (PyCFunction)Context_close, METH_NOARGS, "Close context."},
	{0, 0, 0, 0}
};

static PyTypeObject ContextType = {
	PyObject_HEAD_INIT(NULL)
	0,																/* ob_size */
	"osso.Context",													/* tp_name */
	sizeof(Context),												/* tp_basicsize */
	0,																/* tp_itemsize */
	(destructor)Context_dealloc,									/* tp_dealloc */
	0,																/* tp_print */
	0,																/* tp_getattr */
	0,																/* tp_setattr */
	0,																/* tp_compare */
	0,																/* tp_repr */
	0,																/* tp_as_number */
	0,																/* tp_as_sequence */
	0,																/* tp_as_mapping */
	0,																/* tp_hash */
	0,																/* tp_call */
	0,																/* tp_str */
	0,																/* tp_getattro */
	0,																/* tp_setattro */
	0,																/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_BASETYPE,	/* tp_flags */
	"OSSO Context class",											/* tp_doc */
	0,																/* tp_traverse */
	0,																/* tp_clear */
	0,																/* tp_richcompare */
	0,																/* tp_weaklistoffset */
	0,																/* tp_iter */
	0,																/* tp_iternext */
	Context_methods,												/* tp_methods */
	0,																/* tp_members */
	0,																/* tp_getset */
	0,																/* tp_base */
	0,																/* tp_dict */
	0,																/* tp_descr_get */
	0,																/* tp_descr_set */
	0,																/* tp_dictoffset */
	(initproc)Context_init,											/* tp_init */
	0,																/* tp_alloc */
	Context_new,													/* tp_new */
};

/**
 * XXX: TEST
 */

#include <stdio.h>

PyObject *
test(PyObject *self, PyObject *args)
{
	PyObject *py_test;
	osso_rpc_t rpc_t;
	int i;
	PyObject *tuple_args;
	osso_rpc_t *rpc_t_p;
	GArray *rpc_args;

	rpc_t.type = DBUS_TYPE_INT32;
	rpc_t.value.i = 25;
	py_test = _rpc_t_to_python(&rpc_t);
	printf("_rpc_t_to_python: %ld\n", PyInt_AsLong(py_test));

	rpc_t.value.i = -25;
	py_test = _rpc_t_to_python(&rpc_t);
	printf("_rpc_t_to_python: %ld\n", PyInt_AsLong(py_test));

	rpc_t.type = DBUS_TYPE_STRING;
	rpc_t.value.s = "Test 123...";
	py_test = _rpc_t_to_python(&rpc_t);
	printf("_rpc_t_to_python: %s\n", PyString_AsString(py_test));

	rpc_t.value.s = NULL;
	py_test = _rpc_t_to_python(&rpc_t);
	if (py_test == Py_None) {
		printf("_rpc_t_to_python: OK. invalid.\n");
	} else {
		printf("_rpc_t_to_python: ERROR\n");
	}

	rpc_args = _rpc_args_py_to_c(args);
	for (i = 0; i < rpc_args->len; i++) {
		rpc_t_p = g_array_index(rpc_args, osso_rpc_t *, i);
		printf("rpc_type: %d\n", rpc_t_p->type);
	}

	tuple_args = _rpc_args_c_to_py(rpc_args);
	PyObject_Print(tuple_args, stdout, 0);
	printf("\n");
	_release_rpc_args(rpc_args);

	Py_RETURN_NONE;
}

static struct PyMethodDef osso_methods[] = {
	{"test", (PyCFunction)test, METH_VARARGS, "Test Module."},
	{0, 0, 0, 0}
};

PyMODINIT_FUNC
initosso(void)
{
	PyObject *module;

	/* prepare types */
	ContextType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&ContextType) < 0) {
		return;
	}

	/* initialize module */
	module = Py_InitModule3("osso", osso_methods, NULL);

	/* add exceptions */
	OssoException = PyErr_NewException("osso.OssoException", 0, 0);
	OssoRPCException = PyErr_NewException("osso.OssoRPCException", OssoException, 0);
	OssoInvalidException = PyErr_NewException("osso.OssoInvalidException", OssoException, 0);
	OssoNameException = PyErr_NewException("osso.OssoNameException", OssoException, 0);
	OssoNoStateException = PyErr_NewException("osso.OssoNoStateException", OssoException, 0);
	OssoStateSizeException = PyErr_NewException("osso.OssoStateSizeException", OssoException, 0);

	Py_INCREF(OssoException);
	Py_INCREF(OssoRPCException);
	Py_INCREF(OssoInvalidException);
	Py_INCREF(OssoNameException);
	Py_INCREF(OssoNoStateException);
	Py_INCREF(OssoStateSizeException);

	PyModule_AddObject(module, "OssoException", OssoException);
	PyModule_AddObject(module, "OssoRPCException", OssoRPCException);
	PyModule_AddObject(module, "OssoInvalidException", OssoInvalidException);
	PyModule_AddObject(module, "OssoNameException", OssoNameException);
	PyModule_AddObject(module, "OssoNoStateException", OssoNoStateException);
	PyModule_AddObject(module, "OssoStateSizeException", OssoStateSizeException);

	/* add types */
	Py_INCREF(&ContextType);
	PyModule_AddObject(module, "Context", (PyObject *)&ContextType);

	/* add contants */
	/* : */
	/* : */
	/* : */
}


/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch
 */
