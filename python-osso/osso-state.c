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
