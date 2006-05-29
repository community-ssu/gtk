/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
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
  HildonFileSelection widget
*/

#ifndef __HILDON_FILE_SELECTION_H__
#define __HILDON_FILE_SELECTION_H__

#include <gtk/gtkcontainer.h>
#include <gtk/gtkfilefilter.h>
#include "hildon-file-system-model.h"

G_BEGIN_DECLS
#define HILDON_TYPE_FILE_SELECTION (hildon_file_selection_get_type())
#define HILDON_FILE_SELECTION(obj) \
  (GTK_CHECK_CAST (obj, HILDON_TYPE_FILE_SELECTION, HildonFileSelection))
#define HILDON_FILE_SELECTION_CLASS(klass) \
  (GTK_CHECK_CLASS_CAST ((klass), HILDON_TYPE_FILE_SELECTION, \
  HildonFileSelectionClass))
#define HILDON_IS_FILE_SELECTION(obj) \
  (GTK_CHECK_TYPE (obj, HILDON_TYPE_FILE_SELECTION))
#define HILDON_IS_FILE_SELECTION_CLASS(klass) \
  (GTK_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_FILE_SELECTION))
/**
 * HildonFileSelectionMode:
 * @HILDON_FILE_SELECTION_MODE_LIST: List mode.
 * @HILDON_FILE_SELECTION_MODE_THUMBNAILS: Thumbnail mode.
 *
 * View mode used for content pane.
 */
    typedef enum {
    HILDON_FILE_SELECTION_MODE_LIST,
    HILDON_FILE_SELECTION_MODE_THUMBNAILS
} HildonFileSelectionMode;

/**
 * HildonFileSelectionSortKey:
 * @HILDON_FILE_SELECTION_SORT_NAME: Sort by name.
 *                                   This is the default setting.
 * @HILDON_FILE_SELECTION_SORT_TYPE: Sort by MIME type.
 * @HILDON_FILE_SELECTION_SORT_MODIFIED: Sort by modification time.
 * @HILDON_FILE_SELECTION_SORT_SIZE: Sort by file size. 
 * 
 * Defines the sort key used in content pane. Folders are always
 * sorted by name, regardless of this setting. Because of this
 * setting doesn't affect navigation pane.
 */
typedef enum {
    HILDON_FILE_SELECTION_SORT_NAME = 0,
    HILDON_FILE_SELECTION_SORT_TYPE,
    HILDON_FILE_SELECTION_SORT_MODIFIED,
    HILDON_FILE_SELECTION_SORT_SIZE
} HildonFileSelectionSortKey;

/**
 * HildonFileSelectionPane:
 * @HILDON_FILE_SELECTION_PANE_NAVIGATION: Navigation pane (left).
 * @HILDON_FILE_SELECTION_PANE_CONTENT: Content pane (right).
 *
 * Defines the logical panes. These can be used to query active
 * pane or change it. 
 */
typedef enum {
    HILDON_FILE_SELECTION_PANE_NAVIGATION = 0,
    HILDON_FILE_SELECTION_PANE_CONTENT
} HildonFileSelectionPane;

/**
 * HildonFileSelectionVisibleColumns:
 * @HILDON_FILE_SELECTION_SHOW_NAME: Show filename.
 * @HILDON_FILE_SELECTION_SHOW_MODIFIED: Show modification time.
 * @HILDON_FILE_SELECTION_SHOW_SIZE: Show file size. 
 * @HILDON_FILE_SELECTION_SHOW_ALL: Show all information.
 *
 * Defines what information about files and folder is shown in the context pane. You
 * Can combine the values any way you like.
 */
typedef enum {
    HILDON_FILE_SELECTION_SHOW_NAME = 1,
    HILDON_FILE_SELECTION_SHOW_MODIFIED = 2,
    HILDON_FILE_SELECTION_SHOW_SIZE = 4,
    HILDON_FILE_SELECTION_SHOW_ALL = 7
} HildonFileSelectionVisibleColumns;

typedef struct _HildonFileSelection HildonFileSelection;
typedef struct _HildonFileSelectionClass HildonFileSelectionClass;

/**
 * HildonFileSelectionPrivate:
 *
 * This structure contains just internal data and shouldn't be
 * accessed directly.
 */
