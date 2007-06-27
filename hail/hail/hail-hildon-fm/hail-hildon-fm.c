#include <hail/hailfactory.h>
#include "hail-hildon-fm.h"

#include "hailfileselection.h"

#include <hildon/hildon-file-selection.h>

/* Hail factories class definition */
HAIL_ACCESSIBLE_FACTORY (HAIL_TYPE_FILE_SELECTION, hail_file_selection, hail_file_selection_new)

void
hail_hildon_fm_init (void)
{
  HAIL_WIDGET_SET_FACTORY (HILDON_TYPE_FILE_SELECTION, hail_file_selection);
}
