/*
 * This file is part of libosso-help
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

#include "osso-helplib-private.h"
#include "internal.h"

#include <gtk/gtk.h>

#include <png.h>    /* libpng */
#include <jpeglib.h> /* linjpeg */
#include <setjmp.h>  /* Used for libjpeg error handling */

/* Height limit for showing a pic in full size
 *
 * This should be around the size of 'normal' Help text in browser.
 */
#define NORMAL_HEIGHT   13
gboolean dialog_mode; /*internal.h*/

/*---( HelpLib portion -- HTML conversion )---*/

/**
  Get the resolution (width & height) of a .PNG picture
  
  @param fn filename
  @param wref reference to store the width
  @param href reference to store the height
  @return #TRUE for success
*/
static
gboolean png_resolution( const char *fname, guint *wref, guint *href )
{
    gboolean ok= FALSE;
    FILE *f;
    
    /* TBD: Could optimize this by keeping the 'png_p' and 'info_p'
     *      structures alive (static), but it didn't seem to work
     *      as expected.. So, for now they're made afresh each time.
     */
        
    png_structp png_p=
        png_create_read_struct( PNG_LIBPNG_VER_STRING, 
                                NULL, NULL, NULL );
    if (!png_p) {       
        ULOG_DEBUG ("libPNG error: Unable to init 'png_p'!\n" );
        return FALSE;
    }

    png_infop info_p= png_create_info_struct( png_p );
    if (!info_p) {
        png_destroy_read_struct( &png_p, NULL, NULL );
        ULOG_DEBUG ("libPNG error: Unable to init 'info_p'!\n" );
        return FALSE;
    }

    /* libPNG set up, let's see the file */
    f= fopen( fname, "rb" );
    if (f) {
        /*void*/ png_init_io( png_p, f );
        /*void*/ png_read_info( png_p, info_p );
        
        fclose(f), f=NULL;      /* we got what we wanted, thanks */
        
        if (wref) *wref= info_p->width;
        if (href) *href= info_p->height;
    
        ok= TRUE;
    }

    png_destroy_read_struct( &png_p, &info_p, NULL );

    return ok;  
}


static
void jpeg_error_exit(j_common_ptr cinfo)
{
    /* Return control to the setjmp point */
    longjmp(cinfo->client_data, 1);
}

static
gboolean jpeg_resolution( const char *fname, guint *wref, guint *href )
{
    gboolean ok= FALSE;
    FILE *f = NULL;
    jmp_buf setjmp_buffer;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    jpeg_create_decompress(&cinfo);
    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = jpeg_error_exit;
    cinfo.client_data = (void *) &setjmp_buffer;

    f= fopen( fname, "rb" );
    if (setjmp(setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error and
           we got here by the longjmp call in jpeg_error_exit */
        /* Nothing is done here because destroying of jpeg data structure
           and closing of file is doen in common code to successfull case */
    }
    else
    {
        /* This part is normally run */
        if (f) {
            jpeg_stdio_src(&cinfo, f);
            jpeg_read_header(&cinfo, TRUE);

            if (wref) *wref= cinfo.image_width;
            if (href) *href= cinfo.image_height;

            ok= TRUE;
        }
    }

    jpeg_destroy_decompress(&cinfo);
    fclose(f), f=NULL;

    return ok;  
}


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
    if (strstr( fname, ".png" )) {
        ULOG_DEBUG ("PNG processing: %s\n", fname );

        ok= png_resolution( fname, &w, &h );

        if (ok) {
          ULOG_DEBUG("PNG resolution: %dx%d\n", w, h );
        } else {
           ULOG_DEBUG ("PNG resolution: failed\n" );
        }
    }
    else if (strstr( fname, ".jpg" ) || strstr( fname, ".jpeg" )) {
        ULOG_DEBUG ("JPEG processing: %s\n", fname );

        ok= jpeg_resolution( fname, &w, &h );

        if (ok) {
          ULOG_DEBUG("JPEG resolution: %dx%d\n", w, h );
        } else {
           ULOG_DEBUG ("JPEG resolution: failed\n" );
        }
    }

    /* If we don't know the height, or it is low enough, 
       just pass on the picture. */
    if (h <= NORMAL_HEIGHT) {
        snprintf( buf, bufsize, "<img src=\"file://%s\"/>", fname );
    } else {
        /* TBD: This size reducement needs adjusting, based on needs
         *      of Help material.
         *
         * If <= 2*NORMAL_HEIGHT, reduce to normal height (maintaining
         * aspect ratio).
         *
         * If higher, reduce to 2*NORMAL_HEIGHT (maintaining aspect ratio)
         */
        if (h <= 2*NORMAL_HEIGHT) {
            w /= ((float)h) / NORMAL_HEIGHT;
            h= NORMAL_HEIGHT;
        } else {
            w /= ((float)h) / (2*NORMAL_HEIGHT);
            h= 2*NORMAL_HEIGHT;
        }

        snprintf( buf, bufsize,
                  "<img src=\"file://%s\" width=%d height=%d/>",
                  fname, w, h );
    }

    return TRUE;
}
