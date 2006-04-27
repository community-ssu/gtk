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
#include <marshal.h>
#include <time.h>
#include <libosso.h>
#include <pygtk/pygtk.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

static PyObject *OssoException;
static PyObject *OssoRPCException;
static PyObject *OssoInvalidException;
static PyObject *OssoNameException;
static PyObject *OssoNoStateException;
static PyObject *OssoStateSizeException;

static PyObject *set_rpc_callback = NULL;
static PyObject *rpc_async_callback = NULL;
static PyObject *autosave_callback = NULL;
static PyObject *time_notification_callback = NULL;
static PyObject *device_state_callback = NULL;
static PyObject *mime_callback = NULL;
static PyObject *exit_callback = NULL;

/* Default values for .._with_defaults functions */
#define MAX_IF_LEN 255
#define MAX_SVC_LEN 255
#define MAX_OP_LEN 255
#define OSSO_BUS_ROOT      "com.nokia"
#define OSSO_BUS_ROOT_PATH "/com/nokia"

/* ----------------------------------------------- */
/* Helper functions                                */
/* ----------------------------------------------- */

char *
appname_to_valid_path_component(char *application)
{
    char *copy = NULL;
    char *p = NULL;

    if (application == NULL)
		return NULL;

    copy = g_strdup(application);

    if (copy == NULL)
		return NULL;

    for (p = g_strrstr(copy, "."); p != NULL; p = g_strrstr(p + 1, ".")) {
        *p = '/';
    }
    return copy;
}

