/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2007 Nokia Corporation.  All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
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

#ifndef HILDON_FM1_COMPAT_H
#define HILDON_FM1_COMPAT_H

/* This is a thin compatibility layer on top of libhildonfm.so.2 that
   gives you (more or less) the ABI of the old libhildonfm.so.1.  This
   works well enough since we only removed functions in
   libhildonfm.so.2 but didn't change existing functions in an
   incompatible way.

   However, this hack is only a migration helper.  It will not be
   officially supported.
*/

#include "hildon-file-selection.h"

gboolean hildon_file_selection_set_current_folder (HildonFileSelection *self,
						   const GtkFilePath *folder,
						   GError **error);

GtkFilePath * hildon_file_selection_get_current_folder (HildonFileSelection *self);

gboolean hildon_file_selection_select_path (HildonFileSelection *self,
					    const GtkFilePath *path,
					    GError **error);

void hildon_file_selection_unselect_path (HildonFileSelection *self,
					  const GtkFilePath *path);

GSList * hildon_file_selection_get_selected_paths (HildonFileSelection *self);

#endif /* !HILDON_FM1_COMPAT_H */
