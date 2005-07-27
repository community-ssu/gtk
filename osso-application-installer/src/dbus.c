/**
        @file dbus.c

        DBUS functionality for OSSO Application Installer
        <p>
*/

/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */


#include <gtk/gtk.h>
#include <libosso.h>
#include "appdata.h"
#include "dbus.h"
#include "core.h"


gint dbus_req_handler(const gchar *interface, const gchar *method,
                      GArray *arguments, gpointer data,
                      osso_rpc_t *retval)
{
    (void) interface;
    return dbus_message_handler(method, arguments, data, retval);
}
 

gint dbus_message_handler(const gchar *method, GArray *arguments, 
                          gpointer data, osso_rpc_t *retval)
{
  AppData *app_data;
  app_data = (AppData *) data;
  osso_rpc_t val = g_array_index(arguments, osso_rpc_t, 0);
  
  (void) arguments;  
  g_assert(method);

  fprintf(stderr, "dbus: method '%s' called\n", method);
  
  /* Catch the message and define what you want to do with it */        
  if (g_ascii_strcasecmp(method, DBUS_METHOD_MIME_OPEN) == 0) {
    fprintf(stderr, "Got method 'mime_open' with method '%s'\n",
	    DBUS_METHOD_MIME_OPEN);
    if ( (val.type == DBUS_TYPE_STRING)
        && (val.value.s != NULL) ) {

      install_package_from_uri (val.value.s, app_data);

      retval->type = DBUS_TYPE_BOOLEAN;
      retval->value.b = TRUE;
      return OSSO_OK;
    }

  } else if (g_ascii_strcasecmp(method,
             "application_installer_display_infoprint") == 0) {
    /* "application_installer_display_infoprint" displays 
       the infoprint with defined string */
    osso_system_note_infoprint(app_data->app_osso_data->osso,
     "I'm here!", retval);
    
    return DBUS_TYPE_BOOLEAN;

  } else {
    osso_log(LOG_ERR, "Unknown DBUS method: %s\n", method);
  }
  
  return DBUS_TYPE_INVALID;
}


/* Send d-bus message */
osso_return_t send_dbus_message(const gchar *app, const gchar *method,
                                GArray *args, osso_rpc_t *retval, 
                                AppData *app_data)
{
    /* Removed args, since we need to process them somehow */  
    return osso_rpc_run(app_data->app_osso_data->osso,
                        "com.nokia.app_launcher", 
                        "/com/nokia/app_launcher", 
                        "app_launcher",
                        method, retval, 
                        DBUS_TYPE_INVALID);
}


/* Depending on the state of hw, do something */
void hw_event_handler(osso_hw_state_t *state, gpointer data)
{
    (void) data;
  
    if (state->shutdown_ind) {
        /* Rebooting */ 
        gtk_main_quit();
    }
    if (state->memory_low_ind) {
        /* Memory low */
    }
    if (state->save_unsaved_data_ind) {
        /* Battery low */
    }
    if (state->system_inactivity_ind) {
        /* Minimum activity */ 
    }
}


/* Define topping */
void osso_top_callback(const gchar *arguments, AppData *app_data)
{
    g_assert(app_data);
    gtk_window_present(GTK_WINDOW(app_data));
}


static osso_return_t osso_rpc_cb(const gchar *interface,
				 const gchar *method,
				 GArray *args,
				 gpointer data,
				 osso_rpc_t *retval);

static osso_return_t osso_rpc_cb(const gchar *interface, 
			  const gchar *method,
			  GArray *args, 
			  gpointer data, 
			  osso_rpc_t *retval )
{
  retval->type = DBUS_TYPE_BOOLEAN;
  retval->value.b = TRUE;
  return OSSO_OK;
}


/* Do initialization for OSSO, create osso context, set topping callback,
   dbus-message handling callbaks, and hw-event callbacks. TODO: System 
   bus still not seem working well, so HW-event callbacks no tested */
gboolean init_osso(AppData *app_data)
{
    osso_return_t ret;
  
    /* Init osso */
    osso_log(LOG_INFO, "Initializing osso");
    app_data->app_osso_data->osso = osso_initialize(PACKAGE_NAME, 
                                  PACKAGE_VERSION, TRUE, NULL);
  
    if (app_data->app_osso_data->osso==NULL) {
        osso_log(LOG_ERR, "Osso initialization failed");
        return FALSE;
    }
    g_assert(app_data->app_osso_data->osso);

    /* RPC callback */
    ret = osso_rpc_set_default_cb_f(app_data->app_osso_data->osso,
				    (osso_rpc_cb_f *)osso_rpc_cb, 
				    NULL);
    if (ret != OSSO_OK) {
      ULOG_CRIT( "Error setting RPC callback: %d", ret);
      return FALSE;
    }

    /* Set topping callback */
    osso_application_set_top_cb(app_data->app_osso_data->osso, 
                           (osso_application_top_cb_f *) osso_top_callback,
      (gpointer) app_data);
  
    /* Set handling d-bus messages from session bus */
    ret = osso_rpc_set_cb_f(app_data->app_osso_data->osso,
                           "com.nokia.osso_application_installer", 
                            "/com/nokia/osso_application_installer",
                            "com.nokia.osso_application_installer", 
                            dbus_req_handler, 
			    (gpointer) app_data);
  
    if (ret != OSSO_OK) {
        osso_log(LOG_ERR, "Could not set callback for receiving messages");
    }
  
    /* Set handling changes in HW states. Note: not tested */
    ret = osso_hw_set_event_cb(app_data->app_osso_data->osso, 
			       NULL,
			       hw_event_handler, 
			       app_data);
  
    if (ret != OSSO_OK) {
        osso_log(LOG_ERR, "Could not set callback for HW monitoring");
    }
  
    return TRUE;
}


/* Deinitialize osso specific data TODO:Check of return values from osso */
gboolean deinit_osso(AppData *app_data)
{  
    if (!app_data || !app_data->app_osso_data) return FALSE;
    if (!app_data->app_osso_data->osso) return TRUE;

    /* Unset callbacks */
    g_assert(app_data->app_osso_data->osso);
    osso_application_unset_top_cb(app_data->app_osso_data->osso,
      (osso_application_top_cb_f *) osso_top_callback, NULL);
  
    osso_rpc_unset_cb_f(app_data->app_osso_data->osso, 
                        "com.nokia.osso_application_installer", 
                        "/com/nokia/osso_application_installer",
                        "com.nokia.osso_application_installer", 
                        dbus_req_handler, app_data);

    osso_hw_unset_event_cb(app_data->app_osso_data->osso, NULL);
  
    /* Deinit osso */
    osso_deinitialize(app_data->app_osso_data->osso);
  
    return TRUE;
}
