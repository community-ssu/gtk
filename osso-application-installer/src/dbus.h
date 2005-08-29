/**
   @file dbus.h
   
   Header for DBUS handling functions
   <p>
*/

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


#ifndef AI_DBUS_H
#define AI_DBUS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "appdata.h"

/**
 Initialize osso

 @param app Application specific data
*/
void init_osso (AppData *app_data);

/** Register a handler for mime-open that calls do_install
 */
void register_mime_open_handler (AppData *app_data);

/**
 Deinitialize osso
*/
void deinit_osso (AppData *app_data);

#endif /* AI_DBUS_H */
