/**
  @file appdata.h

  Header file for defining the data structures the whole
  application needs to function properly.
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

/*
  Application data structure. Pointer to this is passed eg. with
  UI event callbacks.  This structure should contain ALL
  application related data, which otherwise would be impossible
  to pass with events to callbacks. It makes complicated
  intercommunication between widgets possible.

  So when you add a widget to ui, put a pointer to it inside
  this struct.
*/

/* GTK */
#include <gtk/gtk.h>

/* Hildon */
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>

#include <libosso.h>
#include <osso-log.h>

#ifndef APPDATA_H
#define APPDATA_H

/* UI related application data */
typedef struct _AppUIData AppUIData;
struct _AppUIData {
  GtkWidget *main_dialog;
  HildonAppView *main_view;
  /* Different UI widgets */
  GtkWidget *main_hbox;
  GtkWidget *treeview;
  /* package list, scrolledwindow */
  GtkWidget *package_list;
  /* Labels that are shown as alternatives to the package list */
  GtkWidget *empty_list_label;
  GtkWidget *database_unavailable_label;
  /* main label, textbuffer */
  GtkTextBuffer *main_label;
  /* main dialog buttons */
  GtkWidget *installnew_button;
  GtkWidget *uninstall_button;
  GtkWidget *close_button;
  /* event id for show popup menu timer callback */
  guint32 popup_event_id;
  /* column objects in treeview */
  GtkTreeViewColumn *name_column;
  GtkTreeViewColumn *version_column;
  GtkTreeViewColumn *size_column;
};

/* OSSO related application data */
typedef struct _AppOSSOData AppOSSOData;
struct _AppOSSOData {
  osso_context_t *osso;
};

/* General application data */
typedef struct _AppData AppData;
struct _AppData {
  AppUIData *app_ui_data;
  AppOSSOData *app_osso_data;
};

#endif /* APPDATA_H */
