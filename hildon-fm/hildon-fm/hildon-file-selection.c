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
  HildonFileSelection widget

  Completely rewritten, now data structure is separated to GtkTreeModel
  compatible object.

  Issues concerning GtkTreeView::horizontal-separator
   * This space is concerned as cell area => alignment 1.0
                                          => end of text is clipped)
   * This comes also to the last column => Looks wery bad in all trees.
*/

#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <gdk/gdkkeysyms.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <libintl.h>
#include <osso-log.h>

#include "hildon-file-selection.h"
#include <hildon-widgets/gtk-infoprint.h>
#include <hildon-widgets/hildon-defines.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _(String) dgettext(PACKAGE, String)

/* I wonder where does that additional +2 come from. 
    Anyway I have to add it to make cell 60 + two 
    margin pixels high. Now height is pixel perfect. */
#define THUMBNAIL_CELL_HEIGHT (60 + HILDON_MARGIN_DEFAULT * 2 + 2)

/* Row height should be 30 and 4 is a mysterious constant.
    I just wonder why the constant is now 4 instead of 2
    used in thumbnail view??? */
#define LIST_CELL_HEIGHT (30 + 4)

/* These sizes are even more confusing. Magical constant is now 1!?# */
#define TREE_CELL_HEIGHT (30 + 1)

G_DEFINE_TYPE(HildonFileSelection, hildon_file_selection,
              GTK_TYPE_CONTAINER)

static GObject *hildon_file_selection_constructor(GType type,
                                                  guint
                                                  n_construct_properties,
                                                  GObjectConstructParam *
                                                  construct_properties);
static void
hildon_file_selection_set_current_folder_iter(HildonFileSelection * self,
                                              GtkTreeIter * main_iter);
static void hildon_file_selection_close_load_banner(HildonFileSelection *
                                                    self);
static gboolean view_path_to_main_iter(GtkTreeModel *model,
  GtkTreeIter *iter, GtkTreePath *path);
static void hildon_file_selection_real_row_insensitive(HildonFileSelection *self,
  GtkTreeIter *location);

static guint signal_folder_changed,
             signal_file_activated,
             signal_selection_changed; /* Signal ids */

static guint signal_navigation_pane_context_menu,
             signal_content_pane_context_menu;

static guint signal_items_dropped, signal_location_insensitive;

/* Property id:s */
enum {
    PROP_MODEL = 1,
    PROP_LOCAL_ONLY,
    PROP_DND,
    PROP_EMPTY_TEXT,
    PROP_VISIBLE_COLUMNS,
    PROP_SAFE_FOLDER,
    PROP_ACTIVE_PANE
};

struct _HildonFileSelectionPrivate {
    GtkWidget *scroll_dir;
    GtkWidget *scroll_list;
    GtkWidget *scroll_thumb;
    GtkWidget *dir_tree;
    GtkWidget *view[3]; /* List, thumbnail, empty */
    GtkWidget *separator;

    GtkTreeModel *main_model;
    GtkTreeModel *sort_model;   /* HildonFileSystemModel doesn't implement
                                   GtkTreeSortable */
    GtkTreeModel *dir_filter;
    GtkTreeModel *view_filter;

    GtkTreeRowReference *to_be_selected;
    GTimer *autosel_timer;
    GtkNotebook *view_selector;
    GtkFileFilter *filter;

    HildonFileSelectionMode mode;       /* User requested mode. Actual
                                           mode is either this or an empty
                                           view containing a message */
    guint banner_timeout_id;
    gboolean banner_shown;
    gboolean content_pane_last_used;

    /* Slightly kludgish flag. This is set when widget is shown and content pane
        should have focus, but there is nothing yet. We use this to move focus when
        there is some content */
    gboolean force_content_pane;

    /* Properties */

    HildonFileSelectionVisibleColumns visible_columns;
    gboolean drag_enabled;
    gboolean local_only;
    GtkFilePath *safe_folder;
};

static void scroll_to_cursor(GtkTreeView *tree)
{
  GtkTreePath *path;

  gtk_tree_view_get_cursor(tree, &path, NULL);

  if (path)
  {
    gtk_tree_view_scroll_to_cell(tree, path, NULL, FALSE, 0.0f, 0.0f);
    gtk_tree_path_free(path);
  }
}

/* Focuses the given view and scrolls to the cursor. This become
    quite complicated, because the cursor is not neccesarily set.
    We use cursor if it's found then we try selection and finally
    fall back to first item */
static void activate_view(GtkWidget *view)
{
  if (GTK_IS_TREE_VIEW(view) && GTK_WIDGET_DRAWABLE(view))
  {
    if (!GTK_WIDGET_HAS_FOCUS(view))
      gtk_widget_grab_focus(view);

    scroll_to_cursor(GTK_TREE_VIEW(view));
  }
}

static gboolean
hildon_file_selection_content_pane_visible(HildonFileSelectionPrivate *
                                           priv)
{
    return GTK_WIDGET_VISIBLE(GTK_WIDGET(priv->view_selector));
}

static GtkWidget *get_current_view(HildonFileSelectionPrivate * priv)
{
    if (hildon_file_selection_content_pane_visible(priv))
      return priv->view[gtk_notebook_get_current_page(priv->view_selector)];

    return NULL;
}

/* If there is no content AND loading is ready we switch to 
    information view. Otherwise we show curretly asked view */
static gint get_view_to_be_displayed(HildonFileSelectionPrivate *priv)
{
  GtkTreeModel *model, *child_model;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint result;

  model = priv->view_filter;

  /* We have no model => no files/folders */
  if (!model)
    return 2;

  /* We have something => display normal view */
  if (gtk_tree_model_get_iter_first(model, &iter))
    return priv->mode;

  /* We have currently nothing, check if we are loading */
  g_object_get(model, "child-model", &child_model, "virtual-root", &path, NULL);

  if (gtk_tree_model_get_iter(child_model, &iter, path))
  {
    gboolean ready;
    gtk_tree_model_get(child_model, &iter, HILDON_FILE_SYSTEM_MODEL_COLUMN_LOAD_READY, &ready, -1);
    result = ready ? 2 : gtk_notebook_get_current_page(priv->view_selector);
  }
  else /* Virtual root really must be found */
  {
    result = 2;
    g_assert_not_reached();
  }

  gtk_tree_path_free(path);
  g_object_unref(child_model);

  return result;
}

static void rebind_models(HildonFileSelectionPrivate *priv)
{
  GtkWidget *current_view = get_current_view(priv);

  if (priv->view[0] == current_view)
  {
     gtk_tree_view_set_model(GTK_TREE_VIEW(priv->view[0]),
        priv->view_filter);
     gtk_tree_view_set_model(GTK_TREE_VIEW(priv->view[1]), NULL);
  }
  else if (priv->view[1] == current_view)
  {
     gtk_tree_view_set_model(GTK_TREE_VIEW(priv->view[0]), NULL);
     gtk_tree_view_set_model(GTK_TREE_VIEW(priv->view[1]),
        priv->view_filter);
  }
  else
  {
     gtk_tree_view_set_model(GTK_TREE_VIEW(priv->view[0]), NULL);
     gtk_tree_view_set_model(GTK_TREE_VIEW(priv->view[1]), NULL);
  }
}

/* This method sync selections between listview and thumbnail view */
static void hildon_file_selection_sync_selections(HildonFileSelectionPrivate *priv,
                                                  GtkWidget * source, GtkWidget * target)
{
    GtkTreeSelection *selection;
    GList *paths, *iter;
    GtkTreePath *path;

    gtk_tree_view_get_cursor(GTK_TREE_VIEW(source), &path, NULL);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(source));
    paths = gtk_tree_selection_get_selected_rows(selection, NULL);

    rebind_models(priv);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(target));
    gtk_tree_selection_unselect_all(selection);

    /* Need to place cursor before selecting. Otherwise our patch removes the selection */
    if (path)
    {
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(target), path, NULL, FALSE);
      gtk_tree_path_free(path);
    }

    for (iter = paths; iter; iter = iter->next)
      gtk_tree_selection_select_path(selection, iter->data);

    g_list_foreach(paths, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(paths);
}

static gboolean expand_cursor_row(GtkTreeView *tree)
{
  GtkTreePath *path;
  gboolean result;

  gtk_tree_view_get_cursor(tree, &path, NULL);
  if (!path)
    return FALSE;

  result = gtk_tree_view_expand_row(tree, path, FALSE);
  gtk_tree_path_free(path);
  
  return result;
}

static void 
hildon_file_selection_inspect_view(HildonFileSelectionPrivate *priv)
{
    GtkWidget *view;
    gint current_page, target_page;
    gboolean content_focused = FALSE;

    if (!hildon_file_selection_content_pane_visible(priv))
        return;

    current_page = gtk_notebook_get_current_page(priv->view_selector);
    target_page = get_view_to_be_displayed(priv);
    view = get_current_view(priv);

    if (current_page != target_page)
    {
      content_focused = GTK_WIDGET_HAS_FOCUS(view) || 
          priv->force_content_pane || priv->content_pane_last_used;
      gtk_notebook_set_current_page(priv->view_selector, target_page);

      if (current_page == HILDON_FILE_SELECTION_MODE_THUMBNAILS &&
        target_page == HILDON_FILE_SELECTION_MODE_LIST)
        hildon_file_selection_sync_selections(priv, priv->view[1],
                                              priv->view[0]);
      else if (current_page == HILDON_FILE_SELECTION_MODE_LIST &&
        target_page == HILDON_FILE_SELECTION_MODE_THUMBNAILS)
        hildon_file_selection_sync_selections(priv, priv->view[0],
                                              priv->view[1]);
      else
        rebind_models(priv);

      view = get_current_view(priv);

      if (content_focused)
      {    
        if (!GTK_WIDGET_CAN_FOCUS(view))
          view = priv->dir_tree;

        activate_view(view);
      }
    }
    
    if (priv->force_content_pane && 
        expand_cursor_row(GTK_TREE_VIEW(priv->dir_tree)))
      priv->force_content_pane = FALSE;
}

static void hildon_file_selection_forall(GtkContainer * self,
                                         gboolean include_internals,
                                         GtkCallback callback,
                                         gpointer callback_data)
{
    g_return_if_fail(callback != NULL);

    if (include_internals) {
        HildonFileSelectionPrivate *priv =
            HILDON_FILE_SELECTION(self)->priv;

        (*callback) (priv->scroll_dir, callback_data);
        (*callback) (priv->separator, callback_data);
        (*callback) (GTK_WIDGET(priv->view_selector), callback_data);
    }
}

static void hildon_file_selection_size_request(GtkWidget * self,
                                               GtkRequisition *
                                               requisition)
{
    GtkWidget *content_view;
    GtkRequisition l_req, m_req, r_req;
    HildonFileSelectionPrivate *priv = HILDON_FILE_SELECTION(self)->priv;

    /* We call child requisition of our own children, but we ignore the
       results completely (scrolls don't return good values) */
    gtk_widget_size_request(priv->scroll_dir, &l_req);
    gtk_widget_size_request(priv->separator, &m_req);
    gtk_widget_size_request(GTK_WIDGET(priv->view_selector), &r_req);

    /* Trees know the needed sizes */
    content_view = get_current_view(priv);
    gtk_widget_get_child_requisition(priv->dir_tree, &l_req);

    if (content_view) {
        gtk_widget_get_child_requisition(content_view, &r_req);
        requisition->width = l_req.width + r_req.width;
        requisition->height = (l_req.height > r_req.height
                               ? l_req.height
                               : r_req.height);
        /* Separator height is not a concern */

        requisition->width += 25;       /* Scrollbar width */
    } else {
        requisition->width = l_req.width;
        requisition->height = l_req.height;
    }
}

static void hildon_file_selection_size_allocate(GtkWidget * self,
                                                GtkAllocation * allocation)
{
    GtkAllocation alloc;
    GtkRequisition req;
    HildonFileSelectionPrivate *priv = HILDON_FILE_SELECTION(self)->priv;
    GtkWidget *view;

    self->allocation = *allocation;

    if (hildon_file_selection_content_pane_visible(priv)) {
        alloc.x = allocation->x;
        alloc.y = allocation->y;
        alloc.width = (35 * allocation->width) / 100; /* 35% of space to
                                                      navigation pane */
        alloc.height = allocation->height;

        if (GTK_WIDGET_DRAWABLE(priv->separator)) {        
          gtk_widget_get_child_requisition(priv->separator, &req);
          alloc.width -= req.width;
          gtk_widget_size_allocate(priv->scroll_dir, &alloc);
          alloc.x += alloc.width;
          alloc.width = req.width;
          gtk_widget_size_allocate(priv->separator, &alloc);
       }
        else
          gtk_widget_size_allocate(priv->scroll_dir, &alloc);

        alloc.x += alloc.width;
        alloc.width = allocation->width - (alloc.x - allocation->x);

        /* <kludge> */

        view = get_current_view(priv);
        if (view == priv->view[0]) {  /* For list mode only */
            GtkTreeViewColumn *col;
            GtkRequisition win_req, tree_req;
            gint sum_width;
            GList *columns, *iter;

            /* Let's try a kludge to force name column not to be so wide.
               Don't try this at home ;) */

            gtk_widget_get_child_requisition(GTK_WIDGET
                                             (priv->view_selector),
                                             &win_req);
            gtk_widget_get_child_requisition(view, &tree_req);

            sum_width = (tree_req.height > allocation->height
                         ? win_req.width - tree_req.width
                         : 0);    /* Scrollbar width */

            g_assert(sum_width >= 0);
            columns = iter =
                gtk_tree_view_get_columns(GTK_TREE_VIEW(view));

            while (iter) {
                col = GTK_TREE_VIEW_COLUMN(iter->data);

                if (col->requested_width > 0)
                    sum_width += col->requested_width;

                iter = iter->next;
            }

            if (sum_width > alloc.width) {
                col = GTK_TREE_VIEW_COLUMN(g_list_nth_data(columns, 1));
                if (col->requested_width > sum_width - alloc.width)
                    col->requested_width -= sum_width - alloc.width;
                else
                    ULOG_WARN
                        ("Too little space for show the whole filetree.");
            }

            g_list_free(columns);
        }

        /* </kludge> */

        gtk_widget_size_allocate(GTK_WIDGET(priv->view_selector), &alloc);
    } else
        gtk_widget_size_allocate(priv->scroll_dir, allocation);
}

