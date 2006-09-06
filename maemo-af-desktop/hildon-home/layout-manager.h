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
* @file layout-manager.h
*/

#ifndef LAYOUT_MANAGER_H
#define LAYOUT_MANAGER_H

#include <gtk/gtkwidget.h>
#include <libosso.h>
#include "hildon-home-titlebar.h"

G_BEGIN_DECLS

/** layout_mode_begin
 *
 * Enters layout mode. 
 *
 * @param titlebar the titlebar widget 
 *
 * @param applet_area the main applet area
 *
 * @param added_applets the list of new applets to be added.
 *
 * @param removed_applets the list of on-screen applets to be removed.
 */

void layout_mode_begin (HildonHomeTitlebar *titlebar,
			GtkFixed           *applet_area,
			GList              *added_applets,
			GList              *removed_applets);

void layout_mode_cancel (void);
void layout_mode_accept (void);

/** layout_mode_end
 *
 * Quits layout mode. 
 *
 * @param rollback TRUE if the applets must be moved back to their
 * original places.
 */

void layout_mode_end (gboolean rollback);

/** layout_mode_init
 *
 *  Sets some internal parameters.
 */

void layout_mode_init (osso_context_t * osso_context);

G_END_DECLS

#endif /* LAYOUT_MANAGER_H */
