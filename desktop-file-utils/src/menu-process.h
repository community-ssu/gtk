/* Tree of desktop entries */

/*
 * Copyright (C) 2002 - 2004 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MENU_PROCESS_H
#define MENU_PROCESS_H

#include <glib.h>

typedef struct DesktopEntryTree     DesktopEntryTree;
typedef struct DesktopEntryTreeNode DesktopEntryTreeNode;

typedef enum
{
  PATH_RESOLUTION_NOT_FOUND,
  PATH_RESOLUTION_IS_DIR,
  PATH_RESOLUTION_IS_ENTRY
} PathResolution;

typedef struct
{
  gboolean is_dir;
  int depth;
  char *menu_id;
  const char *menu_basename;
  const char *menu_fullpath;
  const char *menu_fullpath_localized;
  const char *filesystem_path_to_entry;
} DesktopEntryForeachInfo;

typedef gboolean (* DesktopEntryTreeForeachFunc) (DesktopEntryTree        *tree,
                                                  DesktopEntryForeachInfo *info,
                                                  void                    *data);

typedef enum
{
  DESKTOP_ENTRY_TREE_PRINT_NAME          = 1 << 0,
  DESKTOP_ENTRY_TREE_PRINT_GENERIC_NAME  = 1 << 1,
  DESKTOP_ENTRY_TREE_PRINT_COMMENT       = 1 << 2,
  DESKTOP_ENTRY_TREE_PRINT_TEST_RESULTS  = 1 << 3
} DesktopEntryTreePrintFlags;

DesktopEntryTree* desktop_entry_tree_load  (const char  *filename,
                                            const char  *only_show_in_desktop,
                                            const char  *create_chaining_to,
                                            GError     **error);
void              desktop_entry_tree_ref   (DesktopEntryTree *tree);
void              desktop_entry_tree_unref (DesktopEntryTree *tree);

/* after calling this, the tree is out-of-date vs. what's on disk */
void              desktop_entry_tree_invalidate (DesktopEntryTree *tree,
                                                 const char       *dirname);

/* These don't return references; the DesktopEntryTree is just immutable. */
gboolean desktop_entry_tree_get_node     (DesktopEntryTree       *tree,
                                          const char             *path,
                                          DesktopEntryTreeNode  **node);

PathResolution desktop_entry_tree_resolve_path (DesktopEntryTree       *tree,
                                                const char             *path,
                                                DesktopEntryTreeNode  **node,
                                                char                  **real_fs_absolute_path_p,
                                                char                  **entry_relative_name_p);
void     desktop_entry_tree_list_subdirs (DesktopEntryTree       *tree,
                                          DesktopEntryTreeNode   *parent_node,
                                          DesktopEntryTreeNode ***subdirs,
                                          int                    *n_subdirs);
void     desktop_entry_tree_list_entries (DesktopEntryTree       *tree,
                                          DesktopEntryTreeNode   *parent_node,
                                          char                 ***entries,
                                          int                    *n_entries);
void     desktop_entry_tree_list_all     (DesktopEntryTree       *tree,
                                          DesktopEntryTreeNode   *parent_node,
                                          char                 ***names,
                                          int                    *n_names,
                                          int                    *n_subdirs);
gboolean desktop_entry_tree_has_entries  (DesktopEntryTree       *tree,
                                          DesktopEntryTreeNode   *parent_node);
GTime    desktop_entry_tree_get_mtime    (DesktopEntryTree       *tree);


/* returns a copy of .directory file absolute path */
char*       desktop_entry_tree_node_get_directory (DesktopEntryTreeNode *node);
/* returns relative name of the subdir */
const char* desktop_entry_tree_node_get_name      (DesktopEntryTreeNode *node);

void desktop_entry_tree_foreach           (DesktopEntryTree            *tree,
                                           const char                  *parent_dir,
                                           DesktopEntryTreeForeachFunc  func,
                                           void                        *user_data);
void desktop_entry_tree_print             (DesktopEntryTree            *tree,
                                           DesktopEntryTreePrintFlags   flags);
void desktop_entry_tree_write_symlink_dir (DesktopEntryTree            *tree,
                                           const char                  *dirname);
void desktop_entry_tree_dump_desktop_list (DesktopEntryTree            *tree);

void menu_set_verbose_queries      (gboolean    value);

/* Editing stuff */
gboolean desktop_entry_tree_include (DesktopEntryTree *tree,
                                     const char       *menu_path_dirname,
                                     const char       *relative_entry_name,
                                     const char       *override_fs_dirname,
                                     GError          **error);
gboolean desktop_entry_tree_exclude (DesktopEntryTree *tree,
                                     const char       *menu_path_dirname,
                                     const char       *relative_entry_name,
                                     GError          **error);
gboolean desktop_entry_tree_mkdir   (DesktopEntryTree *tree,
                                     const char       *menu_path_dirname,
                                     GError          **error);
gboolean desktop_entry_tree_rmdir   (DesktopEntryTree *tree,
                                     const char       *menu_path_dirname,
                                     GError          **error);
gboolean desktop_entry_tree_move    (DesktopEntryTree *tree,
                                     const char       *menu_path_dirname_src,
                                     const char       *menu_path_dirname_dest,
                                     const char       *menu_path_basename,
                                     const char       *override_fs_dirname_dest,
                                     GError          **error);


/* Diff */

/* If a dir is replaced by a file,
 * we send DIR_DELETED and FILE_CREATED
 * (and vice versa obviously)
 */
typedef enum
{
  DESKTOP_ENTRY_TREE_DIR_CREATED,
  DESKTOP_ENTRY_TREE_DIR_DELETED,
  DESKTOP_ENTRY_TREE_DIR_CHANGED,
  DESKTOP_ENTRY_TREE_FILE_CREATED,
  DESKTOP_ENTRY_TREE_FILE_DELETED,
  DESKTOP_ENTRY_TREE_FILE_CHANGED
} DesktopEntryTreeChangeType;

typedef struct
{
  DesktopEntryTreeChangeType type;
  char *path;
} DesktopEntryTreeChange;

GSList* desktop_entry_tree_diff (DesktopEntryTree *old,
                                 DesktopEntryTree *new);

void desktop_entry_tree_change_free (DesktopEntryTreeChange *change);

/* Monitoring */
typedef void (* DesktopEntryTreeChangedFunc) (DesktopEntryTree *tree,
					      gpointer          user_data);

void desktop_entry_tree_add_monitor    (DesktopEntryTree            *tree,
					DesktopEntryTreeChangedFunc  callback,
					gpointer                     user_data);
void desktop_entry_tree_remove_monitor (DesktopEntryTree            *tree,
					DesktopEntryTreeChangedFunc  callback,
					gpointer                     user_data);

#endif /* MENU_PROCESS_H */
