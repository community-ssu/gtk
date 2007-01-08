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

#include "menu-tree-cache.h"
#include "menu-process.h"
#include "menu-layout.h"
#include "menu-overrides.h"
#include "canonicalize.h"
#include <glib.h>
#include <string.h>
#include <errno.h>

#include <libintl.h>
#define _(x) gettext ((x))
#define N_(x) x

/* We need to keep track of what absolute path we're using for each
 * menu basename, and then cache the loaded tree for each absolute
 * path.
 */
typedef struct
{
  char             *canonical_path;
  char             *create_chaining_to;
  DesktopEntryTree *tree;
  GError           *load_failure_reason;
  MenuOverrideDir  *overrides;
  GSList           *changes;
  unsigned int      needs_reload : 1;
} CacheEntry;

struct DesktopEntryTreeCache
{
  int refcount;

  char       *only_show_in;
  GHashTable *entries;
  GHashTable *basename_to_canonical;
};

static void
free_cache_entry (void *data)
{
  CacheEntry *entry = data;

  g_free (entry->canonical_path);
  g_free (entry->create_chaining_to);
  if (entry->tree)
    desktop_entry_tree_unref (entry->tree);
  if (entry->load_failure_reason)
    g_error_free (entry->load_failure_reason);

  g_slist_foreach (entry->changes,
                   (GFunc) desktop_entry_tree_change_free,
                   NULL);
  g_slist_free (entry->changes);
  
#if 1
  memset (entry, 0xff, sizeof (*entry));
#endif
  
  g_free (entry);
}

DesktopEntryTreeCache*
desktop_entry_tree_cache_new (const char *only_show_in)
{
  DesktopEntryTreeCache *cache;

  cache = g_new0 (DesktopEntryTreeCache, 1);

  cache->refcount = 1;

  cache->only_show_in = g_strdup (only_show_in);
  cache->entries = g_hash_table_new_full (g_str_hash,
                                          g_str_equal,
                                          NULL,
                                          free_cache_entry);
  cache->basename_to_canonical =
    g_hash_table_new_full (g_str_hash,
                           g_str_equal,
                           g_free, g_free);
  
  return cache;
}

void
desktop_entry_tree_cache_ref (DesktopEntryTreeCache *cache)
{
  g_return_if_fail (cache != NULL);
  
  cache->refcount += 1;
}

void
desktop_entry_tree_cache_unref (DesktopEntryTreeCache *cache)
{
  g_return_if_fail (cache != NULL);
  cache->refcount -= 1;
  if (cache->refcount == 0)
    {
      g_free (cache->only_show_in);
      g_hash_table_destroy (cache->entries);
      g_hash_table_destroy (cache->basename_to_canonical);
      
      g_free (cache);
    }
}

static void
handle_desktop_entry_tree_changed (DesktopEntryTree *tree,
				   CacheEntry       *entry)
{
  entry->needs_reload = TRUE;
}

static gboolean
reload_entry (DesktopEntryTreeCache *cache,
              CacheEntry            *entry,
              GError               **error)
{
  if (entry->needs_reload)
    {
      DesktopEntryTree *reloaded;
      GError *tmp_error;

      menu_verbose ("Reloading cache entry\n");
      
      tmp_error = NULL;
      reloaded = desktop_entry_tree_load (entry->canonical_path,
                                          cache->only_show_in,
                                          entry->create_chaining_to,
                                          &tmp_error);

      if (entry->tree)
        {
          if (reloaded != NULL)
            {
              GSList *changes;
              
              changes = desktop_entry_tree_diff (entry->tree,
                                                 reloaded);
              entry->changes = g_slist_concat (entry->changes, changes);
            }
          
	  desktop_entry_tree_remove_monitor (entry->tree,
					     (DesktopEntryTreeChangedFunc) handle_desktop_entry_tree_changed,
					     entry);
          desktop_entry_tree_unref (entry->tree);
        }
      
      if (entry->load_failure_reason)
        g_error_free (entry->load_failure_reason);
      
      entry->load_failure_reason = tmp_error;
      entry->tree = reloaded;

      desktop_entry_tree_add_monitor (entry->tree,
				      (DesktopEntryTreeChangedFunc) handle_desktop_entry_tree_changed,
				      entry);

      entry->needs_reload = FALSE;
    }
  
  if (entry->tree == NULL)
    {
      g_assert (entry->load_failure_reason != NULL);
      
      menu_verbose ("Load failure cached, reason for failure: %s\n",
                    entry->load_failure_reason->message);
      
      if (error)
        *error = g_error_copy (entry->load_failure_reason);
      return FALSE;
    }
  else
    return TRUE;
}

