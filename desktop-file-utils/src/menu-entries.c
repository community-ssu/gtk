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

#include <config.h>

#include "menu-entries.h"
#include "menu-monitor.h"
#include "menu-util.h"
#include "canonicalize.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#include <libintl.h>
#define _(x) gettext ((x))
#define N_(x) x


typedef struct CachedDir CachedDir;

struct Entry
{
  char *relative_path;
  char *absolute_path;
  
  unsigned int *categories; /* 0-terminated array of atoms */
  
  guint type : 4;
  guint refcount : 24;
  guint nodisplay : 1;
};

struct EntryDirectory
{
  char *absolute_path;
  CachedDir *root;
  guint flags : 4;
  guint refcount : 24;
};

static unsigned int entry_cache_intern_atom (EntryCache   *cache,
                                             const char   *str);
static const char*  entry_cache_atom_name   (EntryCache   *cache,
                                             unsigned int  atom) G_GNUC_UNUSED;
static unsigned int entry_cache_get_atom    (EntryCache   *cache,
                                             const char   *str);

static void        entry_cache_clear_unused (EntryCache  *cache) G_GNUC_UNUSED;

static CachedDir*  cached_dir_load           (EntryCache                 *cache,
					      const char                 *canonical_path,
					      gboolean                    is_legacy,
					      GError                    **err);
static void        cached_dir_mark_used      (CachedDir                  *dir);
static void        cached_dir_mark_unused    (CachedDir                  *dir);
static void        cached_dir_add_monitor    (CachedDir                  *dir,
					      EntryDirectory             *ed,
					      EntryDirectoryChangedFunc   callback,
					      gpointer                    user_data);
static void        cached_dir_remove_monitor (CachedDir                  *dir,
					      EntryDirectory             *ed,
					      EntryDirectoryChangedFunc   callback,
					      gpointer                    user_data);
static GSList*     cached_dir_get_subdirs    (CachedDir                  *dir);
static GSList*     cached_dir_get_entries    (CachedDir                  *dir);
static const char* cached_dir_get_name       (CachedDir                  *dir);
static Entry*      cached_dir_find_entry     (CachedDir                  *dir,
					      const char                 *name);
static int         cached_dir_count_entries  (CachedDir                  *dir);
static EntryCache* cached_dir_get_cache      (CachedDir                  *dir);
static gboolean    cached_dir_is_legacy      (CachedDir                  *dir);


static Entry*
entry_new (EntryType   type,
           const char *relative_path,
           const char *absolute_path,
	   gboolean    nodisplay)
{
  Entry *e;

  e = g_new0 (Entry, 1);
  
  e->categories = NULL;
  e->type = type;
  e->relative_path = g_strdup (relative_path);
  e->absolute_path = g_strdup (absolute_path);
  e->nodisplay = nodisplay != FALSE;
  e->refcount = 1;
  
  return e;
}

void
entry_ref (Entry *entry)
{
  g_return_if_fail (entry != NULL);
  g_return_if_fail (entry->refcount > 0);
  
  entry->refcount += 1;
}

void
entry_unref (Entry *entry)
{
  g_return_if_fail (entry != NULL);
  g_return_if_fail (entry->refcount > 0);

  entry->refcount -= 1;
  if (entry->refcount == 0)
    {
      g_free (entry->categories);

      g_free (entry->relative_path);
      g_free (entry->absolute_path);
      
      g_free (entry);
    }
}

const char*
entry_get_absolute_path (Entry *entry)
{
  return entry->absolute_path;
}

const char*
entry_get_relative_path (Entry *entry)
{
  return entry->relative_path;
}

const char*
entry_get_name (Entry *entry)
{
  const char *base;

  g_assert (entry->relative_path);
  
  base = strrchr (entry->relative_path, '/');
  if (base)
    return base + 1;
  else
    return entry->relative_path;
}

gboolean
entry_get_nodisplay (Entry *entry)
{
  g_return_val_if_fail (entry->type == ENTRY_DIRECTORY, FALSE);

  return entry->nodisplay;
}

gboolean
entry_has_category (Entry      *entry,
                    EntryCache *cache,
                    const char *category)
{
  int i;
  unsigned int a;
#define DETAILED_HAS_CATEGORY 0
  
#if DETAILED_HAS_CATEGORY
  menu_verbose ("  checking whether entry %s has category %s\n",
                entry->relative_path, category);
#endif
  
  if (entry->categories == NULL)
    {
#if DETAILED_HAS_CATEGORY
      menu_verbose ("   entry has no categories\n");
#endif
      return FALSE;
    }

  a = entry_cache_get_atom (cache, category);
  if (a == 0)
    {
#if DETAILED_HAS_CATEGORY
      menu_verbose ("   no entry has this category, category not interned\n");
#endif
      return FALSE;
    }
  
  i = 0;
  while (entry->categories[i] != 0)
    {
#if DETAILED_HAS_CATEGORY
      menu_verbose ("   %s %s\n",
                    category, entry_cache_atom_name (cache,
                                                     entry->categories[i]));
#endif
      if (a == entry->categories[i])
        return TRUE;
      
      ++i;
    }

#if DETAILED_HAS_CATEGORY
  menu_verbose ("   does not have category %s\n", category);
#endif
  
  return FALSE;
}

static gboolean
entry_has_category_atom (Entry       *entry,
                         unsigned int atom)
{
  int i;

#if DETAILED_HAS_CATEGORY
  menu_verbose ("  checking whether entry %s has category atom %u\n",
                entry->relative_path, atom);
#endif
  
  if (entry->categories == NULL)
    {
#if DETAILED_HAS_CATEGORY
      menu_verbose ("   entry has no categories\n");
#endif
      return FALSE;
    }
  
  i = 0;
  while (entry->categories[i] != 0)
    {
#if DETAILED_HAS_CATEGORY
      menu_verbose ("   %u %u\n",
                    atom, entry->categories[i]);
#endif

      if (atom == entry->categories[i])
        return TRUE;
      
      ++i;
    }

#if DETAILED_HAS_CATEGORY
  menu_verbose ("   does not have category atom %u\n", atom);
#endif
  
  return FALSE;
}

Entry*
entry_get_by_absolute_path (EntryCache *cache,
                            const char *path)
{
  CachedDir* dir;
  char *dirname;
  char *basename;
  char *canonical;
  Entry *retval;

  retval = NULL;
  dirname = g_path_get_basename (path);

  canonical = g_canonicalize_file_name (dirname, FALSE);
  if (canonical == NULL)
    {
      menu_verbose ("Error %d getting entry \"%s\": %s\n", errno, path,
                    g_strerror (errno));
      g_free (dirname);
      return NULL;
    }

  basename = g_path_get_dirname (path);
  
  dir = cached_dir_load (cache, dirname, 0, NULL);

  if (dir != NULL)
    retval = cached_dir_find_entry (dir, basename);

  g_free (dirname);
  g_free (basename);

  if (retval)
    entry_ref (retval);
  return retval;
}

