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
#include <hildon-widgets/hildon-window.h>
#include <hildon-widgets/hildon-program.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-grid.h>
#include <hildon-base-lib/hildon-base-dnotify.h>
#include <hildon-widgets/gtk-infoprint.h>
#include <hildon-widgets/hildon-defines.h>

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
#define HILDON_CONTROL_PANEL_MENU_RFS _("copa_me_tools_rfs")
#define HILDON_CONTROL_PANEL_MENU_CUD _("copa_me_tools_cud")
#define HILDON_CONTROL_PANEL_MENU_HELP _("copa_me_tools_help")
#define HILDON_CONTROL_PANEL_MENU_CLOSE _("copa_me_close")

#define RESET_FACTORY_SETTINGS_INFOBANNER_OK _("rfs_bd_ok")
#define RESET_FACTORY_SETTINGS_INFOBANNER_CANCEL _("rfs_bd_cancel")
#define HILDON_CP_RFS_WARNING _("refs_ia_text")
#define HILDON_CP_CUD_WARNING _("cud_ia_text")

#define HILDON_CP_RFS_WARNING_TITLE _("rfs_ti_restore")
#define HILDON_CP_CUD_WARNING_TITLE _("cud_ti_clear")

#define HILDON_CP_RFS_HELP_TOPIC "Features_restorefactorysettings_closealldialog"
#define HILDON_CP_CUD_HELP_TOPIC "Features_clearuserdata_dialog"

#define HILDON_CP_CODE_DIALOG_HELP_TOPIC "Features_restorefactorysettings_passwordquerydialog"

#define RESET_FACTORY_SETTINGS_IB_WRONG_LOCKCODE dgettext("hildon-libs", "secu_info_incorrectcode")

#define HILDON_CP_SYSTEM_DIR ".osso/hildon-cp"
#define HILDON_CP_CONF_USER_FILENAME "hildon-cp.conf"
#define HILDON_CP_SYSTEM_DIR_ACCESS   0755
#define HILDON_ENVIRONMENT_USER_HOME_DIR "HOME"
#define HILDON_CP_CONF_FILE_FORMAT    \
 "iconsize=%20s\n"
#define HILDON_CP_MAX_FILE_LENGTH 30 /* CONF FILE FORMAT MUST NOT BE LARGER */


#define HILDON_CP_DBUS_MCE_SERVICE "com.nokia.mce"
#define HILDON_CP_DBUS_MCE_REQUEST_IF "com.nokia.mce.request"
#define HILDON_CP_DBUS_MCE_REQUEST_PATH "/com/nokia/mce/request"
#define HILDON_CP_MCE_PASSWORD_VALIDATE "validate_devicelock_code"
#define HILDON_CP_DEFAULT_SALT "$1$JE5Gswee$"

/* max length of state save file */

#define HILDON_CONTROL_PANEL_STATEFILE_MAX 1024
#define OSSO_HELP_ID_CONTROL_PANEL "Utilities_controlpanel_mainview"
#define HILDON_CP_ASSUMED_LOCKCODE_MAXLENGTH 5 /* Note that max number of characters is removed from widget specs. */

gboolean hildon_cp_rfs( osso_context_t *osso, 
                        const char *warning,
                        const char *title,
                        const char *script,
                        const char *help_topic );

G_END_DECLS

#endif
