/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005-2006 Nokia Corporation.
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
 * HildonFileSystemModel
 */

#include <gtk/gtksettings.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtktreednd.h>
#include <osso-thumbnail-factory.h>
#include "hildon-file-system-model.h"
#include "hildon-file-system-private.h"
#include "hildon-file-system-settings.h"
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <errno.h>

#include "hildon-file-common-private.h"
#include <osso-mime.h>

/*  Reload contents of removable devices after this amount of seconds */
#define RELOAD_THRESHOLD 30  
#define THUMBNAIL_WIDTH 80      /* For images inside thumbnail folder */
#define THUMBNAIL_HEIGHT 60
#define THUMBNAIL_ICON 64       /* Size for icon theme icons used in
                                   thumbnail mode. Using the value 60 made
                                   icons to have size 60x51!!! */
#define DEFAULT_MAX_CACHE 50
#define MIN_CACHE 20

static const char *EXPANDED_EMBLEM_NAME = "qgn_list_gene_fldr_exp";
static const char *COLLAPSED_EMBLEM_NAME = "qgn_list_gene_fldr_clp";

static GQuark hildon_file_system_model_quark = 0;
static guint signal_finished_loading = 0;
static guint signal_device_disconnected = 0;

typedef struct {
    GNode *base_node;
    gboolean mounted;
} removable_type;

typedef struct {
    GtkFilePath *path;
    GtkFileInfo *info;
    GtkFileFolder *folder;
    GdkPixbuf *icon_cache;
    GdkPixbuf *icon_cache_expanded;
    GdkPixbuf *icon_cache_collapsed;
    GdkPixbuf *thumbnail_cache;
    gchar *name_cache;
    gchar *title_cache;
    gchar *key_cache;
    HildonFileSystemModelItemType type;
    HildonFileSystemModel *model;
    OssoThumbnailFactoryHandle thumbnail_handle;
    time_t load_time;
    gboolean present_flag;
    gboolean available; /* Set by code */
    GError *error;      /* Set if cannot get children */
    gchar *thumb_title, *thumb_author;
    gboolean mmc2;
} HildonFileSystemModelNode;

typedef struct {
    GNode *parent_node;
    GtkFileFolder *folder;
    GSList *children;
    GSList *iter;
} delayed_list_type;

struct _HildonFileSystemModelPrivate {
    GNode *roots;
    GType column_types[HILDON_FILE_SYSTEM_MODEL_NUM_COLUMNS];
    gint stamp;

    GtkFileSystem *filesystem;
    GtkIconTheme *icon_theme;
    removable_type mmc;
    removable_type mmc2;

    GtkFilePath *gateway_path;
  	
    GNode *local_device_node; 
    GNode *gateway_node;
    GtkWidget *ref_widget;      /* Any widget on the same screen, needed
                                   to return correct icons */
    GQueue *cache_queue;
    GQueue *delayed_lists;
    GQueue *reload_list;  /* Queueing all loads started implicitly by GtkTreeModel interface
                             instead of just errors is much more handy... */
    /* We have to keep references to emblems ourselves. They are used only
       while composed image is made, so our new cache approach would free
       them immediately after composed image is ready */
    GdkPixbuf *expanded_emblem, *collapsed_emblem;
    guint timeout_id;
    gboolean gateway_accessed;

    /* Properties */
    gchar *backend_name;
    gchar *alternative_root_dir;
    gboolean multiroot;
    
    /* fs_settings signal id's */
    gulong signal_handler_id[4];
};

/* Property id:s */
enum {
    PROP_BACKEND = 1,
    PROP_BACKEND_OBJECT,
    PROP_THUMBNAIL_CALLBACK,
    PROP_REF_WIDGET,
    PROP_ROOT_DIR,
    PROP_MULTI_ROOT
};

static void hildon_file_system_model_iface_init(GtkTreeModelIface * iface);
static void
hildon_file_system_model_drag_source_iface_init(GtkTreeDragSourceIface *iface);

static void hildon_file_system_model_init(HildonFileSystemModel * self);
static void hildon_file_system_model_class_init(HildonFileSystemModelClass
                                                * klass);
static GObject *
hildon_file_system_model_constructor(GType type,
                                     guint n_construct_properties,
                                     GObjectConstructParam *
                                     construct_properties);

static GNode *
hildon_file_system_model_add_node(GtkTreeModel * model,
                                  GNode * parent_node,
                                  GtkFileFolder * parent_folder,
                                  const GtkFilePath * path,
                                  HildonFileSystemModelItemType type);
static void hildon_file_system_model_remove_node_list(GtkTreeModel * model,
                                                      GNode * parent_node,
                                                      GSList * children);
static void hildon_file_system_model_change_node_list(GtkTreeModel * model,
                                                      GNode * parent_node,
                                                      GtkFileFolder *
                                                      folder,
                                                      GSList * children);
 static gboolean is_node_loaded(HildonFileSystemModelPrivate *priv,
  	                            GNode * node);
static GNode *
hildon_file_system_model_kick_node(GNode *node, gpointer data);
static void
clear_model_node_caches(HildonFileSystemModelNode *model_node);
static void
hildon_file_system_model_delayed_add_children(HildonFileSystemModel *
                                              model, GNode * node, gboolean force);
static void unlink_file_folder(GNode *node);
static gboolean
link_file_folder(GNode *node, const GtkFilePath *path, GError **error);
static void 
hildon_file_system_model_folder_finished_loading(GtkFileFolder *monitor, 
  gpointer data);
static void emit_node_changed(GNode *node);

static void
gateway_changed(GObject *settings, GParamSpec *param, gpointer data);
static void
btname_changed(GObject *settings, GParamSpec *param, gpointer data);
static void 
flightmode_changed(GObject *settings, GParamSpec *param, gpointer data);

#define CAST_GET_PRIVATE(o) \
    ((HildonFileSystemModelPrivate *) HILDON_FILE_SYSTEM_MODEL(o)->priv)
#define MODEL_FROM_NODE(n) ((HildonFileSystemModelNode *) n->data)->model

/* Note! G_IMPLEMENT_INTERFACE macros together form the 5th parameter for
   G_DEFINE_TYPE_EXTENDED */
G_DEFINE_TYPE_EXTENDED(HildonFileSystemModel, hildon_file_system_model,
                       G_TYPE_OBJECT, 0,
                       G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_MODEL,
                           hildon_file_system_model_iface_init)
                       G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_DRAG_SOURCE,
                           hildon_file_system_model_drag_source_iface_init))

static void handle_possibly_finished_node(GNode *node)
{
    GtkTreeIter iter;
    HildonFileSystemModel *model = MODEL_FROM_NODE(node);

    if (is_node_loaded(model->priv, node))
  	{
    	  GNode *child_node = g_node_first_child(node);
  	 
    	  while (child_node) {
    	     HildonFileSystemModelNode *model_node = child_node->data;
  	 
           /* We do not want to ever kick off devices by accident */ 
    	     if (model_node->present_flag || model_node->type >= HILDON_FILE_SYSTEM_MODEL_MMC)
    	         child_node = g_node_next_sibling(child_node);
    	     else
    	        child_node = hildon_file_system_model_kick_node(child_node, model);
    	  }

      iter.stamp = model->priv->stamp;
      iter.user_data = node;
      g_signal_emit(model, signal_finished_loading, 0, &iter);
  	}
}

/* This default handler is activated when device tree (mmc/gateway)
   is automatically removed. This handler removes the tree, but the
   root node stays in the tree. Thus iter provided as parameter will
   stay valid. */
static void hildon_file_system_model_real_device_disconnected(
  HildonFileSystemModel *self, GtkTreeIter *iter)
{
  GNode *node, *child;
  HildonFileSystemModelNode *model_node;

  g_return_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(self));
  g_return_if_fail(iter->stamp == self->priv->stamp);
  ULOG_INFO(__FUNCTION__);
  
  node = iter->user_data;
  unlink_file_folder(node);

  child = node->children;
  while (child)
    child = hildon_file_system_model_kick_node(child, self);

  model_node = node->data;
  g_return_if_fail(model_node != NULL);

  clear_model_node_caches(model_node);
  model_node->load_time = 0;
}

static void send_device_disconnected(GNode *node)
{
  HildonFileSystemModel *model;
  GtkTreeIter iter;

  g_return_if_fail(node != NULL);

  model = MODEL_FROM_NODE(node);
  iter.stamp = model->priv->stamp;
  iter.user_data = node;
  g_signal_emit(model, signal_device_disconnected, 0, &iter);
}

/* Returns the device node that is the parent of the given node. Fills
   the type pointer by node type if given */
static GNode *get_device_for_node(GNode *node, gint *type)
{
  while (node)  
  {
    HildonFileSystemModelNode *model_node = node->data;

    if (model_node && 
        model_node->type >= HILDON_FILE_SYSTEM_MODEL_MMC)
    {
      if (type) 
        *type = model_node->type;

      return node;
    }

    node = node->parent;
  }

  return NULL;
}

static void handle_load_error(GNode *node)
{
  HildonFileSystemModelNode *model_node;

  model_node = node->data;

  g_return_if_fail(model_node != NULL);
  g_return_if_fail(model_node->error != NULL);

  ULOG_ERR(model_node->error->message);

  /* We failed to connect to device before the call expired.
     We want disconnect the whole device in question, not 
     just to kick of the individual node that caused problems */
  if (g_error_matches(model_node->error,
      GTK_FILE_SYSTEM_ERROR, GTK_FILE_SYSTEM_ERROR_TIMEOUT))
  {
    GNode *device_node = get_device_for_node(node, NULL);
    if (device_node)
    {
      node = device_node;
      model_node = device_node->data;
    }
  }

  /* We do not kick of devices because of errors */
  if (model_node->type >= HILDON_FILE_SYSTEM_MODEL_MMC)
  {
    g_clear_error(&model_node->error);
    send_device_disconnected(node);
    emit_node_changed(node);
  }
  else if (g_error_matches(model_node->error, 
      GTK_FILE_SYSTEM_ERROR, GTK_FILE_SYSTEM_ERROR_NONEXISTENT))
    /* No longer present, we remove this node totally */
    hildon_file_system_model_kick_node(node, model_node->model);
  else /* Some other error, we represent this as disabled */
    emit_node_changed(node);    
}

static void delayed_list_free(delayed_list_type *list)
{
  gtk_file_paths_free(list->children);
  g_free(list);
}

static gboolean
hildon_file_system_model_delayed_add_node_list_timeout(gpointer data)
{
    HildonFileSystemModel *model;
    HildonFileSystemModelPrivate *priv;
    delayed_list_type *current_list;
    GNode *node;

    GDK_THREADS_ENTER();

    model = HILDON_FILE_SYSTEM_MODEL(data);
    priv = model->priv;

    /* Handle pending reloads one at a time. We can now handle errors
       inside delayed_add_children, since we are called from idle and
       we can do modifications to model. */
  	if ( (node = g_queue_pop_head(priv->reload_list)) != NULL)
  	{
  	  hildon_file_system_model_delayed_add_children(model, node, FALSE);
          GDK_THREADS_LEAVE();
  	  return TRUE;
  	}
  
    current_list = g_queue_peek_head(priv->delayed_lists);
    if (!current_list) { /* No items to insert => remove idle handler */
        priv->timeout_id = 0;
        GDK_THREADS_LEAVE();
        return FALSE;
    }

    /* Back to one addition per idle, old approach caused too
       long delays... */

      /* Ok, lets add one item from the list and return to main loop. This
         idle handler is then called again. */
        hildon_file_system_model_add_node(GTK_TREE_MODEL(data),
                                      current_list->parent_node,
                                      current_list->folder,
                                      current_list->iter->data,
                                      HILDON_FILE_SYSTEM_MODEL_UNKNOWN);

      current_list->iter = g_slist_next(current_list->iter);

    if (current_list->iter)
    {
      GDK_THREADS_LEAVE();
      return TRUE;  /* Ok, there is items left. Continue with this
                       idle handler */
    }

    /* Current list ends here. We now have to check
  	         if loading of some folder is really finished. If this is a
  	         case we then have to check if there are unflagged
  	         paths in that folder (paths to be removed) */

    node = current_list->parent_node;
    delayed_list_free(current_list);
    g_queue_pop_head(priv->delayed_lists);
    handle_possibly_finished_node(node);

    GDK_THREADS_LEAVE();

    return TRUE;
}

