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

#ifndef HAVE_HN_WM_WATCHABLE_APP_H
#define HAVE_HN_WM_WATCHABLE_APP_H

#include "hn-wm.h"

/** 
 * Create a new watchable app instance from a MBDotDesktop
 * 
 * @param desktop MBDotDesktop .desktop file representation 
 * 
 * @return pointer to HNWMWatchableApp instance, NULL if missing fields
 */
HNWMWatchableApp*
hn_wm_watchable_app_new (const char * file);

/** 
 * Create a new watchable app instance for applications without
 * .desktop file
 * 
 * @return pointer to HNWMWatchableApp instance, NULL if missing fields
 */
HNWMWatchableApp*
hn_wm_watchable_app_new_dummy (void);

/**
 * Updates fields of the application represented by app to those represented
 * by update.
 *
 * NB: this function is intended for use by the hn_wm_dnotify_cb() handler
 *     and should be used only with great caution anywhere else.
 *
 * @return TRUE if app was modified, FALSE otherwise
 */
gboolean
hn_wm_watchable_app_update (HNWMWatchableApp *app, HNWMWatchableApp *update);

/** 
 * Get the X-Osso-Service field set via .desktop file of an 
 * HNWMWatchableApp instance. 
 * You should not free the result.
 * 
 * @param app HNWMWatchableApp instance
 * 
 * @return exec field value 
 */
const gchar*
hn_wm_watchable_app_get_service (HNWMWatchableApp *app);
const gchar*
hn_wm_watchable_app_get_exec (HNWMWatchableApp *app);

/** 
 * Get the name field set via .desktop file of an HNWMWatchableApp instance. 
 * You should not free the result.
 * 
 * @param app HNWMWatchableApp instance
 * 
 * @return name field value
 */
const gchar*
hn_wm_watchable_app_get_name (HNWMWatchableApp *app);

/** 
 * Get the icon field set via .desktop file of an HNWMWatchableApp instance. 
 * You should not free the result.
 * 
 * @param app HNWMWatchableApp instance
 * 
 * @return icon field value
 */
const gchar*
hn_wm_watchable_app_get_icon_name (HNWMWatchableApp *app);

/** 
 * Get the class field set via .desktop file of an HNWMWatchableApp instance. 
 * You should not free the result.
 * 
 * @param app HNWMWatchableApp instance 
 * 
 * @return class field value
 */
const gchar*
hn_wm_watchable_app_get_class_name (HNWMWatchableApp *app);

/** 
 * Check if app needs startup banner. 
 * 
 * @param app HNWMWatchableApp instance
 * 
 * @return TRUE if app requires banner when starting, FALSE if not
 */
gboolean
hn_wm_watchable_app_has_startup_notify (HNWMWatchableApp *app);

/** 
 * Check if app window(s) are minimised.
 * FIXME: may break with windows as views
 *
 * @param app HNWMWatchableApp instance
 * 
 * @return TRUE if minimised, FALSE if not. 
 */
gboolean
hn_wm_watchable_app_is_minimised (HNWMWatchableApp *app);

/** 
 * Sets minimised state of app.
 * 
 * @param app HNWMWatchableApp instance
 * @param minimised TRUE if app minimised, FALSE if not.
 */
void
hn_wm_watchable_app_set_minimised (HNWMWatchableApp *app,
				   gboolean          minimised);

/** 
 * Check if windows exist referencing this app instance
 * 
 * @param app HNWMWatchableApp instance
 * 
 * @return TRUE if windows exist for this app instance, FALSE if not
 */
gboolean
hn_wm_watchable_app_has_windows (HNWMWatchableApp *app);

/** 
 * Check if hibernating windows exist referencing this app instance
 * 
 * @param app HNWMWatchableApp instance
 * 
 * @return TRUE if windows exist for this app instance, FALSE if not
 */
gboolean
hn_wm_watchable_app_has_hibernating_windows (HNWMWatchableApp *app);

