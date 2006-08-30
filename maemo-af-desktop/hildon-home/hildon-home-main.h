/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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
 * @file hildon-home-main.h
 */

#ifndef HILDON_CLOCKAPP_HOME_MAIN_H
#define HILDON_CLOCKAPP_HOME_MAIN_H

#include "hildon-home-interface.h"

G_BEGIN_DECLS

/* generic and not home specific values */
#define COMMON_STRING(a) dgettext("hildon-common-strings", a)
#define FM(a) dgettext("hildon-fm", a)

#define HILDON_MENU_KEY                 GDK_F4

#define WINDOW_WIDTH                    800
#define WINDOW_HEIGHT                   480
#define HILDON_HOME_ENV_HOME            "HOME"

#define HILDON_HOME_FLASH_FULL_TEXT      dgettext("ke-recv", "cerm_device_memory_full")
#define HILDON_HOME_MMC_NOT_OPEN_TEXT   COMMON_STRING("sfil_ni_cannot_open_mmc_cover_open")
#define HILDON_HOME_MMC_NOT_OPEN_CLOSE  COMMON_STRING("sfil_ni_cannot_open_mmc_cover_open_ok")

/* Home */
#define HILDON_HOME_NAME                "Home"
#define HILDON_HOME_WINDOW_NAME         "HildonHome"
#define HILDON_HOME_VERSION             "2.1.0"
#define HILDON_HOME_SYSTEM_DIR          ".osso/hildon-home"
#define HILDON_HOME_SYSTEM_DIR_ACCESS   0755
#define HILDON_HOME_PATH_STR_LENGTH     2048

/* Default factory values */
#define HILDON_HOME_FACTORY_FILENAME    "/etc/hildon-home/hildon-home.conf"
#define HILDON_HOME_FACTORY_FORMAT    \
 "WShide=%d\nWSproperties=%d\nBGchange=%d\nBGfile=%200s\n"

/* Hard code values which are used when no other values are available */
#define HILDON_HOME_HC_USER_IMAGE_DIR    "MyDocs/.images"
#define HILDON_HOME_CONF_USER_FILENAME   "hildon-home.conf"
#define HILDON_HOME_CONF_USER_ORIGINAL_FILENAME  "user_filename.txt"
#define HILDON_HOME_CONF_USER_IMAGE_DIR  "MyDocs/.images"
#define HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR  "/usr/share/backgrounds"
  	 
/* background image related definitions */ 	 
#define BG_DESKTOP_GROUP           "Desktop Entry"
#define BG_DESKTOP_IMAGE_NAME      "Name"
#define BG_DESKTOP_IMAGE_FILENAME  "File"
#define BG_DESKTOP_IMAGE_PRIORITY  "X-Order"
#define BG_IMG_INFO_FILE_TYPE      "desktop"
#define HOME_BG_IMG_DEFAULT_PRIORITY  15327 /* this is a random number */
#define BG_LOADING_PIXBUF_NULL    -526
#define BG_LOADING_OTHER_ERROR    -607
#define BG_LOADING_RENAME_FAILED  -776
#define BG_LOADING_SUCCESS        0
#define MAX_CHARS_HERE            6

#define HILDON_HOME_SET_BG_TITLE    _("home_ti_set_backgr")
#define HILDON_HOME_SET_BG_OK       _("home_bd_set_backgr_ok")
#define HILDON_HOME_SET_BG_PREVIEW  _("home_bd_set_backgr_preview")
#define HILDON_HOME_SET_BG_IMAGE    _("home_bd_set_backgr_image")  
#define HILDON_HOME_SET_BG_CANCEL   _("home_bd_set_backgr_cancel")   

#define HILDON_HOME_SET_BG_RESPONSE_PREVIEW  GTK_RESPONSE_YES
#define HILDON_HOME_SET_BG_RESPONSE_IMAGE    GTK_RESPONSE_APPLY 
 
#define HILDON_HOME_SET_BG_COLOR_TITLE     _("home_fi_set_backgr_color")
#define HILDON_HOME_SET_BG_IMAGE_TITLE     _("home_fi_set_backgr_image")
#define HILDON_HOME_SET_BG_IMAGE_NONE      _("home_va_set_backgr_none")
#define HILDON_HOME_SET_BG_MODE_TITLE      _("home_fi_set_backgr_mode") 
#define HILDON_HOME_SET_BG_MODE_CENTERED   _("home_va_set_backgr_centered")
#define HILDON_HOME_SET_BG_MODE_SCALED     _("home_va_set_backgr_scaled")
#define HILDON_HOME_SET_BG_MODE_STRETCHED  _("home_va_set_backgr_stretched")
#define HILDON_HOME_SET_BG_MODE_TILED      _("home_va_set_backgr_tiled") 
  
