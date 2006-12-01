/**
 * osso-system-note.c
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
/* SystemNote type default methods                    */
/* ----------------------------------------------- */

PyObject *
SystemNote_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Context *self;

	self = (Context *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->context = NULL;
	}

	return (PyObject *)self;
}


int
SystemNote_init(Context *self, PyObject *args, PyObject *kwds)
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
SystemNote_close(Context *self)
{
	if (!_check_context(self->context)) return 0;
	self->context = NULL;
	Py_RETURN_NONE;
}


void
SystemNote_dealloc(Context *self)
{
	if (self->context == NULL)
		return;

	self->context = NULL;
	return;
}


static struct PyMethodDef SystemNote_methods[] = {
	/* System Notification */
	{"system_note_dialog", (PyCFunction)Context_system_note_dialog, METH_VARARGS | METH_KEYWORDS,
		"c.system_note.system_note_dialog(message[, type='notice']) -> object\n"
		"\n"
		"This method requests that a system note (a window that is modal to the whole system)\n"
		"is shown. Application that do have a GUI should not use this method but the hildon_note\n"
		"widget directly. The \"type\" argument should be 'warning', 'error', 'wait' or 'notice'."},
	{"system_note_infoprint", (PyCFunction)Context_system_note_infoprint, METH_VARARGS | METH_KEYWORDS,
		"c.system_note.system_note_infoprint(message) -> object\n"
		"\n"
		"This method requests that the statusbar shows an infoprint (aka information banner).\n"
		"This allow non-GUI applications to display some information to the user.\n"
		"Application that do have a GUI should not use this method but the gtk_infoprint\n"
		"widget directly."},
	/* Default */
	{"close", (PyCFunction)SystemNote_close, METH_NOARGS, "Close SytemNote context."},
	{0, 0, 0, 0}
};

static PyTypeObject SystemNoteType = {
	PyObject_HEAD_INIT(NULL)
	0,																/* ob_size */
	"osso.SystemNote",													/* tp_name */
	sizeof(Context),												/* tp_basicsize */
	0,																/* tp_itemsize */
	(destructor)SystemNote_dealloc,									/* tp_dealloc */
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
	"OSSO SystemNote class",											/* tp_doc */
	0,																/* tp_traverse */
	0,																/* tp_clear */
	0,																/* tp_richcompare */
	0,																/* tp_weaklistoffset */
	0,																/* tp_iter */
	0,																/* tp_iternext */
	SystemNote_methods,												/* tp_methods */
	0,																/* tp_members */
	0,																/* tp_getset */
	0,																/* tp_base */
	0,																/* tp_dict */
	0,																/* tp_descr_get */
	0,																/* tp_descr_set */
	0,																/* tp_dictoffset */
	(initproc)SystemNote_init,											/* tp_init */
	0,																/* tp_alloc */
	SystemNote_new,													/* tp_new */
};

static struct PyMethodDef osso_methods[] = {
	{0, 0, 0, 0}
};

PyMODINIT_FUNC
initsystem_note(void)
{
	PyObject *module;

	/* prepare types */
	SystemNoteType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&SystemNoteType) < 0) {
		return;
	}

	/* initialize module */
	module = Py_InitModule3("system_note", osso_methods,
			"FIXME: put documentation about SytemNote module.");

	/* add types */
	Py_INCREF(&SystemNoteType);
	PyModule_AddObject(module, "SystemNote", (PyObject *)&SystemNoteType);

	/* add contants */
	/* : */
	/* : */
	/* : */
}


PyObject *
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


PyObject *
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

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
