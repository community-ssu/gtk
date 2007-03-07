/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

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

#ifndef HAVE_HD_WM_WATCHED_WINDOW_H
#define HAVE_HD_WM_WATCHED_WINDOW_H

#include <libhildonwm/hd-wm.h>
#include <libhildonwm/hd-wm-util.h>
#include <libhildonwm/hd-wm-types.h>

/* For watched_window_sync(), should go in enum */

#define HD_WM_SYNC_NAME                (1<<1)
#define HD_WM_SYNC_BIN_NAME            (1<<2)
#define HD_WM_SYNC_WMHINTS             (1<<3)
#define HD_WM_SYNC_TRANSIANT           (1<<4)
#define HD_WM_SYNC_HILDON_APP_KILLABLE (1<<5)
#define HD_WM_SYNC_WM_STATE            (1<<6)
#define HD_WM_SYNC_HILDON_VIEW_LIST    (1<<7)
#define HD_WM_SYNC_HILDON_VIEW_ACTIVE  (1<<8)
#define HD_WM_SYNC_ICON                (1<<9)
#define HD_WM_SYNC_USER_TIME           (1<<10)
#define HD_WM_SYNC_WINDOW_ROLE         (1<<11)
#define HD_WM_SYNC_ALL                 (G_MAXULONG)

/* Application relaunch indicator data*/
#define RESTORED "restored"

HDWMWatchedWindow*
hd_wm_watched_window_new (Window            xid,
			  HDWMWatchableApp *app);
gboolean
hd_wm_watched_window_props_sync (HDWMWatchedWindow *win, gulong props);

HDWMWatchableApp*
hd_wm_watched_window_get_app (HDWMWatchedWindow *win);

Window
hd_wm_watched_window_get_x_win (HDWMWatchedWindow *win);

void
hd_wm_watched_window_reset_x_win (HDWMWatchedWindow * win);

gboolean
hd_wm_watched_window_is_urgent (HDWMWatchedWindow *win);

gboolean
hd_wm_watched_window_wants_no_initial_focus (HDWMWatchedWindow *win);

const gchar*
hd_wm_watched_window_get_name (HDWMWatchedWindow *win);

const gchar*
hd_wm_watched_window_get_sub_name (HDWMWatchedWindow *win);

void
hd_wm_watched_window_set_name (HDWMWatchedWindow *win,
			       const gchar       *name);

void
hd_wm_watched_window_set_gdk_wrapper_win (HDWMWatchedWindow *win,
                                          GdkWindow         *wrapper_win);

GdkWindow *
hd_wm_watched_window_get_gdk_wrapper_win (HDWMWatchedWindow *win);

const gchar*
hd_wm_watched_window_get_hibernation_key (HDWMWatchedWindow *win);


GtkWidget*
hd_wm_watched_window_get_menu (HDWMWatchedWindow *win);

void
hd_wm_watched_window_set_menu (HDWMWatchedWindow *win,
			       GtkWidget         *menu);

/* Note, returns a copy of pixbuf will need freeing */
GdkPixbuf*
hd_wm_watched_window_get_custom_icon (HDWMWatchedWindow *win);

GList*
hd_wm_watched_window_get_views (HDWMWatchedWindow *win);

gint
hd_wm_watched_window_get_n_views (HDWMWatchedWindow *win);

gboolean
hd_wm_watched_window_attempt_signal_kill (HDWMWatchedWindow *win,
                                          int sig,
                                          gboolean ensure);

gboolean
hd_wm_watched_window_is_hibernating (HDWMWatchedWindow *win);

void
hd_wm_watched_window_awake (HDWMWatchedWindow *win);

void
hd_wm_watched_window_destroy (HDWMWatchedWindow *win);

void
hd_wm_ping_timeout( HDWMWatchedWindow *win );

void
hd_wm_ping_timeout_cancel( HDWMWatchedWindow *win );

void
hd_wm_watched_window_close (HDWMWatchedWindow *win);

void
hd_wm_watched_window_set_info (HDWMWatchedWindow *win,
			       HDEntryInfo       *info);

HDEntryInfo *
hd_wm_watched_window_peek_info (HDWMWatchedWindow *win);

HDEntryInfo *
hd_wm_watched_window_create_new_info (HDWMWatchedWindow *win);

void
hd_wm_watched_window_destroy_info (HDWMWatchedWindow *win);

gboolean
hd_wm_watched_window_is_active (HDWMWatchedWindow *win);

#endif
