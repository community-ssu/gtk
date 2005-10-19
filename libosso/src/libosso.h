/**
 * @file libosso.h
 * This file is the public API for the Libosso library.
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
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

#ifndef LIBOSSO_H_
#define LIBOSSO_H_

#include <glib.h>
#include <time.h>
#include <syslog.h>

#include <dbus/dbus-protocol.h>

G_BEGIN_DECLS


/**
 * This enum represents the return values that most Libosso
 * functions use.*/
typedef enum {
  OSSO_OK = 0, /**<The function executed normally. */
  OSSO_ERROR = -1, /**<Some kind of an error occured. */
  OSSO_INVALID = -2, /**<At least one parameter is invalid. */
  OSSO_RPC_ERROR = -4, /**< Osso RPC method returned an error. */
  OSSO_ERROR_NAME = -3,
  OSSO_ERROR_NO_STATE = -5, /**< No state file found to read. */
  OSSO_ERROR_STATE_SIZE = -6 /**< The size of the given structure is
			    * different from the saved size */
} osso_return_t;


/**
 * \defgroup Init Initialization
 *
 * @{
 *
 * This is the return type for the osso_initialize function.
 */
typedef struct osso_af_context_t osso_context_t;

/**
 * This function initializes the library, connects to the D-BUS session
 * and system busses, integrates with the GLib main loop, and
 * initializes the library for use. #osso_initialize should be called
 * only once by the application.
 * @param application The name of the application.
 * This name forms the last part of the default (D-BUS) service name of the
 * application.
 * The only valid characters that the name may contain are letters a-z and
 * the underscore '_'.
 * @param version The version string of the application. It must be 
 * comparable with strcmp().
 * @param activation If this is TRUE, then the library assumes that the
 * application binary has been launched by the D-BUS daemon, and thus will
 * connect to the D-BUS activation bus (the D-BUS bus where the D-BUS message
 * that resulted in auto-activation of the application came -- either the
 * session or system bus). This parameter should always be FALSE and
 * considered obsolete, because otherwise Libosso might behave strangely
 * since it normally uses both session and system bus.
 * @param context The GLib main loop context to connect to, or NULL for
 * the default context.
 * @return A context to use in later calls to this library. NULL is
 * returned if an error happened.
 */
osso_context_t * osso_initialize(const gchar *application,
				 const gchar* version,
				 gboolean activation,
				 GMainContext *context);
/**
 * This function closes the message bus connections and frees all memory
 * allocated by the Libosso context. This function renders the context to 
 * unusable state. #osso_initialize has to be called again to get a valid
 * Libosso context.
 * @param osso The library reference as returned by #osso_initialize.
 */
void osso_deinitialize(osso_context_t *osso);

/* @} */
/************************************************************************/
/**
 * \defgroup RPC Remote Procedure Calls (RPC)
 *
 * These functions provide wrappers for D-BUS message passing.
 */
/* @{*/

/**
 * The argument element used in the generic RPC functions is a GArray of 
 * these #osso_rpc_t structures.
 */
typedef struct {
  int type; /**<The type of argument */
  union {
    guint32 u; /**<Type is DBUS_TYPE_UINT32 */
    gint32 i; /**<Type is DBUS_TYPE_INT32 */
    gboolean b; /**<Type is DBUS_TYPE_BOOLEAN */
    gdouble d; /**<Type is DBUS_TYPE_DOUBLE */
    gchar *s; /**<Type is DBUS_TYPE_STRING */
  } value; /**<The way the param is interpreted depends on the #type field.*/
}
osso_rpc_t;

/**
 * This function frees the contents of the #osso_rpc_t structure
 * pointed to be #rpc.  (It does not free the structure itself.)  You
 * need to call this for structures filled by #osso_rpc_run and
 * related functions.
 *
 * This function will call #g_free to free the memory pointed to by
 * #rpc->value.s when #rpc->type is #DBUS_TYPE_STRING.  This guarantee
 * allows you to use this function as the #retval_free parameter for
 * #osso_rpc_set_cb_f, etc, when you get that string from #g_strdup,
 * #g_strdup_printf, etc.
 *
 * @param rpc  The structure whose contents is to be freed.
 */
void osso_rpc_free_val (osso_rpc_t *rpc);

/**
 * This is the type for the generic RPC function. This function is called
 * every time a method is requested on the registered interface.
 * @param interface The interface that the method is called on.
 * @param method The method that is called.
 * @param arguments A GArray of #osso_rpc_t structures.  This array
 * and the osso_rpc_t values in it are only valid until the callback
 * returns.
 * @param retval The return value of the method. This should be set to
 * DBUS_TYPE_INVALID for no reply.  See #osso_rpc_set_cb_f and
 * #osso_rpc_free_val for how the memory associated with #retval is
 * managed.
 * @param data An application specific pointer.
 * @return #OSSO_OK if the function executed successfully. retval is set to
 * DBUS_TYPE_INVALID for no reply. #OSSO_ERROR, if an error occured, a
 * dbus_error will be returned, and the retval should be of type
 * DBUS_TYPE_STRING with an error message string as value.
 */
typedef gint (osso_rpc_cb_f)(const gchar *interface, const gchar *method,
			     GArray *arguments, gpointer data,
			     osso_rpc_t *retval);

/**
 * This is the type for the asyncronous RPC return value callback
 * function.  This function is called when the asynchronous function
 * returns.
 * @param interface The interface of the remote object.
 * @param method The RPC function that was called.
 * @param retval The value that was returned.  The structure pointed
 * to by #retval and its contents are only valid until the callback
 * returns.
 * @param data An application specific pointer specified when the callback
 * was registered.
 */
