/**
    @file core.h

    Core functionality of Application Installer.
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


#ifndef AI_CORE_H_
#define AI_CORE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <osso-log.h>

#include "definitions.h"
#include "main.h"
#include "ui/interface.h"

#define META_PACKAGE "maemo"

typedef struct
{
  GString *name;
  GString *size;
  GString *description;
  GString *version;
  gchar *rejection_reason;
} PackageInfo;

enum
{
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_VERSION,
  COLUMN_SIZE,
  COLUMN_BROKEN,
  NUM_COLUMNS
};


/**
Lists user installed packages on the device for main dialog. List includes
package name, version and installed size.

@param app_data AppData pointer
@return List of user installed packages on the device
*/
GtkTreeModel *list_packages (AppData *app_data);


/**
Adds columns to treeview.

@param app_ui_data AppUIData pointer
@param treeview Treeview of the package list
*/
void add_columns (AppUIData *app_ui_data, GtkTreeView *treeview);

gchar *package_description (AppData *app_data, gchar *package);


/**
Installs given package on the device.

@param deb Debian package to be installed
@param app_data Application data structure
@return TRUE on success, FALSE on failure
*/
void install_package (gchar *deb, AppData *app_data);

void install_package_from_uri (gchar *uri, AppData *app_data);

/**
Uninstalls selected package from the device.

@param deb Package name to be uninstalled
@param app_data Application data structure
@return TRUE on success, FALSE on failure
*/
void uninstall_package (gchar *package, gchar *size, AppData *app_data);


/**
Checks if any packages installed

@param model GtkTreeModel from list_packages().
@return Returns true if any packages are installed
*/
gboolean any_packages_installed (GtkTreeModel *model);


/**
Returns how much space left on device

@return Returns amount of space in kB left on device
*/
gint space_left_on_device(void);


#endif /* AI_CORE_H_ */