static void hildon_file_selection_finalize(GObject * obj)
{
    HildonFileSelection *self = HILDON_FILE_SELECTION(obj);
    HildonFileSelectionPrivate *priv = self->priv;

    ULOG_DEBUG("%s (%p)", __FUNCTION__, (gpointer) obj);

    /* We have to remove these by hand */
    g_signal_handlers_disconnect_by_func
        (priv->main_model,
         (gpointer) hildon_file_selection_close_load_banner,
         self);
    g_signal_handlers_disconnect_by_func
        (priv->main_model,
        (gpointer) hildon_file_selection_inspect_view,
        priv);

    g_source_remove_by_user_data(self); /* Banner checking timeout */
    gtk_widget_unparent(priv->scroll_dir);
    gtk_widget_unparent(priv->separator);

    gtk_widget_unparent(GTK_WIDGET(priv->view_selector));
    /* This gives warnings to console about unref_tree_helpers */
    /* This have homething to do with content pane filter model */

    /* Works also with NULLs */
    gtk_tree_row_reference_free(priv->to_be_selected);

    /* These objects really should dissappear. Otherwise we have a
        reference leak somewhere. */
    g_assert(G_OBJECT(priv->dir_filter)->ref_count == 1);

    g_object_unref(priv->dir_filter);
    if (priv->view_filter)
    {
      g_assert(G_OBJECT(priv->view_filter)->ref_count == 1);
      g_object_unref(priv->view_filter);
      priv->view_filter = NULL;
    }

    g_assert(G_OBJECT(priv->sort_model)->ref_count == 1);
    g_object_unref(priv->sort_model);

    /* Setting filter don't cause refiltering any more, 
        because view_filter is already set to NULL. Failing this setting caused 
        segfaults earlier. */
    hildon_file_selection_set_filter(self, NULL);   
    g_timer_destroy(self->priv->autosel_timer);

    G_OBJECT_CLASS(hildon_file_selection_parent_class)->finalize(obj);
}

static gboolean filter_func(GtkTreeModel * model, GtkTreeIter * iter,
                            gpointer data)
{
    gboolean is_folder, result, has_local_path;
    GtkFileFilterInfo info;
    HildonFileSelectionPrivate *priv = data;

    if (priv->local_only) {
        gtk_tree_model_get(model, iter,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_HAS_LOCAL_PATH,
                           &has_local_path, -1);
        if (!has_local_path)
            return FALSE;
    }

    /* All files are shown if no filter is present */
    if (!priv->filter)
        return TRUE;

    gtk_tree_model_get(model, iter,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_FOLDER,
                       &is_folder, -1);

    if (is_folder)      /* Folders are always displayed */
        return TRUE;

    memset(&info, 0, sizeof(GtkFileFilterInfo));
    info.contains = gtk_file_filter_get_needed(priv->filter);

    if (info.contains & GTK_FILE_FILTER_FILENAME)
        gtk_tree_model_get(model, iter,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_LOCAL_PATH,
                           &info.filename, -1);
    if (info.contains & GTK_FILE_FILTER_URI)
        gtk_tree_model_get(model, iter,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_URI, &info.uri,
                           -1);
    if (info.contains & GTK_FILE_FILTER_DISPLAY_NAME)
        gtk_tree_model_get(model, iter,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_NAME,
                           &info.display_name, -1);
    if (info.contains & GTK_FILE_FILTER_MIME_TYPE)
        gtk_tree_model_get(model, iter,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_MIME_TYPE,
                           &info.mime_type, -1);

/*  g_print("%d\t%s\t%s\t%s\t%s\n",
    info.contains, info.filename, info.uri,
    info.display_name, info.mime_type);*/

    result = gtk_file_filter_filter(priv->filter, &info);

    g_free((char *) info.filename);     /* Casting away const is safe in
                                           this case */
    g_free((char *) info.uri);
    g_free((char *) info.display_name);
    g_free((char *) info.mime_type);

    return result;
}

static gint sort_function(GtkTreeModel * model, GtkTreeIter * a,
                          GtkTreeIter * b, gpointer data)
{
    gboolean dir_a, dir_b, mmc_a, mmc_b;
    gint value, type_a, type_b;
    HildonFileSelectionSortKey key;
    GtkSortType order;
    char *mime_a, *mime_b;

    g_assert(HILDON_IS_FILE_SELECTION(data));

    gtk_tree_model_get(model, a, HILDON_FILE_SYSTEM_MODEL_COLUMN_TYPE,
                       &type_a, -1);
    gtk_tree_model_get(model, b, HILDON_FILE_SYSTEM_MODEL_COLUMN_TYPE,
                       &type_b, -1);

    /* Don't sort devices */
    if (type_a > HILDON_FILE_SYSTEM_MODEL_MMC
        || type_b > HILDON_FILE_SYSTEM_MODEL_MMC)
        return 0;

    hildon_file_selection_get_sort_key(HILDON_FILE_SELECTION(data), &key,
                                       &order);
    dir_a = (type_a >= HILDON_FILE_SYSTEM_MODEL_FOLDER);
    dir_b = (type_b >= HILDON_FILE_SYSTEM_MODEL_FOLDER);

    /* We have to manually reverse order if we always want to keep
       directories before files */
    if (dir_a != dir_b)
        return order ==
            GTK_SORT_ASCENDING ? (dir_a ? -1 : 1) : (dir_a ? 1 : -1);

    /* Ok, now both are either files or directories (or MMC:s) */
    mmc_a = (type_a == HILDON_FILE_SYSTEM_MODEL_MMC);
    mmc_b = (type_b == HILDON_FILE_SYSTEM_MODEL_MMC);

    /* MMC:s always go after directories (currently even when sorted to
       reverse order) */
    if (mmc_a != mmc_b)
        return order ==
            GTK_SORT_ASCENDING ? (mmc_a ? 1 : -1) : (mmc_a ? -1 : 1);

    /* Sort by name. This allways applies for directories and also for
       files when name sorting is selected */
    if (dir_a || key == HILDON_FILE_SELECTION_SORT_NAME) {
        gchar *title_a, *title_b;
        gint value;

        gtk_tree_model_get(model, a,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_SORT_KEY,
                           &title_a, -1);
        gtk_tree_model_get(model, b,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_SORT_KEY,
                           &title_b, -1);

        value = strcmp(title_a, title_b);

        /* Directories are always sorted alphabetically, so we 
            have to reverse order in descending mode. Actually
            we should do a separate sort model for navigation pane,
            but that would be a much bigger change. Temporarily we'll
            do this hack. */
        if (dir_a && order == GTK_SORT_DESCENDING)
          value = -value;

        g_free(title_a);
        g_free(title_b);
        return value;
    }

    if (key == HILDON_FILE_SELECTION_SORT_SIZE) {
        gint64 size_a, size_b;

        gtk_tree_model_get(model, a,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_SIZE,
                           &size_a, -1);
        gtk_tree_model_get(model, b,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_SIZE,
                           &size_b, -1);

        return size_a > size_b ? 1 : (size_a == size_b ? 0 : -1);
    }

    if (key == HILDON_FILE_SELECTION_SORT_MODIFIED) {
        GtkFileTime time_a, time_b;

        gtk_tree_model_get(model, a,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_TIME,
                           &time_a, -1);
        gtk_tree_model_get(model, b,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_TIME,
                           &time_b, -1);

        return time_a > time_b ? 1 : (time_a == time_b ? 0 : -1);
    }

    /* Note! Actually we should sort by extension, not by MIME type.
       Getting extension is also related to other problem */
    gtk_tree_model_get(model, a, HILDON_FILE_SYSTEM_MODEL_COLUMN_MIME_TYPE,
                       &mime_a, -1);
    gtk_tree_model_get(model, b, HILDON_FILE_SYSTEM_MODEL_COLUMN_MIME_TYPE,
                       &mime_b, -1);

    value = strcmp(mime_a, mime_b);

    g_free(mime_a);
    g_free(mime_b);

    return value;
}