typedef void (osso_rpc_async_f)(const gchar *interface,
				const gchar *method,
				osso_rpc_t *retval, gpointer data);

/**
 * This function calls an RPC function of an other application. This call
 * is blocking.
 * If the application providing the function is not already running, it
 * will be started by the D-BUS auto-activation mechanism.
 * 
 * The variable arguments work in a type-value pairs. The type argument
 * defines the type of the following value. If the type is G_TYPE_STRING,
 * then the value is a pointer to a
 * string. The list must end in a DBUS_TYPE_INVALID. The supported types
 * are:
 *  - DBUS_TYPE_BOOLEAN
 *    - The value is a gboolean.
 *  - DBUS_TYPE_INT, DBUS_TYPE_UINT
 *    - The value is an int or an unsigned int.
 *  - DBUS_TYPE_DOUBLE
 *    - The value is a float.
 *  - DBUS_TYPE_STRING
 *    - The value is a pointer to a string.
 *    - If the pointer is NULL it is translated to a DBUS_TYPE_NIL.
 *  - DBUS_TYPE_NIL
 *    - No value, this represents a NULL type.
 * @param osso The library context as returned by #osso_initialize.
 * @param service The service name of the remote service,
 * e.g. com.nokia.application.
 * @param object_path The object path of the remote object.
 * @param interface The interface that the RPC function belongs to.
 * @param method The RPC function to call.
 * @param retval A pointer to a structure where the return value can
 * be stored. If a reply is not interesting, or not desired, this can
 * be set to NULL. When this is non-NULL, the call blocks to wait for
 * the return value. If the expected reply does not come within the
 * RPC timeout value set with the #osso_rpc_set_timeout function,
 * #OSSO_ERROR will be returned and the retval variable will be set to
 * #OSSO_RPC_ERROR.  If NULL is given, the function returns
 * immediately after the call has been initiated (D-BUS message sent).
 * You should call #osso_rpc_free_val for this structure when it is no
 * longer needed.
 * @param argument_type The type of the first argument.
 * @param ... The first argument value, and then a type-value list of other
 * arguments.
 * This list must end in a DBUS_TYPE_INVALID type.
 * @return #OSSO_OK on success, #OSSO_INVALID if a parameter
 * is invalid. If the remote method returns an error, or does not return
 * anything, then #OSSO_ERROR is returned, and retval is set to an error
 * message. #OSSO_ERROR is also returned if any other kind of error
 * occurs, such as an IO error.
 *
 */
osso_return_t osso_rpc_run (osso_context_t * osso, const gchar * service,
                            const gchar * object_path,
                            const gchar * interface, const gchar * method,
                            osso_rpc_t * retval, int argument_type, ...);

/**
 * This function is a wrapper for #osso_rpc_run. It calls an RPC 
 * function of an other application. This call is blocking.
 * The service name of the other application is "com.nokia.A", where A
 * is the application parameter passed to this function. Similarly, the
 * object path is "/com/nokia/A", and the interface "com.nokia.A".
 * If the application providing the service is not already running, it
 * will be started by the D-BUS auto-activation mechanism.
 * 
 * @param osso The library context as returned by #osso_initialize.
 * @param application The name of the remote application.
 * @param method The RPC function to call.
 * @param retval A pointer to a structure where the return value can be
 * stored. If the application does not want to handle the reply, this can be
 * set to NULL. When this is non-NULL, the call blocks to wait for
 * the return value. If the expected reply does not come within the
 * RPC timeout value set with the #osso_rpc_set_timeout function, #OSSO_ERROR
 * will be returned and the retval variable will be set to #OSSO_RPC_ERROR.
 * You should call #osso_rpc_free_val for this structure when it is no
 * longer needed.
 * @param argument_type The type of the first argument.
 * @param ... The first argument value, and then a type-value list of other
 * arguments.
 * The list must be terminated with DBUS_TYPE_INVALID type. 
 * See #osso_rpc_run for more information.
 * @return #OSSO_OK if the message was sent. #OSSO_INVALID if a parameter
 * is invalid. If the remote method returns an error, or does not return
 * anything, then #OSSO_ERROR is returned, and retval is set to an error
 * message. #OSSO_ERROR is also returned if any other kind of error
 * occures, like IO error.
 */
osso_return_t osso_rpc_run_with_defaults (osso_context_t * osso,
                                          const gchar * application,
                                          const gchar * method,
                                          osso_rpc_t * retval,
                                          int argument_type, ...);

/**
 * This function calls an RPC function of an other application.
 * This call is non-blocking; a callback function is registered for the
 * return value of the called RPC function.
 * If the application providing the service is not already running, it
 * will be started by the D-BUS auto-activation mechanism.
 *
 * @param osso The library context as returned by #osso_initialize.
 * @param service The service name of the other application,
 * e.g. com.nokia.application.
 * @param object_path The object path of the target object.
 * @param interface The interface that the RPC function belongs to.
 * @param method The RPC function to call.
 * @param async_cb A pointer to a function to be called when the call
 * returns. If the call times out, async_cb will be called with an error
 * generated by the D-BUS library. If this is NULL, this function behaves
 * just like #osso_rpc_run, with the argument retval set to NULL.
 * @param data Arbitrary application specific pointer that will be passed
 * to the callback and ignored by Libosso.
 * @param argument_type The type of the first argument.
 * @param ... The first argument value, and then a type-value list of other
 * arguments.
 * The list must be terminated with DBUS_TYPE_INVALID type. 
 * See #osso_rpc_run for more information.
 * @return #OSSO_OK on success, #OSSO_INVALID if a parameter
 * is invalid, and #OSSO_ERROR if an error occurred.
 * the message)
 */
