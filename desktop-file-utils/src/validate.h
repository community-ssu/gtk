#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "desktop_file.h"

gboolean desktop_file_validate (GnomeDesktopFile *df,
                                const char       *filename);
gboolean desktop_file_fixup    (GnomeDesktopFile *df,
                                const char       *filename);