static void hildon_file_selection_set_property(GObject * object,
                                               guint property_id,
                                               const GValue * value,
                                               GParamSpec * pspec)
{
    HildonFileSelectionPrivate *priv = HILDON_FILE_SELECTION(object)->priv;

    switch (property_id) {
    case PROP_MODEL:
        g_assert(priv->main_model == NULL);     /* We come here exactly
                                                   once */
        priv->main_model = GTK_TREE_MODEL(g_value_get_object(value));
        break;
    case PROP_DND:
        priv->drag_enabled = g_value_get_boolean(value);
        break;
    case PROP_LOCAL_ONLY:
    {
        gboolean new_state = g_value_get_boolean(value);

        if (new_state != priv->local_only) {
            priv->local_only = new_state;
            if (priv->view_filter) {
                gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER
                                               (priv->view_filter));
                hildon_file_selection_inspect_view(priv);
            }
        }
        break;
    }
    case PROP_EMPTY_TEXT:
        gtk_label_set_text(GTK_LABEL(priv->view[2]),
                           g_value_get_string(value));
        break;
    case PROP_VISIBLE_COLUMNS:
        priv->visible_columns = g_value_get_int(value);
        break;
    case PROP_SAFE_FOLDER:
        gtk_file_path_free(priv->safe_folder);  /* No need to check NULL */
        priv->safe_folder = g_value_get_boxed(value);
        break;
    case PROP_ACTIVE_PANE:
        activate_view(g_value_get_int(value) == HILDON_FILE_SELECTION_PANE_NAVIGATION ?
          priv->dir_tree : get_current_view(priv));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void hildon_file_selection_get_property(GObject * object,
                                               guint property_id,
                                               GValue * value,
                                               GParamSpec * pspec)
{
    HildonFileSelectionPrivate *priv = HILDON_FILE_SELECTION(object)->priv;

    switch (property_id) {
    case PROP_MODEL:
        g_value_set_object(value, priv->main_model);
        break;
    case PROP_DND:
        g_value_set_boolean(value, priv->drag_enabled);
        break;
    case PROP_LOCAL_ONLY:
        g_value_set_boolean(value, priv->local_only);
        break;
    case PROP_EMPTY_TEXT:
        g_value_set_string(value,
                           gtk_label_get_text(GTK_LABEL(priv->view[2])));
        break;
    case PROP_VISIBLE_COLUMNS:
        g_value_set_int(value, (gint) priv->visible_columns);
        break;
    case PROP_SAFE_FOLDER:
        g_value_set_boxed(value, priv->safe_folder);
        break;
    case PROP_ACTIVE_PANE:
        g_value_set_int(value, (gint) priv->content_pane_last_used);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void hildon_file_selection_map(GtkWidget *widget)
{
  GtkWidget *view;
  HildonFileSelectionPrivate *priv = HILDON_FILE_SELECTION(widget)->priv;

  GTK_WIDGET_CLASS(hildon_file_selection_parent_class)->map(widget);
  view = get_current_view(priv);

  activate_view(priv->dir_tree);

  if (hildon_file_selection_content_pane_visible(priv))
  {
    if (GTK_WIDGET_CAN_FOCUS(view))
    {
      expand_cursor_row(GTK_TREE_VIEW(priv->dir_tree));
      activate_view(view);
    }
    else
      priv->force_content_pane = TRUE;
  }
}

static void hildon_file_selection_class_init(HildonFileSelectionClass *
                                             klass)
{
    GParamSpec *pspec;
    GObjectClass *object;
    GtkWidgetClass *widget;

    hildon_file_selection_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(klass, sizeof(HildonFileSelectionPrivate));
    object = G_OBJECT_CLASS(klass);
    widget = GTK_WIDGET_CLASS(klass);

    object->constructor = hildon_file_selection_constructor;
    object->finalize = hildon_file_selection_finalize;
    object->set_property = hildon_file_selection_set_property;
    object->get_property = hildon_file_selection_get_property;
    widget->size_request = hildon_file_selection_size_request;
    widget->size_allocate = hildon_file_selection_size_allocate;
    widget->map = hildon_file_selection_map;

    /* Don't do actual show all, that would mess things up */
    widget->show_all = gtk_widget_show;
    GTK_CONTAINER_CLASS(klass)->forall = hildon_file_selection_forall;
    klass->location_insensitive = hildon_file_selection_real_row_insensitive;

  /**
   * HildonFileSelection::current-folder-changed:
   * @self: a #HildonFileSelection widget
   *
   * ::current-folder-changed signal is emitted when current folder
   * changes in navigation pane.
   * Use #hildon_file_selection_get_current_folder to receive folder path.
   */
    signal_folder_changed =
        g_signal_new("current-folder-changed", HILDON_TYPE_FILE_SELECTION,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HildonFileSelectionClass,
                                     current_folder_changed), NULL, NULL,
                     gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * HildonFileSelection::file-activated:
   * @self: a #HildonFileSelection widget
   *
   * ::file-activated signal is emitted when user selects an item from
   * content pane.
   */
    signal_file_activated =
        g_signal_new("file-activated", HILDON_TYPE_FILE_SELECTION,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HildonFileSelectionClass,
                                     file_activated), NULL, NULL,
                     gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * HildonFileSelection::selection-changed:
   * @self: a #HildonFileSelection widget
   *
   * This signal is emitted when selection changes on content pane. Use
   * #hildon_file_selection_get_selected_paths to receive paths.
   */
    signal_selection_changed =
        g_signal_new("selection-changed", HILDON_TYPE_FILE_SELECTION,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HildonFileSelectionClass,
                                     selection_changed), NULL, NULL,
                     gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * HildonFileSelection::navigation-pane-context-menu:
   * @self: a #HildonFileSelection widget
   *
   * This signal is emitted when a context menu (if any) should be
   * displayed on navigation pane.
   */
    signal_navigation_pane_context_menu =
        g_signal_new("navigation-pane-context-menu",
                     HILDON_TYPE_FILE_SELECTION, G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HildonFileSelectionClass,
                                     navigation_pane_context_menu), NULL,
                     NULL, gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * HildonFileSelection::content-pane-context-menu:
   * @self: a #HildonFileSelection widget
   *
   * This signal is emitted when a context menu (if any) should be
   * displayed on content pane.
   */
    signal_content_pane_context_menu =
        g_signal_new("content-pane-context-menu",
                     HILDON_TYPE_FILE_SELECTION, G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HildonFileSelectionClass,
                                     content_pane_context_menu), NULL,
                     NULL, gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * HildonFileSelection::items-dropped:
   * @self: a #HildonFileSelection widget
   * @destination: Destination path (this is quaranteed to be a directory).
   * @sources: List of paths that should be transferred to destination.
   *
   * This signal is emitted when user drags one or more items to some
   * folder. This signal is emitted only if drag'n'drop is enabled
   * during widget creation.
   */
    signal_items_dropped =
        g_signal_new("items-dropped", HILDON_TYPE_FILE_SELECTION,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HildonFileSelectionClass,
                                     items_dropped), NULL, NULL,
                     gtk_marshal_VOID__POINTER_POINTER, G_TYPE_NONE, 2,
                     G_TYPE_POINTER, G_TYPE_POINTER);
    /* Not portable to other languages */

  /**
   * HildonFileSelection::location-insensitive:
   * @self: a #HildonFileSelection widget
   * @location: insensitive location.
   *
   * This signal is emitted when user tries to interact with dimmed
   * location.
   */
    signal_location_insensitive =
        g_signal_new("location-insensitive", HILDON_TYPE_FILE_SELECTION,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HildonFileSelectionClass,
                                     location_insensitive), NULL, NULL,
                     g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 1,
                     GTK_TYPE_TREE_ITER);

    g_object_class_install_property(object, PROP_MODEL,
        g_param_spec_object("model",
                            "Data moodel",
                            "Set the HildonFileSystemModel to display",
                            HILDON_TYPE_FILE_SYSTEM_MODEL,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

    pspec = g_param_spec_boolean("drag-enabled", "Drag\'n\'' drop enabled",
                                 "Ask file selection to enable "
                                 "drag\'n\'drop support",
                                 FALSE,
                                 G_PARAM_CONSTRUCT_ONLY |
                                 G_PARAM_READWRITE);
    g_object_class_install_property(object, PROP_DND, pspec);

    pspec = g_param_spec_string("empty-text", "Empty text",
                                "String to use when selected folder "
                                "is empty",
                                NULL, G_PARAM_READWRITE);
    g_object_class_install_property(object, PROP_EMPTY_TEXT, pspec);

    g_object_class_install_property(object, PROP_VISIBLE_COLUMNS,
      g_param_spec_int("visible-columns", "Visible columns",
                                     "Defines the columns shown on content pane",
                                     0, HILDON_FILE_SELECTION_SHOW_ALL,
                                     HILDON_FILE_SELECTION_SHOW_ALL,
                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object, PROP_SAFE_FOLDER,
      g_param_spec_boxed("safe-folder", "Safe folder",
                                     "Safe folder to use as fallback in various operations",
                                     GTK_TYPE_FILE_PATH, G_PARAM_READWRITE));                                      

    g_object_class_install_property(object, PROP_ACTIVE_PANE,
      g_param_spec_int("active-pane", "Active pane",
                                     "Which pane (navigation or content) has (or last time had) active focus.\n"
                                     "This in formation can be used to find which pane should be used to query selection.",
                                     HILDON_FILE_SELECTION_PANE_NAVIGATION,
                                     HILDON_FILE_SELECTION_PANE_CONTENT, 
                                     HILDON_FILE_SELECTION_PANE_NAVIGATION, G_PARAM_READWRITE));
}

static void hildon_file_selection_content_pane_selection_changed(GtkWidget
                                                                 * widget,
                                                                 gpointer
                                                                 data)
{
    g_assert(HILDON_IS_FILE_SELECTION(data));
    g_signal_emit(data, signal_selection_changed, 0);
}
#if 0
/* Checks whether the given path matches current content pane path */
static gboolean
hildon_file_selection_matches_current_view(HildonFileSelectionPrivate *
                                           priv, GtkTreePath * path)
{
    if (GTK_IS_TREE_MODEL_FILTER(priv->view_filter)) {
        GtkTreePath *current_path;
        gint result;

        g_object_get(priv->view_filter, "virtual-root", &current_path,
                     NULL);
        g_assert(current_path); /* Content pane should always have root
                                   */

        result = gtk_tree_path_compare(path, current_path);
        gtk_tree_path_free(current_path);

        return (result == 0);
    }

    return FALSE;
}
#endif
static gboolean hildon_file_selection_check_load_banner(gpointer data)
{
    HildonFileSelection *self;
    GtkTreeIter iter;
    gboolean ready;

    self = HILDON_FILE_SELECTION(data);
    
    if (hildon_file_selection_get_current_folder_iter(self, &iter))
    {
      gtk_tree_model_get(self->priv->main_model, &iter,
        HILDON_FILE_SYSTEM_MODEL_COLUMN_LOAD_READY, &ready, -1);

      if (!ready)
      {
        GtkWidget *window =
            gtk_widget_get_ancestor(GTK_WIDGET(data), GTK_TYPE_WINDOW);

        if (window) {
            self->priv->banner_shown = TRUE;
            ULOG_DEBUG("Showing update banner");
            gtk_banner_show_animation(GTK_WINDOW(window), _("ckdg_pb_updating"));
        }
      }
    }

    self->priv->banner_timeout_id = 0;
    return FALSE;       /* Don't call again */
}

static void hildon_file_selection_close_load_banner(HildonFileSelection *
                                                    self)
{
    GtkWidget *window =
        gtk_widget_get_ancestor(GTK_WIDGET(self), GTK_TYPE_WINDOW);

    if (window) {
        self->priv->banner_shown = FALSE;
        gtk_banner_close(GTK_WINDOW(window));
    }

    /* Load can finish with no contents */
    hildon_file_selection_inspect_view(self->priv);
}

static void 
hildon_file_selection_check_close_load_banner(GtkTreeModel *model, 
  GtkTreeIter *iter, gpointer data)
{
  GtkTreeIter current_iter;
  HildonFileSelection *self = HILDON_FILE_SELECTION(data);

  if (hildon_file_selection_get_current_folder_iter(self, &current_iter) &&
      iter->user_data == current_iter.user_data)
    hildon_file_selection_close_load_banner(self);
}

static void
get_safe_folder_tree_iter(HildonFileSelectionPrivate *priv, GtkTreeIter *iter)
{
  GtkTreeIter main_iter, sort_iter;

  if (priv->safe_folder == NULL || 
      !hildon_file_system_model_search_path(
        HILDON_FILE_SYSTEM_MODEL(priv->main_model), 
        priv->safe_folder, &main_iter, NULL, TRUE))
  {
    gboolean success;
    success = gtk_tree_model_get_iter_first(priv->main_model, &main_iter);
    g_assert(success);  /* This really should work */
  }
      
  gtk_tree_model_sort_convert_child_iter_to_iter(GTK_TREE_MODEL_SORT
                                                   (priv->sort_model),
                                                   &sort_iter, &main_iter);
  gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER
                                                     (priv->dir_filter), iter, &sort_iter);
}

static gboolean delayed_select_idle(gpointer data)
{
  GtkTreeSelection *selection;
  GtkTreeView *tree;
  GtkTreePath *path;
  GtkTreeIter iter, parent;
  HildonFileSelectionPrivate *priv = HILDON_FILE_SELECTION(data)->priv;

  if (!priv->to_be_selected)
    return FALSE;

  path = gtk_tree_row_reference_get_path(priv->to_be_selected);

  if (!path) /* Row reference is invalidated. Try a safe folder */
  {
    get_safe_folder_tree_iter(priv, &iter);
    path = gtk_tree_model_get_path(priv->dir_filter, &iter);
  }
  else if (g_timer_elapsed(priv->autosel_timer, NULL) < 0.4)
  {
      /* Fast updates in a row are probably caused
          by a deleted folder and we are updating the tree.
         In this case we have special method for updating the cursor. */

    gboolean root_found = FALSE, available;
    HildonFileSystemModelItemType type;

    if (gtk_tree_model_get_iter(priv->dir_filter, &iter, path))
      while (TRUE)
      {
        gtk_tree_model_get(priv->dir_filter, &iter,
          HILDON_FILE_SYSTEM_MODEL_COLUMN_TYPE, &type, 
          HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE, &available, 
          -1);

        if (type >= HILDON_FILE_SYSTEM_MODEL_MMC) {
          root_found = available;
          break;
        }

        if (!gtk_tree_model_iter_parent(priv->dir_filter, &parent, &iter))
          break;
  
        iter = parent;
      }

    if (!root_found) /* Moving to safe folder */
      get_safe_folder_tree_iter(priv, &iter);

    gtk_tree_path_free(path);
    path = gtk_tree_model_get_path(priv->dir_filter, &iter);
  }

  g_assert(path != NULL);
  tree = GTK_TREE_VIEW(priv->dir_tree);
  gtk_tree_view_expand_to_path(tree, path);
  gtk_tree_view_scroll_to_cell(tree, path, NULL, FALSE, 0, 0);
  selection = gtk_tree_view_get_selection(tree);
  gtk_tree_selection_select_path(selection, path);
  gtk_tree_path_free(path);

  /* This should not be needed, but appearently some bug in GtkTreeView 
      patch requires this. Otherwise focus is totally lost. */
  activate_view(priv->dir_tree); 

  gtk_tree_row_reference_free(priv->to_be_selected);
  priv->to_be_selected = NULL;
  g_timer_start(priv->autosel_timer);

  return FALSE;
}

static void
hildon_file_selection_delayed_select_path(HildonFileSelection *self, 
  GtkTreePath *dir_filter_path)
{
  if (self->priv->to_be_selected) 
    gtk_tree_row_reference_free(self->priv->to_be_selected);
  else  /* If we don't have previous path, then we need new idle */
     g_idle_add(delayed_select_idle, self);

   self->priv->to_be_selected = gtk_tree_row_reference_new(self->priv->dir_filter, dir_filter_path);
}

static gboolean path_is_visible(GtkTreeView *view, GtkTreePath *path)
{
  GtkTreePath *temp;
  gboolean result;

  if (gtk_tree_path_get_depth(path) <= 1)
    return TRUE;

  temp = gtk_tree_path_copy(path);
  gtk_tree_path_up(temp);
  result = gtk_tree_view_row_expanded(view, temp);
  gtk_tree_path_free(temp);
  return result;
}

static void hildon_file_selection_row_insensitive(GtkTreeView *tree,
  GtkTreePath *path, gpointer data)
{
  GtkTreeIter iter;

  if (view_path_to_main_iter(gtk_tree_view_get_model(tree), &iter, path))
    g_signal_emit(data, signal_location_insensitive, 0, &iter);
}

/* By default, we handle dimmed MMC and gateway. If applications do not want
   this to happen, signal emission can be stopped */
static void hildon_file_selection_real_row_insensitive(HildonFileSelection *self,
  GtkTreeIter *location)
{
  HildonFileSystemModelItemType type;
  GtkWindow *window;

  g_assert(HILDON_IS_FILE_SELECTION(self));
  
  gtk_tree_model_get(self->priv->main_model, location,
      HILDON_FILE_SYSTEM_MODEL_COLUMN_TYPE, &type,
      -1);

  /* The following call can (in theory) return NULL. That's why use C style cast */
  window = (GtkWindow *) gtk_widget_get_ancestor(GTK_WIDGET(self), GTK_TYPE_WINDOW);

  if (type == HILDON_FILE_SYSTEM_MODEL_MMC)
    gtk_infoprint(window, "hfil_ib_mmc_not_present");
  else if (type == HILDON_FILE_SYSTEM_MODEL_GATEWAY)
    gtk_infoprint(window, "sfil_ib_no_connections_flightmode");
}

static void hildon_file_selection_selection_changed(GtkTreeSelection *
                                                    selection,
                                                    gpointer data)
{
    HildonFileSelectionPrivate *priv;
    GtkTreeModel *model;        /* Navigation pane filter model */
    GtkTreeIter iter;

    g_assert(HILDON_IS_FILE_SELECTION(data));
    priv = HILDON_FILE_SELECTION(data)->priv;
    priv->force_content_pane = FALSE;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {

        GtkTreeIter sort_iter, main_iter;
        GtkTreePath *sort_path;
        GError *error = NULL;
        gboolean success;

        g_assert(model == priv->dir_filter
          && GTK_IS_TREE_MODEL_FILTER(model));

        gtk_tree_model_filter_convert_iter_to_child_iter
            (GTK_TREE_MODEL_FILTER(priv->dir_filter), &sort_iter, &iter);
        gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT
                                                       (priv->sort_model),
                                                       &main_iter,
                                                       &sort_iter);

        if (hildon_file_selection_content_pane_visible(priv)) {
            sort_path =
                gtk_tree_model_get_path(priv->sort_model, &sort_iter);
#if 0
            /* Check that we have actually changed the folder */
            if (hildon_file_selection_matches_current_view
                (priv, sort_path)) {
                gtk_tree_path_free(sort_path);
                return;
            }
#endif
            if (priv->view_filter)
                g_object_unref(priv->view_filter);

            priv->view_filter =
                gtk_tree_model_filter_new(priv->sort_model, sort_path);
            gtk_tree_path_free(sort_path);

            g_assert(priv->view_filter != NULL);
            gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER
                                                   (priv->view_filter),
                                                   filter_func, priv,
                                                   NULL);

            rebind_models(priv);            
            hildon_file_selection_inspect_view(priv);

            /* These DON'T affect colums that have AUTOSIZE as sizing type
               ;) */
            gtk_tree_view_columns_autosize(GTK_TREE_VIEW(priv->view[0]));
            gtk_tree_view_columns_autosize(GTK_TREE_VIEW(priv->view[1]));

            /* All of the refresh problems seem to happen only with
               matchbox. Some only if we have not enough space, some only
               if we run with Teema */

            /* Hmmmm... In problem cases we run just size request, not
               size allocate. With fvwm we run size allocate in all cases. 
             */
            /* When there is not set_size_request, or requested -1 then
               this fails. Any positive value works fine */

            /* This was finally a clear Matchbox bug. This was caused by
               unreceived configure event that caused screen freeze. */
        }
        g_signal_emit(data, signal_folder_changed, 0);

	hildon_file_selection_close_load_banner(HILDON_FILE_SELECTION(data));
        if (!priv->banner_shown && priv->banner_timeout_id == 0)
            priv->banner_timeout_id =
                g_timeout_add(500, hildon_file_selection_check_load_banner,
                              data);

        /* This can invalidate non-persisting iterators, so we need to do
           mounting as last action. */
        success =
            _hildon_file_system_model_mount_device_iter
            (HILDON_FILE_SYSTEM_MODEL(priv->main_model), &main_iter,
             &error);
        if (!success && error)
        {
            char *name;
            GtkWidget *window;

            window = gtk_widget_get_ancestor(GTK_WIDGET(data), GTK_TYPE_WINDOW);

            gtk_tree_model_get(priv->main_model, &main_iter, 
              HILDON_FILE_SYSTEM_MODEL_COLUMN_DISPLAY_NAME, &name,
              -1);

            gtk_infoprintf((GtkWindow *) window, _("sfil_ib_cannot_connect_device"), name);

            ULOG_ERR("Mount failed (error #%d): %s", error->code, error->message);
            g_error_free(error);
            g_free(name);
        }
    }
    else if (priv->view_filter)
    {
      GtkTreePath *current_path, *temp_path;

      g_object_get(priv->view_filter, "virtual-root", &temp_path, NULL);
      g_assert(temp_path); /* Content pane should always have root
                                   (no device level allowed) */

      current_path = gtk_tree_model_filter_convert_child_path_to_path(
        GTK_TREE_MODEL_FILTER(priv->dir_filter), temp_path);
      gtk_tree_path_free(temp_path);

      g_object_unref(priv->view_filter);
      priv->view_filter = NULL;

      gtk_tree_view_set_model(GTK_TREE_VIEW(priv->view[0]), NULL);
      gtk_tree_view_set_model(GTK_TREE_VIEW(priv->view[1]), NULL);
      hildon_file_selection_inspect_view(priv);

      while (gtk_tree_path_up(current_path))
      {
        if (path_is_visible(GTK_TREE_VIEW(priv->dir_tree), current_path))
        {
          hildon_file_selection_delayed_select_path(HILDON_FILE_SELECTION(data), current_path);
          break;
        }
      }

      gtk_tree_path_free(current_path);
    }
}

static void hildon_file_selection_row_activated(GtkTreeView * view,
                                                GtkTreePath * path,
                                                GtkTreeViewColumn * col,
                                                gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    HildonFileSelection *self;
/*    GtkWidget *content_view;*/

    self = HILDON_FILE_SELECTION(data);

    model = gtk_tree_view_get_model(view); /* Content pane filter model */

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gboolean is_folder, is_available;

        gtk_tree_model_get(model, &iter,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_FOLDER,
                           &is_folder, 
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE,
                           &is_available, -1);

        if (is_folder) {
          if (is_available) {
            GtkTreePath *sort_path, *filter_path;
/*            GtkTreeIter sort_iter, main_iter;*/
/*
            gtk_tree_model_filter_convert_iter_to_child_iter
                (GTK_TREE_MODEL_FILTER(model), &sort_iter, &iter);
            gtk_tree_model_sort_convert_iter_to_child_iter
                (GTK_TREE_MODEL_SORT(self->priv->sort_model), &main_iter,
                 &sort_iter);
*/
            sort_path = gtk_tree_model_filter_convert_path_to_child_path(
              GTK_TREE_MODEL_FILTER(model), path);
            filter_path = gtk_tree_model_filter_convert_child_path_to_path(
              GTK_TREE_MODEL_FILTER(self->priv->dir_filter), sort_path);

            hildon_file_selection_delayed_select_path(self, filter_path);
            gtk_tree_path_free(sort_path);
            gtk_tree_path_free(filter_path);

/*      gboolean result;
        printf("Row \"%s\" activated\n", (const gchar *) path);
        result =*/
 /*           hildon_file_selection_set_current_folder_iter(self,
                                                          &main_iter);

            content_view = get_current_view(self->priv);
            if (GTK_IS_TREE_VIEW(content_view))
              activate_view(content_view);
            else
              activate_view(self->priv->dir_tree);
*/
/*          printf("Result = %d\n", result);*/
          }
        } else {
/*          printf("Selected item \"%s\" was not a directory. "
                   "Sending signal...\n", (const gchar *) path); */
            g_signal_emit(data, signal_file_activated, 0);
        }
    }
}

