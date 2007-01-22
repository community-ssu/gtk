/* vim: set encoding=iso-8859-1 : */
/* 
 * This file is the public API for the Maemo User Application LIbrary
 * (a new API, eventually replacing the current Libosso API).
 *
 * This file is part of libosso
 *
 * Copyright (C) 2006 Nokia Corporation. All rights reserved.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

/******************************************************
 * NEW API DEVELOPMENT - THESE ARE SUBJECT TO CHANGE!
 * muali = maemo user application library
 ******************************************************/

#ifndef MUALI_H
#define MUALI_H

#include "osso-log.h" 

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-protocol.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int muali_error_t;

/* Use these definitions for error codes. You can rely on the success
 * code always being zero and error being non-zero. New definitions
 * may come later. */
#define MUALI_ERROR_SUCCESS     0       /* success (not an error) */
#define MUALI_ERROR_INVALID     1       /* invalid arguments */ 
#define MUALI_ERROR_OOM         2       /* out of memory */
#define MUALI_ERROR             3       /* generic error code */
#define MUALI_ERROR_TIMEOUT     4       /* timeout happened */

typedef struct _muali_context_t muali_context_t;

typedef struct {
  int type;             /* type of argument */
  int data_len;         /* only used with MUALI_TYPE_DATA to tell
                           the length of the data */
  union {
    int b;              /* MUALI_TYPE_BOOL */
    char by;            /* MUALI_TYPE_BYTE */
    int i;              /* MUALI_TYPE_INT */
    unsigned int u;     /* MUALI_TYPE_UINT */
    double d;           /* MUALI_TYPE_DOUBLE */
    char *s;            /* MUALI_TYPE_STRING */
    void *data;         /* MUALI_TYPE_DATA */
  } value;
} muali_arg_t;

/* Values of the type member of muali_arg_t. New definitions may
 * come later. */
#define MUALI_TYPE_INVALID      0
#define MUALI_TYPE_BOOL         1
#define MUALI_TYPE_BYTE         2
#define MUALI_TYPE_INT          3
#define MUALI_TYPE_UINT         4
#define MUALI_TYPE_DOUBLE       5
#define MUALI_TYPE_STRING       6
#define MUALI_TYPE_DATA         7

typedef enum {
        MUALI_BUS_IRRELEVANT = 0,
        MUALI_BUS_SYSTEM,
        MUALI_BUS_SESSION,
        MUALI_BUS_BOTH
} muali_bus_type;

typedef struct {
        const char *service;    /* service name or NULL */
        const char *path;       /* object path or NULL */
        const char *interface;  /* interface name or NULL */
        const char *name;       /* name of message/signal or NULL */

        /* Received event type, in case it is one of the
         * pre-defined events */
        int event_type;

        /* Message bus type. The type may be MUALI_BUS_IRRELEVANT
         * for e.g. pre-defined events. */
        muali_bus_type bus_type;

        /* This can be an ID string of the reply or a received message. */
        const char *message_id;

        /* received error message, or NULL */
        const char *error;

        /* Array of arguments (coming or going), terminating to
         * argument of type MUALI_TYPE_INVALID. args is NULL if the
         * message has no arguments.
         * Note that this is ignored in case of a vararg function. */
        const muali_arg_t *args;
} muali_event_info_t;

typedef void (muali_handler_t)(muali_context_t *context,
                               const muali_event_info_t *info,
                               void *user_data);

/****************************************************/
/* Event types. New event types may be added later. */
/* Some event types may be 'secret', i.e. not meant */
/* for applications to handle, and defined in       */
/* another header file.                             */
/****************************************************/

/* A non-event. The event type is not relevant. */
#define MUALI_EVENT_NONE                0

#define MUALI_EVENT_MESSAGE             1 /* message received */
#define MUALI_EVENT_SIGNAL              2 /* signal received */
#define MUALI_EVENT_MESSAGE_OR_SIGNAL   3 /* message or signal received */
#define MUALI_EVENT_REPLY               4 /* reply received */
#define MUALI_EVENT_ERROR               5 /* error reply received */
#define MUALI_EVENT_REPLY_OR_ERROR      6 /* reply or error received */

#define MUALI_EVENT_LOWMEM_ON           7 /* low memory state */
#define MUALI_EVENT_LOWMEM_OFF          8 /* low memory state over */
#define MUALI_EVENT_LOWMEM_BOTH         9 /* both low memory events */

#define MUALI_EVENT_INACTIVITY_ON      10 /* system inactive */
#define MUALI_EVENT_INACTIVITY_OFF     11 /* system no longer inactive */
#define MUALI_EVENT_INACTIVITY_BOTH    12 /* both inactivity signals */

#define MUALI_EVENT_DISPLAY_ON         13 /* display on */
#define MUALI_EVENT_DISPLAY_DIMMED     14 /* display dimmed */
#define MUALI_EVENT_DISPLAY_OFF        15 /* display off */
#define MUALI_EVENT_DISPLAY_ALL        16 /* all display signals */


/**********************************/
/*          FUNCTIONS             */
/**********************************/


