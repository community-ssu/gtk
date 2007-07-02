/*
 * This file is part of hildon-help
 *
 * Copyright (C) 2006 Nokia Corporation. All rights reserved.
 *
 * Contact: Jakub Pavelek <jakub.pavelek@nokia.com>
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

#include "hildon-help-private.h"
#include "internal.h"

#include <gtk/gtk.h>

/* Height limit for showing a pic in full size
 *
 * This should be around the size of 'normal' Help text in browser.
 */
#define FORCED_IMAGE_HEIGHT 26
gboolean dialog_mode; /*internal.h*/

/*---( HelpLib portion -- HTML conversion )---*/

/**
  Write an HTML tag entry for an inline picture to 'buf'.

  Note: Picture is scaled to "browser default text size" (whichever that is)

  @param buf text buffer
  @param bufsize size of @buf, in bytes
  @param fname filename (with path) of a png, gif, or other such picture
*/
gboolean graphic_tag( char *buf, size_t bufsize, const char *fname)
{
    guint w=0, h=0;
    gboolean ok;

    if (!buf || bufsize <= 0 || !fname) return FALSE;

    /*
     * Basic functionality (no scaling):
     *
     * snprintf( buf, bufsize, "<img src=\"file://%s\"/>", fname );
     */
    ULOG_DEBUG ("processing: %s\n", fname );
    ok = gdk_pixbuf_get_file_info (fname, &w, &h) != NULL;
    if (ok) {
	ULOG_DEBUG("resolution: %dx%d\n", w, h );
    } else {
	ULOG_DEBUG ("resolution: failed\n" );
	w = h = 0;
    }

    /* Force image height to 26 pixels */
    if (h != FORCED_IMAGE_HEIGHT) {
        w /= ((float)h) / (FORCED_IMAGE_HEIGHT);
        h= FORCED_IMAGE_HEIGHT;
    }
    snprintf( buf, bufsize,
              "<img src=\"file://%s\" width=%d height=%d/>",
              fname, w, h );

    return TRUE;
}
