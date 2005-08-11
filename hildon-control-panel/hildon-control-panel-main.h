/*
 * This file is part of hildon-control-panel
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


#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE
#endif /* dbus_api_subject_to_change */

#ifndef HILDON_CLOCKAPP_CP_MAIN_H
#define HILDON_CLOCKAPP_CP_MAIN_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <locale.h>
#include <libintl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <linux/limits.h> /* not portable. see comments in hildon-desk source code */

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <dbus/dbus.h>

#include <hildon-widgets/hildon-get-password-dialog.h>
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-grid.h>
#include <hildon-base-lib/hildon-base-dnotify.h>
#include <hildon-lgpl/hildon-widgets/gtk-infoprint.h>
#include <hildon-lgpl/hildon-widgets/hildon-defines.h>

#include <libmb/mbdotdesktop.h>
#include <libmb/mbutil.h>

#include <libosso.h>
#include <osso-helplib.h>

#include <unistd.h>

#include "hildon-cp-applist.h"

G_BEGIN_DECLS

#define _(a) gettext(a)

#define APP_NAME "controlpanel"
#define APP_VERSION "0.1"

#define HILDON_CONTROL_PANEL_TITLE _("copa_ap_cp_name")
#define HILDON_CONTROL_PANEL_MENU_OPEN _("copa_me_open")
#define HILDON_CONTROL_PANEL_MENU_SUB_VIEW _("copa_me_view")
#define HILDON_CONTROL_PANEL_MENU_SMALL_ITEMS _("copa_me_view_small")
#define HILDON_CONTROL_PANEL_MENU_LARGE_ITEMS _("copa_me_view_large")
#define HILDON_CONTROL_PANEL_MENU_SUB_TOOLS _("copa_me_tools")
#define HILDON_CONTROL_PANEL_MENU_SETUP_WIZARD _("copa_me_tools_setup_wizard")
#define HILDON_CONTROL_PANEL_MENU_RESET_SETTINGS _("copa_me_tools_rfs")
#define HILDON_CONTROL_PANEL_MENU_HELP _("copa_me_tools_help")
#define HILDON_CONTROL_PANEL_MENU_CLOSE _("copa_me_close")

#define RESET_FACTORY_SETTINGS_PASSWORD_DIALOG_TITLE \
 _("rfs_ti_lock_code_protected")
#define RESET_FACTORY_SETTINGS_PASSWORD_DIALOG_CAPTION _("rfs_fi_enter_lock_code")
#define RESET_FACTORY_SETTINGS_PASSWORD_OK _("refs_bd_ok")
#define RESET_FACTORY_SETTINGS_PASSWORD_CANCEL _("refs_bd_cancel2")


#define RESET_FACTORY_SETTINGS_INFOBANNER \
 _("refs_la_all_application_must_be_closed")
#define RESET_FACTORY_SETTINGS_INFOBANNER_CLOSE_ALL _("refs_bd_close_all")
#define RESET_FACTORY_SETTINGS_INFOBANNER_CANCEL _("refs_bd_cancel1")

#define RESET_FACTORY_SETTINGS_IB_WRONG_LOCKCODE _("rfs_ia_incorrect_lock_code")
#define RESET_FACTORY_SETTINGS_IB_NO_LOCKCODE _("rfs_ia_enter_lock_code")

/* The logical IDs for the applets, needed for automatical generation
   of the .pot files*/

#define HILDON_CP_APPINST_APPLET _("ai_ap_application_title")
#define HILDON_CP_BUPREST_APPLET _("back_ti_dia001_title")
#define HILDON_CP_CERTMAN_APPLET _("cema_ap_application_title")
#define HILDON_CP_CONNE_APPLET _("conn_set_ti_conn_set")
#define HILDON_CP_DATETIME_APPLET _("dati_ap_application_title")
#define HILDON_CP_DEVICE_APPLET _("devi_ap_application_title")
#define HILDON_CP_DISPLAY_APPLET _("disp_ap_application_title")
#define HILDON_CP_LANGREG_APPLET _("cpal_ti_language_and_regional_title")
#define HILDON_CP_MEMORY_APPLET _("memo_ti_memory")
#define HILDON_CP_PERS_APPLET _("pers_ti_personalization")
#define HILDON_CP_SCREENCAL_APPLET _("ctrp_ti_screen_calibration")
#define HILDON_CP_SECURITY_APPLET _("secu_security_dialog_title")
#define HILDON_CP_SOUNDS_APPLET _("snds_ti_sound_settings")
#define HILDON_CP_TEXTINPUT_APPLET _("tein_ti_text_input_title")

#define HILDON_CP_SYSTEM_DIR ".osso/hildon-cp"
#define HILDON_CP_CONF_USER_FILENAME "hildon-cp.conf"
#define HILDON_CP_SYSTEM_DIR_ACCESS   0755
#define HILDON_ENVIRONMENT_USER_HOME_DIR "HOME"
#define HILDON_CP_CONF_FILE_FORMAT    \
 "iconsize=%20s\n"
#define HILDON_CP_MAX_FILE_LENGTH 30 /* CONF FILE FORMAT MUST NOT BE LARGER */

/* max length of state save file */

#define HILDON_CONTROL_PANEL_STATEFILE_MAX 1024
#define OSSO_HELP_ID_CONTROL_PANEL "Utilities_controlpanel_mainview"
#define HILDON_CP_ASSUMED_LOCKCODE_MAXLENGTH 5 /* Note that max number of characters is removed from widget specs. */

G_END_DECLS

#endif