EntryDirectory*
entry_directory_load  (EntryCache     *cache,
                       const char     *path,
                       EntryLoadFlags  flags,
                       GError        **err)
{
  char *canonical;
  CachedDir *cd;
  EntryDirectory *ed;

  menu_verbose ("Loading entry directory \"%s\"\n", path);
  
  canonical = g_canonicalize_file_name (path, FALSE);
  if (canonical == NULL)
    {
      g_set_error (err, ENTRY_ERROR,
                   ENTRY_ERROR_BAD_PATH,
                   _("Filename \"%s\" could not be canonicalized: %s\n"),
                   path, g_strerror (errno));
      menu_verbose ("Error %d canonicalizing \"%s\": %s\n", errno, path,
                    g_strerror (errno));
      return NULL;
    }

  cd = cached_dir_load (cache, canonical, flags & ENTRY_LOAD_LEGACY, err);
  if (cd == NULL)
    {
      g_free (canonical);
      return NULL;
    }

  ed = g_new0 (EntryDirectory, 1);
  ed->absolute_path = canonical;
  ed->root = cd;
  ed->flags = flags;
  ed->refcount = 1;

  cached_dir_mark_used (ed->root);
  
  return ed;
}

void
entry_directory_ref (EntryDirectory *ed)
{
  ed->refcount += 1;
}

void
entry_directory_unref (EntryDirectory *ed)
{
  g_return_if_fail (ed != NULL);
  g_return_if_fail (ed->refcount > 0);

  ed->refcount -= 1;  
  if (ed->refcount == 0)
    {
      cached_dir_mark_unused (ed->root);
      ed->root = NULL;
      g_free (ed->absolute_path);

      g_free (ed);
    }
}

static Entry*
entry_from_cached_entry (EntryDirectory *ed,
                         Entry          *src,
                         const char     *relative_path)
{
  Entry *e;
  int n_categories_to_copy;
  int n_categories_total;

  if (src->type != ENTRY_DESKTOP)
    return NULL;

  /* try to avoid a copy (no need to change path
   * or add "Legacy" keyword). This should
   * hold for the usual /usr/share/applications/foo.desktop
   * case, so is worthwhile.
   */
  if ((ed->flags & ENTRY_LOAD_LEGACY) == 0 &&
      strcmp (src->relative_path, relative_path) == 0)
    {
      entry_ref (src);
      return src;
    }
  
  e = entry_new (src->type, relative_path, src->absolute_path, src->nodisplay);
  
  n_categories_to_copy = 0;
  if (src->categories)
    {
      while (src->categories[n_categories_to_copy] != 0)
        ++n_categories_to_copy;
    }

  n_categories_total = n_categories_to_copy;
  if (ed->flags & ENTRY_LOAD_LEGACY && n_categories_total > 0)
    n_categories_total += 1;
  
  if (n_categories_total > 0)
    {
      e->categories = g_new (unsigned int, n_categories_total + 1);
      
      if (src->categories)
        {
          memcpy (e->categories,
                  src->categories,
                  n_categories_to_copy * sizeof (src->categories[0]));
        }

      if (ed->flags & ENTRY_LOAD_LEGACY)
        {
          e->categories[n_categories_total - 1] =
            entry_cache_intern_atom (cached_dir_get_cache (ed->root),
                                     "Legacy");
        }

      e->categories[n_categories_total] = 0;
    }

  return e;
}

Entry*
entry_directory_get_desktop (EntryDirectory *ed,
                             const char     *relative_path)
{
  Entry *src;
  
  if ((ed->flags & ENTRY_LOAD_DESKTOPS) == 0)
    return NULL;
  
  src = cached_dir_find_entry (ed->root, relative_path);
  if (src == NULL)
    return NULL;
  else
    return entry_from_cached_entry (ed, src, relative_path);
}

Entry*
entry_directory_get_directory (EntryDirectory *ed,
                               const char     *relative_path)
{
  Entry *src;
  Entry *e;
  
  if ((ed->flags & ENTRY_LOAD_DIRECTORIES) == 0)
    return NULL;
  
  src = cached_dir_find_entry (ed->root, relative_path);
  if (src == NULL)
    return NULL;

  if (src->type != ENTRY_DIRECTORY)
    return NULL;
  
  e = entry_new (src->type, relative_path, src->absolute_path, src->nodisplay);
  
  return e;
}

typedef gboolean (* EntryDirectoryForeachFunc) (EntryDirectory *ed,
                                                Entry          *src,
                                                const char     *relative_path,
                                                void           *data1,
                                                void           *data2);
static gboolean
entry_directory_foreach_recursive (EntryDirectory           *ed,
                                   CachedDir                *cd,
                                   char                     *parent_path,
                                   int                       parent_path_len,
                                   EntryDirectoryForeachFunc func,
                                   void                     *data1,
                                   void                     *data2)
{
  GSList *tmp;
  char *child_path_start;

  if (parent_path_len > 0)
    {
      parent_path[parent_path_len] = '/';
      child_path_start = parent_path + parent_path_len + 1;
    }
  else
    {
      child_path_start = parent_path + parent_path_len;
    }
  
  tmp = cached_dir_get_entries (cd);
  while (tmp != NULL)
    {
      Entry *src;
      char *path;

      src = tmp->data;

      if (src->type == ENTRY_DESKTOP &&
          (ed->flags & ENTRY_LOAD_DESKTOPS) == 0)
        goto next;

      if (src->type == ENTRY_DIRECTORY &&
          (ed->flags & ENTRY_LOAD_DIRECTORIES) == 0)
        goto next;
      
      strcpy (child_path_start,
              src->relative_path);

      if (cached_dir_is_legacy(cd)) {	      
         path = child_path_start;
      } else {
         path = parent_path;
      }

      if (!(* func) (ed, src, path,
                     data1, data2))
        return FALSE;

    next:
      tmp = tmp->next;
    }

  tmp = cached_dir_get_subdirs (cd);
  while (tmp != NULL)
    {
      CachedDir *sub;
      int path_len;
      const char *name;

      sub = tmp->data;
      name = cached_dir_get_name (sub);

      strcpy (child_path_start, name);

      path_len = (child_path_start - parent_path) + strlen (name);   
      
      if (!entry_directory_foreach_recursive (ed, sub,
                                              parent_path,
                                              path_len,
                                              func, data1, data2))
        return FALSE;
      
      tmp = tmp->next;
    }
  
  return TRUE;
}

static void
entry_directory_foreach (EntryDirectory           *ed,
                         EntryDirectoryForeachFunc func,
                         void                     *data1,
                         void                     *data2)
{
  char *parent_path;

  parent_path = g_new (char, PATH_MAX + 2);
  *parent_path = '\0';

  entry_directory_foreach_recursive (ed, ed->root,
                                     parent_path,
                                     0,
                                     func, data1, data2);

  g_free (parent_path);

#if 0
  menu_verbose ("%d entries in the EntryDirectory after foreach\n",
                cached_dir_count_entries (ed->root));
#endif
}

static gboolean
list_all_func (EntryDirectory *ed,
               Entry          *src,
               const char     *relative_path,
               void           *data1,
               void           *data2)
{
  EntrySet *set = data1;
  Entry *e;

  e = entry_from_cached_entry (ed, src, relative_path);
  entry_set_add_entry (set, e);
  entry_unref (e);

  return TRUE;
}

void
entry_directory_get_all_desktops (EntryDirectory *ed,
                                  EntrySet       *add_to_set)
{
  entry_directory_foreach (ed, list_all_func, add_to_set, NULL);
}