static const char *get_date_string(GtkFileTime file_time,
                                   const gchar * format_today,
                                   const gchar * format_current_year,
                                   const gchar * format_other)
{
    static char buffer[256];
    GDate ftime, now;
    struct tm *time_struct;
    time_t time_val;

    /* Bad, but this was also used in GtkFileChooserDefault */
    g_date_set_time(&ftime, (GTime) file_time);
    g_date_set_time(&now, (GTime) time(NULL));

    /* Too bad. We cannot use GDate function, because it doesn't handle
       time, just dates */
    time_val = (time_t) file_time;
    time_struct = localtime(&time_val);

    if (ftime.year == now.year)
    {
      if (ftime.month == now.month && ftime.day == now.day)
        strftime(buffer, sizeof(buffer), format_today, time_struct);
      else
        strftime(buffer, sizeof(buffer), format_current_year, time_struct);
    }
    else
        strftime(buffer, sizeof(buffer), format_other, time_struct);

    return buffer;
}

static void thumbnail_data_func(GtkTreeViewColumn * tree_column,
                                GtkCellRenderer * cell,
                                GtkTreeModel * model, GtkTreeIter * iter,
                                gpointer data)
{
    HildonFileSystemModelItemType type;
    PangoAttrList *attrs = NULL;
    PangoAttribute *attr;
    gint64 file_time;
    char *title;
    gchar buffer[256];
    gchar buffer2[256];
    const gchar *line2;
    gchar *thumb_title = NULL, *thumb_author = NULL;
    gboolean sensitive, found1, found2;
    GdkColor color1, color2;
    gint bytes1, bytes2;

    gtk_tree_model_get(model, iter,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_DISPLAY_NAME,
                       &title,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_TYPE,
                       &type,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_TIME,
                       &file_time, 
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE, 
                       &sensitive,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_TITLE,
                       &thumb_title,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_AUTHOR,
                       &thumb_author, -1);

    if (type == HILDON_FILE_SYSTEM_MODEL_MMC) 
    {
      gboolean available;
      gint64 free_space;

      gtk_tree_model_get(model, iter, 
          HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE, &available,
          HILDON_FILE_SYSTEM_MODEL_COLUMN_FREE_SPACE, &free_space, -1);

      /* Of course the catecories and names are not the 
	 same here than in list view */
      if (!available)
        buffer2[0] = 0;
      else if (free_space < 1024 * 1024)
        g_snprintf(buffer2, sizeof(buffer2), _("sfil_li_mmc_free_kb"), 
            (gint) free_space / 1024);
      else
        g_snprintf(buffer2, sizeof(buffer2), _("sfil_li_mmc_free_mb"), 
            (gint) free_space / (1024 * 1024));

      line2 = buffer2;
    }
    else if (type >= HILDON_FILE_SYSTEM_MODEL_FOLDER) 
    {
        gint n = gtk_tree_model_iter_n_children(model, iter);

        g_snprintf(buffer2, sizeof(buffer2),
                   dngettext(PACKAGE,
                             "sfil_li_folder_contents_item",
                             "sfil_li_folder_contents_items", n),  n );
	line2 = buffer2;
    }
    else
    {
        line2 = get_date_string(file_time,
				_("sfil_li_modified_today"),
				_("sfil_li_modified_thisyear"),
				_("sfil_li_modified_earlier"));
	
    }

    if(thumb_title && thumb_title[0] )
    {
      g_free(title);
      title = thumb_title;
    }

    if(thumb_author && thumb_author[0])
    {
      strncpy(buffer2, thumb_author, sizeof(buffer2));
      line2 = buffer2;
      g_free(thumb_author);
    }

    /* Colors set this way do not seem to dim automatically. 
     * So this is only for active cells */
    if (sensitive)  
    {
      found1 = gtk_style_lookup_logical_color(GTK_WIDGET(data)->style, 
					      "DefaultTextColor", &color1);
      found2 = gtk_style_lookup_logical_color(GTK_WIDGET(data)->style, 
					      "SecondaryTextColor", &color2);

      attrs = pango_attr_list_new();
      bytes1 = strlen(title);
      bytes2 = strlen(line2);

      if (found1)
      {
        attr = pango_attr_foreground_new(color1.red, color1.green, color1.blue);
        attr->start_index = 0;
        attr->end_index = bytes1;
        pango_attr_list_insert(attrs, attr);
      }

      if (found2)
      {
        attr = pango_attr_foreground_new(color2.red, color2.green, color2.blue);
        attr->start_index = bytes1 + 1;
        attr->end_index = bytes1 + bytes2 + 1;
        pango_attr_list_insert(attrs, attr);
      } 
    }

    /* No localization needed for this one */
    g_snprintf(buffer, sizeof(buffer), "%s\n%s", title, line2); 
    g_object_set(cell, "text", buffer, "attributes", attrs, 
		 "sensitive", sensitive, NULL);

    if (attrs)
      pango_attr_list_unref(attrs);

    g_free(title);
}
	

static void list_date_data_func(GtkTreeViewColumn * tree_column,
                                GtkCellRenderer * cell,
                                GtkTreeModel * tree_model,
                                GtkTreeIter * iter, gpointer data)
{
    gint64 file_time;
    gboolean is_folder, sensitive;
    const gchar *date_string;

    gtk_tree_model_get(tree_model, iter,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_FOLDER,
                       &is_folder,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_TIME,
                       &file_time, 
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE,
                       &sensitive, -1);

    /* No date information for folders in list view */
    date_string = is_folder ? "" : get_date_string
      (file_time, 
       _("sfil_li_date_today"), _("sfil_li_date_this_year"), 
       _("sfil_li_date_other"));

    g_object_set(cell, "text", date_string, "sensitive", sensitive, NULL);
}

static void list_size_data_func(GtkTreeViewColumn * tree_column,
                                GtkCellRenderer * cell,
                                GtkTreeModel * tree_model,
                                GtkTreeIter * iter, gpointer data)
{
    char buffer[256];
    gint64 size;
    gboolean is_folder, sensitive;

    gtk_tree_model_get(tree_model, iter,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_FOLDER,
                       &is_folder,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_SIZE, &size,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE,
                       &sensitive, -1);

    /* No size info for folders in list view */

    if (is_folder)
        buffer[0] = 0; 
    else if (size < (gint64) 100)
        g_snprintf(buffer, sizeof(buffer), 
            _("sfil_li_size_max_100B"), (gint) size); /* safe conversion */
    else if (size < (gint64) 1024)
        g_snprintf(buffer, sizeof(buffer), 
            _("sfil_li_size_100B_10kB"), size / 1024.0f);
    else if (size < (gint64) 100 * 1024)
        g_snprintf(buffer, sizeof(buffer), 
            _("sfil_li_size_10kB_100kB"), (gint) size / 1024);  /* safe */
    else if (size < (gint64) 10 * 1024 * 1024)
        g_snprintf(buffer, sizeof(buffer), 
            _("sfil_li_size_100kB_10MB"), size / (1024.0f * 1024.0f));
    else if (size < (gint64) 100 * 1024 * 1024)
        g_snprintf(buffer, sizeof(buffer), 
            _("sfil_li_size_10MB_100MB"), (gint) size / (1024 * 1024));
    else if (size < 10 * ((gint64) 1024 * 1024 * 1024))
        g_snprintf(buffer, sizeof(buffer), 
            _("sfil_li_size_100MB_10GB"), size / (1024.0 * 1024.0 * 1024.0));
    else
        /* Following calculation doesn't fit into 32 bits if the filesize is larger than 2^62 ;) */
        g_snprintf(buffer, sizeof(buffer), 
            _("sfil_li_size_more_than_100GB"), (gint) (size / (1024 * 1024 * 1024)));

    g_object_set(cell, "text", buffer, "sensitive", sensitive, NULL);
}

static void hildon_file_selection_navigation_pane_context(GtkWidget *
                                                          widget,
                                                          gpointer data)
{
    g_assert(HILDON_IS_FILE_SELECTION(data));
    ULOG_DEBUG(__FUNCTION__);
    g_signal_emit(data, signal_navigation_pane_context_menu, 0);
}

static void hildon_file_selection_content_pane_context(GtkWidget * widget,
                                                       gpointer data)
{
    g_assert(HILDON_IS_FILE_SELECTION(data));
    ULOG_DEBUG(__FUNCTION__);
    g_signal_emit(data, signal_content_pane_context_menu, 0);
}

static gboolean hildon_file_selection_on_content_pane_key(GtkWidget *
                                                          widget,
                                                          GdkEventKey *
                                                          event,
                                                          gpointer data)
{
    HildonFileSelectionPrivate *priv = HILDON_FILE_SELECTION(data)->priv;

    if (!hildon_file_selection_content_pane_visible(priv))
        return FALSE;

    switch (event->keyval) {
    case GDK_KP_Left:
    case GDK_Left:
    case GDK_leftarrow:
        activate_view(priv->dir_tree);
        return TRUE;
    case GDK_KP_Right:
    case GDK_Right:
    case GDK_rightarrow:
        g_signal_emit_by_name(gtk_widget_get_ancestor
                              (widget, GTK_TYPE_WINDOW), "move_focus",
                              GTK_DIR_TAB_FORWARD);
        return TRUE;
    }

    return FALSE;
}

/* Implement moving from navigation pane to content pane when folder is
   already expanded and right key is pressed */
