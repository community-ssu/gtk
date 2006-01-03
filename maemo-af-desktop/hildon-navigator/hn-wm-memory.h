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

/**
* @file windowmanager.h
*/

#ifndef HAVE_HN_WM_MEMORY_H
#define HAVE_HN_WM_MEMROY_H

#include "hn-wm.h"

gboolean 
hn_wm_memory_get_limits (guint *pages_used,
			 guint *pages_available);

int 
hn_wm_memory_kill_all_watched (gboolean only_kill_able_to_hibernate);

void 		
hn_wm_shutdown_func(void);

void            
hn_wm_memory_bgkill_func(gboolean is_on) ;

void            
hn_wm_memory_lowmem_func(gboolean is_on);

void 
hn_wm_memory_explain_lowmem (void);

void 
hn_wm_memory_connect_lowmem_explain(GtkWidget *widget);

void 
hn_wm_memory_update_lowmem_ui (gboolean lowmem);

int
hn_wm_memory_kill_lru (void);

#endif
