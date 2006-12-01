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

#include "osso.h"

/* ----------------------------------------------- */
/* Context type default methods                    */
/* ----------------------------------------------- */

PyObject *
Context_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Context *self;

	self = (Context *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->context = NULL;
	}

	return (PyObject *)self;
}


int
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


PyObject *
Context_close(Context *self)
{
	if (!_check_context(self->context)) return 0;
	osso_deinitialize(self->context);
	self->context = NULL;
	Py_RETURN_NONE;
}


void
Context_dealloc(Context *self)
{
	if (self->context == NULL)
		return;

	osso_deinitialize(self->context);
	self->context = NULL;
	return;
}


static struct PyMethodDef Context_methods[] = {
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
initcontext(void)
{
	PyObject *module;

	/* prepare types */
	ContextType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&ContextType) < 0) {
		return;
	}

	/* initialize module */
	module = Py_InitModule3("context", osso_methods,
			"FIXME: put documentation about Context module.");

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
