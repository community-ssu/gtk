#include <hail/hailfactory.h>
#include <hail-hildon-libs.h>

#include "hailappview.h"
#include "hailcaption.h"
#include "hailcodedialog.h"
#include "hailcolorbutton.h"
#include "hailcolorselector.h"
#include "haildateeditor.h"
#include "haildialog.h"
#include "hailfindtoolbar.h"
#include "hailgrid.h"
#include "hailgriditem.h"
#include "hailnumbereditor.h"
#include "hailrangeeditor.h"
#include "hailtimeeditor.h"
#include "hailtimepicker.h"
#include "hailvolumebar.h"
#include "hailweekdaypicker.h"
#include "hailwindow.h"

#include <hildon/hildon-caption.h>
#include <hildon/hildon-code-dialog.h>
#include <hildon/hildon-color-button.h>
#include <hildon/hildon-color-chooser.h>
#include <hildon/hildon-date-editor.h>
#include <hildon/hildon-find-toolbar.h>
#include <hildon/hildon-number-editor.h>
#include <hildon/hildon-range-editor.h>
#include <hildon/hildon-time-editor.h>
#include <hildon/hildon-time-picker.h>
#include <hildon/hildon-volumebar.h>
#include <hildon/hildon-weekday-picker.h>
#include <hildon/hildon-window.h>


/* Hail factories class definition */
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_CAPTION, hail_caption, hail_caption_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_FIND_TOOLBAR, hail_find_toolbar, hail_find_toolbar_new)
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_VOLUME_BAR, hail_volume_bar, hail_volume_bar_new)
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

void
hail_hildon_libs_init (void)
{
  HAIL_WIDGET_SET_FACTORY (GTK_TYPE_DIALOG, hail_dialog);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_CAPTION, hail_caption);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_FIND_TOOLBAR, hail_find_toolbar);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_VOLUMEBAR, hail_volume_bar);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_WEEKDAY_PICKER, hail_weekday_picker);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_TIME_PICKER, hail_time_picker);
/*   HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_COLOR_CHOOSER, hail_color_chooser); */
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_DATE_EDITOR, hail_date_editor);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_NUMBER_EDITOR, hail_number_editor);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_TIME_EDITOR, hail_time_editor);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_RANGE_EDITOR, hail_range_editor);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_COLOR_BUTTON, hail_color_button);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_WINDOW, hail_window);
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_CODE_DIALOG, hail_code_dialog);

}
