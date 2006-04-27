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
