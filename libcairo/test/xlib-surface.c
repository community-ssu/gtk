/*
 * Copyright © 2005 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Carl D. Worth <cworth@cworth.org>
 */

#include <stdio.h>
#include <stdlib.h>

#include "cairo.h"
#include "cairo-xlib.h"
#include "cairo-xlib-xrender.h"
#include "cairo-test.h"

#include "cairo-boilerplate-xlib.h"

#include "buffer-diff.h"

#define SIZE 100
#define OFFSCREEN_OFFSET 50

cairo_bool_t result = 0;

static void
draw_pattern (cairo_surface_t *surface)
{
    cairo_t *cr = cairo_create (surface);
    int i;

    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); /* white */
    cairo_paint (cr);

    cairo_set_source_rgba (cr, 0, 0.0, 0.0, 0.50); /* half-alpha-black */

    for (i = 1; i <= 3; i++) {
	int inset = SIZE / 8 * i;

	cairo_rectangle (cr,
			 inset,            inset,
			 SIZE - 2 * inset, SIZE - 2 * inset);
	cairo_fill (cr);
    }

    cairo_destroy (cr);
}

static void
erase_pattern (cairo_surface_t *surface)
{
    cairo_t *cr = cairo_create (surface);

    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0); /* black */
    cairo_paint (cr);

    cairo_destroy (cr);
}

static cairo_test_status_t
do_test (Display        *dpy,
	 unsigned char  *reference_data,
	 unsigned char  *test_data,
	 unsigned char  *diff_data,
	 cairo_bool_t    use_render,
	 cairo_bool_t    use_pixmap,
	 cairo_bool_t    set_size,
	 cairo_bool_t    offscreen)
{
    cairo_surface_t *surface;
    cairo_surface_t *test_surface;
    cairo_t *test_cr;
    buffer_diff_result_t result;
    Drawable drawable;
    int screen = DefaultScreen (dpy);

    if (use_pixmap && offscreen)
	return CAIRO_TEST_SUCCESS;

    if (use_pixmap) {
	drawable = XCreatePixmap (dpy, DefaultRootWindow (dpy),
				  SIZE, SIZE, DefaultDepth (dpy, screen));
    } else {
	XSetWindowAttributes xwa;
	int x, y;

	xwa.override_redirect = True;

	if (offscreen) {
	    x = - OFFSCREEN_OFFSET;
	    y = - OFFSCREEN_OFFSET;
	} else {
	    x = 0;
	    y = 0;
	}

	drawable = XCreateWindow (dpy, DefaultRootWindow (dpy),
				  x, y, SIZE, SIZE, 0,
				  DefaultDepth (dpy, screen), InputOutput,
				  DefaultVisual (dpy, screen),
				  CWOverrideRedirect, &xwa);
	XMapWindow (dpy, drawable);
    }

    surface = cairo_xlib_surface_create (dpy,
					 drawable,
					 DefaultVisual (dpy, screen),
					 SIZE, SIZE);

    if (!use_render)
	cairo_boilerplate_xlib_surface_disable_render (surface);

    if (set_size) {
	cairo_xlib_surface_set_size (surface, SIZE, SIZE);

	if (cairo_xlib_surface_get_width (surface) != SIZE ||
	    cairo_xlib_surface_get_height (surface) != SIZE)
	    return CAIRO_TEST_FAILURE;
    }

    draw_pattern (surface);

    test_surface = cairo_image_surface_create_for_data (test_data,
							CAIRO_FORMAT_RGB24,
							SIZE, SIZE,
							SIZE * 4);

    test_cr = cairo_create (test_surface);
    cairo_set_source_surface (test_cr, surface, 0, 0);
    cairo_paint (test_cr);

    cairo_destroy (test_cr);
    cairo_surface_destroy (test_surface);

    /* We erase the surface to black in case we get the same
     * memory back again for the pixmap case.
     */
    erase_pattern (surface);
    cairo_surface_destroy (surface);

    if (use_pixmap)
	XFreePixmap (dpy, drawable);
    else
	XDestroyWindow (dpy, drawable);

    if (offscreen) {
	size_t offset = 4 * (SIZE * OFFSCREEN_OFFSET + OFFSCREEN_OFFSET);

	buffer_diff_noalpha (reference_data + offset,
			     test_data + offset,
			     diff_data + offset,
			     SIZE - OFFSCREEN_OFFSET,
			     SIZE - OFFSCREEN_OFFSET,
			     4 * SIZE,
			     &result);
    } else {
	buffer_diff_noalpha (reference_data,
			     test_data,
			     diff_data,
			     SIZE,
			     SIZE,
			     4 * SIZE,
			     &result);
    }

    cairo_test_log ("xlib-surface: %s, %s, %s%s: %s\n",
		    use_render ? "   render" : "no-render",
		    set_size ? "   size" : "no-size",
		    use_pixmap ? "pixmap" : "window",
		    use_pixmap ?
		    "           " :
		    (offscreen ? ", offscreen" : ",  onscreen"),
		    result.pixels_changed ? "FAIL" : "PASS");

    if (result.pixels_changed)
	return CAIRO_TEST_FAILURE;
    else
	return CAIRO_TEST_SUCCESS;
}