/* This is used as a callback */
static void
clear_present_flag(GNode *node)
{
  HildonFileSystemModelNode *model_node;
  	 
  g_assert(node != NULL && node->data != NULL);
  	 
  model_node = node->data;
  model_node->present_flag = FALSE;
}

static void 
hildon_file_system_model_ensure_idle(HildonFileSystemModel *self)
{
  if (self->priv->timeout_id == 0)
  {
    self->priv->timeout_id =
      g_idle_add_full (G_PRIORITY_DEFAULT_IDLE + 20,
                       hildon_file_system_model_delayed_add_node_list_timeout,
                       self, NULL);
  }
}

/* Adds the given list of children to be added to the model. The list must
   be a copy (this class takes ownership) */
static void
hildon_file_system_model_delayed_add_node_list(HildonFileSystemModel *
                                               model, GNode * parent,
                                               GtkFileFolder * folder,
                                               GSList * children)
{
    if (children) {
        delayed_list_type *new_list;

        new_list = g_new(delayed_list_type, 1);
        new_list->parent_node = parent;
        new_list->folder = folder;
        new_list->children = children;
        new_list->iter = children;

        hildon_file_system_model_ensure_idle(model);

        g_queue_push_tail(model->priv->delayed_lists, new_list);
    }
}

/* We are not any more called directly by GtkTreeModel interface methods, so we can modify
   model and send notifications */
static void
hildon_file_system_model_delayed_add_children(HildonFileSystemModel *
                                              model, GNode * node, gboolean force)
{
    HildonFileSystemModelNode *model_node;
    time_t current_time;
    gboolean removable;

    model_node = node->data;
    g_assert(model_node != NULL);

    /* Check if we really need to load children. We don't want 
        to reload if not needed and we don't want to restart existing
        async loadings. We also don't try to access gateway if not accessed yet.  */
    if (model_node->type < HILDON_FILE_SYSTEM_MODEL_FOLDER ||
       (model_node->type == HILDON_FILE_SYSTEM_MODEL_GATEWAY &&
  	    !model->priv->gateway_accessed) ||
      model_node->folder == NULL ||
      !is_node_loaded(model->priv, node) || model_node->path == NULL)
      return;

    current_time = time(NULL);
    removable = !gtk_file_system_path_is_local(model->priv->filesystem, 
        model_node->path);

    if (model_node->load_time == 0 || force || 
       ((abs(current_time - model_node->load_time) > RELOAD_THRESHOLD) &&
        (removable || model_node->error)))
    {
      /* Unix backend can fail to set children to NULL if it encounters error */
      GSList *children = NULL;

      g_clear_error(&model_node->error);
      _hildon_file_system_prepare_banner();

      /* List children do not work reliably with bluetooth connections. It can
         still succeed, even though the connection has died already. This
         if statement can be removed when the backend works better... */
      if (removable)
      {
        unlink_file_folder(node);
        if (!link_file_folder(node, model_node->path, &model_node->error))
        {
          _hildon_file_system_cancel_banner();
          ULOG_INFO("ERROR: %s", model_node->error->message);
          handle_load_error(node);
          return;
        }
      }

      /* We have to set load time every time. Otherwise we have a deadlock:
         load_children => error => notify => load_children => error */
      model_node->load_time = current_time;

      /* We clear present flags for existing children, so we are able to
          use this to detect if children are actually removed */
      g_node_children_foreach(node, G_TRAVERSE_ALL,
          (GNodeForeachFunc) clear_present_flag, NULL);

      ULOG_INFO("Delayed add for path %s", (char *) model_node->path);

      /* Unix backend sends finished loading even before returning children.
         This causes our internal bookkeeping fail. */
      g_signal_handlers_block_by_func(model_node->folder, 
        hildon_file_system_model_folder_finished_loading, model);      

      /* Patched GnomeVFS now also reports errors. */
      if (gtk_file_folder_list_children(model_node->folder, 
          &children, &model_node->error))
      {
        hildon_file_system_model_delayed_add_node_list(model, node, 
                          model_node->folder, children);
      }
      else 
      {
        g_assert(children == NULL);
        ULOG_INFO("ERROR: %s", model_node->error->message);
        handle_load_error(node);
      }

      g_signal_handlers_unblock_by_func(model_node->folder, 
        hildon_file_system_model_folder_finished_loading, model);

      _hildon_file_system_cancel_banner();
    }
}

static GNode *get_node(HildonFileSystemModelPrivate * priv,
                       GtkTreeIter * iter)
{
    if (iter) {
        g_return_val_if_fail(iter->stamp == priv->stamp, NULL);
        return iter->user_data;
    } else
        return priv->roots;
}

static gint max_cached_nodes(void)
{
  static gint max_len = 0;
  
  if (G_UNLIKELY(max_len < MIN_CACHE))
  {
    const gchar *env_var;
  
    env_var = g_getenv("FILE_SELECTION_MAX_CACHED_NODES");
    if (env_var)
      max_len = atoi(env_var);
    if (max_len < MIN_CACHE)
      max_len = DEFAULT_MAX_CACHE;
  }

  return max_len;
}

/* We make sure that given node is in the priority queue. If it
   already exists, we raise it to top. If not, we place it to top
   and possibly remove last item. */
static void update_cache_queue(HildonFileSystemModelPrivate *priv, GNode *node)
{
  GList *item;

  if ( (item = g_queue_find(priv->cache_queue, node)) != NULL)
  {
    g_queue_unlink(priv->cache_queue, item);
    g_queue_push_head_link(priv->cache_queue, item);
  }
  else
  {
    g_queue_push_head(priv->cache_queue, node);
    
    if (g_queue_get_length(priv->cache_queue) > max_cached_nodes())
    {
      HildonFileSystemModelNode *model_node;
      node = g_queue_pop_tail(priv->cache_queue);
      model_node = node->data;
      ULOG_INFO("Clearing caches for %s", (char *) model_node->path);
      clear_model_node_caches(model_node);
    }
  }
}

/**********************************************/
/* Start of GTK_TREE_MODEL interface methods */
/**********************************************/

static GtkTreeModelFlags hildon_file_system_model_get_flags(GtkTreeModel *
                                                            model)
{
    return GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint hildon_file_system_model_get_n_columns(GtkTreeModel * model)
{
    return HILDON_FILE_SYSTEM_MODEL_NUM_COLUMNS;
}

static GType hildon_file_system_model_get_column_type(GtkTreeModel * model,
                                                      gint index)
{
    g_return_val_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(model), G_TYPE_NONE);
    g_return_val_if_fail(0 <= index
                         && index < HILDON_FILE_SYSTEM_MODEL_NUM_COLUMNS,
                         G_TYPE_NONE);

    return CAST_GET_PRIVATE(model)->column_types[index];
}

/* The following method is mostly borrowed from GtkTreeStore */
static gboolean hildon_file_system_model_get_iter(GtkTreeModel * model,
                                                  GtkTreeIter * iter,
                                                  GtkTreePath * path)
{
    gint *indices;
    gint i, depth;

    TRACE;
    indices = gtk_tree_path_get_indices(path);
    depth = gtk_tree_path_get_depth(path);

    g_return_val_if_fail(depth > 0, FALSE);

    if (!gtk_tree_model_iter_nth_child(model, iter, NULL, indices[0]))
        return FALSE;

    for (i = 1; i < depth; i++) {
        GtkTreeIter parent_iter = *iter;

        if (!gtk_tree_model_iter_nth_child
            (model, iter, &parent_iter, indices[i]))
            return FALSE;
    }

    return TRUE;
}

static GtkTreePath *hildon_file_system_model_get_path(GtkTreeModel * model,
                                                      GtkTreeIter * iter)
{
    GNode *node, *parent;
    GtkTreePath *path;
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(model);

    g_return_val_if_fail(iter->stamp == priv->stamp, NULL);
    g_return_val_if_fail(iter != NULL, NULL);

    TRACE;

    node = iter->user_data;
    path = gtk_tree_path_new();

    g_assert(node != NULL);

    while (!G_NODE_IS_ROOT(node)) {     /* Don't take take fake root into
                                           account */
        parent = node->parent;
        gtk_tree_path_prepend_index(path,
                                    g_node_child_position(parent, node));
        node = parent;
    }

    return path;
}

inline static GdkPixbuf
    *hildon_file_system_model_create_image(HildonFileSystemModelPrivate *
                                           priv,
                                           HildonFileSystemModelNode *
                                           model_node, gint size)
{
    return _hildon_file_system_create_image(priv->filesystem, 
        priv->ref_widget, model_node->path, 
        model_node->type, size, model_node->mmc2 ? TRUE : FALSE);
}

/* Creates a new pixbuf conatining normal image and given emblem */
static GdkPixbuf
    *hildon_file_system_model_create_composite_image
    (HildonFileSystemModelPrivate * priv,
     HildonFileSystemModelNode * model_node, GdkPixbuf *emblem)
{
    GdkPixbuf *plain, *result;

    plain =
        hildon_file_system_model_create_image(priv, model_node,
                                              TREE_ICON_SIZE);
    if (!plain) return NULL;
    if (!emblem) return plain;

    result = gdk_pixbuf_copy(plain);
    if (!result)  /* Not an assert anymore */
      return plain;

    /* This causes read errors according to valgrind. I wonder why is that 
     */
    gdk_pixbuf_composite(emblem, result, 0, 0, 
      MIN(gdk_pixbuf_get_width(emblem), gdk_pixbuf_get_width(result)), 
      MIN(gdk_pixbuf_get_height(emblem), gdk_pixbuf_get_height(result)),
      0, 0, 1, 1, GDK_INTERP_NEAREST, 255);
    g_object_unref(plain);

    return result;
}

/* This function searches the insert queue and checks if it contains
   something for the given node */
static gint queue_finder(gconstpointer a, gconstpointer b)
{
    const delayed_list_type *list = a;
    const GNode *search_node = b;

    if (list->parent_node == search_node)
      return 0;

    return -1;
}

static gboolean is_node_loaded(HildonFileSystemModelPrivate *priv,
                           GNode * node)
{
  HildonFileSystemModelNode *model_node = node->data;

  if (!model_node->folder)  /* If there is no folder then think this as loaded */
    return TRUE;

  return g_queue_find(priv->reload_list, node) == NULL &&
     gtk_file_folder_is_finished_loading(model_node->folder) &&
     g_queue_find_custom(priv->delayed_lists, node, queue_finder) == NULL; 
}

static void emit_node_changed(GNode *node)
{
  HildonFileSystemModelNode *model_node;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;

  g_assert(node != NULL);

  model_node = node->data;
  model = GTK_TREE_MODEL(model_node->model);
  iter.stamp = CAST_GET_PRIVATE(model)->stamp;
  iter.user_data = node;
  path = hildon_file_system_model_get_path(model, &iter);
  gtk_tree_model_row_changed(model, path, &iter);
  gtk_tree_path_free(path);
}

static void 
thumbnail_handled(OssoThumbnailFactoryHandle handle,
  gpointer data, GdkPixbuf *thumbnail, GError *error)
{
  GNode *node;
  HildonFileSystemModelNode *model_node;

  node = data;
  model_node = node->data;

  g_assert(model_node != NULL);
  g_assert(handle == model_node->thumbnail_handle);
  ULOG_DEBUG(__FUNCTION__);

  model_node->thumbnail_handle = NULL;

  if (!error)
  {
    const gchar *tmp_title, *tmp_artist, *noimage;
    gchar *thumb_title, *thumb_author;

    g_assert(GDK_IS_PIXBUF(thumbnail));

    tmp_title = gdk_pixbuf_get_option(thumbnail,
        OSSO_THUMBNAIL_OPTION_PREFIX "Title");
    tmp_artist = gdk_pixbuf_get_option(thumbnail,
        OSSO_THUMBNAIL_OPTION_PREFIX "Artist");
    noimage = gdk_pixbuf_get_option(thumbnail,
        OSSO_THUMBNAIL_OPTION_PREFIX "Noimage");

    if(tmp_title)
    {
        thumb_title = g_strdup(tmp_title);
        g_strstrip(thumb_title);
        model_node->thumb_title = thumb_title;
    }
    if(tmp_artist)
    {
        thumb_author = g_strdup(tmp_artist);
        g_strstrip(thumb_author);
        model_node->thumb_author = thumb_author;
    }

    if(!noimage)
    {
    if (model_node->thumbnail_cache)    
      g_object_unref(model_node->thumbnail_cache);
   
    g_object_ref(thumbnail);    /* Thumbnail factory will release reference to this */
    model_node->thumbnail_cache = thumbnail;
    }

    ULOG_DEBUG("Size = %dx%d", gdk_pixbuf_get_width(thumbnail), gdk_pixbuf_get_height(thumbnail));

    emit_node_changed(node);
  }
}

