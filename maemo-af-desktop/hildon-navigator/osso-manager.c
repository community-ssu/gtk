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


/*
 *
 * @file osso-manager.c
 *
 */

 
/* System includes */
#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

/* log include */
#include <log-functions.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

/* hildon includes */
#include "osso-manager.h"



static int start_osso(osso_manager_t *man, GMainContext *context);
static int osso_callback(const gchar *interface,
                        const gchar *method,
                        GArray *arguments,
                        gpointer data,
                        osso_rpc_t *retval);


static int start_osso(osso_manager_t *man, GMainContext *context)
{
    if (man->osso != NULL)
        return -1;
    
    man->osso = osso_initialize(TASKNAV,TASKNAV_VERSION,FALSE,NULL);

    if (!man->osso)
    {
        osso_log(LOG_ERR,
                "Could not initialize osso from task nav. DBUS running ?");

        return -1;
        
    }
    osso_rpc_set_default_cb_f(man->osso,(osso_rpc_cb_f*)osso_callback,man);
    return 0;
}


osso_manager_t *osso_manager_singleton_get_instance( void )
{
    static osso_manager_t *instance =NULL;

    
    if (!instance)
    {
        instance = g_malloc0(sizeof(osso_manager_t));
    
        instance->methods = g_array_new(FALSE,TRUE,sizeof(osso_method));
    
        if (start_osso(instance, NULL))
        {
            osso_log(LOG_ERR, "Could not connect to osso");
            return NULL;
        }
    }

    
    return instance;
}

static int osso_callback(const gchar *interface,
                        const gchar *method,
                        GArray *arguments,
                        gpointer data,
                        osso_rpc_t *retval)
{

    osso_manager_t *man;
    int n;

    man = (osso_manager_t *)data;
    
    retval->type = DBUS_TYPE_INVALID;
    
    
       
    for ( n=0;n< man->methods->len;n++)
    {
        osso_method *met;
        met = &g_array_index(man->methods,osso_method,n);
        if (!strncmp(method,met->name,METHOD_NAME_LEN))
        {
            
            if (met->method(arguments,met->data) == OSSO_ERROR)
            {
                retval->type = DBUS_TYPE_STRING; /* err */
                retval->value.s = "Error in callback method!\n";
                osso_log(LOG_ERR,"Callback method %s failed\n",method);
                return OSSO_ERROR;
            }
            break;
        }
    }


    if (n == man->methods->len)
    {
        retval->type = DBUS_TYPE_STRING; /* err */
        retval->value.s = "Unknown method called!\n";
        
        return OSSO_ERROR;
    }

    retval->type = DBUS_TYPE_INT32;
    retval->value.i = 0;

    return OSSO_OK;
}

void add_method_cb(osso_manager_t *man,const gchar *name,
                tasknav_cb_f *method,gpointer data)
{
    osso_method met;
                             
    g_snprintf(met.name,METHOD_NAME_LEN,"%s",name);
    met.method = method;
    met.data = data;

    g_array_append_val(man->methods,met);
}

void osso_manager_launch(osso_manager_t *man, const gchar *app,
        const gchar *launch_param)
{
    char service[SERVICE_NAME_LEN],
        path[PATH_NAME_LEN],
        interface[INTERFACE_NAME_LEN],
        tmp[TMP_NAME_LEN];

    /* If we have full service name we will use it*/
    if (g_strrstr(app,"."))
    {
        g_snprintf(service,SERVICE_NAME_LEN,"%s",app);
        g_snprintf(interface,INTERFACE_NAME_LEN,"%s",service);
        g_snprintf(tmp,TMP_NAME_LEN,"%s",app);
        g_snprintf(path,PATH_NAME_LEN,"/%s",g_strdelimit(tmp,".",'/'));
    }
    else /* we will use com.nokia prefix*/
    {
        g_snprintf(service,SERVICE_NAME_LEN,"%s.%s",OSSO_BUS_ROOT,app);
        g_snprintf(path,PATH_NAME_LEN,"%s/%s",OSSO_BUS_ROOT_PATH,app);
        g_snprintf(interface,INTERFACE_NAME_LEN,"%s",service);
    }

    d_log(LOG_D,"Launch: s:%s\np:%s\ni:%s\n",service,path,interface);

    if (launch_param != NULL)
      {
	if ( (osso_rpc_run( man->osso, service, path, interface,
			    OSSO_BUS_TOP ,
			    NULL, DBUS_TYPE_STRING, launch_param,
			    DBUS_TYPE_INVALID )) != OSSO_OK )
	  {
	    osso_log(LOG_ERR, "Failed to restart!" );
	  }
      }
    else
      {
	if ( (osso_rpc_run( man->osso, service, path, interface,
			    OSSO_BUS_TOP ,
			    NULL, DBUS_TYPE_INVALID )) != OSSO_OK ) {
	  osso_log(LOG_ERR, "Failed to launch!" );
	}
      }
}


void osso_manager_infoprint(osso_manager_t *man, const gchar *message)
{
    osso_rpc_t retval;
    if (osso_system_note_infoprint(man->osso, message, &retval) != OSSO_OK)
        osso_log(LOG_ERR,"Could not show infoprint %s\n",message);

}

/** Method to set the x window to be used by the osso manager */
void osso_manager_set_window(osso_manager_t *man,Window win)
{
    g_assert(win);
    man->window=win;
}

int is_service_running(const char *service)
{
    osso_manager_t *man;
    DBusConnection *con;
    DBusError error;
    char buf[DBUS_BUF_SIZE];
    int n;

    g_snprintf(buf,sizeof(buf),"%s%s",SERVICE_PREFIX,service);

    man = osso_manager_singleton_get_instance();
    if(man == NULL)
    {
        return -1;
    }
    
    con = (DBusConnection *)osso_get_dbus_connection(man->osso);

    dbus_error_init(&error);
    n = dbus_bus_name_has_owner(con,buf,&error);

    if (dbus_error_is_set(&error))
    {
        osso_log(LOG_ERR, "Error detecting if %s is running: %s\n",
                            service,
                            error.message);
        dbus_error_free(&error);
        return -1;
    }

    return n;
}

osso_context_t *get_context(osso_manager_t *man)
{
    return man->osso;
}