static gboolean
list_in_category_func (EntryDirectory *ed,
                       Entry          *src,
                       const char     *relative_path,
                       void           *data1,
                       void           *data2)
{
  EntrySet *set = data1;
  unsigned int category_atom = GPOINTER_TO_UINT (data2);

  if (entry_has_category_atom (src, category_atom) ||
      ((ed->flags & ENTRY_LOAD_LEGACY) &&
       entry_cache_get_atom (cached_dir_get_cache (ed->root),
                             "Legacy") == category_atom))
    {
      Entry *e;
      e = entry_from_cached_entry (ed, src, relative_path);
      entry_set_add_entry (set, e);
      entry_unref (e);
    }
  
  return TRUE;
}

void
entry_directory_get_by_category (EntryDirectory *ed,
                                 const char     *category,
                                 EntrySet       *set)
{
  unsigned int category_atom;

  category_atom = entry_cache_get_atom (cached_dir_get_cache (ed->root),
                                        category);
  
  menu_verbose (" Getting entries in dir with category %s (atom = %u)\n",
                category, category_atom);

  if (category_atom != 0)
    entry_directory_foreach (ed, list_in_category_func, set,
                             GUINT_TO_POINTER (category_atom));
}

void
entry_directory_add_monitor (EntryDirectory            *ed,
			     EntryDirectoryChangedFunc  callback,
			     gpointer                   user_data)
{
  cached_dir_add_monitor (ed->root, ed, callback, user_data);
}

void
entry_directory_remove_monitor (EntryDirectory            *ed,
				EntryDirectoryChangedFunc  callback,
				gpointer                   user_data)
{
  cached_dir_remove_monitor (ed->root, ed, callback, user_data);
}

struct EntryDirectoryList
{
  int refcount;
  GSList *dirs;
  int length;
};

EntryDirectoryList*
entry_directory_list_new (void)
{
  EntryDirectoryList *list;

  list = g_new0 (EntryDirectoryList, 1);

  list->refcount = 1;
  list->dirs = NULL;
  list->length = 0;
  
  return list;
}

void
entry_directory_list_ref (EntryDirectoryList *list)
{
  g_return_if_fail (list != NULL);
  g_return_if_fail (list->refcount > 0);
  
  list->refcount += 1;
}

void
entry_directory_list_unref (EntryDirectoryList *list)
{
  g_return_if_fail (list != NULL);
  g_return_if_fail (list->refcount > 0);
  
  list->refcount -= 1;
  if (list->refcount == 0)
    {
      entry_directory_list_clear (list);
      g_free (list);
    }
}

static void
dir_unref_foreach (void *element,
                   void *user_data)
{
  EntryDirectory *ed = element;
  entry_directory_unref (ed);
}

void
entry_directory_list_clear (EntryDirectoryList *list)
{
  g_slist_foreach (list->dirs, dir_unref_foreach, NULL);
  g_slist_free (list->dirs);
  list->dirs = NULL;
  list->length = 0;
}

void
entry_directory_list_prepend (EntryDirectoryList *list,
                              EntryDirectory     *dir)
{
  entry_directory_ref (dir);
  list->dirs = g_slist_prepend (list->dirs, dir);
  list->length += 1;
}

void
entry_directory_list_append  (EntryDirectoryList *list,
                              EntryDirectory     *dir)
{
  entry_directory_ref (dir);
  list->dirs = g_slist_append (list->dirs, dir);
  list->length += 1;
}

int
entry_directory_list_get_length (EntryDirectoryList *list)
{
  return list->length;
}

void
entry_directory_list_append_list (EntryDirectoryList *list,
                                  EntryDirectoryList *to_append)
{
  GSList *tmp;
  GSList *new_dirs;

  if (to_append->length == 0)
    return;
  
  new_dirs = NULL;
  tmp = to_append->dirs;
  while (tmp != NULL)
    {
      entry_directory_ref (tmp->data);
      new_dirs = g_slist_prepend (new_dirs, tmp->data);
      list->length += 1;
      
      tmp = tmp->next;
    }

  new_dirs = g_slist_reverse (new_dirs);
  list->dirs = g_slist_concat (list->dirs, new_dirs);
}

Entry*
entry_directory_list_get_desktop (EntryDirectoryList *list,
                                  const char         *relative_path)
{
  GSList *tmp;

  tmp = list->dirs;
  while (tmp != NULL)
    {
      Entry *e;

      e = entry_directory_get_desktop (tmp->data, relative_path);
      if (e && e->type == ENTRY_DESKTOP)
        return e;
      else if (e)
        entry_unref (e);
      
      tmp = tmp->next;
    }

  return NULL;
}

Entry*
entry_directory_list_get_directory (EntryDirectoryList *list,
                                    const char         *relative_path)
{
  GSList *tmp;

  tmp = list->dirs;
  while (tmp != NULL)
    {
      Entry *e;

      e = entry_directory_get_directory (tmp->data, relative_path);
      if (e && e->type == ENTRY_DIRECTORY)
        return e;
      else if (e)
        entry_unref (e);
      
      tmp = tmp->next;
    }

  return NULL;
}

static void
entry_directory_list_add (EntryDirectoryList       *list,
                          EntryDirectoryForeachFunc func,
                          EntrySet                 *set,
                          void                     *data2)
{
  /* The only tricky thing here is that desktop files later
   * in the search list with the same relative path
   * are "hidden" by desktop files earlier in the path,
   * so we have to do the earlier files first causing
   * the later files to replace the earlier files
   * in the EntrySet
   */
  GSList *tmp;
  GSList *reversed;

  /* We go from the end of the list so we can just
   * g_hash_table_replace and not have to do two
   * hash lookups (check for existing entry, then insert new
   * entry)
   */
  reversed = g_slist_copy (list->dirs);
  reversed = g_slist_reverse (reversed);

  tmp = reversed;
  while (tmp != NULL)
    {
      entry_directory_foreach (tmp->data, func, set, data2);
      
      tmp = tmp->next;
    }

  g_slist_free (reversed);
}

static gboolean
get_all_func (EntryDirectory *ed,
              Entry          *src,
              const char     *relative_path,
              void           *data1,
              void           *data2)
{
  EntrySet *set = data1;
  Entry *e;

  e = entry_from_cached_entry (ed, src, relative_path);
  entry_set_add_entry (set, e);
  entry_unref (e);

  return TRUE;
}

void
entry_directory_list_get_all_desktops (EntryDirectoryList *list,
                                       EntrySet           *set)
{
  menu_verbose (" Storing all of list %p in set %p\n",
                list, set);
  
  entry_directory_list_add (list, get_all_func,
                            set, NULL);
}

static gboolean
get_by_category_func (EntryDirectory *ed,
                      Entry          *src,
                      const char     *relative_path,
                      void           *data1,
                      void           *data2)
{
  EntrySet *set = data1;
  const char *category = data2;
  Entry *e;
  
  if (entry_has_category (src, cached_dir_get_cache (ed->root),
                          category) ||
      ((ed->flags & ENTRY_LOAD_LEGACY) &&
       strcmp (category, "Legacy") == 0))
    {
      e = entry_from_cached_entry (ed, src, relative_path);
      entry_set_add_entry (set, e);
      entry_unref (e);
    }

  return TRUE;
}