static GdkPixbuf *get_expanded_emblem(HildonFileSystemModelPrivate *priv)
{
  if (!priv->expanded_emblem)
    priv->expanded_emblem = _hildon_file_system_load_icon_cached(
      priv->icon_theme, EXPANDED_EMBLEM_NAME, TREE_ICON_SIZE);

  return priv->expanded_emblem;
}

static GdkPixbuf *get_collapsed_emblem(HildonFileSystemModelPrivate *priv)
{
  if (!priv->collapsed_emblem)
    priv->collapsed_emblem = _hildon_file_system_load_icon_cached(
      priv->icon_theme, COLLAPSED_EMBLEM_NAME, TREE_ICON_SIZE);

  return priv->collapsed_emblem;
}

static void hildon_file_system_model_get_value(GtkTreeModel * model,
                                               GtkTreeIter * iter,
                                               gint column, GValue * value)
{
    GNode *node;
    GtkFileInfo *info;
    GtkFilePath *path;
    GtkFileSystem *fs;
    HildonFileSystemModelNode *model_node;
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(model);

    g_return_if_fail(iter && priv->stamp == iter->stamp);
    TRACE;
#if 0  /* Enable the following to get statistics which info is requested.
          Actually TYPE and SORT_KEY are the two most often asked fields */
    {
      static gint columns[HILDON_FILE_SYSTEM_MODEL_NUM_COLUMNS] = { 0 };
      gint i;

      columns[column]++;
      for (i = 0; i < HILDON_FILE_SYSTEM_MODEL_NUM_COLUMNS; i++)
        fprintf(stderr, "%d ", columns[i]);

      fprintf(stderr, "\n");
    }
#endif
    g_value_init(value, priv->column_types[column]);

    node = iter->user_data;
    model_node = node->data;
    g_assert(model_node != NULL);

    info = model_node->info;
    path = model_node->path;
    fs = priv->filesystem;

    switch (column) {
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_GTK_PATH:
        g_assert(path != NULL);
        g_value_set_boxed(value, path);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_LOCAL_PATH:
        g_assert(path != NULL);
        g_value_take_string
            (value, gtk_file_system_path_to_filename(fs, path));
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_URI:
        g_assert(path != NULL);
        g_value_take_string(value,
                           gtk_file_system_path_to_uri(fs, path));
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_NAME:
        /* Gtk+'s display name contains also extension */
        if (model_node->name_cache == NULL)
          model_node->name_cache = _hildon_file_system_create_file_name(fs, 
              path, model_node->type, model_node->info,
              model_node->mmc2 ? TRUE : FALSE);
        g_value_set_string(value, model_node->name_cache);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_DISPLAY_NAME:
        if (!model_node->title_cache)
          model_node->title_cache = _hildon_file_system_create_display_name(fs, 
              path, model_node->type, model_node->info,
              model_node->mmc2 ? TRUE : FALSE);
        g_value_set_string(value, model_node->title_cache);

        /* We try to reload children, if we are actually showing something */
        /* This action should be performed for initial access only. Otherwise
           we always end up loading all the children of a node after long 
           time of inactivity. This is not good in case of gateway, since the
           old BT connection is already timed out. UI asks for reloads if
           user actually does something, so we just make sure that children
           initially appear where they should. */
        if (model_node->type >= HILDON_FILE_SYSTEM_MODEL_FOLDER &&
            model_node->load_time == 0)
          _hildon_file_system_model_queue_reload(
            HILDON_FILE_SYSTEM_MODEL(model), iter, FALSE);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_SORT_KEY:
        /* We cannot just use display_key from GtkFileInfo, because it is case sensitive */
        if (!model_node->key_cache)
        {
          gchar *name, *casefold;

          name = _hildon_file_system_create_file_name(fs, path, 
                            model_node->type, model_node->info,
                            model_node->mmc2 ? TRUE : FALSE);
          casefold = g_utf8_casefold(name, -1);
          model_node->key_cache = g_utf8_collate_key(casefold, -1);
          g_free(casefold);
          g_free(name);
        }

        g_value_set_string(value, model_node->key_cache);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_MIME_TYPE:
        /* get_mime_type do not make a duplicate */
        g_value_set_string(value, info ? gtk_file_info_get_mime_type(info) : "");
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_SIZE:
        g_value_set_int64(value, info ? gtk_file_info_get_size(info) : 0);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_TIME:
        g_value_set_int64(value,
                          info ? gtk_file_info_get_modification_time(info) : 0);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_FOLDER:
        g_value_set_boolean(value, 
            model_node->type >= HILDON_FILE_SYSTEM_MODEL_FOLDER);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE:
        g_value_set_boolean(value, path != NULL && model_node->available && 
            /* Folders that cause access errors are dimmed. Devices are not */
            (!model_node->error || model_node->type >= HILDON_FILE_SYSTEM_MODEL_MMC) &&
            (model_node->type != HILDON_FILE_SYSTEM_MODEL_GATEWAY || 
	     !_hildon_file_system_settings_get_flight_mode()));
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_HAS_LOCAL_PATH:
        g_value_set_boolean(value,
            path ? gtk_file_system_path_is_local(fs, path) : FALSE);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_TYPE:
        g_value_set_int(value, model_node->type);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON:
        if (!model_node->icon_cache)
            model_node->icon_cache =
                hildon_file_system_model_create_image(priv, model_node,
                                                      TREE_ICON_SIZE);

        g_value_set_object(value, model_node->icon_cache);
        update_cache_queue(priv, node);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON_EXPANDED:
        if (!model_node->icon_cache_expanded)
            model_node->icon_cache_expanded =
                hildon_file_system_model_create_composite_image
                    (priv, model_node, get_expanded_emblem(priv));

        g_value_set_object(value, model_node->icon_cache_expanded);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON_COLLAPSED:
        if (!model_node->icon_cache_collapsed)
            model_node->icon_cache_collapsed =
                hildon_file_system_model_create_composite_image
                    (priv, model_node, get_collapsed_emblem(priv));

        g_value_set_object(value, model_node->icon_cache_collapsed);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_THUMBNAIL:
        if (!model_node->thumbnail_cache) {
            if (path && !model_node->thumbnail_handle)
            {
              gchar *uri =
                  gtk_file_system_path_to_uri(priv->filesystem, path);

              if (uri && info && gtk_file_info_get_mime_type(info)) 
              {  /* This can fail with GtkFileSystemUnix if the
                           name contains invalid UTF-8 */
                model_node->thumbnail_handle =
                    osso_thumbnail_factory_load(uri, 
                        gtk_file_info_get_mime_type(info),
                        THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, 
                        thumbnail_handled, node);
              }
              g_free(uri);
            }
            else if (model_node->thumbnail_handle)
              osso_thumbnail_factory_move_front(model_node->thumbnail_handle);

            /* the following if clause handles the hourglass icon */
            if (path && info && !model_node->thumbnail_cache)
            {
              OssoMimeCategory cat;
              const gchar *mime_type;
              gchar *uri = gtk_file_system_path_to_uri(priv->filesystem,
                                                       path);
              
              mime_type = gtk_file_info_get_mime_type(info);
              /* FIXME: hack to workaround problem with Sketch files.
               * Second check is because we cannot make thumbnails for
               * images on the Gateway. */
              if (strcmp(mime_type, "sketch/png") != 0 &&
                  (uri && !g_str_has_prefix(uri, "obex://")))
              {
                cat = osso_mime_get_category_for_mime_type(mime_type);
                if (cat == OSSO_MIME_CATEGORY_IMAGES)
                  model_node->thumbnail_cache =
                    _hildon_file_system_load_icon_cached(
                        gtk_icon_theme_get_default(),
                        "qgn_list_gene_image_file_wait", THUMBNAIL_ICON);
              }
              g_free(uri);
            }

            if (!model_node->thumbnail_cache)
              model_node->thumbnail_cache =
                 hildon_file_system_model_create_image(priv, model_node,
                                                       THUMBNAIL_ICON);
        }
        g_value_set_object(value, model_node->thumbnail_cache);
        update_cache_queue(priv, node);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_LOAD_READY:
        g_value_set_boolean(value, is_node_loaded(priv, node));
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_FREE_SPACE:
    {
        gint64 free_space = 0;

        if (model_node->type == HILDON_FILE_SYSTEM_MODEL_MMC && path)
        {
          gchar *local_path;

          local_path = gtk_file_system_path_to_filename(fs, path);
          if (local_path)
          {
            struct statfs buf;

            if (statfs(local_path, &buf) == 0)
              free_space = ((gint64) buf.f_bavail) * ((gint64) buf.f_bsize);

            g_free(local_path);
          }       
        }

        g_value_set_int64(value, free_space);
        break;
    }
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_TITLE:
        g_value_set_string(value, model_node->thumb_title);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_AUTHOR:
        g_value_set_string(value, model_node->thumb_author);
        break;
    case HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_HIDDEN:
    {
      gboolean result;

      if (model_node->type > HILDON_FILE_SYSTEM_MODEL_FOLDER || !info)
        result = FALSE;
      else
        result = gtk_file_info_get_is_hidden(info);

      g_value_set_boolean(value, result);
      break;
    }
    default:
        g_assert_not_reached();
    };
}

static gboolean hildon_file_system_model_iter_next(GtkTreeModel * model,
                                                   GtkTreeIter * iter)
{
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(model);

    g_return_val_if_fail(iter != NULL, FALSE);
    g_return_val_if_fail(priv->stamp == iter->stamp, FALSE);
    TRACE;
    iter->user_data = g_node_next_sibling((GNode *) iter->user_data);

    return iter->user_data != NULL;
}

static gint hildon_file_system_model_iter_n_children(GtkTreeModel * model,
                                                     GtkTreeIter * iter)
{
    HildonFileSystemModel *self;
    HildonFileSystemModelPrivate *priv;

    TRACE;
    g_assert(HILDON_IS_FILE_SYSTEM_MODEL(model));
    self = HILDON_FILE_SYSTEM_MODEL(model);
    priv = self->priv;

    if (iter == NULL)   /* Roots are always in tree, we don't need to ask
                           loading */
        return g_node_n_children(priv->roots);

    g_return_val_if_fail(priv->stamp == iter->stamp, 0);

    return g_node_n_children(iter->user_data);
}

static gboolean hildon_file_system_model_iter_has_child(GtkTreeModel *
                                                        model,
                                                        GtkTreeIter * iter)
{
    TRACE;
    return (hildon_file_system_model_iter_n_children(model, iter) > 0);
}

static gboolean hildon_file_system_model_iter_nth_child(GtkTreeModel *
                                                        model,
                                                        GtkTreeIter * iter,
                                                        GtkTreeIter *
                                                        parent, gint n)
{
    GNode *node;
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(model);
    TRACE;

    if (parent) {
        g_return_val_if_fail(parent->stamp == priv->stamp, FALSE);
        node = g_node_nth_child(parent->user_data, n);
    } else
        node = g_node_nth_child(priv->roots, n);

    iter->stamp = priv->stamp;
    iter->user_data = node;

    return node != NULL;
}

static gboolean hildon_file_system_model_iter_children(GtkTreeModel *
                                                       model,
                                                       GtkTreeIter * iter,
                                                       GtkTreeIter *
                                                       parent)
{
    TRACE;
    return hildon_file_system_model_iter_nth_child(model, iter, parent, 0);
}

static gboolean hildon_file_system_model_iter_parent(GtkTreeModel * model,
                                                     GtkTreeIter * iter,
                                                     GtkTreeIter * child)
{
    GNode *node;
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(model);
    
    TRACE;
    g_return_val_if_fail(child->stamp == priv->stamp, FALSE);

    node = child->user_data;
    iter->stamp = priv->stamp;
    iter->user_data = node->parent;

    return node->parent != NULL && node->parent != priv->roots;
}

/*********************************************/
/* End of GTK_TREE_MODEL interface methods */
/*********************************************/

