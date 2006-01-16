/* gtkfilesystem.h
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
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
