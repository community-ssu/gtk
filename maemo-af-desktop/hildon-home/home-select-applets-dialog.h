/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 **/

 /**
  * @file home-select-applets-dialog.h
  * 
  **/
 
#ifndef HOME_SELECT_APPLETS_DIALOG_H
#define HOME_SELECT_APPLETS_DIALOG_H

#include "hildon-base-lib/hildon-base-dnotify.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* generic values */
#define _(a) gettext(a)

/* 'Select Applets'-dialog constants */
#define HILDON_HOME_APPLETS_DESKTOP_DIR       "/usr/share/applications/hildon-home/"
#define HILDON_HOME_APPLETS_DESKTOP_GROUP     "Desktop Entry"
#define HILDON_HOME_APPLETS_DESKTOP_NAME_KEY  "Name"
#define HILDON_HOME_APPLETS_SELECT_TITLE      _("home_ti_select_applets")
#define HILDON_HOME_APPLETS_SELECT_OK         _("home_bd_select_applets_ok")
#define HILDON_HOME_APPLETS_SELECT_CANCEL     _("home_bd_select_applets_cancel")


/* Select Applets-dialog columns in TreeViewModel */
enum {
    CHECKBOX_COL = 0,     /* Checkbox */
    APPLET_NAME_COL,      /* Applet name string */
    DESKTOP_FILE_COL,     /* Desktop file string */
    APPLETS_LIST_COLUMNS  /* Number of columns */
};

typedef struct {
    GList* list_data;
    GtkTreeModel* model_data; 
} SelectAppletsData;


/* Public function declarations */
void show_select_applets_dialog(GList *applets, 
		                GList **added_list, 
				GList **removed_list);

void select_applets_selected(GtkEventBox *home_event_box,
		             GtkFixed *home_fixed);

G_END_DECLS

#endif