static gint path_compare_helper(gconstpointer a, gconstpointer b)
{
    /* This is really a macro, so we cannot pass it directly to search
       function */
    if (a == NULL || b == NULL)
      return 1;

    return gtk_file_path_compare((GtkFilePath *) a, (GtkFilePath *) b);
}

static GNode
    *hildon_file_system_model_search_folder(GtkFileFolder * folder)
{
    return (GNode *) g_object_get_qdata(G_OBJECT(folder),
                                        hildon_file_system_model_quark);
}

static void hildon_file_system_model_files_added(GtkFileFolder * monitor,
                                                 GSList * paths,
                                                 gpointer data)
{
  if (paths != NULL)
  {
    GNode *node;

    ULOG_INFO("Adding files (monitor = %p)", (void *) monitor);

    node = hildon_file_system_model_search_folder(monitor);
    if (node != NULL)
        hildon_file_system_model_delayed_add_node_list(data, node, monitor,
                                                       gtk_file_paths_copy
                                                       (paths));
    else
        ULOG_ERR_F("Data destination not found!");
  }
}

static void hildon_file_system_model_files_removed(GtkFileFolder * monitor,
                                                   GSList * paths,
                                                   gpointer data)
{
  if (paths != NULL)
  {
    GNode *node;

    ULOG_INFO("Removing files (monitor = %p)", (void *) monitor);

    node = hildon_file_system_model_search_folder(monitor);
    if (node != NULL)
        hildon_file_system_model_remove_node_list(data, node, paths);
    else
        ULOG_ERR_F("Data destination not found!");
  }
}

static void hildon_file_system_model_dir_removed(GtkFileFolder * monitor,
                                                 gpointer data)
{
    ULOG_ERR_F("Dir removed callback called, but this method is not "
               "implemented (and probably there is no need to implement "
               "it either).");
}

static void hildon_file_system_model_files_changed(GtkFileFolder * monitor,
                                                   GSList * paths,
                                                   gpointer data)
{
  if (paths != NULL)
  {
    GNode *node;

    ULOG_INFO("Files changed (monitor = %p)", (void *) monitor);

    node = hildon_file_system_model_search_folder(monitor);
    if (node != NULL)
        hildon_file_system_model_change_node_list(data, node, monitor,
                                                  paths);
    else
        ULOG_ERR_F("Data destination not found!");
  }
}

static void hildon_file_system_model_folder_finished_loading(GtkFileFolder *monitor, gpointer data)
{
  GNode *node = hildon_file_system_model_search_folder(monitor);
  g_assert(node != NULL);
  ULOG_INFO("Finished loading (monitor = %p)", (void *) monitor);
  handle_possibly_finished_node(node);
}

static GNode
    *hildon_file_system_model_search_path_internal
    (HildonFileSystemModelPrivate * priv, GNode * parent_node,
     const GtkFilePath * path, gboolean recursively)
{
    const gchar *folder_string, *test_string;
    GNode *node;

    g_assert(parent_node != NULL && path != NULL);

    /* Not allocated dynamically */
    folder_string = gtk_file_path_get_string(path);
    
    for (node = g_node_first_child(parent_node); node;
         node = g_node_next_sibling(node)) {
        HildonFileSystemModelNode *model_node = node->data;

        if (model_node->path == NULL)
          continue;

        test_string = gtk_file_path_get_string(model_node->path);

        /* Let's make sure that we survise with folder names both ending and not ending to slash */
        if (_hildon_file_system_compare_ignore_last_separator(folder_string, test_string))
            return node;

        if (recursively) {
            /* Allways peek all children from top level, because MMC:s are 
               not neccesarily located physically under device directories 
             */
          gint test_len = strlen(test_string);

          if (parent_node == priv->roots ||
                (g_ascii_strncasecmp(folder_string, test_string, test_len) == 0
                 && (folder_string[test_len] == G_DIR_SEPARATOR)))
          {
                GNode *result = 
                      hildon_file_system_model_search_path_internal(priv,
                                                                     node,
                                                                     path,
                                                                     TRUE);
                if (result) return result;
          }
        }
    }

    return NULL;
}

static void hildon_file_system_model_send_has_child_toggled(GtkTreeModel *
                                                            model,
                                                            GNode *
                                                            parent_node)
{
    GtkTreePath *tree_path;
    GtkTreeIter iter;
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(model);

    iter.stamp = priv->stamp;
    iter.user_data = parent_node;
    tree_path = hildon_file_system_model_get_path(model, &iter);
    gtk_tree_model_row_has_child_toggled(model, tree_path, &iter);
    gtk_tree_path_free(tree_path);
}

static gint search_folder_helper(gconstpointer a, gconstpointer b)
{
  const delayed_list_type *list = a;
  return list->folder != b; /* We have to return 0 if found */
}

static void
unlink_file_folder(GNode *node)
{
  HildonFileSystemModelNode *model_node = node->data;
  g_assert(model_node != NULL);

  if (model_node->folder) 
  {
    GQueue *queue;
    GList *link;

    g_object_set_qdata(G_OBJECT(model_node->folder),
                       hildon_file_system_model_quark, NULL);

    g_signal_handlers_disconnect_by_func
        (model_node->folder, 
         (gpointer) hildon_file_system_model_dir_removed,
         model_node->model);
    g_signal_handlers_disconnect_by_func
        (model_node->folder, 
         (gpointer) hildon_file_system_model_files_added,
         model_node->model);
    g_signal_handlers_disconnect_by_func
        (model_node->folder, 
         (gpointer) hildon_file_system_model_files_removed,
         model_node->model);
    g_signal_handlers_disconnect_by_func
        (model_node->folder,
         (gpointer) hildon_file_system_model_files_changed,
         model_node->model);
    g_signal_handlers_disconnect_by_func
        (model_node->folder,
         (gpointer) hildon_file_system_model_folder_finished_loading,
         model_node->model);

    /* Remove possibly pending nodes from queue */
    queue = model_node->model->priv->delayed_lists;
    while ((link = g_queue_find_custom(queue, 
            model_node->folder, search_folder_helper)) != NULL)
    {
      delayed_list_free(link->data);
      g_queue_delete_link(queue, link);
    }

    g_object_unref(model_node->folder);
    model_node->folder = NULL;
  }
}

static gboolean
link_file_folder(GNode *node, const GtkFilePath *path, GError **error)
{
  HildonFileSystemModel *model;
  HildonFileSystemModelNode *model_node;

  g_assert(node != NULL && path != NULL);
  model_node = node->data;
  g_assert(model_node != NULL);

  /* Folder already exists */
  if (model_node->folder)
    return TRUE;

  model = model_node->model;
  g_assert(HILDON_IS_FILE_SYSTEM_MODEL(model));

  model_node->folder =
     gtk_file_system_get_folder(model->priv->filesystem, 
            path, GTK_FILE_INFO_ALL, error);

   if (!GTK_IS_FILE_FOLDER(model_node->folder))
   {
      ULOG_ERR_F("Failed to create monitor for path %s", (char *) path);
      return FALSE;
   }

   g_signal_connect_object(model_node->folder, "deleted",
       G_CALLBACK(hildon_file_system_model_dir_removed),
       model, 0);
   g_signal_connect_object(model_node->folder, "files-added",
       G_CALLBACK(hildon_file_system_model_files_added),
       model, 0);
   g_signal_connect_object(model_node->folder, "files-removed",
       G_CALLBACK
       (hildon_file_system_model_files_removed), model, 0);
   g_signal_connect_object(model_node->folder, "files-changed",
       G_CALLBACK
       (hildon_file_system_model_files_changed), model, 0);
   g_signal_connect_object(model_node->folder, "finished-loading",
       G_CALLBACK
        (hildon_file_system_model_folder_finished_loading), model, 0);

   g_object_set_qdata(G_OBJECT(model_node->folder),
        hildon_file_system_model_quark, node);

  if (!model_node->path)
    model_node->path = gtk_file_path_copy(path);

  return TRUE;
}

static gboolean hildon_file_system_model_destroy_model_node(GNode * node,
                                                        gpointer data)
{
    HildonFileSystemModelNode *model_node = node->data;
    g_assert(HILDON_IS_FILE_SYSTEM_MODEL(data));

    g_queue_remove_all(HILDON_FILE_SYSTEM_MODEL(data)->priv->reload_list, node);
    g_queue_remove_all(HILDON_FILE_SYSTEM_MODEL(data)->priv->cache_queue, node);

    if (model_node)
    {
      ULOG_INFO("Remove [%s]", (const char *) model_node->path);

      gtk_file_path_free(model_node->path);
      unlink_file_folder(node);

      if (model_node->info)
        gtk_file_info_free(model_node->info);

      g_clear_error(&model_node->error);

      clear_model_node_caches(model_node);
      g_free(model_node);
      node->data = NULL;
    }

    return FALSE;
}

/* Kicks off the node and all the children. Both GNodes and ModelNodes.
    returns the next sibling of the deleted node */
static GNode * 
hildon_file_system_model_kick_node(GNode *node, gpointer data)
{
  HildonFileSystemModelPrivate *priv;
  GNode *parent_node;
  GNode *destroy_node = node;

  GtkTreePath *tree_path;
  GtkTreeIter iter;

  priv = CAST_GET_PRIVATE(data);

  g_assert(node != NULL && (node->parent != NULL || node == priv->roots));

  iter.stamp = priv->stamp;
  iter.user_data = destroy_node;
  tree_path =
        hildon_file_system_model_get_path(GTK_TREE_MODEL(data), &iter);

  gtk_tree_model_row_deleted(GTK_TREE_MODEL(data), tree_path);
  gtk_tree_path_free(tree_path);
  	 
  parent_node = node->parent;
  node = g_node_next_sibling(node);

  g_node_traverse(destroy_node, G_POST_ORDER, G_TRAVERSE_ALL, 
      -1, hildon_file_system_model_destroy_model_node, data);

  g_node_destroy(destroy_node);

  if (parent_node && parent_node != priv->roots && parent_node->children ==NULL)
    hildon_file_system_model_send_has_child_toggled( GTK_TREE_MODEL(data), 
						     parent_node);

  return node;
}

static void set_mmc_state(GtkTreeModel *model, GtkFilePath *mount_path,
                          gboolean internal_mmc)
{
    HildonFileSystemModelNode *model_node;
    HildonFileSystemModelPrivate *priv;
    GNode *node;
    GError *error = NULL;
    gboolean new_state;

    priv = CAST_GET_PRIVATE(model);
    new_state = mount_path != NULL;

    if (internal_mmc) {
      node = priv->mmc2.base_node;

      ULOG_INFO_F("new state = %d, old state = %d", new_state,
                  priv->mmc2.mounted);

      /* MMC device not in the tree or same state as previously */
      if (node == NULL || new_state == priv->mmc2.mounted)
        return;
    } else {
      node = priv->mmc.base_node;

      ULOG_INFO_F("new state = %d, old state = %d", new_state,
                  priv->mmc.mounted);

      /* MMC device not in the tree or same state as previously */
      if (node == NULL || new_state == priv->mmc.mounted)
        return;
    }

    model_node = node->data;

    if (new_state) {
        ULOG_INFO_L("MMC %d mounted", internal_mmc);

        if (!link_file_folder(node, mount_path, &error))
        {
          ULOG_ERR(error->message);
          g_error_free(error);
          new_state = FALSE;
        }
    } else {
        ULOG_INFO_L("MMC %d unmounted", internal_mmc);
        
        gtk_file_path_free(model_node->path);
        send_device_disconnected(node);        
        model_node->path = NULL;
        model_node->load_time = 0;
    }

    clear_model_node_caches(model_node);
    if (internal_mmc) {
        priv->mmc2.mounted = new_state;
    } else {
        priv->mmc.mounted = new_state;
    }
    emit_node_changed(node);
}

