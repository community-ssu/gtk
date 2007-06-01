/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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

#ifndef __HILDON_HOME_L10N_H__
#define __HILDON_HOME_L10N_H__

#include <glib/gi18n.h>

G_BEGIN_DECLS

/* dgettext macros */
#define _MAD(a)                     dgettext("maemo-af-desktop", (a))
#define _HCS(a)                     dgettext("hildon-common-strings", (a))
#define _HILDON_FM(a)               dgettext("hildon-fm", (a))
#define _KE_RECV(a)                 dgettext("ke-recv", (a))

/* title bar menu */
#define HH_MENU_TITLE               _MAD("home_ap_home_view")

#define HH_MENU_SELECT              _MAD("home_me_select_applets")
#define HH_MENU_APPLET_SETTINGS     _MAD("home_me_applet_settings")
#define HH_MENU_EDIT_LAYOUT         _MAD("home_me_edit_layout")
#define HH_MENU_TOOLS               _MAD("home_me_tools")

#define HH_MENU_SET_BACKGROUND      _MAD("home_me_set_background")
#define HH_MENU_PERSONALISATION     _MAD("home_me_set_theme")
#define HH_MENU_CALIBRATION         _MAD("home_me_screen_calibration")
#define HH_MENU_HELP                _MAD("home_me_help")

#define HH_MENU_LAYOUT_TITLE        _MAD("home_ti_layout_mode")

#define HH_MENU_LAYOUT_SELECT       _MAD("home_me_layout_select_applets")
#define HH_MENU_LAYOUT_ACCEPT       _MAD("home_me_layout_accept_layout")
#define HH_MENU_LAYOUT_CANCEL       _MAD("home_me_layout_cancel")
#define HH_MENU_LAYOUT_SNAP_TO_GRID "Snap to grid"
#define HH_MENU_LAYOUT_HELP         _MAD("home_me_layout_help")

/* applet settings unavailable banner */
#define HH_APPLET_SETTINGS_BANNER   _MAD("home_ib_not_available")

/* layout mode unavailable banner */
#define HH_LAYOUT_UNAVAIL_BANNER    _MAD("home_ib_select_applets")

/* layout mode starting banner */
#define HH_LAYOUT_MODE_BANNER       _MAD("home_pb_layout_mode")

/* cancel layout dialog */
#define HH_LAYOUT_CANCEL_TEXT       _MAD("home_nc_cancel_layout")
#define HH_LAYOUT_CANCEL_YES        _MAD("home_bd_cancel_layout_yes")
#define HH_LAYOUT_CANCEL_NO         _MAD("home_bd_cancel_layout_no")

/* overlapping applets notification */
#define HH_LAYOUT_OVERLAP_TEXT      _MAD("home_ni_overlapping_applets")

/* set background dialog */
#define HH_SET_BG_TITLE           _MAD("home_ti_set_backgr")
#define HH_SET_BG_OK              _MAD("home_bd_set_backgr_ok")
#define HH_SET_BG_CANCEL          _MAD("home_bd_set_backgr_cancel")
#define HH_SET_BG_PREVIEW         _MAD("home_bd_set_backgr_preview")
#define HH_SET_BG_IMAGE           _MAD("home_bd_set_backgr_image")
#define HH_SET_BG_COLOR_TITLE     _MAD("home_fi_set_backgr_color")
#define HH_SET_BG_IMAGE_TITLE     _MAD("home_fi_set_backgr_image")
#define HH_SET_BG_IMAGE_NONE      _MAD("home_va_set_backgr_none")
#define HH_SET_BG_MODE_TITLE      _MAD("home_fi_set_backgr_mode")
#define HH_SET_BG_MODE_CENTERED   _MAD("home_va_set_backgr_centered")
#define HH_SET_BG_MODE_SCALED     _MAD("home_va_set_backgr_scaled")
#define HH_SET_BG_MODE_STRETCHED  _MAD("home_va_set_backgr_stretched")
#define HH_SET_BG_MODE_TILED      _MAD("home_va_set_backgr_tiled")

/* generic notification dialogs */
#define HH_LOW_MEMORY_TEXT          _KE_RECV("memr_ib_operation_disabled")
#define HH_FLASH_FULL_TEXT          _KE_RECV("cerm_device_memory_full")
#define HH_NO_CONNECTION_TEXT       _HCS("sfil_ni_cannot_open_no_connection")
#define HH_FILE_CORRUPTED_TEXT      _HCS("ckct_ni_unable_to_open_file_corrupted")
#define HH_MMC_OPEN_TEXT            _HCS("sfil_ni_cannot_open_mmc_cover_open")


/* help topics */
#define HH_HELP                     "uiframework_home_normal_mode"
#define HH_HELP_LAYOUT_MODE         "uiframework_home_layout_mode"
#define HH_HELP_SELECT_APPLETS      "uiframework_home_select_applets"
#define HH_HELP_SET_BACKGROUND      "uiframework_home_set_background"
#define HH_HELP_SELECT_IMAGE        "uiframework_home_select_image"

#endif

