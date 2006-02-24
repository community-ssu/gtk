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

#ifndef UTIL_H
#define UTIL_H

#include <gtk/gtk.h>

#include "main.h"

void ask_yes_no (const gchar *question_fmt,
		 void (*cont) (bool res, void *data), void *data);

void ask_yes_no_with_details (const gchar *question_fmt,
			      package_info *pi, bool installed,
			      void (*cont) (bool res, void *data), void *data);

void annoy_user (const gchar *text);
void annoy_user_with_details (const gchar *text,
			      package_info *pi, bool installed);
void annoy_user_with_log (const gchar *text);

void show_progress (const char *title);
void set_progress (const gchar *title, float fraction);
void hide_progress ();

GtkWidget *make_small_text_view (const char *text);

typedef void package_info_callback (package_info *);

GtkWidget *get_global_package_list_widget ();
void set_global_package_list (GList *packages,
			      bool installed,
			      package_info_callback *selected,
			      package_info_callback *activated);
void global_package_info_changed (package_info *pi);

typedef void section_activated (section_info *);

GtkWidget *get_global_section_list_widget ();
void set_global_section_list (GList *sections, section_activated *act);

void size_string_general (char *buf, size_t n, int bytes);
void size_string_detailed (char *buf, size_t n, int bytes);

void show_deb_file_chooser (void (*cont) (char *filename, void *data),
			    void *data);

GdkPixbuf *pixbuf_from_base64 (const char *base64);

void localize_file (char *uri,
		    void (*cont) (char *local, void *data),
		    void *data);

#endif /* !UTIL_H */
