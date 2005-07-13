/**
  @file ui.h

  Application user interface definitions
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

#ifndef UI_H
#define UI_H

/* popup menu */
static GtkActionEntry action_entries[] = {
  { "ai_error_details_csm_copy", NULL,
    "ai_error_details_csm_copy", NULL, NULL, 
    G_CALLBACK(on_copy_activate) },

  { "ai_error_details_csm_help", NULL,
    "ai_error_details_csm_help", NULL, NULL, 
    G_CALLBACK(on_help_activate) },
};
static guint n_action_entries = G_N_ELEMENTS(action_entries);


/* popup menu definition */
static const gchar *popup_info = 
"<ui>"
"  <popup name='Popup'>"
"    <menuitem action='ai_error_details_csm_copy'/>"
"    <separator name='Sep1'/>"
"    <menuitem action='ai_error_details_csm_help'/>"
"  </popup>"
"</ui>";


#if 0
/*
	A special workaround structure to make Debian package build process
	find certain localized strings.
	
	User interface strings above are referred to directly by their
	identifiers, i.e. the standard gettext macro _("") is not used.
	The Debian package build process fails to recognize these
	strings as they are referred to without using the standard macro.
	Hence, they get removed from generated localization files.
	
	This structure contains dummy references to those strings so that
	they are included in the po file build process.
*/
void localize_strings()
{
  char * strings_to_localize[] =
  {
    _("ai_error_details_csm_copy"),
    _("ai_error_details_csm_help")
  };
}
#endif

#endif /* AI_UI_H */
