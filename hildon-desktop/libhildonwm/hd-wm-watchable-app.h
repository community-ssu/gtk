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
* @file hn-wm-watchable-app.h 
*/

#ifndef __HD_WM_WATCHABLE_APP_H__
#define __HD_WM_WATCHABLE_APP_H__

#include "hd-wm.h"

/** 
 * Create a new watchable app instance from a MBDotDesktop
 * 
 * @param desktop MBDotDesktop .desktop file representation 
 * 
 * @return pointer to HDWMWatchableApp instance, NULL if missing fields
 */
HDWMWatchableApp*
hd_wm_watchable_app_new (const char * file);

/** 
 * Create a new watchable app instance for applications without
 * .desktop file
 * 
 * @return pointer to HDWMWatchableApp instance, NULL if missing fields
 */
HDWMWatchableApp*
hd_wm_watchable_app_new_dummy (void);

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
hd_wm_watchable_app_update (HDWMWatchableApp *app, HDWMWatchableApp *update);

/** 
 * Get the X-Osso-Service field set via .desktop file of an 
 * HDWMWatchableApp instance. 
 * You should not free the result.
 * 
 * @param app HDWMWatchableApp instance
 * 
 * @return exec field value 
 */
const gchar*
hd_wm_watchable_app_get_service (HDWMWatchableApp *app);
const gchar*
hd_wm_watchable_app_get_exec (HDWMWatchableApp *app);

/** 
 * Set the name field set manually of an HDWMWatchableApp instance.
 * If HDWMWatchableApp is not dummy it does nothing.
 * 
 * @param app HDWMWatchableApp instance
 * 
 * @return name field value
 */
void 
hd_wm_watchable_app_dummy_set_name (HDWMWatchableApp *app, const gchar *name);

/** 
 * Get the name field set via .desktop file of an HDWMWatchableApp instance. 
 * You should not free the result.
 * 
 * @param app HDWMWatchableApp instance
 * 
 * @return name field value
 */
const gchar*
hd_wm_watchable_app_get_name (HDWMWatchableApp *app);

/** 
 * Get the localized name field set via .desktop file of
 * an HDWMWatchableApp instance. If X-Text-Domain is not set,
 * it uses the maemo-af-desktop text domain.
 * You should not free the result.
 * 
 * @param app HDWMWatchableApp instance
 * 
 * @return name field value
 */
const gchar*
hd_wm_watchable_app_get_localized_name (HDWMWatchableApp *app);

/** 
 * Get the icon field set via .desktop file of an HDWMWatchableApp instance. 
 * You should not free the result.
 * 
 * @param app HDWMWatchableApp instance
 * 
 * @return icon field value
 */
const gchar*
hd_wm_watchable_app_get_icon_name (HDWMWatchableApp *app);

/** 
 * Get the class field set via .desktop file of an HDWMWatchableApp instance. 
 * You should not free the result.
 * 
 * @param app HDWMWatchableApp instance 
 * 
 * @return class field value
 */
const gchar*
hd_wm_watchable_app_get_class_name (HDWMWatchableApp *app);

/** 
 * Check if app needs startup banner. 
 * 
 * @param app HDWMWatchableApp instance
 * 
 * @return TRUE if app requires banner when starting, FALSE if not
 */
gboolean
hd_wm_watchable_app_has_startup_notify (HDWMWatchableApp *app);

/** 
 * Check if app window(s) are minimised.
 * FIXME: may break with windows as views
 *
 * @param app HDWMWatchableApp instance
 * 
 * @return TRUE if minimised, FALSE if not. 
 */
gboolean
hd_wm_watchable_app_is_minimised (HDWMWatchableApp *app);

/** 
 * Sets minimised state of app.
 * 
 * @param app HDWMWatchableApp instance
 * @param minimised TRUE if app minimised, FALSE if not.
 */
void
hd_wm_watchable_app_set_minimised (HDWMWatchableApp *app,
				   gboolean          minimised);

/** 
 * Check if windows exist referencing this app instance
 * 
 * @param app HDWMWatchableApp instance
 * 
 * @return TRUE if windows exist for this app instance, FALSE if not
 */
gboolean
hd_wm_watchable_app_has_windows (HDWMWatchableApp *app);

