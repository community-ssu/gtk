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
	/* RPC */
	{"rpc_run", (PyCFunction)Context_rpc_run, METH_VARARGS | METH_KEYWORDS,
		"c.rpc_run(service, object_path, interface, method[, rpc_args[, wait_reply[, system_bus]]]) -> object\n\n"
		"Run a RPC method with arguments inside rpc_args tuple.\n\n"
		"Usage example:\n\n"
		"\tc.rpc_run('com.nokia.backup', '/com/nokia/backup', 'com.nokia.backup', 'backup_finish', True)\n"},
	{"rpc_run_with_defaults", (PyCFunction)Context_rpc_run_with_defaults, METH_VARARGS | METH_KEYWORDS,
		"c.rpc_run_with_defaults(application, method[, rpc_args, wait_reply]) -> object\n"
		"\n"
		"Run a RPC method using default parameters.\n"
		"\n"
		"Usage example:\n\n"
		"\tc.rpc_run_with_defaults('tn_mail', 'send_recv_button_focus', (status,)) # status is a int var\n"},
	{"rpc_async_run", (PyCFunction)Context_rpc_async_run, METH_VARARGS | METH_KEYWORDS,
		"c.rpc_async_run(service, object_path, interface, method, callback[, user_data[, rpc_args]])\n\n"
		"Run a RPC method and call 'callback' after finished.\n\n"
		"Usage example:\n\n"
		"\tdef my_func(interface, method, user_data=None):\n"
		"\t\tpass\n\n"
		"\tc.rpc_async_run('com.nokia.backup', '/com/nokia/backup', 'com.nokia.backup', 'backup_finish', my_func, 'hello!', (True,))\n"},
	{"rpc_async_run_with_defaults", (PyCFunction)Context_rpc_async_run_with_defaults, METH_VARARGS | METH_KEYWORDS,
		"c.rpc_async_run_with_defaults(application, method, callback[, user_data[, rpc_args]])\n\n"
		"Run a RPC method using default parameters and call 'callback' after finished.\n\n"
		"Usage example:\n\n"
		"\tdef my_func(interface, method, user_data=None):\n"
		"\t\tpass\n\n"
		"\tc.rpc_async_run_with_defaults('tn_mail', 'send_recv_button_focus', my_func, 'hello!', (True,))\n"},
	{"set_rpc_callback", (PyCFunction)Context_set_rpc_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_rpc_callback(service, object_path, interface, callback[, user_data])\n\n"
		"This method registers a callback function for handling RPC calls to a\n"
		"given object of a service. Use None in callback parameter to unset this\n"
		"callback. The callback will receive the parameters: interface, method,\n"
		"arguments, user_data.\n"},
	{"set_rpc_default_callback", (PyCFunction)Context_set_rpc_default_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_rpc_default_callback(callback[, user_data])\n\n"
		"This method registers a callback function for handling RPC calls to\n"
		"the default service of the application. The default service is\n"
		"'"OSSO_BUS_ROOT".A' where A is the application's name of the context.\n"
		"Use None in callback parameter to unset this callback. The callback\n"
		"will receive the parameters: interface, method, arguments,\n"
		"user_data.\n"},
	{"get_rpc_timeout", (PyCFunction)Context_get_rpc_timeout, METH_NOARGS,
		"c.get_rpc_timeout() -> int\n\nReturn the timeout value used by RPC methods."},
	{"set_rpc_timeout", (PyCFunction)Context_set_rpc_timeout, METH_VARARGS,
		"c.set_rpc_timeout(timeout)\n\nSet the timeout value used by RPC methods."},
	/* Applications */
	{"application_top", (PyCFunction)Context_application_top, METH_VARARGS | METH_KEYWORDS,
		"c.application_top(application, arguments) -> object\n"
		"\n"
		"This method tops an application. If the application is not already\n"
		"running, D-BUS will launch it via the auto-activation mechanism."},
	/* Autosaving */
	{"set_autosave_callback", (PyCFunction)Context_set_autosave_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_autosave_callback(callback[, user_data])\n\n"
		"This method registers an autosave callback function. Use None in\n"
		"callback parameter to unset this callback. The callback will receive a\n"
		"parameter with user_data.\n"},
	{"userdata_changed", (PyCFunction)Context_userdata_changed, METH_NOARGS,
		"c.userdata_changed()\n\n"
		"This method is called by the application when the user data has been\n"
		"changed, so that Libosso knows that a call to the autosave callback is\n"
		"needed in the future to save the user data. The dirty state will be\n"
		"cleared every time the application's autosave callback function is\n"
		"called."},
	{"force_autosave", (PyCFunction)Context_force_autosave, METH_NOARGS,
		"c.force_autosave()\n\n"
		"This method forces a call to the application's autosave function, and\n"
		"resets the autosave timeout. The application should call this method\n"
		"whenever it is switched to background (untopped)."},
	{"get_name", (PyCFunction)Context_get_name, METH_NOARGS, "c.get_name() -> string\n\nReturn the application name.\n"},
	{"get_version", (PyCFunction)Context_get_version, METH_NOARGS, "c.get_version() -> string\n\nReturn the application version.\n"},
	/* Statusbar */
	{"statusbar_send_event", (PyCFunction)Context_statusbar_send_event, METH_VARARGS | METH_KEYWORDS,
		"c.statusbar_send_event(name, int_arg1, int_arg2, string_arg3)\n\n"
		"Send an event to statusbar with int_arg1, int_arg2 and string_arg3\n"
		"parameters."},
	/* Time */
	{"set_time_notification_callback", (PyCFunction)Context_set_time_notification_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_time_notification_callback(callback[, user_data])\n\n"
		"This method registers an callback function that is called when the\n"
		"system time is changed. The callback will receive a parameter with\n"
		"user_data.\n"},
	{"set_time", (PyCFunction)Context_set_time, METH_VARARGS,
		"c.set_time(time_sequence)\n\n"
		"This method sets the system and hardware time, and notifies about the\n"
		"changing of time over the D-BUS system bus. Notice: does not currently\n"
		"change the time. The time_sequence parameter is a sequence with 9\n"
		"elements (like sequences returned by time standard Python module). These\n"
		"elements are (year, mon, month_day, hour, min, sec, week_day, year_day,\n"
		"is_daylight_saving)."},
	/* System Notification */
	{"system_note_dialog", (PyCFunction)Context_system_note_dialog, METH_VARARGS | METH_KEYWORDS,
		"c.system_note_dialog(message[, type='notice']) -> object\n"
		"\n"
		"This method requests that a system note (a window that is modal to the whole system)\n"
		"is shown. Application that do have a GUI should not use this method but the hildon_note\n"
		"widget directly. The \"type\" argument should be 'warning', 'error', 'wait' or 'notice'."},
	{"system_note_infoprint", (PyCFunction)Context_system_note_infoprint, METH_VARARGS | METH_KEYWORDS,
		"c.system_note_infoprint(message) -> object\n"
		"\n"
		"This method requests that the statusbar shows an infoprint (aka information banner).\n"
		"This allow non-GUI applications to display some information to the user.\n"
		"Application that do have a GUI should not use this method but the gtk_infoprint\n"
		"widget directly."},
	/* State saving */
	{"state_write", (PyCFunction)Context_state_write, METH_VARARGS | METH_KEYWORDS,
		"c.state_write(object)\n\n"
		"This method write a state object to disk. Any existing files will be\n"
		"overwritten."},
	{"state_read", (PyCFunction)Context_state_read, METH_VARARGS | METH_KEYWORDS,
		"c.state_read() -> object\n\n"
		"This method read a state object from disk.\n"},
	/* Plugins */
	{"plugin_execute", (PyCFunction)Context_plugin_execute, METH_VARARGS | METH_KEYWORDS,
		"c.plugin_execute(filename, user_activated[, user_data])\n\n"
		"Calls the execute() function of a plugin. The filename parameter is\n"
		"shared object (.so) file of the plugin. It should include the '.so'\n"
		"sufix, but not a path. The user_activated is True if the plugin was\n"
		"activated by a user (as opposed to activated by restoring software\n"
		"state) and the user_data parameteris the GTK top-level widget.\n"
		"It is needed so that the widgets created by plugin can be made a child\n"
		"of the main application that utilizes the plugin."},
	{"plugin_save_state", (PyCFunction)Context_plugin_save_state, METH_VARARGS | METH_KEYWORDS,
		"c.plugin_save_state(filename, user_activated[, user_data])\n\n"
		"This method is used to tell a plugin to save its state."},
	/* Device state */
	{"display_state_on", (PyCFunction)Context_display_state_on, METH_VARARGS | METH_KEYWORDS,
		"c.display_state_on()\n\n"
		"Request to turn on the display as if the user had pressed a key or the\n"
		"touch screen. This can be used after completing a long operation such as\n"
		"downloading a large file or after retrieving e-mails."},
	{"display_blanking_pause", (PyCFunction)Context_display_blanking_pause, METH_VARARGS | METH_KEYWORDS,
		"c.display_blanking_pause()\n\n"
		"Request not to blank the display. This method must be called again\n"
		"within 60 seconds to renew the request. The method is used, for\n"
		"example, by the video player during video playback. Alson prevents\n"
		"suspending the device."},
	{"set_device_state_callback", (PyCFunction)Context_set_device_state_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_device_callback(callback[, user_data[, shutdown[, save_data\n"
		"    [, memory_low[, system_inactivity[, mode]]]]]]\n"
		"  )\n\n"
		"This method registers a callback function that is called whenever the\n"
		"state device changes. The first call to this method will also check\n"
		"the current state of the device, and if the state is available, the\n"
		"corresponding callback function will be called immediately. Callback\n"
		"function will receive the shutdown, save_data, memory_low,\n"
		"system_inactivity, mode and user_data parameters with state device. If\n"
		"you specify shutdown, save_data, memory_low, system_inactivity\n"
		"(booleans) or mode ('normal', 'flight', 'invalid', 'offline') the\n"
		"callback function will be called only when this state changes. Use\n"
		"None in callback parameter to unset this callback."},
	/* MIME */
	{"set_mime_callback", (PyCFunction)Context_set_mime_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_mime_callback(callback[, user_data])\n\n"
		"This method registers a MIME callback function that Libosso calls\n"
		"when the user wants the application to open file(s) of a MIME type\n"
		"handled by the application.Use in callback parameter to unset this\n"
		"callback. The callback will receive a parameter with a list of URIs and\n"
		"user_data.\n"},
	/* Others */
	/*
	{"tasknav_mail_add", (PyCFunction)Context_tasknav_mail_add, METH_VARARGS | METH_KEYWORDS,
		"c.tasknav_mail_add(id, subject, sender, has_attachment, date)\n\n"
		"This method adds a mail to be displayed in the mail popup of the\n"
		"tasknavigator."},
	{"tasknav_mail_del", (PyCFunction)Context_tasknav_mail_del, METH_VARARGS | METH_KEYWORDS,
		"c.tasknav_mail_add(id)\n\n"
		"This method removes an email from mail popup of the tasknavigator."},
	{"tasknav_mail_set_outbox_count", (PyCFunction)Context_tasknav_mail_set_outbox_count, METH_VARARGS | METH_KEYWORDS,
		"c.tasknav_mail_set_outbox_count(count)\n\n"
		"This method updates the outbox message count in the task navigator mail\n"
		"window."},
	*/
  	{"set_exit_callback", (PyCFunction)Context_set_exit_callback, METH_VARARGS | METH_KEYWORDS,
		"c.set_exit_callback(callback[, user_data])\n\n"
		"This method registers the application's exit callback function. when\n"
		"Libosso calls the application's exit callback function, the application\n"
		"should save its GUI state and unsaved user data and exit as soon as\n"
		"possible. The callback will receive user_data."},
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
initosso(void)
{
	PyObject *module;

	/* prepare types */
	ContextType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&ContextType) < 0) {
		return;
	}

	/* initialize module */
	module = Py_InitModule3("osso", osso_methods,
			"FIXME: put documentation about RPC, Application, Autosave, Statusbar, etc.");

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
