/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005-2006 Nokia Corporation.  All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
/*
  hildon-file-system-info.h

  New API for querying info about files.
*/

#ifndef __HILDON_FILE_SYSTEM_INFO_H__
#define __HILDON_FILE_SYSTEM_INFO_H__

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

typedef struct _HildonFileSystemInfoHandle HildonFileSystemInfoHandle;
typedef struct _HildonFileSystemInfo HildonFileSystemInfo;

typedef void (*HildonFileSystemInfoCallback) (HildonFileSystemInfoHandle *handle,
                                              HildonFileSystemInfo *info,
                                              const GError *error, gpointer data);

HildonFileSystemInfoHandle *hildon_file_system_info_async_new(const gchar *uri,
    HildonFileSystemInfoCallback callback, gpointer data);
const gchar *hildon_file_system_info_get_display_name(HildonFileSystemInfo *info);
GdkPixbuf *hildon_file_system_info_get_icon(HildonFileSystemInfo *info, GtkWidget *ref_widget);
GdkPixbuf *hildon_file_system_info_get_icon_at_size(HildonFileSystemInfo *info, 
						    GtkWidget *ref_widget,
						    gint size);
void hildon_file_system_info_async_cancel(HildonFileSystemInfoHandle *handle);

#ifndef HILDON_DISABLE_DEPRECATED
HildonFileSystemInfo *hildon_file_system_info_new(const gchar *uri, GError **error);
void hildon_file_system_info_free(HildonFileSystemInfo *info);
#endif

G_END_DECLS

#endif
