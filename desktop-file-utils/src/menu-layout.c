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

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "menu-layout.h"
#include "canonicalize.h"
#include "menu-entries.h"
#include "menu-parser.h"
#include "dfu-test.h"
#include "menu-util.h"

#include <libintl.h>
#define _(x) gettext ((x))
#define N_(x) x

typedef struct MenuFile MenuFile;
typedef struct MenuNodeMenu MenuNodeMenu;
typedef struct MenuNodeRoot MenuNodeRoot;
typedef struct MenuNodeLegacyDir MenuNodeLegacyDir;

struct MenuFile
{
  char *filename;
  MenuNode *root;
};

struct MenuNode
{
  /* Node lists are circular, for length-one lists
   * prev/next point back to the node itself.
   */
  MenuNode *prev;
  MenuNode *next;
  MenuNode *parent;
  MenuNode *children;  

  char *content;

  guint refcount : 20;
  guint type : 7;
};

struct MenuNodeRoot
{
  MenuNode    node;
  char       *basedir;
  char       *name;
  EntryCache *entry_cache;
};

struct MenuNodeMenu
{
  MenuNode node;
  MenuNode *name_node; /* cache of the <Name> node */
  EntryDirectoryList *app_dirs;
  EntryDirectoryList *dir_dirs;
  GSList *monitors;
};

struct MenuNodeLegacyDir
{
  MenuNode node;
  char *prefix;
};

typedef struct
{
  MenuNodeMenuChangedFunc callback;
  gpointer                user_data;
} MenuNodeMenuMonitor;

/* root nodes (no parent) never have siblings */
#define NODE_NEXT(node)                                 \
((node)->parent == NULL) ?                              \
  NULL : (((node)->next == (node)->parent->children) ?  \
          NULL : (node)->next)

static int
null_safe_strcmp (const char *a,
                  const char *b)
{
  if (a == NULL && b == NULL)
    return 0;
  else if (a == NULL)
    return -1;
  else if (b == NULL)
    return 1;
  else
    return strcmp (a, b);
}

static void
handle_entry_directory_changed (EntryDirectory *dir,
				MenuNode       *node)
{
  MenuNodeMenu *nm;
  GSList *tmp;

  g_assert (node->type == MENU_NODE_MENU);

  nm = (MenuNodeMenu *) node;

  tmp = nm->monitors;
  while (tmp != NULL)
    {
      MenuNodeMenuMonitor *monitor = tmp->data;

      monitor->callback (node, monitor->user_data);

      tmp = tmp->next;
    }
}

static void
remove_entry_directory_list (MenuNodeMenu *nm,
			     gboolean      apps)
{
  EntryDirectoryList *list;

  list = apps ? nm->app_dirs : nm->dir_dirs;

  entry_directory_list_remove_monitors (list,
					(EntryDirectoryChangedFunc) handle_entry_directory_changed,
					nm);
  entry_directory_list_unref (list);

  if (apps)
    nm->app_dirs = NULL;
  else
    nm->dir_dirs = NULL;
}

void
menu_node_ref (MenuNode *node)
{
  g_return_if_fail (node != NULL);
  node->refcount += 1;
}

void
menu_node_unref (MenuNode *node)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (node->refcount > 0);
  
  node->refcount -= 1;
  if (node->refcount == 0)
    {
      MenuNode *iter;
      MenuNode *next;
      
      /* unref children */
      iter = node->children;
      while (iter != NULL)
        {
          next = NODE_NEXT (iter);
          menu_node_unref (iter);
          iter = next;
        }

      if (node->type == MENU_NODE_MENU)
        {
          MenuNodeMenu *nm = (MenuNodeMenu*) node;

          if (nm->name_node)
            menu_node_unref (nm->name_node);

          if (nm->app_dirs)
	    remove_entry_directory_list (nm, TRUE);

          if (nm->dir_dirs)
	    remove_entry_directory_list (nm, FALSE);

	  if (nm->monitors)
	    {
	      GSList *tmp;

	      tmp = nm->monitors;
	      while (tmp != NULL)
		{
		  g_free (tmp->data);
		  tmp = tmp->next;
		}
	      g_slist_free (nm->monitors);
	    }
        }
      else if (node->type == MENU_NODE_LEGACY_DIR)
        {
          MenuNodeLegacyDir *nld = (MenuNodeLegacyDir*) node;

          g_free (nld->prefix);
        }
      else if (node->type == MENU_NODE_ROOT)
        {
          MenuNodeRoot *nr = (MenuNodeRoot*) node;

          if (nr->entry_cache)
            entry_cache_unref (nr->entry_cache); 
          g_free (nr->basedir);
          g_free (nr->name);
        }
      
      /* free ourselves */
      g_free (node->content);
      g_free (node);
    }
}

MenuNode*
menu_node_new (MenuNodeType type)
{
  MenuNode *node;

  if (type == MENU_NODE_MENU)
    node = (MenuNode*) g_new0 (MenuNodeMenu, 1);
  else if (type == MENU_NODE_LEGACY_DIR)
    node = (MenuNode*) g_new0 (MenuNodeLegacyDir, 1);
  else if (type == MENU_NODE_ROOT)
    node = (MenuNode*) g_new0 (MenuNodeRoot, 1);
  else
    node = g_new0 (MenuNode, 1);

  node->type = type;
  
  node->refcount = 1;
  
  /* we're in a list of one node */
  node->next = node;
  node->prev = node;
  
  return node;
}

MenuNode*
menu_node_deep_copy (MenuNode *node)
{
  MenuNode *copy;
  MenuNode *iter;
  MenuNode *next;
  
  copy = menu_node_copy_one (node);

  /* Copy children */
  iter = node->children;
  while (iter != NULL)
    {
      MenuNode *child;
      
      next = NODE_NEXT (iter);

      child = menu_node_deep_copy (iter);
      menu_node_append_child (copy, child);
      menu_node_unref (child);
      
      iter = next;
    }

  return copy;
}

