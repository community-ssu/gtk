/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005-2006 Nokia Corporation.  All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
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
 * hildon-file-system-info.h
 *
 * New API for querying info about files.
*/

#include "hildon-file-system-info.h"
#include "hildon-file-system-private.h"
#include "hildon-file-common-private.h"
#include <string.h>

struct _HildonFileSystemInfo
{
  GtkFileSystem *fs;
  GtkFilePath *path;  
  GtkFileInfo *info;
  HildonFileSystemSpecialLocation *location;

  gchar *name_cache;
  GdkPixbuf *icon_cache;
  gint size;
};

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
    GtkFilePath *path, *parent_path;
    HildonFileSystemInfo *result;
    HildonFileSystemSpecialLocation *location;
    GtkFileInfo *info = NULL;
    GtkFileFolder *folder;

    g_return_val_if_fail(uri != NULL, NULL);

    fs = hildon_file_system_create_backend("gnome-vfs", TRUE);
    g_return_val_if_fail(GTK_IS_FILE_SYSTEM(fs), NULL);

    path = gtk_file_system_uri_to_path(fs, uri);
    if (!path)
    {
      g_set_error(error, GTK_FILE_SYSTEM_ERROR, 
          GTK_FILE_SYSTEM_ERROR_INVALID_URI, "Invalid uri: %s", uri);
      g_object_unref(fs);
      return NULL;
    }    
  
    if (!gtk_file_system_get_parent(fs, path, &parent_path, error))
    { /* error is set */
      gtk_file_path_free(path);
      g_object_unref(fs);
      return NULL;
    }

    location = _hildon_file_system_get_special_location(fs, path);

    if (!location)
    {
        if (parent_path) {
            folder = gtk_file_system_get_folder(fs, parent_path, 
                        GTK_FILE_INFO_ALL, error);
            if (folder)
            {
                info = gtk_file_folder_get_info(folder, path, error);
                g_object_unref(folder);
            }
            if (!info)
            {
              g_object_unref(fs);
              gtk_file_path_free(parent_path);
              gtk_file_path_free(path);
              return NULL;
            }
        } else {
            info = gtk_file_info_new();
            gtk_file_info_set_is_folder(info, TRUE);
            gtk_file_info_set_display_name(info, gtk_file_path_get_string(path));
        }
    }

    gtk_file_path_free(parent_path);

    result = g_new0(HildonFileSystemInfo, 1);
    result->fs = fs;
    result->path = path;
    result->info = info;
    result->location = location;

    return result;    
}

/***********************************/
/*** Backported Async API Begins ***/
/***********************************/

struct _HildonFileSystemInfoHandle {
    int not_used;
};

struct AsyncIdleData {
    HildonFileSystemInfoCallback cb;
    gchar *uri;
    gpointer data;
    HildonFileSystemInfoHandle *handle;
};

struct AsyncCb {
    gpointer data;
};

static guint execute_callbacks_idle_id = 0;
static GSList *callbacks = NULL;

static void deliver_async_info(gpointer data)
{
    HildonFileSystemInfo *info = NULL;
    struct AsyncIdleData *aid = data;
    GError *error = NULL;

    GDK_THREADS_ENTER();

    info = hildon_file_system_info_new(aid->uri, &error);
    (*aid->cb)(aid->handle, info, error, aid->data);

    GDK_THREADS_LEAVE();

    g_free(aid->uri);
    aid->uri = NULL;
    g_free(aid->handle);
    aid->handle = NULL;
}

static gboolean execute_callbacks_idle(gpointer data)
{
    GSList *l;

    /* TODO: should be mutexed? */
    for (l = callbacks; l != NULL; l = l->next) {
        struct AsyncCb *acb;

        acb = l->data;
        if (acb != NULL) {
            deliver_async_info(acb->data);
            g_free(l->data);
            l->data = NULL;
        }
    }
    g_slist_free (callbacks);
    callbacks = NULL;

    execute_callbacks_idle_id = 0;

    return FALSE;
}

