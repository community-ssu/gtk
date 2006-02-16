/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef SETTINGS_H
#define SETTINGS_H

void load_settings ();
void save_settings ();

// User serviceable settings
//
extern bool   clean_after_install;
extern int    update_interval_index;
extern int    package_sort_key;
extern int    package_sort_sign;

#define UPDATE_INTERVAL_SESSION 0
#define UPDATE_INTERVAL_WEEK    1
#define UPDATE_INTERVAL_MONTH   2
#define UPDATE_INTERVAL_NEVER   3

#define SORT_BY_NAME    0
#define SORT_BY_VERSION 1
#define SORT_BY_SIZE    2

// Persistent state
//
extern int  last_update;    // not a time_t until 2036
extern bool red_pill_mode;

void show_settings_dialog ();
void show_sort_settings_dialog ();

#endif /* !SETTINGS_H */
