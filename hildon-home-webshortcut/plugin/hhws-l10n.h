/*
 * This file is part of hildon-home-webshortcut
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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

#ifndef __HHWS_L10N_H__
#define __HHWS_L10N_H__

#define _(String) dgettext ("hildon-home-image-viewer", String)
#define _CS(a) dgettext("hildon-common-strings", a)
#define _KE(a) dgettext("ke-recv", a)
#define _MAD(a) dgettext("maemo-af-desktop", a)


#define HHWS_FILE_CHOOSER_TITLE     _("ckdg_ti_select_image")
#define HHWS_SET_TITLE              _("home_ti_web_shrt_props_title")
#define HHWS_SET_OK                 _("home_bd_web_shrt_props_ok")
#define HHWS_SET_BROWSE             _("home_bd_web_shrt_props_browse")
#define HHWS_SET_CANCEL             _("home_bd_web_shrt_props_cancel")
#define HHWS_SET_IMAGE              _("home_fi_web_shrt_props_image")
#define HHWS_SET_URL                _("home_fi_web_shrt_props_url")
#define HHWS_URL_REQ                _("home_ib_url_required_note_text")
#define HHWS_IAP_SCAN_FAIL          _("home_ib_scan_fail")
#define HHWS_CLOSE                  _("ws_me_close")
#define HHWS_SETTINGS               _("ws_me_settings")
#define HHWS_FLASH_FULL             _KE("cerm_device_memory_full")
#define HHWS_CORRUPTED_FILE         _CS("ckct_ni_unable_to_open_file_corrupted")
#define HHWS_COULD_NOT_OPEN         _CS("sfil_ni_unable_to_open_file_not_found")
#define HHWS_NOT_SUPPORTED          _CS("ckct_ni_unable_to_open_unsupported_file_format")
#define HHWS_TITLEBAR_MENU          _MAD("ws_ap_web_shortcut")
#define HHWS_MMC_NOT_OPEN_TEXT      _CS("sfil_ni_cannot_open_mmc_cover_open")
#define HHWS_MMC_NOT_OPEN_CLOSE     _("sfil_ni_cannot_open_mmc_cover_open_ok")
        
#endif