osso_return_t osso_rpc_async_run (osso_context_t * osso,
                                  const gchar * service,
                                  const gchar * object_path,
                                  const gchar * interface,
                                  const gchar * method,
                                  osso_rpc_async_f * async_cb, gpointer data,
                                  int argument_type, ...);

/**
 * This function calls an RPC function for the default service
 * of an other application. This call is non-blocking; a callback function is
 * registered for the return value of the called RPC function.
 * The service name of the other application is "com.nokia.A", where A
 * is the application parameter passed to this function. Similarly, the
 * object path is "/com/nokia/A", and the interface "com.nokia.A".
 * If the application providing the service is not already running, it
 * will be started by the D-BUS auto-activation mechanism.
 *
 * @param osso The library context as returned by #osso_initialize.
 * @param application The name of the application providing the service.
 * @param method The RPC function to call.
 * @param async_cb A pointer to a function to be called when the RPC call
 * returns. If the call times out, async_cb will be called with an error
 * generated by the D-BUS library. If this parameter is NULL, this function
 * behaves just like #osso_rpc_run_with_defaults, with the retval argument
 * set to NULL.
 * @param data An application specific pointer that is given to the cb
 * function.
 * @param argument_type The type of the first argument.
 * @param ... The first argument value, and then a type-value list of other
 * arguments.
 * The list must be terminated with DBUS_TYPE_INVALID type. 
 * See #osso_rpc_run for more information.
 * @return #OSSO_OK if the message was sent. #OSSO_INVALID if a parameter
 * is invalid, and #OSSO_ERROR if an error occurs (like failure to send
 * the message)
 */
osso_return_t osso_rpc_async_run_with_defaults (osso_context_t * osso,
                                                const gchar * application,
                                                const gchar * method,
                                                osso_rpc_async_f * async_cb,
                                                gpointer data,
                                                int argument_type, ...);

/**
 * The type for functions that free the contents of a #osso_rpc_t
 * structure that is used as a #retval with an RPC callback.  See
 * #osso_rpc_set_cb_f.
 */
typedef void osso_rpc_retval_free_f (osso_rpc_t *retval);

/* The API of the following two functions was changed incompatibly in
   version 0.10.0 of libosso.  The API breaking was done on purpose
   since the old API was leading to bugs and needed to be phased out
   forcefully.  However, in order to keep binary compatibility, we
   actually provide both the old API with the old names and the new
   API with new names.  At compile time, macros are used to make the
   new API visible with the old names.  This gives us the desired API
   break.
*/
#define osso_rpc_set_cb_f         osso_rpc_set_cb_f_with_free
#define osso_rpc_set_default_cb_f osso_rpc_set_default_cb_f_with_free

/**
 * This function registers a callback function for handling RPC calls to
 * a given object of a service.
 * @param osso The library context as returned by #osso_initialize.
 * @param service The service name to set up, e.g. com.nokia.application.
 * @param object_path The object path that this object has.
 * @param interface The interface that this object implements.
 * @param cb The function to register.
 * @param data Arbitrary application specific pointer that will be passed
 * to the callback and ignored by Libosso.
 * @param retval_free A function that is called with the #osso_rpc_t
 * structure that has been filled by #cb when that structure is no
 * longer needed by libosso.  In particular, you should pass
 * #osso_rpc_free_val here when you set #retval->value.s to a string
 * that should be freed with #g_free.  Likewise, you should pass #NULL
 * here when you set #retval->value.s to a string that should not be
 * freed.
 * @return #OSSO_OK on success, #OSSO_INVALID if a parameter is
 * invalid, and #OSSO_ERROR if an error occurred.
 */
osso_return_t osso_rpc_set_cb_f (osso_context_t * osso, const gchar * service,
                                 const gchar * object_path,
                                 const gchar * interface, osso_rpc_cb_f * cb,
                                 gpointer data,
				 osso_rpc_retval_free_f *retval_free);

/**
 * This function registers a callback function for handling RPC calls to the
 * default service of the application. The default service is "com.nokia.A",
 * where A is the application's name as given to #osso_initialize.
 * @param osso The library context as returned by #osso_initialize.
 * @param cb The function to register.
 * @param data Arbitrary application specific pointer that will be passed
 * to the callback and ignored by Libosso.
 * @param retval_free As for #osso_rpc_set_cb_f.
 * @return #OSSO_OK on success, #OSSO_INVALID if a
 * parameter is invalid, and #OSSO_ERROR if an error occurred.
 */
osso_return_t osso_rpc_set_default_cb_f (osso_context_t * osso,
                                         osso_rpc_cb_f * cb, gpointer data,
					 osso_rpc_retval_free_f *retval_free);

/**
 * This function unregisters an RPC callback function.
 * @param osso The library context as returned by #osso_initialize.
 * @param service The service name to unregister.
 * @param object_path The object path that this object has.
 * @param interface The interface that this object implements.
 * @param cb The function that was registered.
 * @param data The same pointer that was used with the
 * #osso_rpc_set_cb_f call.
 * @return #OSSO_OK on success, #OSSO_INVALID if a
 * parameter is invalid, and #OSSO_ERROR if an error occurred.
 */
osso_return_t osso_rpc_unset_cb_f (osso_context_t * osso,
                                   const gchar * service,
                                   const gchar * object_path,
                                   const gchar * interface,
                                   osso_rpc_cb_f * cb, gpointer data);

