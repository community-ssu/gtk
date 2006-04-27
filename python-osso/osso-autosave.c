/**
 * osso-autosave.c
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

static PyObject *autosave_callback = NULL;

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


PyObject *
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


PyObject *
Context_userdata_changed(Context *self)
{
	osso_return_t ret;

	if (!_check_context(self->context)) return 0;

	ret = osso_application_userdata_changed(self->context);

	Py_RETURN_NONE;
}


PyObject *
Context_force_autosave(Context *self)
{
	osso_return_t ret;

	if (!_check_context(self->context)) return 0;

	ret = osso_application_autosave_force(self->context);

	Py_RETURN_NONE;
}


PyObject *
Context_get_name(Context *self)
{
	const char *name;

	if (!_check_context(self->context)) return 0;

	name = osso_application_name_get(self->context);
	return Py_BuildValue("s", name);
}


PyObject *
Context_get_version(Context *self)
{
	const char *version;

	if (!_check_context(self->context)) return 0;

	version = osso_application_version_get(self->context);
	return Py_BuildValue("s", version);
}

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
