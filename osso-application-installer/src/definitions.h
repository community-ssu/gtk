/**
   	@file definitions.h

   	All definitions used widely in the application.
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


#define UI_MARGIN_HORIZONTAL     12

#define FAKEROOT_BINARY          "fakeroot"
#define DPKG_BINARY              "dpkg"
#define DPKG_QUERY_BINARY        "dpkg-query"
#define DPKG_METHOD_LIST         1
#define DPKG_METHOD_INSTALL      2
#define DPKG_METHOD_UNINSTALL    3
#define DPKG_METHOD_FIELDS       4
#define DPKG_METHOD_STATUS       5
#define DPKG_METHOD_LIST_BUILTIN 6

#define DEFAULT_FOLDER           "/"

#define PATH_ENV                 "PATH"
#define PATH_ADD                 ":/sbin:/usr/sbin"

#define AI_HELP_KEY_MAIN "Utilities_ControlPanelAppletApplicationInstaller_applicationinstaller"
#define AI_HELP_KEY_ERROR "Utilities_ControlPanelAppletApplicationInstaller_errordetails"
#define AI_HELP_KEY_DEPENDS "Utilities_ControlPanelAppletApplicationInstaller_dependencyconflict"
#define AI_HELP_KEY_INSTALL "Utilities_ControlPanelAppletApplicationInstaller_installapplication"
