/* Cache of DesktopEntryTree */
/*
 * Copyright (C) 2003, 2004 Red Hat, Inc.
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

#ifndef MENU_TREE_CACHE_H
#define MENU_TREE_CACHE_H

#include "menu-process.h"

typedef struct DesktopEntryTreeCache DesktopEntryTreeCache;

DesktopEntryTreeCache* desktop_entry_tree_cache_new    (const char            *only_show_in);
void                   desktop_entry_tree_cache_ref    (DesktopEntryTreeCache *cache);
void                   desktop_entry_tree_cache_unref  (DesktopEntryTreeCache *cache);
DesktopEntryTree*      desktop_entry_tree_cache_lookup (DesktopEntryTreeCache *cache,
                                                        const char            *menu_file,
                                                        gboolean               create_user_file,
                                                        GError               **error);

gboolean               desktop_entry_tree_cache_create (DesktopEntryTreeCache *cache,
                                                        const char            *menu_file,
                                                        const char            *menu_path,
                                                        GError               **error);
gboolean               desktop_entry_tree_cache_delete (DesktopEntryTreeCache *cache,
                                                        const char            *menu_file,
                                                        const char            *menu_path,
                                                        GError               **error);
gboolean               desktop_entry_tree_cache_mkdir  (DesktopEntryTreeCache *cache,
                                                        const char            *menu_file,
                                                        const char            *menu_path,
                                                        GError               **error);
gboolean               desktop_entry_tree_cache_rmdir  (DesktopEntryTreeCache *cache,
                                                        const char            *menu_file,
                                                        const char            *menu_path,
                                                        GError               **error);

GSList*                desktop_entry_tree_cache_get_changes (DesktopEntryTreeCache *cache,
                                                             const char            *menu_file);

#endif /* MENU_TREE_CACHE_H */
