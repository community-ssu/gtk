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
#include "ui/represent.h"



/* Errormessage strings to catch */
#define DEB_FORMAT_FAILURE           "not a debian format archive"
#define DEB_FORMAT_FAILURE2          "unexpected end of file"
#define DPKG_NO_SUCH_PACKAGE         "is not installed and no info"
#define DPKG_NO_SUCH_PACKAGE2        "not-installed"
#define DPKG_NO_SUCH_PACKAGE3        "dpkg-deb --contents"

#define DPKG_STATUS_PREFIX           "status: "
#define DPKG_DESCRIPTION_PREFIX      "Description: "
#define DPKG_SIZE_PREFIX             "Installed-Size: "
#define DPKG_VERSION_PREFIX          "Version: "

#define DPKG_STATUS_SUFFIX_INSTALL   ": installed"
#define DPKG_STATUS_SUFFIX_UNINSTALL ": not-installed"

#define DPKG_ERROR_DEPENDENCY        "depends on"
#define DPKG_ERROR_LOCKED            "dpkg: status database area is locked"
#define DPKG_ERROR_INSTALL_DEPENDS   "  Package "
#define DPKG_ERROR_INCOMPATIBLE      "does not match system"
#define DPKG_ERROR_CANNOT_ACCESS     "cannot access archive"
#define DPKG_ERROR_NO_SUCH_FILE      "failed to read archive"
#define DPKG_ERROR_MEMORYFULL        "No space left on device"
#define DPKG_ERROR_MEMORYFULL2       "not enough space"
#define DPKG_ERROR_TIMEDOUT          "operation time-outed"
#define DPKG_ERROR_CLOSE_APP         "close application before installing"
#define DPKG_STATUS_UNINSTALL_ERROR  "Errors were encountered while processing"
#define DPKG_STATUS_INSTALL_ERROR    "dpkg: error processing"

#define DPKG_FIELD_ERROR             "is not installed"
#define DPKG_FIELD_NAME              "Package:"
#define DPKG_FIELD_VERSION           "Version:"
#define DPKG_FIELD_SIZE              "Installed-Size:"
#define DPKG_FIELD_MAINTAINER        "Maintainer:"
#define DPKG_FIELD_DEPENDS           "Depends:"
#define DPKG_FIELD_DESCRIPTION       "Description:"

#define DPKG_DEB_PACKAGE_SUFFIX      ".deb"
#define DPKG_TAR_INVALID_OPTION      "tar: invalid option"

#define META_PACKAGE                 "maemo"
#define READ_BUFFER_LENGTH           80

#define DPKG_INFO_INSTALLED          1
#define DPKG_INFO_PACKAGE            2

#define MESSAGE_DOUBLECLICK          ""


typedef struct
{
  gchar *name;
  gchar *version;
  gchar *size;
} ListItems;

typedef struct
{
  GString *name;
  GString *size;
  GString *description;
  GString *version;
  //GString *maintainer;
  //GString *depends;
} PackageInfo;

typedef struct
{
  GString *out;
  GString *err;
  GArray *list;
} DpkgOutput;

typedef struct
{
  const gchar *name;
  const gchar *version;
  const gchar *size;
} App;

enum
{
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_VERSION,
  COLUMN_SIZE,
  NUM_COLUMNS
};


/**
Calls dpkg for different functionalities like package listing, installing
and uninstalling.

@param method Usage method
@param args Arguments for dpkg usage
@param app_data AppData structure if we want to have progressbar, NULL otherw.
@return Structure including dpkg's output
*/
DpkgOutput use_dpkg(guint method, gchar **args, AppData *app_data);


/**
Calls dpkg wrapper to perform dpkg operations as user install.

@param cmdline commandline string to be wrapped
@param cmd_stdout Where the stdout of cmd is stored
@param cmd_stderr Where the stderr of cmd is stored
@param wrap_it TRUE if wrapped, FALSE if not wrapped (non-wrapped is faster)
@return Returns OSSO_OK
*/
gint dpkg_wrapper(const gchar *cmdline, gchar **cmd_stdout, 
		  gchar **cmd_stderr, gboolean wrap_it);



/**
Lists user installed packages on the device for main dialog. List includes
package name, version and installed size.

@param app_ui_data AppUIData pointer
@return List of user installed packages on the device
*/
GtkTreeModel *list_packages(AppUIData *app_ui_data);


/**
Lists built-in packages on the device for testing before package
installation. List includes space separated list of the package names.

@return List of built-in packages on the device
*/
const gchar *list_builtin_packages(void);


/**
Adds columns to treeview.

@param app_ui_data AppUIData pointer
@param treeview Treeview of the package list
*/
void add_columns(AppUIData *app_ui_data, GtkTreeView *treeview);


/**
Digs up name, size and description of given installed package or
filename.deb.

@param package_or_deb Package or .deb info is dug from
@param installed Is the package installed (1) or from .deb file (0)
@return Returns ListItems struct with filled fields
*/
PackageInfo package_info(gchar *package_or_deb, gint installed);


/**
Shows description of the selected package in the package list.

@param package Package which description to show
@param installed Relay this to package_info, is it already installed
@return Package's description
*/
gchar *show_description(gchar *package, gint installed);


/**
Returns list of packages that are dependent on this package 

@param package Package which dependecies to find out
@return List of packages dependant on this package
*/
gchar *show_remove_dependencies(gchar *package);


/**
Installs given package on the device.

@param deb Debian package to be installed
@param app_data Application data structure
@return TRUE on success, FALSE on failure
*/
gboolean install_package(gchar *deb, AppData *app_data);


/**
Uninstalls selected package from the device.

@param deb Package name to be uninstalled
@param app_data Application data structure
@return TRUE on success, FALSE on failure
*/
gboolean uninstall_package(gchar *package, AppData *app_data);


/**
Checks if any packages installed

@param model GtkTreeModel from list_packages() or NULL to load it
@return Returns true if any packages installed
*/
gboolean any_packages_installed(GtkTreeModel *model);


/**
Verbalizes dpkg error message

@param err dpkg stderr output
@return Returns localized string based on dpkg err
*/
gchar *verbalize_error(gchar *err);


/**
Returns how much space left on device

@return Returns amount of space in kB left on device
*/
gint space_left_on_device(void);


#endif /* AI_CORE_H_ */
