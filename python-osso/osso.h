/**
 * osso.h
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

#ifndef __OSSO_H__
#define __OSSO_H__

#include <Python.h>
#include <marshal.h>
#include <time.h>
#include <libosso.h>
#include <pygtk/pygtk.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

PyObject *OssoException;
PyObject *OssoRPCException;
PyObject *OssoInvalidException;
PyObject *OssoNameException;
PyObject *OssoNoStateException;
PyObject *OssoStateSizeException;

/* Default values for .._with_defaults functions */
#define MAX_IF_LEN 255
#define MAX_SVC_LEN 255
#define MAX_OP_LEN 255
#define OSSO_BUS_ROOT      "com.nokia"
#define OSSO_BUS_ROOT_PATH "/com/nokia"

/* Helper functions */
char *appname_to_valid_path_component(char *application);
PyObject *_rpc_t_to_python(osso_rpc_t *arg);
void _python_to_rpc_t(PyObject *py_arg, osso_rpc_t *rpc_arg);
PyObject *_rpc_args_c_to_py(GArray *args);
void _argfill(DBusMessage *msg, void *raw_tuple);

/* Context */
typedef struct {
	PyObject_HEAD
	osso_context_t *context;
} Context;

/* Context type default methods */
char _check_context(osso_context_t *context);
void _set_exception(osso_return_t err, osso_rpc_t *retval);
PyObject *Context_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
int Context_init(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_close(Context *self);
void Context_dealloc(Context *self);

/* RPC */
PyObject *Context_rpc_run(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_rpc_run_with_defaults(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_rpc_async_run(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_rpc_async_run_with_defaults(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_set_rpc_callback(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_set_rpc_default_callback(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_get_rpc_timeout(Context *self);
PyObject *Context_set_rpc_timeout(Context *self, PyObject *args);

/* Applications */
PyObject *Context_application_top(Context *self, PyObject *args, PyObject *kwds);

/* Autosaving */
PyObject *Context_set_autosave_callback(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_userdata_changed(Context *self);
PyObject *Context_force_autosave(Context *self);
PyObject *Context_get_name(Context *self);
PyObject *Context_get_version(Context *self);

/* Statusbar */
PyObject *Context_statusbar_send_event(Context *self, PyObject *args, PyObject *kwds);

/* Time */
PyObject *Context_set_time_notification_callback(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_set_time(Context *self, PyObject *args);

/* System notification */
PyObject *Context_system_note_dialog(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_system_note_infoprint(Context *self, PyObject *args, PyObject *kwds);

/* State saving */
PyObject *Context_state_write(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_state_read(Context *self);

/* Plugins */
PyObject *Context_plugin_execute(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_plugin_save_state(Context *self, PyObject *args, PyObject *kwds);

/* Device State */
PyObject *Context_display_state_on(Context *self);
PyObject *Context_display_blanking_pause(Context *self);
PyObject *Context_set_device_state_callback(Context *self, PyObject *args, PyObject *kwds);

/* MIME types */
PyObject *Context_set_mime_callback(Context *self, PyObject *args, PyObject *kwds);

/* Others */
/*
PyObject *Context_tasknav_mail_add(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_tasknav_mail_del(Context *self, PyObject *args, PyObject *kwds);
PyObject *Context_tasknav_mail_set_outbox_count(Context *self, PyObject *args, PyObject *kwds);
*/
PyObject *Context_set_exit_callback(Context *self, PyObject *args, PyObject *kwds);

#endif

/* vim:ts=4:noet:sw=4:sws=4:si:ai:showmatch:foldmethod=indent
 */