static void
_set_exception(osso_return_t err, osso_rpc_t *retval)
{
	char *err_msg = NULL;
	if ((retval != NULL) && (retval->type == DBUS_TYPE_STRING) && (retval->value.s != NULL)) {
		err_msg = strdupa(retval->value.s);
	}

	switch (err) {
		case OSSO_ERROR:
			PyErr_SetString(OssoException, ((err_msg != NULL) ? err_msg : "OSSO error."));
			break;
		case OSSO_INVALID:
			PyErr_SetString(OssoInvalidException, ((err_msg != NULL) ? err_msg : "Invalid parameter."));
			break;
		case OSSO_RPC_ERROR:
			PyErr_SetString(OssoRPCException, ((err_msg != NULL) ? err_msg : "Error in RPC method call."));
			break;
		case OSSO_ERROR_NAME:
			PyErr_SetString(OssoNameException, ((err_msg != NULL) ? err_msg : "Invalid name."));
			break;
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
_rpc_t_to_python(osso_rpc_t *arg)
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

static PyObject *
_rpc_args_c_to_py(GArray *args)
{
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

static void
_argfill(DBusMessage *msg, void *raw_tuple)
{
	osso_rpc_t arg;
	int count;
	int size;
	PyObject *tuple;
	PyObject *py_arg;
	
	tuple = (PyObject *)raw_tuple;
	size = PyTuple_Size(tuple);
	for (count = 0; count < size; count++) {
		py_arg = PyTuple_GetItem(tuple, count);
		_python_to_rpc_t(py_arg, &arg);
		dbus_message_append_args(msg, arg.type, &arg.value, DBUS_TYPE_INVALID);
	}
}

static char _check_context(osso_context_t *context)
{
	if (context == NULL) {
		PyErr_SetString(OssoException, "Context invalid. Not initialized.");
		return FALSE;
	}
	return TRUE;
}


/* ----------------------------------------------- */
/* Context                                         */
/* ----------------------------------------------- */

typedef struct {
	PyObject_HEAD
	osso_context_t *context;
} Context;

/* ----------------------------------------------- */
/* Context type default methods                    */
/* ----------------------------------------------- */

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
Context_close(Context *self)
{
	if (!_check_context(self->context)) return 0;
	osso_deinitialize(self->context);
	self->context = NULL;
	Py_RETURN_NONE;
}


static void
Context_dealloc(Context *self)
{
	if (self->context == NULL)
		return;

	osso_deinitialize(self->context);
	self->context = NULL;
	return;
}


/* ----------------------------------------------- */
/* RPC (done)                                      */
/* ----------------------------------------------- */

static PyObject *
Context_rpc_run(Context *self, PyObject *args, PyObject *kwds)
{
	char *service = NULL;
	char *object_path = NULL;
	char *interface = NULL;
	char *method = NULL;
	char wait_reply = FALSE;
	char use_system_bus = FALSE;
	PyObject *py_rpc_args = NULL;
	osso_rpc_t retval;
	osso_return_t ret;

	static char *kwlist[] = { "service", "object_path", "interface", "method",
								"rpc_args", "wait_reply", "use_system_bus", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "ssss|Obb:Context.rpc_run",
				kwlist, &service, &object_path, &interface, &method,
				&py_rpc_args, &wait_reply, &use_system_bus)) {
		return NULL;
	}

	if (py_rpc_args != NULL) {
		if (!PyTuple_Check(py_rpc_args)) {
			PyErr_SetString(PyExc_TypeError,
								"RPC arguments must be in a tuple.");
			return NULL;
		}
	} else {
		py_rpc_args = PyTuple_New(0);
	}

	if (use_system_bus) {
		ret = osso_rpc_run_system_with_argfill(self->context,
					service,
					object_path,
					interface,
					method,
					(wait_reply ?  &retval : NULL),
					_argfill,
					py_rpc_args);
	} else {
		ret = osso_rpc_run_with_argfill(self->context, service, object_path,
				interface, method, (wait_reply ? &retval : NULL), _argfill,
				py_rpc_args);
	}

	if (ret != OSSO_OK) {
		_set_exception(ret, ((wait_reply) ? &retval : NULL));
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
	char use_system_bus = FALSE;
	char *copy = NULL;
	PyObject *py_rpc_args = NULL;
	osso_rpc_t retval;
	osso_return_t ret;

	static char *kwlist[] = { "application", "method", "rpc_args",
								"wait_reply", "use_system_bus", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"ss|Ob:Context.run_with_defaults", kwlist, &application,
				&method, &py_rpc_args, &wait_reply, &use_system_bus)) {
		return NULL;
	}

	if (py_rpc_args != NULL) {
		if (!PyTuple_Check(py_rpc_args)) {
			PyErr_SetString(PyExc_TypeError,
								"RPC arguments must be in a tuple.");
			return NULL;
		}
	} else {
		py_rpc_args = PyTuple_New(0);
	}

	g_snprintf(service, MAX_SVC_LEN, OSSO_BUS_ROOT ".%s", application);
    copy = appname_to_valid_path_component(application);
    if (copy == NULL) {
		 PyErr_SetString(OssoException, "Invalid application name.");
		 return NULL;
    }
    g_snprintf(object_path, MAX_OP_LEN, OSSO_BUS_ROOT_PATH "/%s", copy);
    g_free(copy);
	copy = NULL;
    g_snprintf(interface, MAX_IF_LEN, "%s", service);

	if (use_system_bus) {
		ret = osso_rpc_run_system_with_argfill(self->context,
					service,
					object_path,
					interface,
					method,
					(wait_reply ?  &retval : NULL),
					_argfill,
					py_rpc_args);
	} else {
		ret = osso_rpc_run_with_argfill(self->context,
					service,
					object_path,
					interface,
					method,
					(wait_reply ?  &retval : NULL),
					_argfill,
					py_rpc_args);
	}

	if (ret != OSSO_OK) {
		_set_exception(ret, ((wait_reply) ? &retval : NULL));
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
	PyObject *py_args;
	PyObject *py_ret;
	PyGILState_STATE state;

	state = PyGILState_Ensure();

	if (rpc_async_callback == NULL) {
		return;
	}
	
	py_args = Py_BuildValue("(ssO)", interface, method, data);
	py_ret = PyEval_CallObject(rpc_async_callback, py_args);
	
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
	osso_return_t ret;

	static char *kwlist[] = { "service", "object_path", "interface", "method",
								"callback", "user_data", "rpc_args", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"ssssO|OO:Context.rpc_async_run", kwlist, &service,
				&object_path, &interface, &method, &py_func, &py_data,
				&py_rpc_args)) {
		return NULL;
	}

	/* py_rpc_args */
	if (py_rpc_args != NULL) {
		if (!PyTuple_Check(py_rpc_args)) {
			PyErr_SetString(PyExc_TypeError,
								"RPC arguments must be in a tuple.");
			return NULL;
		}
	} else {
		py_rpc_args = PyTuple_New(0);
	}

	if (py_func != Py_None) {
		if (!PyCallable_Check(py_func)) {
			PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
			return NULL;
		}
		Py_XINCREF(py_func);
		Py_XDECREF(rpc_async_callback);
		rpc_async_callback = py_func;
	} else {
		Py_XDECREF(rpc_async_callback);
		rpc_async_callback = NULL;
	}

	if (rpc_async_callback == NULL) {
		PyErr_SetString(PyExc_TypeError, "Callback cannot be None.");
		return NULL;
	}

	ret = osso_rpc_async_run_with_argfill(self->context, service, object_path,
										interface, method,
										_wrap_rpc_async_handler, py_data,
										_argfill, py_rpc_args);

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}
	Py_RETURN_NONE;
}


static PyObject *
Context_rpc_async_run_with_defaults(Context *self, PyObject *args, PyObject *kwds)
{
	char *application;
	char *method;
	PyObject *py_func = NULL;
	PyObject *py_data = NULL;
	PyObject *py_rpc_args = NULL;

    char service[MAX_SVC_LEN] = {0};
	char object_path[MAX_OP_LEN] = {0};
	char interface[MAX_IF_LEN] = {0};
	char *copy = NULL;
	osso_return_t ret;

	static char *kwlist[] = { "application", "method", "callback", "user_data",
								"rpc_args", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"ssO|OO:Context.rpc_async_run_with_defaults", kwlist,
				&application, &method, &py_func, &py_data, &py_rpc_args)) {
		return NULL;
	}

	if (py_rpc_args != NULL) {
		if (!PyTuple_Check(py_rpc_args)) {
			PyErr_SetString(PyExc_TypeError,
								"RPC arguments must be in a tuple.");
			return NULL;
		}
	} else {
		py_rpc_args = PyTuple_New(0);
	}

	if (!PyCallable_Check(py_func)) {
		PyErr_SetString(PyExc_TypeError,
							"callback parameter must be callable");
		return NULL;
	}
	Py_XINCREF(py_func);
	Py_XDECREF(rpc_async_callback);
	rpc_async_callback = py_func;

    g_snprintf(service, MAX_SVC_LEN, OSSO_BUS_ROOT ".%s", application);
    copy = appname_to_valid_path_component(application);
    if (copy == NULL) {
		 PyErr_SetString(OssoException, "Invalid application name.");
		 return NULL;
    }
    g_snprintf(object_path, MAX_OP_LEN, OSSO_BUS_ROOT_PATH "/%s", copy);
    g_free(copy);
	copy = NULL;
    g_snprintf(interface, MAX_IF_LEN, "%s", service);

	ret = osso_rpc_async_run_with_argfill(self->context, service, object_path,
										interface, method,
										_wrap_rpc_async_handler, py_data,
										_argfill, py_rpc_args);

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}
	Py_RETURN_NONE;
}