static gboolean real_volumes_changed(gpointer data)
{
    GtkFileSystemVolume *vol;
    GtkFilePath *path = NULL;
    GtkFileSystem *fs = CAST_GET_PRIVATE(data)->filesystem;

    ULOG_DEBUG_F("entered");

    vol = _hildon_file_system_get_volume_for_location(fs, 
               HILDON_FILE_SYSTEM_MODEL_MMC, NULL, FALSE);
    if (vol)
    {
      if (gtk_file_system_volume_get_is_mounted(fs, vol))
        path = gtk_file_system_volume_get_base_path(fs, vol);

      gtk_file_system_volume_free(fs, vol);
    }

    set_mmc_state(GTK_TREE_MODEL(data), path, FALSE);
    gtk_file_path_free(path);

    /* second memory card */

    if (g_getenv("INTERNAL_MMC_MOUNTPOINT") != NULL)
    {
      path = NULL;
      vol = _hildon_file_system_get_volume_for_location(fs, 
               HILDON_FILE_SYSTEM_MODEL_MMC, NULL, TRUE);
      if (vol)
      {
        if (gtk_file_system_volume_get_is_mounted(fs, vol))
          path = gtk_file_system_volume_get_base_path(fs, vol);

        gtk_file_system_volume_free(fs, vol);
      }

      set_mmc_state(GTK_TREE_MODEL(data), path, TRUE);
      gtk_file_path_free(path);
    }

    return FALSE;
}

static GNode *
hildon_file_system_model_add_node(GtkTreeModel * model,
                                  GNode * parent_node,
                                  GtkFileFolder * parent_folder,
                                  const GtkFilePath * path,
                                  HildonFileSystemModelItemType type)
{
    GNode *node;
    HildonFileSystemModelPrivate *priv;
    HildonFileSystemModelNode *model_node;
    GtkFileInfo *file_info = NULL;
    GtkTreePath *tree_path;
    GtkTreeIter iter;
    HildonFileSystemModelItemType special_type;

    /* Path can be NULL for removable devices that are not present */
    g_return_val_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(model), NULL);
    g_return_val_if_fail(parent_node != NULL, NULL);

    priv = CAST_GET_PRIVATE(model);

    ULOG_INFO("Adding %s", (const char *) path);

    /* First check if this item is already part of the model */
    if (path)
    {
        GNode *node;
  	 
  	    node = hildon_file_system_model_search_path_internal(priv,
  	                  parent_node, path, FALSE);
  	 
  	    if (node)
  	    {
  	       HildonFileSystemModelNode *model_node = node->data;
  	       g_assert(model_node);
  	       model_node->present_flag = TRUE;
           return NULL;
         }
    }

    _hildon_file_system_prepare_banner();

    if (parent_folder) {
        GError *error = NULL;
        /* This can cause main loop execution on vfs backend */
        g_assert(path != NULL); /* Non-existing paths should not come here */

      /* If we have received the path that we are adding by some other
         means than listing the folder (like via some API function),
         then the backend will send us "files-added" signal immediately 
         after we first time use it. This is *not good*, because it 
         places "garbage list" info processing queue and rest of 
         the model believes that model is loading. See bug #14040. */
        g_signal_handlers_block_by_func(parent_folder,
          hildon_file_system_model_files_added, model);
        file_info = gtk_file_folder_get_info(parent_folder, path, &error);
        g_signal_handlers_unblock_by_func(parent_folder,
          hildon_file_system_model_files_added, model);

        /* If file is created and then renamed it can happen that file with this name no longer exists. */
        if (error)
        {
          ULOG_ERR(error->message);
          g_error_free(error);
          return NULL;
        }

        g_assert(file_info != NULL);
        g_assert(type == HILDON_FILE_SYSTEM_MODEL_UNKNOWN);
        type =
            (gtk_file_info_get_is_folder(file_info) ?
             HILDON_FILE_SYSTEM_MODEL_FOLDER :
             HILDON_FILE_SYSTEM_MODEL_FILE);
    } 

    g_assert(type != HILDON_FILE_SYSTEM_MODEL_UNKNOWN);

    model_node = g_new0(HildonFileSystemModelNode, 1);
    model_node->info = file_info;
    model_node->model = HILDON_FILE_SYSTEM_MODEL(model);
    model_node->present_flag = TRUE;
    model_node->available = TRUE;
    model_node->mmc2 = FALSE;

    if (path)
      model_node->path = gtk_file_path_copy(path);    

    node = g_node_new(model_node);
    g_node_append(parent_node, node);

    if (path && type >= HILDON_FILE_SYSTEM_MODEL_FOLDER)
    {
  	    /* We don't want to establish gateway connection until
  	        user makes tries to access gateway. */
  	    if (type != HILDON_FILE_SYSTEM_MODEL_GATEWAY ||
  	        priv->gateway_accessed)
  	    {
          GError *error = NULL;

          if (!link_file_folder(node, path, &error))
          {
            ULOG_ERR(error->message);
            g_error_free(error);
          }
        }

        special_type = _hildon_file_system_get_special_location(priv->filesystem, path);
        if (special_type == -1)
        {
          type = HILDON_FILE_SYSTEM_MODEL_MMC;
          model_node->mmc2 = TRUE;
        }
        else if (special_type != HILDON_FILE_SYSTEM_MODEL_UNKNOWN)
          type = special_type;
    }

    _hildon_file_system_cancel_banner();

    /* The following should be replaced by appending the functionality into
       GtkFileInfo, but this requires API changes into gtkfilesystem.h,
       gtkfilesystemunix.h and gtkfilesystemgnomevfs.h (as well as into 
       memory backend). Currently we can handle only local files, but 
       that's better than nothing... */
    if (type <= HILDON_FILE_SYSTEM_MODEL_FOLDER)
    {
      gchar *local_path = gtk_file_system_path_to_filename(priv->filesystem, path);

      if (local_path)
      {
        if (access(local_path, R_OK) != 0)
        {
          GFileError code = g_file_error_from_errno(errno);
          if (code == G_FILE_ERROR_ACCES)
            g_set_error(&model_node->error, G_FILE_ERROR, code, local_path);
        }

        g_free(local_path);
      }      
    }

    model_node->type = type;

    /* We need to report first that new like has been inserted */

    iter.stamp = priv->stamp;
    iter.user_data = node;
    tree_path = hildon_file_system_model_get_path(model, &iter);
    gtk_tree_model_row_inserted(model, tree_path, &iter);
    gtk_tree_path_free(tree_path);

    if (g_node_n_children(parent_node) <= 1 &&
        parent_node != priv->roots) {
        /* Don't emit signals for fake root */
        hildon_file_system_model_send_has_child_toggled(model,
                                                        parent_node);
    }

    return node;
}

static void
clear_model_node_caches(HildonFileSystemModelNode *model_node)
{
  if (model_node->icon_cache)
  {
    g_object_unref(model_node->icon_cache);
    model_node->icon_cache = NULL;
  }
  if (model_node->icon_cache_expanded)
  {
    g_object_unref(model_node->icon_cache_expanded);
    model_node->icon_cache_expanded = NULL;
  }
  if (model_node->icon_cache_collapsed)
  {
    g_object_unref(model_node->icon_cache_collapsed);
    model_node->icon_cache_collapsed = NULL;
  }
  if (model_node->thumbnail_cache)
  {
    g_object_unref(model_node->thumbnail_cache);
    model_node->thumbnail_cache = NULL;
  }
  if (model_node->thumbnail_handle)
  {
    osso_thumbnail_factory_cancel(model_node->thumbnail_handle);
    model_node->thumbnail_handle = NULL;
  }

  g_free(model_node->title_cache);
  g_free(model_node->name_cache);
  g_free(model_node->key_cache);
  model_node->title_cache = NULL;
  model_node->key_cache = NULL;
  model_node->name_cache = NULL;

  if(model_node->thumb_title)
  {
    g_free(model_node->thumb_title);
    model_node->thumb_title = NULL;
  }
  if(model_node->thumb_author)
  {
    g_free(model_node->thumb_author);
    model_node->thumb_author = NULL;
  }
}

static void hildon_file_system_model_remove_node_list(GtkTreeModel * model,
                                                      GNode * parent_node,
                                                      GSList * children)
{
    GNode *child_node = g_node_first_child(parent_node);

    while (child_node) {
        HildonFileSystemModelNode *model_node = child_node->data;
        if (g_slist_find_custom
            (children, model_node->path, path_compare_helper))
            child_node = hildon_file_system_model_kick_node(child_node, model);
        else
            child_node = g_node_next_sibling(child_node);
    }
}

static void hildon_file_system_model_change_node_list(GtkTreeModel * model,
                                                      GNode * parent_node,
                                                      GtkFileFolder *
                                                      folder,
                                                      GSList * children)
{
    GNode *node;

    g_return_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(model));
    g_return_if_fail(parent_node != NULL);
    g_return_if_fail(GTK_IS_FILE_FOLDER(folder));
    g_return_if_fail(children != NULL);

    for (node = g_node_first_child(parent_node); node;
         node = g_node_next_sibling(node)) {
        HildonFileSystemModelNode *model_node = node->data;

        if (g_slist_find_custom
            (children, model_node->path, path_compare_helper)) {

            ULOG_INFO("Path changed [%s]", (char *) model_node->path);

            /* Ok, current node is updated. We need to refresh it and send
               needed signals. Visible information of special nodes is not going to change */

            clear_model_node_caches(model_node);

            if (model_node->info && model_node->type <= HILDON_FILE_SYSTEM_MODEL_FOLDER)
            {
              GError *error = NULL;

              gtk_file_info_free(model_node->info);

              model_node->info =
                gtk_file_folder_get_info(folder, model_node->path, &error);

              if (error)
              {
                g_assert(model_node->info == NULL);
                ULOG_ERR(error->message);
                g_error_free(error);
              }
            }

            emit_node_changed(node);
        }
    }
}

static void wait_node_load(HildonFileSystemModelPrivate * priv,
                           GNode * node)
{
  HildonFileSystemModelNode *model_node = node->data;

  if (model_node->folder)   /* Sanity check: node has to be a folder */
  {
    ULOG_INFO("Waiting folder [%s] to load", (char *) model_node->path);
    while (!is_node_loaded(priv, node))
    {
        g_usleep(2000); /* microseconds */
        if (gtk_events_pending())       /* Don't do while here, because
                                           there can be enormous amount of 
                                           events waiting */
            gtk_main_iteration();
    }
    ULOG_INFO("Folder [%s] loaded", (char *) model_node->path);
  }
}

static void hildon_file_system_model_init(HildonFileSystemModel * self)
{
    HildonFileSystemModelPrivate *priv;

    priv =
        G_TYPE_INSTANCE_GET_PRIVATE(self, HILDON_TYPE_FILE_SYSTEM_MODEL,
                                    HildonFileSystemModelPrivate);
    self->priv = (gpointer) priv;

    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_GTK_PATH] =
        GTK_TYPE_FILE_PATH;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_LOCAL_PATH] =
        G_TYPE_STRING;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_URI] =
        G_TYPE_STRING;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_NAME] =
        G_TYPE_STRING;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_DISPLAY_NAME] =
        G_TYPE_STRING;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_SORT_KEY] =
        G_TYPE_STRING;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_MIME_TYPE] =
        G_TYPE_STRING;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_SIZE] =
        G_TYPE_INT64;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_TIME] =
        G_TYPE_INT64;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_FOLDER] =
        G_TYPE_BOOLEAN;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_AVAILABLE] =
        G_TYPE_BOOLEAN;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_HAS_LOCAL_PATH] =
        G_TYPE_BOOLEAN;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_TYPE] = G_TYPE_INT;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON] =
        GDK_TYPE_PIXBUF;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON_EXPANDED] =
        GDK_TYPE_PIXBUF;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_ICON_COLLAPSED] =
        GDK_TYPE_PIXBUF;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_THUMBNAIL] =
        GDK_TYPE_PIXBUF;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_LOAD_READY] =
        G_TYPE_BOOLEAN;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_FREE_SPACE] =
        G_TYPE_INT64;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_TITLE] =
        G_TYPE_STRING;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_AUTHOR] =
        G_TYPE_STRING;
    priv->column_types[HILDON_FILE_SYSTEM_MODEL_COLUMN_IS_HIDDEN] =
        G_TYPE_BOOLEAN;

    g_assert(HILDON_FILE_SYSTEM_MODEL_NUM_COLUMNS == 22);

    priv->stamp = g_random_int();
    priv->icon_theme = gtk_icon_theme_get_default();
    priv->delayed_lists = g_queue_new();
    priv->reload_list = g_queue_new();
    priv->cache_queue = g_queue_new();
}

static void hildon_file_system_model_dispose(GObject *self)
{
  HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(self);

  ULOG_DEBUG(__FUNCTION__);

  if (priv->ref_widget)
  {
    g_object_unref(priv->ref_widget);
    priv->ref_widget = NULL;
  }
  if (priv->timeout_id)
  {
    g_source_remove(priv->timeout_id);
    priv->timeout_id = 0;
  }
  /* This won't work in finalize (removing nodes sends signals) */
  if (priv->roots)
  {
    hildon_file_system_model_kick_node(priv->roots, self);
    priv->roots = NULL;
  }

  G_OBJECT_CLASS(hildon_file_system_model_parent_class)->dispose(self);
}