static gboolean hildon_file_selection_on_navigation_pane_key(GtkWidget *
                                                             widget,
                                                             GdkEventKey *
                                                             event,
                                                             gpointer data)
{
    GtkTreeView *tree;
    GtkWidget *content_view;
    GtkTreePath *path;
    gboolean expanded;
    HildonFileSelectionPrivate *priv = HILDON_FILE_SELECTION(data)->priv;
    gboolean result = FALSE;

    content_view = get_current_view(priv);
    tree = GTK_TREE_VIEW(widget);
    gtk_tree_view_get_cursor(tree, &path, NULL);

    if (!path) return FALSE;

    expanded = gtk_tree_view_row_expanded(tree, path);

    switch (event->keyval) {
    case GDK_KP_Left:
    case GDK_Left:
    case GDK_leftarrow:
    {
        if (!expanded) {
            g_signal_emit_by_name(gtk_widget_get_ancestor
                                  (widget, GTK_TYPE_WINDOW), "move_focus",
                                  GTK_DIR_TAB_BACKWARD);
            result = TRUE;
        }
        break;
    }
    case GDK_KP_Right:
    case GDK_Right:
    case GDK_rightarrow:
    {
        GtkTreeModel *model;
        GtkTreeIter iter;

        model = gtk_tree_view_get_model(tree);

        /* Well, we also have to check situation if line cannot be
           expanded at all. Also in this case we focus content pane */
        if (expanded
            || (gtk_tree_model_get_iter(model, &iter, path)
                && !gtk_tree_model_iter_has_child(model, &iter))) {

            if (hildon_file_selection_content_pane_visible(priv)) 
            {
                activate_view(content_view);                
            }
            else
            {
            /* If there is no content pane, we focus the next widget */

            g_signal_emit_by_name(gtk_widget_get_ancestor
                                  (widget, GTK_TYPE_WINDOW), "move_focus",
                                  GTK_DIR_TAB_FORWARD);
            }
            result = TRUE;
        }

        break;
    }
    case GDK_KP_Enter:
    case GDK_ISO_Enter:
    case GDK_Return:
        if (expanded)
          gtk_tree_view_collapse_row(tree, path);
        else
          gtk_tree_view_expand_row(tree, path, FALSE);

        result = TRUE;
        break;
    }
    gtk_tree_path_free(path);

    return result;
}

static void
navigation_pane_focus(GObject *object, GParamSpec *pspec, gpointer data)
{
  if (GTK_WIDGET_HAS_FOCUS(object))
  {
    HildonFileSelectionPrivate *priv = HILDON_FILE_SELECTION(data)->priv;

    if (priv->content_pane_last_used)
    {
      hildon_file_selection_unselect_all( HILDON_FILE_SELECTION(data) );
      priv->content_pane_last_used = FALSE;
      scroll_to_cursor(GTK_TREE_VIEW(object));
      g_object_notify(data, "active-pane");
    }
  }
}

static void
content_pane_focus(GObject *object, GParamSpec *pspec, gpointer data)
{
  if (GTK_WIDGET_HAS_FOCUS(object))
  {
    HildonFileSelectionPrivate *priv = HILDON_FILE_SELECTION(data)->priv;

    if (!priv->content_pane_last_used)
    {
      priv->content_pane_last_used = TRUE;
      scroll_to_cursor(GTK_TREE_VIEW(object));
      g_object_notify(data, "active-pane");
    }
  }
}

static void hildon_file_selection_create_thumbnail_view(HildonFileSelection
                                                        * self)
{
    GtkTreeSelection *selection;
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;
    GtkTreeView *tree;

    self->priv->view[1] = gtk_tree_view_new();
    tree = GTK_TREE_VIEW(self->priv->view[1]);

    /* Fixed height mode has some kind of problems when appending new content to
        view. Seems that this is patch bug or Gtk 2.4 bug. Anyway, this works fine with
        Gtk 2.6. Temporarily commented out fixed height mode. */
    g_object_set(tree, "force_list_kludge", TRUE, /*"fixed-height-mode", TRUE,*/NULL); 

    col = gtk_tree_view_column_new();
/*    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);*/
/*    gtk_tree_view_column_set_fixed_width(col, 80 + 16);*/  /* Thumbnail width and borders, ugly. Doesn't match the theme, wonder why???? */
    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_cell_renderer_set_fixed_size(renderer, -1, THUMBNAIL_CELL_HEIGHT);

    gtk_tree_view_append_column(tree, col);
    gtk_tree_view_column_pack_start(col, renderer, FALSE);
    gtk_tree_view_column_add_attribute
        (col, renderer, "pixbuf",
         HILDON_FILE_SYSTEM_MODEL_COLUMN_THUMBNAIL);
    gtk_tree_view_column_add_attribute
        (col, renderer, "sensitive",
        HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE);

    col = gtk_tree_view_column_new();
/*    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);*/
    gtk_tree_view_column_set_expand(col, TRUE);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(tree, col);
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_set_cell_data_func(col, renderer,
                                            thumbnail_data_func, self,
                                            NULL);

    selection = gtk_tree_view_get_selection(tree);
    g_signal_connect_object
        (selection, "changed",
         G_CALLBACK(hildon_file_selection_content_pane_selection_changed),
         self, 0);
    g_signal_connect_object(tree, "key-press-event",
                     G_CALLBACK(hildon_file_selection_on_content_pane_key),
                     self, 0);
    gtk_widget_tap_and_hold_setup(GTK_WIDGET(tree), NULL, NULL,
                                  GTK_TAP_AND_HOLD_NONE);
    g_signal_connect_object(tree, "tap-and-hold",
                     G_CALLBACK
                     (hildon_file_selection_content_pane_context), self, 0);
    g_signal_connect_object(tree, "notify::has-focus",
                     G_CALLBACK(content_pane_focus), self, 0);
}

static void hildon_file_selection_create_list_view(HildonFileSelection *
                                                   self)
{
    GtkTreeSelection *selection;
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;
    GtkTreeView *tree;

    self->priv->view[0] = gtk_tree_view_new();
    tree = GTK_TREE_VIEW(self->priv->view[0]);
    g_object_set(tree, "force_list_kludge", TRUE, NULL); 
    /* Fixed height seems to require setting fixed widths for every
       column. This is not a good thing in our case. We would _absolutely_
       need GTK_TREE_VIEW_COLUMN_GROW_ONLY to be supported as well in
       fixed-height-mode */
/*  g_object_set(tree, "fixed-height-mode", TRUE, NULL);*/

    col = gtk_tree_view_column_new();
/*      gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width(col, 26);*/
    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_cell_renderer_set_fixed_size(renderer, -1, LIST_CELL_HEIGHT);

/*  gtk_tree_view_column_set_spacing(col, 0);*/
/*  g_object_set(renderer, "background", "yellow", "background-set",
                 TRUE, NULL);*/
    gtk_tree_view_append_column(tree, col);
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute
        (col, renderer, "pixbuf",
         HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON);
    gtk_tree_view_column_add_attribute
        (col, renderer, "sensitive",
        HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE);

    col = gtk_tree_view_column_new();
/*      gtk_tree_view_column_set_sizing(col, GTK_TREE_VEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width(col, 128);*/
/*  gtk_tree_view_column_set_spacing(col, 0);*/
    gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_append_column(tree, col);
    renderer = gtk_cell_renderer_text_new();
/*  g_object_set(renderer, "background", "green", "background-set",
                 TRUE, NULL);*/
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute
        (col, renderer, "text",
         HILDON_FILE_SYSTEM_MODEL_COLUMN_DISPLAY_NAME);
    gtk_tree_view_column_add_attribute
        (col, renderer, "sensitive",
        HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE);

    if (self->priv->visible_columns & HILDON_FILE_SELECTION_SHOW_MODIFIED)
    {
      col = gtk_tree_view_column_new();
  /*      gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(col, 128);*/
  /*  gtk_tree_view_column_set_spacing(col, 0);*/
      renderer = gtk_cell_renderer_text_new();
      g_object_set(renderer, /*"background", "red", "background-set",
                   TRUE,*/ "xalign", 1.0f, NULL);
      gtk_tree_view_append_column(tree, col);
      gtk_tree_view_column_pack_start(col, renderer, TRUE);
      gtk_tree_view_column_set_cell_data_func(col, renderer,
                                            list_date_data_func, self,
                                            NULL);
    }

    if (self->priv->visible_columns & HILDON_FILE_SELECTION_SHOW_SIZE)
    {
      col = gtk_tree_view_column_new();
/*      gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width(col, 128);*/
/*  gtk_tree_view_column_set_spacing(col, 0);*/
      renderer = gtk_cell_renderer_text_new();
      g_object_set(renderer, "xalign", 1.0f,
                 /*"background", "blue", "background-set", TRUE,*/ NULL);
      gtk_tree_view_append_column(tree, col);
      gtk_tree_view_column_pack_start(col, renderer, TRUE);
      gtk_tree_view_column_set_cell_data_func(col, renderer,
                                            list_size_data_func, self,
                                            NULL);
    }

    selection = gtk_tree_view_get_selection(tree);
    g_signal_connect_object
        (selection, "changed",
         G_CALLBACK (hildon_file_selection_content_pane_selection_changed),
         self, 0);
    gtk_widget_tap_and_hold_setup(GTK_WIDGET(tree), NULL, NULL,
                                  GTK_TAP_AND_HOLD_NONE);
    g_signal_connect_object(tree, "tap-and-hold",
                     G_CALLBACK
                     (hildon_file_selection_content_pane_context), self, 0);
    g_signal_connect_object(tree, "key-press-event",
                     G_CALLBACK(hildon_file_selection_on_content_pane_key),
                     self, 0);
    g_signal_connect_object(tree, "notify::has-focus",
                     G_CALLBACK(content_pane_focus), self, 0);
}

static void hildon_file_selection_create_dir_view(HildonFileSelection *
                                                  self)
{
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;
    GtkTreeSelection *selection;

    self->priv->dir_filter =
        gtk_tree_model_filter_new(self->priv->sort_model, NULL);
    gtk_tree_model_filter_set_visible_column
        (GTK_TREE_MODEL_FILTER
         (self->priv->dir_filter),
         HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_FOLDER);

    self->priv->dir_tree =
        gtk_tree_view_new_with_model(self->priv->dir_filter);
/*  g_object_set(self->priv->dir_tree, "fixed-height-mode", TRUE, NULL);*/

    col = gtk_tree_view_column_new();
/*      gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);*/
    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_cell_renderer_set_fixed_size(renderer, -1, TREE_CELL_HEIGHT);
    gtk_tree_view_append_column(GTK_TREE_VIEW(self->priv->dir_tree), col);
    gtk_tree_view_column_pack_start(col, renderer, FALSE);

    gtk_tree_view_column_add_attribute
        (col, renderer, "sensitive",
        HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE);
    gtk_tree_view_column_add_attribute
        (col, renderer,
         "pixbuf-expander-closed",
         HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON_COLLAPSED);
    gtk_tree_view_column_add_attribute
        (col, renderer,
         "pixbuf-expander-open",
         HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON_EXPANDED);
    gtk_tree_view_column_add_attribute
        (col, renderer, "pixbuf",
         HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute
        (col, renderer, "text",
         HILDON_FILE_SYSTEM_MODEL_COLUMN_DISPLAY_NAME);
    gtk_tree_view_column_add_attribute
        (col, renderer, "sensitive",
        HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE);

    /* Selection handling */
    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->dir_tree));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
    g_signal_connect_object(selection, "changed",
                     G_CALLBACK(hildon_file_selection_selection_changed),
                     self, 0);
    gtk_widget_tap_and_hold_setup(GTK_WIDGET(self->priv->dir_tree), NULL,
                                  NULL, GTK_TAP_AND_HOLD_NONE);
    g_signal_connect_object(self->priv->dir_tree, "tap-and-hold",
                     G_CALLBACK
                     (hildon_file_selection_navigation_pane_context),
                     self, 0);
    g_signal_connect_object(self->priv->dir_tree, "key-press-event",
                     G_CALLBACK
                     (hildon_file_selection_on_navigation_pane_key), self, 0);
    g_signal_connect_object(self->priv->dir_tree, "notify::has-focus",
                     G_CALLBACK(navigation_pane_focus), self, 0);
}

static void on_drag_data_received(GtkWidget * widget,
                                  GdkDragContext * context, int x, int y,
                                  GtkSelectionData * seldata, guint info,
                                  guint time, gpointer userdata)
{
    GtkTreeView *view;
    GtkTreeModel *dest_model;
    GtkTreePath *path;
    GtkTreeIter dest_iter;
    GtkFilePath *destination;
    HildonFileSelection *self;
    gboolean is_folder, is_available;

    g_assert(GTK_IS_TREE_VIEW(widget));
    g_assert(HILDON_IS_FILE_SELECTION(userdata));

    /* This is needed. Otherwise Gtk will give long description why we
       need this */
    g_signal_stop_emission_by_name(widget, "drag-data-received");

    self = HILDON_FILE_SELECTION(userdata);
    view = GTK_TREE_VIEW(widget);
    dest_model = gtk_tree_view_get_model(view);

    gtk_tree_view_get_path_at_pos(view, x, y, &path, NULL, NULL, NULL);

    if (!path || !gtk_tree_model_get_iter(dest_model, &dest_iter, path)) {
        /* The only allowed case to not have working iterator is when item 
           is dropped to empty space of content pane. In this case
           location will be current folder. So, it's equal to drag to the
           current folder of navigation pane and empty space of content
           pane */

        GtkTreeSelection *selection;

        view = GTK_TREE_VIEW(self->priv->dir_tree);
        selection = gtk_tree_view_get_selection(view);

        if ((widget != self->priv->view[0]
             && widget != self->priv->view[1])
            || !gtk_tree_selection_get_selected(selection, &dest_model,
                                                &dest_iter)) {
            ULOG_INFO_L("Rejected");
            /* No, we didn't get iterator */
            gtk_drag_finish(context, FALSE, FALSE, time);
            return;
        }
    }

    gtk_tree_path_free(path);

    /* We now have a working iterator to drop destination */
    gtk_tree_model_get(dest_model, &dest_iter,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_FOLDER,
                       &is_folder, 
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE,
                       &is_available, -1);

    if (!is_folder || !is_available) {
        ULOG_INFO_L("Destination is not an available folder");
        gtk_drag_finish(context, FALSE, FALSE, time);
        return;
    }

    gtk_tree_model_get(dest_model, &dest_iter,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_GTK_PATH,
                       &destination, -1);

    if (seldata->length >= 0 && seldata->format == 8) {
        char *buffer, *linebreak;
        GSList *sources;

        buffer = (char *) seldata->data;
        sources = NULL;

        while ((linebreak = strchr(buffer, '\n')) != NULL) {
            GtkFilePath *file_path;
            gchar *item = g_strndup(buffer, linebreak - buffer);

            file_path = gtk_file_path_new_steal(item);
            sources = g_slist_append(sources, file_path);
            buffer = linebreak + 1;
        }

        g_signal_emit(userdata, signal_items_dropped, 0, destination,
                      sources);
        gtk_file_paths_free(sources);
        gtk_drag_finish(context, TRUE, FALSE, time);
    } else
        gtk_drag_finish(context, FALSE, FALSE, time);

    gtk_file_path_free(destination);
}