static gint
_wrap_rpc_callback_handler(const gchar *interface, const gchar *method, GArray *arguments, gpointer data, osso_rpc_t *retval)
{
	PyObject *py_args = NULL;
	PyObject *py_ret = NULL;
	PyGILState_STATE state;

	state = PyGILState_Ensure();

	if (set_rpc_callback == NULL) {
		return OSSO_ERROR;
	}
	
	py_args = Py_BuildValue("(ssOO)", interface, method, _rpc_args_c_to_py(arguments), data);
	py_ret = PyEval_CallObject(set_rpc_callback, py_args);

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
	const char *service;
	const char *object_path;
	const char *interface;
	PyObject *py_func = NULL;
	PyObject *py_data = NULL;

	osso_return_t ret;

	static char *kwlist[] = { "service", "object_path", "interface", "callback",
								"user_data", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"sssO|O:Context.set_rpc_callback", kwlist, &service,
				&object_path, &interface, &py_func, &py_data)) {
		return NULL;
	}

	if (py_func != Py_None) {
		if (!PyCallable_Check(py_func)) {
			PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
			return NULL;
		}
		Py_XINCREF(py_func);
		Py_XDECREF(set_rpc_callback);
		set_rpc_callback = py_func;
	} else {
		Py_XDECREF(set_rpc_callback);
		set_rpc_callback = NULL;
	}

	if (py_func != Py_None) {
		if (!PyCallable_Check(py_func)) {
			PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
			return NULL;
		}
		Py_XINCREF(py_func);
		Py_XDECREF(set_rpc_callback);
		set_rpc_callback = py_func;

		ret = osso_rpc_set_cb_f(self->context, service, object_path, interface,
									_wrap_rpc_callback_handler, py_data);
	} else {
		ret = osso_rpc_unset_cb_f(self->context, service, object_path, interface,
									_wrap_rpc_callback_handler, py_data);
		Py_XDECREF(set_rpc_callback);
		set_rpc_callback = NULL;
	}

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


static PyObject *
Context_set_rpc_default_callback(Context *self, PyObject *args, PyObject *kwds)
{
	PyObject *py_func = NULL;
	PyObject *py_data = NULL;
	osso_return_t ret;

	static char *kwlist[] = { "callback", "user_data", 0 };

	if (!_check_context(self->context)) return 0;
	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"O|O:Context.set_rpc_default_callback", kwlist, &py_func,
				&py_data)) {
		return NULL;
	}

	if (py_func != Py_None) {
		if (!PyCallable_Check(py_func)) {
			PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
			return NULL;
		}
		Py_XINCREF(py_func);
		Py_XDECREF(set_rpc_callback);
		set_rpc_callback = py_func;
	} else {
		Py_XDECREF(set_rpc_callback);
		set_rpc_callback = NULL;
	}

	if (set_rpc_callback != NULL) {
		ret = osso_rpc_set_default_cb_f(self->context,
				_wrap_rpc_callback_handler, py_data);
	} else {
		ret = osso_rpc_unset_default_cb_f(self->context,
				_wrap_rpc_callback_handler, py_data);
	}

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


static PyObject *
Context_get_rpc_timeout(Context *self)
{
	osso_return_t ret;
	int timeout;

	if (!_check_context(self->context)) return 0;

	ret = osso_rpc_get_timeout(self->context, &timeout);

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	return Py_BuildValue("i", timeout);
}


static PyObject *
Context_set_rpc_timeout(Context *self, PyObject *args)
{
	osso_return_t ret;
	int timeout = 0;

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTuple(args, "i:Context.set_rpc_timeout", &timeout)) {
		return NULL;
	}

	ret = osso_rpc_set_timeout(self->context, timeout);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}
	Py_RETURN_NONE;
}


/* ----------------------------------------------- */
/* Applications (done)                             */
/* ----------------------------------------------- */

static PyObject *
Context_application_top(Context *self, PyObject *args, PyObject *kwds)
{
	osso_return_t ret;
	char *application = NULL;
	char *arguments = NULL;

	static char *kwlist[] = { "application", "arguments", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|s:Context.application_top",
				kwlist, &application, &arguments))
	{
		return NULL;
	}

	ret = osso_application_top(self->context, application, arguments);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


/* ----------------------------------------------- */
/* Autosaving (done)                               */
/* ----------------------------------------------- */

static void
_wrap_autosave_callback_handler(gpointer data)
{
	PyObject *py_args = NULL;
	PyGILState_STATE state;

	state = PyGILState_Ensure();

	if (autosave_callback == NULL) {
		return;
	}
	
	py_args = Py_BuildValue("(O)", data);
	PyEval_CallObject(autosave_callback, py_args);

	PyGILState_Release(state);
	return;
}


static PyObject *
Context_set_autosave_callback(Context *self, PyObject *args, PyObject *kwds)
{
	PyObject *py_func = NULL;
	PyObject *py_data = NULL;
	osso_return_t ret;

	static char *kwlist[] = { "callback", "user_data", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"O|O:Context.set_autosave_callback", kwlist, &py_func,
				&py_data)) {
		return NULL;
	}

	if (py_func != Py_None) {
		if (!PyCallable_Check(py_func)) {
			PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
			return NULL;
		}
		Py_XINCREF(py_func);
		Py_XDECREF(autosave_callback);
		autosave_callback = py_func;
	} else {
		Py_XDECREF(autosave_callback);
		autosave_callback = NULL;
	}

	if (autosave_callback != NULL) {
		ret = osso_application_set_autosave_cb(self->context,
				_wrap_autosave_callback_handler, py_data);
	} else {
		ret = osso_application_unset_autosave_cb(self->context,
				_wrap_autosave_callback_handler, py_data);
	}

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


static PyObject *
Context_userdata_changed(Context *self)
{
	osso_return_t ret;

	if (!_check_context(self->context)) return 0;

	ret = osso_application_userdata_changed(self->context);

	Py_RETURN_NONE;
}


