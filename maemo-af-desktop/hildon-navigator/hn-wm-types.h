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

#ifndef HILDON_NAVIGATOR_WM_TYPES_H
#define HILDON_NAVIGATOR_WM_TYPES_H

enum
{
  HN_ATOM_WM_CLASS,
  HN_ATOM_WM_NAME,
  HN_ATOM_WM_STATE,
  HN_ATOM_WM_TRANSIENT_FOR,
  HN_ATOM_WM_HINTS,

  HN_ATOM_NET_WM_WINDOW_TYPE,
  HN_ATOM_NET_WM_WINDOW_TYPE_MENU,
  HN_ATOM_NET_WM_WINDOW_TYPE_NORMAL,
  HN_ATOM_NET_WM_WINDOW_TYPE_DIALOG,
  HN_ATOM_NET_WM_WINDOW_TYPE_DESKTOP,
  HN_ATOM_NET_WM_STATE,
  HN_ATOM_NET_WM_STATE_MODAL,
  HN_ATOM_NET_SHOWING_DESKTOP,
  HN_ATOM_NET_WM_PID,
  HN_ATOM_NET_ACTIVE_WINDOW,
  HN_ATOM_NET_CLIENT_LIST,
  HN_ATOM_NET_WM_ICON,
  HN_ATOM_NET_WM_USER_TIME,

  HN_ATOM_HILDON_APP_KILLABLE,
  HN_ATOM_HILDON_VIEW_LIST,
  HN_ATOM_HILDON_VIEW_ACTIVE,

  HN_ATOM_MB_WIN_SUB_NAME,
  HN_ATOM_MB_COMMAND,
  HN_ATOM_MB_CURRENT_APP_WINDOW,
  HN_ATOM_MB_APP_WINDOW_LIST_STACKING,

  HN_ATOM_UTF8_STRING,

  HN_ATOM_COUNT
};

typedef struct HNWM HNWM;

/* 
 *  HNWMWatchableApp instances are created from .desktop files and through
 *  key values represent windows that are valid for watching/tracking via 
 *  the HN.
 */
typedef struct HNWMWatchableApp      HNWMWatchableApp;

/* A HNWMWatchedWindow is a running watched / tracked instance of a window 
 * that references a valid HNWMWatchableApp. A watched window *may* contain
 * a list of views ( see below ). 
 */
typedef struct HNWMWatchedWindow     HNWMWatchedWindow;

/* A HNWMWatchedWindowView is a window in a window of a watched window,
 * but to WM it appears as a *single* window, yet to HN as multiples via
 * having a _NET_CLIENT_LIST and _NET_ACTIVE_ID set on *it* rather than 
 * the usual root window.
 */
typedef struct HNWMWatchedWindowView HNWMWatchedWindowView;

/* callbacks for external application switch code - actual menu updates */
typedef struct HNWMCallbacks         HNWMCallbacks;

/* Used to pass data to launch banner timeout callback */
typedef struct HNWMLaunchBannerInfo  HNWMLaunchBannerInfo;

#endif
