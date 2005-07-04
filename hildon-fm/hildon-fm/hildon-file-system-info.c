/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Luc Pionchon <luc.pionchon@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
/*
  hildon-file-system-info.h

  New API for querying info about files.
*/

#include "hildon-file-system-info.h"
#include "hildon-file-system-private.h"
#include <string.h>

struct _HildonFileSystemInfo
{
  GtkFileSystem *fs;
  GConfClient *client;
  GtkFilePath *path;  
  GtkFileInfo *info;
  HildonFileSystemModelItemType type;
  gchar *bluetooth_address;

  gchar *name_cache;
  GdkPixbuf *icon_cache;
};

static GtkFileInfo *get_info_for_path(GtkFileSystem *fs, const GtkFilePath *path, GError **error)
{
  GtkFilePath *parent_path;
  GtkFileFolder *folder;
  GtkFileInfo *result = NULL;
  
  if (gtk_file_system_get_parent(fs, path, &parent_path, error))
  {
    if (parent_path)
    {
      folder = gtk_file_system_get_folder(fs, parent_path, GTK_FILE_INFO_ALL, error);
      if (folder)
      {
        result = gtk_file_folder_get_info(folder, path, error);
        g_object_unref(folder);
      }
      gtk_file_path_free(parent_path);
    }
    else  
    {  /* There is no parent, so we have are toplevel folder. Let's create our own info */
      result = gtk_file_info_new();
      gtk_file_info_set_is_folder(result, TRUE);
      gtk_file_info_set_display_name(result, gtk_file_path_get_string(path));
    }
  }

  return result;
}

static gchar *get_bluetooth_address_from_uri(const gchar *uri)
{
  const gchar *endp;
  gsize len;

  if (!g_str_has_prefix(uri, "obex://["))
    return NULL;

  uri += 8; /* Skip obex://[ */
  endp = strchr(uri, ']');
  len = endp ? endp - uri : 0;

  return (len > 0 ? g_strndup(uri, len) : NULL);
}

/**
 * hildon_file_system_info_new:
 * @uri: an URI string.
 * @error: a place to store a possible error or %NULL.
 *
 * This function is used to query file statistics (the same ones
 * shown in the file selection widgets). Currently name and
 * icon are supported.
 *
 * Returns: a HildonFileSystemInfo or %NULL if erros was encontered.
 */
HildonFileSystemInfo *
hildon_file_system_info_new(const gchar *uri, GError **error)
{
    GtkFileSystem *fs;
    GtkFilePath *path;
    HildonFileSystemInfo *result;
    HildonFileSystemModelItemType type;
    GtkFileInfo *info = NULL;
    gchar *bluetooth_address;

    g_return_val_if_fail(uri != NULL, NULL);

    _hildon_file_system_ensure_locations();
    fs = hildon_file_system_create_backend("gnome-vfs", TRUE);
    g_assert(GTK_IS_FILE_SYSTEM(fs)); /* Creations should always succeed. */    

    path = gtk_file_system_uri_to_path(fs, uri);
    if (!path)
    {
      g_set_error(error, GTK_FILE_SYSTEM_ERROR, 
          GTK_FILE_SYSTEM_ERROR_INVALID_URI, "Invalid uri: %s", uri);
      g_object_unref(fs);
      return NULL;
    }

    bluetooth_address = get_bluetooth_address_from_uri(uri);
    type = bluetooth_address ? 
                    HILDON_FILE_SYSTEM_MODEL_GATEWAY : 
                    _hildon_file_system_get_special_location(fs, path);

    if (!type)
    {
      info = get_info_for_path(fs, path, error);
      if (!info)
      {
        g_object_unref(fs);
        g_free(bluetooth_address);
        gtk_file_path_free(path);
        return NULL;
      }
      
      type = gtk_file_info_get_is_folder(info) ? 
                    HILDON_FILE_SYSTEM_MODEL_FOLDER :
                    HILDON_FILE_SYSTEM_MODEL_FILE;  
    }

    result = g_new0(HildonFileSystemInfo, 1);
    result->fs = fs;
    result->client = gconf_client_get_default();
    result->path = path;
    result->info = info;
    result->type = type;
    result->bluetooth_address = bluetooth_address;

    return result;    
}

/**
 * hildon_file_system_info_get_display_name:
 * @info: a #HildonFileSystemInfo pointer.
 *
 * Gets user visible name of the location. All special places
 * are named using the current locale.
 *
 * Returns: Visible name of the location. The value is owned by
 * HildonFileSystemInfo and there is no need to free it.
 */
const gchar *
hildon_file_system_info_get_display_name(HildonFileSystemInfo *info)
{
  g_return_val_if_fail(info != NULL, NULL);

  if (info->name_cache == NULL)
    info->name_cache = _hildon_file_system_create_display_name(info->client, 
        info->type, info->bluetooth_address, info->info);

  return info->name_cache;
}

/**
 * hildon_file_system_info_get_icon:
 * @info: a #HildonFileSystemInfo pointer.
 *
 * Gets icon for the location. All special places
 * have their special icons.
 *
 * Returns: a #GdkPixbuf. The value is owned by
 * HildonFileSystemInfo and there is no need to free it.
 */
GdkPixbuf *
hildon_file_system_info_get_icon(HildonFileSystemInfo *info, GtkWidget *ref_widget)
{
  g_return_val_if_fail(info != NULL && GTK_IS_WIDGET(ref_widget), NULL);

  /* render icon seems to internally require that the folder is already created by get_folder */  
  if (info->icon_cache == NULL)
    info->icon_cache = _hildon_file_system_create_image(info->fs, gtk_icon_theme_get_default(), 
                  info->client, ref_widget, info->path, info->bluetooth_address, 
                  info->type, TREE_ICON_SIZE);

  return info->icon_cache;
}

/**
 * hildon_file_system_info_free:
 * @info: a #HildonFileSystemInfo pointer.
 *
 * Releases all resources allocated by info
 * structure.
 */
void hildon_file_system_info_free(HildonFileSystemInfo *info)
{
  g_return_if_fail(info != NULL);

  if (info->info)
    gtk_file_info_free(info->info);
  if (info->icon_cache)
    g_object_unref(info->icon_cache);

  gtk_file_path_free(info->path);
  g_free(info->bluetooth_address);
  g_free(info->name_cache);
  g_object_unref(info->fs);
  g_object_unref(info->client);

  g_free(info);
}