/**
 * This function unregisters an RPC callback function for the default service.
 * @param osso The library context as returned by #osso_initialize.
 * @param cb The RPC function to unregister.
 * @param data The same pointer that was used with the
 * #osso_rpc_set_default_cb_f call.
 * @return #OSSO_OK on success, #OSSO_INVALID if a
 * parameter is invalid, and #OSSO_ERROR if an error occurred.
 */
osso_return_t osso_rpc_unset_default_cb_f (osso_context_t * osso,
                                           osso_rpc_cb_f * cb, gpointer data);

/**
 * Sets the timeout value used by the RPC functions.
 * @param osso The library context as returned by #osso_initialize.
 * @param timeout The new timeout value.
 * @return #OSSO_OK on success, and #OSSO_INVALID on invalid parameter.
 */
osso_return_t osso_rpc_get_timeout (osso_context_t * osso, gint * timeout);

/**
 * Returns the current RPC timeout value.
 * @param osso The library context as returned by #osso_initialize.
 * @param timeout A pointer where to return the timeout value.
 * @return #OSSO_OK on success, and #OSSO_INVALID on invalid parameter.
 */
osso_return_t osso_rpc_set_timeout(osso_context_t * osso, gint timeout);

/* @}*/
/**********************************************************************/
/**
 * \defgroup Apps Applications
 *
 * @{
 */

/**
 * This is the type of the exit callback function.
 * @param die_now This parameter is obsolete and should be ignored.
 * @param data The data given to #osso_application_set_exit_cb.
 */
typedef void (osso_application_exit_cb)(gboolean die_now, gpointer data);

/**
 * This function registers the application's exit callback function.
 * When Libosso calls the application's exit callback function, the application
 * should save its GUI state and unsaved user data and exit as soon as possible.
 *
 * @param osso The library context as returned by #osso_initialize.
 * @param cb The exit callback function to register.
 * @param data Arbitrary application specific pointer that will be passed
 * to the callback and ignored by Libosso.
 * */
osso_return_t osso_application_set_exit_cb(osso_context_t *osso,
					   osso_application_exit_cb *cb,
					   gpointer data);

/**
 * This function tops an application (i.e. brings it to the foreground).
 * If the application is not already running,
 * D-BUS will launch it via the auto-activation mechanism. By using this
 * function, only one instance of the application will be running at any
 * given time. If the application is already running, this function will
 * only bring it to the foreground.
 * 
 * @param osso The library context as returned by #osso_initialize.
 * @param application The name of the application to top.
 * @param arguments This string lists some parameters for the
 * application. It can be used to give the names of the files to open, etc.
 * If no arguments are to be passed, this can be NULL.
 * @return #OSSO_OK if the application launch request was successfully sent
 * to the D-BUS daemon, #OSSO_ERROR if not.
 *
 * <h2>Example</h2>
 * This is an example on how to parse the argument from a top message. The
 * format is the same as a commandline with long options, i.e.
 * --param=value... So that the same arguments can be easily implemented
 * as commandline arguments.
 * @code
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>

void top_handler(const gchar *arguments, gpointer data);
{
  gchar **argv;
  gint argc;
  GError *err

  // this will split the arguments string into something similar to
  // what one would get in main.
  g_shell_parse_argv(arguments, &argc, &argv, &err);

  // This forces getopt to start from the first argument every time
  optind=0;
  optarg=argv;

  // call the generic commandline parser
  parse_commandline(argc, argv);

  // Free memory allocated previously.
  g_strfreev(argv);
}

void parse_commandline(int argc, char *argv[])
{
  static struct option long_options[] = {
    {"to", 1, 0, 't'}, // "to" requires an argument
    {"files", 1, 0, 'f'},
    {"url", 1, 0, 'u'},
    {0, 0, 0, 0}
  };
  gint index = 0, r;

  while(1) {
    // This function will parse and return the next argument,
    // see getopt(3) for more info.
    r = getopt_long(argc, argv, "t:f:u:", long_options, &index);

    // No more arguments
    if (r == -1) break;

    switch(r) {
      case 't':
        // do something with the argument, optarg is a global, external
        // variable that will point to the parameter value
        handle_to(optarg);
        break;
      // And so on
    }
  }
}
 * @endcode
 */
osso_return_t osso_application_top(osso_context_t *osso,
				   const gchar *application,
				   const gchar *arguments);

/**
 * This is the type for the top callback function.
 * @deprecated {This feature will be removed from Libosso soon, use
 * osso_rpc functions instead}
 * 
 * @param arguments This string lists some extra parameters for the
 * application. It can be used to give the names of the files to open, etc.
 * @param data The data that was set with the #osso_application_set_top_cb
 * function.
 * 
 */
typedef void(osso_application_top_cb_f)(const gchar *arguments,
					gpointer data);

/**
 * This function registers a top event callback function.
 * @deprecated {This feature will be removed from Libosso soon, use
 * osso_rpc functions instead}
 * 
 * @param osso The library context as returned by #osso_initialize.
 * @param cb The callback function.
 * @param data Arbitrary application specific pointer that will be passed
 * to the callback and ignored by Libosso.
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occurred, or
 * #OSSO_INVALID if some parameter is invalid.
 */
osso_return_t osso_application_set_top_cb(osso_context_t *osso,
					  osso_application_top_cb_f *cb,
					  gpointer data);

/**
 * This function unregisters a top event callback function.
 * @deprecated {This feature will be removed from Libosso soon, use
 * osso_rpc functions instead}
 * 
 * @param osso The library context as returned by #osso_initialize.
 * @param cb The callback function.
 * @param data The same pointer that was used with the call to
 * #osso_application_set_top_cb
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occurred, or
 * #OSSO_INVALID if some parameter is invalid.
 */
