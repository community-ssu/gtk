/**
 * osso-plugin.c
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
Plugin_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Context *self;

	self = (Context *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->context = NULL;
	}

	return (PyObject *)self;
}


int
Plugin_init(Context *self, PyObject *args, PyObject *kwds)
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
Plugin_close(Context *self)
{
	if (!_check_context(self->context)) return 0;
	osso_deinitialize(self->context);
	self->context = NULL;
	Py_RETURN_NONE;
}


void
Plugin_dealloc(Context *self)
{
	if (self->context == NULL)
		return;

	self->context = NULL;
	return;
}


static struct PyMethodDef Plugin_methods[] = {
	/* Plugins */
	{"plugin_execute", (PyCFunction)Context_plugin_execute, METH_VARARGS | METH_KEYWORDS,
		"c.plugin.plugin_execute(filename, user_activated[, user_data])\n\n"
		"Calls the execute() function of a plugin. The filename parameter is\n"
		"shared object (.so) file of the plugin. It should include the '.so'\n"
		"sufix, but not a path. The user_activated is True if the plugin was\n"
		"activated by a user (as opposed to activated by restoring software\n"
		"state) and the user_data parameteris the GTK top-level widget.\n"
		"It is needed so that the widgets created by plugin can be made a child\n"
		"of the main application that utilizes the plugin."},
	{"plugin_save_state", (PyCFunction)Context_plugin_save_state, METH_VARARGS | METH_KEYWORDS,
		"c.plugin.plugin_save_state(filename, user_activated[, user_data])\n\n"
		"This method is used to tell a plugin to save its state."},
	/* Default */
	{"close", (PyCFunction)Plugin_close, METH_NOARGS, "Close Plugin context method."},
	{0, 0, 0, 0}
};

static PyTypeObject PluginType = {
	PyObject_HEAD_INIT(NULL)
	0,																/* ob_size */
	"osso.Plugin",													/* tp_name */
	sizeof(Context),												/* tp_basicsize */
	0,																/* tp_itemsize */
	(destructor)Plugin_dealloc,										/* tp_dealloc */
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
	"OSSO Plugin class",											/* tp_doc */
	0,																/* tp_traverse */
	0,																/* tp_clear */
	0,																/* tp_richcompare */
	0,																/* tp_weaklistoffset */
	0,																/* tp_iter */
	0,																/* tp_iternext */
	Plugin_methods,													/* tp_methods */
	0,																/* tp_members */
	0,																/* tp_getset */
	0,																/* tp_base */
	0,																/* tp_dict */
	0,																/* tp_descr_get */
	0,																/* tp_descr_set */
	0,																/* tp_dictoffset */
	(initproc)Plugin_init,											/* tp_init */
	0,																/* tp_alloc */
	Plugin_new,														/* tp_new */
};

static struct PyMethodDef osso_methods[] = {
	{0, 0, 0, 0}
};

PyMODINIT_FUNC
initplugin(void)
{
	PyObject *module;

	/* prepare types */
	PluginType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&PluginType) < 0) {
		return;
	}

	/* initialize module */
	module = Py_InitModule3("plugin", osso_methods,
			"FIXME: put documentation about Plugin module.");

	/* add types */
	Py_INCREF(&PluginType);
	PyModule_AddObject(module, "Plugin", (PyObject *)&PluginType);

	/* add contants */
	/* : */
	/* : */
	/* : */
}

	
PyObject *
Context_plugin_execute(Context *self, PyObject *args, PyObject *kwds)
{
	osso_return_t ret = OSSO_OK;
	const char *filename = NULL;
	char user_activated = 0;
	PyObject *user_data = NULL;

	void *data = NULL;
	GObject *obj = NULL;

	static char *kwlist[] = { "filename", "user_activated", "user_data", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "sb|O:Context.plugin_execute",
				kwlist, &filename, &user_activated, &user_data))
	{
		return NULL;
	}

	if (user_data != NULL) {
		if (PyObject_HasAttrString(user_data, "__gtype__")) {
			obj = pygobject_get(user_data);
			if (GTK_IS_WINDOW(obj)) {
				data = (void *)obj;
			}
		}
	}

	ret = osso_cp_plugin_execute(self->context, filename, data, user_activated);

	/* FIXME: Why some plugins return -1?
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}
	*/

	Py_RETURN_NONE;
}


PyObject *
Context_plugin_save_state(Context *self, PyObject *args, PyObject *kwds)
{
	osso_return_t ret = OSSO_OK;
	const char *filename = NULL;
	PyObject *user_data = NULL;

	static char *kwlist[] = { "filename", "user_data", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|O:Context.plugin_save_state",
				kwlist, &filename, &user_data))
	{
		return NULL;
	}

	ret = osso_cp_plugin_save_state(self->context, filename, user_data);

	/* FIXME: Why some plugins return -1?
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}
	*/

	Py_RETURN_NONE;
}

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