static PyObject *
Context_force_autosave(Context *self)
{
	osso_return_t ret;

	if (!_check_context(self->context)) return 0;

	ret = osso_application_autosave_force(self->context);

	Py_RETURN_NONE;
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

/* ----------------------------------------------- */
/* Statusbar (done)                                */
/* ----------------------------------------------- */

static PyObject *
Context_statusbar_send_event(Context *self, PyObject *args, PyObject *kwds)
{
	const char *name;
	gint arg1;
	gint arg2;
	const char *arg3;
	osso_return_t ret;
	osso_rpc_t retval;

	static char *kwlist[] = {"name", "arg1", "arg2", "arg3", 0};

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "siis:Context.statusbar_send_event", kwlist, &name, &arg1, &arg2, &arg3)) {
		return NULL;
	}

	ret = osso_statusbar_send_event(self->context, name, arg1, arg2, arg3, &retval);
	if (ret != OSSO_OK) {
		_set_exception(ret, &retval);
		return NULL;
	}

	return _rpc_t_to_python(&retval);
}


/* ----------------------------------------------- */
/* Time (done)                                     */
/* ----------------------------------------------- */

static void
_wrap_time_notification_callback_handler(gpointer data)
{
	PyObject *py_args = NULL;
	PyGILState_STATE state;

	state = PyGILState_Ensure();

	if (time_notification_callback == NULL) {
		return;
	}
	
	py_args = Py_BuildValue("(O)", data);
	PyEval_CallObject(time_notification_callback, py_args);

	PyGILState_Release(state);
	return;
}


static PyObject *
Context_set_time_notification_callback(Context *self, PyObject *args, PyObject *kwds)
{
	PyObject *py_func = NULL;
	PyObject *py_data = NULL;
	osso_return_t ret = OSSO_OK;

	static char *kwlist[] = { "callback", "user_data", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"O|O:Context.set_time_notification_callback", kwlist, &py_func,
				&py_data)) {
		return NULL;
	}

	if (py_func != Py_None) {
		if (!PyCallable_Check(py_func)) {
			PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
			return NULL;
		}
		Py_XINCREF(py_func);
		Py_XDECREF(time_notification_callback);
		time_notification_callback = py_func;
	} else {
		Py_XDECREF(time_notification_callback);
		time_notification_callback = NULL;
	}

	if (time_notification_callback != NULL) {
		ret = osso_time_set_notification_cb(self->context,
				_wrap_time_notification_callback_handler, py_data);
	} else {
		/* FIXME: Where is osso_time_unset_notification_cb function? Is this the
		 * correct way to unset this handler? */
		ret = osso_time_set_notification_cb(self->context, NULL, py_data);
	}

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


static PyObject *
Context_set_time(Context *self, PyObject *args)
{
	time_t epoch;
	struct tm time;
	osso_return_t ret;

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTuple(args,
				"(iiiiiiiii):Context.set_time",
					&time.tm_year, &time.tm_mon, &time.tm_mday,
					&time.tm_hour, &time.tm_min, &time.tm_sec,
					&time.tm_wday, &time.tm_yday, &time.tm_isdst)) {
		return NULL;
	}

	/* remove python adjusts */
	time.tm_year = time.tm_year - 1900;
	time.tm_mon--;

	epoch = mktime(&time);
	if (epoch == -1) {
		PyErr_SetString(PyExc_TypeError, "Invalid date/time.");
		return NULL;
	}
	
	ret = osso_time_set(self->context, epoch);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}
	
	Py_RETURN_NONE;
}


/* ----------------------------------------------- */
/* System notification (done)                      */
/* ----------------------------------------------- */

static PyObject *
Context_system_note_dialog(Context *self, PyObject *args, PyObject *kwds)
{
	osso_return_t ret;
	char *message = NULL;
	char *type_string = NULL;
	osso_system_note_type_t type = OSSO_GN_NOTICE;
	osso_rpc_t retval;

	static char *kwlist[] = { "message", "type", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|s:Context.system_note_dialog",
				kwlist, &message, &type_string))
	{
		return NULL;
	}

	if (type_string) {
		if (!strcasecmp(type_string, "warning")) {
			type = OSSO_GN_WARNING;
		} else if (!strcasecmp(type_string, "error")) {
			type = OSSO_GN_ERROR;
		} else if (!strcasecmp(type_string, "wait")) {
			type = OSSO_GN_WAIT;
		}
	}
	
	ret = osso_system_note_dialog(self->context, message, type, &retval);
	if (ret != OSSO_OK) {
		_set_exception(ret, &retval);
		return NULL;
	}

	return _rpc_t_to_python(&retval);
}


static PyObject *
Context_system_note_infoprint(Context *self, PyObject *args, PyObject *kwds)
{
	char *message = NULL;
	osso_return_t ret;
	osso_rpc_t retval;

	static char *kwlist[] = { "message", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s:Context.system_note_infoprint",
				kwlist, &message))
	{
		return NULL;
	}

	ret = osso_system_note_infoprint(self->context, message, &retval);
	if (ret != OSSO_OK) {
		_set_exception(ret, &retval);
		return NULL;
	}

	return _rpc_t_to_python(&retval);
}

/* ----------------------------------------------- */
/* State saving                                    */
/* ----------------------------------------------- */

static PyObject *
Context_state_write(Context *self, PyObject *args, PyObject *kwds)
{
	osso_state_t osso_state;
	osso_return_t ret = OSSO_OK;
	PyObject *state = NULL;
	PyObject *py_marshal_state = NULL;
	char *marshal_state = NULL;

	static char *kwlist[] = { "state", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:Context.state_write",
				kwlist, &state))
	{
		return NULL;
	}

	py_marshal_state = PyMarshal_WriteObjectToString(state, Py_MARSHAL_VERSION);
	PyString_AsStringAndSize(py_marshal_state, &marshal_state, &osso_state.state_size);
	osso_state.state_data = (void *) marshal_state;

	ret = osso_state_write(self->context, &osso_state);

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


