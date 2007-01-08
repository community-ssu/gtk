/* Menu layout in-memory data structure (a custom "DOM tree") */

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

#ifndef MENU_LAYOUT_H
#define MENU_LAYOUT_H

#include <glib.h>

typedef struct MenuNode MenuNode;
typedef struct Entry Entry;
typedef struct EntryDirectory EntryDirectory;
typedef struct EntryDirectoryList EntryDirectoryList;
typedef struct EntrySet EntrySet;
typedef struct EntryCache EntryCache;

typedef enum
{
  MENU_NODE_ROOT,
  MENU_NODE_PASSTHROUGH,
  MENU_NODE_MENU,
  MENU_NODE_APP_DIR,
  MENU_NODE_DEFAULT_APP_DIRS,
  MENU_NODE_DIRECTORY_DIR,
  MENU_NODE_DEFAULT_DIRECTORY_DIRS,
  MENU_NODE_DEFAULT_MERGE_DIRS,
  MENU_NODE_NAME,
  MENU_NODE_DIRECTORY,
  MENU_NODE_ONLY_UNALLOCATED,
  MENU_NODE_NOT_ONLY_UNALLOCATED,
  MENU_NODE_INCLUDE,
  MENU_NODE_EXCLUDE,
  MENU_NODE_FILENAME,
  MENU_NODE_CATEGORY,
  MENU_NODE_ALL,
  MENU_NODE_AND,
  MENU_NODE_OR,
  MENU_NODE_NOT,
  MENU_NODE_MERGE_FILE,
  MENU_NODE_MERGE_DIR,
  MENU_NODE_LEGACY_DIR,
  MENU_NODE_KDE_LEGACY_DIRS,
  MENU_NODE_MOVE,
  MENU_NODE_OLD,
  MENU_NODE_NEW,
  MENU_NODE_DELETED,
  MENU_NODE_NOT_DELETED,
  MENU_NODE_LAYOUT,
  MENU_NODE_DEFAULT_LAYOUT,
  MENU_NODE_MENUNAME,
  MENU_NODE_SEPARATOR,
  MENU_NODE_MERGE
} MenuNodeType;

MenuNode*    menu_node_new          (MenuNodeType type);
void         menu_node_ref          (MenuNode *node);
void         menu_node_unref        (MenuNode *node);
MenuNode*    menu_node_deep_copy    (MenuNode *node);
MenuNode*    menu_node_copy_one     (MenuNode *node);
MenuNode*    menu_node_get_next     (MenuNode *node);
MenuNode*    menu_node_get_parent   (MenuNode *node);
MenuNode*    menu_node_get_children (MenuNode *node);
MenuNode*    menu_node_get_root     (MenuNode *node);
int          menu_node_get_depth    (MenuNode *node);

void menu_node_insert_before (MenuNode *node,
                              MenuNode *new_sibling);
void menu_node_insert_after  (MenuNode *node,
                              MenuNode *new_sibling);
void menu_node_prepend_child (MenuNode *parent,
                              MenuNode *new_child);
void menu_node_append_child  (MenuNode *parent,
                              MenuNode *new_child);

void menu_node_unlink (MenuNode *node);
void menu_node_steal  (MenuNode *node);

MenuNodeType menu_node_get_type            (MenuNode *node);
const char*  menu_node_get_content         (MenuNode *node);
const char*  menu_node_get_basedir         (MenuNode *node);
const char*  menu_node_get_menu_name       (MenuNode *node);
char*        menu_node_get_content_as_path (MenuNode *node);

void         menu_node_set_content   (MenuNode   *node,
                                      const char *content);

const char* menu_node_legacy_dir_get_prefix (MenuNode   *node);
void        menu_node_legacy_dir_set_prefix (MenuNode   *node,
                                             const char *prefix);
const char* menu_node_root_get_basedir      (MenuNode   *node);
void        menu_node_root_set_basedir      (MenuNode   *node,
                                             const char *dirname);
const char* menu_node_root_get_name         (MenuNode   *node);
void        menu_node_root_set_name         (MenuNode   *node,
                                             const char *menu_name);
void        menu_node_root_set_entry_cache  (MenuNode   *node,
                                             EntryCache *entry_cache);

const char*         menu_node_menu_get_name              (MenuNode   *node);
EntryDirectoryList* menu_node_menu_get_app_entries       (MenuNode   *node);
EntryDirectoryList* menu_node_menu_get_directory_entries (MenuNode   *node);

typedef void (* MenuNodeMenuChangedFunc) (MenuNode *node,
					  gpointer  user_data);

void menu_node_menu_add_monitor    (MenuNode                *node,
				    MenuNodeMenuChangedFunc  callback,
				    gpointer                 user_data);
void menu_node_menu_remove_monitor (MenuNode                *node,
				    MenuNodeMenuChangedFunc  callback,
				    gpointer                 user_data);

const char* menu_node_move_get_old (MenuNode *node);
const char* menu_node_move_get_new (MenuNode *node);

void menu_node_debug_print (MenuNode *node);

void menu_node_remove_redundancy (MenuNode *node);

typedef struct MenuCache MenuCache;

MenuCache* menu_cache_new   (void);
void       menu_cache_ref   (MenuCache *cache);
void       menu_cache_unref (MenuCache *cache);

const char* menu_cache_get_filename_for_node (MenuCache *cache,
                                              MenuNode  *node);

void       menu_cache_invalidate (MenuCache  *cache,
                                  const char *canonical);

/* Return the pristine menu node for the file.
 * Changing this node will change what gets written
 * back to disk, if you do menu_node_sync_for_file.
 * get_for_file returns a new reference count.
 * Right now the cache never actually gets cleared.
 */
MenuNode* menu_cache_get_menu_for_file           (MenuCache   *cache,
                                                  const char  *filename,
                                                  const char  *create_chaining_to,
                                                  GError     **error);
MenuNode* menu_cache_get_menu_for_canonical_file (MenuCache   *cache,
                                                  const char  *filename,
                                                  const char  *create_chaining_to,
                                                  GError     **error);
gboolean  menu_cache_sync_for_file               (MenuCache   *cache,
                                                  const char  *filename,
                                                  GError     **error);

#endif /* MENU_LAYOUT_H */
