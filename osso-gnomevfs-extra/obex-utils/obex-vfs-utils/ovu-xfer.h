/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __OVU_XFER_H__
#define __OVU_XFER_H__

#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

/* This function is a wrapper around gnome_vfs_async_xfer and works exactly like
 * it, but makes it works for OBEX sources/targets as well. Note that this
 * function now only directly calls gnome_vfs_async_xfer, so there is no need to
 * use this function.
 */
GnomeVFSResult ovu_async_xfer (GnomeVFSAsyncHandle               **handle_return,
			       GList                              *source_uri_list,
			       GList                              *target_uri_list,
			       GnomeVFSXferOptions                 xfer_options,
			       GnomeVFSXferErrorMode               error_mode,
			       GnomeVFSXferOverwriteMode           overwrite_mode,
			       int				   priority,
			       GnomeVFSAsyncXferProgressCallback   progress_update_callback,
			       gpointer                            update_callback_data,
			       GnomeVFSXferProgressCallback        progress_sync_callback,
			       gpointer                            sync_callback_data);

#endif /* __OVU_XFER_H__ */
