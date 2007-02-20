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

/**
* @file windowmanager.h
*/

#ifndef __HD_WM_WATCHED_WINDOW_VIEW_H__
#define __HD_WM_WATCHED_WINDOW_VIEW_H__

#include <libhildonwm/hd-wm.h>

HDWMWatchedWindowView*
hd_wm_watched_window_view_new (HDWMWatchedWindow *win,
			       int                view_id);

HDWMWatchedWindow*
hd_wm_watched_window_view_get_parent (HDWMWatchedWindowView *view);

gint
hd_wm_watched_window_view_get_id (HDWMWatchedWindowView *view);

GtkWidget*
hd_wm_watched_window_view_get_menu (HDWMWatchedWindowView *view);

const gchar*
hd_wm_watched_window_view_get_name (HDWMWatchedWindowView *view);

void
hd_wm_watched_window_view_set_name (HDWMWatchedWindowView *view,
				    const gchar           *name);
void
hd_wm_watched_window_view_destroy (HDWMWatchedWindowView *view);

void
hd_wm_watched_window_view_close_window (HDWMWatchedWindowView *view);

void
hd_wm_watched_window_view_set_info (HDWMWatchedWindowView *view,
				    HDEntryInfo           *info);

HDEntryInfo *
hd_wm_watched_window_view_get_info (HDWMWatchedWindowView *view);

gboolean
hd_wm_watched_window_view_is_active (HDWMWatchedWindowView *view);

#endif/* HD_WM_WATCHED_WINDOW_VIEW_H */
