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

#ifndef HAVE_HN_WM_UTILS_H
#define HAVE_HN_WM_UTILS_H

#include "hn-wm.h"

gulong 
hn_wm_util_getenv_long (gchar *env_str, gulong val_default);

void*
hn_wm_util_get_win_prop_data_and_validate (Window     xwin, 
					   Atom       prop, 
					   Atom       type, 
					   int        expected_format,
					   int        expected_n_items,
					   int       *n_items_ret);

gboolean
hn_wm_util_send_x_message (Window        xwin_src, 
			   Window        xwin_dest, 
			   Atom          delivery_atom,
			   long          mask,
			   unsigned long data0,
			   unsigned long data1,
			   unsigned long data2,
			   unsigned long data3,
			   unsigned long data4);

gint hn_wm_get_vmdata_for_pid(gint pid);

#endif