MenuNode*
menu_node_copy_one (MenuNode *node)
{
  MenuNode *copy;

  copy = menu_node_new (node->type);

  copy->content = g_strdup (node->content);

  if (copy->type == MENU_NODE_ROOT)
    {
      ((MenuNodeRoot*)copy)->basedir =
        g_strdup (((MenuNodeRoot*)node)->basedir);
      ((MenuNodeRoot*)copy)->name =
        g_strdup (((MenuNodeRoot*)node)->name);
    }
  else if (copy->type == MENU_NODE_LEGACY_DIR)
    ((MenuNodeLegacyDir*)copy)->prefix =
      g_strdup (((MenuNodeLegacyDir*)node)->prefix);
  
  return copy;
}

MenuNode*
menu_node_get_next (MenuNode *node)
{
  return NODE_NEXT (node);
}

MenuNode*
menu_node_get_parent (MenuNode *node)
{
  return node->parent;
}

MenuNode*
menu_node_get_children (MenuNode *node)
{
  return node->children;
}

MenuNode*
menu_node_get_root (MenuNode *node)
{
  MenuNode *parent;

  parent = node;
  while (parent->parent != NULL)
    parent = parent->parent;

  return parent;
}

int
menu_node_get_depth (MenuNode *node)
{
  MenuNode *parent;
  int depth;

  depth = 0;
  
  parent = node;
  while (parent->parent != NULL)
    {
      parent = parent->parent;
      depth += 1;
    }

  return depth;
}

const char*
menu_node_get_basedir (MenuNode *node)
{
  MenuNode *root;

  root = menu_node_get_root (node);

  if (root->type == MENU_NODE_ROOT)
    {
      MenuNodeRoot *nr = (MenuNodeRoot*) root;

      if (nr->basedir == NULL)
        menu_verbose ("Menu node root has null basedir\n");
      
      return nr->basedir;
    }
  else
    {
      menu_verbose ("Menu node root has type %d not root\n",
                    root->type);
      return NULL;
    }
}

const char*
menu_node_get_menu_name (MenuNode *node)
{
  MenuNode *root;

  root = menu_node_get_root (node);

  if (root->type == MENU_NODE_ROOT)
    {
      MenuNodeRoot *nr = (MenuNodeRoot*) root;

      if (nr->name == NULL)
        menu_verbose ("Menu node root has null name\n");
      
      return nr->name;
    }
  else
    {
      menu_verbose ("Menu node root has type %d not root\n",
                    root->type);
      return NULL;
    }
}

char*
menu_node_get_content_as_path (MenuNode *node)
{
  if (node->content == NULL)
    {
      menu_verbose ("  (node has no content to get as a path)\n");
      return NULL;
    }
  
  if (g_path_is_absolute (node->content))
    {
      menu_verbose ("Path \"%s\" is absolute\n",
                    node->content);
      return g_strdup (node->content);
    }
  else
    {
      const char *basedir;

      basedir = menu_node_get_basedir (node);
      
      if (basedir == NULL)
        {
          menu_verbose ("No basedir available, using \"%s\" as-is\n",
                        node->content);
          return g_strdup (node->content);
        }
      else
        {
          menu_verbose ("Using basedir \"%s\" filename \"%s\"\n",
                        basedir, node->content);
          return g_build_filename (basedir, node->content, NULL);
        }
    }
}

#define RETURN_IF_NO_PARENT(node)                               \
do                                                              \
{                                                               \
  if ((node)->parent == NULL)                                   \
    {                                                           \
      g_warning ("To add siblings to a menu node, "             \
                 "it must not be the root node, "               \
                 "and must be linked in below some root node\n" \
                 "node parent = %p and type = %d",              \
                 (node)->parent, (node)->type);                 \
      return;                                                   \
    }                                                           \
}                                                               \
while (0)

#define RETURN_IF_HAS_ENTRY_DIRS(node)                          \
do                                                              \
{                                                               \
  if ((node)->type == MENU_NODE_MENU &&                         \
      (((MenuNodeMenu*)(node))->app_dirs != NULL ||             \
       ((MenuNodeMenu*)(node))->dir_dirs != NULL))              \
    {                                                           \
      g_warning ("node acquired ->app_dirs or ->dir_dirs "      \
                 "while not rooted in a tree\n");               \
      return;                                                   \
    }                                                           \
}                                                               \
while (0)

void
menu_node_insert_before (MenuNode *node,
                         MenuNode *new_sibling)
{
  RETURN_IF_NO_PARENT (node);
  RETURN_IF_HAS_ENTRY_DIRS (new_sibling);
  g_return_if_fail (new_sibling != NULL);
  g_return_if_fail (new_sibling->parent == NULL);

  new_sibling->next = node;
  new_sibling->prev = node->prev;
  node->prev = new_sibling;
  new_sibling->prev->next = new_sibling;

  new_sibling->parent = node->parent;
  
  if (node == node->parent->children)
    node->parent->children = new_sibling;

  menu_node_ref (new_sibling);
}

void
menu_node_insert_after (MenuNode *node,
                        MenuNode *new_sibling)
{
  RETURN_IF_NO_PARENT (node);
  RETURN_IF_HAS_ENTRY_DIRS (new_sibling);
  g_return_if_fail (new_sibling != NULL);
  g_return_if_fail (new_sibling->parent == NULL);

  new_sibling->prev = node;
  new_sibling->next = node->next;
  node->next = new_sibling;
  new_sibling->next->prev = new_sibling;
  
  new_sibling->parent = node->parent;
  
  menu_node_ref (new_sibling);
}

void
menu_node_prepend_child (MenuNode *parent,
                         MenuNode *new_child)
{
  RETURN_IF_HAS_ENTRY_DIRS (new_child);
  
  if (parent->children)
    menu_node_insert_before (parent->children, new_child);
  else
    {
      parent->children = new_child;
      new_child->parent = parent;
      menu_node_ref (new_child);
    }
}

void
menu_node_append_child (MenuNode *parent,
                        MenuNode *new_child)
{  
  RETURN_IF_HAS_ENTRY_DIRS (new_child);
  
  if (parent->children)
    menu_node_insert_after (parent->children->prev, new_child);
  else
    {
      parent->children = new_child;
      new_child->parent = parent;
      menu_node_ref (new_child);
    }
}

void
menu_node_unlink (MenuNode *node)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (node->parent != NULL);

  menu_node_steal (node);
  menu_node_unref (node);
}

