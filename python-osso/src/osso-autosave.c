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

/* ----------------------------------------------- */
/* Autosave type default methods                    */
/* ----------------------------------------------- */

PyObject *
Autosave_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Context *self;

	self = (Context *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->context = NULL;
	}

	return (PyObject *)self;
}


int
Autosave_init(Context *self, PyObject *args, PyObject *kwds)
{	
	PyObject *ossocontext = NULL;
	Context *fullcontext = NULL;
	
	if (!PyArg_ParseTuple(args, "O", &ossocontext))
		return -1;

	fullcontext = (Context *)ossocontext;
	self->context = fullcontext->context;
	
	if (self->context == NULL) {
		PyErr_SetString(OssoException, "Context not initialize yet.");
		return -1;
	}

	return 0;
}


PyObject *
Autosave_close(Context *self)
{
	if (!_check_context(self->context)) return 0;
	self->context = NULL;
	Py_RETURN_NONE;
}


void
Autosave_dealloc(Context *self)
{
	if (self->context == NULL)
		return;

	self->context = NULL;
	return;
}


static struct PyMethodDef Autosave_methods[] = {
	/* Autosaving */
	{"set_autosave_callback", (PyCFunction)Context_set_autosave_callback, METH_VARARGS | METH_KEYWORDS,
		"c.autosave.set_autosave_callback(callback[, user_data])\n\n"
		"This method registers an autosave callback function. Use None in\n"
		"callback parameter to unset this callback. The callback will receive a\n"
		"parameter with user_data.\n"},
	{"userdata_changed", (PyCFunction)Context_userdata_changed, METH_NOARGS,
		"c.autosave.userdata_changed()\n\n"
		"This method is called by the application when the user data has been\n"
		"changed, so that Libosso knows that a call to the autosave callback is\n"
		"needed in the future to save the user data. The dirty state will be\n"
		"cleared every time the application's autosave callback function is\n"
		"called."},
	{"force_autosave", (PyCFunction)Context_force_autosave, METH_NOARGS,
		"c.autosave.force_autosave()\n\n"
		"This method forces a call to the application's autosave function, and\n"
		"resets the autosave timeout. The application should call this method\n"
		"whenever it is switched to background (untopped)."},
	{"get_name", (PyCFunction)Context_get_name, METH_NOARGS, "c.autosave.get_name() -> string\n\nReturn the application name.\n"},
	{"get_version", (PyCFunction)Context_get_version, METH_NOARGS, "c.autosave.get_version() -> string\n\nReturn the application version.\n"},
	/* Default */
	{"close", (PyCFunction)Autosave_close, METH_NOARGS, "Close autosave context."},
	{0, 0, 0, 0}
};

static PyTypeObject AutosaveType = {
	PyObject_HEAD_INIT(NULL)
	0,																/* ob_size */
	"osso.Autosave",												/* tp_name */
	sizeof(Context),												/* tp_basicsize */
	0,																/* tp_itemsize */
	(destructor)Autosave_dealloc,									/* tp_dealloc */
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
	"OSSO Autosave class",											/* tp_doc */
	0,																/* tp_traverse */
	0,																/* tp_clear */
	0,																/* tp_richcompare */
	0,																/* tp_weaklistoffset */
	0,																/* tp_iter */
	0,																/* tp_iternext */
	Autosave_methods,												/* tp_methods */
	0,																/* tp_members */
	0,																/* tp_getset */
	0,																/* tp_base */
	0,																/* tp_dict */
	0,																/* tp_descr_get */
	0,																/* tp_descr_set */
	0,																/* tp_dictoffset */
	(initproc)Autosave_init,										/* tp_init */
	0,																/* tp_alloc */
	Autosave_new,													/* tp_new */
};

static struct PyMethodDef osso_methods[] = {
	{0, 0, 0, 0}
};

PyMODINIT_FUNC
initautosave(void)
{
	PyObject *module;

	/* prepare types */
	AutosaveType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&AutosaveType) < 0) {
		return;
	}

	/* initialize module */
	module = Py_InitModule3("autosave", osso_methods,
			"FIXME: put documentation about Autosave module.");

	/* add types */
	Py_INCREF(&AutosaveType);
	PyModule_AddObject(module, "Autosave", (PyObject *)&AutosaveType);

	/* add contants */
	/* : */
	/* : */
	/* : */
}

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