/* "Dummy" asyncronous function using the old syncronous API. */
HildonFileSystemInfoHandle *hildon_file_system_info_async_new(const gchar *uri, 
                    HildonFileSystemInfoCallback callback, gpointer data)
{
    struct AsyncIdleData *aid;
    struct AsyncCb *acb;
    HildonFileSystemInfoHandle *handle;

    aid = g_malloc(sizeof(struct AsyncIdleData));
    if (aid == NULL) {
        ULOG_ERR_F("could not allocate memory");
        return NULL;
    }
    aid->cb = callback;
    aid->uri = g_strdup(uri);
    if (aid->uri == NULL) {
        ULOG_ERR_F("could not allocate memory");
        g_free(aid);
        return NULL;
    }
    aid->data = data;
    aid->handle = handle = g_malloc(sizeof(HildonFileSystemInfoHandle));
    if (handle == NULL) {
        ULOG_ERR_F("could not allocate memory");
        g_free(aid->uri);
        g_free(aid);
        return NULL;
    }
    acb = g_malloc(sizeof(struct AsyncCb));
    if (acb == NULL) {
        ULOG_ERR_F("could not allocate memory");
        g_free(aid->handle);
        g_free(aid->uri);
        g_free(aid);
        return NULL;
    }

    acb->data = aid;

    /* TODO: should be mutexed? */
    callbacks = g_slist_append(callbacks, acb);

    if (!execute_callbacks_idle_id) {
        execute_callbacks_idle_id = g_idle_add(execute_callbacks_idle, NULL);
    }

    return handle;
}

void hildon_file_system_info_async_cancel(HildonFileSystemInfoHandle *handle)
{
    GSList *l;

    /* TODO: should be mutexed? */
    for (l = callbacks; l != NULL; l = l->next) {
        struct AsyncCb *acb;
        acb = l->data;
        if (acb != NULL) {
            struct AsyncIdleData *aid;
            aid = acb->data;
            if (aid->handle == handle) {
                g_free(aid->uri);
                aid->uri = NULL;
                g_free(aid->handle);
                aid->handle = NULL;
                g_free(aid);
                acb->data = NULL;
                callbacks = g_slist_delete_link(callbacks, l);
                break;
            }
        }
    }
}

/*********************************/
/*** Backported Async API Ends ***/
/*********************************/


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
    info->name_cache = _hildon_file_system_create_display_name(info->fs, 
        info->path, info->location, info->info);

  return info->name_cache;
}

/**
 * hildon_file_system_info_get_icon:
 * @info: a #HildonFileSystemInfo pointer.
 *
 * @ref_widget: Any widget on the same screen as the new icon.
 * Gets icon for the location. All special places
 * have their special icons. Note that this function is just
 * a special case of hildon_file_system_info_get_icon_at_size.
 *
 * Returns: a #GdkPixbuf. The value is owned by
 * HildonFileSystemInfo and there is no need to free it.
 */
GdkPixbuf *
hildon_file_system_info_get_icon(HildonFileSystemInfo *info, GtkWidget *ref_widget)
{
  return hildon_file_system_info_get_icon_at_size(info, 
         ref_widget, TREE_ICON_SIZE);
}

/**
 * hildon_file_system_info_get_icon_at_size:
 * @info: a #HildonFileSystemInfo pointer.
 * @ref_widget: Any widget on the same screen as the new icon.
 * @size: Desired size of the icon.
 *
 * Gets icon for the location. All special places
 * have their special icons.
 *
 * Returns: a #GdkPixbuf. The value is owned by
 * HildonFileSystemInfo and there is no need to free it.
 */
GdkPixbuf *
hildon_file_system_info_get_icon_at_size(HildonFileSystemInfo *info, 
  GtkWidget *ref_widget, gint size)
{
  g_return_val_if_fail(info != NULL && GTK_IS_WIDGET(ref_widget), NULL);

  if (info->icon_cache != NULL && size != info->size)
  {
    g_object_unref(info->icon_cache);
    info->icon_cache = NULL;
  }

  /* render icon seems to internally require that the folder is already created by get_folder */  
  if (info->icon_cache == NULL)
    info->icon_cache = _hildon_file_system_create_image(info->fs,  
                  ref_widget, info->path, 
                  info->location, size);

  info->size = size;

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
  if (info->location)
    g_object_unref(info->location);

  gtk_file_path_free(info->path);
  g_free(info->name_cache);
  g_object_unref(info->fs);

  g_free(info);
}