static void
recursive_clean_entry_directory_lists (MenuNode *node,
                                       gboolean  apps)
{
  MenuNodeMenu *nm;
  MenuNode *iter;
  EntryDirectoryList *dirs;
  
  if (node->type != MENU_NODE_MENU)
    return;

  nm = (MenuNodeMenu*) node;

  dirs = apps ? nm->app_dirs : nm->dir_dirs;

  if (dirs == NULL || entry_directory_list_get_length (dirs) == 0)
    return; /* child menus continue to have valid lists */

  remove_entry_directory_list (nm, apps);
      
  iter = node->children;
  while (iter != NULL)
    {
      MenuNode *next;
      
      next = NODE_NEXT (iter);
      if (iter->type == MENU_NODE_MENU)
        recursive_clean_entry_directory_lists (iter, apps);
      iter = next;
    }
}

void
menu_node_steal (MenuNode *node)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (node->parent != NULL);

  switch (node->type)
    {
    case MENU_NODE_NAME:
      {
        MenuNodeMenu *nm = (MenuNodeMenu*) node->parent;
        if (nm->name_node == node)
          {
            menu_node_unref (nm->name_node);
            nm->name_node = NULL;
          }
      }
      break;
    case MENU_NODE_APP_DIR:
      recursive_clean_entry_directory_lists (node, TRUE);
      break;
    case MENU_NODE_DIRECTORY_DIR:
      recursive_clean_entry_directory_lists (node, FALSE);
      break;

    default:
      break;
    }

  if (node->parent && node->parent->children == node)
    {
      if (node->next != node)
        node->parent->children = node->next;
      else
        node->parent->children = NULL;
    }
  
  /* these are no-ops for length-one node lists */
  node->prev->next = node->next;
  node->next->prev = node->prev;
    
  node->parent = NULL;
  
  /* point to ourselves, now we're length one */
  node->next = node;
  node->prev = node;
}

MenuNodeType
menu_node_get_type (MenuNode *node)
{
  return node->type;
}

const char*
menu_node_get_content (MenuNode *node)
{
  return node->content;
}

void
menu_node_set_content   (MenuNode   *node,
                         const char *content)
{
  if (node->content == content)
    return;
  
  g_free (node->content);
  node->content = g_strdup (content);
}

const char*
menu_node_menu_get_name (MenuNode *node)
{
  MenuNodeMenu *nm;
  
  g_return_val_if_fail (node->type == MENU_NODE_MENU, NULL);

  nm = (MenuNodeMenu*) node;

  if (nm->name_node == NULL)
    {
      MenuNode *iter;
      
      iter = node->children;
      while (iter != NULL)
        {
          MenuNode *next;
          
          next = NODE_NEXT (iter);

          if (iter->type == MENU_NODE_NAME)
            {
              nm->name_node = iter;
              menu_node_ref (nm->name_node);
              break;
            }
          
          iter = next;
        }
    }
  
  if (nm->name_node)
    return menu_node_get_content (nm->name_node);
  else
    return NULL;
}

const char*
menu_node_move_get_old (MenuNode *node)
{
  MenuNode *iter;
  
  iter = node->children;
  while (iter != NULL)
    {
      MenuNode *next;
      
      next = NODE_NEXT (iter);
      
      if (iter->type == MENU_NODE_OLD)
        return iter->content;

      iter = next;
    }

  return NULL;
}

const char*
menu_node_move_get_new (MenuNode *node)
{
  MenuNode *iter;
  
  iter = node->children;
  while (iter != NULL)
    {
      MenuNode *next;
      
      next = NODE_NEXT (iter);
      
      if (iter->type == MENU_NODE_NEW)
        return iter->content;

      iter = next;
    }

  return NULL;
}

const char*
menu_node_legacy_dir_get_prefix (MenuNode   *node)
{
  MenuNodeLegacyDir *nld;
  
  g_return_val_if_fail (node->type == MENU_NODE_LEGACY_DIR, NULL);

  nld = (MenuNodeLegacyDir*) node;
  
  return nld->prefix;
}

void
menu_node_legacy_dir_set_prefix (MenuNode   *node,
                                 const char *prefix)
{
  MenuNodeLegacyDir *nld;
  
  g_return_if_fail (node->type == MENU_NODE_LEGACY_DIR);

  nld = (MenuNodeLegacyDir*) node;

  if (nld->prefix == prefix)
    return;
  
  g_free (nld->prefix);
  nld->prefix = g_strdup (prefix);
}

const char*
menu_node_root_get_basedir (MenuNode *node)
{
  MenuNodeRoot *nr;
  
  g_return_val_if_fail (node->type == MENU_NODE_ROOT, NULL);

  nr = (MenuNodeRoot*) node;
  
  return nr->basedir;
}

void
menu_node_root_set_basedir (MenuNode   *node,
                            const char *dirname)
{
  MenuNodeRoot *nr;
  
  g_return_if_fail (node->type == MENU_NODE_ROOT);

  nr = (MenuNodeRoot*) node;

  if (nr->basedir == dirname)
    return;
  
  g_free (nr->basedir);
  nr->basedir = g_strdup (dirname);

  menu_verbose ("Set basedir \"%s\"\n", nr->basedir ? nr->basedir : "(none)");
}

const char*
menu_node_root_get_name (MenuNode *node)
{
  MenuNodeRoot *nr;
  
  g_return_val_if_fail (node->type == MENU_NODE_ROOT, NULL);

  nr = (MenuNodeRoot*) node;
  
  return nr->name;
}

void
menu_node_root_set_name (MenuNode   *node,
                         const char *menu_name)
{
  MenuNodeRoot *nr;
  
  g_return_if_fail (node->type == MENU_NODE_ROOT);

  nr = (MenuNodeRoot*) node;

  if (nr->name == menu_name)
    return;
  
  g_free (nr->name);
  nr->name = g_strdup (menu_name);

  menu_verbose ("Set name \"%s\"\n", nr->name ? nr->name : "(none)");
}

void
menu_node_root_set_entry_cache (MenuNode   *node,
                                EntryCache *entry_cache)
{
  MenuNodeRoot *nr;
  
  g_return_if_fail (node->type == MENU_NODE_ROOT);

  nr = (MenuNodeRoot*) node;

  if (nr->entry_cache == entry_cache)
    return;

  if (nr->entry_cache)
    entry_cache_unref (nr->entry_cache);
  nr->entry_cache = entry_cache;
  if (nr->entry_cache)
    entry_cache_ref (nr->entry_cache);
}

