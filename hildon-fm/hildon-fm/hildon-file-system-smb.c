/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2007 Nokia Corporation.  All rights reserved.
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

#include <glib.h>
#include <string.h>

#include "hildon-file-system-smb.h"
#include "hildon-file-system-settings.h"
#include "hildon-file-system-dynamic-device.h"
#include "hildon-file-common-private.h"
#include "hildon-file-system-private.h"

static void
hildon_file_system_smb_class_init (HildonFileSystemSmbClass *klass);
static void
hildon_file_system_smb_finalize (GObject *obj);
static void
hildon_file_system_smb_init (HildonFileSystemSmb *device);
static HildonFileSystemSpecialLocation*
hildon_file_system_smb_create_child_location (HildonFileSystemSpecialLocation
                                               *location, gchar *uri);

static gboolean
hildon_file_system_smb_is_visible (HildonFileSystemSpecialLocation *location);

static gboolean
hildon_file_system_smb_is_available (HildonFileSystemSpecialLocation *location);
static GtkFileSystemHandle *
hildon_file_system_smb_get_folder (HildonFileSystemSpecialLocation *location,
                                   GtkFileSystem                  *file_system,
                                   const GtkFilePath              *path,
                                   GtkFileInfoType                 types,
                                   GtkFileSystemGetFolderCallback  callback,
                                   gpointer                        data);

static GtkFileSystemHandle *
hildon_file_system_smb_get_workgroups_folder (GtkFileSystem *file_system,
                                              const GtkFilePath *path,
                                              GtkFileInfoType types,
                                              GtkFileSystemGetFolderCallback callback,
                                              gpointer data);

G_DEFINE_TYPE (HildonFileSystemSmb,
               hildon_file_system_smb,
               HILDON_TYPE_FILE_SYSTEM_REMOTE_DEVICE);

static void
hildon_file_system_smb_class_init (HildonFileSystemSmbClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    HildonFileSystemSpecialLocationClass *location =
            HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS (klass);

    gobject_class->finalize = hildon_file_system_smb_finalize;
    location->create_child_location =
      hildon_file_system_smb_create_child_location;
    location->get_folder = hildon_file_system_smb_get_folder;

    location->requires_access = FALSE;
    location->is_visible = hildon_file_system_smb_is_visible;
    location->is_available = hildon_file_system_smb_is_available;
}

static void
iap_connected_changed (GObject *settings, GParamSpec *param, gpointer data)
{
  HildonFileSystemSmb *smb = HILDON_FILE_SYSTEM_SMB (data);

  smb->has_children = FALSE;
  g_signal_emit_by_name (data, "connection-state");
  g_signal_emit_by_name (data, "rescan");
}

static void
hildon_file_system_smb_init (HildonFileSystemSmb *device)
{
    HildonFileSystemSettings *fs_settings;
    HildonFileSystemSpecialLocation *location;

    fs_settings = _hildon_file_system_settings_get_instance ();

    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (device);
    location->compatibility_type = HILDON_FILE_SYSTEM_MODEL_GATEWAY;
    location->fixed_icon = g_strdup ("qgn_list_filesys_samba");
    location->fixed_title = g_strdup (_("sfil_li_samba"));
    location->failed_access_message = NULL;

    device->has_children = FALSE;

    device->connected_handler_id =
      g_signal_connect (fs_settings,
                        "notify::iap-connected",
                        G_CALLBACK (iap_connected_changed),
                        device);
}

static void
hildon_file_system_smb_finalize (GObject *obj)
{
    HildonFileSystemSmb *smb;
    HildonFileSystemSettings *fs_settings;

    smb = HILDON_FILE_SYSTEM_SMB (obj);

    fs_settings = _hildon_file_system_settings_get_instance ();
    if (g_signal_handler_is_connected (fs_settings,
                                       smb->connected_handler_id))
      {
        g_signal_handler_disconnect (fs_settings,
                                     smb->connected_handler_id);
      }

    G_OBJECT_CLASS (hildon_file_system_smb_parent_class)->finalize (obj);
}

HildonFileSystemSpecialLocation*
hildon_file_system_smb_create_child_location (HildonFileSystemSpecialLocation
                                              *location, gchar *uri)
{
  HildonFileSystemSpecialLocation *child = NULL;
  gchar *skipped, *found;

  skipped = uri + strlen (location->basepath) + 1;
  found = strchr (skipped, G_DIR_SEPARATOR);

  if (found == NULL || found[1] == 0)
    {
      child = g_object_new(HILDON_TYPE_FILE_SYSTEM_DYNAMIC_DEVICE, NULL);
      HILDON_FILE_SYSTEM_REMOTE_DEVICE (child)->accessible =
        HILDON_FILE_SYSTEM_REMOTE_DEVICE (location)->accessible;
      hildon_file_system_special_location_set_icon (child,
                                                    "qgn_list_filesys_samba");
      child->failed_access_message = _("sfil_ib_cannot_connect_device");
      child->basepath = g_strdup (uri);
      HILDON_FILE_SYSTEM_SMB (location)->has_children = TRUE;
    }

  return child;
}

