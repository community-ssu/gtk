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
    int i;              /* MUALI_TYPE_INT */
    unsigned int u;     /* MUALI_TYPE_UINT */
    long l;             /* MUALI_TYPE_LONG */
    unsigned long ul;   /* MUALI_TYPE_ULONG */
    double d;           /* MUALI_TYPE_DOUBLE */
    char c;             /* MUALI_TYPE_CHAR */
    char *s;            /* MUALI_TYPE_STRING */
    void *data;         /* MUALI_TYPE_DATA */
  } value;
} muali_arg_t;

/* Values of the type member of muali_arg_t. New definitions may
 * come later. */
#define MUALI_TYPE_BOOL         1
#define MUALI_TYPE_INT          2
#define MUALI_TYPE_UINT         3
#define MUALI_TYPE_LONG         4
#define MUALI_TYPE_ULONG        5
#define MUALI_TYPE_DOUBLE       6
#define MUALI_TYPE_CHAR         7
#define MUALI_TYPE_STRING       8
#define MUALI_TYPE_DATA         9

typedef struct {
        const char *service;    /* service name or NULL */
        const char *path;       /* object path or NULL */
        const char *interface;  /* interface name or NULL */
        const char *name;       /* name of message/signal or NULL */

        /* event type, in case it is one of the pre-defined events */
        int event_type;

        /* This can be ID of the reply or a received message. */
        long message_id;

        /* received error message, or NULL */
        const char *error;

        /* muali-internal information, ignore this */
        void *muali_internal;

        /* Arguments (coming or going), or NULL. Note that this is
         * ignored in case of a vararg function. */
        const muali_arg_t *args[];
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


/**********************************/
/*          FUNCTIONS             */
/**********************************/

/**
 * This function registers a handler for event, message, or signal.
 *
 * @param context Muali context.
 * @param info Structure specifying the type of events to handle,
 *             or #NULL if the event_type argument is provided.
 *             Providing this or the event_type argument is mandatory.
 * @param event_type Specifies the event to handle. Providing this or
 *                   the info argument is mandatory.
 * @param handler The handler function to call when the event happens.
 * @param user_data Optional user data to pass to the handler when it
 *                  is called.
 * @param handler_id Return location for the handler ID, which is used
 *                   to unregister the handler.
 *
 * @return #MUALI_ERROR_SUCCESS on success.
 */
muali_error_t muali_set_event_handler(muali_context_t *context,
                                      const muali_event_info_t *info,
                                      int event_type,
                                      muali_handler_t *handler,
                                      const void *user_data,
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
muali_error_t muali_send(muali_context_t *context,
                         muali_handler_t *reply_handler,
                         const void *user_data,
                         long *reply_id,
                         const char *service,
                         const char *string);

muali_error_t muali_send_varargs(muali_context_t *context,
                                 muali_handler_t *reply_handler,
                                 const void *user_data,
                                 long *reply_id,
                                 const char *service,
                                 int arg_type, ...);

/* blocking */
muali_error_t muali_send_and_wait(muali_context_t *context,
                                  muali_event_info_t *reply,
                                  const char *service,
                                  const char *string);

muali_error_t muali_send_and_wait_varargs(muali_context_t *context,
                                          muali_event_info_t *reply,
                                          const char *service,
                                          int arg_type, ...);

/****************************************/
/* 'advanced' message or signal sending */
/****************************************/

/* asyncronous */
muali_error_t muali_send_any(muali_context_t *context,
                             muali_handler_t *reply_handler,
                             const void *user_data,
                             long *reply_id,
                             const muali_event_info_t *info);

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

muali_error_t muali_reply(muali_context_t *context,
                          long message_id,
                          const char *string);

muali_error_t muali_reply_varargs(muali_context_t *context,
                                  long message_id,
                                  int arg_type, ...);

muali_error_t muali_reply_error(muali_context_t *context,
                                long message_id,
                                const char *name,
                                const char *message);


#ifdef __cplusplus
}
#endif

#endif /* MUALI_H */