#define HILDON_HOME_FILE_CHOOSER_ACTION_PROP  "action"
#define HILDON_HOME_FILE_CHOOSER_TITLE_PROP   "title"
#define HILDON_HOME_FILE_CHOOSER_TITLE        _("home_ti_select_image")
#define HILDON_HOME_FILE_CHOOSER_SELECT_PROP  "open-button-text"
#define HILDON_HOME_FILE_CHOOSER_SELECT       _("home_bd_select_image")
#define HILDON_HOME_FILE_CHOOSER_EMPTY_PROP   "empty-text"
#define HILDON_HOME_FILE_CHOOSER_EMPTY        _("home_li_no_images")


#define HILDON_HOME_CONF_USER_FORMAT \
        "red=%d\ngreen=%d\nblue=%d\nmode=%d\n"
#define HILDON_HOME_CONF_DEFAULT_COLOR 0

        
#define HILDON_HOME_USER_PLUGIN_PATH "/var/lib/install/usr/lib/hildon-home/"
#define HILDON_HOME_USER_PLUGIN_CONF_FORMAT    \
    "plugin=%s\nwidth=%d\nheight=%d\nx=%d\ny=%d\n"

#define STARTUP_LOCK_FILE  "/var/lock/hildon-home-startup"
#define STARTUP_LOCK_TIME  10000

/* user saved values */
#define HILDON_HOME_TEMPORARY_FILENAME_EXT   "_tmp"
#define HILDON_HOME_BG_USER_FILENAME         "hildon_home_bg_user.png"
#define HILDON_HOME_BG_USER_FILENAME_TEMP    "hildon_home_bg_user.png_tmp"
#define HILDON_HOME_BG_FILENAME_FORMAT       "%s\n"
#define HILDON_HOME_BG_FILENAME_FORMAT_SAVE  "%200s\n"

#define HILDON_HOME_IMAGE_LOADER       "home-image-loader"
#define HILDON_HOME_IMAGE_LOADER_PATH  "/usr/bin/home-image-loader"
#define HILDON_HOME_IMAGE_LOADER_NICE  19

#define HILDON_HOME_BLEND_IMAGE_TITLEBAR_NAME  "HildonHomeTitleBar"
#define HILDON_HOME_BLEND_IMAGE_SIDEBAR_NAME   "HildonHomeLeftEdge"

#define HILDON_HOME_ORIGINAL_IMAGE_TITLEBAR  "original_titlebar.png"
#define HILDON_HOME_ORIGINAL_IMAGE_SIDEBAR   "original_sidebar.png"

#define HILDON_HOME_WIDTH        (WINDOW_WIDTH-HILDON_TASKNAV_WIDTH)
#define HILDON_HOME_HEIGHT       WINDOW_HEIGHT
#define HILDON_HOME_X            HILDON_TASKNAV_WIDTH
#define HILDON_HOME_Y            0
#define HILDON_HOME_AREA_WIDTH   HILDON_HOME_WIDTH 
#define HILDON_HOME_AREA_HEIGHT  HILDON_HOME_HEIGHT
#define HILDON_HOME_AREA_X       0
#define HILDON_HOME_AREA_Y       0


/* title bar and menu */
#define HILDON_HOME_TITLEBAR_NAME     "HildonHomeTitleBar"
#define HILDON_HOME_TITLEBAR_WIDTH    (WINDOW_WIDTH-HILDON_TASKNAV_WIDTH)
#define HILDON_HOME_TITLEBAR_X        0
#define HILDON_HOME_TITLEBAR_Y        0
#define HILDON_HOME_TITLEBAR_LEFT_X  "0"
#define HILDON_HOME_TITLEBAR_TOP_Y   "0"

#define HILDON_HOME_TITLEBAR_MENU_NAME   "menu_force_with_corners"
#define HILDON_HOME_TITLEBAR_MENU_LABEL  _("home_ap_home_view")

#define HILDON_HOME_TITLEBAR_MENU_SELECT_APPLETS   _("home_me_select_applets")
#define HILDON_HOME_TITLEBAR_MENU_APPLET_SETTINGS  _("home_me_applet_settings")
#define HILDON_HOME_MENU_APPLET_SETTINGS_NOAVAIL   _("home_ib_not_available")
#define HILDON_HOME_MENU_EDIT_LAYOUT_NOAVAIL       _("home_ib_select_applets")
#define HILDON_HOME_TITLEBAR_MENU_EDIT_LAYOUT      _("home_me_edit_layout")
#define HILDON_HOME_TITLEBAR_MENU_TOOLS            _("home_me_tools")
#define HILDON_HOME_TITLEBAR_SUB_SET_BG            _("home_me_tools_set_background")
#define HILDON_HOME_TITLEBAR_SUB_PERSONALISATION   _("home_me_tools_personalisation")
#define HILDON_HOME_TITLEBAR_SUB_CALIBRATION       _("home_me_tools_screen_calibration")
#define HILDON_HOME_TITLEBAR_SUB_HELP              _("home_me_tools_help")

