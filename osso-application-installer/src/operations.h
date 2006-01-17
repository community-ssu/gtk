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

#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <gtk/gtk.h>

#include <apt-pkg/pkgcache.h>

/* Large scale operations of the Application installer
 */

void operations_init ();

/* Update the local copy of the package meta information from all
   repositories.  Return value is true if the cache is up-to-date
   afterwards.  All relevant error messages etc are handled by these
   functions.
*/

bool update_package_cache_no_confirm ();
bool update_package_cache ();

/* Get a list of relevant packages with all the information we need
   for the UI.  This list is only valid until the next
   update_package_cache.  You can free it after that, but not access
   it.
*/

struct version_info {
  const gchar *version;
  const gchar *section;
  const gchar *maintainer;
  const gchar *short_description;
  pkgCache::VerIterator cache;
};

struct package_info {
  const gchar *name;
  version_info *installed;
  version_info *available;
  gint installed_size;
  pkgCache::PkgIterator cache;

  // for simulated install/upgrade
  bool install_simulated;
  bool install_possible;
  gint download_size;
  gint install_user_size_delta;
 
  // for simulated uninstall
  bool uninstall_simulated;
  bool uninstall_possible;
  gint uninstall_user_size_delta;
};

GList *get_package_list (bool installed_flag,
			 bool skip_nonavailable_flag);
void free_package_list (GList *pl);

struct section_info {
  const gchar *name;
  GList *packages;
};

GList *sectionize_packages (GList *packages, bool installed);

gchar *get_long_description (version_info *v);

void simulate_install (package_info *pi);
void simulate_uninstall (package_info *pi);

void show_install_summary (package_info *pi, GString *str);
void show_uninstall_summary (package_info *pi, GString *str);
void show_dependencies (version_info *vi, GString *str);

struct progress_with_cancel;

void setup_install (package_info *pi);
void setup_uninstall (package_info *pi);
bool do_operations (progress_with_cancel *pc);

#endif /* !OPERATIONS_H */
