/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/* 
 * This file is part of libhildonwm
 *
 * Copyright (C) 2005, 2006, 2007 Nokia Corporation.
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

#ifndef __HD_WM_TYPES_H__
#define __HD_WM_TYPES_H__

enum
{
  HD_ATOM_WM_CLASS,
  HD_ATOM_WM_NAME,
  HD_ATOM_WM_STATE,
  HD_ATOM_WM_TRANSIENT_FOR,
  HD_ATOM_WM_HINTS,
  HD_ATOM_WM_WINDOW_ROLE,
  
  HD_ATOM_NET_WM_WINDOW_TYPE,
  HD_ATOM_NET_WM_WINDOW_TYPE_MENU,
  HD_ATOM_NET_WM_WINDOW_TYPE_NORMAL,
  HD_ATOM_NET_WM_WINDOW_TYPE_DIALOG,
  HD_ATOM_NET_WM_WINDOW_TYPE_DESKTOP,
  HD_ATOM_NET_WM_STATE,
  HD_ATOM_NET_WM_STATE_MODAL,
  HD_ATOM_NET_SHOWING_DESKTOP,
  HD_ATOM_NET_WM_PID,
  HD_ATOM_NET_ACTIVE_WINDOW,
  HD_ATOM_NET_WORKAREA,
  HD_ATOM_NET_CLIENT_LIST,
  HD_ATOM_NET_WM_ICON,
  HD_ATOM_NET_WM_USER_TIME,
  HD_ATOM_NET_WM_NAME,
  HD_ATOM_NET_CLOSE_WINDOW,
  HD_ATOM_NET_WM_STATE_FULLSCREEN,
  
  HD_ATOM_HILDON_APP_KILLABLE,
  HD_ATOM_HILDON_ABLE_TO_HIBERNATE,
  HD_ATOM_HILDON_FROZEN_WINDOW,
  HD_ATOM_HILDON_TN_ACTIVATE,

  HD_ATOM_MB_WIN_SUB_NAME,
  HD_ATOM_MB_COMMAND,
  HD_ATOM_MB_CURRENT_APP_WINDOW,
  HD_ATOM_MB_APP_WINDOW_LIST_STACKING,
  HD_ATOM_MB_NUM_MODAL_WINDOWS_PRESENT,

  HD_ATOM_UTF8_STRING,

  HD_ATOM_STARTUP_INFO,
  HD_ATOM_STARTUP_INFO_BEGIN,

  HD_ATOM_COUNT
};

enum
{
  HD_TN_ACTIVATE_KEY_FOCUS        = 0,
  HD_TN_ACTIVATE_MAIN_MENU        = 1,
  HD_TN_ACTIVATE_ALL_MENU      = 2,
  HD_TN_ACTIVATE_PLUGIN1_MENU     = 3,
  HD_TN_ACTIVATE_PLUGIN2_MENU     = 4,
  HD_TN_ACTIVATE_LAST_APP_WINDOW  = 5,
  HD_TN_DEACTIVATE_KEY_FOCUS      = 6,
  
  HD_TN_ACTIVATE_LAST             = 7
};


/* 
 *  HDWMWatchableApp instances are created from .desktop files and through
 *  key values represent windows that are valid for watching/tracking via 
 *  the HN.
 */

/* callbacks for external application switch code - actual menu updates */
typedef struct HDWMCallbacks         HDWMCallbacks;

typedef struct _HDWMApplication     HDWMApplication;
typedef struct _HDWMWindow   HDWMWindow;
typedef struct _HDWMDesktop HDWMDesktop;

typedef struct _HDEntryInfo HDEntryInfo;

#endif/*__HD_WM_TYPES_H__*/