static void
menu_node_menu_ensure_entry_lists (MenuNode *node)
{
  MenuNodeMenu *nm;
  MenuNode *parent;
  MenuNode *iter;
  GSList *app_dirs;
  GSList *dir_dirs;
  GSList *tmp;
  MenuNodeRoot *root;
  
  g_return_if_fail (node->type == MENU_NODE_MENU);

  nm = (MenuNodeMenu*) node;  

  if (nm->app_dirs && nm->dir_dirs)
    return;

  root = (MenuNodeRoot*) menu_node_get_root (node);
  g_assert (root->node.type == MENU_NODE_ROOT);
  g_assert (root->entry_cache != NULL); /* can't get entry lists
                                         * unless an entry cache has been
                                         * set.
                                         */
  
  app_dirs = NULL;
  dir_dirs = NULL;
      
  iter = node->children;
  while (iter != NULL)
    {
      MenuNode *next;
      
      next = NODE_NEXT (iter);
      
      if ((nm->app_dirs == NULL && iter->type == MENU_NODE_APP_DIR) ||
          (nm->dir_dirs == NULL && iter->type == MENU_NODE_DIRECTORY_DIR) ||
          iter->type == MENU_NODE_LEGACY_DIR)
        {
          EntryDirectory *ed;

          if (iter->type == MENU_NODE_APP_DIR)
            {
              char *path;

              path = menu_node_get_content_as_path (iter);
              
              ed = entry_directory_load (root->entry_cache,
                                         path,
                                         ENTRY_LOAD_DESKTOPS, NULL);
              if (ed != NULL)
                app_dirs = g_slist_prepend (app_dirs, ed);

              g_free (path);
            }
          else if (iter->type == MENU_NODE_DIRECTORY_DIR)
            {
              char *path;

              path = menu_node_get_content_as_path (iter);
              
              ed = entry_directory_load (root->entry_cache,
                                         path,
                                         ENTRY_LOAD_DIRECTORIES, NULL);
              if (ed != NULL)
                dir_dirs = g_slist_prepend (dir_dirs, ed);

              g_free (path);
            }
          else if (iter->type == MENU_NODE_LEGACY_DIR)
            {
              char *path;

              path = menu_node_get_content_as_path (iter);

              if (nm->app_dirs == NULL) /* if we haven't already loaded app dirs */
                {
                  ed = entry_directory_load (root->entry_cache,
                                             path,
                                             ENTRY_LOAD_DESKTOPS | ENTRY_LOAD_LEGACY,
                                             NULL);
                  if (ed != NULL)
                    app_dirs = g_slist_prepend (app_dirs, ed);
                }

              if (nm->dir_dirs == NULL) /* if we haven't already loaded dir dirs */
                {
                  ed = entry_directory_load (root->entry_cache,
                                             path,
                                             ENTRY_LOAD_DIRECTORIES | ENTRY_LOAD_LEGACY,
                                             NULL);
                  if (ed != NULL)
                    dir_dirs = g_slist_prepend (dir_dirs, ed);
                  
                  g_free (path);
                }
            }
        }
          
      iter = next;
    }

  if (nm->app_dirs == NULL)
    {
      if (app_dirs == NULL)
        {
          /* If we don't have anything new to add, just find parent
           * EntryDirectoryList, or create an empty one if we're a
           * root node
           */
          if (node->parent == NULL ||
              node->parent->type == MENU_NODE_ROOT)
            nm->app_dirs = entry_directory_list_new ();
          else
            {
              nm->app_dirs = menu_node_menu_get_app_entries (node->parent);
              entry_directory_list_ref (nm->app_dirs);
            }
        }
      else
        {
          /* The app_dirs are currently in the same order they
           * should be for the directory list
           */
          nm->app_dirs = entry_directory_list_new ();
          
          tmp = app_dirs;
          while (tmp != NULL)
            {
              entry_directory_list_append (nm->app_dirs, tmp->data);
              entry_directory_unref (tmp->data);
              tmp = tmp->next;
            }

          g_slist_free (app_dirs);
          
          /* Get all the EntryDirectory from parent nodes */
          parent = node->parent;
          while (parent != NULL && parent->type != MENU_NODE_ROOT)
            {
              EntryDirectoryList *plist;
              
              plist = menu_node_menu_get_app_entries (parent);
              
              if (plist != NULL)
                entry_directory_list_append_list (nm->app_dirs,
                                                  plist);
              
              parent = parent->parent;
            }
        }

      entry_directory_list_add_monitors (nm->app_dirs,
					 (EntryDirectoryChangedFunc) handle_entry_directory_changed,
					 nm);
    }
  else
    {
      g_assert (app_dirs == NULL);
    }

  if (nm->dir_dirs == NULL)
    {
      if (dir_dirs == NULL)
        {
          /* If we don't have anything new to add, just find parent
           * EntryDirectoryList, or create an empty one if we're a
           * root node
           */
          if (node->parent == NULL ||
              node->parent->type == MENU_NODE_ROOT)
            nm->dir_dirs = entry_directory_list_new ();
          else
            {
              nm->dir_dirs = menu_node_menu_get_directory_entries (node->parent);
              entry_directory_list_ref (nm->dir_dirs);
            }
        }
      else
        {
          /* The dir_dirs are currently in the same order they
           * should be for the directory list
           */
          nm->dir_dirs = entry_directory_list_new ();
          
          tmp = dir_dirs;
          while (tmp != NULL)
            {
              entry_directory_list_append (nm->dir_dirs, tmp->data);
              entry_directory_unref (tmp->data);
              tmp = tmp->next;
            }

          g_slist_free (dir_dirs);
          
          /* Get all the EntryDirectory from parent nodes */
          parent = node->parent;
          while (parent != NULL && parent->type != MENU_NODE_ROOT)
            {
              EntryDirectoryList *plist;
              
              plist = menu_node_menu_get_directory_entries (parent);
              
              if (plist != NULL)
                entry_directory_list_append_list (nm->dir_dirs,
                                                  plist);
              
              parent = parent->parent;
            }
        }

      entry_directory_list_add_monitors (nm->dir_dirs,
					 (EntryDirectoryChangedFunc) handle_entry_directory_changed,
					 nm);
    }
  else
    {
      g_assert (dir_dirs == NULL);
    }  
}
     