#define CLIMB_RATE 4

static void drag_begin(GtkWidget * widget, GdkDragContext * drag_context,
                       gpointer user_data)
{
    GdkPixbuf *pixbuf;
    GtkTreeSelection *selection;
    gint column, w, h, dest;
    HildonFileSelection *self;
    GList *rows, *row;
    GtkTreeModel *model;
    GtkTreeIter iter;

    g_assert(GTK_IS_TREE_VIEW(widget));
    g_assert(HILDON_IS_FILE_SELECTION(user_data));

    self = HILDON_FILE_SELECTION(user_data);
    column =
        (widget ==
         self->priv->
         view[1] ? HILDON_FILE_SYSTEM_MODEL_COLUMN_THUMBNAIL :
         HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
    rows = gtk_tree_selection_get_selected_rows(selection, &model);

    if (!rows)
        return;

    /* Lets find out how much space we need for drag icon */
    w = h = dest = 0;

    for (row = rows; row; row = row->next)
        if (gtk_tree_model_get_iter(model, &iter, row->data)) {
            GdkPixbuf *buf;
            gint right, bottom;

            gtk_tree_model_get(model, &iter, column, &buf, -1);
	    if( !buf )
	      return;
            right = gdk_pixbuf_get_width(buf) + dest;
            bottom = gdk_pixbuf_get_height(buf) + dest;
            dest += CLIMB_RATE;

            w = MAX(w, right);
            h = MAX(h, bottom);

            g_object_unref(buf);
        }

    pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
    gdk_pixbuf_fill(pixbuf, 0);

    /* Ok, let's combine selected icons to allocated pixbuf */

    dest = 0;


    for (row = rows; row; row = row->next)
        if (gtk_tree_model_get_iter(model, &iter, row->data)) {
            GdkPixbuf *buf;

            gtk_tree_model_get(model, &iter, column, &buf, -1);
            w = gdk_pixbuf_get_width(buf);
            h = gdk_pixbuf_get_height(buf);
	    /*   g_assert(gdk_pixbuf_get_has_alpha(buf));*/
	    gdk_pixbuf_composite(buf, pixbuf, dest, dest, w, h, dest, dest,
                                 1, 1, GDK_INTERP_NEAREST, 255);
            dest += CLIMB_RATE;

	    g_object_unref(buf);
        }



    g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(rows);

    gtk_drag_set_icon_pixbuf(drag_context, pixbuf, w/2, h/2);
    g_object_unref(pixbuf);
}

static void drag_data_get_helper(GtkTreeModel * model, GtkTreePath * path,
                                 GtkTreeIter * iter, gpointer data)
{
    GtkFilePath *file_path;
    GString **buffer = data;

    gtk_tree_model_get(model, iter,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_GTK_PATH,
                       &file_path, -1);

    /* Should we send URI:s instead of raw file paths ?? */
    g_string_append(*buffer, (const char *) file_path);
    g_string_append_c(*buffer, '\n');

    gtk_file_path_free(file_path);
}

static void drag_data_get(GtkWidget * widget,
                          GdkDragContext * drag_context,
                          GtkSelectionData * selection_data, guint info,
                          guint time, gpointer data)
{
    GString *buffer = g_string_new("");

    gtk_tree_selection_selected_foreach(gtk_tree_view_get_selection
                                        (GTK_TREE_VIEW(widget)),
                                        drag_data_get_helper, &buffer);
    gtk_selection_data_set(selection_data, selection_data->target, 8,
                           (guchar *) buffer->str, buffer->len);

    g_string_free(buffer, TRUE);
}

static gboolean drag_motion_expand_timeout(gpointer data)
{
    GtkTreeView *view;
    GtkTreePath *path;

    g_return_val_if_fail(GTK_IS_TREE_VIEW(data), FALSE);

    view = GTK_TREE_VIEW(data);
    gtk_tree_view_get_drag_dest_row(view, &path, NULL);

    if (path) {
        gtk_tree_view_expand_row(view, path, FALSE);
        gtk_tree_path_free(path);
    }

    return FALSE;
}

static gboolean path_is_available_row(GtkTreeView *view, GtkTreePath *path)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean available;

  model = gtk_tree_view_get_model(view);

  if (!gtk_tree_model_get_iter(model, &iter, path))
    return FALSE;

  gtk_tree_model_get(model, &iter, 
    HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE, &available, -1);

  return available;   
}

static gboolean drag_motion(GtkWidget * widget,
                            GdkDragContext * drag_context, gint x, gint y,
                            guint time, gpointer userdata)
{
    GtkTreeView *view;
    GtkTreePath *path;
    HildonFileSelection *self;
    gboolean row_at_pos, result = FALSE;

    g_assert(GTK_IS_TREE_VIEW(widget));
    g_assert(HILDON_IS_FILE_SELECTION(userdata));

    self = HILDON_FILE_SELECTION(userdata);
    view = GTK_TREE_VIEW(widget);

    row_at_pos = gtk_tree_view_get_dest_row_at_pos(view, x, y, &path, NULL);

    /* All of the content pane area is valid drop target, unless we drop over an
       insensitive item */
    if (widget == self->priv->view[0] || widget == self->priv->view[1])
    {
      result = !row_at_pos || path_is_available_row(view, path);
    }
    else {
      /* We have to be on the navigation pane */
      g_assert(widget == self->priv->dir_tree);

      if (row_at_pos && path_is_available_row(view, path)) {
        /* I don't know if this is the best way to reset the timeout
           counter */
        g_source_remove_by_user_data(widget);
        g_timeout_add(1500, drag_motion_expand_timeout, widget);
        result = TRUE;
      }
    }

    if (row_at_pos && result)
      gtk_tree_view_set_drag_dest_row(view, path,
        GTK_TREE_VIEW_DROP_INTO_OR_AFTER);

    if (path)
      gtk_tree_path_free(path);

    return result;
}

static void drag_end(GtkWidget * widget, GdkDragContext * drag_context,
                     gpointer user_data)
{
}

static void hildon_file_selection_setup_dnd_view(HildonFileSelection *
                                                 self, GtkWidget * view)
{
    static GtkTargetEntry target_entries[] = {
        {"STRING", 0, 0},
        {"text/plain", 0, 0},
        {"text/uri-list", 0, 1}
    };

    gtk_drag_dest_set(view, GTK_DEST_DEFAULT_ALL, target_entries,
                      G_N_ELEMENTS(target_entries),
                      GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);

    g_signal_connect_object(view, "drag-data-received",
                     G_CALLBACK(on_drag_data_received), self, 0);

    gtk_drag_source_set(view, GDK_BUTTON1_MASK, target_entries,
                        G_N_ELEMENTS(target_entries), GDK_ACTION_COPY);

    g_signal_connect_object(view, "drag-begin", G_CALLBACK(drag_begin), self,0 );
    g_signal_connect_object(view, "drag-data-get", G_CALLBACK(drag_data_get),
                     self, 0);
    g_signal_connect_object(view, "drag-end", G_CALLBACK(drag_end), self, 0);
    g_signal_connect_object(view, "drag-motion", G_CALLBACK(drag_motion), self, 0);
    /* We don't connect "drag-data-delete", because file system
       notifications tell us about what to update */
    /* g_signal_connect(view, "drag-data-delete",
       G_CALLBACK(drag_data_delete), self); */
}

static void hildon_file_selection_init(HildonFileSelection * self)
{
    self->priv =
        G_TYPE_INSTANCE_GET_PRIVATE(self, HILDON_TYPE_FILE_SELECTION,
                                    HildonFileSelectionPrivate);
    self->priv->mode = HILDON_FILE_SELECTION_MODE_THUMBNAILS;

    GTK_WIDGET_SET_FLAGS(GTK_WIDGET(self), GTK_NO_WINDOW);

    self->priv->autosel_timer = g_timer_new();
    self->priv->scroll_dir = gtk_scrolled_window_new(NULL, NULL);
    self->priv->scroll_list = gtk_scrolled_window_new(NULL, NULL);
    self->priv->scroll_thumb = gtk_scrolled_window_new(NULL, NULL);
    self->priv->view_selector = GTK_NOTEBOOK(gtk_notebook_new());
    self->priv->separator = gtk_vseparator_new();

    gtk_container_set_border_width(GTK_CONTAINER(self->priv->scroll_dir),
        HILDON_MARGIN_DEFAULT);
    gtk_container_set_border_width(GTK_CONTAINER(self->priv->view_selector),
        HILDON_MARGIN_DEFAULT);

    gtk_notebook_set_show_tabs(self->priv->view_selector, FALSE);
    gtk_notebook_set_show_border(self->priv->view_selector, FALSE);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
                                   (self->priv->scroll_dir),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
                                   (self->priv->scroll_list),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
                                   (self->priv->scroll_thumb),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
                                        (self->priv->scroll_dir),
                                        GTK_SHADOW_NONE);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
                                        (self->priv->scroll_list),
                                        GTK_SHADOW_NONE);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
                                        (self->priv->scroll_thumb),
                                        GTK_SHADOW_NONE);

    gtk_widget_set_parent(self->priv->scroll_dir, GTK_WIDGET(self));
    gtk_widget_set_parent(self->priv->separator, GTK_WIDGET(self));
    gtk_widget_set_parent(GTK_WIDGET(self->priv->view_selector),
                          GTK_WIDGET(self));

    /* This needs to exist before set properties are called */
    self->priv->view[2] = gtk_label_new(_("hfil_li_no_files_folders_to_show"));
    gtk_misc_set_alignment(GTK_MISC(self->priv->view[2]), 0.5f, 0.0f);
    gtk_notebook_append_page(self->priv->view_selector, self->priv->view[2], NULL);
}

static GObject *hildon_file_selection_constructor(GType type,
                                                  guint
                                                  n_construct_properties,
                                                  GObjectConstructParam *
                                                  construct_properties)
{
    GObject *obj;
    HildonFileSelection *self;
    HildonFileSelectionPrivate *priv;
    GObject *dir_scroll;
    GtkTreeSortable *sortable;

/*  GtkTreeIter iter;*/

    obj =
        G_OBJECT_CLASS(hildon_file_selection_parent_class)->
        constructor(type, n_construct_properties, construct_properties);

    /* Now construction parameters (=backend) are applied. Let's finalize
       construction */
    self = HILDON_FILE_SELECTION(obj);
    priv = self->priv;

    priv->sort_model =
        gtk_tree_model_sort_new_with_model(priv->main_model);

    sortable = GTK_TREE_SORTABLE(priv->sort_model);
    gtk_tree_sortable_set_sort_func(sortable,
                                    HILDON_FILE_SELECTION_SORT_NAME,
                                    sort_function, self, NULL);
    gtk_tree_sortable_set_sort_func(sortable,
                                    HILDON_FILE_SELECTION_SORT_TYPE,
                                    sort_function, self, NULL);
    gtk_tree_sortable_set_sort_func(sortable,
                                    HILDON_FILE_SELECTION_SORT_MODIFIED,
                                    sort_function, self, NULL);
    gtk_tree_sortable_set_sort_func(sortable,
                                    HILDON_FILE_SELECTION_SORT_SIZE,
                                    sort_function, self, NULL);
    gtk_tree_sortable_set_sort_column_id(sortable,
                                         (gint)
                                         HILDON_FILE_SELECTION_SORT_NAME,
                                         GTK_SORT_ASCENDING);

    hildon_file_selection_create_dir_view(self);
    hildon_file_selection_create_list_view(self);
    hildon_file_selection_create_thumbnail_view(self);

    gtk_container_add(GTK_CONTAINER(priv->scroll_dir), priv->dir_tree);
    gtk_container_add(GTK_CONTAINER(priv->scroll_list), priv->view[0]);
    gtk_container_add(GTK_CONTAINER(priv->scroll_thumb), priv->view[1]);

    gtk_notebook_prepend_page(priv->view_selector, priv->scroll_thumb,
                              NULL);
    gtk_notebook_prepend_page(priv->view_selector, priv->scroll_list,
                              NULL);

    gtk_widget_show_all(GTK_WIDGET(priv->view_selector));
    gtk_widget_show_all(priv->scroll_dir);

    /* Also the views of the navigation pane are trees (and this is
       needed). Let's deny expanding */
    g_signal_connect(priv->view[0], "test-expand-row",
                     G_CALLBACK(gtk_true), NULL);
    g_signal_connect(priv->view[1], "test-expand-row",
                     G_CALLBACK(gtk_true), NULL);
    g_signal_connect_after(priv->view[0], "row-activated",
                           G_CALLBACK(hildon_file_selection_row_activated),
                           self);
    g_signal_connect_after(priv->view[1], "row-activated",
                           G_CALLBACK(hildon_file_selection_row_activated),
                           self);
    g_signal_connect_after(priv->view[0], "row-insensitive",
                           G_CALLBACK(hildon_file_selection_row_insensitive),
                           self);
    g_signal_connect_after(priv->view[1], "row-insensitive",
                           G_CALLBACK(hildon_file_selection_row_insensitive),
                           self);
    g_signal_connect_after(priv->dir_tree, "row-insensitive",
                           G_CALLBACK(hildon_file_selection_row_insensitive),
                           self);

    /* Use sort model when possible, because we own it. In last case we
       have to use main model. This handler has to be removed manually,
       because main_model can live long after we are destroyed
       We have to connect after, otherwise iterator stamps invalidate
       when cursor is set. */
    g_signal_connect_data(priv->main_model, "row-has-child-toggled",
                             G_CALLBACK
                             (hildon_file_selection_inspect_view), priv, NULL,
                              G_CONNECT_SWAPPED | G_CONNECT_AFTER);
    g_signal_connect_object(priv->main_model, "finished-loading",
                             G_CALLBACK
                             (hildon_file_selection_check_close_load_banner),
                             self, 0);
    hildon_file_selection_set_select_multiple(self, FALSE);

    /* Make the separator show up when scrolbar is hidden and vice versa */
    dir_scroll =
        G_OBJECT(GTK_SCROLLED_WINDOW(priv->scroll_dir)->vscrollbar);
    g_signal_connect_swapped(dir_scroll, "show",
                             G_CALLBACK(gtk_widget_hide), priv->separator);
    g_signal_connect_swapped(dir_scroll, "hide",
                             G_CALLBACK(gtk_widget_show), priv->separator);

    if (!GTK_WIDGET_VISIBLE(dir_scroll))
        gtk_widget_show(priv->separator);

    /* Ouch, GtkTreeModelSort implements just GtkDragSource, not
       GtkDragDest. We have to implement from scratch. */
    if (priv->drag_enabled) {
        hildon_file_selection_setup_dnd_view(self, priv->view[0]);
        hildon_file_selection_setup_dnd_view(self, priv->view[1]);
        hildon_file_selection_setup_dnd_view(self, priv->dir_tree);
    }

    return obj;
}

