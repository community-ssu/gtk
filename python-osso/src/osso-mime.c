/**
 * osso-mime.c
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

static PyObject *mime_callback = NULL;

/* ----------------------------------------------- */
/* Mime type default methods                    */
/* ----------------------------------------------- */

PyObject *
Mime_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Context *self;

	self = (Context *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->context = NULL;
	}

	return (PyObject *)self;
}


int
Mime_init(Context *self, PyObject *args, PyObject *kwds)
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
Mime_close(Context *self)
{
	if (!_check_context(self->context)) return 0;
	self->context = NULL;
	Py_RETURN_NONE;
}


void
Mime_dealloc(Context *self)
{
	if (self->context == NULL)
		return;

	self->context = NULL;
	return;
}


static struct PyMethodDef Mime_methods[] = {
	/* MIME */
	{"set_mime_callback", (PyCFunction)Context_set_mime_callback, METH_VARARGS | METH_KEYWORDS,
		"c.mime.set_mime_callback(callback[, user_data])\n\n"
		"This method registers a MIME callback function that Libosso calls\n"
		"when the user wants the application to open file(s) of a MIME type\n"
		"handled by the application.Use in callback parameter to unset this\n"
		"callback. The callback will receive a parameter with a list of URIs and\n"
		"user_data.\n"},
	/* Default */
	{"close", (PyCFunction)Mime_close, METH_NOARGS, "Close Mime context."},
	{0, 0, 0, 0}
};

static PyTypeObject MimeType = {
	PyObject_HEAD_INIT(NULL)
	0,																/* ob_size */
	"osso.Mime",													/* tp_name */
	sizeof(Context),												/* tp_basicsize */
	0,																/* tp_itemsize */
	(destructor)Mime_dealloc,										/* tp_dealloc */
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
	"OSSO Mime class",												/* tp_doc */
	0,																/* tp_traverse */
	0,																/* tp_clear */
	0,																/* tp_richcompare */
	0,																/* tp_weaklistoffset */
	0,																/* tp_iter */
	0,																/* tp_iternext */
	Mime_methods,													/* tp_methods */
	0,																/* tp_members */
	0,																/* tp_getset */
	0,																/* tp_base */
	0,																/* tp_dict */
	0,																/* tp_descr_get */
	0,																/* tp_descr_set */
	0,																/* tp_dictoffset */
	(initproc)Mime_init,											/* tp_init */
	0,																/* tp_alloc */
	Mime_new,														/* tp_new */
};

static struct PyMethodDef osso_methods[] = {
	{0, 0, 0, 0}
};

PyMODINIT_FUNC
initmime(void)
{
	PyObject *module;

	/* prepare types */
	MimeType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&MimeType) < 0) {
		return;
	}

	/* initialize module */
	module = Py_InitModule3("mime", osso_methods,
			"FIXME: put documentation about RPC, Application, Autosave, Statusbar, etc.");

	/* add types */
	Py_INCREF(&MimeType);
	PyModule_AddObject(module, "Mime", (PyObject *)&MimeType);

	/* add contants */
	/* : */
	/* : */
	/* : */
}

	
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


PyObject *
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

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
