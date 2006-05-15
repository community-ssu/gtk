/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
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
 * hildon-file-system-private.h
 *
 * Functions in this package are internal helpers of the
 * HildonFileSystem and should not be called by
 * applications.
 */

#ifndef __HILDON_FILE_SYSTEM_PRIVATE_H__
#define __HILDON_FILE_SYSTEM_PRIVATE_H__

#include <gtk/gtkicontheme.h>
#include "hildon-file-system-common.h"

G_BEGIN_DECLS

#define TREE_ICON_SIZE 26 /* Left side icons */

gboolean 
_hildon_file_system_compare_ignore_last_separator(const char *a, const char *b);

void _hildon_file_system_ensure_locations(void);

gint _hildon_file_system_get_special_location(GtkFileSystem *fs, const GtkFilePath *path);

GdkPixbuf *_hildon_file_system_create_image(GtkFileSystem *fs, 
      GtkWidget *ref_widget, GtkFilePath *path, 
      HildonFileSystemModelItemType type, gint size, gboolean mmc2);

gchar *_hildon_file_system_create_file_name(GtkFileSystem *fs, 
  const GtkFilePath *path, HildonFileSystemModelItemType type,  
  GtkFileInfo *info, gboolean mmc2);

gchar *_hildon_file_system_create_display_name(GtkFileSystem *fs, 
  const GtkFilePath *path, HildonFileSystemModelItemType type,  
  GtkFileInfo *info, gboolean mmc2);

GtkFilePath *_hildon_file_system_path_for_location(GtkFileSystem *fs, 
  HildonFileSystemModelItemType type, gboolean mmc2);

/* You can omit either type or base */
GtkFileSystemVolume *
_hildon_file_system_get_volume_for_location(GtkFileSystem *fs, 
  HildonFileSystemModelItemType type,
  const GtkFilePath *base, gboolean mmc2);

gchar *_hildon_file_system_search_extension(gchar *name, const gchar *mime);
long _hildon_file_system_parse_autonumber(const char *start);
void _hildon_file_system_remove_autonumber(char *name);

GdkPixbuf *_hildon_file_system_load_icon_cached(GtkIconTheme *theme, 
  const gchar *name, gint size);

G_END_DECLS

#endif