static PyObject *
Context_state_read(Context *self)
{
	osso_state_t osso_state;
	osso_return_t ret = OSSO_OK;
	PyObject *py_marshal_state = NULL;

	if (!_check_context(self->context)) return 0;

	osso_state.state_size = 0;
	osso_state.state_data = NULL;

	ret = osso_state_read(self->context, &osso_state);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	py_marshal_state = PyMarshal_ReadObjectFromString((char *)osso_state.state_data, osso_state.state_size);
	if (py_marshal_state == NULL) {
		return NULL;
	}

	return py_marshal_state;
}


/* ----------------------------------------------- */
/* Plugins (done)                                  */
/* ----------------------------------------------- */

PyObject *
Context_plugin_execute(Context *self, PyObject *args, PyObject *kwds)
{
	osso_return_t ret = OSSO_OK;
	const char *filename = NULL;
	char user_activated = 0;
	PyObject *user_data = NULL;

	void *data = NULL;
	GObject *obj = NULL;

	static char *kwlist[] = { "filename", "user_activated", "user_data", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "sb|O:Context.plugin_execute",
				kwlist, &filename, &user_activated, &user_data))
	{
		return NULL;
	}

	if (user_data != NULL) {
		if (PyObject_HasAttrString(user_data, "__gtype__")) {
			obj = pygobject_get(user_data);
			if (GTK_IS_WINDOW(obj)) {
				data = (void *)obj;
			}
		}
	}

	ret = osso_cp_plugin_execute(self->context, filename, data, user_activated);

	/* FIXME: Why some plugins return -1?
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}
	*/

	Py_RETURN_NONE;
}


PyObject *
Context_plugin_save_state(Context *self, PyObject *args, PyObject *kwds)
{
	osso_return_t ret = OSSO_OK;
	const char *filename = NULL;
	PyObject *user_data = NULL;

	static char *kwlist[] = { "filename", "user_data", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|O:Context.plugin_save_state",
				kwlist, &filename, &user_data))
	{
		return NULL;
	}

	ret = osso_cp_plugin_save_state(self->context, filename, user_data);

	/* FIXME: Why some plugins return -1?
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}
	*/

	Py_RETURN_NONE;
}


/* ----------------------------------------------- */
/* Device State (done)                             */
/* ----------------------------------------------- */

static PyObject *
Context_display_state_on(Context *self)
{
	osso_return_t ret = OSSO_OK;

	if (!_check_context(self->context)) return 0;

	ret = osso_display_state_on(self->context);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


static PyObject *
Context_display_blanking_pause(Context *self)
{
	osso_return_t ret = OSSO_OK;

	if (!_check_context(self->context)) return 0;

	ret = osso_display_blanking_pause(self->context);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


static void
_wrap_device_state_callback_handler(osso_hw_state_t *hw_state, void *data)
{
	PyObject *py_args = NULL;
	PyGILState_STATE state;
	char *hw_state_str;

	state = PyGILState_Ensure();

	if (device_state_callback == NULL) {
		return;
	}

	switch (hw_state->sig_device_mode_ind) {
		case OSSO_DEVMODE_NORMAL:
			hw_state_str = "normal";
			break;
		case OSSO_DEVMODE_FLIGHT:
			hw_state_str = "flight";
			break;
		case OSSO_DEVMODE_OFFLINE:
			hw_state_str = "offline";
			break;
		case OSSO_DEVMODE_INVALID:
			hw_state_str = "invalid";
			break;
		default:
			hw_state_str = "";
	}

	py_args = Py_BuildValue("(bbbbsO)", 
			hw_state->shutdown_ind,
			hw_state->save_unsaved_data_ind,
			hw_state->memory_low_ind,
			hw_state->system_inactivity_ind,
			hw_state_str,
			data);

	PyEval_CallObject(device_state_callback, py_args);

	PyGILState_Release(state);
	return;
}


static PyObject *
Context_set_device_state_callback(Context *self, PyObject *args, PyObject *kwds)
{
	PyObject *py_func = NULL;
	PyObject *py_data = NULL;
	char shutdown = 0;
	char save_data = 0;
	char memory_low = 0;
	char system_inactivity = 0;
	char *mode = NULL;
	osso_hw_state_t hw_state;
	osso_return_t ret;

	static char *kwlist[] = { "callback", "user_data", "shutdown", "save_data",
		"memory_low", "system_inactivity", "mode", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"O|bbbbsO:Context.set_device_state_callback", kwlist,
				&py_func, &shutdown, &save_data, &memory_low,
				&system_inactivity, &mode, &py_data)) {
		return NULL;
	}

	if (py_func != Py_None) {
		if (!PyCallable_Check(py_func)) {
			PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
			return NULL;
		}
		Py_XINCREF(py_func);
		Py_XDECREF(device_state_callback);
		device_state_callback = py_func;
	} else {
		Py_XDECREF(device_state_callback);
		device_state_callback = NULL;
	}
	
	hw_state.shutdown_ind = shutdown;
	hw_state.save_unsaved_data_ind = save_data;
	hw_state.memory_low_ind = memory_low;
	hw_state.system_inactivity_ind = system_inactivity;
	
	if (!strcasecmp(mode, "normal")) {
		hw_state.sig_device_mode_ind = OSSO_DEVMODE_NORMAL;
	} else if (!strcasecmp(mode, "flight")) {
		hw_state.sig_device_mode_ind = OSSO_DEVMODE_FLIGHT;
	} else if (!strcasecmp(mode, "offline")) {
		hw_state.sig_device_mode_ind = OSSO_DEVMODE_OFFLINE;
	} else if (!strcasecmp(mode, "invalid")) {
		hw_state.sig_device_mode_ind = OSSO_DEVMODE_INVALID;
	} else {
		PyErr_SetString(OssoException, "Invalid device mode. Use 'normal',"
				"'flight', 'offline' or 'invalid' instead.");
		return NULL;
	}

	if (autosave_callback != NULL) {
		ret = osso_hw_set_event_cb(self->context, &hw_state,
				_wrap_device_state_callback_handler, py_data);
	} else {
		ret = osso_hw_unset_event_cb(self->context, &hw_state);
	}

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


/* ----------------------------------------------- */
/* MIME types                                      */
/* ----------------------------------------------- */

static void
_wrap_mime_callback_handler(void *data, int argc, char **argv)
{
	int i;
	PyObject *py_args = NULL;
	PyObject *uris = NULL;
	PyGILState_STATE state;

	state = PyGILState_Ensure();

	if (mime_callback == NULL) {
		return;
	}

	uris = PyTuple_New(argc);
	if (uris == NULL) {
		return;
	}
	for (i = 0; i < argc; i++) {
		PyTuple_SetItem(uris, i, Py_BuildValue("s", argv[i]));
	}

	py_args = Py_BuildValue("(OO)", uris, data);
	PyEval_CallObject(mime_callback, py_args);

	PyGILState_Release(state);
	return;
}


static PyObject *
Context_set_mime_callback(Context *self, PyObject *args, PyObject *kwds)
{
	PyObject *py_func = NULL;
	PyObject *py_data = NULL;
	osso_return_t ret;

	static char *kwlist[] = { "callback", "user_data", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"O|O:Context.set_mime_callback", kwlist, &py_func,
				&py_data)) {
		return NULL;
	}

	if (py_func != Py_None) {
		if (!PyCallable_Check(py_func)) {
			PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
			return NULL;
		}
		Py_XINCREF(py_func);
		Py_XDECREF(mime_callback);
		mime_callback = py_func;
	} else {
		Py_XDECREF(mime_callback);
		mime_callback = NULL;
	}

	if (mime_callback != NULL) {
		ret = osso_mime_set_cb(self->context,
				_wrap_mime_callback_handler, py_data);
	} else {
		ret = osso_mime_unset_cb(self->context);
	}

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


/* ----------------------------------------------- */
/* Others                                          */
/* ----------------------------------------------- */

static PyObject *
Context_tasknav_mail_add(Context *self, PyObject *args, PyObject *kwds)
{
	guint id = 0;
	const char *subject = NULL;
	const char *sender = NULL;
	const char *date = NULL;
	gboolean has_attachment = 0;

	osso_return_t ret;

	static char *kwlist[] = {"id", "subject", "sender", "has_attachment", "date", 0};

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"issbs:Context.tasknav_mail_add", kwlist, &id, &subject,
				&sender, &has_attachment, &date)) {
		return NULL;
	}

	ret = osso_tasknav_mail_add(self->context, id, subject, sender, has_attachment, date);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


static PyObject *
Context_tasknav_mail_del(Context *self, PyObject *args, PyObject *kwds)
{
	guint id = 0;

	osso_return_t ret;

	static char *kwlist[] = {"id", 0};

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"i:Context.tasknav_mail_del", kwlist, &id)) {
		return NULL;
	}

	ret = osso_tasknav_mail_del(self->context, id);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


