/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005-2006 Nokia Corporation.  All rights reserved.
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
/*
 * hildon-file-system-info.h
 *
 * API for querying info about files.
 */

#include "hildon-file-system-info.h"
#include "hildon-file-system-private.h"
#include "hildon-file-common-private.h"
#include <string.h>

struct _HildonFileSystemInfo
{
  gboolean free_after_callback;

  GtkFileSystem *fs;
  GtkFilePath *path;  
  GtkFileInfo *info;
  HildonFileSystemSpecialLocation *location;

  gchar *name_cache;
  GdkPixbuf *icon_cache;
  gint size;

  GtkFileSystemHandle *get_info_handle;
  guint idle_handler_id;

  HildonFileSystemInfoCallback callback;
  gpointer userdata; /* For callback */
};

void
hildon_file_system_info_free (HildonFileSystemInfo *info)
{
  /* The info structure doubles as the handle, but that role must be
     over now.
  */
  g_assert (info->get_info_handle == NULL);
  g_assert (info->idle_handler_id == 0);

  if (info->info)
    gtk_file_info_free (info->info);

  if (info->icon_cache)
    g_object_unref(info->icon_cache);
  
  gtk_file_path_free(info->path);
  g_free(info->name_cache);
  g_object_unref(info->fs);
  
  g_slice_free (HildonFileSystemInfo, info);
}


static void
get_info_callback (GtkFileSystemHandle *handle,
		   const GtkFileInfo *file_info,
		   const GError *error,
		   gpointer data)
{
  gboolean cancelled = handle->cancelled;
  HildonFileSystemInfo *info = data;

  g_object_unref (handle);
  info->get_info_handle = NULL;

  info->info = (GtkFileInfo *)file_info;

  if (!cancelled)
    info->callback ((HildonFileSystemInfoHandle *)info,
		    error ? NULL : info, 
		    error, info->userdata);

  /* The documented behavior of the async API is to free the info
     structure after the callback returns.  However, the syncronous
     API needs to hands the structure to the user, and it resets
     FREE_AFTER_CALLBACK so that it stays valid.  We have to make a
     copy of the GtkFileInfo structure, so that it satys valid, too.

     When the sync API is removed, we can remove the
     FREE_AFTER_CALLBACK kluge as well.
  */

  if (info->free_after_callback)
    {
      info->info = NULL;
      hildon_file_system_info_free (info);
    }
  else
    info->info = gtk_file_info_copy (info->info);
}

static gboolean
idle_callback (gpointer data)
{
  HildonFileSystemInfo *info = data;

  info->idle_handler_id = 0;
  info->callback ((HildonFileSystemInfoHandle *)info,
		  info, NULL, info->userdata);

  /* See get_info_handle for the meaning of the FREE_AFTER_CALLBACK
     kluge.  We don't need to worry about info->info here since it is
     always NULL for special locations.
  */
  if (info->free_after_callback)
    hildon_file_system_info_free (info);

  return FALSE;
}

void
hildon_file_system_info_async_cancel (HildonFileSystemInfoHandle *handle)
{
  HildonFileSystemInfo *info = (HildonFileSystemInfo *)handle;
  
  if (info->get_info_handle)
    {
      /* get_info_callback takes care of the cleanup.
       */
      gtk_file_system_cancel_operation (info->get_info_handle);
    }
  else if (info->idle_handler_id > 0)
    {
      /* We need to do the cleanup ourselves.
       */
      g_source_remove (info->idle_handler_id);
      info->idle_handler_id = 0;
      hildon_file_system_info_free (info);
    }
  else
    {
      /* The operation has been completed already and the handle is
	 invalid.  So we can't do anything but complain.
      */
      ULOG_ERR ("attempt to cancel a completed "
		"hildon_file_system_info_async_new operation");
    }
}

/**
 * hildon_file_system_info_async_new:
 * @uri: an URI string.
 * @callback: a pointer to callback function to be invoked when
 *            file information is available.
 * @data: User data for callback function.
 *
 * This function is used to query file statistics (the same ones
 * shown in the file selection widgets). Currently name and
 * icon are supported.
 *
 * The info structure is only valid while the callback is running.
 * You must not free it yourself.
 *
 * The handle is valid until the callback has returned or until
 * hildon_file_system_info_async_cancel has been called on it.  Once
 * the callback has been entered, you can no longer call
 * hildon_file_system_info_async_cancel.
 *
 * Returns: a HildonFileSystemInfoHandle or %NULL if an error was 
 *          before setting up asyncronous operation.
 */
HildonFileSystemInfoHandle *
hildon_file_system_info_async_new (const gchar *uri, 
				   HildonFileSystemInfoCallback callback,
				   gpointer userdata)
{
  GtkFileSystem *fs;
  GtkFilePath *path;
  HildonFileSystemInfo *result;
  
  g_return_val_if_fail (uri != NULL, NULL);

  fs = hildon_file_system_create_backend ("gnome-vfs", TRUE);
  g_return_val_if_fail (GTK_IS_FILE_SYSTEM(fs), NULL);
  
  path = gtk_file_system_uri_to_path (fs, uri);
  if (!path)
    {
      g_object_unref(fs);
      return NULL;
    }
  
  result = g_slice_new0 (HildonFileSystemInfo);
  result->free_after_callback = TRUE;
  result->fs = fs;
  result->path = path;
  result->info = NULL;
  result->location = _hildon_file_system_get_special_location(fs, path);
  
  result->callback = callback;
  result->userdata = userdata;

  if (result->location)
    {
      /* We have all the information already and we could call the
	 callback right here.  But doing that would complicate our
	 API and thus we set up a idle handler.
      */
      result->idle_handler_id = g_idle_add (idle_callback, result);
    }
  else
    {
      result->get_info_handle =
	gtk_file_system_get_info (fs, path, 
				  GTK_FILE_INFO_ALL,
				  get_info_callback,
				  result);
    }

  return (HildonFileSystemInfoHandle *) result;
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
                  ref_widget, info->info, 
                  info->location, size);

  info->size = size;

  return info->icon_cache;
}

#ifndef HILDON_DISABLE_DEPRECATED

typedef struct {
  GMainLoop *loop;
  HildonFileSystemInfo *info;
  GError **error;
} sync_data;

static void
sync_callback  (HildonFileSystemInfoHandle *handle,
		HildonFileSystemInfo *info,
		const GError *error,
		gpointer data)
{
  sync_data *c = data;

  g_propagate_error (c->error, g_error_copy (error));

  if (info)
    info->free_after_callback = FALSE;
  c->info = info;

  g_main_loop_quit (c->loop);
}

HildonFileSystemInfo *
hildon_file_system_info_new (const gchar *uri, GError **error)
{
  sync_data c;
  HildonFileSystemInfoHandle *handle;

  handle = hildon_file_system_info_async_new (uri, sync_callback, &c);
  if (handle)
    {
      c.loop = g_main_loop_new (NULL, 0);
      c.error = error;
      g_main_loop_run (c.loop);
      return c.info;
    }
  else
    {
      /* XXX - This is the only reason why info_async_new can return
	       NULL.
      */
      g_set_error (error,
		   GTK_FILE_SYSTEM_ERROR, 
		   GTK_FILE_SYSTEM_ERROR_INVALID_URI,
		   "Invalid uri: %s", uri);
      return NULL;
    }
}
#endif
