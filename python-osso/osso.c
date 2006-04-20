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

/* Default values for .._with_defaults functions */
#define MAX_IF_LEN 255
#define MAX_SVC_LEN 255
#define MAX_OP_LEN 255
#define OSSO_BUS_ROOT      "com.nokia"
#define OSSO_BUS_ROOT_PATH "/com/nokia"

gchar *
appname_to_valid_path_component(const char *application)
{
    gchar *copy = NULL;
    gchar *p = NULL;

    g_assert(application != NULL);

    copy = g_strdup(application);
    if (copy == NULL) return NULL;

    for (p = g_strrstr(copy, "."); p != NULL; p = g_strrstr(p + 1, ".")) {
        *p = '/';
    }
    return copy;
}


/* helper functions */
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
/* / helper functions */

/* Type: Context */
typedef struct {
	PyObject_HEAD
	osso_context_t *context;
} Context;


/* Context method */
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
		_set_exception(ret, &retval);
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
		 PyErr_SetString(OssoException, "Invalid application.");
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


/* rpc_async_run */
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

	if (!PyCallable_Check(py_func)) {
		PyErr_SetString(PyExc_TypeError,
							"callback parameter must be callable");
		return NULL;
	}
	Py_XINCREF(py_func);
	Py_XDECREF(rpc_async_callback);
	rpc_async_callback = py_func;

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
		 PyErr_SetString(OssoException, "Invalid application.");
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
/* /rpc_async_run */


/* set_rpc_callback */
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
	const gchar *service;
	const gchar *object_path;
	const gchar *interface;
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

	if (!PyCallable_Check(py_func)) {
		PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
		return NULL;
	}
	Py_XINCREF(py_func);
	Py_XDECREF(set_rpc_callback);
	set_rpc_callback = py_func;

	ret = osso_rpc_set_cb_f(self->context, service, object_path, interface,
								_wrap_rpc_callback_handler, py_data);

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}
/* /set_rpc_callback */


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
	if (!_check_context(self->context)) return;
	Context_close(self);
	return;
}


static struct PyMethodDef Context_methods[] = {
	{"get_name", (PyCFunction)Context_get_name, METH_NOARGS, "c.get_name() -> string\n\nReturn the application name.\n"},
	{"get_version", (PyCFunction)Context_get_version, METH_NOARGS, "c.get_name() -> string\n\nReturn the application version.\n"},
	{"rpc_run", (PyCFunction)Context_rpc_run, METH_VARARGS | METH_KEYWORDS, \
		"c.rpc_run(service, object_path, interface, method[, rpc_args[, wait_reply[, system_bus]]]) -> object\n"
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
	{"get_rpc_timeout", (PyCFunction)Context_get_rpc_timeout, METH_NOARGS,
		"c.get_rpc_timeout() -> int\n\nReturn the timeout value used by RPC functions."},
	{"set_rpc_timeout", (PyCFunction)Context_set_rpc_timeout, METH_VARARGS,
		"c.set_rpc_timeout(timeout)\n\nSet the timeout value used by RPC functions."},
	{"application_top", (PyCFunction)Context_application_top, METH_VARARGS | METH_KEYWORDS,
		"c.application_top(application, arguments) -> object\n"
		"\n"
		"This method tops an application. If the application is not already\n"
		"running, D-BUS will launch it via the auto-activation mechanism."},
	{"system_note_dialog", (PyCFunction)Context_system_note_dialog, METH_VARARGS | METH_KEYWORDS,
		"c.system_note_dialog(message[, type='notice']) -> object\n"
		"\n"
		"This method requests that a system note (a window that is modal to the whole system)\n"
		"is shown. Application that do have a GUI should not use this function but the hildon_note\n"
		"widget directly. The \"type\" argument should be 'warning', 'error', 'wait' or 'notice'."},
	{"system_note_infoprint", (PyCFunction)Context_system_note_infoprint, METH_VARARGS | METH_KEYWORDS,
		"c.system_note_infoprint(message) -> object\n"
		"\n"
		"This method requests that the statusbar shows an infoprint (aka information banner).\n"
		"This allow non-GUI applications to display some information to the user.\n"
		"Application that do have a GUI should not use this function but the gtk_infoprint\n"
		"widget directly."},
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


/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