osso_return_t osso_application_unset_top_cb(osso_context_t *osso,
					    osso_application_top_cb_f *cb,
					    gpointer data);

/**********************************************************************/
/* @}*/
/**
 * \defgroup Autosave Autosaving
 *
 * How to use autosave:<br />
 * The application registers callback function(s) that is/are called by
 * Libosso to save the user data (such as an unsaved document). Whenever
 * the user data changes, the
 * application calls #osso_application_userdata_changed to tell Libosso
 * that the callback(s) should be called in the future.
 *
 * Libosso will call the callback(s) when:
 * -# a "dirty data" timer in Libosso expires 
 * -# Libosso gets a message from the system that unsaved user data should
 *    be saved (e.g. at shutdown)
 * 
 * The application should call #osso_application_autosave_force whenever
 * it is switched to the background (untopped).
 *
 * After the autosave callbacks have been called, the timer inside Libosso
 * is reset and the application needs to call #osso_application_userdata_changed
 * again when it has new "dirty" user data.
 * 
 */

/*@{*/

/**
 * This is the type for the autosave callback function.
 *
 * @param data Arbitrary application specific pointer.
 */
typedef void(osso_application_autosave_cb_f)(gpointer data);

/**
 * This function registers an autosave callback function.
 *
 * @param osso The library context as returned by #osso_initialize.
 * @param cb The callback function.
 * @param data Arbitrary application specific pointer that will be passed
 * to the callback and ignored by Libosso.
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occurred, or
 * #OSSO_INVALID if some parameter is invalid.
 */
osso_return_t osso_application_set_autosave_cb(osso_context_t *osso,
					       osso_application_autosave_cb_f *cb,
					       gpointer data);

/**
 * This function unregisters an autosave callback function.
 *
 * @param osso The library context as returned by #osso_initialize.
 * @param cb The callback function.
 * @param data Arbitrary application specific pointer that will be passed
 * to the callback and ignored by Libosso.
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occurred, or
 * #OSSO_INVALID if some parameter is invalid.
 */
osso_return_t osso_application_unset_autosave_cb(osso_context_t *osso,
						 osso_application_autosave_cb_f *cb,
						 gpointer data);

/**
 * This function is called by the application when the user data
 * has been changed, so that Libosso knows that a call to the autosave
 * callback is needed in the future to save the user data. The
 * dirty state will be cleared every time the application's autosave
 * callback function is called.
 *
 * @param osso The library context as returned by #osso_initialize.
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occurred or
 * #OSSO_INVALID if some parameter is invalid.
 */
osso_return_t osso_application_userdata_changed(osso_context_t *osso);

/**
 * This function forces a call to the application's autosave function,
 * and resets the autosave timeout.
 *
 * @param osso The library context as returned by #osso_initialize.
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occurred, or
 * #OSSO_INVALID if some parameter is invalid.
 */
osso_return_t osso_application_autosave_force(osso_context_t *osso);

/*
 * Returns the application name of a Libosso context.
 * @param osso The library context as returned by #osso_initialize.
 * @return The name of the parent application, or NULL if the context
 * is invalid.
 */
const gchar * osso_application_name_get(osso_context_t *osso);

/*
 * Returns the application version of a Libosso context.
 * @param osso The library context as returned by #osso_initialize.
 * @return The version of the application, or NULL if the context is
 * invalid.
 */
const gchar * osso_application_version_get(osso_context_t *osso);

/*@}*/

/**********************************************************************/
/**
 * \defgroup Statusbar Statusbar
 */
/* @{*/

/**
 * This function sends a statusbar event notification over the D-BUS.
 * If the applet is not loaded, the statusbar application will load it.
 * @param osso The library context as returned by #osso_initialize.
 * @param name the name of the applet that should receive the event.
 * @param argument1 This is the first argument of the event. It's
 * interpretation depends on event_type.
 * @param argument2 This is the second argument of the event. It's
 * interpretation depends on event_type.
 * @param argument3 This is the third argument of the event. It's
 * interpretation depends on event_type. NULL can be used here.
 * @param retval This parameter will be used to store possible return
 * data of the call. If NULL, no data will be expected (and the return 
 * value is unreliable).
 * You should call #osso_rpc_free_val for this structure when it is no
 * longer needed.
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occurred, or
 * #OSSO_INVALID if some parameter is invalid.
 */
osso_return_t osso_statusbar_send_event(osso_context_t *osso,
					const gchar *name,
					gint argument1, gint argument2,
					const gchar *argument3,
					osso_rpc_t *retval);

/* @}*/
/**
 * \defgroup Time Time
 */
/* @{*/

/**
 * This is the type for the time change notification callback function.
 * @param data An application specific data pointer.
 */
typedef void(osso_time_cb_f)(gpointer data);

/**
 * This function registers a callback that is called whenever the time is
 * changed with this library.
 * @param osso The library context as returned by #osso_initialize.
 * @param cb Function that is called when the system time is changed.
 * @param data Arbitrary application specific pointer that will be passed
 * to the callback and ignored by Libosso.
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occurred, or
 * #OSSO_INVALID if some parameter is invalid.
 */
osso_return_t osso_time_set_notification_cb(osso_context_t *osso,
					    osso_time_cb_f *cb,
					    gpointer data);
/**
 * This function sets the system and hardware time, and notifies about the
 * changing of time over the D-BUS system bus.
 * Notice: does not currently change the time.
 * @param osso The library context as returned by #osso_initialize.
 * @param new_time The new time in epoch format.
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occurred, or
 * #OSSO_INVALID if new_time or osso context is invalid.
 */
osso_return_t osso_time_set(osso_context_t *osso, time_t new_time);

