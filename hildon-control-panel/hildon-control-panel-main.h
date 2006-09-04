/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005 Nokia Corporation.
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

#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE
#endif /* dbus_api_subject_to_change */

#ifndef HILDON_CONTROLPANEL_MAIN_H
#define HILDON_CONTROLPANEL_MAIN_H

/* System includes */
#include <libintl.h>
#include <strings.h>

/* Osso includes */
#include <libosso.h>
#include <osso-helplib.h>

/* GLib includes */
#include <glib.h>
#include <glib/gstdio.h>

/* GDK includes */
#include <gdk/gdkkeysyms.h>

/* GTK includes */
#include <gtk/gtk.h>

/* Hildon widgets */
#include <hildon-widgets/hildon-program.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/gtk-infoprint.h>

#include <hildon-base-lib/hildon-base-dnotify.h>
#include <hildon-base-lib/hildon-base-types.h>

/* Control Panel includes */
#include "hildon-cp-applist.h"
#include "hildon-cp-item.h"
#include "cp-grid.h"


G_BEGIN_DECLS


#define _(a) gettext(a)

#define APP_NAME     "controlpanel"
#define APP_VERSION  "0.1"

#define HILDON_CONTROL_PANEL_TITLE             _("copa_ap_cp_name")
#define HILDON_CONTROL_PANEL_MENU_OPEN         _("copa_me_open")
#define HILDON_CONTROL_PANEL_MENU_SUB_VIEW     _("copa_me_view")
#define HILDON_CONTROL_PANEL_MENU_SMALL_ITEMS  _("copa_me_view_small")
#define HILDON_CONTROL_PANEL_MENU_LARGE_ITEMS  _("copa_me_view_large")
#define HILDON_CONTROL_PANEL_MENU_SUB_TOOLS    _("copa_me_tools")
#define HILDON_CONTROL_PANEL_MENU_SETUP_WIZARD _("copa_me_tools_setup_wizard")
#define HILDON_CONTROL_PANEL_MENU_RFS          _("copa_me_tools_rfs")
#define HILDON_CONTROL_PANEL_MENU_CUD          _("copa_me_tools_cud")
#define HILDON_CONTROL_PANEL_MENU_HELP         _("copa_me_tools_help")
#define HILDON_CONTROL_PANEL_MENU_CLOSE        _("copa_me_close")

#define RESET_FACTORY_SETTINGS_INFOBANNER_OK     _("rfs_bd_ok")
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

#define HCP_GCONF_ICON_SIZE         "/apps/osso/apps/controlpanel/icon_size"

#define HCP_STATE_GROUP             "HildonControlPanel"
#define HCP_STATE_FOCUSSED          "Focussed"
#define HCP_STATE_SCROLL_VALUE      "ScrollValue"
#define HCP_STATE_EXECUTE           "Execute"


#define OSSO_HELP_ID_CONTROL_PANEL "Utilities_controlpanel_mainview"
#define HILDON_CP_ASSUMED_LOCKCODE_MAXLENGTH 5 /* Note that max number of characters is removed from widget specs. */


/* DBus RPC */
#define HCP_RPC_SERVICE                     "com.nokia.controlpanel"
#define HCP_RPC_PATH                        "/com/nokia/controlpanel/rpc"
#define HCP_RPC_INTERFACE                   "com.nokia.controlpanel.rpc"
#define HCP_RPC_METHOD_RUN_APPLET           "run_applet"
#define HCP_RPC_METHOD_SAVE_STATE_APPLET    "save_state_applet"
#define HCP_RPC_METHOD_TOP_APPLICATION      "top_application"
#define HCP_RPC_METHOD_IS_APPLET_RUNNING    "is_applet_running"

typedef struct _HCP HCP;

struct _HCP {
    HildonProgram *program;
    HildonWindow *window;
    HCPAppList *al;
    HCPItem *focused_item;
    GtkWidget *view;
    GtkWidget *large_icons_menu_item;
    GtkWidget *small_icons_menu_item;
    GList *grids;
    osso_context_t *osso;

    /* for state save data */
    gint icon_size;
    gchar *saved_focused_filename;
    gint scroll_value;
    gint execute;
};
 
gboolean hcp_rfs (HCP *hcp,
                  const char *warning,
                  const char *title,
                  const char *script,
                  const char *help_topic);


G_END_DECLS

#endif
