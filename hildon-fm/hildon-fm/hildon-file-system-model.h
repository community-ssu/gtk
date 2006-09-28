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
/*
  HildonFileSystemModel.h
*/

#ifndef __HILDON_FILE_SYSTEM_MODEL_H__
#define __HILDON_FILE_SYSTEM_MODEL_H__

#include <glib-object.h>
#include <gtk/gtktreemodel.h>
#include "hildon-file-system-common.h"

G_BEGIN_DECLS
#define HILDON_TYPE_FILE_SYSTEM_MODEL (hildon_file_system_model_get_type())
#define HILDON_FILE_SYSTEM_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST (obj, HILDON_TYPE_FILE_SYSTEM_MODEL, \
  HildonFileSystemModel))
#define HILDON_FILE_SYSTEM_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_FILE_SYSTEM_MODEL, \
  HildonFileSystemModelClass))
#define HILDON_IS_FILE_SYSTEM_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE (obj, HILDON_TYPE_FILE_SYSTEM_MODEL))
#define HILDON_IS_FILE_SYSTEM_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_FILE_SYSTEM_MODEL))

typedef enum {
    HILDON_FILE_SYSTEM_MODEL_COLUMN_GTK_PATH = 0,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_LOCAL_PATH,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_URI,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_NAME,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_DISPLAY_NAME,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_SORT_KEY,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_MIME_TYPE,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_SIZE,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_TIME,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_FOLDER,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_HAS_LOCAL_PATH,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_TYPE,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON_EXPANDED,  /* Normal icon with
                                                       expanded emblem */
    HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON_COLLAPSED, /* Normal icon with 
                                                       collapsed emblem */
    HILDON_FILE_SYSTEM_MODEL_COLUMN_THUMBNAIL,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_LOAD_READY,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_FREE_SPACE, /* Only for devices */

    HILDON_FILE_SYSTEM_MODEL_COLUMN_TITLE,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_AUTHOR,

    HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_HIDDEN,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_UNAVAILABLE_REASON,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_FAILED_ACCESS_MESSAGE,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_SORT_WEIGHT,
    HILDON_FILE_SYSTEM_MODEL_COLUMN_EXTRA_INFO,

    HILDON_FILE_SYSTEM_MODEL_NUM_COLUMNS
} HildonFileSystemModelColumns;

/**
 * HildonFileSystemModelThumbnailCallback:
 * @uri: Location of the source file.
 * @path: Local path of the source file
 *        (can be %NULL if no local path is available).
 * @thumbnail_file: Name of the thumbnail file to be generated.
 * 
 * Setting this callback is depricated and it don't do anything.
 *
 * Returns: %TRUE, if thumbnail generation succeeded.
 */
typedef gboolean(*HildonFileSystemModelThumbnailCallback) (const gchar *
                                                           uri,
                                                           const gchar *
                                                           path,
                                                           const gchar *
                                                           thumbnail_file);

typedef struct _HildonFileSystemModelPrivate HildonFileSystemModelPrivate;

typedef struct _HildonFileSystemModel HildonFileSystemModel;

struct _HildonFileSystemModel {
    GObject parent;
    HildonFileSystemModelPrivate *priv;
};

typedef struct {
    GObjectClass parent_class;

    void (*finished_loading) (HildonFileSystemModel *model, GtkTreeIter *iter);
    void (*device_disconnected) (HildonFileSystemModel *model, 
      GtkTreeIter *iter);
} HildonFileSystemModelClass;

GType hildon_file_system_model_get_type(void);

/* Backwards compatibility */
#define hildon_file_system_model_create_backend hildon_file_system_create_backend
#ifndef HILDON_DISABLE_DEPRECATED
gboolean hildon_file_system_model_finished_loading(HildonFileSystemModel *
                                                   model);
#endif
/* These search from current model, nothing is loaded */

gboolean hildon_file_system_model_search_local_path(HildonFileSystemModel *
                                                    model,
                                                    const gchar * path,
                                                    GtkTreeIter * iter,
                                                    GtkTreeIter *
                                                    start_iter,
                                                    gboolean recursive);
gboolean hildon_file_system_model_search_uri(HildonFileSystemModel * model,
                                             const gchar * uri,
                                             GtkTreeIter * iter,
                                             GtkTreeIter * start_iter,
                                             gboolean recursive);
gboolean hildon_file_system_model_search_path(HildonFileSystemModel *
                                              model,
                                              const GtkFilePath * path,
                                              GtkTreeIter * iter,
                                              GtkTreeIter * start_iter,
                                              gboolean recursive);

/* Like previous ones, but now we try to load missing peaces */
gboolean hildon_file_system_model_load_local_path(HildonFileSystemModel * model,
                                            const gchar * path,
                                            GtkTreeIter * iter);
gboolean hildon_file_system_model_load_uri(HildonFileSystemModel * model,
                                            const gchar * uri,
                                            GtkTreeIter * iter);
gboolean hildon_file_system_model_load_path(HildonFileSystemModel * model,
                                            const GtkFilePath * path,
                                            GtkTreeIter * iter);

/* Returns new unique name to given directory */
gchar *hildon_file_system_model_new_item(HildonFileSystemModel * model,
                                         GtkTreeIter * parent,
                                         const gchar * stub_name,
                                         const gchar * extension);

/* Convenience functions to use the previous one */
gchar *hildon_file_system_model_autoname_uri(HildonFileSystemModel *model, 
  const gchar *uri, GError **error); 

void hildon_file_system_model_iter_available(HildonFileSystemModel *model,
  GtkTreeIter *iter, gboolean available);

void hildon_file_system_model_reset_available(HildonFileSystemModel *model);

/* Not for public use */

void _hildon_file_system_model_queue_reload(HildonFileSystemModel *model,
  GtkTreeIter *parent_iter, gboolean force);

GtkFileSystem
    *_hildon_file_system_model_get_file_system(HildonFileSystemModel *
                                               model);
gboolean _hildon_file_system_model_mount_device_iter(HildonFileSystemModel
                                                     * model,
                                                     GtkTreeIter * iter,
                                                     GError ** error);

void _hildon_file_system_model_prioritize_folder(HildonFileSystemModel *model,
                                                 GtkTreeIter *folder_iter);

G_END_DECLS
#endif
