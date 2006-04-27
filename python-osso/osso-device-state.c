/**
 * osso-device-state.c
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

static PyObject *device_state_callback = NULL;

static void
_wrap_device_state_callback_handler(osso_hw_state_t *hw_state, void *data)
{
	PyObject *py_args = NULL;
	PyGILState_STATE state;
	char *hw_state_str;

	state = PyGILState_Ensure();

	if (device_state_callback == NULL) {
		return;
	}

	switch (hw_state->sig_device_mode_ind) {
		case OSSO_DEVMODE_NORMAL:
			hw_state_str = "normal";
			break;
		case OSSO_DEVMODE_FLIGHT:
			hw_state_str = "flight";
			break;
		case OSSO_DEVMODE_OFFLINE:
			hw_state_str = "offline";
			break;
		case OSSO_DEVMODE_INVALID:
			hw_state_str = "invalid";
			break;
		default:
			hw_state_str = "";
	}

	py_args = Py_BuildValue("(bbbbsO)", 
			hw_state->shutdown_ind,
			hw_state->save_unsaved_data_ind,
			hw_state->memory_low_ind,
			hw_state->system_inactivity_ind,
			hw_state_str,
			data);

	PyEval_CallObject(device_state_callback, py_args);

	PyGILState_Release(state);
	return;
}


PyObject *
Context_set_device_state_callback(Context *self, PyObject *args, PyObject *kwds)
{
	PyObject *py_func = NULL;
	PyObject *py_data = NULL;
	char shutdown = 0;
	char save_data = 0;
	char memory_low = 0;
	char system_inactivity = 0;
	char *mode = NULL;
	osso_hw_state_t hw_state;
	osso_return_t ret;

	static char *kwlist[] = { "callback", "user_data", "shutdown", "save_data",
		"memory_low", "system_inactivity", "mode", 0 };

	if (!_check_context(self->context)) return 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds,
				"O|bbbbsO:Context.set_device_state_callback", kwlist,
				&py_func, &shutdown, &save_data, &memory_low,
				&system_inactivity, &mode, &py_data)) {
		return NULL;
	}

	if (py_func != Py_None) {
		if (!PyCallable_Check(py_func)) {
			PyErr_SetString(PyExc_TypeError, "callback parameter must be callable");
			return NULL;
		}
		Py_XINCREF(py_func);
		Py_XDECREF(device_state_callback);
		device_state_callback = py_func;
	} else {
		Py_XDECREF(device_state_callback);
		device_state_callback = NULL;
	}
	
	hw_state.shutdown_ind = shutdown;
	hw_state.save_unsaved_data_ind = save_data;
	hw_state.memory_low_ind = memory_low;
	hw_state.system_inactivity_ind = system_inactivity;
	
	if (!strcasecmp(mode, "normal")) {
		hw_state.sig_device_mode_ind = OSSO_DEVMODE_NORMAL;
	} else if (!strcasecmp(mode, "flight")) {
		hw_state.sig_device_mode_ind = OSSO_DEVMODE_FLIGHT;
	} else if (!strcasecmp(mode, "offline")) {
		hw_state.sig_device_mode_ind = OSSO_DEVMODE_OFFLINE;
	} else if (!strcasecmp(mode, "invalid")) {
		hw_state.sig_device_mode_ind = OSSO_DEVMODE_INVALID;
	} else {
		PyErr_SetString(OssoException, "Invalid device mode. Use 'normal',"
				"'flight', 'offline' or 'invalid' instead.");
		return NULL;
	}

	if (device_state_callback != NULL) {
		ret = osso_hw_set_event_cb(self->context, &hw_state,
				_wrap_device_state_callback_handler, py_data);
	} else {
		ret = osso_hw_unset_event_cb(self->context, &hw_state);
	}

	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


PyObject *
Context_display_state_on(Context *self)
{
	osso_return_t ret = OSSO_OK;

	if (!_check_context(self->context)) return 0;

	ret = osso_display_state_on(self->context);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}


PyObject *
Context_display_blanking_pause(Context *self)
{
	osso_return_t ret = OSSO_OK;

	if (!_check_context(self->context)) return 0;

	ret = osso_display_blanking_pause(self->context);
	if (ret != OSSO_OK) {
		_set_exception(ret, NULL);
		return NULL;
	}

	Py_RETURN_NONE;
}

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
