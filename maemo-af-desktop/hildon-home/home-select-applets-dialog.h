/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
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
 */

 /**
  * @file home-select-applets-dialog.h
  * 
  **/
 
#ifndef HOME_SELECT_APPLETS_DIALOG_H
#define HOME_SELECT_APPLETS_DIALOG_H

#include "hildon-base-lib/hildon-base-dnotify.h"
#include <gtk/gtk.h>

#include <glib/gi18n.h>
#include <libosso.h>
#include "hildon-home-titlebar.h"

G_BEGIN_DECLS


/* 'Select Applets'-dialog constants */
#define HOME_APPLETS_MAXIMUM_VISIBLE_ROWS 8
#define HOME_APPLETS_MARGIN_DEFAULT 6
/* a long time ago 30 was speced to be a text height */
#define HOME_APPLETS_DIALOG_HEIGHT  (HOME_APPLETS_MAXIMUM_VISIBLE_ROWS * 30 + 2 * HOME_APPLETS_MARGIN_DEFAULT)  
#define HOME_APPLETS_DESKTOP_SUFFIX    ".desktop"
#define HOME_APPLETS_DESKTOP_DIR       "/usr/share/applications/hildon-home/"
#define HOME_APPLETS_DESKTOP_GROUP     "Desktop Entry"
#define HOME_APPLETS_DESKTOP_NAME_KEY  "Name"
#define HOME_APPLETS_SELECT_TITLE      _("home_ti_select_applets")
#define HOME_APPLETS_SELECT_OK         _("home_bd_select_applets_ok")
#define HOME_APPLETS_SELECT_CANCEL     _("home_bd_select_applets_cancel")


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
void show_select_applets_dialog (osso_context_t  *osso_home,
				 GList           *applets, 
        		         GList          **added_list, 
		                 GList          **removed_list);

void select_applets_selected (osso_context_t     *osso_home,
			      HildonHomeTitlebar *titlebar,
                              GtkFixed           *applet_area);

G_END_DECLS

#endif
