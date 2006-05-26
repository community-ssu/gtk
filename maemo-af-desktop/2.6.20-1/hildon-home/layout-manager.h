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
 */

/**
* @file layout-manager.h
*/

#ifndef LAYOUT_MANAGER_H
#define LAYOUT_MANAGER_H

/**
 * @layout_mode_begin
 *
 * @param GtkEventBox *home_event_box the home applet area eventbox
 * @param GtkFixed * the home (fixed) container
 * @param GList * added_applets The list of added applet, or NULL
 * @param GList * removed_applets The list of applets scheduled for removal 
 * or NULL
 * @param GtkWidget * titlebar_label the titlebar label
 *
 * @return void
 *
 * Initializes the applets in layoutmode Nodes, connects to signal
 * handlers, dims statusbar and tasknav.
 **/

void layout_mode_begin (GtkEventBox * home_event_box,
			GtkFixed * home_fixed,
			GList * added_applets,
			GList * removed_applets,
                        GtkWidget * titlebar_label); 


/**
 * @layout_mode_end
 *
 * @param gboolean rollback should the layout mode changes be annulled
 *
 * @return void
 *
 * Deinitializes everything. Always the last function called despite how
 * layout mode ends.
 * If rollback is specified, everything done
 * during layout mode (and the changes indicated when layout mode
 * begin is called (removing added applets and restoring old ones)
 * are reversed.
 **/

void layout_mode_end (gboolean rollback);

/** layout_mode_init
 *
 *  Sets some internal parameters.
 */

void layout_mode_init (osso_context_t * osso_context);



G_END_DECLS

#endif /* LAYOUT_MANAGER_H */
