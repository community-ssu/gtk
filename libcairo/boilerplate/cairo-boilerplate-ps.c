/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/*
 * Copyright © 2004,2006 Red Hat, Inc.
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

#include "cairo-boilerplate.h"
#include "cairo-boilerplate-ps.h"
#include "cairo-boilerplate-ps-private.h"

#include <cairo-ps.h>
#include <cairo-ps-surface-private.h>
#include <cairo-paginated-surface-private.h>

cairo_user_data_key_t	ps_closure_key;

typedef struct _ps_target_closure
{
    char		*filename;
    int			 width;
    int			 height;
    cairo_surface_t	*target;
} ps_target_closure_t;

cairo_surface_t *
_cairo_boilerplate_ps_create_surface (const char		 *name,
				      cairo_content_t		  content,
				      int			  width,
				      int			  height,
				      cairo_boilerplate_mode_t	  mode,
				      void			**closure)
{
    ps_target_closure_t	*ptc;
    cairo_surface_t *surface;

    /* Sanitize back to a real cairo_content_t value. */
    if (content == CAIRO_TEST_CONTENT_COLOR_ALPHA_FLATTENED)
	content = CAIRO_CONTENT_COLOR_ALPHA;

    *closure = ptc = xmalloc (sizeof (ps_target_closure_t));

    xasprintf (&ptc->filename, "%s-ps-%s-out.ps",
	       name, cairo_boilerplate_content_name (content));

    ptc->width = width;
    ptc->height = height;

    surface = cairo_ps_surface_create (ptc->filename, width, height);
    if (cairo_surface_status (surface)) {
	free (ptc->filename);
	free (ptc);
	return NULL;
    }
    cairo_surface_set_fallback_resolution (surface, 72., 72.);

    if (content == CAIRO_CONTENT_COLOR) {
	ptc->target = surface;
	surface = cairo_surface_create_similar (ptc->target,
						CAIRO_CONTENT_COLOR,
						width, height);
    } else {
	ptc->target = NULL;
    }

    if (cairo_surface_set_user_data (surface,
		                     &ps_closure_key,
				     ptc,
				     NULL) != CAIRO_STATUS_SUCCESS) {
	cairo_surface_destroy (surface);
	free (ptc->filename);
	free (ptc);
	return NULL;
    }

    return surface;
}

cairo_status_t
_cairo_boilerplate_ps_surface_write_to_png (cairo_surface_t *surface, const char *filename)
{
    ps_target_closure_t *ptc = cairo_surface_get_user_data (surface, &ps_closure_key);
    char    command[4096];

    /* Both surface and ptc->target were originally created at the
     * same dimensions. We want a 1:1 copy here, so we first clear any
     * device offset on surface.
     *
     * In a more realistic use case of device offsets, the target of
     * this copying would be of a different size than the source, and
     * the offset would be desirable during the copy operation. */
    cairo_surface_set_device_offset (surface, 0, 0);

    if (ptc->target) {
	cairo_t *cr;
	cr = cairo_create (ptc->target);
	cairo_set_source_surface (cr, surface, 0, 0);
	cairo_paint (cr);
	cairo_show_page (cr);
	cairo_destroy (cr);

	cairo_surface_finish (surface);
	surface = ptc->target;
    }

    cairo_surface_finish (surface);
    sprintf (command, "gs -q -r72 -g%dx%d -dSAFER -dBATCH -dNOPAUSE -sDEVICE=pngalpha -sOutputFile=%s %s",
	     ptc->width, ptc->height, filename, ptc->filename);
    if (system (command) == 0)
	return CAIRO_STATUS_SUCCESS;
    return CAIRO_STATUS_WRITE_ERROR;
}

void
_cairo_boilerplate_ps_cleanup (void *closure)
{
    ps_target_closure_t *ptc = closure;
    if (ptc->target)
	cairo_surface_destroy (ptc->target);
    free (ptc->filename);
    free (ptc);
}

cairo_status_t
cairo_boilerplate_ps_surface_force_fallbacks (cairo_surface_t *abstract_surface)
{
    cairo_paginated_surface_t *paginated = (cairo_paginated_surface_t*) abstract_surface;
    cairo_ps_surface_t *surface;

    if (cairo_surface_get_type (abstract_surface) != CAIRO_SURFACE_TYPE_PS)
	return CAIRO_STATUS_SURFACE_TYPE_MISMATCH;

    surface = (cairo_ps_surface_t*) paginated->target;

    surface->force_fallbacks = TRUE;

    return CAIRO_STATUS_SUCCESS;
}
