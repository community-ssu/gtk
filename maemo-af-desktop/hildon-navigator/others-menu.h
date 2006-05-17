/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
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
 */


/**
* @file others-menu.h
*/


#ifndef OTHERS_MENU_H
#define OTHERS_MENU_H

#include <gtk/gtkbutton.h>
#include <gtk/gtktreemodel.h>

/* Button Icon */
#define OTHERS_MENU_ICON_NAME "qgn_grid_tasknavigator_others"
#define OTHERS_MENU_ICON_SIZE 64

/* Menu icons */
#define MENU_ITEM_DEFAULT_ICON ""

#define MENU_ITEM_SUBMENU_ICON        "qgn_list_gene_fldr_cls"
#define MENU_ITEM_SUBMENU_ICON_DIMMED "qgn_list_gene_nonreadable_fldr"
#define MENU_ITEM_DEFAULT_APP_ICON    "qgn_list_gene_default_app"
#define MENU_ITEM_ICON_SIZE           26

#define BORDER_WIDTH 7
#define BUTTON_HEIGHT 90


#define MENU_Y_POS 180
#define MENU_MAX_WIDTH 360

#define OM_BUTTON_PRESS_EVENT_COLLAPSE_TIMEOUT 100

#define WORKAREA_ATOM "_NET_WORKAREA"
#define NAVIGATOR_BUTTON_THREE "hildon-navigator-button-three"

typedef struct OthersMenu OthersMenu_t;
typedef struct _om_changed_cb_data_st _om_changed_cb_data_t;

typedef void (_as_update_callback)(GKeyFile *keyfile);

OthersMenu_t *others_menu_init( void );

GtkWidget *others_menu_get_button( OthersMenu_t *am );

void others_menu_initialize_menu( OthersMenu_t *am , void *as_menu_cb);

void others_menu_deinit( OthersMenu_t *am );

void others_menu_get_items( GtkWidget *widget, OthersMenu_t *am,
		GtkTreeModel *model, GtkTreeIter *iter );

#endif /* OTHERS_MENU_H */