EntryDirectoryList*
menu_node_menu_get_app_entries (MenuNode *node)
{
  MenuNodeMenu *nm;
  
  g_return_val_if_fail (node->type == MENU_NODE_MENU, NULL);

  nm = (MenuNodeMenu*) node;
  
  menu_node_menu_ensure_entry_lists (node);
  
  return nm->app_dirs;
}

EntryDirectoryList*
menu_node_menu_get_directory_entries (MenuNode *node)
{
  MenuNodeMenu *nm;
  
  g_return_val_if_fail (node->type == MENU_NODE_MENU, NULL);

  nm = (MenuNodeMenu*) node;

  menu_node_menu_ensure_entry_lists (node);
  
  return nm->dir_dirs;
}

void
menu_node_menu_add_monitor (MenuNode                *node,
			    MenuNodeMenuChangedFunc  callback,
			    gpointer                 user_data)
{
  MenuNodeMenu *nm;
  GSList *tmp;
  MenuNodeMenuMonitor *monitor;
  
  g_return_if_fail (node->type == MENU_NODE_MENU);

  nm = (MenuNodeMenu *) node;

  tmp = nm->monitors;
  while (tmp != NULL)
    {
      monitor = tmp->data;

      if (monitor->callback  == callback &&
	  monitor->user_data == user_data)
	break;

      tmp = tmp->next;
    }

  if (tmp == NULL)
    {
      monitor            = g_new0 (MenuNodeMenuMonitor, 1);
      monitor->callback  = callback;
      monitor->user_data = user_data;

      nm->monitors = g_slist_append (nm->monitors, monitor);
    }
}

void
menu_node_menu_remove_monitor (MenuNode                *node,
			       MenuNodeMenuChangedFunc  callback,
			       gpointer                 user_data)
{
  MenuNodeMenu *nm;
  GSList *tmp;
  
  g_return_if_fail (node->type == MENU_NODE_MENU);

  nm = (MenuNodeMenu *) node;

  tmp = nm->monitors;
  while (tmp != NULL)
    {
      MenuNodeMenuMonitor *monitor = tmp->data;
      GSList              *next = tmp->next;

      if (monitor->callback == callback &&
	  monitor->user_data == user_data)
	{
	  nm->monitors = g_slist_delete_link (nm->monitors, tmp);
	  g_free (monitor);
	}

      tmp = next;
    }
}

/* Remove sequences of include/exclude of the same file, etc. */
static gboolean
nodes_have_same_content (MenuNode *a,
                         MenuNode *b)
{
  const char *ac;
  const char *bc;

  ac = menu_node_get_content (a);
  bc = menu_node_get_content (b);

  if (null_safe_strcmp (ac, bc) == 0)
    return TRUE;
  else
    return FALSE;
}

static void
remove_redundant_nodes (MenuNode    *node,
                        MenuNodeType type1,
                        gboolean     have_type2,
                        MenuNodeType type2)
{
  MenuNode *child;
  MenuNode *prev;

  menu_verbose ("Removing redundancy in menu node %p for types %d and %d\n",
                node, type1, type2);

  prev = NULL;
  child = menu_node_get_children (node);
  while (child != NULL)
    {
      MenuNodeType t;
      
      t = menu_node_get_type (child);

      if (t == type1 ||
          (have_type2 && t == type2))
        {
          if (prev &&
              nodes_have_same_content (prev, child))
            {
              menu_verbose ("Consolidating two adjacent nodes with types %d %d content %s\n",
                            prev->type, child->type,
                            menu_node_get_content (child) ?
                            menu_node_get_content (child) : "(none)");
              menu_node_unlink (prev);
            }
          prev = child;
        }
      else if (t == MENU_NODE_MERGE_FILE ||
               t == MENU_NODE_MERGE_DIR)
        {
          /* merging in a file means we can't consolidate as we don't
           * know what's in between
           */
          menu_verbose ("Can't consolidate nodes across MergeFile/MergeDir\n");
          prev = NULL;
        }

      child = menu_node_get_next (child);
    }
}

static void
remove_redundant_match_rule (MenuNode    *node,
                             MenuNodeType type1)
{
  MenuNode *child;
  MenuNode *prev;

  menu_verbose ("Removing redundant match rules in menu node %p for type %d\n",
                node, type1);

  prev = NULL;
  child = menu_node_get_children (node);
  while (child != NULL)
    {
      MenuNodeType t;
      
      t = menu_node_get_type (child);

      if (t == type1)
        {
          if (prev &&
              nodes_have_same_content (prev, child))
            {
              menu_verbose ("Consolidating two adjacent nodes with types %d %d content %s\n",
                            prev->type, child->type,
                            menu_node_get_content (child) ?
                            menu_node_get_content (child) : "(none)");
              menu_node_unlink (prev);
            }
          prev = child;
        }
      else
        {
          /* Can't merge across intervening stuff */
          menu_verbose ("Can't consolidate nodes across node of type %d\n", t);
          switch (t)
            {
            case MENU_NODE_MERGE_FILE:
            case MENU_NODE_MERGE_DIR:
            case MENU_NODE_ALL:
            case MENU_NODE_AND:
            case MENU_NODE_OR:
            case MENU_NODE_NOT:
            case MENU_NODE_CATEGORY:
            case MENU_NODE_FILENAME:
              prev = NULL;
              break;
            default:
              break;
            }
        }

      child = menu_node_get_next (child);
    }
}

static void
remove_empty_containers (MenuNode    *node)
{
  MenuNode *child;

  menu_verbose ("Removing empty container nodes in %p\n", node);

  child = menu_node_get_children (node);
  while (child != NULL)
    {
      MenuNodeType t;
      MenuNode *next;

      next = menu_node_get_next (child);
      
      t = menu_node_get_type (child);

      switch (t)
        {
        case MENU_NODE_MENU:
        case MENU_NODE_INCLUDE:
        case MENU_NODE_EXCLUDE:
        case MENU_NODE_AND:
        case MENU_NODE_OR:
        case MENU_NODE_NOT:
        case MENU_NODE_MOVE:
          if (menu_node_get_children (child) == NULL)
            menu_node_unlink (child);
          break;
          
        default:
          break;
        }
      
      child = next;
    }
}


/* Note to self: if we removed redundant LegacyDir (which we
 * probably should) it needs to take into account the prefix=
 * attribute
 */