static cairo_bool_t
check_visual (Display *dpy)
{
    Visual *visual = DefaultVisual (dpy, DefaultScreen (dpy));

    if ((visual->red_mask   == 0xff0000 &&
	 visual->green_mask == 0x00ff00 &&
	 visual->blue_mask  == 0x0000ff) ||
	(visual->red_mask   == 0x0000ff &&
	 visual->green_mask == 0x00ff00 &&
	 visual->blue_mask  == 0xff0000))
	return 1;
    else
	return 0;
}

int
main (void)
{
    Display *dpy;
    unsigned char *reference_data;
    unsigned char *test_data;
    unsigned char *diff_data;
    cairo_surface_t *reference_surface;
    cairo_bool_t use_pixmap;
    cairo_bool_t set_size;
    cairo_bool_t offscreen;
    cairo_test_status_t status, result = CAIRO_TEST_SUCCESS;

    cairo_test_init ("xlib-surface");

    dpy = XOpenDisplay (NULL);
    if (!dpy) {
	cairo_test_log ("xlib-surface: Cannot open display, skipping\n");
	cairo_test_fini ();
	return 0;
    }

    if (!check_visual (dpy)) {
	cairo_test_log ("xlib-surface: default visual is not RGB24 or BGR24, skipping\n");
	cairo_test_fini ();
	return 0;
    }

    reference_data = malloc (SIZE * SIZE * 4);
    test_data = malloc (SIZE * SIZE * 4);
    diff_data = malloc (SIZE * SIZE * 4);

    reference_surface = cairo_image_surface_create_for_data (reference_data,
							     CAIRO_FORMAT_RGB24,
							     SIZE, SIZE,
							     SIZE * 4);

    draw_pattern (reference_surface);
    cairo_surface_destroy (reference_surface);

    for (set_size = 0; set_size <= 1; set_size++)
	for (use_pixmap = 0; use_pixmap <= 1; use_pixmap++)
	    for (offscreen = 0; offscreen <= 1; offscreen++) {
		status = do_test (dpy,
				  reference_data, test_data, diff_data,
				  1, use_pixmap, set_size, offscreen);
		if (status)
		    result = status;
	    }

    for (set_size = 0; set_size <= 1; set_size++)
	for (use_pixmap = 0; use_pixmap <= 1; use_pixmap++)
	    for (offscreen = 0; offscreen <= 1; offscreen++) {
		status = do_test (dpy,
				  reference_data, test_data, diff_data,
				  0, use_pixmap, set_size, offscreen);
		if (status)
		    result = status;
	    }

    free (reference_data);
    free (test_data);
    free (diff_data);

    XCloseDisplay (dpy);

    cairo_debug_reset_static_data ();

    cairo_test_fini ();

    return result;
}
