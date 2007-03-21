/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2005, 2006 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include <hildon/hildon-caption.h>
#include <hildon/hildon-code-dialog.h>
#include <hildon/hildon-color-button.h>
#include <hildon/hildon-color-chooser.h>
#include <hildon/hildon-date-editor.h>
#include <hildon/hildon-file-selection.h>
#include <hildon/hildon-find-toolbar.h>
#include <hildon/hildon-number-editor.h>
#include <hildon/hildon-range-editor.h>
#include <hildon/hildon-time-editor.h>
#include <hildon/hildon-time-picker.h>
#include <hildon/hildon-volumebar.h>
#include <hildon/hildon-weekday-picker.h>
#include <hildon/hildon-window.h>
#include "hailfactory.h"
#include "hail.h"

/* Hail factories class definition */
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_CAPTION, hail_caption, hail_caption_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_FIND_TOOLBAR, hail_find_toolbar, hail_find_toolbar_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_VOLUME_BAR, hail_volume_bar, hail_volume_bar_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_FILE_SELECTION, hail_file_selection, hail_file_selection_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_WEEKDAY_PICKER, hail_weekday_picker, hail_weekday_picker_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_DIALOG, hail_dialog, hail_dialog_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_TIME_PICKER, hail_time_picker, hail_time_picker_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_COLOR_SELECTOR, hail_color_selector, hail_color_selector_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_DATE_EDITOR, hail_date_editor, hail_date_editor_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_NUMBER_EDITOR, hail_number_editor, hail_number_editor_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_TIME_EDITOR, hail_time_editor, hail_time_editor_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_RANGE_EDITOR, hail_range_editor, hail_range_editor_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_COLOR_BUTTON, hail_color_button, hail_color_button_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_WINDOW, hail_window, hail_window_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_CODE_DIALOG, hail_code_dialog, hail_code_dialog_new)

/*
 *   These exported symbols are hooked by gnome-program
 * to provide automatic module initialization and shutdown.
 */
extern void hildon_accessibility_module_init     (void);
extern void hildon_accessibility_module_shutdown (void);

static int hail_initialized = FALSE;

/* Module initialization. It gets instances of hail factories, and
 * binds them to the Hildon widgets */
static void
hail_accessibility_module_init (void)
{
  if (hail_initialized)
    {
      return;
    }
  hail_initialized = TRUE;

  fprintf (stderr, "Hildon Accessibility Module initialized\n");

  HAIL_WIDGET_SET_FACTORY (GTK_TYPE_DIALOG, hail_dialog);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_CAPTION, hail_caption);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_FIND_TOOLBAR, hail_find_toolbar);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_VOLUMEBAR, hail_volume_bar);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_FILE_SELECTION, hail_file_selection);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_WEEKDAY_PICKER, hail_weekday_picker);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_TIME_PICKER, hail_time_picker);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_COLOR_CHOOSER, hail_color_chooser);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_DATE_EDITOR, hail_date_editor);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_NUMBER_EDITOR, hail_number_editor);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_TIME_EDITOR, hail_time_editor);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_RANGE_EDITOR, hail_range_editor);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_COLOR_BUTTON, hail_color_button);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_WINDOW, hail_window);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_CODE_DIALOG, hail_code_dialog);

}

/**
 * hildon_accessibility_module_init:
 * 
 **/
void
hildon_accessibility_module_init (void)
{
  hail_accessibility_module_init ();
}

/**
 * hildon_accessibility_module_shutdown:
 * 
 **/
void
hildon_accessibility_module_shutdown (void)
{
  if (!hail_initialized)
    {
      return;
    }
  hail_initialized = FALSE;

  fprintf (stderr, "Hildon Accessibility Module shutdown\n");

}

/* Gtk module initialization for Hail. It's called on Gtk 
 * initialization, when it request the module to be loaded */
int
gtk_module_init (gint *argc, char** argv[])
{
  hail_accessibility_module_init ();

  return 0;
}