static gboolean
hildon_file_system_smb_is_visible (HildonFileSystemSpecialLocation *location)
{
  if (!HILDON_FILE_SYSTEM_SMB (location)->has_children)
    return FALSE;
  else
    return hildon_file_system_smb_is_available (location);
}

static gboolean
hildon_file_system_smb_is_available (HildonFileSystemSpecialLocation *location)
{
  HildonFileSystemSettings *fs_settings;
  gboolean connected;

  fs_settings = _hildon_file_system_settings_get_instance ();
  g_object_get (fs_settings, "iap-connected", &connected, NULL);
  return connected;
}

static GtkFileSystemHandle *
hildon_file_system_smb_get_folder (HildonFileSystemSpecialLocation *location,
                                   GtkFileSystem                  *file_system,
                                   const GtkFilePath              *path,
                                   GtkFileInfoType                 types,
                                   GtkFileSystemGetFolderCallback  callback,
                                   gpointer                        data)
{
  return hildon_file_system_smb_get_workgroups_folder (file_system,
                                                       path,
                                                       types,
                                                       callback,
                                                       data);
}


/* Collapsing GtkFileFolders

   XXX - This just implements enough to make the weird smb://
   namespace work.
 */

#define MY_TYPE_FILE_FOLDER             (my_file_folder_get_type ())
#define MY_FILE_FOLDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_FILE_FOLDER, MyFileFolder))
#define MY_IS_FILE_FOLDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_FILE_FOLDER))
#define MY_FILE_FOLDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MY_TYPE_FILE_FOLDER, MyFileFolderClass))
#define MY_IS_FILE_FOLDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MY_TYPE_FILE_FOLDER))
#define MY_FILE_FOLDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_FILE_FOLDER, MyFileFolderClass))

typedef struct _MyFileFolder      MyFileFolder;
typedef struct _MyFileFolderClass MyFileFolderClass;

struct _MyFileFolderClass
{
  GObjectClass parent_class;
};

struct _MyFileFolder
{
  GObject parent_instance;

  GtkFileSystem *filesystem;
  GtkFileInfoType types;
  GtkFileFolder *root;
  GList *children;
  int n_children_waiting;    /* The number of child folders we have
                                asked for but not yet received.
			     */
};

static GType my_file_folder_get_type (void);
static void my_file_folder_iface_init (GtkFileFolderIface *iface);
static void my_file_folder_init (MyFileFolder *impl);
static void my_file_folder_finalize (GObject *object);

static GtkFileInfo *my_file_folder_get_info (GtkFileFolder  *folder,
                                             const GtkFilePath    *path,
                                             GError        **error);
static gboolean my_file_folder_list_children (GtkFileFolder  *folder,
                                              GSList        **children,
                                              GError        **error);
static gboolean my_file_folder_is_finished_loading (GtkFileFolder *folder);

G_DEFINE_TYPE_WITH_CODE (MyFileFolder, my_file_folder, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_FILE_FOLDER,
                                                my_file_folder_iface_init))

static void
my_file_folder_class_init (MyFileFolderClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = my_file_folder_finalize;
}

static void
my_file_folder_iface_init (GtkFileFolderIface *iface)
{
  iface->get_info = my_file_folder_get_info;
  iface->list_children = my_file_folder_list_children;
  iface->is_finished_loading = my_file_folder_is_finished_loading;
}

static void
my_file_folder_init (MyFileFolder *impl)
{
  impl->filesystem = NULL;
  impl->root = NULL;
  impl->children = NULL;
}

static void
my_file_folder_finalize (GObject *object)
{
  MyFileFolder *folder = MY_FILE_FOLDER (object);
  GList *c;

  if (folder->root)
    g_object_unref (folder->root);

  for (c = folder->children; c; c = c->next)
    g_object_unref (c->data);
  g_list_free (folder->children);

  g_object_unref (folder->filesystem);

  G_OBJECT_CLASS (my_file_folder_parent_class)->finalize (object);
}