void
entry_directory_list_get_by_category (EntryDirectoryList *list,
                                      const char         *category,
                                      EntrySet           *set)
{
  menu_verbose (" Storing list %p category %s in set %p\n",
                list, category, set);
  entry_directory_list_add (list, get_by_category_func,
                            set, (char*) category);
}

static gboolean
get_inverse_func (EntryDirectory *ed,
                  Entry          *src,
                  const char     *relative_path,
                  void           *data1,
                  void           *data2)
{
  EntrySet *inverse = data1;
  EntrySet *set = data2;
  
  if (entry_set_lookup (set, relative_path) == NULL)
    {
      /* not in the original set, so add to inverse set */
      Entry *e;
      
      e = entry_from_cached_entry (ed, src, relative_path);
      entry_set_add_entry (inverse, e);
      entry_unref (e);
    }

  return TRUE;
}

void
entry_directory_list_invert_set (EntryDirectoryList *list,
                                 EntrySet           *set)
{
  EntrySet *inverse;

  menu_verbose (" Inverting set %p relative to list %p\n",
                set, list);
  
  inverse = entry_set_new ();
  entry_directory_list_add (list, get_inverse_func,
                            inverse, set);

  entry_set_swap_contents (set, inverse);

  entry_set_unref (inverse);
}

void
entry_directory_list_add_monitors (EntryDirectoryList        *list,
				   EntryDirectoryChangedFunc  callback,
				   gpointer                   user_data)
{
  GSList *tmp;

  tmp = list->dirs;
  while (tmp != NULL)
    {
      entry_directory_add_monitor (tmp->data, callback, user_data);
      tmp = tmp->next;
    }
}

void
entry_directory_list_remove_monitors (EntryDirectoryList        *list,
				      EntryDirectoryChangedFunc  callback,
				      gpointer                   user_data)
{
  GSList *tmp;

  tmp = list->dirs;
  while (tmp != NULL)
    {
      entry_directory_remove_monitor (tmp->data, callback, user_data);
      tmp = tmp->next;
    }
}

struct EntrySet
{
  int refcount;
  GHashTable *hash;
};

EntrySet*
entry_set_new (void)
{
  EntrySet *set;

  set = g_new0 (EntrySet, 1);
  set->refcount = 1;

  menu_verbose (" New entry set %p\n", set);
  
  return set;
}

void
entry_set_ref (EntrySet *set)
{
  set->refcount += 1;
}

void
entry_set_unref (EntrySet *set)
{
  g_return_if_fail (set != NULL);
  g_return_if_fail (set->refcount > 0);

  set->refcount -= 1;
  if (set->refcount == 0)
    {
      menu_verbose (" Deleting entry set %p\n", set);
      
      if (set->hash)
        g_hash_table_destroy (set->hash);
      g_free (set);
    }
}

void
entry_set_add_entry (EntrySet *set,
                     Entry    *entry)
{
  menu_verbose (" Adding to set %p entry %s\n",
                set, entry->relative_path);
  
  if (set->hash == NULL)
    {
      set->hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         NULL, (GDestroyNotify) entry_unref);      
    }

  entry_ref (entry);
  g_hash_table_replace (set->hash,
                        entry->relative_path,
                        entry);
}

void
entry_set_remove_entry (EntrySet *set,
                        Entry    *entry)
{
  menu_verbose (" Removing from set %p entry %s\n",
                set, entry->relative_path);
  
  if (set->hash == NULL)
    return;
  
  g_hash_table_remove (set->hash,
                       entry->relative_path);
}

Entry*
entry_set_lookup (EntrySet   *set,
                  const char *relative_path)
{
  if (set->hash == NULL)
    return NULL;

  return g_hash_table_lookup (set->hash, relative_path);
}

void
entry_set_clear (EntrySet *set)
{
  menu_verbose (" Clearing set %p\n", set);
  
  if (set->hash != NULL)
    {
      g_hash_table_destroy (set->hash);
      set->hash = NULL;
    }
}

static void
entry_hash_listify_foreach (void *key, void *value, void *data)
{
  GSList **list = data;
  Entry *e = value;

  g_assert (e != NULL);
  
  *list = g_slist_prepend (*list, e);
  entry_ref (e);
}

GSList*
entry_set_list_entries (EntrySet *set)
{
  GSList *list;

  if (set->hash == NULL)
    return NULL;
  
  list = NULL;
  g_hash_table_foreach (set->hash, entry_hash_listify_foreach, &list);

  return list;
}

int
entry_set_get_count (EntrySet *set)
{
  if (set->hash)
    return g_hash_table_size (set->hash);
  else
    return 0;
}

static void
union_foreach (void *key, void *value, void *data)
{
  /* we are iterating over "with" adding anything not
   * already in "set". We unconditionally overwrite
   * the stuff in "set" because we can assume
   * two entries with the same name are equivalent.
   */
  EntrySet *set = data;
  Entry *e = value;

  g_assert (set != NULL);
  g_assert (e != NULL);
  entry_set_add_entry (set, e);
}

void
entry_set_union (EntrySet *set,
                 EntrySet *with)
{
  menu_verbose (" Union of %p and %p\n", set, with);
  
  if (entry_set_get_count (with) == 0)
    {
      /* A fast simple case */
      return;
    }
  else
    {
      g_assert (with->hash);

      g_hash_table_foreach (with->hash,
                            union_foreach,
                            set);
    }
}

typedef struct
{
  EntrySet *set;
  EntrySet *with;
} IntersectData;

static gboolean
intersect_foreach_remove (void *key, void *value, void *data)
{
  IntersectData *id = data;

  /* return TRUE to remove if the key is not in the other set */
  if (g_hash_table_lookup (id->with->hash, key) == NULL)
    {
      menu_verbose (" Removing from %p entry %s\n",
                    id->set, (char*) key);
      return TRUE;
    }
  else
    return FALSE;
}

void
entry_set_intersection (EntrySet *set,
                        EntrySet *with)
{
  menu_verbose (" Intersection of %p and %p\n", set, with);
  
  if (entry_set_get_count (set) == 0 ||
      entry_set_get_count (with) == 0)
    {
      /* A fast simple case */
      entry_set_clear (set);
      return;
    }
  else
    {
      /* Remove everything in "set" which is not in "with" */
      IntersectData id;

      g_assert (set->hash);
      g_assert (with->hash);

      id.set = set;
      id.with = with;
      
      g_hash_table_foreach_remove (set->hash, intersect_foreach_remove, &id);
    }
}

typedef struct
{
  EntrySet *set;
  EntrySet *other;
} SubtractData;


static gboolean
subtract_foreach_remove (void *key, void *value, void *data)
{
  SubtractData *sd = data;

  /* return TRUE to remove if the key IS in the other set */
  if (g_hash_table_lookup (sd->other->hash, key) != NULL)
    {
      menu_verbose (" Removing from %p entry %s\n",
                    sd->set, (char*) key);      
      return TRUE;
    }
  else
    return FALSE;
}

void
entry_set_subtract (EntrySet *set,
                    EntrySet *other)
{
  menu_verbose (" Subtract from %p set %p\n", set, other);
  
  if (entry_set_get_count (set) == 0 ||
      entry_set_get_count (other) == 0)
    {
      /* A fast simple case */
      return;
    }
  else
    {
      /* Remove everything in "set" which is not in "other" */      
      SubtractData sd;
      
      g_assert (set->hash);
      g_assert (other->hash);

      sd.set = set;
      sd.other = other;
      
      g_hash_table_foreach_remove (set->hash, subtract_foreach_remove, &sd);
    }
}

