/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/* 
 * This file is part of libhildonwm
 *
 * Copyright (C) 2005, 2006, 2007 Nokia Corporation.
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
* @file hn-wm-watchable-app.h 
*/

#ifndef __HD_WM_WATCHABLE_APPLICATION_H__
#define __HD_WM_WATCHABLE_APPLICATION_H__

#include <libhildonwm/hd-wm.h>
#include <libhildonwm/hd-wm-types.h>

#define HD_WM_TYPE_APPLICATION            (hd_wm_application_get_type ())
#define HD_WM_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_WM_TYPE_APPLICATION, HDWMApplication))
#define HD_WM_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_WM_TYPE_APPLICATION, HDWMApplicationClass))
#define HD_WM_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_WM_TYPE_APPLICATION))
#define HD_IS_WM_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_WM_TYPE_APPLICATION))
#define HD_WM_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_WM_TYPE_APPLICATION, HDWMApplicationClass))

typedef struct _HDWMApplicationClass HDWMApplicationClass;
typedef struct _HDWMApplicationPrivate HDWMApplicationPrivate;

struct _HDWMApplication
{
  GObject parent;

  HDWMApplicationPrivate *priv;
};

struct _HDWMApplicationClass
{
  GObjectClass parent_class;
};

GType 
hd_wm_application_get_type (void);

/* Used to pass data to launch banner timeout callback */
typedef struct HDWMLaunchBannerInfo HDWMLaunchBannerInfo;

/** 
 * Create a new watchable app instance from a MBDotDesktop
 * 
 * @param desktop MBDotDesktop .desktop file representation 
 * 
 * @return pointer to HDWMApplication instance, NULL if missing fields
 */
HDWMApplication*
hd_wm_application_new (const gchar * file);

/** 
 * Create a new watchable app instance for applications without
 * .desktop file
 * 
 * @return pointer to HDWMApplication instance, NULL if missing fields
 */
HDWMApplication*
hd_wm_application_new_dummy (void);

/**
 * Updates fields of the application represented by app to those represented
 * by update.
 *
 * NB: this function is intended for use by the hd_wm_dnotify_cb() handler
 *     and should be used only with great caution anywhere else.
 *
 * @return TRUE if app was modified, FALSE otherwise
 */
gboolean
hd_wm_application_update (HDWMApplication *app, HDWMApplication *update);

/** 
 * Get the X-Osso-Service field set via .desktop file of an 
 * HDWMApplication instance. 
 * You should not free the result.
 * 
 * @param app HDWMApplication instance
 * 
 * @return exec field value 
 */
const gchar*
hd_wm_application_get_service (HDWMApplication *app);
const gchar*
hd_wm_application_get_exec (HDWMApplication *app);

/** 
 * Set the name field set manually of an HDWMApplication instance.
 * If HDWMApplication is not dummy it does nothing.
 * 
 * @param app HDWMApplication instance
 * 
 * @return name field value
 */
void 
hd_wm_application_dummy_set_name (HDWMApplication *app, const gchar *name);

/** 
 * Get the name field set via .desktop file of an HDWMApplication instance. 
 * You should not free the result.
 * 
 * @param app HDWMApplication instance
 * 
 * @return name field value
 */
const gchar*
hd_wm_application_get_name (HDWMApplication *app);

/** 
 * Get the localized name field set via .desktop file of
 * an HDWMApplication instance. If X-Text-Domain is not set,
 * it uses the maemo-af-desktop text domain.
 * You should not free the result.
 * 
 * @param app HDWMApplication instance
 * 
 * @return name field value
 */
const gchar*
hd_wm_application_get_localized_name (HDWMApplication *app);

/** 
 * Get the icon field set via .desktop file of an HDWMApplication instance. 
 * You should not free the result.
 * 
 * @param app HDWMApplication instance
 * 
 * @return icon field value
 */
const gchar*
hd_wm_application_get_icon_name (HDWMApplication *app);

/** 
 * Get the class field set via .desktop file of an HDWMApplication instance. 
 * You should not free the result.
 * 
 * @param app HDWMApplication instance 
 * 
 * @return class field value
 */
const gchar*
hd_wm_application_get_class_name (HDWMApplication *app);

