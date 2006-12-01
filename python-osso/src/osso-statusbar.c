/**
 * osso-statusba.c
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

/* ----------------------------------------------- */
/* Statusbar type default methods                    */
/* ----------------------------------------------- */

PyObject *
Statusbar_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Context *self;

	self = (Context *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->context = NULL;
	}

	return (PyObject *)self;
}


int
Statusbar_init(Context *self, PyObject *args, PyObject *kwds)
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
Statusbar_close(Context *self)
{
	if (!_check_context(self->context)) return 0;
	self->context = NULL;
	Py_RETURN_NONE;
}


void
Statusbar_dealloc(Context *self)
{
	if (self->context == NULL)
		return;

	self->context = NULL;
	return;
}


static struct PyMethodDef Statusbar_methods[] = {
	/* Statusbar */
	{"statusbar_send_event", (PyCFunction)Context_statusbar_send_event, METH_VARARGS | METH_KEYWORDS,
		"c.statusbar.statusbar_send_event(name, int_arg1, int_arg2, string_arg3)\n\n"
		"Send an event to statusbar with int_arg1, int_arg2 and string_arg3\n"
		"parameters."},
	/* Default */
	{"close", (PyCFunction)Statusbar_close, METH_NOARGS, "Close Statusbar context."},
	{0, 0, 0, 0}
};

static PyTypeObject StatusbarType = {
	PyObject_HEAD_INIT(NULL)
	0,																/* ob_size */
	"osso.Statusbar",												/* tp_name */
	sizeof(Context),												/* tp_basicsize */
	0,																/* tp_itemsize */
	(destructor)Statusbar_dealloc,									/* tp_dealloc */
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
	"OSSO Statusbar class",											/* tp_doc */
	0,																/* tp_traverse */
	0,																/* tp_clear */
	0,																/* tp_richcompare */
	0,																/* tp_weaklistoffset */
	0,																/* tp_iter */
	0,																/* tp_iternext */
	Statusbar_methods,												/* tp_methods */
	0,																/* tp_members */
	0,																/* tp_getset */
	0,																/* tp_base */
	0,																/* tp_dict */
	0,																/* tp_descr_get */
	0,																/* tp_descr_set */
	0,																/* tp_dictoffset */
	(initproc)Statusbar_init,										/* tp_init */
	0,																/* tp_alloc */
	Statusbar_new,													/* tp_new */
};

static struct PyMethodDef osso_methods[] = {
	{0, 0, 0, 0}
};

PyMODINIT_FUNC
initstatusbar(void)
{
	PyObject *module;

	/* prepare types */
	StatusbarType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&StatusbarType) < 0) {
		return;
	}

	/* initialize module */
	module = Py_InitModule3("statusbar", osso_methods,
			"FIXME: put documentation about RPC, Application, Autosave, Statusbar, etc.");

	/* add types */
	Py_INCREF(&StatusbarType);
	PyModule_AddObject(module, "Statusbar", (PyObject *)&StatusbarType);

	/* add contants */
	/* : */
	/* : */
	/* : */
}

	
PyObject *
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

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
