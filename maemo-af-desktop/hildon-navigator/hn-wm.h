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
#include <sys/time.h>
#include <gtk/gtk.h>

#include "hn-wm-types.h"

#define HN_WANT_DEBUG 1 /* Set to 1 for more verbose hn */

#if (HN_WANT_DEBUG)
#define HN_DBG(x, a...) \
 fprintf(stderr, __FILE__ ":%d,%s() " x "\n", __LINE__, __func__, ##a)
#else
#define HN_DBG(x, a...) do {} while (0)
#endif
#define HN_MARK() HN_DBG("--mark--");

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

/* DBus/Banner etc related defines
 */
#define APP_LAUNCH_BANNER_METHOD_INTERFACE   "com.nokia.tasknav.app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD_PATH        "/com/nokia/tasknav/app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD             "app_launch_banner"
#define APP_LAUNCH_BANNER_MSG_LOADING        "ckct_ib_application_loading"
#define APP_LAUNCH_BANNER_MSG_RESUMING       "ckct_ib_application_resuming"
#define APP_LAUNCH_BANNER_MSG_LOADING_FAILED "ckct_ib_application_loading_failed"
/* Timeout of the launch banner, in secons */
#define APP_LAUNCH_BANNER_TIMEOUT            20
/* Timeout of the launch banner in lowmem situation */
#define APP_LAUNCH_BANNER_TIMEOUT_LOWMEM     40
/* Interval for checking for new window or timeout, in seconds */
#define APP_LAUNCH_BANNER_CHECK_INTERVAL     0.5

/* .desktop file related defines, mainly for keys
 */
#define DESKTOP_VISIBLE_FIELD         "Name"
#define DESKTOP_LAUNCH_FIELD          "X-Osso-Service"
#define DESKTOP_EXEC_FIELD            "Exec"
#define DESKTOP_ICON_FIELD            "Icon"
#define DESKTOP_ICON_PATH_FIELD       "X-Icon-path"
#define DESKTOP_SUP_WMCLASS           "StartupWMClass"
#define DESKTOP_STARTUPNOTIFY         "StartupNotify"
#define DESKTOP_SUFFIX                ".desktop"

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

#define SAVE_METHOD      "save"
#define KILL_APPS_METHOD "kill_app"

#define UNKNOWN_TITLE         "Unknown"

#undef  TN_ALWAYS_FOCUSABLE

#ifdef TN_ALWAYS_FOCUSABLE
#define TN_DEFAULT_FOCUS TRUE
#else
#define TN_DEFAULT_FOCUS FALSE
#endif

typedef struct
{
  gpointer entry_ptr;
  gulong view_id;
  gchar *service;
  gchar *wm_class;
  gulong window_id;
} menuitem_comp_t;

struct HNWMLaunchBannerInfo
{
  GtkWidget         *parent;
  GtkWidget         *banner;
  struct timeval     launch_time;
  gchar             *msg;
  HNWMWatchableApp  *app;
};

/**  Send 'top' request for a certain existing window/view
 *
 * @param info the window/view to be topped
 *
 */
void hn_wm_top_item (HNEntryInfo *info);


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
hn_wm_init (HNAppSwitcher *as);

/*
 *  Because of the way in which the hildon_dnotify lib works and the way in
 *  which other_menu is designed, this callback has to be installed from
 *  the others_menu
 */
void
hn_wm_dnotify_cb ( char *path, void * data);

/* keyboard handling functions */

void
hn_wm_activate(guint32 what);

void
hn_wm_focus_active_window (void);

gboolean
hn_wm_fullscreen_mode (void);

/*
 * These are simple getters/setters that replace direct use of global
 * hnwm->something. In order to ensure that we do not incure performance
 * penalty due to the function call, we declare all of these as
 * 'extern inline'; this ensures that these functions will always be inlined.
 */


extern inline Atom
hn_wm_get_atom(gint indx);

extern inline GHashTable *
hn_wm_get_watched_windows(void);

extern inline GHashTable *
hn_wm_get_hibernating_windows(void);

extern inline gboolean
hn_wm_is_lowmem_situation(void);

extern inline void
hn_wm_set_lowmem_situation(gboolean b);

extern inline gboolean
hn_wm_is_bg_kill_situation(void);

extern inline void
hn_wm_set_bg_kill_situation(gboolean b);

extern inline HNAppSwitcher *
hn_wm_get_app_switcher(void);

extern inline gint
hn_wm_get_timer_id(void);

extern inline void
hn_wm_set_timer_id(gint id);

extern inline void
hn_wm_set_about_to_shutdown(gboolean b);

extern inline gboolean
hn_wm_get_about_to_shutdown(void);

extern inline GList *
hn_wm_get_banner_stack(void);

extern inline void
hn_wm_set_banner_stack(GList * l);

extern inline gulong
hn_wm_get_lowmem_banner_timeout(void);

extern inline gulong
hn_wm_get_lowmem_timeout_multiplier(void);

extern inline HNWMWatchedWindow *
hn_wm_get_active_window(void);

/*
 * reset the active window to NULL
 *
 * NB: we intentionally do not provide a setter -- the active window is our
 * business alone and must not be done from anywhere else than the WM itself.
 * The reset function is only to be called from hn_wm_watched_window_destroy().
 */
extern inline void
hn_wm_reset_active_window(void);

#endif /* HILDON_NAVIGATOR_WM_H */

