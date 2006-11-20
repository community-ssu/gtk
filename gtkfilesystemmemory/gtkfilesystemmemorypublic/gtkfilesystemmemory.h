/* gtkfilesystem.h
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __GTK_FILE_SYSTEM_MEMORY_H__
#define __GTK_FILE_SYSTEM_MEMORY_H__

#include <glib-object.h>
#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#include <gtk/gtkfilesystem.h>
#undef GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED

G_BEGIN_DECLS


enum
{
  GTK_FILE_SYSTEM_MEMORY_COLUMN_ICON,
  GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME,
  GTK_FILE_SYSTEM_MEMORY_COLUMN_MIME,
  GTK_FILE_SYSTEM_MEMORY_COLUMN_MOD_TIME,
  GTK_FILE_SYSTEM_MEMORY_COLUMN_SIZE,
  GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_HIDDEN,
  GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER
};

typedef struct _GtkFileSystemMemory GtkFileSystemMemory;


GtkTreePath *
gtk_file_system_memory_file_path_to_tree_path( GtkFileSystem *file_system,
                                               const GtkFilePath *path );

gboolean
gtk_file_system_memory_file_path_to_tree_iter( GtkFileSystem *file_system,
                                               GtkTreeIter *iter, const GtkFilePath *path );

GtkFilePath *
gtk_file_system_memory_tree_path_to_file_path( GtkFileSystem *file_system,
                                               GtkTreePath *path );

GtkFilePath *
gtk_file_system_memory_tree_iter_to_file_path( GtkFileSystem *file_system,
                                               GtkTreeIter *iter );

gboolean gtk_file_system_memory_remove( GtkFileSystem *file_system,
                                        GtkTreePath *path );


G_END_DECLS

#endif /* __GTK_FILE_SYSTEM_MEMORY_H__ */
