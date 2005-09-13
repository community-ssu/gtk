/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 *
 */

#ifndef OSSO_MANAGER_H
#define OSSO_MANAGER_H

#include <libosso.h>
#include <X11/Xlib.h>
/* hildon includes */
#include "hildon-navigator.h"

#define APP_NAME_LEN 64

#define OSSO_BUS_ROOT          "com.nokia"
#define OSSO_BUS_ROOT_PATH     "/com/nokia"
#define OSSO_BUS_TOP           "top_application"

#define TASKNAV "tasknav"
#define TASKNAV_VERSION "0.1"

#define METHOD_NAME_LEN 64

#define SERVICE_PREFIX "com.nokia."

#define SERVICE_NAME_LEN 255
#define PATH_NAME_LEN 255
#define INTERFACE_NAME_LEN 255
#define TMP_NAME_LEN 255

#define DBUS_BUF_SIZE 128

typedef struct osso_manager osso_manager_t;

G_BEGIN_DECLS

struct osso_manager {
    osso_context_t  *osso;
    GArray          *methods;
    Window          window;
};



/** Call back function pointer type used by plugins to listen to
 *  the messages from DBUS.
 *
 *  @param arguments The argument table from DBUS as specified un
 *      libosso.h
 *  @param data The data pointer passed to the add_method_cb 
 *
 *  @return 0 on success, -1 on error
 */
typedef int (tasknav_cb_f)(GArray *arguments, gpointer data);

typedef struct osso_method {
    gchar       name[METHOD_NAME_LEN];
    tasknav_cb_f  *method;
    gpointer data;
} osso_method;

typedef struct { 
    gchar name[APP_NAME_LEN];
} app_name_t;




/** Returns a global instance of the osso manager. 
 *  
 */
osso_manager_t *osso_manager_singleton_get_instance( void );

/** Used by plugins to add a listener method for DBUS
 *      messags.
 *  @param manager Osso manager as returned by 
 *              osso_manager_singleton_get_instance
 *  @param methodname The name of the method visible to the
 *                      DBUS
 *  @param method A pointer to the callback method to call.
 *  @param data A data pointer to be passed to the method called
 */  
void add_method_cb(osso_manager_t *manager,
                        const gchar *methodname,
                            tasknav_cb_f *method,
                            gpointer data);

/** Routine to launch applications **/
void osso_manager_launch(osso_manager_t *man,const gchar *app,
            const gchar *launch_data);


/** Routine to print an infoprint trough osso*/
void osso_manager_infoprint(osso_manager_t *man, const gchar *message);

/** Method to set the x window to be used by the osso manager */
void osso_manager_set_window(osso_manager_t *man,Window win);



/** Check if a service given is connected to D-BUS*/
int is_service_running(const char *service);

/** Getter for the osso context */

osso_context_t *get_context(osso_manager_t *man);

G_END_DECLS

#endif /* OSSO_MANAGER_H */