void
menu_node_remove_redundancy (MenuNode *node)
{
  MenuNode *child;

  menu_verbose ("Removing redundancy in menu node %p\n",
                node);
  
  remove_redundant_match_rule (node, MENU_NODE_FILENAME);
  remove_redundant_match_rule (node, MENU_NODE_CATEGORY);
  
  remove_redundant_nodes (node, MENU_NODE_DELETED,
                          TRUE, MENU_NODE_NOT_DELETED);

  remove_redundant_nodes (node, MENU_NODE_APP_DIR,
                          FALSE, 0);

  remove_redundant_nodes (node, MENU_NODE_DIRECTORY_DIR,
                          FALSE, 0);

  remove_redundant_nodes (node, MENU_NODE_DIRECTORY,
                          FALSE, 0);

  remove_empty_containers (node);
  
  /* Recurse */
  
  child = menu_node_get_children (node);
  while (child != NULL)
    {
      if (menu_node_get_type (child) == MENU_NODE_MENU)
        menu_node_remove_redundancy (child);

      child = menu_node_get_next (child);
    }
}

/*
 * The menu cache
 */
struct MenuCache
{
  int         refcount;
  GSList     *menu_files;
};

static void menu_node_append_to_string (MenuNode *node,
                                        int       depth,
                                        GString  *str);

MenuCache*
menu_cache_new (void)
{
  MenuCache *cache;

  cache = g_new0 (MenuCache, 1);

  cache->refcount = 1;
  
  return cache;
}

void
menu_cache_ref (MenuCache *cache)
{
  g_return_if_fail (cache != NULL);
  g_return_if_fail (cache->refcount > 0);

  cache->refcount += 1;
}

void
menu_cache_unref (MenuCache *cache)
{
  g_return_if_fail (cache != NULL);
  g_return_if_fail (cache->refcount > 0);

  cache->refcount -= 1;

  if (cache->refcount == 0)
    {
      /* FIXME free cache->menu_files */

      g_free (cache);
    }
}

static MenuFile*
find_file_by_name (MenuCache  *cache,
                   const char *filename)
{
  GSList *tmp;

  tmp = cache->menu_files;
  while (tmp != NULL)
    {
      MenuFile *f = tmp->data;
      if (strcmp (filename, f->filename) == 0)
        return f;
      tmp = tmp->next;
    }

  return NULL;
}

static MenuFile*
find_file_by_node (MenuCache *cache,
                   MenuNode  *node)
{
  GSList *tmp;
  MenuNode *root;

  root = menu_node_get_root (node);
  g_assert (root->type == MENU_NODE_ROOT);
  
  tmp = cache->menu_files;
  while (tmp != NULL)
    {
      MenuFile *f = tmp->data;
      if (root == f->root)
        return f;
      tmp = tmp->next;
    }

  return NULL;
}

const char*
menu_cache_get_filename_for_node (MenuCache *cache,
                                  MenuNode  *node)
{
  MenuFile *file;

  file = find_file_by_node (cache, node);
  if (file)
    return file->filename;
  else
    return NULL;
}

static void
drop_menu_file (MenuCache *cache,
                MenuFile  *file)
{
  cache->menu_files = g_slist_remove (cache->menu_files, file);

  menu_node_unref (file->root);
  
  g_free (file->filename);
  g_free (file);
}

void
menu_cache_invalidate (MenuCache  *cache,
                       const char *canonical)
{
  GSList *tmp;
  int len;

  menu_verbose ("Invalidating menu cache below \"%s\"\n",
                canonical);
  
  len = strlen (canonical);
  
  tmp = cache->menu_files;
  while (tmp != NULL)
    {
      GSList *next = tmp->next;
      MenuFile *file = tmp->data;

      if (strncmp (file->filename, canonical, len) == 0)
        {
          menu_verbose ("Dropping menu file \"%s\" due to cache invalidation\n",
                        file->filename);
          drop_menu_file (cache, file); /* removes "tmp" */
        }
      
      tmp = next;
    }
}

static gboolean
create_menu_file (const char *canonical,
                  const char *create_chaining_to,
                  GError    **error)
{
  GString *str;
  char *escaped;
  gboolean retval;

  menu_verbose ("Creating new menu file \"%s\" chaining to \"%s\"\n",
                canonical, create_chaining_to);
  
  str = g_string_new (NULL);

  g_string_append (str,
                   "<!DOCTYPE Menu PUBLIC \"-//freedesktop//DTD Menu 1.0//EN\"\n"
                   "\"http://www.freedesktop.org/standards/menu-spec/1.0/menu.dtd\">\n"
                   "\n"
                   "<!-- File created by desktop-file-utils version " VERSION " -->\n"
                   "<Menu>\n"
                   "  <Name>Applications</Name>\n"
                   "  <MergeFile>");

  escaped = g_markup_escape_text (create_chaining_to, -1);

  g_string_append (str, escaped);

  g_free (escaped);
  
  g_string_append (str,
                   "</MergeFile>\n"
                   "</Menu>\n");

  retval = g_file_save_atomically (canonical, str->str, str->len, error);

  g_string_free (str, TRUE);

  return retval;
}

MenuNode*
menu_cache_get_menu_for_canonical_file (MenuCache  *cache,
                                        const char *canonical,
                                        const char *create_chaining_to,
                                        GError    **error)
{
  MenuFile *file;
  MenuNode *node;

  menu_verbose ("menu_cache_get_menu_for_canonical_file(): \"%s\" chaining to \"%s\"\n",
                canonical, create_chaining_to ? create_chaining_to : "(none)");
  
  file = find_file_by_name (cache, canonical);
  
  if (file)
    {
      menu_verbose ("Got orig nodes for file \"%s\" from cache\n", canonical);
      menu_node_ref (file->root);
      return file->root;
    }

  menu_verbose ("File \"%s\" not in cache\n", canonical);
  
  node = menu_load (canonical, error);

  if (node == NULL &&
      create_chaining_to)
    {
      GError *tmp_error;
      tmp_error = NULL;
      if (!create_menu_file (canonical, create_chaining_to,
                             &tmp_error))
        {
          menu_verbose ("Failed to create file \"%s\": %s\n",
                        canonical, tmp_error->message);
          g_error_free (tmp_error);
        }
      else
        {
          g_clear_error (error);
          node = menu_load (canonical, error);
        }
    }

  if (node == NULL)
    return NULL; /* FIXME - cache failures? */

  g_assert (node->type == MENU_NODE_ROOT);
  
  file = g_new0 (MenuFile, 1);

  file->filename = g_strdup (canonical);
  file->root = node;

  cache->menu_files = g_slist_prepend (cache->menu_files, file);

  menu_node_ref (file->root);
  
  return file->root;
}

