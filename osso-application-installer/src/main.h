/**
    @file main.h

    Main header file for OSSO Application Installer
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


#ifndef AI_MAIN_H_
#define AI_MAIN_H_

#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <locale.h>
#include <osso-log.h>
#include <signal.h>

#define USE_GNOME_VFS 1

#ifdef USE_GNOME_VFS
# include <libgnomevfs/gnome-vfs.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "appdata.h"
#include "applicationinstaller-i18n.h"
#include "core.h"
#include "dbus.h"
#include "definitions.h"
#include "ui/interface.h"

#endif /* AI_MAIN_H_ */