static CacheEntry*
lookup_canonical_entry (DesktopEntryTreeCache *cache,
                        const char            *canonical,
                        const char            *create_chaining_to,
                        GError               **error)
{
  CacheEntry *entry;

  menu_verbose ("Looking up canonical filename in tree cache: \"%s\"\n",
                canonical);
  
  entry = g_hash_table_lookup (cache->entries, canonical);
  if (entry == NULL)
    {
      char *basename;
      
      entry = g_new0 (CacheEntry, 1);
      entry->canonical_path = g_strdup (canonical);
      entry->create_chaining_to = g_strdup (create_chaining_to);

      entry->needs_reload = TRUE;
      
      g_hash_table_replace (cache->entries, entry->canonical_path,
                            entry);
      basename = g_path_get_basename (entry->canonical_path);
      g_hash_table_replace (cache->basename_to_canonical,
                            basename, g_strdup (entry->canonical_path));
    }

  g_assert (entry != NULL);

  return entry;
}

static CacheEntry*
lookup_absolute_entry (DesktopEntryTreeCache *cache,
                       const char            *absolute,
                       const char            *create_chaining_to,
                       GError               **error)
{
  CacheEntry *entry;
  char *canonical;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  
  menu_verbose ("Looking up absolute filename in tree cache: \"%s\"\n",
                absolute);
  
  /* First just guess the absolute is already canonical and see if
   * it's in the cache, to avoid the canonicalization. Don't
   * want to cache the failed lookup so check the hash table
   * directly instead of doing lookup_canonical() first.
   */
  if (g_hash_table_lookup (cache->entries, absolute) != NULL)
    {
      entry = lookup_canonical_entry (cache, absolute, create_chaining_to, error);
      if (entry != NULL)
        return entry;
    }

  /* Now really canonicalize it and try again */
  canonical = g_canonicalize_file_name (absolute, TRUE);
  if (canonical == NULL)
    {
      g_set_error (error, G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   _("Could not find or canonicalize the file \"%s\"\n"),
                   absolute);
      menu_verbose ("Failed to canonicalize: \"%s\": %s\n",
                    absolute, g_strerror (errno));
      return NULL;
    }
  
  entry = lookup_canonical_entry (cache, canonical, create_chaining_to, error);
  g_free (canonical);
  return entry;
}

/* FIXME this cache has the usual cache problem - when to expire it?
 * Right now I think "never" is probably a good enough answer, but at
 * some point we might want to re-evaluate.
 */