void
entry_set_swap_contents (EntrySet *a,
                         EntrySet *b)
{
  GHashTable *tmp;

  menu_verbose (" Swap contents of %p and %p\n", a, b);
  
  tmp = a->hash;
  a->hash = b->hash;
  b->hash = tmp;
}

/*
 * Big global cache of desktop entries
 */

struct CachedDir
{
  EntryCache *cache;
  CachedDir *parent;
  char *name;
  GSList *entries;
  GSList *subdirs;
  MenuMonitor *monitor;
  GSList *monitors;
  guint have_read_entries : 1;
  guint use_count : 27;
  guint is_legacy : 1;
};

struct EntryCache
{
  int refcount;
  CachedDir *root_dir;
  char *only_show_in_name;
  GHashTable *atoms_by_name;
  GHashTable *names_by_atom;
  unsigned int next_atom;
};

typedef struct
{
  EntryDirectory            *ed;
  EntryDirectoryChangedFunc  callback;
  gpointer                   user_data;
} CachedDirMonitor;

static CachedDir* cached_dir_ensure         (EntryCache  *cache,
                                             const char  *canonical);
static void       cached_dir_scan_recursive (CachedDir   *dir,
                                             const char  *path);
static void       cached_dir_free           (CachedDir   *dir);
static void       cached_dir_invalidate     (CachedDir   *dir);


static CachedDir*
cached_dir_new (EntryCache *cache,
                const char *name)
{
  CachedDir *dir;

  dir = g_new0 (CachedDir, 1);

  dir->cache = cache;
  dir->name = g_strdup (name);
  dir->is_legacy = 0;

  menu_verbose ("New cached dir \"%s\"\n", name);
  
  return dir;
}

static void
cached_dir_clear_entries (CachedDir *dir)
{
  GSList *tmp;

  tmp = dir->entries;
  while (tmp != NULL)
    {
      entry_unref (tmp->data);
      tmp = tmp->next;
    }
  g_slist_free (dir->entries);
  dir->entries = NULL;
}

static void
cached_dir_clear_all_children (CachedDir *dir)
{
  GSList *tmp;

  cached_dir_clear_entries (dir);
  
  tmp = dir->subdirs;
  while (tmp != NULL)
    {
      cached_dir_free (tmp->data);
      tmp = tmp->next;
    }
  g_slist_free (dir->subdirs);
  dir->subdirs = NULL;
}

static void
cached_dir_free (CachedDir *dir)
{
  GSList *tmp;

  cached_dir_clear_all_children (dir);

  if (dir->use_count > 0)
    {
      CachedDir *iter;
      iter = dir;
      while (iter != NULL)
        {
          iter->use_count -= dir->use_count;
          iter = iter->parent;
        }
    }

  g_assert (dir->use_count == 0);

  if (dir->monitor)
    menu_monitor_remove (dir->monitor);

  tmp = dir->monitors;
  while (tmp != NULL)
    {
      g_free (tmp->data);
      tmp = tmp->next;
    }
  g_slist_free (dir->monitors);
  
  g_free (dir->name);
  g_free (dir);
}

static gboolean
cached_dir_is_legacy (CachedDir *cd) {
  return cd->is_legacy;
}

static void
ensure_root_dir (EntryCache *cache)
{
  if (cache->root_dir == NULL)
    cache->root_dir = cached_dir_new (cache, "/");
}

static CachedDir*
find_subdir (CachedDir   *dir,
             const char  *subdir)
{
  GSList *tmp;

  tmp = dir->subdirs;
  while (tmp != NULL)
    {
      CachedDir *sub = tmp->data;

      if (strcmp (sub->name, subdir) == 0)
        return sub;

      tmp = tmp->next;
    }

  return NULL;
}

static Entry*
find_entry (CachedDir   *dir,
            const char  *name)
{
  GSList *tmp;

  tmp = dir->entries;
  while (tmp != NULL)
    {
      Entry *e = tmp->data;

      if (strcmp (e->relative_path, name) == 0)
        return e;

      tmp = tmp->next;
    }

  return NULL;
}

static Entry*
cached_dir_find_entry (CachedDir   *dir,
                       const char  *name)
{
  char **split;
  int i;
  CachedDir *iter;
  
  split = g_strsplit (name, "/", -1);

  iter = dir;
  i = 0;
  while (iter != NULL && split[i] != NULL && *(split[i]) != '\0')
    {
      if (split[i+1] == NULL)
        {
          /* this is the last element, should be an entry */
          Entry *e;
          e = find_entry (iter, split[i]);
          g_strfreev (split);
          return e;
        }
      else
        {
          iter = find_subdir (iter, split[i]);
        }

      ++i;
    }

  g_strfreev (split);
  return NULL;
}

static CachedDir*
cached_dir_lookup (EntryCache *cache,
                   const char *canonical,
                   gboolean    create_if_not_found)
{
  char **split;
  int i;
  CachedDir *dir;

  g_return_val_if_fail (cache != NULL, NULL);
  g_return_val_if_fail (canonical != NULL, NULL);
  
  menu_verbose ("Looking up cached dir \"%s\", create_if_not_found = %d\n",
                canonical, create_if_not_found);
  
  /* canonicalize_file_name doesn't allow empty strings */
  g_assert (*canonical != '\0');  
  
  split = g_strsplit (canonical + 1, "/", -1);

  ensure_root_dir (cache);
  g_assert (cache->root_dir != NULL);
  
  dir = cache->root_dir;

  /* empty strings probably mean a trailing '/' */
  i = 0;
  while (split[i] != NULL && *(split[i]) != '\0')
    {
      CachedDir *cd;

      cd = find_subdir (dir, split[i]);

      if (cd == NULL)
        {
          if (create_if_not_found)
            {
              cd = cached_dir_new (cache, split[i]);
              dir->subdirs = g_slist_prepend (dir->subdirs, cd);
              cd->parent = dir;
            }
          else
            {
              menu_verbose ("No subdir \"%s\" found and not asked to create it\n",
                            split[i]);
              dir = NULL;
              goto out;
            }
        }

      dir = cd;
      
      ++i;
    }

 out:
  g_strfreev (split);

  g_assert (!create_if_not_found || dir != NULL);
  
  return dir;
}

static CachedDir*
cached_dir_ensure (EntryCache *cache,
                   const char *canonical)
{
  return cached_dir_lookup (cache, canonical, TRUE);
}

static CachedDir*
cached_dir_load (EntryCache *cache,
                 const char *canonical_path,
                 gboolean    is_legacy,
                 GError    **err)
{
  CachedDir *retval;

  menu_verbose ("Loading cached dir \"%s\"\n", canonical_path);  

  retval = NULL;

  retval = cached_dir_ensure (cache,
                              canonical_path);
  g_assert (retval != NULL);

  retval->is_legacy = retval->is_legacy || is_legacy;

  cached_dir_scan_recursive (retval, canonical_path);
  
  return retval;
}

