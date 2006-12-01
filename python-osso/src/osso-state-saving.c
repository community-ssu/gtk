/**
 * osso-state.c
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
StateSaving_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Context *self;

	self = (Context *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->context = NULL;
	}

	return (PyObject *)self;
}


int
StateSaving_init(Context *self, PyObject *args, PyObject *kwds)
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
StateSaving_close(Context *self)
{
	if (!_check_context(self->context)) return 0;
	self->context = NULL;
	Py_RETURN_NONE;
}


void
StateSaving_dealloc(Context *self)
{
	if (self->context == NULL)
		return;

	self->context = NULL;
	return;
}


static struct PyMethodDef StateSaving_methods[] = {
	/* State saving */
	{"state_write", (PyCFunction)Context_state_write, METH_VARARGS | METH_KEYWORDS,
		"c.state.state_write(object)\n\n"
		"This method write a state object to disk. Any existing files will be\n"
		"overwritten."},
	{"state_read", (PyCFunction)Context_state_read, METH_VARARGS | METH_KEYWORDS,
		"c.state.state_read() -> object\n\n"
		"This method read a state object from disk.\n"},
	{"close", (PyCFunction)StateSaving_close, METH_NOARGS, "Close State context."},
	{0, 0, 0, 0}
};

static PyTypeObject StateSavingType = {
	PyObject_HEAD_INIT(NULL)
	0,																/* ob_size */
	"osso.StateSaving",												/* tp_name */
	sizeof(Context),												/* tp_basicsize */
	0,																/* tp_itemsize */
	(destructor)StateSaving_dealloc,								/* tp_dealloc */
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
	"OSSO State Saving class",										/* tp_doc */
	0,																/* tp_traverse */
	0,																/* tp_clear */
	0,																/* tp_richcompare */
	0,																/* tp_weaklistoffset */
	0,																/* tp_iter */
	0,																/* tp_iternext */
	StateSaving_methods,											/* tp_methods */
	0,																/* tp_members */
	0,																/* tp_getset */
	0,																/* tp_base */
	0,																/* tp_dict */
	0,																/* tp_descr_get */
	0,																/* tp_descr_set */
	0,																/* tp_dictoffset */
	(initproc)StateSaving_init,										/* tp_init */
	0,																/* tp_alloc */
	StateSaving_new,												/* tp_new */
};

static struct PyMethodDef osso_methods[] = {
	{0, 0, 0, 0}
};

PyMODINIT_FUNC
initstate_saving(void)
{
	PyObject *module;

	/* prepare types */
	StateSavingType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&StateSavingType) < 0) {
		return;
	}

	/* initialize module */
	module = Py_InitModule3("state_saving", osso_methods,
			"FIXME: put documentation about StateSaving module.");

	/* add types */
	Py_INCREF(&StateSavingType);
	PyModule_AddObject(module, "StateSaving", (PyObject *)&StateSavingType);

	/* add contants */
	/* : */
	/* : */
	/* : */
}

	
PyObject *
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
	PyString_AsStringAndSize(py_marshal_state, &marshal_state, (int *)&osso_state.state_size);
	osso_state.state_data = (void *) marshal_state;

	ret = osso_state_write(self->context, &osso_state);

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


PyObject *
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

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
