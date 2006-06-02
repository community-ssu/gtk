/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * libglade-hildon: Hildon widgets for Glade
 *
 * Copyright (C) 2005-2006 Nokia 
 * Contact: Erik Karlsson <erik.b.karlsson@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <glade/glade.h>
#include <glade/glade-build.h>

#include <hildon-widgets/hildon-window.h>
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-find-toolbar.h>

#include <hildon-widgets/hildon-color-button.h>

#include <hildon-widgets/hildon-time-editor.h>
#include <hildon-widgets/hildon-date-editor.h>
#include <hildon-widgets/hildon-number-editor.h>
#include <hildon-widgets/hildon-range-editor.h>
#include <hildon-widgets/hildon-weekday-picker.h>

#include <hildon-widgets/hildon-controlbar.h>
#include <hildon-widgets/hildon-seekbar.h>
#include <hildon-widgets/hildon-hvolumebar.h>
#include <hildon-widgets/hildon-vvolumebar.h>

#include <hildon-widgets/hildon-font-selection-dialog.h>
#include <hildon-widgets/hildon-get-password-dialog.h>
#include <hildon-widgets/hildon-set-password-dialog.h>
#include <hildon-widgets/hildon-sort-dialog.h>
#include <hildon-widgets/hildon-wizard-dialog.h>
#include <hildon-widgets/hildon-note.h>


#define HILDON_TYPE_DATE_EDITOR HILDON_DATE_EDITOR_TYPE
#define HILDON_TYPE_RANGE_EDITOR HILDON_RANGE_EDITOR_TYPE
/*#define HILDON_TYPE_WEEKDAY_PICKER HILDON_WEEKDAY_PICKER_TYPE*/

static GtkWidget *
appview_find_internal_child(GladeXML *xml, GtkWidget *parent,
			       const gchar *childname)
{
    if (!strcmp(childname, "vbox")) {
			GtkWidget *pl;

			pl = HILDON_APPVIEW (parent)->vbox;
			return pl;
    }
    return NULL;
}

void
glade_module_register_widgets(void)
{
/*
    glade_register_custom_prop (GTK_TYPE_COMBO_BOX, "items", combo_box_set_items);
*/

/* lgpl-stuff */
    glade_register_widget (HILDON_TYPE_APP, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (HILDON_TYPE_APPVIEW, glade_standard_build_widget,
			   glade_standard_build_children, appview_find_internal_child);
    glade_register_widget (HILDON_TYPE_CAPTION, glade_standard_build_widget,
			   glade_standard_build_children, NULL);
    glade_register_widget (HILDON_TYPE_FIND_TOOLBAR, glade_standard_build_widget,
			   glade_standard_build_children, NULL);


/* libs-stuff */

    glade_register_widget (HILDON_TYPE_WINDOW, glade_standard_build_widget,
		                               glade_standard_build_children, NULL);
    
    
    glade_register_widget (HILDON_TYPE_COLOR_BUTTON, glade_standard_build_widget,
			   NULL, NULL);

/* Editors */
    glade_register_widget (HILDON_TYPE_TIME_EDITOR, glade_standard_build_widget,
			   NULL, NULL);
/*    glade_register_widget (HILDON_TYPE_DATE_EDITOR, glade_standard_build_widget,
			   NULL, NULL);*/
    glade_register_widget (HILDON_TYPE_NUMBER_EDITOR, glade_standard_build_widget,
			   NULL, NULL);
/*    glade_register_widget (HILDON_TYPE_RANGE_EDITOR, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (HILDON_TYPE_WEEKDAY_PICKER, glade_standard_build_widget,
			   NULL, NULL);
*/
/* Bars */
    glade_register_widget (HILDON_TYPE_CONTROLBAR, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (HILDON_TYPE_SEEKBAR, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (HILDON_TYPE_HVOLUMEBAR, glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (HILDON_TYPE_VVOLUMEBAR, glade_standard_build_widget,
			   NULL, NULL);

/* Dialogs and popups */

 /*   glade_register_widget (HILDON_TYPE_FILE_DETAILS_DIALOG,
         glade_standard_build_widget,
			   NULL, NULL);*/
    glade_register_widget (HILDON_TYPE_FONT_SELECTION_DIALOG,
         glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (HILDON_TYPE_GET_PASSWORD_DIALOG,
         glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (HILDON_TYPE_SET_PASSWORD_DIALOG,
         glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (HILDON_TYPE_NOTE,
         glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (HILDON_TYPE_SORT_DIALOG,
         glade_standard_build_widget,
			   NULL, NULL);
    glade_register_widget (HILDON_TYPE_WIZARD_DIALOG,
         glade_standard_build_widget,
			   NULL, NULL);

    glade_provide("gtk");
}