static void hildon_file_system_model_finalize(GObject * self)
{
    HildonFileSystemSettings *fs_settings;
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(self);
    gint i;

    ULOG_DEBUG(__FUNCTION__);
    g_free(priv->backend_name); /* No need to check NULL */
    g_free(priv->alternative_root_dir);

    g_queue_foreach(priv->delayed_lists, (GFunc) delayed_list_free, NULL);
    g_queue_free(priv->delayed_lists);
    g_queue_free(priv->reload_list); 
    g_queue_free(priv->cache_queue);
    /* Contents of this queue are gone already */

    fs_settings = _hildon_file_system_settings_get_instance();
    
    /* Disconnecting signals connected to the fs_settings
     * notify::flight-mode, notify::btname, notify::gateway */
    for (i=0; i<3; i++)
      if (g_signal_handler_is_connected (fs_settings,
                                         priv->signal_handler_id[i]))
        g_signal_handler_disconnect (fs_settings,
                                     priv->signal_handler_id[i]);

    /* Disconnecting filesystem volumes-changed signal */
    if (g_signal_handler_is_connected (priv->filesystem,
                                       priv->signal_handler_id[3]));
      g_signal_handler_disconnect (priv->filesystem,
                                   priv->signal_handler_id[3]);

    ULOG_INFO("ref count = %d", G_OBJECT(priv->filesystem)->ref_count);
    g_object_unref(priv->filesystem);

    if (priv->expanded_emblem)
      g_object_unref(priv->expanded_emblem);
    if (priv->collapsed_emblem)
      g_object_unref(priv->collapsed_emblem);

    G_OBJECT_CLASS(hildon_file_system_model_parent_class)->finalize(self);
}

