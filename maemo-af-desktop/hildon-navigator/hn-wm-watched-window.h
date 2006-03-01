/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

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

#ifndef HAVE_HN_WM_WATCHED_WINDOW_H
#define HAVE_HN_WM_WATCHED_WINDOW_H

#include "hn-wm.h"
#include "hn-wm-util.h"
#include "hn-wm-watchable-app.h"

/* For watched_window_sync(), should go in enum */

#define HN_WM_SYNC_NAME                (1<<1)
#define HN_WM_SYNC_BIN_NAME            (1<<2)
#define HN_WM_SYNC_WMHINTS             (1<<3)
#define HN_WM_SYNC_TRANSIANT           (1<<4)
#define HN_WM_SYNC_HILDON_APP_KILLABLE (1<<5)
#define HN_WM_SYNC_WM_STATE            (1<<6)
#define HN_WM_SYNC_HILDON_VIEW_LIST    (1<<7)
#define HN_WM_SYNC_HILDON_VIEW_ACTIVE  (1<<8)
#define HN_WM_SYNC_ICON                (1<<9)
#define HN_WM_SYNC_USER_TIME           (1<<10)
#define HN_WM_SYNC_WINDOW_ROLE         (1<<11)
#define HN_WM_SYNC_ALL                 (G_MAXULONG)

HNWMWatchedWindow*
hn_wm_watched_window_new (Window            xid,
			  HNWMWatchableApp *app);
gboolean
hn_wm_watched_window_props_sync (HNWMWatchedWindow *win, gulong props);

HNWMWatchableApp*
hn_wm_watched_window_get_app (HNWMWatchedWindow *win);

Window
hn_wm_watched_window_get_x_win (HNWMWatchedWindow *win);

gboolean
hn_wm_watched_window_is_urgent (HNWMWatchedWindow *win);

gboolean
hn_wm_watched_window_wants_no_initial_focus (HNWMWatchedWindow *win);

const gchar*
hn_wm_watched_window_get_name (HNWMWatchedWindow *win);

const gchar*
hn_wm_watched_window_get_sub_name (HNWMWatchedWindow *win);

void
hn_wm_watched_window_set_name (HNWMWatchedWindow *win,
			       gchar             *name);

const gchar*
hn_wm_watched_window_get_hibernation_key (HNWMWatchedWindow *win);


GtkWidget*
hn_wm_watched_window_get_menu (HNWMWatchedWindow *win);

void
hn_wm_watched_window_set_menu (HNWMWatchedWindow *win,
			       GtkWidget         *menu);

/* Note, returns a copy of pixbuf will need freeing */
GdkPixbuf*
hn_wm_watched_window_get_custom_icon (HNWMWatchedWindow *win);

GList*
hn_wm_watched_window_get_views (HNWMWatchedWindow *win);

void
hn_wm_watched_window_add_view (HNWMWatchedWindow     *win,
			       HNWMWatchedWindowView *view);

void
hn_wm_watched_window_remove_view (HNWMWatchedWindow     *win,
				  HNWMWatchedWindowView *view);
void
hn_wm_watched_window_set_active_view (HNWMWatchedWindow     *win,
				      HNWMWatchedWindowView *view);

HNWMWatchedWindowView*
hn_wm_watched_window_get_active_view (HNWMWatchedWindow     *win);

gboolean
hn_wm_watched_window_attempt_signal_kill (HNWMWatchedWindow *win, int sig);

gboolean
hn_wm_watched_window_is_hibernating (HNWMWatchedWindow *win);

void
hn_wm_watched_window_awake (HNWMWatchedWindow *win);

void
hn_wm_watched_window_destroy (HNWMWatchedWindow *win);


gboolean hn_wm_watched_window_hibernate_func(gpointer key,
                                             gpointer value,
                                             gpointer user_data);

#endif
