/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/* 
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
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


/**
 * @file hn-wm.h
 */

#ifndef __HD_WM_H__
#define __HD_WM_H__

#include <X11/Xlib.h>
#include <sys/time.h>
#include <gtk/gtk.h>

#include <libhildonwm/hd-wm-types.h>
#include <libhildonwm/hd-entry-info.h>
#include <libhildonwm/hd-wm-watchable-app.h>
#include <libhildonwm/hd-keys.h>

#define HN_WANT_DEBUG 0 /* Set to 1 for more verbose hn */

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
#define DESKTOP_TEXT_DOMAIN_FIELD     "X-Text-Domain"
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

#define HD_TYPE_WM            (hd_wm_get_type ())
#define HD_WM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_WM, HDWM))
#define HD_WM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_WM, HDWMClass))
#define HD_IS_WM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_WM))
#define HD_IS_WM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_WM))
#define HD_WM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_WM, HDWMClass))

typedef struct _HDWM HDWM;
typedef struct _HDWMClass HDWMClass;
typedef struct _HDWMPrivate HDWMPrivate;

struct _HDWM
{
  GObject parent;

  HDKeysConfig *keys;

  HDWMPrivate *priv;
};

struct _HDWMClass
{
  GObjectClass parent_class;

  void (*entry_info_added) 	   (HDWM *hdwm,HDEntryInfo *info);
  void (*entry_info_removed) 	   (HDWM *hdwm,HDEntryInfo *info);
  void (*entry_info_changed) 	   (HDWM *hdwm,HDEntryInfo *info);
  void (*entry_info_stack_changed) (HDWM *hdwm,HDEntryInfo *info);
  void (*work_area_changed)        (HDWM *hdwm,GdkRectangle *work_area);
  void (*show_menu)		   (HDWM *hdwm);
  void (*application_starting)     (HDWM *hdwm, gchar *application);
  /* */
};

typedef struct
{
  gpointer entry_ptr;
  gulong view_id;
  gchar *service;
  gchar *wm_class;
  gulong window_id;
} menuitem_comp_t;

struct HDWMLaunchBannerInfo
{
  GtkWidget         *parent;
  GtkWidget         *banner;
  struct timeval     launch_time;
  gchar             *msg;
  HDWMWatchableApp  *app;
};

GType 
hd_wm_get_type (void);

HDWM *
hd_wm_get_singleton (void);

/**  Send 'top' request for a certain existing window/view
 *
 * @param info the window/view to be topped
 *
 */
void hd_wm_top_item (HDEntryInfo *info);


/**  Send 'top' request for a certain service
 *   @param service_name The name of the service that is to be topped
 */
gboolean hd_wm_top_service(const gchar *service_name);

/**
 * Requests the real window manager to top the desktop
 */
void hd_wm_top_desktop(void);

/**
 * Toggle between desktop and the last active application
 */
void hd_wm_toggle_desktop (void);

HDWMWatchedWindow*
hd_wm_lookup_watched_window_via_service (const gchar *service_name);

HDWMWatchedWindow*
hd_wm_lookup_watched_window_via_menu_widget (GtkWidget *menu_widget);

HDWMWatchableApp*
hd_wm_lookup_watchable_app_via_service (const gchar *service_name);

HDWMWatchableApp*
hd_wm_lookup_watchable_app_via_exec (const gchar *exec_name);

HDWMWatchableApp*
hd_wm_lookup_watchable_app_via_menu (GtkWidget *menu);

HDWMWatchedWindow*
hd_wm_lookup_watched_window_view (GtkWidget *menu_widget);

gchar *
hd_wm_compute_watched_window_hibernation_key (Window xwin,
					      HDWMWatchableApp *app);

void
hd_wm_dnotify_register (void);

/* keyboard handling functions */

void
hd_wm_activate_window (guint32 what, GdkWindow *window);

#define hd_wm_activate(what) hd_wm_activate_window (what,NULL)

void
hd_wm_focus_active_window (HDWM *hdwm);

void 
hd_wm_set_others_menu_button (HDWM *hdwm, GtkWidget *widget);

gboolean
hd_wm_fullscreen_mode (void);

void
hd_wm_get_work_area (HDWM *hdwm, GdkRectangle *work_area);

GList * 
hd_wm_get_applications (HDWM *hdwm);

void 
hd_wm_close_application (HDWM *hdwm, HDEntryInfo *entry_info);

void 
hd_wm_add_applications (HDWM *hdwm, HDEntryInfo *entry_info);

gboolean  
hd_wm_remove_applications (HDWM *hdwm, HDEntryInfo *entry_info);

void 
hd_wm_update_client_list (HDWM *hdwm);

void
hd_wm_activate_service (const gchar *service_name,
                        const gchar *launch_parameters);

/*
 * These are simple getters/setters that replace direct use of global
 * hnwm->something. In order to ensure that we do not incure performance
 * penalty due to the function call, we declare all of these as
 * 'extern inline'; this ensures that these functions will always be inlined.
 */


extern inline Atom
hd_wm_get_atom(gint indx);

extern inline GHashTable *
hd_wm_get_watched_windows(void);

extern inline GHashTable *
hd_wm_get_hibernating_windows(void);

extern inline gboolean
hd_wm_is_lowmem_situation(void);

extern inline void
hd_wm_set_lowmem_situation(gboolean b);

extern inline gboolean
hd_wm_is_bg_kill_situation(void);

extern inline void
hd_wm_set_bg_kill_situation(gboolean b);

extern inline gint
hd_wm_get_timer_id(void);

extern inline void
hd_wm_set_timer_id(gint id);

extern inline void
hd_wm_set_about_to_shutdown(gboolean b);

extern inline gboolean
hd_wm_get_about_to_shutdown(void);

extern inline GList *
hd_wm_get_banner_stack(void);

extern inline void
hd_wm_set_banner_stack(GList * l);

extern inline gulong
hd_wm_get_lowmem_banner_timeout(void);

extern inline gulong
hd_wm_get_lowmem_timeout_multiplier(void);

extern inline HDWMWatchedWindow *
hd_wm_get_active_window(void);

extern inline HDWMWatchedWindow *
hd_wm_get_last_active_window(void);

extern inline gboolean
hd_wm_modal_windows_present(void);

/*
 * reset the active window to NULL
 *
 * NB: we intentionally do not provide a setter -- the active window is our
 * business alone and must not be done from anywhere else than the WM itself.
 * The reset function is only to be called from hd_wm_watched_window_destroy().
 */
extern inline void
hd_wm_reset_active_window(void);

extern inline void
hd_wm_reset_last_active_window(void);

#endif/*__HD_WM_H__*/