static void get_selected_paths_helper(GtkTreeModel * model,
                                      GtkTreePath * path,
                                      GtkTreeIter * iter, gpointer data)
{
    GtkFilePath *file_path;
    GSList **list;

    list = data;
    gtk_tree_model_get(model, iter,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_GTK_PATH,
                       &file_path, -1);
    *list = g_slist_append(*list, file_path);   /* file_path is already a
                                                   copy */
}

/*** Public API **********************************************************/

/**
 * hildon_file_selection_new_with_model:
 * @model: a #HildonFileSystemModel to display.
 *
 * Creates a new #HildonFileSelection using given model.
 *
 * Returns: a new #HildonFileSelection
 */
GtkWidget *hildon_file_selection_new_with_model(HildonFileSystemModel *
                                                model)
{
    return
        GTK_WIDGET(g_object_new
                   (HILDON_TYPE_FILE_SELECTION, "model", model, NULL));
}

/**
 * hildon_file_selection_set_mode:
 * @self: a pointer to #HildonFileSelection
 * @mode: New mode for current folder.
 *
 * Swithces file selection between list and thumbnail modes. Note that
 * this function works only after widget is shown because of #GtkNotebook
 * implementation.
 */
void hildon_file_selection_set_mode(HildonFileSelection * self,
                                    HildonFileSelectionMode mode)
{
    g_return_if_fail(HILDON_IS_FILE_SELECTION(self));
    g_return_if_fail(mode == HILDON_FILE_SELECTION_MODE_LIST ||
                     mode == HILDON_FILE_SELECTION_MODE_THUMBNAILS);

    if (self->priv->mode != mode) {
        self->priv->mode = mode;
        hildon_file_selection_inspect_view(self->priv);
    }
}

/**
 * hildon_file_selection_get_mode:
 * @self: a pointer to #HildonFileSelection
 *
 * Gets Current view mode for file selection widget. If widget is not
 * shown this will return an invalid mode (-1). 
 * This is because of #GtkNotebook implementation.
 *
 * Returns: Current view mode.
 */
HildonFileSelectionMode hildon_file_selection_get_mode(HildonFileSelection
                                                       * self)
{
    g_return_val_if_fail(HILDON_IS_FILE_SELECTION(self),
                         HILDON_FILE_SELECTION_MODE_LIST);
    return self->priv->mode;
}

/**
 * hildon_file_selection_set_sort_key:
 * @self: a pointer to #HildonFileSelection
 * @key: New sort key.
 * @order: New sort order.
 *
 * Changes sort settings for views. Key only affects content page,
 * navigation pane is always sorted by name.
 */
void hildon_file_selection_set_sort_key(HildonFileSelection * self,
                                        HildonFileSelectionSortKey key,
                                        GtkSortType order)
{
    g_return_if_fail(HILDON_IS_FILE_SELECTION(self));
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE
                                         (self->priv->sort_model),
                                         (gint) key, order);
}

/**
 * hildon_file_selection_get_sort_key:
 * @self: a pointer to #HildonFileSelection
 * @key: a place to store sort key.
 * @order: a place to store sort order.
 *
 * Currently active sort settings are stored to user provided pointers.
 */
void hildon_file_selection_get_sort_key(HildonFileSelection * self,
                                        HildonFileSelectionSortKey * key,
                                        GtkSortType * order)
{
    g_return_if_fail(HILDON_IS_FILE_SELECTION(self));
    gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE
                                         (self->priv->sort_model),
                                         (gint *) key, order);
}

static void
hildon_file_selection_set_current_folder_iter(HildonFileSelection * self,
                                              GtkTreeIter * main_iter)
{
    GtkTreeIter sort_iter, filter_iter;
    GtkTreeView *view;
    GtkTreePath *treepath;
    gboolean res;
    GtkTreeIter temp_iter;

    g_assert(HILDON_IS_FILE_SELECTION(self));
    ULOG_DEBUG(__FUNCTION__);

    self->priv->force_content_pane = FALSE;

    /* Sanity check that iterator points to a folder. Otherwise take the parent instead */
    gtk_tree_model_get(self->priv->main_model, main_iter, 
      HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_FOLDER, &res, -1);

    if (!res)
    {
      res = gtk_tree_model_iter_parent(self->priv->main_model, &temp_iter, main_iter);
      g_assert(res);
      main_iter = &temp_iter;
    }

    gtk_tree_model_sort_convert_child_iter_to_iter(GTK_TREE_MODEL_SORT
                                                   (self->priv->
                                                    sort_model),
                                                   &sort_iter, main_iter);
    gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER
                                                     (self->priv->
                                                      dir_filter),
                                                     &filter_iter,
                                                     &sort_iter);

    view = GTK_TREE_VIEW(self->priv->dir_tree);
    treepath =
        gtk_tree_model_get_path(self->priv->dir_filter, &filter_iter);

    gtk_tree_view_expand_to_path(view, treepath);
    gtk_tree_view_set_cursor(view, treepath, NULL, FALSE);

    gtk_tree_path_free(treepath);
}

/**
 * hildon_file_selection_set_current_folder:
 * @self: a pointer to #HildonFileSelection
 * @folder: a new folder.
 * @error: a place to store possible error.
 *
 * Changes the content pane to display the given folder.
 *
 * Returns: %TRUE if directory change was succesful,
 *          %FALSE if error occurred.
 */
gboolean hildon_file_selection_set_current_folder(HildonFileSelection *
                                                  self,
                                                  const GtkFilePath *
                                                  folder, GError ** error)
{
    HildonFileSystemModel *file_system_model;
    GtkTreeIter main_iter;

    g_return_val_if_fail(HILDON_IS_FILE_SELECTION(self), FALSE);
    g_return_val_if_fail(folder != NULL, FALSE);

    ULOG_INFO_F("Setting folder to %s", (const char *) folder);

    file_system_model = HILDON_FILE_SYSTEM_MODEL(self->priv->main_model);
    (void) hildon_file_system_model_load_path(file_system_model, folder,
                                              &main_iter);

    /* Iter is actually always pointing to last "good" position in this
       case, regardless of the return value */

    if (main_iter.user_data) {
        /* The following can cause long delays if folder contains lot's of stuff. This is required by
            spesification, however. Let's de that only modes that require it */
        if (GTK_WIDGET_DRAWABLE(self))
          _hildon_file_system_model_load_children(file_system_model, &main_iter);

        hildon_file_selection_set_current_folder_iter(self, &main_iter);
        activate_view(self->priv->dir_tree);
        ULOG_INFO_F("Directory changed successfully");
        return TRUE;
    }
    ULOG_INFO_F("Directory change failed");
    return FALSE;
}

/**
 * hildon_file_selection_get_current_folder:
 * @self: a pointer to #HildonFileSelection
 *
 * Gets a path to the currently active folder (the folder which is
 * displayed in the content pane). You have to release the returned path
 * with #gtk_file_path_free.
 *
 * Returns: a #GtkFilePath. 
 */
GtkFilePath *hildon_file_selection_get_current_folder(HildonFileSelection *
                                                      self)
{
    GtkTreeIter iter;

    if (hildon_file_selection_get_current_folder_iter(self, &iter)) {
        GtkFilePath *path;

        gtk_tree_model_get(self->priv->main_model, &iter,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_GTK_PATH, &path,
                           -1);
        return path;    /* This is already a copy */
    }

    g_assert_not_reached();
    return NULL;
}

/**
 * hildon_file_selection_set_filter:
 * @self: a pointer to #HildonFileSelection
 * @filter: a new #GtkFileFilter.
 *
 * Only the files matching the filter will be displayed in content pane.
 * Use %NULL to remove filtering. 
 */
void hildon_file_selection_set_filter(HildonFileSelection * self,
                                      GtkFileFilter * filter)
{
    g_return_if_fail(HILDON_IS_FILE_SELECTION(self));
    g_return_if_fail(filter == NULL || GTK_IS_FILE_FILTER(filter));

    if (filter != self->priv->filter) {
        if (self->priv->filter)
            g_object_unref(self->priv->filter);

        if (filter) {
            g_object_ref(filter);
            gtk_object_sink(GTK_OBJECT(filter));
        }

        self->priv->filter = filter;

        if (self->priv->view_filter) {
            /* gtk_tree_model_filter_refilter displays many critical
               warnings. This is because of Gtk bug: When refiltering is
               asked gtk uses foreach for THE WHOLE child model (not just
               for pats under the virtual root). In respective callback
               it displays errors when such an item found that has
               greater depth than virtual root but is form different
               branch. We don't need to fear these warnings, though. */

        gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER
                                           (self->priv->view_filter));
        hildon_file_selection_inspect_view(self->priv);
      }
    }
}

/**
 * hildon_file_selection_get_filter:
 * @self: a pointer to #HildonFileSelection
 *
 * Get currently active filter set by #hildon_file_selection_set filter.
 * Can be %NULL if no filter is set.
 *
 * Returns: a #GtkFileFilter or %NULL.
 */
GtkFileFilter *hildon_file_selection_get_filter(HildonFileSelection * self)
{
    g_return_val_if_fail(HILDON_IS_FILE_SELECTION(self), NULL);

    return self->priv->filter;
}

/**
 * hildon_file_selection_set_select_multiple:
 * @self: a pointer to #HildonFileSelection
 * @select_multiple: either %TRUE or %FALSE.
 *
 * If multiple selection is enabled, checkboxes will appear to the last
 * item to the content pane. Multiple selection must be enabled if one
 * wants to call #hildon_file_selection_select_all.
 */
void hildon_file_selection_set_select_multiple(HildonFileSelection * self,
                                               gboolean select_multiple)
{
    GtkSelectionMode mode;

    g_return_if_fail(HILDON_IS_FILE_SELECTION(self));

    mode =
        (select_multiple ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE);

    gtk_tree_selection_set_mode(gtk_tree_view_get_selection
                                (GTK_TREE_VIEW(self->priv->view[0])),
                                mode);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection
                                (GTK_TREE_VIEW(self->priv->view[1])),
                                mode);
}

/**
 * hildon_file_selection_get_select_multiple:
 * @self: a pointer to #HildonFileSelection
 *
 * Gets state of multiple selection.
 *
 * Returns: %TRUE If multiple selection is currently enabled.
 */
gboolean hildon_file_selection_get_select_multiple(HildonFileSelection *
                                                   self)
{
    g_return_val_if_fail(HILDON_IS_FILE_SELECTION(self), FALSE);

    return
        gtk_tree_selection_get_mode(gtk_tree_view_get_selection
                                    (GTK_TREE_VIEW(self->priv->view[0])))
        == GTK_SELECTION_MULTIPLE;
}

/**
 * hildon_file_selection_select_all:
 * @self: a pointer to #HildonFileSelection
 *
 * Selects all files from the content pane. Multiple selection must be
 * enabled before calling this.
 */
void hildon_file_selection_select_all(HildonFileSelection * self)
{
    GtkTreeSelection *selection;

    g_return_if_fail(HILDON_IS_FILE_SELECTION(self));

    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->view[0]));
    gtk_tree_selection_select_all(selection);
    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->view[1]));
    gtk_tree_selection_select_all(selection);
}

/**
 * hildon_file_selection_unselect_all:
 * @self: a pointer to #HildonFileSelection
 *
 * Clears current selection from content pane.
 */
void hildon_file_selection_unselect_all(HildonFileSelection * self)
{
    GtkWidget *view;

    g_return_if_fail(HILDON_IS_FILE_SELECTION(self));

    view = get_current_view(self->priv);
    if (GTK_IS_TREE_VIEW(view))
    {
      GtkTreePath *path;
      GtkTreeSelection *sel;

      gtk_tree_view_get_cursor(GTK_TREE_VIEW(view), &path, NULL);
      sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
      gtk_tree_selection_unselect_all(sel);    

      if (path)
      {
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(view), path, NULL, FALSE);
        gtk_tree_path_free(path);
      }
    }
}

/**
 * hildon_file_selection_get_selected_paths:
 * @self: a pointer to #HildonFileSelection
 *
 * Gets list of selected paths from content pane. You have to release
 * the returned list with #gtk_file_paths_free. If you are interested
 * in item that (probably) has active focus, you have to first get
 * the active pane and then call either #hildon_file_selection_get_selected_paths
 * or #hildon_file_selection_get_current_folder.
 *
 * Returns: a #GSList containing #GtkFilePath objects.
 */
GSList *hildon_file_selection_get_selected_paths(HildonFileSelection *
                                                 self)
{
    GtkWidget *view;

    g_return_val_if_fail(HILDON_IS_FILE_SELECTION(self), NULL);

    view = get_current_view(self->priv);

    if (GTK_IS_TREE_VIEW(view)) {
        GSList *paths = NULL;

        gtk_tree_selection_selected_foreach(gtk_tree_view_get_selection
                                            (GTK_TREE_VIEW(view)),
                                            get_selected_paths_helper,
                                            &paths);
        return paths;
    }

    return NULL;
}