static PyObject *
Context_tasknav_mail_set_outbox_count(Context *self, PyObject *args, PyObject *kwds)
{
	guint count = 0;

	osso_return_t ret;

	static char *kwlist[] = {"count", 0};

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"i:Context.tasknav_mail_set_outbox_count", kwlist, &count)) {
		return NULL;
	}

	ret = osso_tasknav_mail_set_outbox_count(self->context, count);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


static void
_wrap_exit_callback_handler(gboolean die_now, gpointer data)
{
	PyObject *py_args = NULL;
	PyGILState_STATE state;

	state = PyGILState_Ensure();

	if (exit_callback == NULL) {
		return;
	}

	py_args = Py_BuildValue("(O)", data);
	PyEval_CallObject(exit_callback, py_args);

	PyGILState_Release(state);
	return;
}


static PyObject *
Context_set_exit_callback(Context *self, PyObject *args, PyObject *kwds)
{
	PyObject *py_func = NULL;
	PyObject *py_data = NULL;
	osso_return_t ret;

	static char *kwlist[] = { "callback", "user_data", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"O|O:Context.set_exit_callback", kwlist, &py_func,
				&py_data)) {
		return NULL;
	}

	if (py_func != Py_None) {
		if (!PyCallable_Check(py_func)) {
			PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
			return NULL;
		}
		Py_XINCREF(py_func);
		Py_XDECREF(exit_callback);
		exit_callback = py_func;
	} else {
		Py_XDECREF(exit_callback);
		exit_callback = NULL;
	}

	if (exit_callback != NULL) {
		ret = osso_application_set_exit_cb(self->context,
					_wrap_exit_callback_handler, py_data);
	} else {
		/* FIXME: Where is osso_time_unset_notification_cb function? Is this the
		 * correct way to unset this handler? */
		ret = osso_application_set_exit_cb(self->context, NULL, py_data);
	}

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


/* =============================================== */
/* Module: osso                                    */
/* =============================================== */