#define HILDON_CP_DESKTOP_NAME            "Name"
#define HILDON_CP_PLUGIN_PERSONALISATION  "personalisation.desktop"
#define HILDON_CP_PLUGIN_CALIBRATION      "tscalibrate.desktop"

/* help topics */
#define HILDON_HOME_NORMAL_HELP_TOPIC           "uiframework_home_normal_mode"
#define HILDON_HOME_LAYOUT_HELP_TOPIC           "uiframework_home_layout_mode"
#define HILDON_HOME_SELECT_APPLETS_HELP_TOPIC   "uiframework_home_select_applets"
#define HILDON_HOME_SET_BACKGROUND_HELP_TOPIC   "uiframework_home_set_background"
#define HILDON_HOME_SELECT_IMAGE_HELP_TOPIC   "uiframework_home_select_image"

/* The edge offsets used for aligning the menu if no theme information is
   available */

#define HILDON_HOME_TITLEBAR_X_OFFSET_DEFAULT  10 
#define HILDON_HOME_TITLEBAR_Y_OFFSET_DEFAULT  -13 

#define HILDON_HOME_MENUAREA_WIDTH      348
#define HILDON_HOME_MENUAREA_LMARGIN    0

/* skin area */
#define HILDON_HOME_SIDEBAR_NAME    "HildonHomeLeftEdge"
#define HILDON_HOME_SIDEBAR_WIDTH   10
#define HILDON_HOME_SIDEBAR_HEIGHT  (WINDOW_HEIGHT-HILDON_HOME_TITLEBAR_HEIGHT)
#define HILDON_HOME_SIDEBAR_X       0
#define HILDON_HOME_SIDEBAR_Y       HILDON_HOME_TITLEBAR_HEIGHT
#define HILDON_HOME_SIDEBAR_LEFT_X  "0"
#define HILDON_HOME_SIDEBAR_TOP_Y   "60" /* HILDON_HOME_TITLEBAR_HEIGHT */


/* applet area values */
#define HILDON_HOME_APP_AREA_NAME       "HildonHomeAppArea"
#define HILDON_HOME_APP_AREA_WIDTH      (HILDON_HOME_AREA_WIDTH-\
                                         HILDON_HOME_SIDEBAR_WIDTH)
#define HILDON_HOME_APP_AREA_HEIGHT     HILDON_HOME_SIDEBAR_HEIGHT
#define HILDON_HOME_APP_AREA_X          HILDON_HOME_SIDEBAR_WIDTH
#define HILDON_HOME_APP_AREA_Y          HILDON_HOME_SIDEBAR_Y
#define HILDON_HOME_APP_AREA_MARGIN       9
#define HILDON_HOME_APP_AREA_MARGIN_INNER 6

/* generic applet values */
#define HILDON_HOME_APPLET_BORDER_WIDTH 6
#define HILDON_HOME_APPLET_LEFT_X       HILDON_HOME_APP_AREA_X+\
                                        HILDON_HOME_APP_AREA_MARGIN
#define HILDON_HOME_APPLET_TOP_Y        HILDON_HOME_APP_AREA_Y+\
                                        HILDON_HOME_APP_AREA_MARGIN

/* HOM-NOT006*/
#define HILDON_HOME_LOADING_IMAGE_TEXT    _("home_nw_loading_image")
#define HILDON_HOME_LOADING_IMAGE_ANI     "qgn_indi_process_a"
#define HILDON_NOTE_INFORMATION_ICON      "qgn_note_info"

/* WID-NOT201*/
#define HILDON_HOME_NO_MEMORY_TEXT       COMMON_STRING("sfil_ni_not_enough_memory")

/* WID-NOT202 */
#define HILDON_HOME_CONNECTIVITY_TEXT    COMMON_STRING("sfil_ni_cannot_open_no_connection")


#define HILDON_HOME_CORRUPTED_TEXT    COMMON_STRING("ckct_ni_unable_to_open_file_corrupted")

/* FIL-INF010 */
#define HILDON_HOME_FILE_UNREADABLE_TEXT FM("sfil_ib_opening_not_allowed")


#define TRANSIENCY_MAXITER 50
										
enum { 	 
     BG_IMAGE_NAME, 	 
     BG_IMAGE_FILENAME, 	 
     BG_IMAGE_PRIORITY
}; 	

#if 0
void show_no_memory_note(void);
void show_connectivity_broke_note(void);
void show_system_resource_note(void);
void show_file_corrupt_note(void);
void show_file_unreadable_note(void);
void show_mmc_cover_open_note(void);
void show_flash_full_note(void);

void home_layoutmode_menuitem_sensitivity_check(void);
#endif

osso_context_t * home_get_osso_context (void);

G_END_DECLS

#endif