/**
 * This function initialises the library, connects to both the D-Bus session
 * and system busses, integrates with the GLib main loop, and
 * initialises the library for use. #muali_init should be called
 * only once by the program.
 * @param program_name The name of the program.
 * This name forms the last part of the default (D-Bus) service name of the
 * program. Note that the D-Bus service name will be
 * 'com.nokia.program_name', where 'program_name' is the value you gave as the
 * parameter. Note also that this argument must be identical to the
 * X-Osso-Service value in the desktop file, or the D-Bus daemon will kill
 * your program after the program has been auto-started by the daemon.
 * The only valid characters that the name may contain are letters a-z and
 * the underscore '_'.
 * However, you can give a name such as 'org.foo.bar' to
 * have 'bar' as your program's name and 'org.foo.bar' as the D-Bus service
 * name.
 * @param program_version The version string of the application. This will
 * be used to determine if a saved UI state is still valid for the program.
 * @param context The GLib main loop context to connect to, or NULL for
 * the default context.
 * @return A context to use in later calls to this library, or NULL if an
 * error happened.
 */
muali_context_t *muali_init(const char *program_name,
                            const char *program_version,
                            GMainContext *context);

/**
 * This function registers a handler for event, message, or signal.
 *
 * @param context Muali context.
 * @param event_type Specifies the event to handle (should be one of the
 *                   MUALI_EVENT_* values).
 * @param handler The handler function to call when the event happens.
 * @param user_data Optional user data to pass to the handler when it
 *                  is called.
 * @param handler_id Return location for the handler ID, which is used
 *                   to unregister the handler.
 *
 * @return #MUALI_ERROR_SUCCESS on success.
 */
muali_error_t muali_set_event_handler(muali_context_t *context,
                                      int event_type,
                                      muali_handler_t *handler,
                                      void *user_data,
                                      int *handler_id);

/**
 * This function registers a handler for event, message, or signal.
 *
 * @param context Muali context.
 * @param info Structure specifying the type of events to handle.
 * @param handler The handler function to call when the event happens.
 * @param user_data Optional user data to pass to the handler when it
 *                  is called.
 * @param handler_id Return location for the handler ID, which is used
 *                   to unregister the handler.
 *
 * @return #MUALI_ERROR_SUCCESS on success.
 */
muali_error_t muali_set_event_handler_custom(muali_context_t *context,
                                             const muali_event_info_t *info,
                                             muali_handler_t *handler,
                                             void *user_data,
                                             int *handler_id);

/**
 * This function unregisters a handler for event, message, or signal.
 *
 * @param context Muali context.
 * @param handler_id Handler to unregister. The handler will not be
 *                   called after it has been successfully unregistered.
 *
 * @return #MUALI_ERROR_SUCCESS on success.
 */
muali_error_t muali_unset_event_handler(muali_context_t *context,
                                        int handler_id);

/**
 * This function sets the service, path, and interface prefix used when
 * using the 'simple' messaging functions (#muali_send and
 * #muali_send_varargs).
 * The format of the string should be something like "org.maemo".
 * The default is "com.nokia". Note that the prefix is specific to the
 * Muali context and different code could share the same Muali context.
 *
 * @param context Muali context.
 * @param prefix Prefix to use in 'simple' messaging functions.
 *
 * @return #MUALI_ERROR_SUCCESS on success.
 */
muali_error_t muali_set_default_prefix(muali_context_t *context,
                                       const char *prefix);

/**************************/
/* simple message sending */
/**************************/

/* asyncronous */
muali_error_t muali_send_string(muali_context_t *context,
                                muali_handler_t *reply_handler,
                                const void *user_data,
                                muali_bus_type bus_type,
                                const char *destination,
                                const char *message_name,
                                const char *string);

muali_error_t muali_send_varargs(muali_context_t *context,
                                 muali_handler_t *reply_handler,
                                 const void *user_data,
                                 muali_bus_type bus_type,
                                 const char *destination,
                                 const char *message_name,
                                 int arg_type, ...);

/* blocking */
muali_error_t muali_send_string_and_wait(muali_context_t *context,
                                         muali_event_info_t *reply,
                                         const char *destination,
                                         const char *message_name,
                                         const char *string);

muali_error_t muali_send_and_wait_varargs(muali_context_t *context,
                                          muali_event_info_t *reply,
                                          const char *service,
                                          int arg_type, ...);

/* signals */
muali_error_t muali_send_signal(muali_context_t *context,
                                muali_bus_type bus_type,
                                const char *signal_name,
                                const char *argument);

muali_error_t muali_send_signal_full(muali_context_t *context,
                                     muali_bus_type bus_type,
                                     const char *signal_path,
                                     const char *signal_interface,
                                     const char *signal_name,
                                     int arg_type, ...);


/****************************************/
/* 'advanced' message or signal sending */
/****************************************/

muali_error_t muali_send_any_varargs(muali_context_t *context,
                                     muali_handler_t *reply_handler,
                                     const void *user_data,
                                     long *reply_id,
                                     const muali_event_info_t *info,
                                     int arg_type, ...);

/* possibly blocking (sending signal does not block) */
muali_error_t muali_send_any_and_wait(muali_context_t *context,
                                      muali_event_info_t *reply,
                                      const muali_event_info_t *info);

muali_error_t muali_send_any_and_wait_varargs(muali_context_t *context,
                                              muali_event_info_t *reply,
                                              const muali_event_info_t *info,
                                              int arg_type, ...);

/**********************/
/* reply to a message */
/**********************/

muali_error_t muali_reply_string(muali_context_t *context,
                                 const char *message_id,
                                 const char *string);

muali_error_t muali_reply_varargs(muali_context_t *context,
                                  const char *message_id,
                                  int arg_type, ...);

muali_error_t muali_reply_error(muali_context_t *context,
                                const char *message_id,
                                const char *error_name,
                                const char *error_message);


#ifdef __cplusplus
}
#endif

#endif /* MUALI_H */