static CacheEntry*
cache_lookup (DesktopEntryTreeCache *cache,
              const char            *menu_file,
              gboolean               create_user_file,
              GError               **error)
{
  CacheEntry *retval;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  
  retval = NULL;
  
  /* menu_file may be absolute, or if relative should be searched
   * for in the xdg dirs.
   */
  if (g_path_is_absolute (menu_file))
    {
      retval = lookup_absolute_entry (cache, menu_file, NULL, error);
    }
  else
    {
      XdgPathInfo info;
      CacheEntry *entry;
      int i;
      const char *canonical;

      /* Try to avoid the path search */
      canonical = g_hash_table_lookup (cache->basename_to_canonical,
                                       menu_file);
      if (canonical != NULL)
        {
          retval = lookup_canonical_entry (cache, canonical, NULL, error);
          goto out;
        }

      /* Now look for the file in the path */
      init_xdg_paths (&info);
      
      i = 0;
      while (info.config_dirs[i] != NULL)
        {
          char *absolute;
          char *chain_to;

          absolute = g_build_filename (info.config_dirs[i],
                                       "menus", menu_file, NULL);

          if (i == 0 && create_user_file)
            {
              char *dirname;
              
              chain_to = g_build_filename (info.first_system_config,
                                           "menus", menu_file, NULL);
              
              /* Create directory for the user menu file */
              dirname = g_build_filename (info.config_dirs[i], "menus",
                                          NULL);

              menu_verbose ("Will chain to \"%s\" from user file \"%s\" in directory \"%s\"\n",
                            chain_to, absolute, dirname);
              
              /* ignore errors, if it's a problem stuff will fail
               * later.
               */
              g_create_dir (dirname, 0755, NULL);
              g_free (dirname);
            }
          else
            chain_to = NULL;
          
          entry = lookup_absolute_entry (cache, absolute, chain_to, NULL);
          g_free (absolute);
          g_free (chain_to);
          
          if (entry != NULL)
            {
              /* in case an earlier lookup piled up */
              menu_verbose ("Successfully got entry %p\n", entry);
              retval = entry;
              goto out;
            }
          
          ++i;
        }

      /* We didn't find anything */
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_EXIST,
                   _("Did not find file \"%s\" in search path"),
                   menu_file);
    }

 out:

  /* Fail if we can't reload the entry */
  if (retval && !reload_entry (cache, retval, error))
    retval = NULL;
  
  return retval;
}

DesktopEntryTree*
desktop_entry_tree_cache_lookup (DesktopEntryTreeCache *cache,
                                 const char            *menu_file,
                                 gboolean               create_user_file,
                                 GError               **error)
{
  CacheEntry *entry;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  
  entry = cache_lookup (cache, menu_file, create_user_file,
                        error);
  
  if (entry)
    {
      desktop_entry_tree_ref (entry->tree);
      return entry->tree;
    }
  else
    return NULL;
}

static void
try_create_overrides (CacheEntry *entry,
                      const char *menu_file,                      GError    **error)
{
  if (entry->overrides == NULL)
    {
      char *d;
      GString *menu_type;
      XdgPathInfo info;

      init_xdg_paths (&info);

      menu_type = g_string_new (menu_file);
      g_string_truncate (menu_type, menu_type->len - strlen (".menu"));
      g_string_append (menu_type, "-edits");
      
      d = g_build_filename (info.config_home, "menus",
                            menu_type->str, NULL);
      
      entry->overrides = menu_override_dir_create (d, error);

      g_string_free (menu_type, TRUE);
      g_free (d);
    }
}

/* For a menu_file like "applications.menu" override a menu_path entry
 * like "Applications/Games/foo.desktop" by creating the appropriate
 * .desktop file and adding an <Include> and <AppDir>.  If the file is
 * already created in the override dir, do nothing.
 */
