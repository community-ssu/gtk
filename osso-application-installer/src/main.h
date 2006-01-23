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

#ifndef MAIN_H
#define MAIN_H

#include "apt-worker-proto.h"

struct package_info {
  const char *name;
  const char *installed_version;
  int installed_size;
  const char *available_version;
  const char *available_section;

  bool have_info;
  apt_proto_package_info info;
  const char *installed_short_description;
  const char *available_short_description;
};

void call_with_package_info (package_info *pi,
			     void (*func) (package_info *, void *), void *);

package_info *alloc_package_info ();
void free_package_info (package_info *);

struct section_info {
  const char *name;
  GList *packages;
};

void show_current_details ();
void do_current_operation ();

#endif /* !MAIN_H */
