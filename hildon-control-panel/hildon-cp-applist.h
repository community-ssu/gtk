/*
 * This file is part of hildon-control-panel
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

#ifndef HILDON_CONTROLPANEL_DESK_APPLIST_H
#define HILDON_CONTROLPANEL_DESK_APPLIST_H

#include <gtk/gtk.h>
#include <pango/pango-font.h>
#include <libmb/mbdotdesktop.h>


G_BEGIN_DECLS


#define TOOLBAR_ICON_WIDTH  24
#define TOOLBAR_ICON_HEIGHT 24

#define ADDITIONAL_CP_APPLETS_DIR_ENVIRONMENT "User_Applets_Dir"

/* GConf key for controlpanel group list */
#define GCONF_CONTROLPANEL_GROUPS_KEY  "/apps/osso/apps/controlpanel/groups"
#define GCONF_CONTROLPANEL_GROUP_IDS_KEY  "/apps/osso/apps/controlpanel/group_ids"


/* .desktop keys */
#define HCP_DESKTOP_GROUP               "Desktop Entry"
#define HCP_DESKTOP_KEY_NAME            "Name"
#define HCP_DESKTOP_KEY_ICON            "Icon"
#define HCP_DESKTOP_KEY_CATEGORY        "Categories"
#define HCP_DESKTOP_KEY_PLUGIN          "X-control-panel-plugin"

typedef void (hildon_applist_activate_cb_f)(MBDotDesktop*, gpointer data, gboolean user_activated);
typedef void (hildon_applist_focus_cb_f)(MBDotDesktop*, gpointer user_data);

enum
{
  ICON_SIZE_SMALL,
  ICON_SIZE_NORMAL,
  ICON_SIZE_LARGE
};

typedef struct _HCPCategory {
    gchar   *name;
    gchar   *id;
    GSList  *applets;
} HCPCategory;

typedef struct _HCPAppList {
    GHashTable *applets;
    GSList *categories;
} HCPAppList;


HCPAppList *                hcp_al_new (void);

gboolean hildon_cp_applist_focus_item (HCPAppList *al,
                                       const gchar * entryname);

void hcp_al_update_list               (HCPAppList *al);

void 
hildon_cp_applist_initialize( hildon_applist_activate_cb_f callback,
                              gpointer d1,
                              hildon_applist_focus_cb_f focus_callback,
                              gpointer d2,
                              GtkWidget *hildon_appview,
                              const gchar *path );

void
hildon_cp_applist_reread_dot_desktops( void );

GList *
hildon_cp_applist_get_grids( void );

MBDotDesktop*
hildon_cp_applist_get_entry( const gchar * entryname );



G_END_DECLS

#endif