gboolean
desktop_entry_tree_cache_create (DesktopEntryTreeCache *cache,
                                 const char            *menu_file,
                                 const char            *menu_path,
                                 GError               **error)
{
  CacheEntry *entry;
  DesktopEntryTree *tree;
  char *current_fs_path;
  gboolean retval;
  char *menu_path_dirname;
  char *menu_entry_relative_path;
  char *override_dir;
  PathResolution res;
  
  menu_verbose ("Creating \"%s\" in menu %s\n",
                menu_path, menu_file);
  
  entry = cache_lookup (cache, menu_file, TRUE, error);

  if (entry == NULL)
    return FALSE;

  try_create_overrides (entry, menu_file, error);
  
  if (entry->overrides == NULL)
    return FALSE;

  tree = desktop_entry_tree_cache_lookup (cache, menu_file,
                                          TRUE, error);
  if (tree == NULL)
    return FALSE;

  retval = FALSE;
  override_dir = NULL;

  menu_path_dirname = NULL;
  current_fs_path = NULL;
  menu_entry_relative_path = NULL;
  res = desktop_entry_tree_resolve_path (tree, menu_path,
                                         NULL, &current_fs_path,
                                         &menu_entry_relative_path);
  if (res == PATH_RESOLUTION_IS_DIR)
    {
      g_set_error (error, G_FILE_ERROR,
                   G_FILE_ERROR_ISDIR,
                   _("%s is a directory\n"), menu_path);
      goto out;
    }

  /* if there's no existing .desktop file we're based on,
   * we use the simplest possible basename
   */
  if (menu_entry_relative_path == NULL)
    {
      menu_entry_relative_path = g_path_get_basename (menu_path);
      
      menu_verbose ("Using new entry name \"%s\"\n",
                    menu_entry_relative_path);
    }

  g_assert (menu_entry_relative_path != NULL);
  
  menu_path_dirname = g_path_get_dirname (menu_path);
  
  if (!menu_override_dir_add (entry->overrides,
                              menu_path_dirname,
                              menu_entry_relative_path,
                              current_fs_path,
                              error))
    goto out;

  override_dir = menu_override_dir_get_fs_path (entry->overrides,
                                                menu_path_dirname,
                                                NULL);

  /* tell the tree that it needs to reload the .desktop file
   * cache
   */
  desktop_entry_tree_invalidate (tree, override_dir);

  /* Now include the .desktop file in the .menu file */
  if (!desktop_entry_tree_include (tree,
                                   menu_path_dirname,
                                   menu_entry_relative_path,
                                   override_dir,
                                   error))
    goto out;

  /* Mark cache entry to be reloaded next time we cache_lookup() */
  entry->needs_reload = TRUE;
  
  retval = TRUE;
  
 out:

  g_free (override_dir);
  g_free (menu_path_dirname);
  g_free (menu_entry_relative_path);
  g_free (current_fs_path);
  desktop_entry_tree_unref (tree);
  
  return retval;
}

gboolean
desktop_entry_tree_cache_delete (DesktopEntryTreeCache *cache,
                                 const char            *menu_file,
                                 const char            *menu_path,
                                 GError               **error)
{
  CacheEntry *entry;
  DesktopEntryTree *tree;
  gboolean retval;
  char *menu_path_dirname;
  char *menu_entry_relative_path;
  char *override_dir;

  menu_verbose ("Deleting \"%s\" in menu %s\n",
                menu_path, menu_file);
  
  entry = cache_lookup (cache, menu_file, TRUE, error);

  if (entry == NULL)
    return FALSE;

  try_create_overrides (entry, menu_file, error);
  
  if (entry->overrides == NULL)
    return FALSE;

  tree = desktop_entry_tree_cache_lookup (cache, menu_file,
                                          TRUE, error);
  if (tree == NULL)
    return FALSE;
  
  retval = FALSE;
  override_dir = NULL;

  menu_path_dirname = g_path_get_dirname (menu_path);

  menu_entry_relative_path = NULL;
  desktop_entry_tree_resolve_path (tree, menu_path,
                                   NULL, NULL,
                                   &menu_entry_relative_path);
  if (menu_entry_relative_path == NULL)
    {
      g_set_error (error, G_FILE_ERROR,
                   G_FILE_ERROR_EXIST,
                   _("\"%s\" not found\n"),
                   menu_path);
      goto out;
    }

  g_assert (menu_entry_relative_path != NULL);

  /* Ignore errors on this, as it's just mopping
   * up clutter, not required for function
   */
  menu_override_dir_remove (entry->overrides,
                            menu_path_dirname,
                            menu_entry_relative_path,
                            NULL);

  override_dir = menu_override_dir_get_fs_path (entry->overrides,
                                                menu_path_dirname,
                                                NULL);

  /* tell the tree that it needs to reload the .desktop file
   * cache
   */
  desktop_entry_tree_invalidate (tree, override_dir);

  /* Now include the .desktop file in the .menu file */
  if (!desktop_entry_tree_exclude (tree,
                                   menu_path_dirname,
                                   menu_entry_relative_path,
                                   error))
    goto out;
  
  /* Mark cache entry to be reloaded next time we cache_lookup() */
  entry->needs_reload = TRUE;
  
  retval = TRUE;
  
 out:

  g_free (override_dir);
  g_free (menu_path_dirname);
  g_free (menu_entry_relative_path);
  desktop_entry_tree_unref (tree);
  
  return retval;
}

