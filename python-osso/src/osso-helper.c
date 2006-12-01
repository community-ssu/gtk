/**
 * osso-internal.c
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

char _check_context(osso_context_t *context)
{
	if (context == NULL) {
		PyErr_SetString(OssoException, "Context invalid. Not initialized.");
		return FALSE;
	}
	return TRUE;
}

void
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

PyObject *
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

void
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

PyObject *
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

void
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

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