static GtkFileInfo *
my_file_folder_get_info (GtkFileFolder      *folder,
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
my_file_folder_list_children (GtkFileFolder  *folder,
                              GSList        **children,
                              GError        **error)
{
  MyFileFolder *my_folder = MY_FILE_FOLDER (folder);
  GList *c;

  *children = NULL;
  for (c = my_folder->children; c; c = c->next)
    {
      GSList *childrens_children;
      if (!gtk_file_folder_list_children (GTK_FILE_FOLDER (c->data),
                                          &childrens_children, error))
        {
          g_slist_free (*children);
          return FALSE;
        }
      *children = g_slist_append (*children, childrens_children);
    }

  return TRUE;
}

static gboolean
my_file_folder_is_finished_loading (GtkFileFolder *folder)
{
  MyFileFolder *my_folder = MY_FILE_FOLDER (folder);
  GList *c;

  if (!gtk_file_folder_is_finished_loading (GTK_FILE_FOLDER (my_folder->root)))
    return FALSE;

  if (my_folder->n_children_waiting > 0)
    return FALSE;

  for (c = my_folder->children; c; c = c->next)
    {
      if (!gtk_file_folder_is_finished_loading (GTK_FILE_FOLDER (c->data)))
        return FALSE;
    }
  return TRUE;
}

static char *
strchr_reverse (char *front, char *start, char ch)
{
  if (front == NULL)
    return NULL;

  if (start == NULL)
    start = front + strlen (front);

  while (start > front)
    {
      start--;
      if (*start == ch)
        return start;
    }

  return NULL;
}

static GtkFilePath *
my_file_folder_collaps_path (GtkFilePath *from)
{
  /* We collapse the second to last component.
   */

  const char *str = gtk_file_path_get_string (from);
  char *new_str = g_strdup (str);
  char *last_slash = strchr_reverse (new_str, NULL, '/');
  char *second_last_slash = strchr_reverse (new_str, last_slash, '/');

  if (second_last_slash)
    strcpy (second_last_slash + 1, last_slash + 1);

  return gtk_file_path_new_steal (new_str);
}

static void
my_file_folder_child_files_added (GtkFileFolder *folder,
                                 GSList *paths,
                                 gpointer data)
{
  MyFileFolder *my_folder = MY_FILE_FOLDER (data);

  GSList *my_paths;
  my_paths = NULL;
  while (paths)
    {
      GtkFilePath *p = paths->data;
      GtkFilePath *my_p = my_file_folder_collaps_path (p);

      my_paths = g_slist_append (my_paths, my_p);
      paths = paths->next;
    }

  g_signal_emit_by_name (my_folder, "files-added", my_paths);

  {
    GSList *p;
    for (p = my_paths; p; p = p->next)
      gtk_file_path_free (p->data);
    g_slist_free (my_paths);
  }
}

static void
my_file_folder_child_folder_added (GtkFileSystemHandle *handle,
                                  GtkFileFolder *folder,
                                  const GError *error,
                                  gpointer data)
{
  MyFileFolder *my_folder = MY_FILE_FOLDER (data);

  g_object_unref (handle);

  if (folder)
    {
      my_folder->children = g_list_append (my_folder->children,
                                           folder);
      my_folder->n_children_waiting--;
      g_signal_connect (folder, "files-added",
                        G_CALLBACK (my_file_folder_child_files_added),
                        my_folder);
    }
}


static void
my_file_folder_root_files_added (GtkFileFolder *folder,
                                 GSList *paths,
                                 gpointer data)
{
  MyFileFolder *my_folder = MY_FILE_FOLDER (data);

  while (paths)
    {
      GtkFilePath *p = paths->data;
      my_folder->n_children_waiting++;
      gtk_file_system_get_folder (my_folder->filesystem,
                                  p,
                                  my_folder->types,
                                  my_file_folder_child_folder_added,
                                  my_folder);
      paths = paths->next;
    }
}

static void
my_file_folder_setup_root (MyFileFolder *my_folder, GtkFileFolder *root)
{
  my_folder->root = root;
  g_signal_connect (root, "files-added",
                    G_CALLBACK (my_file_folder_root_files_added), my_folder);
}

struct get_root_folder_clos {
  GtkFileSystem *filesystem;
  GtkFileInfoType types;
  GtkFileSystemGetFolderCallback callback;
  gpointer data;
};

static void
get_root_folder_callback (GtkFileSystemHandle *handle,
                          GtkFileFolder *folder,
                          const GError *error,
                          gpointer data)
{
  struct get_root_folder_clos *clos = data;
  MyFileFolder *my_folder = NULL;

  if (!handle->cancelled && folder && error == NULL)
    {
      my_folder = g_object_new (MY_TYPE_FILE_FOLDER, NULL);
      my_folder->filesystem = clos->filesystem;
      my_folder->types = clos->types;
      my_file_folder_setup_root (my_folder, folder);
    }
  else
    {
      if (folder)
        g_object_unref (folder);
    }

  clos->callback (handle, GTK_FILE_FOLDER (my_folder), error, clos->data);
  g_free (clos);
}

static GtkFileSystemHandle *
hildon_file_system_smb_get_workgroups_folder (GtkFileSystem *file_system,
                                              const GtkFilePath *path,
                                              GtkFileInfoType types,
                                              GtkFileSystemGetFolderCallback callback,
                                              gpointer data)
{
  struct get_root_folder_clos *clos = g_new (struct get_root_folder_clos, 1);

  clos->filesystem = g_object_ref (file_system);
  clos->types = types;
  clos->callback = callback;
  clos->data = data;
  return gtk_file_system_get_folder (file_system,
                                     path,
                                     types,
                                     get_root_folder_callback,
                                     clos);
}

