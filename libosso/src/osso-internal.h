/**
 * @file osso-internal.h
 * This file is the private include for all functions.
 * 
 * Copyright (C) 2005-2006 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License.
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

#ifndef OSSO_INTERNAL_H_
# define OSSO_INTERNAL_H_

#define DBUS_API_SUBJECT_TO_CHANGE

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

# include <stdio.h>
# include <stdarg.h>
# include <string.h>
# include <malloc.h>

# include "libosso.h"
# include "osso-log.h"
# include <dbus/dbus.h>
# include <dbus/dbus-glib-lowlevel.h>


# define OSSO_BUS_HOME		"home"
# define OSSO_BUS_TASKNAV	"tasknav"
# define OSSO_BUS_CTRLPAN	"controlpanel"
# define OSSO_BUS_STATUSBAR     "statusbar"

# define OSSO_BUS_ROOT		"com.nokia"
# define OSSO_BUS_ROOT_PATH	"/com/nokia"

# define OSSO_BUS_ACTIVATE	"activate"
# define OSSO_BUS_MIMEOPEN	"mime_open"
# define OSSO_BUS_TOP		"top_application"
# define OSSO_BUS_ADD_TO_HOME   "add_to_home"
# define OSSO_BUS_SYSNOTE       "system_note_dialog"
# define OSSO_BUS_INFOPRINT     "system_note_infoprint"
# define OSSO_BUS_TOP_REQUEST   "top_request"

# define OSSO_RPC_REPLY_TIMEOUT -1

/* DBus interface, service, and object path maximum lengths */
#define MAX_IF_LEN 255
#define MAX_SVC_LEN 255
#define MAX_OP_LEN 255

#define MAX_APP_NAME_LEN 50
#define MAX_VERSION_LEN 30

typedef DBusHandlerResult (_osso_interface_cb_f)(osso_context_t *osso,
						 DBusMessage *msg,
						 gpointer data);

typedef struct {
    osso_hw_cb_f *cb;
    gpointer data;
    gboolean set;
}_osso_hw_cb_data_t;

typedef struct {
  _osso_hw_cb_data_t shutdown_ind;
  _osso_hw_cb_data_t save_unsaved_data_ind;
  _osso_hw_cb_data_t memory_low_ind;
  _osso_hw_cb_data_t system_inactivity_ind;
  _osso_hw_cb_data_t sig_device_mode_ind;
}_osso_hw_cb_t;

typedef struct {
    osso_mime_cb_f *func;
    gpointer data;
}_osso_mime_t;

/**
 * This structure is used to store autosave information
 */
typedef struct {
    osso_application_autosave_cb_f *func; /**< A pointer to the autosave
					   * function */
    gpointer data; /**< An application specific data paointer */
    guint id; /**< The timeout event id, returned by GLib */
}_osso_autosave_t;

typedef struct {
    _osso_interface_cb_f *handler;
    gchar *interface;
    gpointer data;
    gboolean method;
    gboolean can_free_data;
}_osso_interface_t;

typedef struct {
    void *lib;
    gchar *name;
    gchar *svcname;
}_osso_cp_plugin_t;

typedef struct {
    osso_application_exit_cb *cb;
    gpointer data;
}_osso_exit_t;

/**
 * This structure is used to store library specific stuff
 */
struct osso_af_context_t {
    DBusConnection *conn;
    DBusConnection *sys_conn;
    DBusConnection *cur_conn;
    gchar application[MAX_APP_NAME_LEN + 1];
    gchar version[MAX_VERSION_LEN + 1];
    char object_path[MAX_OP_LEN + 1];
    char interface[MAX_IF_LEN + 1];
    GArray *ifs;
    _osso_autosave_t autosave;
    guint log_handler;
    _osso_hw_cb_t hw_cbs;
    osso_hw_state_t hw_state;
    guint rpc_timeout;
    _osso_mime_t mime;
    GArray *cp_plugins;
    _osso_exit_t exit;
};

# ifdef LIBOSSO_DEBUG
#  define dprint(f, a...) (fprintf(stderr, "%s:%d %s(): "f"\n", __FILE__, \
				     __LINE__, __func__, ##a),fflush(stderr))
# else
#  define dprint(f, a...)
# endif /* LIBOSSO_DEBUG */

/**
 * This is the first-level message handler. it checks to see if the message
 * we receive is of any interest to us, if it is the corresponding callback
 * function is called, or if the message is a return for something we are
 * interested in.
 * @param conn The dbus connection that should be used to send replies.
 * @param msg The dbus message.
 * @param data An osso_context_t structure.
 */
DBusHandlerResult _msg_handler(DBusConnection *conn, DBusMessage *msg, 
			       void *data);
void _msg_handler_set_cb_f(osso_context_t *osso, const gchar *interface,
			   _osso_interface_cb_f *cb, gpointer data, 
			   gboolean method);
void _msg_handler_set_cb_f_free_data(osso_context_t *osso, const gchar *interface,
				     _osso_interface_cb_f *cb, gpointer data, 
				     gboolean method);
gpointer _msg_handler_rm_cb_f(osso_context_t *osso, const gchar *interface,
			      _osso_interface_cb_f *cb, gboolean method);
void _msg_handler_set_ret(osso_context_t *osso, gint serial,
			  osso_rpc_t *retval);
void _msg_handler_rm_ret(osso_context_t *osso, gint serial);

void __attribute__ ((visibility("hidden")))
make_default_interface(const char *application, char *interface);

void __attribute__ ((visibility("hidden")))
make_default_service(const char *application, char *service);

/* this is only needed by some unit testing code */
osso_return_t _test_rpc_set_cb_f(osso_context_t *osso, const gchar *service,
                                const gchar *object_path,
			        const gchar *interface,
			        osso_rpc_cb_f *cb, gpointer data,
			        gboolean use_system_bus);

gchar* appname_to_valid_path_component(const gchar *application);
gboolean validate_appname(const gchar *application);

/**
 * This internal function performs a simple validation for the application
 * and version information of the osso_context regarding their validity
 * as components of the filesystem (no slashes, value not NULL etc)
 * @param osso The osso context containing the application name and
 * version information
 * @return TRUE if the context passes the validation, FALSE otherwise.
 */
gboolean validate_osso_context(const osso_context_t * osso);

#define _launch_app(osso, application) \
    osso_rpc_run_with_defaults(osso, application, \
				   OSSO_BUS_ACTIVATE, NULL, NULL);

void _close_all_plugins(osso_context_t *osso);

#endif /* OSSO_INTERNAL_H_ */
