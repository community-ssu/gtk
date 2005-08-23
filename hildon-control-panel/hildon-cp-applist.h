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

#ifndef HILDON_CLOCKAPP_DESK_APPLIST_H
#define HILDON_CLOCKAPP_DESK_APPLIST_H

#include <pango/pango-font.h>
#include <hildon-widgets/hildon-app.h>
#include <libmb/mbdotdesktop.h>

G_BEGIN_DECLS

#define TOOLBAR_ICON_WIDTH  24
#define TOOLBAR_ICON_HEIGHT 24

#define ADDITIONAL_CP_APPLETS_DIR_ENVIRONMENT "User_Applets_Dir"

typedef void (hildon_applist_activate_cb_f)(MBDotDesktop*, gpointer data, gboolean user_activated);
typedef void (hildon_applist_focus_cb_f)(MBDotDesktop*, gpointer data);

enum
{
  ICON_SIZE_SMALL,
  ICON_SIZE_NORMAL,
  ICON_SIZE_LARGE
};

void 
hildon_cp_applist_initialize( hildon_applist_activate_cb_f callback,
                                gpointer d1,
                                hildon_applist_focus_cb_f focus_callback,
                                gpointer d2,
                                GtkWidget *hildon_appview,
                                gchar *path);

void
hildon_cp_applist_reread_dot_desktops( void );

GtkWidget *
hildon_cp_applist_get_grid( void );


MBDotDesktop*
hildon_cp_applist_get_entry( const gchar * entryname);

gboolean
hildon_cp_applist_focus_item( const gchar * entryname);

G_END_DECLS

#endif
