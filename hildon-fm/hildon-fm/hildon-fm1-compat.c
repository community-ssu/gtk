/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005 Nokia Corporation.  All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "hildon-fm1-compat.h"
#include "hildon-file-common-private.h"

gboolean
hildon_file_selection_set_current_folder (HildonFileSelection *self,
					  const GtkFilePath *folder,
					  GError **error)
{
  return _hildon_file_selection_set_current_folder_path (self, folder, error);
}

GtkFilePath *
hildon_file_selection_get_current_folder (HildonFileSelection *self)
{
  return _hildon_file_selection_get_current_folder_path (self);
}

gboolean
hildon_file_selection_select_path (HildonFileSelection *self,
				   const GtkFilePath *path,
				   GError **error)
{
  return _hildon_file_selection_select_path (self, path, error);
}

void
hildon_file_selection_unselect_path (HildonFileSelection *self,
				     const GtkFilePath *path)
{
  return _hildon_file_selection_unselect_path (self, path);
}

GSList *
hildon_file_selection_get_selected_paths (HildonFileSelection *self)
{
  HildonFileSystemModel *model = NULL;
  GtkFileSystem *fs = NULL;
  GSList *uris = NULL, *elt;
  GSList *paths = NULL;

  g_object_get (self, "model", &model, NULL);
  if (model == NULL)
    return NULL;

  fs = _hildon_file_system_model_get_file_system (model);
  if (fs == NULL)
    return NULL;

  uris = hildon_file_selection_get_selected_uris (self);
  
  for (elt = uris; elt; elt = elt->next)
    {
      char *uri = (char *)elt->data;
      paths = g_slist_prepend (paths,
			       gtk_file_system_uri_to_path (fs, uri));
      g_free (uri);
    }
  g_slist_free (uris);

  return g_slist_reverse (paths);
}