static void hildon_file_system_model_set_property(GObject * object,
                                                  guint property_id,
                                                  const GValue * value,
                                                  GParamSpec * pspec)
{
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(object);

    switch (property_id) {
    case PROP_BACKEND:
        g_assert(priv->backend_name == NULL);   /* We come here exactly
                                                   once */
        priv->backend_name = g_value_dup_string(value);
        break;
    case PROP_BACKEND_OBJECT:
        g_assert(priv->filesystem == NULL);
        priv->filesystem = g_value_get_object(value);
        if (priv->filesystem)
          g_object_ref(priv->filesystem);
        break;
    case PROP_THUMBNAIL_CALLBACK:
        ULOG_WARN("Setting thumbnail callback is depricated");
        break;
    case PROP_REF_WIDGET:
        if (priv->ref_widget)
            g_object_unref(priv->ref_widget);
        priv->ref_widget = g_value_get_object(value);
        if (priv->ref_widget)
            g_object_ref(priv->ref_widget);
        break;
    case PROP_ROOT_DIR:
        g_assert(priv->alternative_root_dir == NULL);
        priv->alternative_root_dir = g_value_dup_string(value);
        break;
    case PROP_MULTI_ROOT:
        priv->multiroot = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void hildon_file_system_model_get_property(GObject * object,
                                                  guint property_id,
                                                  GValue * value,
                                                  GParamSpec * pspec)
{
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(object);

    switch (property_id) {
    case PROP_BACKEND:
        g_value_set_string(value, priv->backend_name);
        break;
    case PROP_BACKEND_OBJECT:
        g_value_set_object(value, priv->filesystem);
        break;
    case PROP_THUMBNAIL_CALLBACK:
        ULOG_WARN("Getting thumbnail callback is depricated");
        g_value_set_pointer(value, NULL);
        break;
    case PROP_REF_WIDGET:
        g_value_set_object(value, priv->ref_widget);
        break;
    case PROP_ROOT_DIR:
        g_value_set_string(value, priv->alternative_root_dir);
        break;
    case PROP_MULTI_ROOT:
        g_value_set_boolean(value, priv->multiroot);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void hildon_file_system_model_class_init(HildonFileSystemModelClass
                                                * klass)
{
    GObjectClass *object;

    g_type_class_add_private(klass, sizeof(HildonFileSystemModelPrivate));

    object = G_OBJECT_CLASS(klass);
    object->constructor = hildon_file_system_model_constructor;
    object->dispose = hildon_file_system_model_dispose;
    object->finalize = hildon_file_system_model_finalize;
    object->set_property = hildon_file_system_model_set_property;
    object->get_property = hildon_file_system_model_get_property;
    klass->device_disconnected = 
      hildon_file_system_model_real_device_disconnected;

    g_object_class_install_property(object, PROP_BACKEND,
        g_param_spec_string("backend",
                            "HildonFileChooser backend",
                            "Set GtkFileSystem backend to use",
                            NULL,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

    g_object_class_install_property(object, PROP_BACKEND_OBJECT,
        g_param_spec_object("backend-object",
                            "Backend object",
                            "GtkFileSystem backend to use. Use this"
                            "if you create backend yourself",
                            GTK_TYPE_FILE_SYSTEM,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object, PROP_THUMBNAIL_CALLBACK,
        g_param_spec_pointer("thumbnail-callback",
                             "Thumbnail creation callback",
                             "This callback property is depricated",
                             G_PARAM_READWRITE));

    g_object_class_install_property(object, PROP_REF_WIDGET,
        g_param_spec_object("ref-widget",
                            "Refrence widget",
                            "Any widget on the screen. Needed if "
                            "you want icons.",
                            GTK_TYPE_WIDGET,
                            G_PARAM_READWRITE));

    g_object_class_install_property(object, PROP_ROOT_DIR,
        g_param_spec_string("root-dir",
                            "Root directory",
                            "Specify an alternative root directory. Note that"
                            "gateway and MMCs appear ONLY if you leave"
                            "this to default setting.", NULL,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object, PROP_MULTI_ROOT,
        g_param_spec_boolean("multi-root",
                            "Multiple root directories",
                            "When multiple root directories is enabled, "
                            "each folder under root-dir "
                            "(property) appear as a separate root level folder. "
                            "The directory spesified by root-dir property is not "
                            "displayed itself. This property has effect only when "
                            "root-dir is set.", FALSE,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
                     
    hildon_file_system_model_quark =
        g_quark_from_static_string("HildonFileSystemModel Quark");

    signal_finished_loading =
        g_signal_new("finished-loading", HILDON_TYPE_FILE_SYSTEM_MODEL,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HildonFileSystemModelClass,
                                     finished_loading), NULL, NULL,
                     g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 1, GTK_TYPE_TREE_ITER);

    signal_device_disconnected =
        g_signal_new("device-disconnected", HILDON_TYPE_FILE_SYSTEM_MODEL,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HildonFileSystemModelClass,
                                     device_disconnected), NULL, NULL,
                     g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 1, 
                     GTK_TYPE_TREE_ITER);
}

/* We currently assume that all selectable rows can be dragged */
static gboolean
hildon_file_system_model_row_draggable(GtkTreeDragSource *source, GtkTreePath *path)
{
  ULOG_DEBUG_F("entered");
  return TRUE;
}

static void hildon_file_system_model_iface_init(GtkTreeModelIface * iface)
{
    iface->get_flags = hildon_file_system_model_get_flags;
    iface->get_n_columns = hildon_file_system_model_get_n_columns;
    iface->get_column_type = hildon_file_system_model_get_column_type;
    iface->get_iter = hildon_file_system_model_get_iter;
    iface->get_path = hildon_file_system_model_get_path;
    iface->get_value = hildon_file_system_model_get_value;
    iface->iter_next = hildon_file_system_model_iter_next;
    iface->iter_children = hildon_file_system_model_iter_children;
    iface->iter_has_child = hildon_file_system_model_iter_has_child;
    iface->iter_n_children = hildon_file_system_model_iter_n_children;
    iface->iter_nth_child = hildon_file_system_model_iter_nth_child;
    iface->iter_parent = hildon_file_system_model_iter_parent;
}

/* All bookkeeping related to DnD is in HildonFileSelection, since
   GtkTreeDnD does not support dragging of multiple items. We only
   use the interface because we want GtkTreeView to limit drag start
   points to real rows (not empty space). */
static void
hildon_file_system_model_drag_source_iface_init(GtkTreeDragSourceIface *iface)
{
  iface->row_draggable = hildon_file_system_model_row_draggable;
}

static void
gateway_changed(GObject *settings, GParamSpec *param, gpointer data)
{
  gchar *bluetooth_address;
  HildonFileSystemModelPrivate *priv = HILDON_FILE_SYSTEM_MODEL(data)->priv;
  	 
  /* If we had old contents => kick them off. This is OK for both
     pairing and unpairing */
  if (priv->gateway_node)
  {
    hildon_file_system_model_kick_node(priv->gateway_node, data);
    priv->gateway_node = NULL;
  }
  if (priv->gateway_path)
  {
    gtk_file_path_free(priv->gateway_path);
    priv->gateway_path = NULL;
  }
  	 
  g_object_get(settings, "gateway", &bluetooth_address, NULL);

  if (bluetooth_address && bluetooth_address[0])
  {
    gchar buffer[256];

    g_snprintf(buffer, sizeof(buffer), "obex://[%s]/", bluetooth_address);
    priv->gateway_path =
        gtk_file_system_uri_to_path(priv->filesystem, buffer);

    /* Conversion can fail at least with random data */
    if (priv->gateway_path)
    {	 
      ULOG_INFO("Gateway \"%s\" paired", buffer);
      priv->gateway_node = hildon_file_system_model_add_node(
                             GTK_TREE_MODEL(data), priv->roots, NULL,
                             priv->gateway_path,
                             HILDON_FILE_SYSTEM_MODEL_GATEWAY);
      g_assert(priv->gateway_node != NULL);
    }
    else
    {
      ULOG_ERR("Tried \"%s\", but failed", buffer);
    }
  }

  g_free(bluetooth_address);
}

static void
btname_changed(GObject *settings, GParamSpec *param, gpointer data)
{
  HildonFileSystemModelNode *model_node;
  GNode *node;
  gchar *name;

  g_object_get(settings, "btname", &name, NULL);

  ULOG_INFO("BT name changed into \"%s\"", name);

  node = HILDON_FILE_SYSTEM_MODEL(data)->priv->local_device_node;
  model_node = node->data;
  g_assert(model_node != NULL);
  
  clear_model_node_caches(model_node);
  
  g_assert(model_node->name_cache == NULL);
  g_assert(model_node->title_cache == NULL);
  model_node->name_cache = name; 	 
  model_node->title_cache = g_strdup(name);

  emit_node_changed(node);
}

static void 
flightmode_changed(GObject *settings, GParamSpec *param, gpointer data)
{
  HildonFileSystemModel *self;
  GNode *node;
  gboolean mode;

  self = HILDON_FILE_SYSTEM_MODEL(data);
  g_object_get(settings, "flight-mode", &mode, NULL);

  node = self->priv->gateway_node;
  if (!node) return;

  if (mode)
  {
    ULOG_INFO("Changing into flight mode");
    self->priv->gateway_accessed = FALSE;
    send_device_disconnected(node);
  }

  emit_node_changed(node);
}

static GObject *
hildon_file_system_model_constructor(GType type,
                                     guint n_construct_properties,
                                     GObjectConstructParam *
                                     construct_properties)
{
    GObject *obj;
    HildonFileSystemModelPrivate *priv;
    GtkTreeModel *model;
    GtkFilePath *file_path;
    GtkFilePath *local_device_path;

    obj = G_OBJECT_CLASS(hildon_file_system_model_parent_class)->
            constructor(type, n_construct_properties, construct_properties);
    priv = CAST_GET_PRIVATE(obj);
    model = GTK_TREE_MODEL(obj);
    _hildon_file_system_ensure_locations();

    if (!priv->filesystem)
      priv->filesystem = hildon_file_system_create_backend(
                           priv->backend_name, TRUE);

    priv->roots = g_node_new(NULL);     /* This is a fake root that
                                           contains real ones */

    if (priv->alternative_root_dir == NULL)
    {
      HildonFileSystemSettings *fs_settings;

      local_device_path = _hildon_file_system_path_for_location(
                            priv->filesystem, 
                            HILDON_FILE_SYSTEM_MODEL_LOCAL_DEVICE, FALSE);

      priv->local_device_node = hildon_file_system_model_add_node(model,
                                  priv->roots, NULL, local_device_path,
                                  HILDON_FILE_SYSTEM_MODEL_LOCAL_DEVICE);
      gtk_file_path_free(local_device_path);

      g_assert(priv->local_device_node);

      priv->mmc.base_node = hildon_file_system_model_add_node(model,
                              priv->local_device_node, NULL, NULL,
                              HILDON_FILE_SYSTEM_MODEL_MMC);
      g_assert(priv->mmc.base_node);

      if (g_getenv("INTERNAL_MMC_MOUNTPOINT") != NULL)
      {
        priv->mmc2.base_node = hildon_file_system_model_add_node(model,
                                 priv->local_device_node, NULL, NULL,
                                 HILDON_FILE_SYSTEM_MODEL_MMC);
      }
      else
      {
        priv->mmc2.base_node = NULL;
      }

      real_volumes_changed(obj);

      fs_settings = _hildon_file_system_settings_get_instance();

      priv->signal_handler_id[0] = g_signal_connect_object(fs_settings,
                                     "notify::flight-mode",
                                     G_CALLBACK(flightmode_changed), obj, 0);
      priv->signal_handler_id[1] = g_signal_connect_object(fs_settings, 
                                     "notify::btname",
                                     G_CALLBACK(btname_changed), obj, 0);
      priv->signal_handler_id[2] = g_signal_connect_object(fs_settings,
                                     "notify::gateway",
                                     G_CALLBACK(gateway_changed), obj, 0);

      gateway_changed((GObject *) fs_settings, NULL, obj);
      flightmode_changed((GObject *) fs_settings, NULL, obj);

      priv->signal_handler_id[3] = g_signal_connect_object(priv->filesystem,
                                     "volumes-changed",
                                     G_CALLBACK(real_volumes_changed),
                                     obj, G_CONNECT_SWAPPED);
    }
    else
    {
      ULOG_INFO_F("Alternative root = '%s'", priv->alternative_root_dir);

      file_path = gtk_file_system_filename_to_path(priv->filesystem,
                    priv->alternative_root_dir);

      if (priv->multiroot)
      {
        HildonFileSystemModelNode *model_node;
        GError *error = NULL;

        model_node = g_new0(HildonFileSystemModelNode, 1);
        model_node->type = HILDON_FILE_SYSTEM_MODEL_FOLDER;
        model_node->path = gtk_file_path_copy(file_path);    
        model_node->available = TRUE;
        priv->roots->data = model_node;
        model_node->present_flag = TRUE;
	model_node->model = HILDON_FILE_SYSTEM_MODEL(obj);

        link_file_folder(priv->roots, file_path, &error);

        if (error)
        {
          ULOG_ERR_F(error->message);
          g_error_free(error);
        }
        else
        {
          hildon_file_system_model_delayed_add_children(
            HILDON_FILE_SYSTEM_MODEL(obj), priv->roots, TRUE);
          wait_node_load(priv, priv->roots);          
        }
      }
      else
      {
        hildon_file_system_model_add_node(model, priv->roots, NULL,
                                          file_path,
                                          HILDON_FILE_SYSTEM_MODEL_FOLDER);
      }

      gtk_file_path_free(file_path); 
    }

    return obj;
}

/**
 * hildon_file_system_model_search_local_path:
 * @model: a #HildonFileSystemModel.
 * @path: a #GtkFilePath to load.
 * @iter: a #GtkTreeIter for the result.
 * @start_iter: a #GtkTreeIter for starting point. %NULL for entire model.
 * @recursive: if %FALSE, only immediate children of the parent are
 *   searched. %TRUE searches the entire subtree.
 * 
 * a wrapped for hildon_file_system_model_search_path that accepts local
 *   paths.
 *
 * Returns: %TRUE, if the iterator points to desired file.
 *          %FALSE otherwise.
 */
gboolean hildon_file_system_model_search_local_path(HildonFileSystemModel *
                                                    model,
                                                    const gchar * path,
                                                    GtkTreeIter * iter,
                                                    GtkTreeIter *
                                                    start_iter,
                                                    gboolean recursive)
{
    gboolean result;
    GtkFilePath *filepath;
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(model);

    filepath = gtk_file_system_filename_to_path(priv->filesystem, path);
    result =
        hildon_file_system_model_search_path(model, filepath, iter,
                                             start_iter, recursive);

    gtk_file_path_free(filepath);
    return result;
}

/**
 * hildon_file_system_model_search_uri:
 * @model: a #HildonFileSystemModel.
 * @path: a #GtkFilePath to load.
 * @iter: a #GtkTreeIter for the result.
 * @start_iter: a #GtkTreeIter for starting point. %NULL for entire model.
 * @recursive: if %FALSE, only immediate children of the parent are
 *   searched. %TRUE searches the entire subtree.
 * 
 * a wrapped for hildon_file_system_model_search_path that accepts URIs.
 *
 * Returns: %TRUE, if the iterator points to desired file.
 *          %FALSE otherwise.
 */
gboolean hildon_file_system_model_search_uri(HildonFileSystemModel * model,
                                             const gchar * uri,
                                             GtkTreeIter * iter,
                                             GtkTreeIter * start_iter,
                                             gboolean recursive)
{
    gboolean result;
    GtkFilePath *filepath;
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(model);

    filepath = gtk_file_system_uri_to_path(priv->filesystem, uri);
    result =
        hildon_file_system_model_search_path(model, filepath, iter,
                                             start_iter, recursive);

    gtk_file_path_free(filepath);
    return result;
}

/**
 * hildon_file_system_model_search_path:
 * @model: a #HildonFileSystemModel.
 * @path: a #GtkFilePath to load.
 * @iter: a #GtkTreeIter for the result.
 * @start_iter: a #GtkTreeIter for starting point. %NULL for entire model.
 * @recursive: if %FALSE, only immediate children of the parent are
 *   searched. %TRUE searches the entire subtree.
 * 
 * Searches the model for given path and fills an iterator pointing to it.
 * Note that the path must already exist in model.
 *
 * Returns: %TRUE, if the iterator points to desired file.
 *          %FALSE otherwise.
 */
gboolean hildon_file_system_model_search_path(HildonFileSystemModel *
                                              model,
                                              const GtkFilePath * path,
                                              GtkTreeIter * iter,
                                              GtkTreeIter * start_iter,
                                              gboolean recursive)
{
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(model);

    g_return_val_if_fail(iter != NULL, FALSE);

    iter->stamp = priv->stamp;





    iter->user_data =
        hildon_file_system_model_search_path_internal(priv,
                                                      get_node(priv,
                                                               start_iter),
                                                      path, recursive);
    return iter->user_data != NULL;
}

/**
 * hildon_file_system_model_load_local_path:
 * @model: a #HildonFileSystemModel.
 * @path: a path to load.
 * @iter: a #GtkTreeIter for the result.
 *
 * Converts the given path to #GtkFilePath and calls
 * hildon_file_system_model_load_path.
 *
 * Returns: %TRUE, if the iterator points to desired file.
 *          %FALSE otherwise.
 */
gboolean hildon_file_system_model_load_local_path(HildonFileSystemModel * model,
                                            const gchar * path,
                                            GtkTreeIter * iter)
{
    gboolean result;
    GtkFilePath *filepath;
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(model);

    filepath = gtk_file_system_filename_to_path(priv->filesystem, path);
    if (filepath == NULL)
      return FALSE;

    result = hildon_file_system_model_load_path(model, filepath, iter);

    gtk_file_path_free(filepath);
    return result;
}

/**
 * hildon_file_system_model_load_uri:
 * @model: a #HildonFileSystemModel.
 * @path: an URI to load.
 * @iter: a #GtkTreeIter for the result.
 *
 * Converts the given URI to #GtkFilePath and calls
 * hildon_file_system_model_load_path.
 *
 * Returns: %TRUE, if the iterator points to desired file.
 *          %FALSE otherwise.
 */
gboolean hildon_file_system_model_load_uri(HildonFileSystemModel * model,
                                            const gchar * uri,
                                            GtkTreeIter * iter)
{
    HildonFileSystemSettings *settings;
    gboolean result;
    GtkFilePath *filepath;
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(model);

    filepath = gtk_file_system_uri_to_path(priv->filesystem, uri);
    if (filepath == NULL)
      return FALSE;

    if (!gtk_file_system_path_is_local(priv->filesystem, filepath))
    {
      /* If we're accessing a gateway, its root node doesn't exist until
         settings are read. Wait here until they are */
      settings = _hildon_file_system_settings_get_instance();
      while (!_hildon_file_system_settings_ready(settings))
        gtk_main_iteration();
    }

    result = hildon_file_system_model_load_path(model, filepath, iter);

    gtk_file_path_free(filepath);
    return result;
}

/**
 * hildon_file_system_model_load_path:
 * @model: a #HildonFileSystemModel.
 * @path: a #GtkFilePath to load.
 * @iter: a #GtkTreeIter for the result.
 *
 * This method locates the given path from data model. New branches are
 * loaded if the given path doesn't exist in memory. Otherwise similar to
 * hildon_file_system_model_search_path.
 *
 * Returns: %TRUE, if the iterator points to desired file.
 *          %FALSE otherwise.
 */
gboolean hildon_file_system_model_load_path(HildonFileSystemModel * model,
                                            const GtkFilePath * path,
                                            GtkTreeIter * iter)
{
    GtkFilePath *parent_path;
    GtkTreeIter parent_iter;

    g_return_val_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(model), FALSE);
    g_return_val_if_fail(path != NULL, FALSE);
    g_return_val_if_fail(iter != NULL, FALSE);

    /* Let's see if given path is already in the tree */
    if (hildon_file_system_model_search_path(model, path, iter, NULL, TRUE))
    {
      /* In case of gateway, we may need this to allow accessing contents */
      (void) _hildon_file_system_model_mount_device_iter(model, iter, NULL);
      return TRUE;
    }

    /* No, path was not found. Let's try go one level up and loading more
       contents */
    if (!gtk_file_system_get_parent
        (model->priv->filesystem, path, &parent_path, NULL) || parent_path == NULL) {
        ULOG_ERR_F("Attempt to select folder that is not in user visible area");
        return FALSE; /* Very BAD. We reached the real root. Given
                         folder was probably not under any of our roots */
    }

    if (hildon_file_system_model_load_path(model, parent_path, &parent_iter))
    {
      GNode *parent_node;
      GtkFileFolder *parent_folder;
      HildonFileSystemModelNode *parent_model_node;

      parent_node = parent_iter.user_data;
      g_assert(parent_node != NULL);
      parent_model_node = parent_node->data;
      g_assert(parent_model_node != NULL);

      gtk_file_path_free(parent_path);    
      parent_folder = parent_model_node->folder;

      if (!parent_folder)
        return FALSE;

      /* Ok, if we reached this point we had located "parent_folder". We
         have to add our path to this folder. This is a blocking function,
         but MUCH FASTER than the previous approach that loaded whole levels
         for each folder. */
      iter->user_data = hildon_file_system_model_add_node(GTK_TREE_MODEL(model),
                                  parent_node, parent_folder,
                                  path, HILDON_FILE_SYSTEM_MODEL_UNKNOWN);
      iter->stamp = model->priv->stamp;
    
      return iter->user_data != NULL;
    }

    *iter = parent_iter;   /* Return parent iterator if we cannot
                              found asked path */
    gtk_file_path_free(parent_path);
    return FALSE;
}

void _hildon_file_system_model_queue_reload(HildonFileSystemModel *model,
  GtkTreeIter *parent_iter, gboolean force)
{
  g_return_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(model));
  g_return_if_fail(parent_iter != NULL);
  g_return_if_fail(parent_iter->stamp == model->priv->stamp);
  
  if (g_queue_find(model->priv->reload_list, parent_iter->user_data) == NULL)
  {
    hildon_file_system_model_ensure_idle(model);
    g_queue_push_tail(model->priv->reload_list, parent_iter->user_data);
  }
}

void _hildon_file_system_model_load_children(HildonFileSystemModel *model,
  GtkTreeIter *parent_iter)
{
  g_return_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(model));
  g_return_if_fail(parent_iter != NULL);
  g_return_if_fail(parent_iter->stamp == model->priv->stamp);

  hildon_file_system_model_delayed_add_children(model,
                                                  parent_iter->user_data, FALSE);
  wait_node_load(model->priv, parent_iter->user_data);  
}

GtkFileSystem
    *_hildon_file_system_model_get_file_system(HildonFileSystemModel *
                                               model)
{
    HildonFileSystemModelPrivate *priv = CAST_GET_PRIVATE(model);

    return priv->filesystem;
}

static gint
compare_numbers(gconstpointer a, gconstpointer b)
{
  return GPOINTER_TO_INT(a) - GPOINTER_TO_INT(b);
}

/**
 * hildon_file_system_model_new_item:
 * @model: a #HildonFileSystemModel.
 * @parent: a parent iterator.
 * @stub_name: a boby of the new name.
 * @extension: extension of the new name.
 *
 * Creates a new unique name under #parent. The returned name can be used
 * when creating a new file. If there are no name collisions, stub name
 * will be the final name. If a file with that name already exists, then
 * a number is appended to stub. This function is mainly used by dialog
 * implementations. It's probably not needed in application development.
 *
 * Returns: a New unique name. You have to release this with #g_free. Can be %NULL, if
 * the directory is not yet loaded.
 */
gchar *hildon_file_system_model_new_item(HildonFileSystemModel * model,
                                         GtkTreeIter * parent,
                                         const gchar * stub_name,
                                         const gchar * extension)
{
    GtkTreeModel *treemodel;
    GtkTreeIter iter;
    GList *reserved = NULL;
    GList *list_iter;
    gint final, parent_type;
    gboolean has_next, full_match = FALSE;
    gchar *result, *allocated = NULL;

    g_return_val_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(model), NULL);
    g_return_val_if_fail(stub_name != NULL, NULL);

    treemodel = GTK_TREE_MODEL(model);

    if (!is_node_loaded(model->priv, parent->user_data))
      return NULL;

    gtk_tree_model_get(treemodel, parent, 
      HILDON_FILE_SYSTEM_MODEL_COLUMN_TYPE, &parent_type, 
      -1);

    /* "Root" and "Documents" have translated special 
        locations within them that can cause conflicts 
        with autonaming. */
    if ((parent_type == HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_DOCUMENTS ||
         parent_type == HILDON_FILE_SYSTEM_MODEL_LOCAL_DEVICE) && 
        !extension)
    {
      GtkFileSystem *fs;
      GtkFilePath *path;
      HildonFileSystemModelNode *model_node;

      model_node = ((GNode *) parent->user_data)->data;
      fs = model->priv->filesystem;
      path = gtk_file_system_make_path(fs, model_node->path, stub_name, NULL);

      if (path)
      {
        gint type = _hildon_file_system_get_special_location(fs, path);

        if (type) /* Ok, we are trying to autoname a special folder. Let's use user visible name */
        {
          allocated = _hildon_file_system_create_file_name(fs, path,
                          type == -1 ? HILDON_FILE_SYSTEM_MODEL_MMC : type,
                          NULL, type == -1 ? TRUE : FALSE);
          stub_name = allocated;
        } 

        gtk_file_path_free(path);
      }
    }

    for (has_next = gtk_tree_model_iter_children(treemodel, &iter, parent);
         has_next; has_next = gtk_tree_model_iter_next(treemodel, &iter)) {
        gchar *filename;

        gtk_tree_model_get(treemodel, &iter,
                           HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_NAME,
                           &filename, -1);

        if (g_str_has_prefix(filename, stub_name)
            && (extension == NULL
                || g_str_has_suffix(filename, extension))) {
            /* Ok, we have a possible candidate. If part after the stub
               prior to extension contains just numbers we have to record
               this information. if this part contains characters then we
               are not concerned about this item */

            char *start = filename + strlen(stub_name);

            if (extension)      /* remove extension */
                filename[strlen(filename) - strlen(extension)] = '\0';

            if (*start == 0)
              full_match = TRUE;
            else
            {
              long value = _hildon_file_system_parse_autonumber(start);

  	          if (value >= 0)  /* the string is reserved */ 
                reserved = g_list_insert_sorted(reserved, 
                  GINT_TO_POINTER(value), compare_numbers);
            }
        }

        g_free(filename);
    }

    final = 1;

    for (list_iter = reserved; list_iter; list_iter = list_iter->next)
    {
      gint value = GPOINTER_TO_INT(list_iter->data);
      if (final < value) break;
      final = value + 1;
    }

    g_list_free(reserved);

    if (!full_match)  /* No matches found. Candidate is a good name */
      result = g_strdup(stub_name);
    else
      result = g_strdup_printf("%s (%d)", stub_name, final);

    /* stub_name may point to this, so we have to free 
       this AFTER creating the result */
    g_free(allocated); 

    return result;
}

/* Devices are not mounted automatically, only in response to user action. 
    Additionally, we never try to mount MMC ourselves. So there is mow even 
    less to do here. */
gboolean _hildon_file_system_model_mount_device_iter(HildonFileSystemModel
                                                     * model,
                                                     GtkTreeIter * iter,
                                                     GError ** error)
{
    HildonFileSystemModelNode *model_node;
    HildonFileSystemModelPrivate *priv;
    static gboolean active_flag = FALSE;

    g_return_val_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(model), FALSE);
    g_return_val_if_fail(iter != NULL, FALSE);
    g_return_val_if_fail(model->priv->stamp == iter->stamp, FALSE);

    priv = model->priv;
    model_node = ((GNode *) iter->user_data)->data;

    if (model_node->type == HILDON_FILE_SYSTEM_MODEL_GATEWAY && !active_flag)
    {
      HildonFileSystemSettings *settings;

      settings = _hildon_file_system_settings_get_instance();
      active_flag = TRUE;

      /* We really have to know gateway state until we can continue.
         Entering main loop can be dangerous (some functions
         experience re-entrancy issues). Anyway, this is issue only
         if this function is called almost immediately after creating
         the first model in a process. Then settings are not yet ready.
      */
      while (!_hildon_file_system_settings_ready(settings))
        gtk_main_iteration();

      active_flag = FALSE;

      if (!_hildon_file_system_settings_get_flight_mode())
      {
        gboolean success;

        _hildon_file_system_prepare_banner();
        success = link_file_folder(priv->gateway_node, priv->gateway_path, error);
        _hildon_file_system_cancel_banner();

        if (!success)
            return FALSE;

        priv->gateway_accessed = TRUE;

        hildon_file_system_model_delayed_add_children(model, 
            priv->gateway_node, TRUE);
        return TRUE;
      }
    }

    return FALSE;
}

/**
 * hildon_file_system_model_finished_loading:
 * @model: a #HildonFileSystemModel.
 *
 * Checks if model has data in it's processing queue. 
 * Note! This api is broken an DEPRICATED. It only checks
 * internal processing queue and this information is mostly
 * useless.
 *
 * Returns: %TRUE, data queues are empty.
 */
#ifndef HILDON_DISABLE_DEPRECATED
gboolean hildon_file_system_model_finished_loading(HildonFileSystemModel *
                                                   model)
{
    g_return_val_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(model), FALSE);

    return g_queue_is_empty(model->priv->delayed_lists);
}
#endif
/**
 * hildon_file_system_model_autoname_uri:
 * @model: a #HildonFileSystemModel.
 * @uri: an URI to be autonamed.
 * @error: a GError to hold possible error information.
 * 
 * This function checks if the given URI already exists in the model.
 * if not, then a copy of it is returned unmodified. If the URI already
 * exists then a number is added in a form file://file(2).html.
 *
 * Returns: either the same uri given as parameter or a modified
 * uri that contains proper index number. In both cases free
 * this value using #g_free. Value can be %NULL, if error was
 * encountered.
 */
gchar *hildon_file_system_model_autoname_uri(HildonFileSystemModel *model, 
  const gchar *uri, GError **error)
{
  GtkFileSystem *backend;
  GtkFilePath *folder;
  GtkFilePath * uri_path = NULL;
  gchar *file = NULL;
  gchar *result = NULL;
  GtkTreeIter iter;
  

  g_return_val_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(model), NULL);
  g_return_val_if_fail(uri != NULL, NULL);

  backend = model->priv->filesystem;
 
  
  if ( ((uri_path = gtk_file_system_uri_to_path(backend, uri)) != NULL) &&
        gtk_file_system_get_parent(backend, uri_path, &folder, NULL)    &&
	(folder != NULL)                                                &&
	hildon_file_system_model_load_path(model, folder, &iter) )
  {     
        
      _hildon_file_system_model_load_children(model, &iter);

      GtkTreeIter ret_iter;

      if ( hildon_file_system_model_search_uri(model, uri, &ret_iter, &iter, FALSE) ) 
      {
	 gtk_tree_model_get(GTK_TREE_MODEL(model), &ret_iter,
			    HILDON_FILE_SYSTEM_MODEL_COLUMN_FILE_NAME, &file ,-1); 
      }
  }
   
  gtk_file_path_free(uri_path);
  
  if( file == NULL )
  {     
      gtk_file_path_free(folder);	  
      return g_strdup(uri);
  }
    
  gchar *extension = NULL, *dot, *autonamed;

  dot = _hildon_file_system_search_extension(file, NULL);
  if (dot && dot != file) {
     extension = g_strdup(dot);
     *dot = '\0';
  }

  _hildon_file_system_remove_autonumber(file);

  autonamed = hildon_file_system_model_new_item(model, 
                                &iter, file, extension);
  g_free(file); 
       
  if (autonamed)
  { 
    GtkFilePath *result_path;
      
    if (extension) {
       /* Dot is part of the extension */
        gchar *ext_name = g_strconcat(autonamed, extension, NULL);
        g_free(extension);
        g_free(autonamed);
        autonamed = ext_name;
    }

    result_path = gtk_file_system_make_path(backend, folder, autonamed, error);

    if (result_path) {
       result = gtk_file_system_path_to_uri(backend, result_path);
       gtk_file_path_free(result_path);
    }

    g_free(autonamed);
  }
 
  gtk_file_path_free(folder);
 
  
  return result;
}

/**
 * hildon_file_system_model_iter_available:
 * @model: a #HildonFileSystemModel.
 * @iter: a #GtkTreeIter to location to modify.
 * @available: new availability state.
 *
 * This function sets some paths available/not available. Locations
 * that are not available are usually shown dimmed in the gui. This
 * function can be used if program needs for some reason to disable some
 * locations. By default all paths are available.
 */
void hildon_file_system_model_iter_available(HildonFileSystemModel *model,
  GtkTreeIter *iter, gboolean available)
{
  GNode *node;
  HildonFileSystemModelNode *model_node;

  g_return_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(model));
  g_return_if_fail(iter != NULL);
  g_return_if_fail(model->priv->stamp == iter->stamp);

  node = iter->user_data;
  model_node = node->data;

  if (model_node->available != available)
  {
    model_node->available = available;
    emit_node_changed(node);
  }
}