static struct PyMethodDef Context_methods[] = {
	/* RPC */
	{"rpc_run", (PyCFunction)Context_rpc_run, METH_VARARGS | METH_KEYWORDS,
		"c.rpc_run(service, object_path, interface, method[, rpc_args[, wait_reply[, system_bus]]]) -> object\n\n"
		"Run a RPC method with arguments inside rpc_args tuple.\n\n"
		"Usage example:\n\n"
		"\tc.rpc_run('com.nokia.backup', '/com/nokia/backup', 'com.nokia.backup', 'backup_finish', True)\n"},
	{"rpc_run_with_defaults", (PyCFunction)Context_rpc_run_with_defaults, METH_VARARGS | METH_KEYWORDS,
		"c.rpc_run_with_defaults(application, method[, rpc_args, wait_reply]) -> object\n"
		"\n"
		"Run a RPC method using default parameters.\n"
		"\n"
		"Usage example:\n\n"
		"\tc.rpc_run_with_defaults('tn_mail', 'send_recv_button_focus', (status,)) # status is a int var\n"},
	{"rpc_async_run", (PyCFunction)Context_rpc_async_run, METH_VARARGS | METH_KEYWORDS,
		"c.rpc_async_run(service, object_path, interface, method, callback[, user_data[, rpc_args]])\n\n"
		"Run a RPC method and call 'callback' after finished.\n\n"
		"Usage example:\n\n"
		"\tdef my_func(interface, method, user_data=None):\n"
		"\t\tpass\n\n"
		"\tc.rpc_async_run('com.nokia.backup', '/com/nokia/backup', 'com.nokia.backup', 'backup_finish', my_func, 'hello!', (True,))\n"},
	{"rpc_async_run_with_defaults", (PyCFunction)Context_rpc_async_run_with_defaults, METH_VARARGS | METH_KEYWORDS,
		"c.rpc_async_run_with_defaults(application, method, callback[, user_data[, rpc_args]])\n\n"
		"Run a RPC method using default parameters and call 'callback' after finished.\n\n"
		"Usage example:\n\n"
		"\tdef my_func(interface, method, user_data=None):\n"
		"\t\tpass\n\n"
		"\tc.rpc_async_run_with_defaults('tn_mail', 'send_recv_button_focus', my_func, 'hello!', (True,))\n"},
	{"set_rpc_callback", (PyCFunction)Context_set_rpc_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_rpc_callback(service, object_path, interface, callback[, user_data])\n\n"
		"This method registers a callback function for handling RPC calls to a\n"
		"given object of a service. Use None in callback parameter to unset this\n"
		"callback. The callback will receive the parameters: interface, method,\n"
		"arguments, user_data.\n"},
	{"set_rpc_default_callback", (PyCFunction)Context_set_rpc_default_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_rpc_default_callback(callback[, user_data])\n\n"
		"This method registers a callback function for handling RPC calls to\n"
		"the default service of the application. The default service is\n"
		"'"OSSO_BUS_ROOT".A' where A is the application's name of the context.\n"
		"Use None in callback parameter to unset this callback. The callback\n"
		"will receive the parameters: interface, method, arguments,\n"
		"user_data.\n"},
	{"get_rpc_timeout", (PyCFunction)Context_get_rpc_timeout, METH_NOARGS,
		"c.get_rpc_timeout() -> int\n\nReturn the timeout value used by RPC methods."},
	{"set_rpc_timeout", (PyCFunction)Context_set_rpc_timeout, METH_VARARGS,
		"c.set_rpc_timeout(timeout)\n\nSet the timeout value used by RPC methods."},
	/* Applications */
	{"application_top", (PyCFunction)Context_application_top, METH_VARARGS | METH_KEYWORDS,
		"c.application_top(application, arguments) -> object\n"
		"\n"
		"This method tops an application. If the application is not already\n"
		"running, D-BUS will launch it via the auto-activation mechanism."},
	/* Autosaving */
	{"set_autosave_callback", (PyCFunction)Context_set_autosave_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_autosave_callback(callback[, user_data])\n\n"
		"This method registers an autosave callback function. Use None in\n"
		"callback parameter to unset this callback. The callback will receive a\n"
		"parameter with user_data.\n"},
	{"userdata_changed", (PyCFunction)Context_userdata_changed, METH_NOARGS,
		"c.userdata_changed()\n\n"
		"This method is called by the application when the user data has been\n"
		"changed, so that Libosso knows that a call to the autosave callback is\n"
		"needed in the future to save the user data. The dirty state will be\n"
		"cleared every time the application's autosave callback function is\n"
		"called."},
	{"force_autosave", (PyCFunction)Context_force_autosave, METH_NOARGS,
		"c.force_autosave()\n\n"
		"This method forces a call to the application's autosave function, and\n"
		"resets the autosave timeout. The application should call this method\n"
		"whenever it is switched to background (untopped)."},
	{"get_name", (PyCFunction)Context_get_name, METH_NOARGS, "c.get_name() -> string\n\nReturn the application name.\n"},
	{"get_version", (PyCFunction)Context_get_version, METH_NOARGS, "c.get_version() -> string\n\nReturn the application version.\n"},
	/* Statusbar */
	{"statusbar_send_event", (PyCFunction)Context_statusbar_send_event, METH_VARARGS | METH_KEYWORDS,
		"c.statusbar_send_event(name, int_arg1, int_arg2, string_arg3)\n\n"
		"Send an event to statusbar with int_arg1, int_arg2 and string_arg3\n"
		"parameters."},
	/* Time */
	{"set_time_notification_callback", (PyCFunction)Context_set_time_notification_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_time_notification_callback(callback[, user_data])\n\n"
		"This method registers an callback function that is called when the\n"
		"system time is changed. The callback will receive a parameter with\n"
		"user_data.\n"},
	{"set_time", (PyCFunction)Context_set_time, METH_VARARGS,
		"c.set_time(time_sequence)\n\n"
		"This method sets the system and hardware time, and notifies about the\n"
		"changing of time over the D-BUS system bus. Notice: does not currently\n"
		"change the time. The time_sequence parameter is a sequence with 9\n"
		"elements (like sequences returned by time standard Python module). These\n"
		"elements are (year, mon, month_day, hour, min, sec, week_day, year_day,\n"
		"is_daylight_saving)."},
	/* System Notification */
	{"system_note_dialog", (PyCFunction)Context_system_note_dialog, METH_VARARGS | METH_KEYWORDS,
		"c.system_note_dialog(message[, type='notice']) -> object\n"
		"\n"
		"This method requests that a system note (a window that is modal to the whole system)\n"
		"is shown. Application that do have a GUI should not use this method but the hildon_note\n"
		"widget directly. The \"type\" argument should be 'warning', 'error', 'wait' or 'notice'."},
	{"system_note_infoprint", (PyCFunction)Context_system_note_infoprint, METH_VARARGS | METH_KEYWORDS,
		"c.system_note_infoprint(message) -> object\n"
		"\n"
		"This method requests that the statusbar shows an infoprint (aka information banner).\n"
		"This allow non-GUI applications to display some information to the user.\n"
		"Application that do have a GUI should not use this method but the gtk_infoprint\n"
		"widget directly."},
	/* State saving */
	{"state_write", (PyCFunction)Context_state_write, METH_VARARGS | METH_KEYWORDS,
		"c.state_write(object)\n\n"
		"This method write a state object to disk. Any existing files will be\n"
		"overwritten."},
	{"state_read", (PyCFunction)Context_state_read, METH_VARARGS | METH_KEYWORDS,
		"c.state_read() -> object\n\n"
		"This method read a state object from disk.\n"},
	/* Plugins */
	{"plugin_execute", (PyCFunction)Context_plugin_execute, METH_VARARGS | METH_KEYWORDS,
		"c.plugin_execute(filename, user_activated[, user_data])\n\n"
		"Calls the execute() function of a plugin. The filename parameter is\n"
		"shared object (.so) file of the plugin. It should include the '.so'\n"
		"sufix, but not a path. The user_activated is True if the plugin was\n"
		"activated by a user (as opposed to activated by restoring software\n"
		"state) and the user_data parameteris the GTK top-level widget.\n"
		"It is needed so that the widgets created by plugin can be made a child\n"
		"of the main application that utilizes the plugin."},
	{"plugin_save_state", (PyCFunction)Context_plugin_save_state, METH_VARARGS | METH_KEYWORDS,
		"c.plugin_save_state(filename, user_activated[, user_data])\n\n"
		"This method is used to tell a plugin to save its state."},
	/* Device state */
	{"display_state_on", (PyCFunction)Context_display_state_on, METH_VARARGS | METH_KEYWORDS,
		"c.display_state_on()\n\n"
		"Request to turn on the display as if the user had pressed a key or the\n"
		"touch screen. This can be used after completing a long operation such as\n"
		"downloading a large file or after retrieving e-mails."},
	{"display_blanking_pause", (PyCFunction)Context_display_blanking_pause, METH_VARARGS | METH_KEYWORDS,
		"c.display_blanking_pause()\n\n"
		"Request not to blank the display. This method must be called again\n"
		"within 60 seconds to renew the request. The method is used, for\n"
		"example, by the video player during video playback. Alson prevents\n"
		"suspending the device."},
	{"set_device_state_callback", (PyCFunction)Context_set_device_state_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_device_callback(callback[, user_data[, shutdown[, save_data\n"
		"    [, memory_low[, system_inactivity[, mode]]]]]]\n"
		"  )\n\n"
		"This method registers a callback function that is called whenever the\n"
		"state device changes. The first call to this method will also check\n"
		"the current state of the device, and if the state is available, the\n"
		"corresponding callback function will be called immediately. Callback\n"
		"function will receive the shutdown, save_data, memory_low,\n"
		"system_inactivity, mode and user_data parameters with state device. If\n"
		"you specify shutdown, save_data, memory_low, system_inactivity\n"
		"(booleans) or mode ('normal', 'flight', 'invalid', 'offline') the\n"
		"callback function will be called only when this state changes. Use\n"
		"None in callback parameter to unset this callback."},
	/* MIME */
	{"set_mime_callback", (PyCFunction)Context_set_mime_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_mime_callback(callback[, user_data])\n\n"
		"This method registers a MIME callback function that Libosso calls\n"
		"when the user wants the application to open file(s) of a MIME type\n"
		"handled by the application.Use in callback parameter to unset this\n"
		"callback. The callback will receive a parameter with a list of URIs and\n"
		"user_data.\n"},
	/* Others */
	{"tasknav_mail_add", (PyCFunction)Context_tasknav_mail_add, METH_VARARGS | METH_KEYWORDS,
		"c.tasknav_mail_add(id, subject, sender, has_attachment, date)\n\n"
		"This method adds a mail to be displayed in the mail popup of the\n"
		"tasknavigator."},
	{"tasknav_mail_del", (PyCFunction)Context_tasknav_mail_del, METH_VARARGS | METH_KEYWORDS,
		"c.tasknav_mail_add(id)\n\n"
		"This method removes an email from mail popup of the tasknavigator."},
	{"tasknav_mail_set_outbox_count", (PyCFunction)Context_tasknav_mail_set_outbox_count, METH_VARARGS | METH_KEYWORDS,
		"c.tasknav_mail_set_outbox_count(count)\n\n"
		"This method updates the outbox message count in the task navigator mail\n"
		"window."},
  	{"set_exit_callback", (PyCFunction)Context_set_exit_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_exit_callback(callback[, user_data])\n\n"
		"This method registers the application's exit callback function. when\n"
		"Libosso calls the application's exit callback function, the application\n"
		"should save its GUI state and unsaved user data and exit as soon as\n"
		"possible. The callback will receive user_data."},
	/* Default */
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

static struct PyMethodDef osso_methods[] = {
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
	module = Py_InitModule3("osso", osso_methods,
			"FIXME: put documentation about RPC, Application, Autosave, Statusbar, etc.");

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


/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
