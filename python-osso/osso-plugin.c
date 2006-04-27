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
