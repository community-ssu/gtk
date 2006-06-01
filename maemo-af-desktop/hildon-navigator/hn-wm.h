/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

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

/**
 * @file hn-wm.h
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
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>

#include <libosso.h>
#include <log-functions.h>

#include <libmb/mbdotdesktop.h>
#include <libmb/mbutil.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pwd.h>

#include "hn-wm-types.h"
#include "application-switcher.h"
#include "osso-manager.h"
#include "others-menu.h"
#include "hildon-navigator-interface.h"

#define HN_WANT_DEBUG 0 /* Set to 1 for more verbose hn */

#if (HN_WANT_DEBUG)
#define HN_DBG(x, a...) \
 fprintf(stderr, __FILE__ ":%d,%s() " x "\n", __LINE__, __func__, ##a)
#else
#define HN_DBG(x, a...) do {} while (0)
#endif
#define HN_MARK() HN_DBG("--mark--");

/* .desktop file related defines, mainly for keys
 */
#define DESKTOP_EXEC_FIELD    "Exec"
#define DESKTOP_LAUNCH_FIELD  "X-Osso-Service"
#define DESKTOP_VISIBLE_FIELD "Name"
#define DESKTOP_ICON_FIELD    "Icon"
#define DESKTOP_SUP_WMCLASS   "StartupWMClass"
#define DESKTOP_STARTUPNOTIFY "StartupNotify"
#define DESKTOP_SUFFIX        ".desktop"
#define UNKNOWN_TITLE         "Unknown"

/* DBus/Banner etc related defines
 */
#define APP_LAUNCH_BANNER_METHOD_INTERFACE   "com.nokia.tasknav.app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD_PATH        "/com/nokia/tasknav/app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD             "app_launch_banner"
#define APP_LAUNCH_BANNER_MSG_LOADING        "ckct_ib_application_loading"
#define APP_LAUNCH_BANNER_MSG_LOADING_FAILED "ckct_ib_application_loading_failed"
/* Timeout of the launch banner, in secons */
#define APP_LAUNCH_BANNER_TIMEOUT            20
/* Timeout of the launch banner in lowmem situation */
#define APP_LAUNCH_BANNER_TIMEOUT_LOWMEM     40
/* Interval for checking for new window or timeout, in seconds */
#define APP_LAUNCH_BANNER_CHECK_INTERVAL     0.5

/* Maemo Launcher DBus interface
 */
#define MAEMO_LAUNCHER_SIGNAL_IFACE "org.maemo.launcher"
#define MAEMO_LAUNCHER_SIGNAL_PATH "/org/maemo/launcher"
#define APP_DIED_SIGNAL_NAME "ApplicationDied"

/* Application killer DBus interface
 */
#define APPKILLER_SIGNAL_INTERFACE "com.nokia.osso_app_killer"
#define APPKILLER_SIGNAL_PATH      "/com/nokia/osso_app_killer"
#define APPKILLER_SIGNAL_NAME      "exit"

/* Low memory settings
 */
#define LAUNCH_SUCCESS_TIMEOUT 20

/* For gathering available memory information */
#define LOWMEM_PROC_ALLOWED              "/proc/sys/vm/lowmem_allowed_pages"
#define LOWMEM_PROC_USED                 "/proc/sys/vm/lowmem_used_pages"

#define LOWMEM_LAUNCH_BANNER_TIMEOUT     0
#define LOWMEM_LAUNCH_BANNER_TIMEOUT_MAX 60000
#define LOWMEM_LAUNCH_BANNER_TIMEOUT_ENV "NAVIGATOR_LOWMEM_LAUNCH_BANNER_TIMEOUT"
/* Added by Karoliina 26092005 */
#define LOWMEM_TIMEOUT_MULTIPLIER_ENV    "NAVIGATOR_LOWMEM_TIMEOUT_MULTIPLIER"
#define LOWMEM_TIMEOUT_MULTIPLIER        2
/* */
#define LOWMEM_LAUNCH_THRESHOLD_DISTANCE 2500
#define LOWMEM_LAUNCH_THRESHOLD_DISTANCE_ENV "NAVIGATOR_LOWMEM_LAUNCH_THRESHOLD_DISTANCE"

#define KILL_DELAY 2000        /* 2 seconds */
#define KILL_DELAY_LOWMEM 4000 /* 4 seconds */

struct HNWMLaunchBannerInfo
{
  GtkWidget         *parent;
  GtkWidget         *banner;
  struct timeval     launch_time;
  gchar             *msg;
  HNWMWatchableApp  *app;
};

struct HNWM   /* Our main struct, used globally unfortunatly.. */
{

  Atom          atoms[HN_ATOM_COUNT];

  /* WatchedWindows is a hash of watched windows hashed via in X window ID.
   * As most lookups happen via window ID's makes sense to hash on this,
   */
  GHashTable   *watched_windows;

  /* watched windows that are 'hibernating' - i.e there actually not
   * running any more but still appear in HN as if they are ( for memory ).
   * Split into seperate hash for efficiency reasons.
   */
  GHashTable   *watched_windows_hibernating;

  /* A hash of valid watchable apps ( hashed on class name ). This is built
   * on startup by slurping in /usr/share/applications/hildon .desktop's
   * NOTE: previous code used service/exec/class to refer to class name so
   *       quite confusing.
   */
  GHashTable   *watched_apps;

  ApplicationSwitcher_t *app_switcher;

  /* FIXME: Below memory management related not 100% sure what they do */

  gulong        lowmem_banner_timeout;
  gulong        lowmem_min_distance;
  gulong        lowmem_timeout_multiplier;
  gboolean      lowmem_situation;

  gboolean      bg_kill_situation;
  gint          timer_id;
  gboolean      about_to_shutdown;
};

/* Return the singleton HNWM instance */
HNWM *hn_wm_get_singleton(void);

/**  Send 'top' request for a certain existing window/view
 *
 * @param menuitem The menuitem for the view/window that is to be topped.
 *
 */
void hn_wm_top_view(GtkMenuItem *menuitem);


/**  Send 'top' request for a certain service
 *   @param service_name The name of the service that is to be topped
 */
gboolean hn_wm_top_service(const gchar *service_name);

/**
 * Requests the real window manager to top the desktop
 */
void hn_wm_top_desktop(void);


HNWMWatchedWindow*
hn_wm_lookup_watched_window_via_service (const gchar *service_name);

HNWMWatchedWindow*
hn_wm_lookup_watched_window_via_menu_widget (GtkWidget *menu_widget);

HNWMWatchableApp*
hn_wm_lookup_watchable_app_via_service (const gchar *service_name);

HNWMWatchableApp*
hn_wm_lookup_watchable_app_via_exec (const gchar *exec_name);

HNWMWatchableApp*
hn_wm_lookup_watchable_app_via_menu (GtkWidget *menu);

HNWMWatchedWindow*
hn_wm_lookup_watched_window_view (GtkWidget *menu_widget);

gchar *
hn_wm_compute_watched_window_hibernation_key (Window xwin,
					      HNWMWatchableApp *app);

gboolean
hn_wm_init (ApplicationSwitcher_t *as);

/**** Likely deprecated stuff *****/

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
} menuitem_comp_t;


#include "hn-wm-util.h"
#include "hn-wm-memory.h"
#include "hn-wm-watchable-app.h"
#include "hn-wm-watched-window.h"
#include "hn-wm-watched-window-view.h"

#endif /* HILDON_NAVIGATOR_WM_H */