gboolean
desktop_entry_tree_cache_mkdir (DesktopEntryTreeCache *cache,
                                const char            *menu_file,
                                const char            *menu_path,
                                GError               **error)
{
  CacheEntry *entry;
  DesktopEntryTree *tree;
  DesktopEntryTreeNode *node;
  gboolean retval;

  menu_verbose ("Making directory \"%s\" in menu %s\n",
                menu_path, menu_file);
  
  tree = desktop_entry_tree_cache_lookup (cache, menu_file,
                                          TRUE, error);
  if (tree == NULL)
    return FALSE;
  
  retval = FALSE;
  node = NULL;

  /* resolve_path returns TRUE if it's an entry instead
   * of a dir
   */
  desktop_entry_tree_resolve_path (tree, menu_path,
                                   &node, NULL, NULL);
  if (node != NULL)
    {
      g_set_error (error, G_FILE_ERROR,
                   G_FILE_ERROR_EXIST,
                   _("\"%s\" already exists\n"),
                   menu_path);
      goto out;
    }
  
  /* Create the directory */
  if (!desktop_entry_tree_mkdir (tree,
                                 menu_path,
                                 error))
    goto out;

  entry = cache_lookup (cache, menu_file, TRUE, error);

  if (entry == NULL)
    return FALSE;
  
  /* Mark cache entry to be reloaded next time we cache_lookup() */
  entry->needs_reload = TRUE;
  
  retval = TRUE;
  
 out:

  desktop_entry_tree_unref (tree);
  
  return retval;
}

gboolean
desktop_entry_tree_cache_rmdir (DesktopEntryTreeCache *cache,
                                const char            *menu_file,
                                const char            *menu_path,
                                GError               **error)
{
  CacheEntry *entry;
  DesktopEntryTree *tree;
  DesktopEntryTreeNode *node;
  gboolean retval;

  menu_verbose ("Removing directory \"%s\" in menu %s\n",
                menu_path, menu_file);
  
  tree = desktop_entry_tree_cache_lookup (cache, menu_file,
                                          TRUE, error);
  if (tree == NULL)
    return FALSE;
  
  retval = FALSE;
  node = NULL;

  /* resolve_path returns TRUE if it's an entry instead
   * of a dir
   */
  desktop_entry_tree_resolve_path (tree, menu_path,
                                   &node, NULL, NULL);
  if (node == NULL)
    {
      g_set_error (error, G_FILE_ERROR,
                   G_FILE_ERROR_NOENT,
                   _("\"%s\" doesn't exist\n"),
                   menu_path);
      goto out;
    }

  if (desktop_entry_tree_has_entries (tree, node))
    {
      g_set_error (error, G_FILE_ERROR,
                   G_FILE_ERROR_FAILED,
                   _("\"%s\" is not empty\n"),
                   menu_path);
      goto out;
    }
  
  /* Remove the directory */
  if (!desktop_entry_tree_rmdir (tree,
                                 menu_path,
                                 error))
    goto out;

  entry = cache_lookup (cache, menu_file, TRUE, error);

  if (entry == NULL)
    return FALSE;
  
  /* Mark cache entry to be reloaded next time we cache_lookup() */
  entry->needs_reload = TRUE;
  
  retval = TRUE;
  
 out:

  desktop_entry_tree_unref (tree);
  
  return retval;
}

GSList*
desktop_entry_tree_cache_get_changes (DesktopEntryTreeCache *cache,
                                      const char            *menu_file)
{
  CacheEntry *entry;
  
  menu_verbose ("Getting changes for \"%s\"\n",
                menu_file);
  
  entry = cache_lookup (cache, menu_file, FALSE, NULL);

  if (entry == NULL)
    return NULL;
  else
    {
      GSList *retval = entry->changes;
      entry->changes = NULL;

      return retval;
    }
}
