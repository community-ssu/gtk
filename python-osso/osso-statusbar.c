/**
 * osso-statusba.c
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
Context_statusbar_send_event(Context *self, PyObject *args, PyObject *kwds)
{
	const char *name;
	gint arg1;
	gint arg2;
	const char *arg3;
	osso_return_t ret;
	osso_rpc_t retval;

	static char *kwlist[] = {"name", "arg1", "arg2", "arg3", 0};

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "siis:Context.statusbar_send_event", kwlist, &name, &arg1, &arg2, &arg3)) {
		return NULL;
	}

	ret = osso_statusbar_send_event(self->context, name, arg1, arg2, arg3, &retval);
	if (ret != OSSO_OK) {
		_set_exception(ret, &retval);
		return NULL;
	}

	return _rpc_t_to_python(&retval);
}

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
