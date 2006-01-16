/**
 *  gtkfilesystemprivate.h
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
 *
 * Only reason for this .h file is the variable file_path_to_be_deleted.
 * Have to be available in the gtkfilesystemmemorypublic.c
 * 
 */

#ifndef __GTK_FILE_SYSTEM_MEMORY_PRIVATE_H__
#define __GTK_FILE_SYSTEM_MEMORY_PRIVATE_H__

G_BEGIN_DECLS


#include <gtk/gtktreestore.h>

#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#undef __GNUC__

#include <gtk/gtkfilesystem.h>

#define GTK_TYPE_FILE_SYSTEM_MEMORY (gtk_file_system_memory_get_type ())
#define GTK_FILE_SYSTEM_MEMORY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FILE_SYSTEM_MEMORY, GtkFileSystemMemory))
#define GTK_IS_FILE_SYSTEM_MEMORY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FILE_SYSTEM_MEMORY))

GtkFileSystem *gtk_file_system_memory_new       (void);
GType          gtk_file_system_memory_get_type (void);


struct _GtkFileSystemMemory
{
  GtkTreeStore parent_instance;
  GSList *file_paths_to_be_deleted;
  GtkTreePath *parent_path;
  GSList *bookmarks;
};


G_END_DECLS

#endif /*__GTK_FILE_SYSTEM_MEMORY_PRIVATE_H__*/
