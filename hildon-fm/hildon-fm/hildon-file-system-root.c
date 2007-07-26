/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2006 Nokia Corporation.  All rights reserved.
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

#include <string.h>
#include <glib.h>
#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#include <gtk/gtkfilesystem.h>

#include "hildon-file-common-private.h"
#include "hildon-file-system-private.h"
#include "hildon-file-system-root.h"
#include "hildon-file-system-mmc.h"
#include "hildon-file-system-voldev.h"
#include "hildon-file-system-settings.h"

static void
hildon_file_system_root_class_init (HildonFileSystemRootClass *klass);
static void
hildon_file_system_root_init (HildonFileSystemRoot *device);

static void
hildon_file_system_root_volumes_changed (HildonFileSystemSpecialLocation
                                        *location, GtkFileSystem *fs);

static GtkFileSystemHandle *
hildon_file_system_root_get_folder (HildonFileSystemSpecialLocation *location,
                                    GtkFileSystem                *filesystem,
                                    const GtkFilePath              *path,
                                    GtkFileInfoType                 types,
                                    GtkFileSystemGetFolderCallback  callback,
                                    gpointer                        data);

static HildonFileSystemSpecialLocation*
hildon_file_system_root_create_child_location (HildonFileSystemSpecialLocation
                                               *location, gchar *uri);

G_DEFINE_TYPE (HildonFileSystemRoot,
               hildon_file_system_root,
               HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION);

static void
hildon_file_system_root_class_init (HildonFileSystemRootClass *klass)
{
    HildonFileSystemSpecialLocationClass *location =
            HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS (klass);

    location->volumes_changed = hildon_file_system_root_volumes_changed;
    location->get_folder = hildon_file_system_root_get_folder;
    location->create_child_location =
      hildon_file_system_root_create_child_location;
}

static void
hildon_file_system_root_init (HildonFileSystemRoot *device)
{
}

static void
hildon_file_system_root_volumes_changed (HildonFileSystemSpecialLocation
                                        *location, GtkFileSystem *fs)
{
  g_signal_emit_by_name (location, "rescan");
}

HildonFileSystemSpecialLocation*
hildon_file_system_root_create_child_location (HildonFileSystemSpecialLocation
                                               *location, gchar *uri)
{
  HildonFileSystemSpecialLocation *child = NULL;

  /* XXX - Cough, a bit of hardcoding, it is better to ask GnomeVFS
           whether this is a volume or drive.
   */
  if (g_str_has_prefix (uri, "drive://")
      || (g_str_has_prefix (uri, "file:///media/")
          && strchr (uri + 14, '/') == NULL)
      || (g_str_has_prefix (uri, "file:///media/usb/")
          && strchr (uri + 18, '/') == NULL))
    {
      child = g_object_new (HILDON_TYPE_FILE_SYSTEM_VOLDEV, NULL);
      child->basepath = g_strdup (uri);
    }

  return child;
}

/* Wrapping GtkFileVolumes in a GtkFileFolder
 */

#define ROOT_TYPE_FILE_FOLDER             (root_file_folder_get_type ())
#define ROOT_FILE_FOLDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ROOT_TYPE_FILE_FOLDER, RootFileFolder))
#define ROOT_IS_FILE_FOLDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ROOT_TYPE_FILE_FOLDER))
#define ROOT_FILE_FOLDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ROOT_TYPE_FILE_FOLDER, RootFileFolderClass))
#define ROOT_IS_FILE_FOLDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ROOT_TYPE_FILE_FOLDER))
#define ROOT_FILE_FOLDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ROOT_TYPE_FILE_FOLDER, RootFileFolderClass))

typedef struct _RootFileFolder      RootFileFolder;
typedef struct _RootFileFolderClass RootFileFolderClass;

struct _RootFileFolderClass
{
  GObjectClass parent_class;
};

struct _RootFileFolder
{
  GObject parent_instance;

  GtkFileSystem *filesystem;
  HildonFileSystemRoot *root;
};

static GType root_file_folder_get_type (void);
static void root_file_folder_iface_init (GtkFileFolderIface *iface);
static void root_file_folder_init (RootFileFolder *impl);
static void root_file_folder_finalize (GObject *object);

static GtkFileInfo *root_file_folder_get_info (GtkFileFolder  *folder,
                                             const GtkFilePath    *path,
                                             GError        **error);
static gboolean root_file_folder_list_children (GtkFileFolder  *folder,
                                              GSList        **children,
                                              GError        **error);
static gboolean root_file_folder_is_finished_loading (GtkFileFolder *folder);

G_DEFINE_TYPE_WITH_CODE (RootFileFolder, root_file_folder, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_FILE_FOLDER,
                                                root_file_folder_iface_init))

