/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
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

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

/* hildon includes */
#include "osso-manager.h"



static int start_osso(osso_manager_t *man, GMainContext *context);
#ifdef HAVE_LIBOSSO
static int osso_callback(const gchar *interface,
                        const gchar *method,
                        GArray *arguments,
                        gpointer data,
                        osso_rpc_t *retval);
#endif


static int start_osso(osso_manager_t *man, GMainContext *context)
{
#ifdef HAVE_LIBOSSO
    if (man->osso != NULL)
        return -1;
    
    man->osso = osso_initialize(TASKNAV,TASKNAV_VERSION,FALSE,NULL);

    if (!man->osso)
    {
        g_warning ("Could not initialize osso from task nav. DBUS running ?");

        return -1;
        
    }
    osso_rpc_set_default_cb_f(man->osso,(osso_rpc_cb_f*)osso_callback,man);
#endif
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
            g_warning ("Could not connect to osso");
            return NULL;
        }
    }

    
    return instance;
}

#ifdef HAVE_LIBOSSO
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
                g_warning ("Callback method %s failed\n",method);
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
#endif

void add_method_cb(osso_manager_t *man,const gchar *name,
                tasknav_cb_f *method,gpointer data)
{
    osso_method met;
                             
    g_snprintf(met.name,METHOD_NAME_LEN,"%s",name);
    met.method = method;
    met.data = data;

    g_array_append_val(man->methods,met);
}

