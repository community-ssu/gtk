/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
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

/* glib include */
#include <glib.h>
/* gdkx include */
#include <gdk/gdkx.h>
/* locale include */
#include <locale.h>

/* hildon includes */
#include "osso-manager.h"
#include "hildon-navigator.h"
#include "hn-wm-memory.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* do the actual activating of the application */
/* based on code from the old desk*/
void hildon_navigator_activate( const char* name, const char *exec,
                                const char *param )
{
    osso_manager_t *osso_man;

    osso_man = osso_manager_singleton_get_instance();

    osso_manager_launch(osso_man,exec,param);
    
}

void hildon_navigator_killall(void)
{
    hn_wm_memory_kill_all_watched(FALSE);
}
