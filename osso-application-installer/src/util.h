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

#include "operations.h"

bool ask_yes_no (const gchar *question_fmt);
void annoy_user (const gchar *text);

struct progress_with_cancel {

  void set (const gchar *title, float fraction);
  bool is_cancelled ();

  void close ();

  progress_with_cancel ();
  ~progress_with_cancel ();

 private:
  GtkWidget *dialog;
  GtkProgressBar *bar;
  bool cancelled;
};

typedef void item_widget_attacher (GtkTable *tabel, gint row,
				   gpointer item);

GtkWidget *make_scrolled_table (GList *items, gint columns,
				item_widget_attacher *attacher);

typedef void section_activated (section_info *);
GtkWidget *make_section_list (GList *packages, section_activated *act);

typedef void package_info_callback (package_info *);
GtkWidget *make_package_list (GList *packages,
			      bool installed,
			      package_info_callback *selected,
			      package_info_callback *activated);

GtkWidget *make_small_text_view (const char *text);

#endif /* !UTIL_H */
