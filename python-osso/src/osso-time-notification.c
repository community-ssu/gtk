/**
 * osso-time-notification.c
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

static PyObject *time_notification_callback = NULL;

/* ----------------------------------------------- */
/* TimeNotification type default methods                    */
/* ----------------------------------------------- */

PyObject *
TimeNotification_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Context *self;

	self = (Context *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->context = NULL;
	}

	return (PyObject *)self;
}


int
TimeNotification_init(Context *self, PyObject *args, PyObject *kwds)
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
TimeNotification_close(Context *self)
{
	if (!_check_context(self->context)) return 0;
	self->context = NULL;
	Py_RETURN_NONE;
}


void
TimeNotification_dealloc(Context *self)
{
	if (self->context == NULL)
		return;

	self->context = NULL;
	return;
}


static struct PyMethodDef TimeNotification_methods[] = {
	/* Time */
	{"set_time_notification_callback", (PyCFunction)Context_set_time_notification_callback, METH_VARARGS | METH_KEYWORDS,
		"c.time_notification.set_time_notification_callback(callback[, user_data])\n\n"
		"This method registers an callback function that is called when the\n"
		"system time is changed. The callback will receive a parameter with\n"
		"user_data.\n"},
	{"set_time", (PyCFunction)Context_set_time, METH_VARARGS,
		"c.time_notification.set_time(time_sequence)\n\n"
		"This method sets the system and hardware time, and notifies about the\n"
		"changing of time over the D-BUS system bus. Notice: does not currently\n"
		"change the time. The time_sequence parameter is a sequence with 9\n"
		"elements (like sequences returned by time standard Python module). These\n"
		"elements are (year, mon, month_day, hour, min, sec, week_day, year_day,\n"
		"is_daylight_saving)."},
	/* Default */
	{"close", (PyCFunction)TimeNotification_close, METH_NOARGS, "Close TimeNotification context."},
	{0, 0, 0, 0}
};

static PyTypeObject TimeNotificationType = {
	PyObject_HEAD_INIT(NULL)
	0,																/* ob_size */
	"osso.TimeNotification",													/* tp_name */
	sizeof(Context),												/* tp_basicsize */
	0,																/* tp_itemsize */
	(destructor)TimeNotification_dealloc,									/* tp_dealloc */
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
	"OSSO TimeNotification class",											/* tp_doc */
	0,																/* tp_traverse */
	0,																/* tp_clear */
	0,																/* tp_richcompare */
	0,																/* tp_weaklistoffset */
	0,																/* tp_iter */
	0,																/* tp_iternext */
	TimeNotification_methods,												/* tp_methods */
	0,																/* tp_members */
	0,																/* tp_getset */
	0,																/* tp_base */
	0,																/* tp_dict */
	0,																/* tp_descr_get */
	0,																/* tp_descr_set */
	0,																/* tp_dictoffset */
	(initproc)TimeNotification_init,											/* tp_init */
	0,																/* tp_alloc */
	TimeNotification_new,													/* tp_new */
};

static struct PyMethodDef osso_methods[] = {
	{0, 0, 0, 0}
};

PyMODINIT_FUNC
inittime_notification(void)
{
	PyObject *module;

	/* prepare types */
	TimeNotificationType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&TimeNotificationType) < 0) {
		return;
	}

	/* initialize module */
	module = Py_InitModule3("time_notification", osso_methods,
			"FIXME: put documentation about TimeNotification module.");

	/* add types */
	Py_INCREF(&TimeNotificationType);
	PyModule_AddObject(module, "TimeNotification", (PyObject *)&TimeNotificationType);

	/* add contants */
	/* : */
	/* : */
	/* : */
}


static void
_wrap_time_notification_callback_handler(gpointer data)
{
	PyObject *py_args = NULL;
	PyGILState_STATE state;

	state = PyGILState_Ensure();

	if (time_notification_callback == NULL) {
		return;
	}
	
	py_args = Py_BuildValue("(O)", data);
	PyEval_CallObject(time_notification_callback, py_args);

	PyGILState_Release(state);
	return;
}


PyObject *
Context_set_time_notification_callback(Context *self, PyObject *args, PyObject *kwds)
{
	PyObject *py_func = NULL;
	PyObject *py_data = NULL;
	osso_return_t ret = OSSO_OK;

	static char *kwlist[] = { "callback", "user_data", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"O|O:Context.set_time_notification_callback", kwlist, &py_func,
				&py_data)) {
		return NULL;
	}

	if (py_func != Py_None) {
		if (!PyCallable_Check(py_func)) {
			PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
			return NULL;
		}
		Py_XINCREF(py_func);
		Py_XDECREF(time_notification_callback);
		time_notification_callback = py_func;
	} else {
		Py_XDECREF(time_notification_callback);
		time_notification_callback = NULL;
	}

	if (time_notification_callback != NULL) {
		ret = osso_time_set_notification_cb(self->context,
				_wrap_time_notification_callback_handler, py_data);
	} else {
		/* FIXME: Where is osso_time_unset_notification_cb function? Is this the
		 * correct way to unset this handler? */
		ret = osso_time_set_notification_cb(self->context, NULL, py_data);
	}

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


PyObject *
Context_set_time(Context *self, PyObject *args)
{
	time_t epoch;
	struct tm time;
	osso_return_t ret;

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTuple(args,
				"(iiiiiiiii):Context.set_time",
					&time.tm_year, &time.tm_mon, &time.tm_mday,
					&time.tm_hour, &time.tm_min, &time.tm_sec,
					&time.tm_wday, &time.tm_yday, &time.tm_isdst)) {
		return NULL;
	}

	/* remove python adjusts */
	time.tm_year = time.tm_year - 1900;
	time.tm_mon--;

	epoch = mktime(&time);
	if (epoch == -1) {
		PyErr_SetString(PyExc_TypeError, "Invalid date/time.");
		return NULL;
	}
	
	ret = osso_time_set(self->context, epoch);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}
	
	Py_RETURN_NONE;
}

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