/* @}*/
/**
 * \defgroup Sysnotes System notification
 */
/* @{*/

/**
 * This is the type of system note, in other words it specifies what kind
 * of an icon to use on the note.
 */
typedef enum {
  OSSO_GN_WARNING = 0, /**<The message is a warning.*/
  OSSO_GN_ERROR,	 /**<The message is an error.*/
  OSSO_GN_NOTICE,	 /**<The message is a notice.*/
  OSSO_GN_WAIT
} osso_system_note_type_t;

/**
 * This function will request that a system note (a window that is modal
 * to the whole system) is shown. The note has a single OK button.
 * This function enables non-GUI programs
 * to show some information to the user. Applications
 * that do have a GUI should not use this function but the hildon_note
 * widget directly.
 * @param osso The library context as returned by #osso_initialize.
 * @param message The contents for the system note.
 * @param type The type of system note:
 * OSSO_GN_WARNING - warning dialog with an exclamation mark icon,
 * OSSO_GN_ERROR - error dialog,
 * OSSO_GN_NOTICE - informative dialog with an 'i' icon.
 * @param retval This parameter will be used to store possible return
 * data of from the statusbar. If NULL, no data is expected, or it should be
 * ignored.
 * You should call #osso_rpc_free_val for this structure when it is no
 * longer needed.
 * @return #OSSO_OK if no errors occurred, #OSSO_INVALID if some parameters
 * are invalid, and #OSSO_ERROR if some other error occurs.
 */
osso_return_t osso_system_note_dialog(osso_context_t *osso,
				      const gchar *message,
				      osso_system_note_type_t type,
				      osso_rpc_t *retval);

/**
 * This function requests that the statusbar shows an infoprint (aka
 * information banner).
 * This allows non-GUI applications to display some information to the user.
 * Applications that do have a GUI should not use this function, but the
 * gtk_infoprint widget directly.
 * @param osso The library context as returned by #osso_initialize.
 * @param text The text to display on the infoprint.
 * @param retval This parameter will be used to store a possible return
 * data from the statusbar. If NULL, no data is expected, or it should be ignored.
 * You should call #osso_rpc_free_val for this structure when it is no
 * longer needed.
 * @return #OSSO_OK if no errors occurred, #OSSO_INVALID if some parameters
 * are invalid, and #OSSO_ERROR if some other error occurs.
 */
osso_return_t osso_system_note_infoprint(osso_context_t *osso,
					 const gchar *text,
					 osso_rpc_t *retval);

/* @}*/
/**********************************************************************/
/**
 * \defgroup Statesave State saving
 * State saving functionality is provided for
 * applications as a convenient way of storing and retrieving
 * GUI state information (e.g. the view to the
 * last open document, scroll bar position). The information saved by
 * using these functions does not survive over power off of the device,
 * so other means need to be used to save e.g. the document that the user
 * was writing (see Autosaving functions). The application should load the
 * state information during startup in order to present the user with the
 * previous GUI state from the start. No automatic saving of GUI state is
 * done; it is left as responsibility of the application to call the
 * appropriate Libosso function to do it.
 *
 * The new osso_state_read() and osso_state_write() functions take care of
 * state data reading and writing. The only limitation is
 * that only a fixed-size contiguous memory area can be stored. This means
 * that memory pointed by pointers cannot be stored (unless the pointed
 * area is inside the contiguous memory area).
 * The easiest way to define a contiguous memory area of different data
 * types is to define a structure such as 
 * @code
struct state_data{
  gchar string[STRING_LENGTH];
  guint32 i;
  gchar filename[PATH_MAX];
};
 * @endcode
 * 
 * One particular version (the version is given to osso_initialize) of the
 * application can have only one state file.
 *
 */
/* @{*/

/**
 * This structure represents a state.
 */
typedef struct {
  guint32 state_size; /**< The size of state_data */
  gpointer state_data; /**< A pointer to state data */
} osso_state_t;

/**
 * This function writes a (GUI) state to a file. Any existing state
 * file will be overwritten.
 * @param osso The library context as returned by #osso_initialize.
 * @param state The state to save.
 * @return 
 * OSSO_OK if the operation succeeded.
 * OSSO_ERROR if the state could not be saved.
 * OSSO_INVALID if any argument is invalid.
 * @code
#include <libosso.h>
#include <string.h>
int main()
{
  static char* s = "this is the state";
  osso_context_t *osso;
  osso_state_t state;
  osso_return_t ret;
  osso = osso_initialize("app", "1", 0, NULL);
  if (osso == NULL) return 1;
  state.state_size = strlen(s) + 1;
  state.state_data = (gpointer) s;
  ret = osso_state_write(osso, &state);
  if (ret != OSSO_OK)
    return 1;
  else
    return 0;
}
 * @endcode
 */
osso_return_t osso_state_write(osso_context_t *osso, osso_state_t *state);

/**
 * This function reads a saved (GUI) state.
 * @param osso The library context as returned by #osso_initialize.
 * @param state A pointer to an allocated #osso_state_t structure. The data
 * should point to a memory block that is at least as large as state_size.
 * If the state_data member is NULL, the memory will be dynamically allocated
 * and must be freed by the caller. If the state_size member is zero, the
 * needed space is read from the state file. 
 * @return OSSO_OK if the state reading was successful.
 * OSSO_ERROR if the operation failed for some reason.
 * OSSO_INVALID if function arguments were invalid.
 * OSSO_ERROR_NO_STATE if the state file was not found.
 * OSSO_ERROR_STATE_SIZE if the state is not the specified size.
 * @code
#include <libosso.h>
#include <stdio.h>
int main()
{
  char buf[50];
  osso_context_t *osso;
  osso_state_t state;
  osso_return_t ret;
  buf[0] = '\0';
  osso = osso_initialize("app", "1", 0, NULL);
  if (osso == NULL) return 1;
  state.state_size = 18;
  state.state_data = buf;
  ret = osso_state_read(osso, &state);
  if (ret != OSSO_OK) {
    printf("could not read the state\n");
    return 1;
  } else {
    printf("read state: %s\n", buf);
    return 0;
  }
}
 * @endcode
 */
