/**
 * osso-misc.c
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

static PyObject *exit_callback = NULL;

static void
_wrap_exit_callback_handler(gboolean die_now, gpointer data)
{
	PyObject *py_args = NULL;
	PyGILState_STATE state;

	state = PyGILState_Ensure();

	if (exit_callback == NULL) {
		return;
	}

	py_args = Py_BuildValue("(O)", data);
	PyEval_CallObject(exit_callback, py_args);

	PyGILState_Release(state);
	return;
}


PyObject *
Context_set_exit_callback(Context *self, PyObject *args, PyObject *kwds)
{
	PyObject *py_func = NULL;
	PyObject *py_data = NULL;
	osso_return_t ret;

	static char *kwlist[] = { "callback", "user_data", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"O|O:Context.set_exit_callback", kwlist, &py_func,
				&py_data)) {
		return NULL;
	}

	if (py_func != Py_None) {
		if (!PyCallable_Check(py_func)) {
			PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
			return NULL;
		}
		Py_XINCREF(py_func);
		Py_XDECREF(exit_callback);
		exit_callback = py_func;
	} else {
		Py_XDECREF(exit_callback);
		exit_callback = NULL;
	}

	if (exit_callback != NULL) {
		ret = osso_application_set_exit_cb(self->context,
					_wrap_exit_callback_handler, py_data);
	} else {
		/* FIXME: Where is osso_time_unset_notification_cb function? Is this the
		 * correct way to unset this handler? */
		ret = osso_application_set_exit_cb(self->context, NULL, py_data);
	}

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}

/*
PyObject *
Context_tasknav_mail_add(Context *self, PyObject *args, PyObject *kwds)
{
	guint id = 0;
	const char *subject = NULL;
	const char *sender = NULL;
	const char *date = NULL;
	gboolean has_attachment = 0;

	osso_return_t ret;

	static char *kwlist[] = {"id", "subject", "sender", "has_attachment", "date", 0};

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"issbs:Context.tasknav_mail_add", kwlist, &id, &subject,
				&sender, &has_attachment, &date)) {
		return NULL;
	}

	ret = osso_tasknav_mail_add(self->context, id, subject, sender, has_attachment, date);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


PyObject *
Context_tasknav_mail_del(Context *self, PyObject *args, PyObject *kwds)
{
	guint id = 0;

	osso_return_t ret;

	static char *kwlist[] = {"id", 0};

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"i:Context.tasknav_mail_del", kwlist, &id)) {
		return NULL;
	}

	ret = osso_tasknav_mail_del(self->context, id);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


PyObject *
Context_tasknav_mail_set_outbox_count(Context *self, PyObject *args, PyObject *kwds)
{
	guint count = 0;

	osso_return_t ret;

	static char *kwlist[] = {"count", 0};

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"i:Context.tasknav_mail_set_outbox_count", kwlist, &count)) {
		return NULL;
	}

	ret = osso_tasknav_mail_set_outbox_count(self->context, count);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}
*/

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