typedef struct _HildonFileSelectionPrivate HildonFileSelectionPrivate;

struct _HildonFileSelection {
    GtkContainer parent;
    HildonFileSelectionPrivate *priv;
};

struct _HildonFileSelectionClass {
    GtkContainerClass parent_class;

    /* Application can hook the following signals to get informad about
       interesting events */
    void (*current_folder_changed) (HildonFileSelection * self);
    void (*file_activated) (HildonFileSelection * self);
    void (*selection_changed) (HildonFileSelection * self);
    void (*navigation_pane_context_menu) (HildonFileSelection * self);
    void (*content_pane_context_menu) (HildonFileSelection * self);
    void (*items_dropped) (HildonFileSelection * self,
                           GtkFilePath * destination, GSList * sources);
    void (*location_insensitive) (HildonFileSelection *self, GtkTreeIter *iter);
};

GType hildon_file_selection_get_type(void);
GtkWidget *hildon_file_selection_new_with_model(HildonFileSystemModel *
                                                model);

/* Mode for current location */
void hildon_file_selection_set_mode(HildonFileSelection * self,
                                    HildonFileSelectionMode mode);
HildonFileSelectionMode hildon_file_selection_get_mode(HildonFileSelection
                                                       * self);
void hildon_file_selection_set_sort_key(HildonFileSelection * self,
                                        HildonFileSelectionSortKey key,
                                        GtkSortType order);
void hildon_file_selection_get_sort_key(HildonFileSelection * self,
                                        HildonFileSelectionSortKey * key,
                                        GtkSortType * order);

/* The following methods are needed by GtkFileChooserIface */
gboolean hildon_file_selection_set_current_folder(HildonFileSelection *
                                                  self,
                                                  const GtkFilePath *
                                                  folder, GError ** error);
GtkFilePath *hildon_file_selection_get_current_folder(HildonFileSelection *
                                                      self);
#ifndef HILDON_DISABLE_DEPRECATED
gboolean hildon_file_selection_get_current_content_iter(HildonFileSelection
                                                        * self,
                                                        GtkTreeIter *
                                                        iter);
#endif
gboolean hildon_file_selection_get_current_folder_iter(HildonFileSelection
                                                       * self,
                                                       GtkTreeIter * iter);
#ifndef HILDON_DISABLE_DEPRECATED
gboolean hildon_file_selection_get_active_content_iter(HildonFileSelection
                                                       *self, GtkTreeIter *iter);
gboolean hildon_file_selection_content_iter_is_selected(HildonFileSelection *self,
                                                       GtkTreeIter *iter);
#endif

gboolean hildon_file_selection_select_path(HildonFileSelection * self,
                                           const GtkFilePath * path,
                                           GError ** error);
void hildon_file_selection_unselect_path(HildonFileSelection * self,
                                         const GtkFilePath * path);
void hildon_file_selection_select_all(HildonFileSelection * self);
void hildon_file_selection_unselect_all(HildonFileSelection * self);
void hildon_file_selection_clear_multi_selection(HildonFileSelection * self);
GSList *hildon_file_selection_get_selected_paths(HildonFileSelection *
                                                 self);

void hildon_file_selection_set_select_multiple(HildonFileSelection * self,
                                               gboolean select_multiple);
gboolean hildon_file_selection_get_select_multiple(HildonFileSelection *
                                                   self);

void hildon_file_selection_set_filter(HildonFileSelection * self,
                                      GtkFileFilter * filter);
GtkFileFilter *hildon_file_selection_get_filter(HildonFileSelection *
                                                self);

void hildon_file_selection_dim_current_selection(HildonFileSelection *self);
void hildon_file_selection_undim_all(HildonFileSelection *self);

HildonFileSelectionPane hildon_file_selection_get_active_pane(HildonFileSelection *self);

void hildon_file_selection_hide_content_pane(HildonFileSelection * self);
void hildon_file_selection_show_content_pane(HildonFileSelection * self);

/* Not for public use */
GSList *_hildon_file_selection_get_selected_files(HildonFileSelection
                                                       * self);
void _hildon_file_selection_realize_help(HildonFileSelection *self);

G_END_DECLS
#endif