osso_return_t osso_state_read(osso_context_t *osso, osso_state_t *state);


/* @}*/
/**********************************************************************/
/**
 * \defgroup Ctrlpanel Plugins
 */
/* @{*/
/**
 * Calls the execute() function of a plugin. The
 * plugins are loaded using dlopen(3) and after the execute function
 * returns, unloaded with dlclose(3).
 * @param osso The library context as returned by #osso_initialize.
 * @param filename The shared object (.so) file of the plugin. It should
 * include the ".so" prefix, but not a path.
 * @param data The GTK top-level widget. It is needed so that the widgets
 * created by the plugin can be made a child of the main application that
 * utilizes the plugin. Type is a gpointer so that the plugin does not need
 * to depend on GTK (in which case it should ignore the parameter).
 * @param user_activated If the plugin was activated by a user (as opposed to
 * activated by restoring software state), set to TRUE, else to FALSE.
 * @return the return value of the plugin on success, or #OSSO_ERROR on
 * error.
 */
osso_return_t osso_cp_plugin_execute(osso_context_t *osso,
				     const gchar *filename,
				     gpointer data, gboolean user_activated);

/**
 * This function is used to tell a plugin to save its state.
 *     
 * @param osso The library context as returned by #osso_initialize.
 * @param filename Same as the filename parameter of #osso_cp_plugin_execute
 * @param data A pointer that will be passed to the state save function of
 * the plugin and ignored by Libosso. 
 * @return OSSO_OK if all goes well, OSSO_INVALID if a parameter is
 * invalid, or OSSO_ERROR if some error occurred.
 */
osso_return_t osso_cp_plugin_save_state(osso_context_t *osso,
					const gchar *filename,
					gpointer data);
/* @}*/
/**********************************************************************/
/**
 * \defgroup Devstate Device state
 * Functions in this group offer a convenient way to receive device status
 * information from lower levels of the system.
 */

typedef enum {
  OSSO_DEVMODE_NORMAL = 0, /* Normal mode */
  OSSO_DEVMODE_FLIGHT,     /* Flight mode */
  OSSO_DEVMODE_OFFLINE,    /* Offline mode; unsupported! */
  OSSO_DEVMODE_INVALID     /* Should *never* occur! */
} osso_devmode_t;


/*@{*/
/**
 * This structure represents the device state.
 * NOTE: Libosso sets value of save_unsaved_data_ind to 1 when the signal
 * arrived and back to 0 after calling the application callback.
 */
typedef struct {
  gboolean shutdown_ind; /**<The device is about to be shut down */
  gboolean save_unsaved_data_ind; /**<The application should save unsaved
                                    user data (see Autosaving functions) */
  gboolean memory_low_ind; /**<The system is low on memory */
  gboolean system_inactivity_ind; /**<The application should reduce
                                       its activity */
  osso_devmode_t sig_device_mode_ind; /**<The mode of the device */
} osso_hw_state_t;


/**
 * Request to turn on the display as if the user had pressed a key
 * or the touch screen. This can be used after completing a long operation
 * such as downloading a large file or after retrieving e-mails.
 * 
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occured.
 */ 

osso_return_t osso_display_state_on(osso_context_t *osso);

/**
 * Request not to blank the display. This function must be called again
 * within 60 seconds to renew the request. The function is used, for example,
 * by the video player during video playback. Also prevents suspending the
 * device.
 * 
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occured.
 */

osso_return_t osso_display_blanking_pause(osso_context_t *osso);

/**
 * This is the type for the device state callback function.
 * 
 * @param state The current state of the device.
 * @param data The data that was set with the
 * #osso_hw_set_event_cb function.
 */
typedef void (osso_hw_cb_f)(osso_hw_state_t *state, gpointer data);

/**
 * This function registers a callback function that is called whenever
 * the state of the device changes. The first call to this function will
 * also check the current state of the device, and if the state is available,
 * the corresponding callback function will be called immediately.
 * 
 * @param osso The library context as returned by #osso_initialize.
 * @param state The states the application is interested in. NULL can be
 * passed here to indicate that all signals are of interest.
 * @param cb The callback function.
 * @param data Arbitrary application specific pointer that will be passed
 * to the callback and ignored by Libosso.
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occurred, or
 * #OSSO_INVALID if some parameter is invalid.
 */
osso_return_t osso_hw_set_event_cb(osso_context_t *osso,
				   osso_hw_state_t *state,
				   osso_hw_cb_f *cb, gpointer data);

/**
 * This function unregisters a device state callback function.
 * 
 * @param osso The library context as returned by #osso_initialize.
 * @param state The states specified when #osso_hw_set_event_cb was called.
 * @return #OSSO_OK if all goes well, #OSSO_ERROR if an error occurred, or
 * #OSSO_INVALID if some parameter is invalid.
 */
osso_return_t osso_hw_unset_event_cb(osso_context_t *osso,
				     osso_hw_state_t *state);

/*@}*/
/**********************************************************************/
/**
 * \defgroup MIME MIME types
 */
/*@{*/

