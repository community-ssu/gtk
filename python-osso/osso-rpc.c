/**
 * osso-rpc.c
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

#include "osso.h"

static PyObject *set_rpc_callback = NULL;
static PyObject *rpc_async_callback = NULL;

PyObject *
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


PyObject *
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
_wrap_rpc_async_handler(const char *interface, const char *method, osso_rpc_t *retval, void *data)
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


PyObject *
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


PyObject *
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


static int
_wrap_rpc_callback_handler(const char *interface, const char *method, GArray *arguments, void *data, osso_rpc_t *retval)
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


PyObject *
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


PyObject *
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


PyObject *
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


PyObject *
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

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