/** 
 * Check if hibernating windows exist referencing this app instance
 * 
 * @param app HDWMWatchableApp instance
 * 
 * @return TRUE if windows exist for this app instance, FALSE if not
 */
gboolean
hd_wm_watchable_app_has_hibernating_windows (HDWMWatchableApp *app);

/** 
 * Check if windows (live or hibernating) exist referencing this app instance
 * 
 * @param app HDWMWatchableApp instance
 * 
 * @return TRUE if windows exist for this app instance, FALSE if not
 */
gboolean
hd_wm_watchable_app_has_any_windows (HDWMWatchableApp *app);

/** 
 * Check if application is in hibernation state - i.e not actually
 * running but item exists for it in application switcher. 
 *
 * @param app HDWMWatchableApp instance
 * 
 * @return TRUE is application is hibernating, False if not.
 */
gboolean
hd_wm_watchable_app_is_hibernating (HDWMWatchableApp *app);

/** 
 * Sets the hibernation flag of the app. Does not actually
 * hibernate however.
 *
 * FIXME: this is confusing and should somehow be removed. 
 *
 * @param app HDWMWatchableApp instance 
 * @param hibernate TRUE if hibernating, FALSE if not
 */
void
hd_wm_watchable_app_set_hibernate (HDWMWatchableApp *app,
				   gboolean          hibernate);

/** 
 * Checks if the app is able to hibernate
 * 
 * @param app HDWMWatchableApp instance
 * 
 * @return TRUE if able to hibernate, FALSE if not.
 */
gboolean
hd_wm_watchable_app_is_able_to_hibernate (HDWMWatchableApp *app);

/** 
 * Sets if the application can hibernate.
 * 
 * FIXME: should probably be static
 * @param app HDWMWatchableApp instance
 * @param hibernate TRUE if hibernation possible, FALSE otherwise.
 */
void
hd_wm_watchable_app_set_able_to_hibernate (HDWMWatchableApp *app,
					   gboolean          hibernate);

/** 
 * Destroys an application instance and frees all associated memory.
 * 
 * @param app HDWMWatchableApp instance
 */
void
hd_wm_watchable_app_destroy (HDWMWatchableApp *app);

/** 
 * Shows application died dialog
 *
 * @param app HDWMWatchableApp instance
 */
void
hd_wm_watchable_app_died_dialog_show (HDWMWatchableApp *app);

/** 
 * FIXME: CAn be static ?
 * Shows launch banner for app 
 * 
 * @param parent 
 * @param app HDWMWatchableApp instance
 */
void 
hd_wm_watchable_app_launch_banner_show (HDWMWatchableApp *app);

/** 
 * FIXME: Can be static ?
 * 
 * @param parent 
 */
void 
hd_wm_watchable_app_launch_banner_close (GtkWidget            *parent,
					 HDWMLaunchBannerInfo *info);

/** 
 * FIXME: this can be static
 * 
 * @param data 
 * 
 * @return 
 */
gboolean 
hd_wm_watchable_app_launch_banner_timeout (gpointer data);

void
hd_wm_watchable_app_set_ping_timeout_note (HDWMWatchableApp *app, GtkWidget *note);

GtkWidget*
hd_wm_watchable_app_get_ping_timeout_note (HDWMWatchableApp *app);

HDWMWatchedWindow *
hd_wm_watchable_app_get_active_window (HDWMWatchableApp *app);

void
hd_wm_watchable_app_set_active_window (HDWMWatchableApp *app,
                                       HDWMWatchedWindow * win);

gboolean
hd_wm_watchable_app_is_dummy(HDWMWatchableApp *app);

gboolean
hd_wm_watchable_app_is_launching (HDWMWatchableApp *app);

void
hd_wm_watchable_app_set_launching (HDWMWatchableApp *app,
                                   gboolean launching);

HDEntryInfo *
hd_wm_watchable_app_get_info (HDWMWatchableApp *app);

#if 0
gint
hd_wm_watchable_app_n_windows (HDWMWatchableApp *app);
#endif

gboolean
hd_wm_watchable_app_is_active (HDWMWatchableApp *app);

const gchar *
hd_wm_watchable_app_get_extra_icon (HDWMWatchableApp *app);

void
hd_wm_watchable_app_set_extra_icon (HDWMWatchableApp *app, const gchar *icon);

#endif
