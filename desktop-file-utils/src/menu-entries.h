/* Desktop and directory entries */

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

#ifndef MENU_ENTRIES_H
#define MENU_ENTRIES_H

#include <glib.h>
#include "menu-layout.h"

/* This API is about actually loading directories full of .desktop
 * files and the .desktop/.directory files themselves. It also has
 * EntrySet for manipulating sets of these files.
 */

typedef enum
{
  ENTRY_ERROR_BAD_PATH,
  ENTRY_ERROR_FAILED
} EntryError;

GQuark entry_error_quark (void);

#define ENTRY_ERROR entry_error_quark ()

typedef enum
{
  ENTRY_DESKTOP,
  ENTRY_DIRECTORY
} EntryType;

typedef enum
{
  ENTRY_LOAD_LEGACY      = 1 << 0,
  ENTRY_LOAD_DESKTOPS    = 1 << 1,
  ENTRY_LOAD_DIRECTORIES = 1 << 2
} EntryLoadFlags;

typedef void (*EntryDirectoryChangedFunc) (EntryDirectory *ed,
					   gpointer        user_data);

/* The EntryCache is sort of a global context containing all cached data etc. */
EntryCache *entry_cache_new                   (void);
void        entry_cache_ref                   (EntryCache *cache);
void        entry_cache_unref                 (EntryCache *cache);
void        entry_cache_set_only_show_in_name (EntryCache *cache,
                                               const char *name);
void        entry_cache_invalidate            (EntryCache *cache,
                                               const char *dir);

/* returns a new ref, never has Legacy category, relative path is entry basename */
Entry* entry_get_by_absolute_path (EntryCache *cache,
                                   const char *path);

EntryDirectory* entry_directory_load  (EntryCache     *cache,
                                       const char     *path,
                                       EntryLoadFlags  flags,
                                       GError        **err);
void            entry_directory_ref   (EntryDirectory *dir);
void            entry_directory_unref (EntryDirectory *dir);

/* return a new ref */
Entry* entry_directory_get_desktop   (EntryDirectory *dir,
                                      const char     *relative_path);
Entry* entry_directory_get_directory (EntryDirectory *dir,
                                      const char     *relative_path);

void entry_directory_get_all_desktops (EntryDirectory *dir,
                                       EntrySet       *add_to_set);
void entry_directory_get_by_category  (EntryDirectory *dir,
                                       const char     *category,
                                       EntrySet       *add_to_set);

void entry_directory_add_monitor    (EntryDirectory            *dir,
				     EntryDirectoryChangedFunc  callback,
				     gpointer                   user_data);
void entry_directory_remove_monitor (EntryDirectory            *dir,
				     EntryDirectoryChangedFunc  callback,
				     gpointer                   user_data);

EntryDirectoryList* entry_directory_list_new (void);

void entry_directory_list_ref     (EntryDirectoryList *list);
void entry_directory_list_unref   (EntryDirectoryList *list);
void entry_directory_list_clear   (EntryDirectoryList *list);
/* prepended dirs are searched first */
void entry_directory_list_prepend     (EntryDirectoryList *list,
                                       EntryDirectory     *dir);
void entry_directory_list_append      (EntryDirectoryList *list,
                                       EntryDirectory     *dir);
int  entry_directory_list_get_length  (EntryDirectoryList *list);
void entry_directory_list_append_list (EntryDirectoryList *list,
                                       EntryDirectoryList *to_append);

void entry_directory_list_add_monitors    (EntryDirectoryList        *list,
					   EntryDirectoryChangedFunc  callback,
					   gpointer                   user_data);
void entry_directory_list_remove_monitors (EntryDirectoryList        *list,
					   EntryDirectoryChangedFunc  callback,
					   gpointer                   user_data);

/* return a new ref */
Entry* entry_directory_list_get_desktop   (EntryDirectoryList *list,
                                           const char         *relative_path);
Entry* entry_directory_list_get_directory (EntryDirectoryList *list,
                                           const char         *relative_path);

void entry_directory_list_get_all_desktops (EntryDirectoryList *list,
                                            EntrySet           *set);
void entry_directory_list_get_by_category  (EntryDirectoryList *list,
                                            const char         *category,
                                            EntrySet           *set);

void entry_directory_list_invert_set       (EntryDirectoryList *list,
                                            EntrySet           *set);

void entry_ref   (Entry *entry);
void entry_unref (Entry *entry);

const char* entry_get_absolute_path (Entry      *entry);
const char* entry_get_relative_path (Entry      *entry);
const char* entry_get_name          (Entry      *entry);
gboolean    entry_get_nodisplay     (Entry      *entry);
gboolean    entry_has_category      (Entry      *entry,
		                     EntryCache *cache,
				     const char *category);


EntrySet* entry_set_new          (void);
void      entry_set_ref          (EntrySet *set);
void      entry_set_unref        (EntrySet *set);
void      entry_set_add_entry    (EntrySet *set,
                                  Entry    *entry);
void      entry_set_remove_entry (EntrySet *set,
                                  Entry    *entry);
Entry*    entry_set_lookup       (EntrySet   *set,
                                  const char *relative_path);
void      entry_set_clear        (EntrySet *set);
/* returns a ref to each entry */
GSList*   entry_set_list_entries (EntrySet *set);
int       entry_set_get_count    (EntrySet *set);

void      entry_set_union        (EntrySet *set,
                                  EntrySet *with);
void      entry_set_intersection (EntrySet *set,
                                  EntrySet *with);
void      entry_set_subtract     (EntrySet *set,
                                  EntrySet *other);
void      entry_set_swap_contents (EntrySet *a,
                                   EntrySet *b);

#endif /* MENU_ENTRIES_H */
