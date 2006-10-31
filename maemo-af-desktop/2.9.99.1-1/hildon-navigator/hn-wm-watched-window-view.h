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

#ifndef HAVE_HN_WM_WATCHED_WIN_VIEW_H
#define HAVE_HN_WM_WATCHED_WIN_VIEW_H

#include "hn-wm.h"

HNWMWatchedWindowView*
hn_wm_watched_window_view_new (HNWMWatchedWindow *win,
			       int                view_id);

HNWMWatchedWindow*
hn_wm_watched_window_view_get_parent (HNWMWatchedWindowView *view);

gint
hn_wm_watched_window_view_get_id (HNWMWatchedWindowView *view);

GtkWidget*
hn_wm_watched_window_view_get_menu (HNWMWatchedWindowView *view);

const gchar*
hn_wm_watched_window_view_get_name (HNWMWatchedWindowView *view);

void
hn_wm_watched_window_view_set_name (HNWMWatchedWindowView *view,
				    const gchar           *name);
void
hn_wm_watched_window_view_destroy (HNWMWatchedWindowView *view);

void
hn_wm_watched_window_view_close_window (HNWMWatchedWindowView *view);

void
hn_wm_watched_window_view_set_info (HNWMWatchedWindowView *view,
				    HNEntryInfo           *info);

HNEntryInfo *
hn_wm_watched_window_view_get_info (HNWMWatchedWindowView *view);

gboolean
hn_wm_watched_window_view_is_active (HNWMWatchedWindowView *view);

#endif /* HN_WM_WATCHED_WIN_VIEW_H */