/** 
 * Check if app needs startup banner. 
 * 
 * @param app HDWMApplication instance
 * 
 * @return TRUE if app requires banner when starting, FALSE if not
 */
gboolean
hd_wm_application_has_startup_notify (HDWMApplication *app);

/** 
 * Check if app window(s) are minimised.
 * FIXME: may break with windows as views
 *
 * @param app HDWMApplication instance
 * 
 * @return TRUE if minimised, FALSE if not. 
 */
gboolean
hd_wm_application_is_minimised (HDWMApplication *app);

/** 
 * Sets minimised state of app.
 * 
 * @param app HDWMApplication instance
 * @param minimised TRUE if app minimised, FALSE if not.
 */
void
hd_wm_application_set_minimised (HDWMApplication *app,
				   gboolean          minimised);

/** 
 * Check if windows exist referencing this app instance
 * 
 * @param app HDWMApplication instance
 * 
 * @return TRUE if windows exist for this app instance, FALSE if not
 */
gboolean
hd_wm_application_has_windows (HDWMApplication *app);

/** 
 * Check if hibernating windows exist referencing this app instance
 * 
 * @param app HDWMApplication instance
 * 
 * @return TRUE if windows exist for this app instance, FALSE if not
 */
gboolean
hd_wm_application_has_hibernating_windows (HDWMApplication *app);

/** 
 * Check if windows (live or hibernating) exist referencing this app instance
 * 
 * @param app HDWMApplication instance
 * 
 * @return TRUE if windows exist for this app instance, FALSE if not
 */
gboolean
hd_wm_application_has_any_windows (HDWMApplication *app);

/** 
 * Check if application is in hibernation state - i.e not actually
 * running but item exists for it in application switcher. 
 *
 * @param app HDWMApplication instance
 * 
 * @return TRUE is application is hibernating, False if not.
 */
gboolean
hd_wm_application_is_hibernating (HDWMApplication *app);

/** 
 * Sets the hibernation flag of the app. Does not actually
 * hibernate however.
 *
 * FIXME: this is confusing and should somehow be removed. 
 *
 * @param app HDWMApplication instance 
 * @param hibernate TRUE if hibernating, FALSE if not
 */
void
hd_wm_application_set_hibernate (HDWMApplication *app,
				   gboolean          hibernate);

/** 
 * Checks if the app is able to hibernate
 * 
 * @param app HDWMApplication instance
 * 
 * @return TRUE if able to hibernate, FALSE if not.
 */
gboolean
hd_wm_application_is_able_to_hibernate (HDWMApplication *app);

/** 
 * Sets if the application can hibernate.
 * 
 * FIXME: should probably be static
 * @param app HDWMApplication instance
 * @param hibernate TRUE if hibernation possible, FALSE otherwise.
 */
void
hd_wm_application_set_able_to_hibernate (HDWMApplication *app,
					   gboolean          hibernate);

/** 
 * FIXME: CAn be static ?
 * Shows launch banner for app 
 * 
 * @param parent 
 * @param app HDWMApplication instance
 */
void 
hd_wm_application_launch_banner_show (HDWMApplication *app);

void
hd_wm_application_set_ping_timeout_note (HDWMApplication *app, GObject *note);

GObject *
hd_wm_application_get_ping_timeout_note (HDWMApplication *app);

HDWMWindow *
hd_wm_application_get_active_window (HDWMApplication *app);

void
hd_wm_application_set_active_window (HDWMApplication *app,
                                       HDWMWindow * win);

gboolean
hd_wm_application_is_dummy(HDWMApplication *app);

gboolean
hd_wm_application_is_launching (HDWMApplication *app);

void
hd_wm_application_set_launching (HDWMApplication *app,
                                   gboolean launching);

GHashTable *
hd_wm_application_get_icon_cache (HDWMApplication *app);

HDEntryInfo *
hd_wm_application_get_info (HDWMApplication *app);

#if 0
gint
hd_wm_application_n_windows (HDWMApplication *app);
#endif

gboolean
hd_wm_application_is_active (HDWMApplication *app);

const gchar *
hd_wm_application_get_extra_icon (HDWMApplication *app);

void
hd_wm_application_set_extra_icon (HDWMApplication *app, const gchar *icon);

#endif
