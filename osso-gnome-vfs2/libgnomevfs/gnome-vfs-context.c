/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-context.c - context VFS modules can use to communicate with gnome-vfs proper

   Copyright (C) 1999 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Havoc Pennington <hp@redhat.com> */

#include <config.h>
#include "gnome-vfs-context.h"

#include "gnome-vfs-backend.h"
#include "gnome-vfs-cancellation.h"
#include "gnome-vfs-private-utils.h"
#include "gnome-vfs-utils.h"
#include <stdio.h>

#if 1
#define DEBUG_MSG (x) printf x
#else
#define DEBUG_MSG (x)
#endif


struct GnomeVFSContext {
        GnomeVFSCancellation *cancellation;
};

/* This is a token Context to return in situations
 * where we don't normally have a context: eg, during sync calls
 */
static const GnomeVFSContext sync_context = {NULL};

/**
 * gnome_vfs_context_new:
 * 
 * Creates a new context and cancellation object. Must be called
 * from the main glib event loop.
 *
 * Return value: a newly allocated #GnomeVFSContext.
 */
GnomeVFSContext*
gnome_vfs_context_new (void)
{
        GnomeVFSContext *ctx;

        ctx = g_new0(GnomeVFSContext, 1);

        ctx->cancellation = gnome_vfs_cancellation_new();
 
        return ctx;
}

/**
 * gnome_vfs_context_free:
 * @ctx: context to be freed.
 *
 * Free @ctx and destroy the associated #GnomeVFSCancellation.
 */
void
gnome_vfs_context_free (GnomeVFSContext *ctx)
{
        g_return_if_fail(ctx != NULL);
  
	gnome_vfs_cancellation_destroy(ctx->cancellation);
	
	g_free(ctx);
}

/**
 * gnome_vfs_context_get_cancellation:
 * @ctx: context to get the #GnomeVFSCancellation from.
 *
 * Retrieve the #GnomeVFSCancellation associated with @ctx.
 *
 * Return value: @ctx 's #GnomeVFSCancellation.
 */
GnomeVFSCancellation*
gnome_vfs_context_get_cancellation (const GnomeVFSContext *ctx)
{
        g_return_val_if_fail(ctx != NULL, NULL);
        return ctx->cancellation;
}

/**
 * gnome_vfs_context_peek_current:
 *
 * Get the currently active context. It shouldn't be
 * manipulated but can be compared to contexts module
 * holds to determine whether they are active.
 *
 * Return value: the currently active #GnomeVFSContext.
 */
const GnomeVFSContext *
gnome_vfs_context_peek_current		  (void)
{
	const GnomeVFSContext *ret;
	
	_gnome_vfs_get_current_context ((GnomeVFSContext **)&ret);

	/* If the context is NULL, then this must be a synchronous call */
	if (ret == NULL) {
		ret = &sync_context;
	}

	return ret;
}

/**
 * gnome_vfs_context_check_cancellation_current:
 * 
 * Check to see if the currently active context has been cancelled.
 *
 * Return value: %TRUE if the currently active context has been cancelled, otherwise %FALSE.
 */
gboolean
gnome_vfs_context_check_cancellation_current (void)
{
	const GnomeVFSContext *current_ctx;

	current_ctx = gnome_vfs_context_peek_current ();

	if (current_ctx == &sync_context) {
		return FALSE;
	} else if (current_ctx != NULL) {
		return gnome_vfs_cancellation_check (gnome_vfs_context_get_cancellation (current_ctx));
	} else {
		return FALSE;
	}	
}