/* Used by select/unselect path for selections */
static void
hildon_file_selection_select_unselect_main_iter(HildonFileSelectionPrivate
                                                * priv, GtkTreeIter * iter,
                                                void
                                                (*select_unselect_func)
                                                (GtkTreeSelection *,
                                                 GtkTreeIter *))
{
    GtkWidget *view = get_current_view(priv);

    g_assert(select_unselect_func != NULL);

    if (GTK_IS_TREE_VIEW(view)) {
        GtkTreeIter sort_iter, filter_iter;
        GtkTreeView *treeview = GTK_TREE_VIEW(view);

        gtk_tree_model_sort_convert_child_iter_to_iter(GTK_TREE_MODEL_SORT
                                                       (priv->sort_model),
                                                       &sort_iter, iter);
        gtk_tree_model_filter_convert_child_iter_to_iter
            (GTK_TREE_MODEL_FILTER(priv->view_filter), &filter_iter,
             &sort_iter);
        select_unselect_func(gtk_tree_view_get_selection(treeview),
                             &filter_iter);
    }
}

/**
 * hildon_file_selection_select_path:
 * @self: a pointer to #HildonFileSelection
 * @path: a file to select.
 * @error: a place to store possible error.
 *
 * Selects the given file. If the path doesn't point to current folder the
 * folder is changed accordingly. If multiple selection is disabled then
 * the previous selection will dissappear.
 *
 * Returns: %TRUE if folder was succesfully selected,
 *          %FALSE if the path doesn't contain a valid folder.
 */
gboolean hildon_file_selection_select_path(HildonFileSelection * self,
                                           const GtkFilePath * path,
                                           GError ** error)
{
    GtkTreeIter iter;
    gboolean found;
    GtkWidget *view;

    g_return_val_if_fail(HILDON_IS_FILE_SELECTION(self), FALSE);

    found =
        hildon_file_system_model_load_path(HILDON_FILE_SYSTEM_MODEL
                                           (self->priv->main_model), path,
                                           &iter);

    if (found || iter.user_data != NULL) {      /* We have to found at
                                                   least the parent folder 
                                                 */
        hildon_file_selection_set_current_folder_iter(self, &iter);

        if (found)      /* Directory is changed. Let's select a file
                           inside of that */
        {
            hildon_file_selection_select_unselect_main_iter
                (self->priv, &iter, gtk_tree_selection_select_iter);

            view = get_current_view(self->priv);
            if (GTK_IS_TREE_VIEW(view))
              activate_view(view);
            else
              activate_view(self->priv->dir_tree);
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * hildon_file_selection_unselect_path:
 * @self: a pointer to #HildonFileSelection
 * @path: file to unselect
 *
 * Unselects a currently selected filename. If the filename is not in
 * the current directory, does not exist, or is otherwise not currently
 * selected, does nothing.
 */
void hildon_file_selection_unselect_path(HildonFileSelection * self,
                                         const GtkFilePath * path)
{
    GtkTreeIter iter;

    g_return_if_fail(HILDON_IS_FILE_SELECTION(self));

    if (hildon_file_system_model_search_path
        (HILDON_FILE_SYSTEM_MODEL(self->priv->main_model), path, &iter,
         NULL, TRUE))
        hildon_file_selection_select_unselect_main_iter
            (self->priv, &iter, gtk_tree_selection_unselect_iter);
}

/**
 * hildon_file_selection_hide_content_pane:
 * @self: a pointer to #HildonFileSelection
 *
 * Hides the content pane. This is used in certain file management dialogs.
 */
void hildon_file_selection_hide_content_pane(HildonFileSelection * self)
{
    g_return_if_fail(HILDON_IS_FILE_SELECTION(self));
    gtk_widget_hide(GTK_WIDGET(self->priv->view_selector));
}

/**
 * hildon_file_selection_show_content_pane:
 * @self: a pointer to #HildonFileSelection
 *
 * Shows the content pane. This is used in certain file management dialogs. 
 * The content pane is shown by default. Calling this is needed only if
 * you have hidden 
 */
void hildon_file_selection_show_content_pane(HildonFileSelection * self)
{
    g_return_if_fail(HILDON_IS_FILE_SELECTION(self));
    gtk_widget_show(GTK_WIDGET(self->priv->view_selector));
    hildon_file_selection_selection_changed
        (gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->dir_tree)),
         self);    /* Update content pane */
}

/* Path is a location in content pane filter model - convert it to main model
    iterator if possible */
static gboolean view_path_to_main_iter(GtkTreeModel *model,
  GtkTreeIter *iter, GtkTreePath *path)
{
  GtkTreeModel *sort_model;
  GtkTreeIter filter_iter, sort_iter;

  if (gtk_tree_model_get_iter(model, &filter_iter, path))
  {
    sort_model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model));

    gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER
                                                     (model),
                                                     &sort_iter, &filter_iter);
    gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT
                                                   (sort_model), iter, &sort_iter);
    return TRUE;
  }
  return FALSE;
}

/**
 * hildon_file_selection_get_current_content_iter:
 * @self: a #HildonFileSelection
 * @iter: a #GtkTreeIter for the result.
 *
 * Similar to #hildon_file_selection_get_current_folder_iter but this
 * works with content pane. However, this function fails also
 * if the content pane is not currently visible, more than one item
 * is selected or if the current folder is empty.
 *
 * Note! This function is DEPRECATED. You probably want to use 
 * #hildon_file_selection_get_active_content_iter instead
 * if you are intrested in single item.
 *
 * Returns: %TRUE, if the given iterator is set, %FALSE otherwise.
 */
gboolean hildon_file_selection_get_current_content_iter(HildonFileSelection
                                                        * self,
                                                        GtkTreeIter * iter)
{
    GtkWidget *view;
    GtkTreeSelection *selection;
    GList *selected_paths;
    gboolean result;

    g_return_val_if_fail(HILDON_IS_FILE_SELECTION(self), FALSE);

    if (!hildon_file_selection_content_pane_visible(self->priv))
        return FALSE;

    view = get_current_view(self->priv);
    if (!GTK_IS_TREE_VIEW(view))
        return FALSE;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    if (gtk_tree_selection_count_selected_rows(selection) != 1)
        return FALSE;

    /* get_selected works only in single selection mode. Multi selection
       mode don't work even in a case when only one item is selected. */
    selected_paths = gtk_tree_selection_get_selected_rows(selection, NULL);
    result = view_path_to_main_iter(self->priv->view_filter, iter, selected_paths->data);
    g_list_foreach(selected_paths, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selected_paths);

    return result;
}

/**
 * hildon_file_selection_get_active_content_iter:
 * @self: a #HildonFileSelection
 * @iter: a #GtkTreeIter for the result.
 *
 * Gets an iterator to the item with active focus in content pane 
 * (cursor in #GtkTreeView). Returned item need not to be selected,
 * it just has the active focus. If there is no focus on the content pane,
 * this function fails. 
 *
 * This function differs from 
 * #hildon_file_selection_get_current_content_iter,
 * because  this function uses cursor, not GtkTreeSelection.
 *
 * After checkboxes were removed there is no reason for this
 * function to exists. That's why this is DEPRECATED.
 *
 * Returns: %TRUE, if the given iterator is set, %FALSE otherwise.
 */
gboolean 
hildon_file_selection_get_active_content_iter(HildonFileSelection *self,
  GtkTreeIter *iter)
{
    GtkWidget *view;
    GtkTreePath *path;
    gboolean result;

    g_return_val_if_fail(HILDON_IS_FILE_SELECTION(self), FALSE);

    if (!hildon_file_selection_content_pane_visible(self->priv))
        return FALSE;

    view = get_current_view(self->priv);
    if (!GTK_IS_TREE_VIEW(view) || !self->priv->content_pane_last_used)
        return FALSE;

    gtk_tree_view_get_cursor(GTK_TREE_VIEW(view), &path, NULL);
    if (!path)
      return FALSE;

    result = view_path_to_main_iter(self->priv->view_filter, iter, path);
    gtk_tree_path_free(path);
    return result;
}

/**
 * hildon_file_selection_content_iter_is_selected:
 * @self: a #HildonFileSelection
 * @iter: a #GtkTreeIter for the result.
 *
 * Checks if the given iterator is selected in the content pane.
 * There is no much use for this function. Treat this as
 * DEPRECATED.
 *
 * Returns: %TRUE, if the iterator is selected, %FALSE otherwise.
 */
gboolean 
hildon_file_selection_content_iter_is_selected(HildonFileSelection *self, 
  GtkTreeIter *iter)
{
    GtkWidget *view;
    GList *selected_paths;
    GtkTreePath *main_path, *sort_path, *filter_path;
    gboolean result = FALSE;

    g_return_val_if_fail(HILDON_IS_FILE_SELECTION(self), FALSE);

    if (!hildon_file_selection_content_pane_visible(self->priv))
        return FALSE;

    view = get_current_view(self->priv);
    if (!GTK_IS_TREE_VIEW(view))
        return FALSE;

    main_path = gtk_tree_model_get_path(self->priv->main_model, iter);

    if (main_path)
    {
      sort_path = gtk_tree_model_sort_convert_child_path_to_path(
          GTK_TREE_MODEL_SORT(self->priv->sort_model), main_path);

      if (sort_path)
      {
        filter_path = gtk_tree_model_filter_convert_child_path_to_path(
            GTK_TREE_MODEL_FILTER(self->priv->view_filter), sort_path);

        if (filter_path)
        {
          /* Ok, we now need to check if filter path is present in selection */
          selected_paths = gtk_tree_selection_get_selected_rows(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), NULL);

          result = g_list_find_custom(selected_paths, 
            filter_path, (GCompareFunc) gtk_tree_path_compare) != NULL;

          g_list_foreach(selected_paths, (GFunc) gtk_tree_path_free, NULL);
          g_list_free(selected_paths);

          gtk_tree_path_free(filter_path);
        }
        gtk_tree_path_free(sort_path);
      }
      gtk_tree_path_free(main_path);
    }

    return result;
}

/**
 * hildon_file_selection_get_current_folder_iter:
 * @self: a #HildonFileSelection
 * @iter: a #GtkTreeIter for the result.
 * 
 * Fills the given iterator to match currently selected item in the 
 * navigation pane. Internally this gets the selection from the
 * tree and then the current iterator from selection and converts
 * the result accordingly.
 *
 * Returns: %TRUE, if the given iterator is set, %FALSE otherwise.
 */
gboolean hildon_file_selection_get_current_folder_iter(HildonFileSelection
                                                       * self,
                                                       GtkTreeIter * iter)
{
    GtkTreeSelection *selection;
    GtkTreeIter filter_iter, sort_iter;

    g_return_val_if_fail(HILDON_IS_FILE_SELECTION(self), FALSE);

    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->dir_tree));
    if (!gtk_tree_selection_get_selected(selection, NULL, &filter_iter))
        return gtk_tree_model_get_iter_first(self->priv->main_model, iter);

    gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER
                                                     (self->priv->
                                                      dir_filter),
                                                     &sort_iter,
                                                     &filter_iter);
    gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT
                                                   (self->priv->
                                                    sort_model), iter,
                                                   &sort_iter);

    return TRUE;
}

static void get_selected_files_helper(GtkTreeModel * model,
                                      GtkTreePath * path,
                                      GtkTreeIter * iter, gpointer data)
{
    GtkFilePath *file_path;
    GSList **list;
    gboolean folder;

    list = data;
    gtk_tree_model_get(model, iter,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_FOLDER,
                       &folder, -1);

    if (!folder)
    {
      gtk_tree_model_get(model, iter,
                       HILDON_FILE_SYSTEM_MODEL_COLUMN_GTK_PATH,
                       &file_path, -1);
      *list = g_slist_append(*list, file_path);
    }
}

/* Similar to get_selected_paths, but returns only files. This still uses
    content_pane_last_used, so we do not need to change dialog at all */
GSList *_hildon_file_selection_get_selected_files(HildonFileSelection
                                                       * self)
{
    GtkWidget *view;

    g_return_val_if_fail(HILDON_IS_FILE_SELECTION(self), NULL);

    view = get_current_view(self->priv);

    if (GTK_IS_TREE_VIEW(view) && self->priv->content_pane_last_used) {
        GSList *paths = NULL;

        gtk_tree_selection_selected_foreach(gtk_tree_view_get_selection
                                            (GTK_TREE_VIEW(view)),
                                            get_selected_files_helper,
                                            &paths);
        return paths;
    }

    return NULL;
}

/**
 * hildon_file_selection_dim_current_selection:
 * @self: a #HildonFileSelection.
 *
 * Appends currently selected paths to set of dimmed paths.
 * Note that dimmed paths cannot be selected, so selection
 * no longer contains the same paths after this function.
 */
void hildon_file_selection_dim_current_selection(HildonFileSelection *self)
{
    HildonFileSystemModel *model;
    GtkWidget *view;
    GtkTreeSelection *sel;
    GList *paths, *path;
    GtkTreeIter iter;

    g_return_if_fail(HILDON_IS_FILE_SELECTION(self));

    view = get_current_view(self->priv);

    if (GTK_IS_TREE_VIEW(view)) 
    {
        model = HILDON_FILE_SYSTEM_MODEL(self->priv->main_model);
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
        paths = gtk_tree_selection_get_selected_rows(sel, NULL);
        gtk_tree_selection_unselect_all(sel);

        for (path = paths; path; path = path->next)
          if (view_path_to_main_iter(self->priv->view_filter, &iter, path->data))
            hildon_file_system_model_iter_available(model, &iter, FALSE);

        g_list_foreach(paths, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(paths);
    }
}

/**
 * hildon_file_selection_undim_all:
 * @self: a #HildonFileSelection.
 *
 * Undims all from model that are dimmed by code. Simply calls
 * #hildon_file_system_model_reset_available for underlying model.
 */
void hildon_file_selection_undim_all(HildonFileSelection *self)
{
  g_return_if_fail(HILDON_IS_FILE_SELECTION(self));

  hildon_file_system_model_reset_available(
    HILDON_FILE_SYSTEM_MODEL(self->priv->main_model));  
}

/**
 * hildon_file_selection_get_active_pane:
 * @self: a #HildonFileSelection.
 *
 * Gets the pane that either has active focus or 
 * (in case of no pane has it) last time had it.
 *
 * Returns: Currently active pane.
 */
HildonFileSelectionPane 
hildon_file_selection_get_active_pane(HildonFileSelection *self)
{
  g_return_val_if_fail(HILDON_IS_FILE_SELECTION(self), 
    HILDON_FILE_SELECTION_PANE_NAVIGATION);

  return (HildonFileSelectionPane) self->priv->content_pane_last_used; 
}
