/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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

#ifndef __HILDON_HOME_L10N_H__
#define __HILDON_HOME_L10N_H__

#include <glib/gi18n.h>

G_BEGIN_DECLS

/* dgettext macros */
#define _HCS(a)                     dgettext("hildon-common-strings", (a))
#define _HILDON_FM(a)               dgettext("hildon-fm", (a))
#define _KE_RECV(a)                 dgettext("ke-recv", (a))

/* title bar menu */
#define HH_MENU_TITLE               _("home_ap_home_view")        

#define HH_MENU_SELECT              _("home_me_select_applets") 
#define HH_MENU_APPLET_SETTINGS     _("home_me_applet_settings")
#define HH_MENU_EDIT_LAYOUT         _("home_me_edit_layout")
#define HH_MENU_TOOLS               _("home_me_tools")

#define HH_MENU_SET_BACKGROUND      _("home_me_tools_set_background")
#define HH_MENU_PERSONALISATION     _("home_me_tools_personalisation")
#define HH_MENU_CALIBRATION         _("home_me_tools_screen_calibration")
#define HH_MENU_HELP                _("home_me_tools_help")

#define HH_MENU_LAYOUT_TITLE        _("home_ti_layout_mode")

#define HH_MENU_LAYOUT_SELECT       _("home_me_layout_select_applets")
#define HH_MENU_LAYOUT_ACCEPT       _("home_me_layout_accept_layout")
#define HH_MENU_LAYOUT_CANCEL       _("home_me_layout_cancel")
#define HH_MENU_LAYOUT_HELP         _("home_me_layout_help")

/* applet settings unavailable banner */
#define HH_APPLET_SETTINGS_BANNER   _("home_ib_not_available")

/* layout mode unavailable banner */
#define HH_LAYOUT_UNAVAIL_BANNER    _("home_ib_select_applets")

/* layout mode starting banner */
#define HH_LAYOUT_MODE_BANNER       _("home_pb_layout_mode")

/* cancel layout dialog */
#define HH_LAYOUT_CANCEL_TEXT       _("home_nc_cancel_layout")
#define HH_LAYOUT_CANCEL_YES        _("home_bd_cancel_layout_yes")
#define HH_LAYOUT_CANCEL_NO         _("home_bd_cancel_layout_no")

/* overlapping applets notification */
#define HH_LAYOUT_OVERLAP_TEXT      _("home_ni_overlapping_applets")

/* select applets dialog */
#define HH_SELECT_DIALOG_TITLE      _("home_ti_select_applets")
#define HH_SELECT_DIALOG_OK         _("home_bd_select_applets_ok")
#define HH_SELECT_DIALOG_CANCEL     _("home_bd_select_applets_cancel")

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