static void
handle_cached_dir_changed (MenuMonitor      *monitor,
			   const char       *path,
			   MenuMonitorEvent  event,
			   CachedDir        *dir)
{
  CachedDir *iter;

  menu_verbose ("Notified of '%s' %s - invalidating cache\n",
		path,
		event == MENU_MONITOR_CREATED ? ("created") :
		event == MENU_MONITOR_DELETED ? ("deleted") :
		event == MENU_MONITOR_CHANGED ? ("changed") : ("unknown-event"));
		

  /* FIXME: we should just update the cache rather than invalidating
   *
   * Note, though, that its not just the list of entries we need
   * to re-do, we also need to re-read the files to pull out
   * categories etc.
   */
  cached_dir_invalidate (dir);

  iter = dir;
  while (iter != NULL)
    {
      GSList *tmp;

      tmp = dir->monitors;
      while (tmp != NULL)
	{
	  CachedDirMonitor *monitor = tmp->data;

	  monitor->callback (monitor->ed, monitor->user_data);

	  tmp = tmp->next;
	}

      iter = iter->parent;
    }
}

static void
cached_dir_ensure_monitor (CachedDir  *dir,
			   const char *dirname)
{
  if (dir->monitor != NULL)
    return;
    
  dir->monitor = menu_monitor_add_dir (dirname,
				       (MenuMonitorCallback) handle_cached_dir_changed,
				       dir);
}

static char*
cached_dir_get_full_path (CachedDir *dir)
{
  GString *str;
  GSList *parents;
  GSList *tmp;
  CachedDir *iter;
  
  str = g_string_new ("/");

  parents = NULL;
  iter = dir;
  while (iter != NULL && iter->parent != NULL)
    {
      parents = g_slist_prepend (parents, iter->name);
      iter = iter->parent;
    }
  
  tmp = parents;
  while (tmp != NULL)
    {
      char *subdir = tmp->data;

      g_string_append (str, subdir);
      g_string_append_c (str, '/');
      
      tmp = tmp->next;
    }

  g_slist_free (parents);

  return g_string_free (str, FALSE);
}

static char *
unescape_string (const char *str, int len)
{
  char *res;
  const char *p;
  char *q;
  const char *end;

  /* len + 1 is enough, because unescaping never makes the
   * string longer */
  res = g_new (gchar, len + 1);
  p = str;
  q = res;
  end = str + len;

  while (p < end)
    {
      if (*p == 0)
	{
	  /* Found an embedded null */
	  g_free (res);
	  return NULL;
	}
      if (*p == '\\')
	{
	  p++;
	  if (p >= end)
	    {
	      /* Escape at end of string */
	      g_free (res);
	      return NULL;
	    }
	  
	  switch (*p)
	    {
	    case 's':
              *q++ = ' ';
              break;
           case 't':
              *q++ = '\t';
              break;
           case 'n':
              *q++ = '\n';
              break;
           case 'r':
              *q++ = '\r';
              break;
           case '\\':
              *q++ = '\\';
              break;
           default:
	     /* Invalid escape code */
	     g_free (res);
	     return NULL;
	    }
	  p++;
	}
      else
	*q++ = *p++;
    }
  *q = 0;

  return res;
}

static char*
find_value (const char *str,
            const char *key)
{
  const char *p;
  const char *q;
  int key_len;

  key_len = -1;
  
  /* As a heuristic, scan quickly for the key name */
  p = str;

 scan_again:
  while (*p && *p != *key) /* find first byte */
    ++p;

  if (*p == '\0')
    return NULL;

  if (key_len < 0)
    key_len = strlen (key);

  if (strncmp (p, key, key_len) != 0)
    {
      ++p;
      goto scan_again;
    }

  /* If we get here, then the strncmp passed.
   * verify that we have '^ *key *='
   */
  q = p;
  while (q > str)
    {
      --q;

      if (!(*q == ' ' || *q == '\t'))
        break;
    }
  
  if (!(q == str || *q == '\n' || *q == '\r'))
    return NULL;

  q = p + key_len;
  while (*q)
    {
      if (!(*q == ' ' || *q == '\t'))
        break;
      ++q;
    }

  if (!(*q == '='))
    return NULL;

  /* skip '=' */
  ++q;  
  
  while (*q && (*q == ' ' || *q == '\t'))
    ++q;

  /* point p at start of value, then run q forward to the end of it */
  p = q;
  while (*q && (*q != '\n' && *q != '\r'))
    ++q;
  
  return unescape_string (p, q - p);
}

static char**
string_list_from_desktop_value (const char *raw)
{
  char **retval;
  int i;
  
  retval = g_strsplit (raw, ";", G_MAXINT);

  i = 0;
  while (retval[i])
    ++i;

  /* Drop the empty string g_strsplit leaves in the vector since
   * our list of strings ends in ";"
   */
  --i;
  g_free (retval[i]);
  retval[i] = NULL;

  return retval;
}

static int
count_bytes (const char *s,
             char        byte)
{
  const char *p = s;
  int count = 0;
  while (*p)
    {
      if (*p == byte)
        ++count;
      ++p;
    }
  return count;
}

static unsigned int*
atom_list_from_desktop_value (EntryCache  *cache,
                              const char  *raw)
{
  unsigned int *retval;
  int i;
  int len;
  char *s;
  char *last;
  char *copy;
  
  /* The string list is supposed to end in ';'
   * but it may not if someone is a loser,
   * so we don't know for sure from counting ';'
   * how many we have.
   */
  len = count_bytes (raw, ';');
  retval = g_new (unsigned int,
                  len + 2); /* add 2 in case of missing ';' */

  copy = g_strdup (raw);
  i = 0;
  s = copy;
  last = s;
  while ((s = strchr (s, ';')) != NULL)
    {
      g_assert (*s == ';');
      *s = '\0';
      ++s;

      if (*last != '\0')
        {
          retval[i] = entry_cache_intern_atom (cache, last);
          ++i;
        }

      last = s;
    }

  if (*last != '\0')
    {
      retval[i] = entry_cache_intern_atom (cache, last);
      ++i;
    }

  retval[i] = 0; /* 0-terminate */  

  g_free (copy);
  
  return retval;
}

static Entry*
entry_new_desktop_from_file (EntryCache *cache,
                             const char *filename,
                             const char *basename)
{
  char *str;
  GError *err;
  char *nodisplay;
  char *categories;
  Entry *e;
  
  str = NULL;
  err = NULL;
  if (!g_file_get_contents (filename, &str, NULL, &err))
    {
      menu_verbose ("Could not get contents of \"%s\": %s\n",
                    filename, err->message);
      g_error_free (err);
      return NULL;
    }

  nodisplay = find_value (str, "NoDisplay");
  if (nodisplay != NULL)
    {
      gboolean show;

      show = TRUE;
      if (strcmp (nodisplay, "true") == 0)
	{
	  menu_verbose ("Not showing \"%s\" because of NoDisplay=true\n",
			filename);
	  show = FALSE;
	}

      g_free (nodisplay);

      if (!show)
	{
	  g_free (str);
	  return NULL;
	}
    }

  if (cache->only_show_in_name)
    {
      char *onlyshowin;
      gboolean show;

      show = TRUE;
      
      onlyshowin = find_value (str, "OnlyShowIn");
  
      if (onlyshowin != NULL)
        {
          char **split;
          int i;

          show = FALSE;
          
          split = string_list_from_desktop_value (onlyshowin);
          i = 0;
          while (split[i] != NULL)
            {
              if (strcmp (split[i], cache->only_show_in_name) == 0)
                {
                  show = TRUE;
                  break;
                }
                  
              ++i;
            }

          if (!show)
            menu_verbose ("Not showing \"%s\" due to OnlyShowIn=%s\n",
                          filename, onlyshowin);
          
          g_strfreev (split);
          g_free (onlyshowin);          
        }

      if (!show)
        {
          g_free (str);
          return NULL;
        }
    }

  e = entry_new (ENTRY_DESKTOP, basename, filename, FALSE);
  
  categories = find_value (str, "Categories");
  if (categories != NULL)
    {
      e->categories = atom_list_from_desktop_value (cache,
                                                    categories);
      g_free (categories);
    }
  
  g_free (str);
  
  return e;
}

