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

#ifndef HILDON_NAVIGATOR_WM_H
#define HILDON_NAVIGATOR_WM_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkevents.h>

#include <gtk/gtk.h>

#include <hildon-navigator.h>

#include <libmb/mbdotdesktop.h>

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#define DESKTOP_BIN_FIELD   "Exec"
#define DESKTOP_LAUNCH_FIELD    "X-Osso-Service"
#define DESKTOP_VISIBLE_FIELD "Name"



#define DUMMY_STRING " \0"

#define KILL_APPS_METHOD "kill_app"

/* The window types that we care about */

enum {
    OTHER = 0,
    UNKNOWN,
    NORMAL_WINDOW,
    MODAL_DIALOG,
    DESKTOP,
    MENU
};


#define APPKILLER_SIGNAL_INTERFACE "com.nokia.osso_app_killer"
#define APPKILLER_EXIT_SIGNAL "exit"

#define APP_LAUNCH_BANNER_METHOD_INTERFACE "com.nokia.tasknav.app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD_PATH     "/com/nokia/tasknav/app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD             "app_launch_banner"
#define APP_LAUNCH_BANNER_MSG_LOADING        "ckct_ib_application_loading"
#define APP_LAUNCH_BANNER_MSG_LOADING_FAILED "ckct_ib_application_loading_failed"
/* Timeout of the launch banner, in secons */
#define APP_LAUNCH_BANNER_TIMEOUT            20
/* Interval for checking for new window or timeout, in seconds */
#define APP_LAUNCH_BANNER_CHECK_INTERVAL     0.5

#define LAUNCH_SUCCESS_TIMEOUT 20
#define LAUNCH_FAILED_INSUF_RES "cerm_can_not_open_insuf_res"

#define LOCATION_VAR "STATESAVEDIR"
#define FALLBACK_PREFIX "/tmp/state"
#define KILL_DELAY 2000 /* 2 seconds */

#define HOME_WMCLASS "maemo_af_desktop"

/* hildon, from mb's struct.h */
#define MB_CMD_DESKTOP 3

#define MAX_SESSION_SIZE 2048

/* Ideally, these should probably come from an external header file...
   However, are these available from anywhere else but appkiller headers? */

#define SAVE_METHOD "save"

#define DESKTOP_ICON_FIELD "Icon"
#define DESKTOP_SUP_WMCLASS "StartupWMClass"
#define DESKTOP_SUFFIX ".desktop"
#define UNKNOWN_TITLE "Unknown"

enum
{
    WM_ICON_NAME_ITEM =0, /* Normal icon */
    WM_NAME_ITEM, /* The name of the application */
    WM_EXEC_ITEM, /* The service name of the item */
    WM_DIALOG_ITEM,
    WM_ID_ITEM, /* The window ID */
    WM_BIN_NAME_ITEM, /* The actual binary name */
    WM_MENU_PTR_ITEM, /* Pointer for the new menuitem (as GtkWidget) */
    WM_VIEW_ID_ITEM,
    WM_VIEW_NAME_ITEM, /* The name of the individual view */ 
    WM_KILLABLE_ITEM, /* If set to 1, application can be killed */
    WM_KILLED_ITEM, /* The application has been killed */
    WM_MINIMIZED_ITEM, /* The application has been minimized */
    WM_NUM_TASK_ITEMS
};


union atom_value {
    unsigned char *char_value;
    Atom *atom_value;
    
};

union win_value {
    unsigned char *char_value;
    Window *window_value;
};

union wmstate_value {
    unsigned char *char_value;
    unsigned long *state_value;
};


/* A structure for the viewlist clearance */

typedef struct
{
    Window window_id;
    union win_value *viewlist;
    gulong num_views;
} view_clean_t;

typedef struct
{
    gpointer menu_ptr;
    gulong view_id;
    gchar *service;
    gchar *wm_class;
    gulong window_id;
    gboolean killable;
} menuitem_comp_t;

struct state_data{
    gchar running_apps[MAX_SESSION_SIZE];
} statedata;


/* Structure used by insertion of a new window */

typedef struct
{
    gchar *name;
    gchar *exec;
    gchar *icon;
    gint is_dialog;
    gulong win_id;
    gulong view_id;
    guchar *dialog_name; /* Used to pass the dialog name to dimmer */
    GtkWidget *menuitem_widget;
    
} window_props;


/* Callback definitions*/

/** Function callback to be called when a new window/view
 *  is created.
 *  @param icon_name The name of icon for this menu entry
 *  @param app_name The name of application for this menu entry
 *  @param view_name The title for this window/view
 *  @param data The callback data
 *  @return GtkWidget pointer to this menu item
 */

typedef GtkWidget *(wm_new_window_cb)(const gchar *icon_name,
                                      const gchar *app_name,
                                      const gchar *view_name,
                                      gpointer data);

/** Function callback to be called when a window is removed.
 *
 *  @param menuitem Pointer to the menuitem that corresponds with
 *                  the window that was removed
 *  @param data Pointer to callback data
 */

typedef void (wm_removed_window_cb)(GtkWidget *menuitem,
                                     gpointer data);


/** Function callback to be called when a window has been changed.
 *  Mostly used to signal when the name of the view is set,
 *  because this has probably not yet happened when the CreateNotify
 *  event is handled.
 *
 * @param menuitem Pointer to the menuitem that corresponds with
 *                 the window which properties changed
 * @param icon_name The name of icon for this menu entry
 * @param app_name The name of application for this menu entry
 * @param view_name The title for this window/view
 * @param position_change 0 if no change, 1 if item shall migrate to
 *                        first position, 2 if item shall migrate to
 *                        the last position (i.e. iconized)
 *
 * @param killable TRUE if the application can be killed
 * @param dialog_name Contains the title of the dialog window
 * @param data The callback data
 *
 */

typedef void (wm_updated_window_cb)(GtkWidget *menuitem,
                                    const gchar *icon_name,
                                    const gchar *app_name,
                                    const gchar *view_name,
                                    const guint position_change,
                                    const gboolean killable,
                                    const gchar *dialog_name,
                                    gpointer data);


/** Function callback to be called when the desktop has been topped.
 * @ param The callback data
 */

typedef void (wm_desktop_topped_cb)(gpointer data);






/* public functions of the Task Navigator 'window manager' */

/**
 * @param new_window_cb Pointer to a function that the window manager
 *        calls when a new window is created
 * @param removed_window_cb Pointer to a function that the window manager
 *        calls when a window is removed
 * @param updated_win_cb Pointer to a function that the window manager
 *        calls when properties of a window are updated
 * @param desktop_topped_cb Pointer to a function that is called when
 *        the desktop (i.e. Home) is topped.
 * @param cb_data Data to be passed to the callbacks
 * @return TRUE if initialization succeeded, FALSE otherwise.
 */

gboolean init_window_manager(wm_new_window_cb *new_win_cb,
                             wm_removed_window_cb *removed_win_cb,
                             wm_updated_window_cb *updated_win_cb,
                             wm_desktop_topped_cb *desktop_topped_cb,
                             gpointer cb_data);


/**  Send 'top' request for a certain existing window/view
 *
 * @param menuitem The menuitem for the view/window that is to be topped.
 *
 */

void top_view(GtkMenuItem *menuitem);


/**  Send 'top' request for a certain service
 *   @param service_name The name of the service that is to be topped
 */

void top_service(const gchar *service_name);

/**
 * Requests the real window manager to top the desktop
 */

void top_desktop(void);


#endif /* HILDON_NAVIGATOR_WM_H */
