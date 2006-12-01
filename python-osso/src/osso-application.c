/**
 * osso-application.c
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
/* Application type default methods                */
/* ----------------------------------------------- */

PyObject *
Application_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Context *self;

	self = (Context *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->context = NULL;
	}

	return (PyObject *)self;
}


int
Application_init(Context *self, PyObject *args, PyObject *kwds)
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
Application_close(Context *self)
{
	if (!_check_context(self->context)) return 0;
	self->context = NULL;
	Py_RETURN_NONE;
}


void
Application_dealloc(Context *self)
{
	if (self->context == NULL)
		return;

	self->context = NULL;
	return;
}


static struct PyMethodDef Application_methods[] = {
	/* Applications */
	{"application_top", (PyCFunction)Context_application_top, METH_VARARGS | METH_KEYWORDS,
		"c.application.application_top(application, arguments) -> object\n"
		"\n"
		"This method tops an application. If the application is not already\n"
		"running, D-BUS will launch it via the auto-activation mechanism."},
	/* Default */
	{"close", (PyCFunction)Application_close, METH_NOARGS, "Close application context."},
	{0, 0, 0, 0}
};

static PyTypeObject ApplicationType = {
	PyObject_HEAD_INIT(NULL)
	0,																/* ob_size */
	"osso.Application",												/* tp_name */
	sizeof(Context),												/* tp_basicsize */
	0,																/* tp_itemsize */
	(destructor)Application_dealloc,								/* tp_dealloc */
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
	"OSSO Application class",										/* tp_doc */
	0,																/* tp_traverse */
	0,																/* tp_clear */
	0,																/* tp_richcompare */
	0,																/* tp_weaklistoffset */
	0,																/* tp_iter */
	0,																/* tp_iternext */
	Application_methods,											/* tp_methods */
	0,																/* tp_members */
	0,																/* tp_getset */
	0,																/* tp_base */
	0,																/* tp_dict */
	0,																/* tp_descr_get */
	0,																/* tp_descr_set */
	0,																/* tp_dictoffset */
	(initproc)Application_init,										/* tp_init */
	0,																/* tp_alloc */
	Application_new,												/* tp_new */
};

static struct PyMethodDef osso_methods[] = {
	{0, 0, 0, 0}
};

PyMODINIT_FUNC
initapplication(void)
{
	PyObject *module;

	/* prepare types */
	ApplicationType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&ApplicationType) < 0) {
		return;
	}

	/* initialize module */
	module = Py_InitModule3("application", osso_methods,
			"FIXME: put documentation about Application module.");

	/* add types */
	Py_INCREF(&ApplicationType);
	PyModule_AddObject(module, "Application", (PyObject *)&ApplicationType);

	/* add contants */
	/* : */
	/* : */
	/* : */
}

PyObject *
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

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