static gboolean
reset_callback(GNode *node, gpointer data)
{
  HildonFileSystemModelNode *model_node;

  g_assert(node != NULL);
  
  model_node = node->data;
  if (model_node && !model_node->available)
  {
    model_node->available = TRUE;
    emit_node_changed(node);
  }

  return FALSE;
}

/**
 * hildon_file_system_model_reset_available:
 * @model: a #HildonFileSystemModel.
 *
 * Cancels all changes made by #hildon_file_system_model_iter_available.
 * Selection is back to it's default state.
 */
void hildon_file_system_model_reset_available(HildonFileSystemModel *model)
{
  g_return_if_fail(HILDON_IS_FILE_SYSTEM_MODEL(model));

  g_node_traverse(model->priv->roots, G_POST_ORDER, 
      G_TRAVERSE_ALL, -1, reset_callback, NULL);
}


void
_hildon_file_system_model_prioritize_folder(HildonFileSystemModel *model,
                                            GtkTreeIter *folder_iter)
{
  GNode *folder_node = get_node(model->priv, folder_iter);
  guint n, len;

  /* search delayed_lists for the folder and move it to head */
  for (n = 0, len = g_queue_get_length(model->priv->delayed_lists);
       n < len;
       n++) {
    delayed_list_type *list = g_queue_peek_nth(model->priv->delayed_lists, n);

    if (list->parent_node == folder_node) {
      g_queue_push_head(model->priv->delayed_lists,
                        g_queue_pop_nth(model->priv->delayed_lists, n));
      return;
    }
  }
}