static Entry*
entry_new_directory_from_file (const char *filename,
                               const char *basename)
{
  char *str;
  GError *err;
  char *nodisplay_str;
  gboolean nodisplay;
  
  str = NULL;
  err = NULL;
  if (!g_file_get_contents (filename, &str, NULL, &err))
    {
      menu_verbose ("Could not get contents of \"%s\": %s\n",
                    filename, err->message);
      g_error_free (err);
      return NULL;
    }

  nodisplay = FALSE;
  nodisplay_str = find_value (str, "NoDisplay");
  if (nodisplay_str != NULL)
    {
      if (strcmp (nodisplay_str, "true") == 0)
	{
	  menu_verbose ("Not showing \"%s\" because of NoDisplay=true\n",
			filename);
	  nodisplay = TRUE;
	}

      g_free (nodisplay_str);
    }

  return entry_new (ENTRY_DIRECTORY, basename, filename, nodisplay);
}

static void
load_entries_recursive (CachedDir  *dir,
                        CachedDir  *parent,
                        const char *dirname,
                        const char *basename)
{
  DIR* dp;
  struct dirent* dent;
  char* fullpath;
  char* fullpath_end;
  guint len;
  guint subdir_len;

  /* dir non-NULL means we have a dir but haven't loaded
   * entries for it, parent non-NULL means create the dir
   * below this parent
   */
  g_return_if_fail (parent != NULL || dir != NULL);
  
  if (dir && dir->have_read_entries)
    return;

  menu_verbose ("Reading entries for %s (full path %s)\n",
                dir ? dir->name : "(not created yet)", dirname);
  menu_verbose( "basename %s\n", basename);
  
  dp = opendir (dirname);
  if (dp == NULL)
    {
      menu_verbose (_("Ignoring file \"%s\"\n"),
                    dirname);
      return;
    }

  if (dir == NULL)
    {
      g_assert (parent != NULL);
      
      dir = cached_dir_new (parent->cache, basename);
      dir->is_legacy = parent->is_legacy;
      dir->parent = parent;
      parent->subdirs = g_slist_prepend (parent->subdirs,
                                         dir);
    }

  g_assert (dir != NULL);

  cached_dir_clear_entries (dir);
  g_assert (dir->entries == NULL);

  cached_dir_ensure_monitor (dir, dirname);
  
  len = strlen (dirname);
  subdir_len = PATH_MAX - len;
  
  fullpath = g_new0 (char, subdir_len + len + 2); /* ensure null termination */
  strcpy (fullpath, dirname);
  
  fullpath_end = fullpath + len;
  if (*(fullpath_end - 1) != '/')
    {
      *fullpath_end = '/';
      ++fullpath_end;
    }
  
  while ((dent = readdir (dp)) != NULL)
    {      
      /* ignore . and .. */
      if (dent->d_name[0] == '.' &&
          (dent->d_name[1] == '\0' ||
           (dent->d_name[1] == '.' &&
            dent->d_name[2] == '\0')))
        continue;
      
      len = strlen (dent->d_name);

      if (len < subdir_len)
        {
          strcpy (fullpath_end, dent->d_name);
        }
      else
        continue; /* Shouldn't ever happen since PATH_MAX is available */

      if (g_str_has_suffix (dent->d_name, ".desktop"))
        {
          Entry *e;

          e = entry_new_desktop_from_file (dir->cache,
                                           fullpath, dent->d_name);

          menu_verbose ("Tried loading \"%s\": %s\n",
                        fullpath, e ? "ok" : "failed");
          
          if (e != NULL)
            dir->entries = g_slist_prepend (dir->entries, e);
        }
      else if (g_str_has_suffix (dent->d_name, ".directory"))
        {
          Entry *e;

          e = entry_new_directory_from_file (fullpath, dent->d_name);

          menu_verbose ("Tried loading \"%s\": %s\n",
                        fullpath, e ? "ok" : "failed");
          
          if (e != NULL)
            dir->entries = g_slist_prepend (dir->entries, e);
        }
      else
        {
          /* Try recursing */
          load_entries_recursive (NULL,
                                  dir,
                                  fullpath,
                                  dent->d_name);
        }
    }

  /* if this fails, we really can't do a thing about it
   * and it's not a meaningful error
   */
  closedir (dp);

  g_free (fullpath);

  dir->have_read_entries = TRUE;
}

static void
cached_dir_scan_recursive (CachedDir   *dir,
                           const char  *path)
{
  char *canonical;
  
  /* "path" corresponds to the canonical path to dir,
   * if path is non-NULL
   */
  canonical = NULL;
  if (path == NULL)
    {
      canonical = cached_dir_get_full_path (dir);
      path = canonical;
    }

  load_entries_recursive (dir, NULL, path, dir->name);

  g_free (canonical);
}

static int
mark_used_recursive (CachedDir *dir)
{
  int n_uses_added;
  GSList *tmp;

  /* Mark all children used, adding
   * the number of child uses to our own
   * use count
   */
  
  n_uses_added = 0;
  tmp = dir->subdirs;
  while (tmp != NULL)
    {
      n_uses_added += mark_used_recursive (tmp->data);
      
      tmp = tmp->next;
    }

  dir->use_count += n_uses_added + 1;

  /* return number of uses we added */
  return n_uses_added + 1;
}

static void
cached_dir_mark_used (CachedDir *dir)
{
  CachedDir *iter;
  int n_uses_added;

  n_uses_added = mark_used_recursive (dir);
  
  iter = dir->parent;
  while (iter != NULL)
    {
      iter->use_count += n_uses_added;
      iter = iter->parent;
    }
}

static int
mark_unused_recursive (CachedDir *dir)
{
  int n_uses_removed;
  GSList *tmp;

  /* Mark all children used, adding
   * the number of child uses to our own
   * use count
   */
  
  n_uses_removed = 0;
  tmp = dir->subdirs;
  while (tmp != NULL)
    {
      n_uses_removed += mark_unused_recursive (tmp->data);
      
      tmp = tmp->next;
    }

  dir->use_count -= n_uses_removed + 1;

  /* return number of uses we added */
  return n_uses_removed + 1;
}

static void
cached_dir_mark_unused (CachedDir *dir)
{
  CachedDir *iter;
  int n_uses_removed;

  g_return_if_fail (dir->use_count > 0);
  
  n_uses_removed = mark_unused_recursive (dir);
  
  iter = dir->parent;
  while (iter != NULL)
    {
      iter->use_count -= n_uses_removed;
      iter = iter->parent;
    }
}

