/**
 * @file osso-cp-plugin.h
 * This file is the internal (private) header file for control panel plugin
 * executing and resetting.
 *
 * This file is part of libosso
 *
 * Copyright (C) 2005-2006 Nokia Corporation. All rights reserved.
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

#ifndef OSSO_CPPLUGIN_H
#define OSSO_CPPLUGIN_H

/* includes */
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <dlfcn.h>
# include <limits.h> 
# include <glib.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>

#define OSSO_CP_CLOSE_IF "com.nokia.controlpanel.close"

/**
 * The exec function template that the library expects;
 * @param osso osso context
 * @param data pointer to a widget
 * @param user_activated True iff activated by user.
 * @return OSSO_ERR, if an error occurred, OSSO_OK else
 */
typedef osso_return_t (osso_cp_plugin_exec_f)(osso_context_t * osso,
					      gpointer data,
					      gboolean user_activated);

/**
 *  Function called from plugin lib when application wants to save
 *  state.
 *
 *  @param osso osso_context
 *  @param data plugin data
 *  @return OSSO_OK iff everything was fine.
 *
 */  
typedef osso_return_t (osso_cp_plugin_save_state_f)(osso_context_t *osso, 
						    gpointer data);



static int _close_lib(GArray *a, const gchar *n);

struct _osso_cp {
    gchar *name;
    gpointer data;
};

#endif
