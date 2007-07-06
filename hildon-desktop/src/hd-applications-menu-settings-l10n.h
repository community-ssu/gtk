/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
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

#ifndef __HD_APPLICATIONS_MENU_SETTINGS_L10N_H__
#define __HD_APPLICATIONS_MENU_SETTINGS_L10N_H__

#include <glib/gi18n.h>

G_BEGIN_DECLS

/* dgettext macros */
#define _OAT(a)                     dgettext("osso-applet-tasknavigator", (a))

#define HD_APP_MENU_DIALOG_TITLE                _OAT("tncpa_ti_of_title")

#define HD_APP_MENU_DIALOG_NEW                  _OAT("tncpa_bv_of_new_category")
#define HD_APP_MENU_DIALOG_DELETE               _OAT("tncpa_bv_of_delete")
#define HD_APP_MENU_DIALOG_MOVE                 _OAT("tncpa_bv_of_rename")
#define HD_APP_MENU_DIALOG_DONE                 _OAT("tncpa_bv_of_done")

#define HD_APP_MENU_DIALOG_EMPTY_CATEGORY       _OAT("tncpa_li_of_noapps")

#define HD_APP_MENU_DIALOG_NEW_TITLE            _OAT("tncpa_bv_of_new_category")
#define HD_APP_MENU_DIALOG_NEW_NAME             _OAT("tncpa_lb_name")

#define HD_APP_MENU_DIALOG_RENAME_TITLE         _OAT("tncpa_bv_of_rename_category")

#define HD_APP_MENU_DIALOG_ONLY_EMPTY           _OAT("tncpa_ib_only_empty_fold")

#define HD_APP_MENU_DIALOG_NO_APP_DEL           _OAT("tncpa_ib_no_app_del")
#define HD_APP_MENU_DIALOG_NO_APP_REN           _OAT("tncpa_ib_no_app_ren")


#endif