/** 
 * Check if windows (live or hibernating) exist referencing this app instance
 * 
 * @param app HNWMWatchableApp instance
 * 
 * @return TRUE if windows exist for this app instance, FALSE if not
 */
gboolean
hn_wm_watchable_app_has_any_windows (HNWMWatchableApp *app);

/** 
 * Check if application is in hibernation state - i.e not actually
 * running but item exists for it in application switcher. 
 *
 * @param app HNWMWatchableApp instance
 * 
 * @return TRUE is application is hibernating, False if not.
 */
gboolean
hn_wm_watchable_app_is_hibernating (HNWMWatchableApp *app);

/** 
 * Sets the hibernation flag of the app. Does not actually
 * hibernate however.
 *
 * FIXME: this is confusing and should somehow be removed. 
 *
 * @param app HNWMWatchableApp instance 
 * @param hibernate TRUE if hibernating, FALSE if not
 */
void
hn_wm_watchable_app_set_hibernate (HNWMWatchableApp *app,
				   gboolean          hibernate);

/** 
 * Checks if the app is able to hibernate
 * 
 * @param app HNWMWatchableApp instance
 * 
 * @return TRUE if able to hibernate, FALSE if not.
 */
gboolean
hn_wm_watchable_app_is_able_to_hibernate (HNWMWatchableApp *app);

/** 
 * Sets if the application can hibernate.
 * 
 * FIXME: should probably be static
 * @param app HNWMWatchableApp instance
 * @param hibernate TRUE if hibernation possible, FALSE otherwise.
 */
void
hn_wm_watchable_app_set_able_to_hibernate (HNWMWatchableApp *app,
					   gboolean          hibernate);

/** 
 * Destroys an application instance and frees all associated memory.
 * 
 * @param app HNWMWatchableApp instance
 */
void
hn_wm_watchable_app_destroy (HNWMWatchableApp *app);

/** 
 * Shows application died dialog
 *
 * @param app HNWMWatchableApp instance
 */
void
hn_wm_watchable_app_died_dialog_show (HNWMWatchableApp *app);

/** 
 * FIXME: CAn be static ?
 * Shows launch banner for app 
 * 
 * @param parent 
 * @param app HNWMWatchableApp instance
 */
void 
hn_wm_watchable_app_launch_banner_show (HNWMWatchableApp *app);

/** 
 * FIXME: Can be static ?
 * 
 * @param parent 
 */
void 
hn_wm_watchable_app_launch_banner_close (GtkWidget            *parent,
					 HNWMLaunchBannerInfo *info);

/** 
 * FIXME: this can be static
 * 
 * @param data 
 * 
 * @return 
 */
gboolean 
hn_wm_watchable_app_launch_banner_timeout (gpointer data);

void
hn_wm_watchable_app_set_ping_timeout_note (HNWMWatchableApp *app, GtkWidget *note);

GtkWidget*
hn_wm_watchable_app_get_ping_timeout_note (HNWMWatchableApp *app);

HNWMWatchedWindow *
hn_wm_watchable_app_get_active_window (HNWMWatchableApp *app);

void
hn_wm_watchable_app_set_active_window (HNWMWatchableApp *app,
                                       HNWMWatchedWindow * win);

gboolean
hn_wm_watchable_app_is_dummy(HNWMWatchableApp *app);

gboolean
hn_wm_watchable_app_is_launching (HNWMWatchableApp *app);

void
hn_wm_watchable_app_set_launching (HNWMWatchableApp *app,
                                   gboolean launching);

HNEntryInfo *
hn_wm_watchable_app_get_info (HNWMWatchableApp *app);

#if 0
gint
hn_wm_watchable_app_n_windows (HNWMWatchableApp *app);
#endif

gboolean
hn_wm_watchable_app_is_active (HNWMWatchableApp *app);

const gchar *
hn_wm_watchable_app_get_extra_icon (HNWMWatchableApp *app);

void
hn_wm_watchable_app_set_extra_icon (HNWMWatchableApp *app, const gchar *icon);

#endif
