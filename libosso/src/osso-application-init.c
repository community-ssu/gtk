/** 
 * @file osso-application-init.c
 * Libosso initialisation.
 *
 * Copyright (C) 2005 Nokia Corporation.
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

#include "libosso.h"

static void _run_main_loop( void );


osso_context_t * osso_application_initialize(const gchar *application,
				 const gchar *version,
                                 osso_application_top_cb_f *cb,
                                 gpointer callback_data)
{
    osso_context_t *osso;
    
    if ( (osso = osso_initialize(application,version,TRUE,NULL)) == NULL)
        return NULL;
    
    if (osso_application_set_top_cb(osso,
				    cb,
				    callback_data) != OSSO_OK)
    {
        osso_deinitialize(osso);
        return NULL;
    }
        

    _run_main_loop();
    
    
    return osso;
}

static gboolean _rpc_run_ret_cb(gpointer data);
static void _run_main_loop( void )
{
    GSource *to;
    GMainLoop *loop = NULL;

    
    loop = g_main_loop_new(NULL,FALSE);
    
    to = g_timeout_source_new(500);

/*    g_print("Loop = %p\n",(void *)loop);
    g_print("Context = %p\n",((void **)loop)[0]);
  */  
    g_source_set_callback(to, _rpc_run_ret_cb, loop, NULL);

    g_source_attach(to, NULL);

    g_main_loop_run(loop);
    
    g_free(loop);
}

static gboolean _rpc_run_ret_cb(gpointer data)
{
    g_assert(data);


/*    g_print("rpc_run_ret_cb Loop = %p\n",data);
    g_print("rpc_run_ret_cb Context = %p\n",((void **)data)[0]);*/
    g_main_loop_quit( (GMainLoop *)data);
    return 0;
}

const gchar * osso_application_name_get(osso_context_t *osso)
{
	if ( osso == NULL ) {
		return NULL;
	}

	return (const gchar*) (osso->application);
}

const gchar * osso_application_version_get(osso_context_t *osso)
{
	if ( osso == NULL ) {
		return NULL;
	}

	return (const gchar*) (osso->version);
}
