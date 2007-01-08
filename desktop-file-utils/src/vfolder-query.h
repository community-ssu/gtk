/* Vfolder query */

/*
 * Copyright (C) 2002 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef VFOLDER_QUERY_H
#define VFOLDER_QUERY_H

#include <glib.h>
#include "vfolder-parser.h"
#include "desktop_file.h"

typedef struct _DesktopFileTree DesktopFileTree;

typedef enum
{
  DESKTOP_FILE_TREE_PRINT_NAME          = 1 << 0,
  DESKTOP_FILE_TREE_PRINT_GENERIC_NAME  = 1 << 1,
  DESKTOP_FILE_TREE_PRINT_COMMENT       = 1 << 2
} DesktopFileTreePrintFlags;

DesktopFileTree* desktop_file_tree_new   (Vfolder                   *folder);
void             desktop_file_tree_free  (DesktopFileTree           *tree);
void             desktop_file_tree_print (DesktopFileTree           *tree,
                                          DesktopFileTreePrintFlags  flags);
void             desktop_file_tree_write_symlink_dir (DesktopFileTree *tree,
                                                      const char      *dirname);
void             desktop_file_tree_dump_desktop_list (DesktopFileTree *tree);

void vfolder_set_verbose_queries      (gboolean    value);
void vfolder_set_only_show_in_desktop (const char *desktop_name);


#endif
