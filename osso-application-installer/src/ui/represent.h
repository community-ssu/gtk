/**
  @file represent.h

  Function prototypes and variable definitions for representation
  component.
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

#include <gtk/gtk.h>
#include <libosso.h>
#include <osso-log.h>
#include <stdio.h>
#include <hildon-widgets/hildon-note.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "../appdata.h"
#include "../applicationinstaller-i18n.h"
#include "../definitions.h"
#include "../core.h"

void update_package_list (AppData *app_data);

void present_error_details (AppData *app_data,
			    gchar *title,
			    gchar *details);

void present_error_details_fmt (AppData *app_data,
				gchar *title,
				gchar *details_fmt,
				...);

void present_report_with_details (AppData *app_data,
				  gchar *report,
				  gchar *details);

gboolean confirm_install (AppData *app_data, PackageInfo *info);
gboolean confirm_uninstall (AppData *app_data, gchar *package);