MenuNode*
menu_cache_get_menu_for_file (MenuCache  *cache,
                              const char *filename,
                              const char *create_chaining_to,
                              GError    **error)
{
  char *canonical;
  MenuNode *node;

  menu_verbose ("menu_cache_get_menu_for_file(): \"%s\" chaining to \"%s\"\n",
                filename, create_chaining_to ? create_chaining_to : "(none)");
  
  canonical = g_canonicalize_file_name (filename, create_chaining_to != NULL);
  if (canonical == NULL)
    {
      g_set_error (error, G_FILE_ERROR,
                   G_FILE_ERROR_FAILED,
                   _("Could not canonicalize filename \"%s\"\n"),
                   filename);
      return NULL;
    }
  
  node = menu_cache_get_menu_for_canonical_file (cache,
                                                 canonical,
                                                 create_chaining_to,
                                                 error);

  g_free (canonical);

  return node;
}

static void
append_spaces (GString *str,
               int      depth)
{
  while (depth > 0)
    {
      g_string_append_c (str, ' ');
      --depth;
    }
}

static void
append_children (MenuNode *node,
                 int       depth,
                 GString  *str)
{
  MenuNode *iter;
  
  iter = node->children;
  while (iter != NULL)
    {
      MenuNode *next = NODE_NEXT (iter);
      menu_node_append_to_string (iter, depth, str);
      iter = next;
    }
}

static void
append_simple_with_attr (MenuNode   *node,
                         int         depth,
                         const char *node_name,
                         const char *attr_name,
                         const char *attr_value,
                         GString    *str)
{
  append_spaces (str, depth);
  if (node->content)
    {
      char *escaped;

      escaped = g_markup_escape_text (node->content, -1);

      if (attr_name && attr_value)
        {
          char *attr_escaped;
          attr_escaped = g_markup_escape_text (attr_value, -1);
          
          g_string_append_printf (str, "<%s %s=\"%s\">%s</%s>\n",
                                  node_name, attr_name,
                                  attr_escaped, escaped, node_name);
          g_free (attr_escaped);
        }
      else
        g_string_append_printf (str, "<%s>%s</%s>\n",
                                node_name, escaped, node_name);

      g_free (escaped);
    }
  else
    {
      if (attr_name && attr_value)
        {
          char *attr_escaped;
          attr_escaped = g_markup_escape_text (attr_value, -1);
          
          g_string_append_printf (str, "<%s %s=\"%s\"/>\n",
                                  node_name, attr_name,
                                  attr_escaped);
          g_free (attr_escaped);
        }
      else
        g_string_append_printf (str, "<%s/>\n", node_name);
    }
}

static void
append_simple (MenuNode   *node,
               int         depth,
               const char *node_name,
               GString    *str)
{
  append_simple_with_attr (node, depth, node_name,
                           NULL, NULL, str);
}

static void
append_start (MenuNode   *node,
              int         depth,
              const char *node_name,
              GString    *str)
{
  append_spaces (str, depth);
  g_string_append_printf (str, "<%s>\n", node_name);
}

static void
append_end (MenuNode   *node,
            int         depth,
            const char *node_name,
            GString    *str)
{
  append_spaces (str, depth);
  g_string_append_printf (str, "</%s>\n", node_name);
}

static void
append_container (MenuNode   *node,
                  int         depth,
                  const char *node_name,
                  GString    *str)
{
  append_start (node, depth, node_name, str);
  append_children (node, depth + 2, str);
  append_end (node, depth, node_name, str);
}

static void
menu_node_append_to_string (MenuNode *node,
                            int       depth,
                            GString  *str)
{
  switch (node->type)
    {
    case MENU_NODE_ROOT:
      append_children (node, depth - 1, str); /* -1 to ignore depth of root */
      break;
    case MENU_NODE_PASSTHROUGH:
      g_string_append (str, node->content);
      g_string_append_c (str, '\n');
      break;
    case MENU_NODE_MENU:
      append_container (node, depth, "Menu", str);
      break;
    case MENU_NODE_APP_DIR:
      append_simple (node, depth, "AppDir", str);
      break;
    case MENU_NODE_DEFAULT_APP_DIRS:
      append_simple (node, depth, "DefaultAppDirs", str);
      break;
    case MENU_NODE_DIRECTORY_DIR:
      append_simple (node, depth, "DirectoryDir", str);
      break;
    case MENU_NODE_DEFAULT_DIRECTORY_DIRS:
      append_simple (node, depth, "DefaultDirectoryDirs", str);
      break;
    case MENU_NODE_DEFAULT_MERGE_DIRS:
      append_simple (node, depth, "DefaultMergeDirs", str);
      break;
    case MENU_NODE_NAME:
      append_simple (node, depth, "Name", str);
      break;
    case MENU_NODE_DIRECTORY:
      append_simple (node, depth, "Directory", str);
      break;
    case MENU_NODE_ONLY_UNALLOCATED:
      append_simple (node, depth, "OnlyUnallocated", str);
      break;
    case MENU_NODE_NOT_ONLY_UNALLOCATED:
      append_simple (node, depth, "NotOnlyUnallocated", str);
      break;
    case MENU_NODE_INCLUDE:
      append_container (node, depth, "Include", str);
      break;
    case MENU_NODE_EXCLUDE:
      append_container (node, depth, "Exclude", str);
      break;
    case MENU_NODE_FILENAME:
      append_simple (node, depth, "Filename", str);
      break;
    case MENU_NODE_CATEGORY:
      append_simple (node, depth, "Category", str);
      break;
    case MENU_NODE_ALL:
      append_simple (node, depth, "All", str);
      break;
    case MENU_NODE_AND:
      append_container (node, depth, "And", str);
      break;
    case MENU_NODE_OR:
      append_container (node, depth, "Or", str);
      break;
    case MENU_NODE_NOT:
      append_container (node, depth, "Not", str);
      break;
    case MENU_NODE_MERGE_FILE:
      append_simple (node, depth, "MergeFile", str);
      break;
    case MENU_NODE_MERGE_DIR:
      append_simple (node, depth, "MergeDir", str);
      break;
    case MENU_NODE_LEGACY_DIR:
      {
        MenuNodeLegacyDir *nld = (MenuNodeLegacyDir*) node;
        
        append_simple_with_attr (node, depth, "LegacyDir",
                                 "prefix", nld->prefix, str);
      }
      break;
    case MENU_NODE_KDE_LEGACY_DIRS:
      append_simple (node, depth, "KDELegacyDirs", str);
      break;
    case MENU_NODE_MOVE:
      append_container (node, depth, "Move", str);
      break;
    case MENU_NODE_OLD:
      append_simple (node, depth, "Old", str);
      break;
    case MENU_NODE_NEW:
      append_simple (node, depth, "New", str);
      break;
    case MENU_NODE_DELETED:
      append_simple (node, depth, "Deleted", str);
      break;
    case MENU_NODE_NOT_DELETED:
      append_simple (node, depth, "NotDeleted", str);
      break;
    }
}