static void
recursive_free_unused (CachedDir *dir)
{
  GSList *tmp;
  GSList *prev;

  prev = NULL;
  tmp = dir->subdirs;
  while (tmp != NULL)
    {
      CachedDir *child = tmp->data;
      GSList *next = tmp->next;
      
      if (child->use_count == 0)
        {
          cached_dir_free (child);

          if (prev == NULL)
            {
              g_slist_free (dir->subdirs);
              dir->subdirs = NULL;
            }
          else
            {
              g_assert (prev->next->data == tmp->data);
              g_assert (tmp->data == child);
              prev->next = g_slist_remove (prev->next, prev->next->data);
            }
        }
      else
        {
          prev = tmp;
          
          recursive_free_unused (child);
        }
      
      tmp = next;
    }
}

static void
entry_cache_clear_unused (EntryCache *cache)
{
  if (cache->root_dir != NULL)
    {
      recursive_free_unused (cache->root_dir);
      if (cache->root_dir->use_count == 0)
        {
          cached_dir_free (cache->root_dir);
          cache->root_dir = NULL;
        }
    }
}

static void
cached_dir_add_monitor (CachedDir                 *dir,
			EntryDirectory            *ed,
			EntryDirectoryChangedFunc  callback,
			gpointer                   user_data)
{
  CachedDirMonitor *monitor;
  GSList           *tmp;

  tmp = dir->monitors;
  while (tmp != NULL)
    {
      monitor = tmp->data;

      if (monitor->ed == ed &&
	  monitor->callback == callback &&
	  monitor->user_data == user_data)
	break;

      tmp = tmp->next;
    }

  if (tmp == NULL)
    {
      monitor            = g_new0 (CachedDirMonitor, 1);
      monitor->ed        = ed;
      monitor->callback  = callback;
      monitor->user_data = user_data;

      dir->monitors = g_slist_append (dir->monitors, monitor);
    }
}

static void
cached_dir_remove_monitor (CachedDir                 *dir,
			   EntryDirectory            *ed,
			   EntryDirectoryChangedFunc  callback,
			   gpointer                   user_data)
{
  GSList *tmp;

  tmp = dir->monitors;
  while (tmp != NULL)
    {
      CachedDirMonitor *monitor = tmp->data;
      GSList           *next = tmp->next;

      if (monitor->ed == ed &&
	  monitor->callback == callback &&
	  monitor->user_data == user_data)
	{
	  dir->monitors = g_slist_delete_link (dir->monitors, tmp);
	  g_free (monitor);
	}

      tmp = next;
    }
}

static const char*
cached_dir_get_name (CachedDir *dir)
{
  return dir->name;
}

static GSList*
cached_dir_get_subdirs (CachedDir *dir)
{
  if (!dir->have_read_entries)
    {
      cached_dir_scan_recursive (dir, NULL);
    }

  return dir->subdirs;
}

static GSList*
cached_dir_get_entries (CachedDir   *dir)
{
  if (!dir->have_read_entries)
    {
      cached_dir_scan_recursive (dir, NULL);
    }

  return dir->entries;
}

GQuark
entry_error_quark (void)
{
  static GQuark q = 0;
  if (q == 0)
    q = g_quark_from_static_string ("entry-error-quark");

  return q;
}

static int
cached_dir_count_entries (CachedDir *dir)
{
  int count;
  
  GSList *tmp;

  count = 0;
  
  tmp = dir->subdirs;
  while (tmp != NULL)
    {
      CachedDir *child = tmp->data;

      count += cached_dir_count_entries (child);
      
      tmp = tmp->next;
    }

  count += g_slist_length (dir->entries);

  return count;
}

static EntryCache*
cached_dir_get_cache (CachedDir *dir)
{
  return dir->cache;
}

EntryCache*
entry_cache_new (void)
{
  EntryCache *cache;

  cache = g_new0 (EntryCache, 1);
  cache->refcount = 1;

  cache->next_atom = 1;
  cache->atoms_by_name = g_hash_table_new_full (g_str_hash,
                                                g_str_equal,
                                                g_free, NULL);
  /* this points to same strings as atoms_by_name so must be freed
   * first
   */       
  cache->names_by_atom = g_hash_table_new_full (NULL, NULL,
                                                NULL, NULL);
  
  return cache;
}

void
entry_cache_ref (EntryCache *cache)
{
  g_return_if_fail (cache != NULL);
  g_return_if_fail (cache->refcount > 0);

  cache->refcount += 1;
}

void
entry_cache_unref (EntryCache *cache)
{
  g_return_if_fail (cache != NULL);
  g_return_if_fail (cache->refcount > 0);

  cache->refcount -= 1;
  if (cache->refcount == 0)
    {
      g_hash_table_destroy (cache->names_by_atom); /* must free before atoms_by_name
                                                    * as it uses same strings
                                                    */
      g_hash_table_destroy (cache->atoms_by_name);
      
      g_free (cache->only_show_in_name);
      g_free (cache);
    }
}

void
entry_cache_set_only_show_in_name (EntryCache *cache,
                                   const char *name)
{
  g_return_if_fail (cache != NULL);

  /* Really you're screwed if you do this after stuff has
   * already been loaded...
   */
  
  g_free (cache->only_show_in_name);
  cache->only_show_in_name = g_strdup (name);
}

static void
cached_dir_invalidate (CachedDir *dir)
{
  GSList *tmp;

  menu_verbose ("  (invalidating %s)\n",
                dir->name);
  
  dir->have_read_entries = FALSE;

  tmp = dir->subdirs;
  while (tmp != NULL)
    {
      cached_dir_invalidate (tmp->data);
      tmp = tmp->next;
    }
}

/* Invalidate cached data below filesystem dir */
void
entry_cache_invalidate (EntryCache *cache,
                        const char *dir)
{
  CachedDir *root;

  menu_verbose ("Invalidating cache of .desktop files in \"%s\"\n",
                dir);
  
  root = cached_dir_lookup (cache, dir, FALSE);
  
  if (root)
    cached_dir_invalidate (root);
}

static unsigned int
entry_cache_get_atom (EntryCache   *cache,
                      const char   *str)
{
  unsigned int val;
  
  val = GPOINTER_TO_UINT (g_hash_table_lookup (cache->atoms_by_name,
                                               str));
  
  return val;
}


static unsigned int
entry_cache_intern_atom (EntryCache   *cache,
                         const char   *str)
{
  unsigned int val;
  
  val = entry_cache_get_atom (cache, str);

  if (val == 0)
    {
      char *s;

      s = g_strdup (str);
      val = cache->next_atom;
      cache->next_atom += 1;
      
      g_hash_table_insert (cache->atoms_by_name,
                           s,
                           GUINT_TO_POINTER (val));
      g_hash_table_insert (cache->names_by_atom,
                           GUINT_TO_POINTER (val),
                           s);
    }

  g_assert (val > 0);
  return val;
}

static const char*
entry_cache_atom_name (EntryCache   *cache,
                       unsigned int  atom)
{
  return g_hash_table_lookup (cache->names_by_atom,
                              GUINT_TO_POINTER (atom));                              
}
