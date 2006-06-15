/*
 * Copyright (C) 2002 Alexander Larsson <alexl@redhat.com>.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gnome-icon-lookup.h"
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs.h>

#include <string.h>

#define ICON_NAME_BLOCK_DEVICE          "qgn_list_gene_unknown_file"
#define ICON_NAME_BROKEN_SYMBOLIC_LINK  "qgn_list_filesys_nonreadable_file"
#define ICON_NAME_CHARACTER_DEVICE      "qgn_list_gene_unknown_file"
#define ICON_NAME_DIRECTORY             "qgn_list_filesys_common_fldr"
#define ICON_NAME_EXECUTABLE            "gnome-fs-executable"
#define ICON_NAME_FIFO                  "qgn_list_gene_unknown_file"
#define ICON_NAME_REGULAR               "qgn_list_gene_unknown_file"
#define ICON_NAME_SEARCH_RESULTS        "gnome-fs-search"
#define ICON_NAME_SOCKET                "qgn_list_gene_unknown_file"

#define ICON_NAME_MIME_PREFIX           "gnome-mime-"


/* Returns NULL for regular */
static char *
get_icon_name (const char          *file_uri,
	       GnomeVFSFileInfo    *file_info,
	       const char          *mime_type,
	       GnomeIconLookupFlags flags)
{

  if (file_info &&
      (file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE))
    {
      switch (file_info->type)
	{
        case GNOME_VFS_FILE_TYPE_DIRECTORY:
	  if (mime_type && g_ascii_strcasecmp (mime_type, "x-directory/search") == 0)
	    return g_strdup (ICON_NAME_SEARCH_RESULTS);
	  else
	    return g_strdup (ICON_NAME_DIRECTORY);
        case GNOME_VFS_FILE_TYPE_FIFO:
	  return g_strdup (ICON_NAME_FIFO);
        case GNOME_VFS_FILE_TYPE_SOCKET:
	  return g_strdup (ICON_NAME_SOCKET);
        case GNOME_VFS_FILE_TYPE_CHARACTER_DEVICE:
	  return g_strdup (ICON_NAME_CHARACTER_DEVICE);
        case GNOME_VFS_FILE_TYPE_BLOCK_DEVICE:
	  return g_strdup (ICON_NAME_BLOCK_DEVICE);
        case GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK:
	  /* Non-broken symbolic links return the target's type. */
	  return g_strdup (ICON_NAME_BROKEN_SYMBOLIC_LINK);
        case GNOME_VFS_FILE_TYPE_REGULAR:
        case GNOME_VFS_FILE_TYPE_UNKNOWN:
        default:
	  break;
	}
    }
  
  if (mime_type && g_ascii_strncasecmp (mime_type, "x-directory", strlen ("x-directory")) == 0)
    return g_strdup (ICON_NAME_DIRECTORY);
  
  /* Regular or unknown: */

  /* don't use the executable icon for text files, since it's more useful to display
   * embedded text
   */
  if ((flags & GNOME_ICON_LOOKUP_FLAGS_EMBEDDING_TEXT) &&
      file_info &&
      (file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS) &&
      (file_info->permissions	& (GNOME_VFS_PERM_USER_EXEC
				   | GNOME_VFS_PERM_GROUP_EXEC
				   | GNOME_VFS_PERM_OTHER_EXEC)) &&
      (mime_type == NULL || g_ascii_strcasecmp (mime_type, "text/plain") != 0))
    return g_strdup (ICON_NAME_EXECUTABLE);

  return NULL;
}

static char *
get_vfs_mime_name (const char *mime_type)
{
  const char *vfs_mime_name;
  char *p;

  vfs_mime_name = gnome_vfs_mime_get_icon (mime_type);

  if (vfs_mime_name)
    {
      /* Handle absolute files */
      if (vfs_mime_name[0] == '/')
	return g_strdup (vfs_mime_name);
      
      p = strrchr(vfs_mime_name, '.');

      if (p)
	return g_strndup (vfs_mime_name, p - vfs_mime_name);
      else
	return g_strdup (vfs_mime_name);
    }
  return NULL;
}

static char *
make_mime_name (const char *mime_type)
{
  char *mime_type_without_slashes, *icon_name;
  char *p;
  
  if (mime_type == NULL) {
    return NULL;
  }

  mime_type_without_slashes = g_strdup (mime_type);
  
  while ((p = strchr(mime_type_without_slashes, '/')) != NULL)
    *p = '-';
  
  icon_name = g_strconcat (ICON_NAME_MIME_PREFIX, mime_type_without_slashes, NULL);
  g_free (mime_type_without_slashes);
  
  return icon_name;
}

static char *
make_generic_mime_name (const char *mime_type, gboolean embedd_text)
{
  char *generic_mime_type, *icon_name;
  char *p;

  
  if (mime_type == NULL) {
    return NULL;
  }

  generic_mime_type = g_strdup (mime_type);
  
  icon_name = NULL;
  if ((p = strchr(generic_mime_type, '/')) != NULL)
    {
      *p = 0;

      if (strcmp ("text", generic_mime_type) == 0 && embedd_text)
	icon_name = g_strdup ("gnome-fs-regular");
      else
	icon_name = g_strconcat (ICON_NAME_MIME_PREFIX, generic_mime_type, NULL);
    }
  g_free (generic_mime_type);
  
  return icon_name;
}

char *
gnome_icon_lookup (GtkIconTheme               *icon_theme,
		   const char                 *file_uri,
		   GnomeVFSFileInfo           *file_info,
		   const char                 *mime_type,
		   GnomeIconLookupFlags        flags)
{
  char *icon_name;
  char *mime_name;

  g_return_val_if_fail (GTK_IS_ICON_THEME (icon_theme), NULL);

  if (mime_type)
    {
      mime_name = get_vfs_mime_name (mime_type);
      if (mime_name &&
	  ((mime_name[0] == '/' &&  g_file_test (mime_name, G_FILE_TEST_IS_REGULAR)) ||
	   gtk_icon_theme_has_icon (icon_theme, mime_name)))
	return mime_name;
      g_free (mime_name);
      
      mime_name = make_mime_name (mime_type);
      if (mime_name && gtk_icon_theme_has_icon (icon_theme, mime_name))
	return mime_name;
      g_free (mime_name);
      
      mime_name = make_generic_mime_name (mime_type, flags & GNOME_ICON_LOOKUP_FLAGS_EMBEDDING_TEXT);
      if (mime_name && gtk_icon_theme_has_icon (icon_theme, mime_name))
	return mime_name;
      g_free (mime_name);
    }
      
  icon_name = get_icon_name (file_uri, file_info, mime_type, flags);
  if (icon_name && gtk_icon_theme_has_icon (icon_theme, icon_name))
    return icon_name;
  g_free (icon_name);

  return g_strdup (ICON_NAME_REGULAR);
}

