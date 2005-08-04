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

#include <glib.h>
#include <gdk/gdkx.h>
#include <locale.h>

/* hildon includes */
#include "osso-manager.h"
#include "hildon-navigator.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

static char *build_dir( const char *dir );
/* do the actual activating of the application */
/* based on code from the old desk*/
void hildon_navigator_activate( const char* name, const char *exec,
                                const char *param )
{
    osso_manager_t *osso_man;

    osso_man = osso_manager_singleton_get_instance();

    osso_manager_launch(osso_man,exec,param);
    
}

/*caller must free the memory*/

static char *build_dir( const char *dir )
{

    return g_build_path( "/", PREFIX, dir, NULL );
}

char *hildon_navigator_get_data_dir( void )
{
    return build_dir( DATADIR );
}

char *hildon_navigator_get_bin_dir( void )
{
    return build_dir( BINDIR );
}

char *hildon_navigator_get_app_dir( void )
{
    return build_dir( APPDIR );
}

char *hildon_navigator_get_lib_dir( void )
{
    return build_dir( LIBDIR );
}
char *hildon_navigator_get_root_dir( void )
{
    const char *result = g_getenv( HOME_ENV );
    
    return g_strdup(result);
}
