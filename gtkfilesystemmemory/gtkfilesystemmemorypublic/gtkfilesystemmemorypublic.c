/* gtkfilesystemmemorypublic.c
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
 * To use gtkfilesystemmemory you may need some additional
 * functionality. gtkfilesystemmemorypublic is implementing
 * the public part of the filesystem.
 */

#include <gtk/gtktreestore.h>

#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#undef __GNUC__ /* This is needed because compile option -pedantic
                   disables GNU extensions and code don't detect this */

#include <gtk/gtkfilesystem.h>

#include "../gtkfilesystemmemory/gtkfilesystemmemoryprivate.h"
#include "gtkfilesystemmemory.h"
#include <string.h>

/**
 * gtk_file_system_memory_tree_path_to_file_path:
 * @file_system: #GtkFileSystem
 * @path: #GtkTreePath
 *
 * Converts a #GtkTreePath to #GtkFilePath.
 *
 * Return value: A #GtkFilePath
 */
GtkFilePath *
gtk_file_system_memory_tree_path_to_file_path( GtkFileSystem *file_system,
                                               GtkTreePath *path )
{
  GtkTreeIter iter;
  
  if( !gtk_tree_model_get_iter( GTK_TREE_MODEL(file_system), &iter, path ) )
    return NULL;

  return gtk_file_system_memory_tree_iter_to_file_path( file_system, &iter );
}

/**
 * gtk_file_system_memory_file_path_to_tree_path:
 * @file_system: #GtkFileSystem
 * @path: #GtkFilePath
 *
 * Converts a #GtkFilePath to #GtkTreePath.
 *
 * Return value: A #GtkTreePath
 */
GtkTreePath *
gtk_file_system_memory_file_path_to_tree_path( GtkFileSystem *file_system,
                                               const GtkFilePath *path )
{
  GtkTreeIter iter;

  if (gtk_file_system_memory_file_path_to_tree_iter(file_system, &iter, path ))
    return gtk_tree_model_get_path( GTK_TREE_MODEL(file_system), &iter );

  return NULL; 
}

/**
 * gtk_file_system_memory_file_path_to_tree_iter:
 * @file_system: a #GtkFileSystem
 * @iter: a #GtkTreeIter to store the result.
 * @path: a #GtkFilePath
 *
 * Converts a #GtkFilePath to #GtkTreeIter.
 *
 * Return value: %TRUE, if #iter points to valid location, %FALSE otherwise.
 */
gboolean
gtk_file_system_memory_file_path_to_tree_iter( GtkFileSystem *file_system,
                                               GtkTreeIter *iter, const GtkFilePath *path )
{
  GtkTreeModel *model = NULL;
  GtkTreeIter child;
  gboolean is_folder = FALSE;
  gboolean result = FALSE;
  gint i = 0;
  gchar *str = NULL, *current = NULL;
  gchar **tokens = NULL;

  /* FIXME: this function is fuzzy, needs simplification */

  if( !path )
    return FALSE;

  model = GTK_TREE_MODEL(file_system);

  gtk_tree_model_get_iter_first( model, iter );
  tokens = g_strsplit( (gchar*)path, "/", 0 );

  if( !tokens[0] )
  {
    g_free(tokens);
    return FALSE;
  }
  
  current = tokens[0];

  if( *current == '\0' )
  {
    g_free( current );
    current = tokens[0] = g_strdup("/");
    if( *tokens[1] == '\0' )
    {
      g_free( tokens[1] );
      tokens[1] = NULL;
    }
  }

  while( current )
  {
    gtk_tree_model_get( model, iter, GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME, &str,
                        GTK_FILE_SYSTEM_MEMORY_COLUMN_IS_FOLDER, &is_folder,
                        -1 );
    if( !strcmp(current, str) )
    {
      g_free( str );
      
      if( tokens[i+1] == NULL || *tokens[i+1] == '\0' )
      {
        result = TRUE;
        break;
      }

      if( gtk_tree_model_iter_children( model, &child, iter ) )
      {
        *iter = child;
        g_free( current );
        current = tokens[++i];
        continue;
      }
    }
    else
    {
      g_free( str );
      
      if( *current == '\0' )
        break;
    }
    
    if ( !gtk_tree_model_iter_next( model, iter ) )
      break;
  }

/*  if( !current )
    result = gtk_tree_model_get_path( model, &iter ); 
  else*/
    while( current )
    {
      g_free( current );
      current = tokens[++i];
    }
  
  g_free( tokens );

  return result; 
}

/**
 * gtk_file_system_memory_tree_iter_to_file_path:
 * @file_system: #GtkFileSystem
 * @iter: #GtkTreeIter
 *
 * Converts a #GtkTreeIter to #GtkFilePath.
 *
 * Return value: A #GtkFilePath
 */
GtkFilePath *
gtk_file_system_memory_tree_iter_to_file_path( GtkFileSystem *file_system,
                                               GtkTreeIter *iter )
{
  GtkTreeModel *model= NULL;
  GtkTreeIter iterator;
  GtkTreeIter child;
  GString *path = NULL;
  gchar *name = NULL;

  model = GTK_TREE_MODEL(file_system);
  iterator = *iter;
  path = g_string_new(NULL);

  do
  {
    gtk_tree_model_get( model, &iterator, GTK_FILE_SYSTEM_MEMORY_COLUMN_NAME,
                        &name, -1 );
    if( name == NULL)
    {
      g_string_insert(path, 0, "/?");
    }

    /* FIXME: This doesn't work nicely for other "volumes" than "/".
       for example: c:\foobar => "/c:/foobar" */
    else if( strcmp( name, "/" ) != 0 )
    {
      /* non-root folder */
      g_string_insert(path, 0, name);
      g_string_insert(path, 0, "/");
    }
    g_free(name);

    child = iterator;
  } while( gtk_tree_model_iter_parent(model, &iterator, &child) );

  if (path->len == 0)
  {
    /* asking root node's path */
    g_string_append_c(path, '/');
  }
  
  return (GtkFilePath*)g_string_free(path, FALSE);
}


/**
 * gtk_file_system_memory_remove
 * @file_system: #GtkFileSystem
 * @path: #GtkTreePath
 *
 * Removes the item pointed by the path.
 * Should be used instead of the gtk_tree_store_remove.
 *
 * Return value: TRUE if success.
 */
gboolean gtk_file_system_memory_remove( GtkFileSystem *file_system,
                                        GtkTreePath *path )
{
  /* FIXME: gtkfilesystemmemory library is loaded dynamically, so we can't reference
     anything in it directly. So we can't use GTK_FILE_SYSTEM_MEMORY() macro
     or anything else. 
     
     We cannot really move the type registration here either,
     since that would cause program that just creates empty memorybackend
     through backend name (without linking against this library) to crash 
     because of missing symbol. This is not very likely use case, though. */
  GtkFileSystemMemory *fsm = (GtkFileSystemMemory *)file_system;
  GtkTreeIter iter;

  if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(file_system), &iter, path))
    return FALSE;

  /* save file path for "row-deleted" signal before it's removed from model. */
  fsm->deleted_file_path =
    gtk_file_system_memory_tree_path_to_file_path(file_system, path);

  gtk_tree_store_remove(GTK_TREE_STORE(file_system), &iter);

  gtk_file_path_free(fsm->deleted_file_path);
  fsm->deleted_file_path = NULL;
  return TRUE;
}