static void
root_file_folder_class_init (RootFileFolderClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = root_file_folder_finalize;
}

static void
root_file_folder_iface_init (GtkFileFolderIface *iface)
{
  iface->get_info = root_file_folder_get_info;
  iface->list_children = root_file_folder_list_children;
  iface->is_finished_loading = root_file_folder_is_finished_loading;
}

static void
root_file_folder_init (RootFileFolder *folder)
{
  folder->filesystem = NULL;
  folder->root = NULL;
}

static void
root_file_folder_finalize (GObject *object)
{
  RootFileFolder *folder = ROOT_FILE_FOLDER (object);

  if (folder->root)
    g_object_unref (folder->root);

  G_OBJECT_CLASS (root_file_folder_parent_class)->finalize (object);
}

static GtkFileInfo *
root_file_folder_get_info (GtkFileFolder      *folder,
                           const GtkFilePath  *path,
                           GError            **error)
{
  GtkFileInfo *info;

  /* XXX - maybe provide more detail...
   */

  info = gtk_file_info_new ();
  gtk_file_info_set_display_name (info,
                                  g_basename
                                  (gtk_file_path_get_string (path)));
  gtk_file_info_set_is_folder (info, TRUE);
  return info;
}

static gboolean
root_file_folder_list_children (GtkFileFolder  *folder,
                                GSList        **children,
                                GError        **error)
{
  RootFileFolder *root_folder = ROOT_FILE_FOLDER (folder);
  GtkFileSystem *fs = root_folder->filesystem;

  GnomeVFSVolumeMonitor *monitor;
  GList *volumes, *v, *drives, *d;

  monitor = gnome_vfs_get_volume_monitor ();

  *children = NULL;

  volumes = gnome_vfs_volume_monitor_get_mounted_volumes (monitor);
  for (v = volumes; v; v = v->next)
    {
      GnomeVFSVolume *volume = v->data;

      if (gnome_vfs_volume_is_user_visible (volume))
        {
          const char *uri = gnome_vfs_volume_get_activation_uri (volume);
          *children = g_slist_append (*children,
                                      gtk_file_system_uri_to_path (fs, uri));
        }
    }
  g_list_free (volumes);

  drives = gnome_vfs_volume_monitor_get_connected_drives (monitor);
  for (d = drives; d; d = d->next)
    {
      GnomeVFSDrive *drive = d->data;

      if (gnome_vfs_drive_is_user_visible (drive))
        {
          char *uri =
            g_strdup_printf ("drive://%s",
                             gnome_vfs_drive_get_device_path (drive));
          *children = g_slist_append (*children,
                                      gtk_file_system_uri_to_path (fs, uri));
          g_free (uri);
        }
    }
  g_list_free (drives);

  return TRUE;
}

static gboolean
root_file_folder_is_finished_loading (GtkFileFolder *folder)
{
  return TRUE;
}

struct get_folder_clos {
  GtkFileSystemHandle *handle;
  RootFileFolder *root_folder;
  GtkFileSystemGetFolderCallback callback;
  gpointer data;
};

static gboolean
deliver_get_folder_callback (gpointer data)
{
  struct get_folder_clos *clos = (struct get_folder_clos *)data;
  GDK_THREADS_ENTER ();
  clos->callback (clos->handle, GTK_FILE_FOLDER (clos->root_folder),
                  NULL, clos->data);
  GDK_THREADS_LEAVE ();
  g_free (clos);
  return FALSE;
}

static GtkFileSystemHandle *
hildon_file_system_root_get_folder (HildonFileSystemSpecialLocation *location,
                                    GtkFileSystem                *filesystem,
                                    const GtkFilePath              *path,
                                    GtkFileInfoType                 types,
                                    GtkFileSystemGetFolderCallback  callback,
                                    gpointer                        data)
{
  GtkFileSystemHandle *handle = g_object_new (GTK_TYPE_FILE_SYSTEM_HANDLE,
                                              NULL);
  RootFileFolder *root_folder = g_object_new (ROOT_TYPE_FILE_FOLDER, NULL);
  struct get_folder_clos *clos = g_new (struct get_folder_clos, 1);

  root_folder->filesystem = filesystem;
  root_folder->root = HILDON_FILE_SYSTEM_ROOT (location);
  g_object_ref (location);

  /* This tells the HildonFileSystemModel never to call
     gtk_file_system_cancel_operation on this handle.
  */
  handle->file_system = NULL;

  clos->handle = handle;
  clos->root_folder = root_folder;
  clos->callback = callback;
  clos->data = data;

  g_idle_add (deliver_get_folder_callback, clos);
  return handle;
}