/**
 * This is the type of the MIME callback function.
 * @param data The data pointer that was set with #osso_mime_set_cb.
 * @param argc The number of files to open.
 * @param argv An array of file URIs.  The strings
 * pointed to by the #argv array are only valid until the callback
 * function returns.
 */
typedef void (osso_mime_cb_f)(gpointer data, int argc, gchar **argv);

/**
 * This function registers a MIME callback function that Libosso calls when
 * the user wants the application to open file(s) of a MIME type handled
 * by the application.
 * @param osso The library context as returned by the #osso_initialize
 * function.
 * @param cb The callback function to call.
 * @param data Arbitrary application specific pointer that will be passed
 * to the callback and ignored by Libosso.
 * @return OSSO_OK on success, OSSO_INVALID if some parameter was invalid,
 * and OSSO_ERROR if there was an error.
 */
osso_return_t osso_mime_set_cb(osso_context_t *osso, osso_mime_cb_f *cb,
			       gpointer data);

/**
 * This function unregisters a MIME callback function that was previously
 * registered with the #osso_mime_set_cb function.
 * @param osso The library context as returned by the LibOSSO function
 * #osso_initialize.
 * @return OSSO_OK on success, OSSO_INVALID if the parameter was invalid,
 * and OSSO_ERROR if there was an error.
 */
osso_return_t osso_mime_unset_cb(osso_context_t *osso);
/*@}*/

/**
 *  Returns the D-BUS session bus connection used by the Libosso context.
 *  The return value should be casted to DBusConnection pointer.
 *  @param osso Libosso context as returned by #osso_initialize.
 *  @return Pointer to DBusConnection on a gpointer.
 */
gpointer osso_get_dbus_connection(osso_context_t *osso);


/** 
 *  Returns the D-BUS system bus connection used by the Libosso context.
 *  The return value should be casted to DBusConnection pointer.
 *  @param osso Libosso context as returned by #osso_initialize.
 *  @return Pointer to DBusConnection on a gpointer.
 */
gpointer osso_get_sys_dbus_connection(osso_context_t *osso);



/************************************************************************/
/************************************************************************/
/*******         OBSOLETE API BELOW -- DO NOT USE                     ***/
/************************************************************************/
/************************************************************************/

/**
 * \defgroup outside Not included
 *
 * @note
 * This API is obsolete and should not be used by new code.
 *
 */
/* @{*/

/**
 * This function opens a state file for writing. If the state file already
 * exists, it will be silently truncated.
 * @deprecated Use the more robust osso_state_write() function instead.
 * No new code should use this function.
 * @param osso The library context as returned by #osso_initialize.
 * @return the file descriptor of the opened file. On an error -1 is
 * returned.
 */
int osso_state_open_write(osso_context_t *osso);

/**
 * This function opens a state file for reading.
 * @deprecated Use the more robust osso_state_read() function instead.
 * No new code should use this function.
 * @param osso The library context as returned by #osso_initialize.
 * @return The file descriptor of the opened file. On an error -1 is
 * returned.
 */
int osso_state_open_read(osso_context_t *osso);

/**
 * This function closes the state file.
 * @deprecated Use the more robust osso_state_read() and 
 * osso_state_write() functions instead. No new code should use this
 * function.
 * @param osso The library context as returned by #osso_initialize.
 * @param fd The file descriptor to close.
 */
void osso_state_close(osso_context_t *osso, gint fd);

/** This function adds a mail to be displayed in the
 *      mail popup of the tasknavigator.
 *      @param osso The osso library context returned by #osso_initialize
 *      @param id unique identifier for the mail
 *      @param subject the subject string of the email
 *      @param sender the email address of the sender of the mail
 *      @param attachment true iff the mail has an attatchment
 *      @param date the date the mail was received.
 *
 *
 *      @return #OSSO_OK on success, #OSSO_ERROR in case of an error,
 *              #OSSO_INVALID if a parameter is invalid.
 */
osso_return_t osso_tasknav_mail_add(osso_context_t *osso, guint id,
				    const gchar *subject, const gchar *sender,
				    gboolean attachment, const gchar *date);

/** This function removes an email message from the navigator.
 *
 *      @param osso The osso library context returned by #osso_initialize
 *      @param id unique identifier of the mail to be destroyed
 *
 *      @return #OSSO_OK on success, #OSSO_ERROR in case of an error,
 *              #OSSO_INVALID if a paramter is invalid.
 *
 *
 */
osso_return_t osso_tasknav_mail_del(osso_context_t *osso, guint id);

/** This function updates the outbox message count in the task
 *      navigator mail window.
 *
 *      @param osso The osso library context returned by #osso_initialize
 *      @param count New number of messages in outbox
 *
 *      @return #OSSO_OK on success, #OSSO_ERROR in case of an error,
 *              #OSSO_INVALID if a paramter is invalid.
 *
 *
 */
osso_return_t osso_tasknav_mail_set_outbox_count(osso_context_t *osso,
						 guint count);

/** Osso initialisation routine for applications to call.
 *  This ensures that the environment variables are transferred
 *  correctly before the gtk_init();
 *
 * 
 * @param application The name that this application is known as. This is  
 * not the name that a user sees, but rather the name that other
 * applications use as an address to communicate with this program.
 * @param version The version string of the application. It mus be simply
 * comparable with strcmp().
 * @param cb The callback function to used when application is topped.
 * @param callback_data An arbitrary pointer to some application specific data
 *              to pass to the top callback
*/
osso_context_t * osso_application_initialize(const gchar *application,
					     const gchar *version,
					     osso_application_top_cb_f *cb,
					     gpointer callback_data);


G_END_DECLS

#endif /* LIBOSSO_H_*/
