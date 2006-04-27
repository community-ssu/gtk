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
