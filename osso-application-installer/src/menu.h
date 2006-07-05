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

#ifndef MENU_H
#define MENU_H

#include <gtk/gtk.h>

void create_menu (GtkMenu *main);
void set_details_menu_sensitive (bool);
void set_search_menu_sensitive (bool);
void set_operation_menu_label (const gchar *label, bool sensitive,
			       const gchar *insens);

void set_fullscreen_menu_check (bool f);

GtkWidget *create_package_menu (const char *op_label);

#endif /* !MENU_H */