gboolean
menu_cache_sync_for_file (MenuCache   *cache,
                          const char  *filename,
                          GError     **error)
{
  MenuFile *file;
  char *canonical;
  GString *str;
  gboolean retval;

  retval = FALSE;
  str = NULL;
  
  canonical = g_canonicalize_file_name (filename, TRUE);
  if (canonical == NULL)
    {
      g_set_error (error, G_FILE_ERROR,
                   G_FILE_ERROR_FAILED,
                   _("Could not canonicalize filename \"%s\"\n"),
                   filename);
      goto out;
    }
  
  file = find_file_by_name (cache, canonical);
  
  if (file == NULL)
    {
      g_set_error (error, G_FILE_ERROR,
                   G_FILE_ERROR_FAILED,
                   _("No menu file loaded for filename \"%s\"\n"),
                   filename);
      goto out;
    }

  /* Clean up the menu, to avoid collecting lots of junk over time */
  menu_node_remove_redundancy (file->root);

  /* now save */
  str = g_string_new (NULL);
  menu_node_append_to_string (file->root, 0, str);

  if (!g_file_save_atomically (canonical,
                               str->str, str->len,
                               error))
    goto out;
  
  retval = TRUE;
  
 out:
  g_free (canonical);
  if (str)
    g_string_free (str, TRUE);
  
  return retval;
}

void
menu_node_debug_print (MenuNode *node)
{
  GString *str;

  str = g_string_new (NULL);
  menu_node_append_to_string (node, 0, str);
  g_print ("%s", str->str);
  g_string_free (str, TRUE);
}

#ifdef DFU_BUILD_TESTS
static gboolean
node_has_child (MenuNode *node,
                MenuNode *child)
{
  MenuNode *tmp;

  if (node->children == NULL)
    return FALSE;
  
  tmp = node->children;
  do
    {
      if (child == tmp)
        return TRUE;

      tmp = tmp->next;
    }
  while (tmp != node->children);

  return FALSE;
}

static gboolean
nodes_equal (MenuNode *a,
             MenuNode *b)
{
  MenuNode *iter1;
  MenuNode *iter2;
  
  if (a->type != b->type)
    return FALSE;

  if (null_safe_strcmp (a->content, b->content) != 0)
    return FALSE;
  
  if (a->children && !b->children)
    return FALSE;

  if (!a->children && b->children)
    return FALSE;
  
  iter1 = a->children;
  iter2 = b->children;
  if (iter1 && iter2)
    {
      do
        {      
          if (!nodes_equal (iter1, iter2))
            return FALSE;

          iter1 = iter1->next;
          iter2 = iter2->next;
        }
      while (iter1 != a->children &&
             iter2 != b->children);

      if (iter1 != a->children)
        return FALSE;

      if (iter2 != b->children)
        return FALSE;
    }

  if (a->type == MENU_NODE_ROOT)
    {
      MenuNodeRoot *ar = (MenuNodeRoot*) a;
      MenuNodeRoot *br = (MenuNodeRoot*) b;

      if (null_safe_strcmp (ar->basedir, br->basedir) != 0)
        return FALSE;

      if (null_safe_strcmp (ar->name, br->name) != 0)
        return FALSE;
    }
  else if (a->type == MENU_NODE_LEGACY_DIR)
    {
      MenuNodeLegacyDir *ald = (MenuNodeLegacyDir*) a;
      MenuNodeLegacyDir *bld = (MenuNodeLegacyDir*) b;

      if (null_safe_strcmp (ald->prefix, bld->prefix) != 0)
        return FALSE;
    }
  
  return TRUE;
}

static void
menu_node_check_consistency (MenuNode *node)
{
  MenuNode *tmp;
  
  if (node->parent)
    g_assert (node_has_child (node->parent, node));
  
  tmp = node;
  do
    {
      g_assert (tmp->prev->next == tmp);
      g_assert (tmp->next->prev == tmp);

      if (tmp->children)
        menu_node_check_consistency (tmp->children);

      tmp = tmp->next;
    }
  while (tmp != node);  

  /* be sure copy works */
  tmp = menu_node_deep_copy (node);
  g_assert (nodes_equal (tmp, node));
  menu_node_unref (tmp);
  
  return;
}

gboolean
dfu_test_menu_nodes (const char *test_data_dir)
{
  MenuNode *node;
  MenuNode *child;
  int i;
  
  node = menu_node_new (MENU_NODE_INCLUDE);

  i = 0;
  while (i < 10)
    {
      child = menu_node_new (MENU_NODE_AND);

      menu_node_append_child (node, child);

      menu_node_unref (child);

      menu_node_check_consistency (node);
      
      ++i;
    }

  menu_node_unref (node);

  node = menu_node_new (MENU_NODE_EXCLUDE);

  i = 0;
  while (i < 10)
    {
      child = menu_node_new (MENU_NODE_OR);

      menu_node_prepend_child (node, child);

      menu_node_unref (child);

      menu_node_check_consistency (node);
      
      ++i;
    }

  menu_node_unref (node);
  
  return TRUE;
}
#endif /* DFU_BUILD_TESTS */
