/* gtktreemodelfilter.c
 * Copyright (C) 2000,2001  Red Hat, Inc., Jonathan Blandford <jrb@redhat.com>
 * Copyright (C) 2001-2003  Kristian Rietveld <kris@gtk.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "gtktreemodelfilter.h"
#include "gtkintl.h"
#include "gtktreednd.h"
#include "gtkprivate.h"
#include "gtkalias.h"
#include <string.h>

/* ITER FORMAT:
 *
 * iter->stamp = filter->priv->stamp
 * iter->user_data = FilterLevel
 * iter->user_data2 = FilterElt
 */

/* all paths, iters, etc prefixed with c_ are paths, iters, etc relative to the
 * child model.
 */

/* A few notes:
 *  There are three model/views involved, so there are two mappings:
 *    * this model -> child model: mapped via offset in FilterElt.
 *    * this model -> parent model (or view): mapped via the array index
 *                                            of FilterElt.
 *
 *  Note that there are two kinds of paths relative to the filter model
 *  (those generated from the array indices): paths taking non-visible
 *  nodes into account, and paths which don't.  Paths which take
 *  non-visible nodes into account should only be used internally and
 *  NEVER be passed along with a signal emission.
 *
 *  The filter model has a reference on every node that is not in the root
 *  level and has a parent with ref_count > 1.  Exception is a virtual root
 *  level; all nodes in the virtual root level are referenced too.
 */

typedef struct _FilterElt FilterElt;
typedef struct _FilterLevel FilterLevel;

struct _FilterElt
{
  GtkTreeIter iter;
  FilterLevel *children;
  gint offset;
  gint ref_count;
  gint zero_ref_count;
  GSequenceIter *visible_siter; /* Iter inside the visible_seq */
};

struct _FilterLevel
{
  GSequence *seq;
  GSequence *visible_seq; /* subset of seq containing only visible elt */
  gint ref_count;
  FilterElt *parent_elt;
  FilterLevel *parent_level;
};

#define GTK_TREE_MODEL_FILTER_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_TREE_MODEL_FILTER, GtkTreeModelFilterPrivate))

struct _GtkTreeModelFilterPrivate
{
  gpointer root;
  gint stamp;
  guint child_flags;
  GtkTreeModel *child_model;
  gint zero_ref_count;

  GtkTreePath *virtual_root;

  GtkTreeModelFilterVisibleFunc visible_func;
  gpointer visible_data;
  GDestroyNotify visible_destroy;

  gint modify_n_columns;
  GType *modify_types;
  GtkTreeModelFilterModifyFunc modify_func;
  gpointer modify_data;
  GDestroyNotify modify_destroy;

  gint visible_column;

  gboolean visible_method_set;
  gboolean modify_func_set;

  gboolean in_row_deleted;
  gboolean virtual_root_deleted;

  /* signal ids */
  guint changed_id;
  guint inserted_id;
  guint has_child_toggled_id;
  guint deleted_id;
  guint reordered_id;
};

/* properties */
enum
{
  PROP_0,
  PROP_CHILD_MODEL,
  PROP_VIRTUAL_ROOT
};

#define GTK_TREE_MODEL_FILTER_CACHE_CHILD_ITERS(filter) \
        (((GtkTreeModelFilter *)filter)->priv->child_flags & GTK_TREE_MODEL_ITERS_PERSIST)

#define FILTER_ELT(filter_elt) ((FilterElt *)filter_elt)
#define FILTER_LEVEL(filter_level) ((FilterLevel *)filter_level)
#define GET_ELT(siter) ((FilterElt*) (siter ? g_sequence_get (siter) : NULL))

/* general code (object/interface init, properties, etc) */
static void         gtk_tree_model_filter_tree_model_init                 (GtkTreeModelIface       *iface);
static void         gtk_tree_model_filter_drag_source_init                (GtkTreeDragSourceIface  *iface);
static void         gtk_tree_model_filter_drag_dest_init                  (GtkTreeDragDestIface    *iface);
static void         gtk_tree_model_filter_finalize                        (GObject                 *object);
static void         gtk_tree_model_filter_set_property                    (GObject                 *object,
                                                                           guint                    prop_id,
                                                                           const GValue            *value,
                                                                           GParamSpec              *pspec);
static void         gtk_tree_model_filter_get_property                    (GObject                 *object,
                                                                           guint                    prop_id,
                                                                           GValue                 *value,
                                                                           GParamSpec             *pspec);

/* signal handlers */
static void         gtk_tree_model_filter_row_changed                     (GtkTreeModel           *c_model,
                                                                           GtkTreePath            *c_path,
                                                                           GtkTreeIter            *c_iter,
                                                                           gpointer                data);
static void         gtk_tree_model_filter_row_inserted                    (GtkTreeModel           *c_model,
                                                                           GtkTreePath            *c_path,
                                                                           GtkTreeIter            *c_iter,
                                                                           gpointer                data);
static void         gtk_tree_model_filter_row_has_child_toggled           (GtkTreeModel           *c_model,
                                                                           GtkTreePath            *c_path,
                                                                           GtkTreeIter            *c_iter,
                                                                           gpointer                data);
static void         gtk_tree_model_filter_row_deleted                     (GtkTreeModel           *c_model,
                                                                           GtkTreePath            *c_path,
                                                                           gpointer                data);
static void         gtk_tree_model_filter_rows_reordered                  (GtkTreeModel           *c_model,
                                                                           GtkTreePath            *c_path,
                                                                           GtkTreeIter            *c_iter,
                                                                           gint                   *new_order,
                                                                           gpointer                data);

/* GtkTreeModel interface */
static GtkTreeModelFlags gtk_tree_model_filter_get_flags                       (GtkTreeModel           *model);
static gint         gtk_tree_model_filter_get_n_columns                   (GtkTreeModel           *model);
static GType        gtk_tree_model_filter_get_column_type                 (GtkTreeModel           *model,
                                                                           gint                    index);
static gboolean     gtk_tree_model_filter_get_iter_full                   (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter,
                                                                           GtkTreePath            *path);
static gboolean     gtk_tree_model_filter_get_iter                        (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter,
                                                                           GtkTreePath            *path);
static GtkTreePath *gtk_tree_model_filter_get_path                        (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter);
static void         gtk_tree_model_filter_get_value                       (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter,
                                                                           gint                    column,
                                                                           GValue                 *value);
static gboolean     gtk_tree_model_filter_iter_next                       (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter);
static gboolean     gtk_tree_model_filter_iter_children                   (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter,
                                                                           GtkTreeIter            *parent);
static gboolean     gtk_tree_model_filter_iter_has_child                  (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter);
static gint         gtk_tree_model_filter_iter_n_children                 (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter);
static gboolean     gtk_tree_model_filter_iter_nth_child                  (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter,
                                                                           GtkTreeIter            *parent,
                                                                           gint                    n);
static gboolean     gtk_tree_model_filter_iter_parent                     (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter,
                                                                           GtkTreeIter            *child);
static void         gtk_tree_model_filter_ref_node                        (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter);
static void         gtk_tree_model_filter_unref_node                      (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter);

/* TreeDragSource interface */
static gboolean    gtk_tree_model_filter_row_draggable                    (GtkTreeDragSource      *drag_source,
                                                                           GtkTreePath            *path);
static gboolean    gtk_tree_model_filter_drag_data_get                    (GtkTreeDragSource      *drag_source,
                                                                           GtkTreePath            *path,
                                                                           GtkSelectionData       *selection_data);
static gboolean    gtk_tree_model_filter_drag_data_delete                 (GtkTreeDragSource      *drag_source,
                                                                           GtkTreePath            *path);

/* TreeDragDest interface */
static gboolean    gtk_tree_model_filter_drag_data_received               (GtkTreeDragDest        *drag_dest,
									   GtkTreePath            *dest,
									   GtkSelectionData       *selection_data);
static gboolean    gtk_tree_model_filter_row_drop_possible                (GtkTreeDragDest        *drag_dest,
									   GtkTreePath            *dest,
									   GtkSelectionData       *selection_data);

/* private functions */
static void        gtk_tree_model_filter_build_level                      (GtkTreeModelFilter     *filter,
                                                                           FilterLevel            *parent_level,
                                                                           FilterElt              *parent_elt,
                                                                           gboolean                emit_inserted);

static void        gtk_tree_model_filter_free_level                       (GtkTreeModelFilter     *filter,
                                                                           FilterLevel            *filter_level);

static GtkTreePath *gtk_tree_model_filter_elt_get_path                    (FilterLevel            *level,
                                                                           FilterElt              *elt,
                                                                           GtkTreePath            *root);

static GtkTreePath *gtk_tree_model_filter_add_root                        (GtkTreePath            *src,
                                                                           GtkTreePath            *root);
static GtkTreePath *gtk_tree_model_filter_remove_root                     (GtkTreePath            *src,
                                                                           GtkTreePath            *root);

static void         gtk_tree_model_filter_increment_stamp                 (GtkTreeModelFilter     *filter);

static gboolean     gtk_tree_model_filter_visible                         (GtkTreeModelFilter     *filter,
                                                                           GtkTreeIter            *child_iter);
static void         gtk_tree_model_filter_clear_cache_helper              (GtkTreeModelFilter     *filter,
                                                                           FilterLevel            *level);

static void         gtk_tree_model_filter_real_unref_node                 (GtkTreeModel           *model,
                                                                           GtkTreeIter            *iter,
                                                                           gboolean                propagate_unref);

static void         gtk_tree_model_filter_set_model                       (GtkTreeModelFilter     *filter,
                                                                           GtkTreeModel           *child_model);
static void         gtk_tree_model_filter_ref_path                        (GtkTreeModelFilter     *filter,
                                                                           GtkTreePath            *path);
static void         gtk_tree_model_filter_unref_path                      (GtkTreeModelFilter     *filter,
                                                                           GtkTreePath            *path);
static void         gtk_tree_model_filter_set_root                        (GtkTreeModelFilter     *filter,
                                                                           GtkTreePath            *root);

static GtkTreePath *gtk_real_tree_model_filter_convert_child_path_to_path (GtkTreeModelFilter     *filter,
                                                                           GtkTreePath            *child_path,
                                                                           gboolean                build_levels,
                                                                           gboolean                fetch_children);

static FilterElt   *gtk_tree_model_filter_fetch_child                     (GtkTreeModelFilter     *filter,
                                                                           FilterLevel            *level,
                                                                           gint                    offset,
                                                                           gint                   *index);
static void         gtk_tree_model_filter_remove_node                     (GtkTreeModelFilter     *filter,
                                                                           GtkTreeIter            *iter);
static void         gtk_tree_model_filter_update_children                 (GtkTreeModelFilter     *filter,
                                                                           FilterLevel            *level,
                                                                           FilterElt              *elt);


G_DEFINE_TYPE_WITH_CODE (GtkTreeModelFilter, gtk_tree_model_filter, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
						gtk_tree_model_filter_tree_model_init)
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_SOURCE,
						gtk_tree_model_filter_drag_source_init)
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_DEST,
						gtk_tree_model_filter_drag_dest_init))

static void
gtk_tree_model_filter_init (GtkTreeModelFilter *filter)
{
  filter->priv = GTK_TREE_MODEL_FILTER_GET_PRIVATE (filter);

  filter->priv->visible_column = -1;
  filter->priv->zero_ref_count = 0;
  filter->priv->visible_method_set = FALSE;
  filter->priv->modify_func_set = FALSE;
  filter->priv->in_row_deleted = FALSE;
  filter->priv->virtual_root_deleted = FALSE;
}

static void
gtk_tree_model_filter_class_init (GtkTreeModelFilterClass *filter_class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *) filter_class;

  object_class->set_property = gtk_tree_model_filter_set_property;
  object_class->get_property = gtk_tree_model_filter_get_property;

  object_class->finalize = gtk_tree_model_filter_finalize;

  /* Properties -- FIXME: disabled translations for now, until I can come up with a
   * better description
   */
  g_object_class_install_property (object_class,
                                   PROP_CHILD_MODEL,
                                   g_param_spec_object ("child-model",
                                                        ("The child model"),
                                                        ("The model for the filtermodel to filter"),
                                                        GTK_TYPE_TREE_MODEL,
                                                        GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_VIRTUAL_ROOT,
                                   g_param_spec_boxed ("virtual-root",
                                                       ("The virtual root"),
                                                       ("The virtual root (relative to the child model) for this filtermodel"),
                                                       GTK_TYPE_TREE_PATH,
                                                       GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (object_class, sizeof (GtkTreeModelFilterPrivate));
}

static void
gtk_tree_model_filter_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags = gtk_tree_model_filter_get_flags;
  iface->get_n_columns = gtk_tree_model_filter_get_n_columns;
  iface->get_column_type = gtk_tree_model_filter_get_column_type;
  iface->get_iter = gtk_tree_model_filter_get_iter;
  iface->get_path = gtk_tree_model_filter_get_path;
  iface->get_value = gtk_tree_model_filter_get_value;
  iface->iter_next = gtk_tree_model_filter_iter_next;
  iface->iter_children = gtk_tree_model_filter_iter_children;
  iface->iter_has_child = gtk_tree_model_filter_iter_has_child;
  iface->iter_n_children = gtk_tree_model_filter_iter_n_children;
  iface->iter_nth_child = gtk_tree_model_filter_iter_nth_child;
  iface->iter_parent = gtk_tree_model_filter_iter_parent;
  iface->ref_node = gtk_tree_model_filter_ref_node;
  iface->unref_node = gtk_tree_model_filter_unref_node;
}

static void
gtk_tree_model_filter_drag_source_init (GtkTreeDragSourceIface *iface)
{
  iface->row_draggable = gtk_tree_model_filter_row_draggable;
  iface->drag_data_delete = gtk_tree_model_filter_drag_data_delete;
  iface->drag_data_get = gtk_tree_model_filter_drag_data_get;
}

static void
gtk_tree_model_filter_drag_dest_init (GtkTreeDragDestIface *iface)
{
  iface->drag_data_received = gtk_tree_model_filter_drag_data_received;
  iface->row_drop_possible = gtk_tree_model_filter_row_drop_possible;
}


static void
gtk_tree_model_filter_finalize (GObject *object)
{
  GtkTreeModelFilter *filter = (GtkTreeModelFilter *) object;

  if (filter->priv->virtual_root && !filter->priv->virtual_root_deleted)
    {
      gtk_tree_model_filter_unref_path (filter, filter->priv->virtual_root);
      filter->priv->virtual_root_deleted = TRUE;
    }

  gtk_tree_model_filter_set_model (filter, NULL);

  if (filter->priv->virtual_root)
    gtk_tree_path_free (filter->priv->virtual_root);

  if (filter->priv->root)
    gtk_tree_model_filter_free_level (filter, filter->priv->root);

  g_free (filter->priv->modify_types);
  
  if (filter->priv->modify_destroy)
    filter->priv->modify_destroy (filter->priv->modify_data);

  if (filter->priv->visible_destroy)
    filter->priv->visible_destroy (filter->priv->visible_data);

  /* must chain up */
  G_OBJECT_CLASS (gtk_tree_model_filter_parent_class)->finalize (object);
}

static void
gtk_tree_model_filter_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (object);

  switch (prop_id)
    {
      case PROP_CHILD_MODEL:
        gtk_tree_model_filter_set_model (filter, g_value_get_object (value));
        break;
      case PROP_VIRTUAL_ROOT:
        gtk_tree_model_filter_set_root (filter, g_value_get_boxed (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gtk_tree_model_filter_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (object);

  switch (prop_id)
    {
      case PROP_CHILD_MODEL:
        g_value_set_object (value, filter->priv->child_model);
        break;
      case PROP_VIRTUAL_ROOT:
        g_value_set_boxed (value, filter->priv->virtual_root);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

/* helpers */

static FilterElt *
filter_elt_new (void)
{
  return g_slice_new (FilterElt);
}

static void
filter_elt_free (gpointer elt)
{
  g_slice_free (FilterElt, elt);
}

static gint
filter_elt_cmp (gconstpointer a,
                gconstpointer b,
                gpointer user_data)
{
  const FilterElt *elt_a = a;
  const FilterElt *elt_b = b;

  if (elt_a->offset > elt_b->offset)
    return +1;
  else if (elt_a->offset < elt_b->offset)
    return -1;
  else
    return 0;
}

static FilterElt *
bsearch_elt_with_offset (GSequence *seq,
                         gint offset,
                         GSequenceIter **ret_siter)
{
  GSequenceIter *siter;
  FilterElt dummy;
  gint c;

  dummy.offset = offset;
  siter = g_sequence_search (seq, &dummy, filter_elt_cmp, NULL);

  /* g_sequence_search() always return next item, this is a ugly workaround,
   * surely we should implement g_sequence_lookup() or something like that,
   * see GNOME bug #617254 */
  siter = g_sequence_iter_prev (siter);
  if (!g_sequence_iter_is_end (siter))
    {
      c = filter_elt_cmp (&dummy, GET_ELT (siter), NULL);
      if (c != 0)
        siter = NULL;
    }
  else
    {
      siter = NULL;
    }

  if (ret_siter)
    *ret_siter = siter;

  return GET_ELT (siter);
}

static void
gtk_tree_model_filter_build_level (GtkTreeModelFilter *filter,
                                   FilterLevel        *parent_level,
                                   FilterElt          *parent_elt,
                                   gboolean            emit_inserted)
{
  GtkTreeIter iter;
  GtkTreeIter first_node;
  GtkTreeIter root;
  FilterLevel *new_level;
  gint length = 0;
  gint i;
  gboolean empty = TRUE;

  g_assert (filter->priv->child_model != NULL);

  if (filter->priv->in_row_deleted)
    return;

  if (!parent_level)
    {
      if (filter->priv->virtual_root)
        {
          if (gtk_tree_model_get_iter (filter->priv->child_model, &root, filter->priv->virtual_root) == FALSE)
            return;
          length = gtk_tree_model_iter_n_children (filter->priv->child_model, &root);

          if (gtk_tree_model_iter_children (filter->priv->child_model, &iter, &root) == FALSE)
            return;
        }
      else
        {
          if (!gtk_tree_model_get_iter_first (filter->priv->child_model, &iter))
            return;
          length = gtk_tree_model_iter_n_children (filter->priv->child_model, NULL);
        }
    }
  else
    {
      GtkTreeIter parent_iter;
      GtkTreeIter child_parent_iter;

      parent_iter.stamp = filter->priv->stamp;
      parent_iter.user_data = parent_level;
      parent_iter.user_data2 = parent_elt;

      gtk_tree_model_filter_convert_iter_to_child_iter (filter,
                                                        &child_parent_iter,
                                                        &parent_iter);
      if (gtk_tree_model_iter_children (filter->priv->child_model, &iter, &child_parent_iter) == FALSE)
        return;

      /* stamp may have changed */
      gtk_tree_model_filter_convert_iter_to_child_iter (filter,
                                                        &child_parent_iter,
                                                        &parent_iter);
      length = gtk_tree_model_iter_n_children (filter->priv->child_model, &child_parent_iter);
    }

  g_return_if_fail (length > 0);

  new_level = g_new (FilterLevel, 1);
  new_level->seq = g_sequence_new (filter_elt_free);
  new_level->visible_seq = g_sequence_new (NULL);
  new_level->ref_count = 0;
  new_level->parent_elt = parent_elt;
  new_level->parent_level = parent_level;

  if (parent_elt)
    parent_elt->children = new_level;
  else
    filter->priv->root = new_level;

  /* increase the count of zero ref_counts */
  while (parent_level)
    {
      parent_elt->zero_ref_count++;

      parent_elt = parent_level->parent_elt;
      parent_level = parent_level->parent_level;
    }
  if (new_level != filter->priv->root)
    filter->priv->zero_ref_count++;

  i = 0;

  first_node = iter;

  do
    {
      if (gtk_tree_model_filter_visible (filter, &iter))
        {
          FilterElt *filter_elt;

          filter_elt = filter_elt_new ();
          filter_elt->offset = i;
          filter_elt->zero_ref_count = 0;
          filter_elt->ref_count = 0;
          filter_elt->children = NULL;
          filter_elt->visible_siter = NULL;

          if (GTK_TREE_MODEL_FILTER_CACHE_CHILD_ITERS (filter))
            filter_elt->iter = iter;

          g_sequence_append (new_level->seq, filter_elt);
          filter_elt->visible_siter = g_sequence_append (new_level->visible_seq, filter_elt);
          empty = FALSE;

          if (new_level->parent_level || filter->priv->virtual_root)
            {
              GtkTreeIter f_iter;

              f_iter.stamp = filter->priv->stamp;
              f_iter.user_data = new_level;
              f_iter.user_data2 = filter_elt;

              gtk_tree_model_filter_ref_node (GTK_TREE_MODEL (filter), &f_iter);

              if (emit_inserted)
                {
                  GtkTreePath *f_path;

                  f_path = gtk_tree_model_get_path (GTK_TREE_MODEL (filter),
                                                    &f_iter);
                  gtk_tree_model_row_inserted (GTK_TREE_MODEL (filter),
                                               f_path, &f_iter);
                  gtk_tree_path_free (f_path);
                }
            }
        }
      i++;
    }
  while (gtk_tree_model_iter_next (filter->priv->child_model, &iter));

  if (empty && (new_level != filter->priv->root || filter->priv->virtual_root))
    {
      /* If none of the nodes are visible, we will just pull in the
       * first node of the level and keep a reference on it.  We need this
       * to make sure that we get all signals for this level.
       */
      FilterElt *filter_elt;
      GtkTreeIter f_iter;

      filter_elt = filter_elt_new ();
      filter_elt->offset = 0;
      filter_elt->zero_ref_count = 0;
      filter_elt->ref_count = 0;
      filter_elt->children = NULL;
      filter_elt->visible_siter = NULL;

      if (GTK_TREE_MODEL_FILTER_CACHE_CHILD_ITERS (filter))
        filter_elt->iter = first_node;

      g_sequence_append (new_level->seq, filter_elt);

      f_iter.stamp = filter->priv->stamp;
      f_iter.user_data = new_level;
      f_iter.user_data2 = filter_elt;

      gtk_tree_model_filter_ref_node (GTK_TREE_MODEL (filter), &f_iter);
    }
  else if (empty)
    gtk_tree_model_filter_free_level (filter, new_level);
}

static void
gtk_tree_model_filter_free_level (GtkTreeModelFilter *filter,
                                  FilterLevel        *filter_level)
{
  GSequenceIter *siter;
  GSequenceIter *end_siter;

  g_assert (filter_level);

  end_siter = g_sequence_get_end_iter (filter_level->seq);
  for (siter = g_sequence_get_begin_iter (filter_level->seq);
       siter != end_siter;
       siter = g_sequence_iter_next (siter))
    {
      FilterElt *elt = g_sequence_get (siter);

      if (elt->children)
        gtk_tree_model_filter_free_level (filter, FILTER_LEVEL (elt->children));

      if (filter_level->parent_level || filter->priv->virtual_root)
        {
          GtkTreeIter f_iter;

          f_iter.stamp = filter->priv->stamp;
          f_iter.user_data = filter_level;
          f_iter.user_data2 = elt;

          gtk_tree_model_filter_unref_node (GTK_TREE_MODEL (filter), &f_iter);
        }
    }

  if (filter_level->ref_count == 0)
    {
      FilterLevel *parent_level = filter_level->parent_level;
      FilterElt *parent_elt = filter_level->parent_elt;

      while (parent_level)
        {
	  parent_elt->zero_ref_count--;

	  parent_elt = parent_level->parent_elt;
	  parent_level = parent_level->parent_level;
        }

      if (filter_level != filter->priv->root)
        filter->priv->zero_ref_count--;
    }

  if (filter_level->parent_elt)
    filter_level->parent_elt->children = NULL;
  else
    filter->priv->root = NULL;

  g_sequence_free (filter_level->seq);
  g_sequence_free (filter_level->visible_seq);
  g_free (filter_level);
}

/* Creates paths suitable for accessing the child model. */
static GtkTreePath *
gtk_tree_model_filter_elt_get_path (FilterLevel *level,
                                    FilterElt   *elt,
                                    GtkTreePath *root)
{
  FilterLevel *walker = level;
  FilterElt *walker2 = elt;
  GtkTreePath *path;
  GtkTreePath *real_path;

  g_return_val_if_fail (level != NULL, NULL);
  g_return_val_if_fail (elt != NULL, NULL);

  path = gtk_tree_path_new ();

  while (walker)
    {
      gtk_tree_path_prepend_index (path, walker2->offset);

      walker2 = walker->parent_elt;
      walker = walker->parent_level;
    }

  if (root)
    {
      real_path = gtk_tree_model_filter_add_root (path, root);
      gtk_tree_path_free (path);
      return real_path;
    }

  return path;
}

static GtkTreePath *
gtk_tree_model_filter_add_root (GtkTreePath *src,
                                GtkTreePath *root)
{
  GtkTreePath *retval;
  gint i;

  retval = gtk_tree_path_copy (root);

  for (i = 0; i < gtk_tree_path_get_depth (src); i++)
    gtk_tree_path_append_index (retval, gtk_tree_path_get_indices (src)[i]);

  return retval;
}

static GtkTreePath *
gtk_tree_model_filter_remove_root (GtkTreePath *src,
                                   GtkTreePath *root)
{
  GtkTreePath *retval;
  gint i;
  gint depth;
  gint *indices;

  if (gtk_tree_path_get_depth (src) <= gtk_tree_path_get_depth (root))
    return NULL;

  depth = gtk_tree_path_get_depth (src);
  indices = gtk_tree_path_get_indices (src);

  for (i = 0; i < gtk_tree_path_get_depth (root); i++)
    if (indices[i] != gtk_tree_path_get_indices (root)[i])
      return NULL;

  retval = gtk_tree_path_new ();

  for (; i < depth; i++)
    gtk_tree_path_append_index (retval, indices[i]);

  return retval;
}

static void
gtk_tree_model_filter_increment_stamp (GtkTreeModelFilter *filter)
{
  do
    {
      filter->priv->stamp++;
    }
  while (filter->priv->stamp == 0);

  gtk_tree_model_filter_clear_cache (filter);
}

static gboolean
gtk_tree_model_filter_visible (GtkTreeModelFilter *filter,
                               GtkTreeIter        *child_iter)
{
  if (filter->priv->visible_func)
    {
      return filter->priv->visible_func (filter->priv->child_model,
					 child_iter,
					 filter->priv->visible_data)
	? TRUE : FALSE;
    }
  else if (filter->priv->visible_column >= 0)
   {
     GValue val = {0, };

     gtk_tree_model_get_value (filter->priv->child_model, child_iter,
                               filter->priv->visible_column, &val);

     if (g_value_get_boolean (&val))
       {
         g_value_unset (&val);
         return TRUE;
       }

     g_value_unset (&val);
     return FALSE;
   }

  /* no visible function set, so always visible */
  return TRUE;
}

static void
gtk_tree_model_filter_clear_cache_helper_iter (gpointer data,
                                               gpointer user_data)
{
  GtkTreeModelFilter *filter = user_data;
  FilterElt *elt = data;

  if (elt->zero_ref_count > 0)
    gtk_tree_model_filter_clear_cache_helper (filter, elt->children);
}

static void
gtk_tree_model_filter_clear_cache_helper (GtkTreeModelFilter *filter,
                                          FilterLevel        *level)
{
  g_assert (level);

  g_sequence_foreach (level->seq, gtk_tree_model_filter_clear_cache_helper_iter, NULL);

  if (level->ref_count == 0 && level != filter->priv->root)
    {
      gtk_tree_model_filter_free_level (filter, level);
      return;
    }
}

static FilterElt *
gtk_tree_model_filter_fetch_child (GtkTreeModelFilter *filter,
                                   FilterLevel        *level,
                                   gint                offset,
                                   gint               *index)
{
  gint len;
  GtkTreePath *c_path = NULL;
  GtkTreeIter c_iter;
  GtkTreePath *c_parent_path = NULL;
  GtkTreeIter c_parent_iter;
  FilterElt *elt;
  GSequenceIter *siter;

  /* check if child exists and is visible */
  if (level->parent_elt)
    {
      c_parent_path =
        gtk_tree_model_filter_elt_get_path (level->parent_level,
                                            level->parent_elt,
                                            filter->priv->virtual_root);
      if (!c_parent_path)
        return NULL;
    }
  else
    {
      if (filter->priv->virtual_root)
        c_parent_path = gtk_tree_path_copy (filter->priv->virtual_root);
      else
        c_parent_path = NULL;
    }

  if (c_parent_path)
    {
      gtk_tree_model_get_iter (filter->priv->child_model,
                               &c_parent_iter,
                               c_parent_path);
      len = gtk_tree_model_iter_n_children (filter->priv->child_model,
                                            &c_parent_iter);

      c_path = gtk_tree_path_copy (c_parent_path);
      gtk_tree_path_free (c_parent_path);
    }
  else
    {
      len = gtk_tree_model_iter_n_children (filter->priv->child_model, NULL);
      c_path = gtk_tree_path_new ();
    }

  gtk_tree_path_append_index (c_path, offset);
  gtk_tree_model_get_iter (filter->priv->child_model, &c_iter, c_path);
  gtk_tree_path_free (c_path);

  if (offset >= len || !gtk_tree_model_filter_visible (filter, &c_iter))
    return NULL;

  /* add child */
  elt = filter_elt_new ();
  elt->offset = offset;
  elt->zero_ref_count = 0;
  elt->ref_count = 0;
  elt->children = NULL;
  elt->visible_siter = NULL;

  if (GTK_TREE_MODEL_FILTER_CACHE_CHILD_ITERS (filter))
    elt->iter = c_iter;

  /* don't insert in visible_seq as we don't emit row_inserted */
  siter = g_sequence_insert_sorted (level->seq, elt, filter_elt_cmp, NULL);
  *index = g_sequence_iter_get_position (siter);

  c_iter.stamp = filter->priv->stamp;
  c_iter.user_data = level;
  c_iter.user_data2 = elt;

  if (level->parent_level || filter->priv->virtual_root)
    gtk_tree_model_filter_ref_node (GTK_TREE_MODEL (filter), &c_iter);

  return elt;
}

static void
gtk_tree_model_filter_remove_node (GtkTreeModelFilter *filter,
                                   GtkTreeIter        *iter)
{
  FilterElt *elt, *parent;
  FilterLevel *level, *parent_level;
  gint length;

  gboolean emit_child_toggled = FALSE;

  level = FILTER_LEVEL (iter->user_data);
  elt = FILTER_ELT (iter->user_data2);

  parent = level->parent_elt;
  parent_level = level->parent_level;

  length = g_sequence_get_length (level->seq);

  /* we distinguish a couple of cases:
   *  - root level, length > 1: emit row-deleted and remove.
   *  - root level, length == 1: emit row-deleted and keep in cache.
   *  - level, length == 1: parent->ref_count > 1: emit row-deleted and keep.
   *  - level, length > 1: emit row-deleted and remove.
   *  - else, remove level.
   *
   *  if level != root level and visible nodes == 1, emit row-has-child-toggled.
   */

  if (level != filter->priv->root && g_sequence_get_length (level->visible_seq) == 1)
    emit_child_toggled = TRUE;

  if (length > 1)
    {
      GtkTreePath *path;
      GSequenceIter *siter;

      /* we emit row-deleted, and remove the node from the cache.
       */

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (filter), iter);
      g_sequence_remove (elt->visible_siter);
      elt->visible_siter = NULL;
      gtk_tree_model_filter_increment_stamp (filter);
      iter->stamp = filter->priv->stamp;
      gtk_tree_model_row_deleted (GTK_TREE_MODEL (filter), path);
      gtk_tree_path_free (path);

      while (elt->ref_count > 1)
        gtk_tree_model_filter_real_unref_node (GTK_TREE_MODEL (filter),
                                               iter, FALSE);

      if (parent_level || filter->priv->virtual_root)
        gtk_tree_model_filter_unref_node (GTK_TREE_MODEL (filter), iter);
      else if (elt->ref_count > 0)
        gtk_tree_model_filter_real_unref_node (GTK_TREE_MODEL (filter),
                                               iter, FALSE);

      /* remove the node */
      bsearch_elt_with_offset (level->seq, elt->offset, &siter);
      g_sequence_remove (siter);
    }
  else if ((length == 1 && parent && parent->ref_count > 1)
           || (length == 1 && level == filter->priv->root))
    {
      GtkTreePath *path;

      /* we emit row-deleted, but keep the node in the cache and
       * referenced.
       */

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (filter), iter);
      g_sequence_remove (elt->visible_siter);
      elt->visible_siter = NULL;
      gtk_tree_model_filter_increment_stamp (filter);
      gtk_tree_model_row_deleted (GTK_TREE_MODEL (filter), path);
      gtk_tree_path_free (path);
    }
  else
    {
      GtkTreePath *path;

      /* blow level away */

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (filter), iter);
      g_sequence_remove (elt->visible_siter);
      elt->visible_siter = NULL;
      gtk_tree_model_filter_increment_stamp (filter);
      iter->stamp = filter->priv->stamp;
      gtk_tree_model_row_deleted (GTK_TREE_MODEL (filter), path);
      gtk_tree_path_free (path);

      while (elt->ref_count > 1)
        gtk_tree_model_filter_real_unref_node (GTK_TREE_MODEL (filter),
                                               iter, FALSE);

      gtk_tree_model_filter_free_level (filter, level);
    }

  if (emit_child_toggled)
    {
      GtkTreeIter piter;
      GtkTreePath *ppath;

      piter.stamp = filter->priv->stamp;
      piter.user_data = parent_level;
      piter.user_data2 = parent;

      ppath = gtk_tree_model_get_path (GTK_TREE_MODEL (filter), &piter);

      gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (filter),
                                            ppath, &piter);
      gtk_tree_path_free (ppath);
    }
}

static void
gtk_tree_model_filter_update_children (GtkTreeModelFilter *filter,
				       FilterLevel        *level,
				       FilterElt          *elt)
{
  GtkTreeIter c_iter;
  GtkTreeIter iter;

  if (!elt->visible_siter)
    return;

  iter.stamp = filter->priv->stamp;
  iter.user_data = level;
  iter.user_data2 = elt;

  gtk_tree_model_filter_convert_iter_to_child_iter (filter, &c_iter, &iter);

  if (gtk_tree_model_iter_has_child (filter->priv->child_model, &c_iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (filter),
                                                   &iter);
      gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (filter),
                                            path,
                                            &iter);
      if (path)
        gtk_tree_path_free (path);
    }
}

static void
set_invisible_siter (gpointer data,
                    gpointer user_data)
{
  FilterElt *elt = data;

  g_sequence_remove (elt->visible_siter);
  elt->visible_siter = NULL;
}

/* TreeModel signals */
static void
gtk_tree_model_filter_row_changed (GtkTreeModel *c_model,
                                   GtkTreePath  *c_path,
                                   GtkTreeIter  *c_iter,
                                   gpointer      data)
{
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (data);
  GtkTreeIter iter;
  GtkTreeIter children;
  GtkTreeIter real_c_iter;
  GtkTreePath *path = NULL;

  FilterElt *elt;
  FilterLevel *level;

  gboolean requested_state;
  gboolean current_state;
  gboolean free_c_path = FALSE;

  g_return_if_fail (c_path != NULL || c_iter != NULL);

  if (!c_path)
    {
      c_path = gtk_tree_model_get_path (c_model, c_iter);
      free_c_path = TRUE;
    }

  if (c_iter)
    real_c_iter = *c_iter;
  else
    gtk_tree_model_get_iter (c_model, &real_c_iter, c_path);

  /* is this node above the virtual root? */
  if (filter->priv->virtual_root
      && (gtk_tree_path_get_depth (filter->priv->virtual_root)
          >= gtk_tree_path_get_depth (c_path)))
    goto done;

  /* what's the requested state? */
  requested_state = gtk_tree_model_filter_visible (filter, &real_c_iter);

  /* now, let's see whether the item is there */
  path = gtk_real_tree_model_filter_convert_child_path_to_path (filter,
                                                                c_path,
                                                                FALSE,
                                                                FALSE);

  if (path)
    {
      gtk_tree_model_filter_get_iter_full (GTK_TREE_MODEL (filter),
                                           &iter, path);
      current_state = FILTER_ELT (iter.user_data2)->visible_siter != NULL;
    }
  else
    current_state = FALSE;

  if (current_state == FALSE && requested_state == FALSE)
    /* no changes required */
    goto done;

  if (current_state == TRUE && requested_state == FALSE)
    {
      /* get rid of this node */
      gtk_tree_model_filter_remove_node (filter, &iter);
      goto done;
    }

  if (current_state == TRUE && requested_state == TRUE)
    {
      /* propagate the signal; also get a path taking only visible
       * nodes into account.
       */
      gtk_tree_path_free (path);
      path = gtk_tree_model_get_path (GTK_TREE_MODEL (filter), &iter);
      gtk_tree_model_row_changed (GTK_TREE_MODEL (filter), path, &iter);

      level = FILTER_LEVEL (iter.user_data);
      elt = FILTER_ELT (iter.user_data2);

      /* and update the children */
      if (gtk_tree_model_iter_children (c_model, &children, &real_c_iter))
        gtk_tree_model_filter_update_children (filter, level, elt);

      goto done;
    }

  /* only current == FALSE and requested == TRUE is left,
   * pull in the child
   */
  g_return_if_fail (current_state == FALSE && requested_state == TRUE);

  /* make sure the new item has been pulled in */
  if (!filter->priv->root)
    {
      FilterLevel *root;

      gtk_tree_model_filter_build_level (filter, NULL, NULL, FALSE);

      root = FILTER_LEVEL (filter->priv->root);

      if (root)
        g_sequence_foreach (root->seq, set_invisible_siter, NULL);
    }

  gtk_tree_model_filter_increment_stamp (filter);

  if (!path)
    path = gtk_real_tree_model_filter_convert_child_path_to_path (filter,
                                                                  c_path,
                                                                  TRUE,
                                                                  TRUE);

  if (!path)
    /* parent is probably being filtered out */
    goto done;

  gtk_tree_model_filter_get_iter_full (GTK_TREE_MODEL (filter), &iter, path);

  level = FILTER_LEVEL (iter.user_data);
  elt = FILTER_ELT (iter.user_data2);

  /* elt->visible_siter can be set at this point if it was pulled in above */
  if (!elt->visible_siter)
    elt->visible_siter = g_sequence_insert_sorted (level->visible_seq, elt,
        filter_elt_cmp, NULL);

  if ((level->parent_elt && level->parent_elt->visible_siter) || !level->parent_elt)
    {
      /* visibility changed -- reget path */
      gtk_tree_path_free (path);
      path = gtk_tree_model_get_path (GTK_TREE_MODEL (filter), &iter);

      gtk_tree_model_row_inserted (GTK_TREE_MODEL (filter), path, &iter);

      if (gtk_tree_model_iter_children (c_model, &children, c_iter))
        gtk_tree_model_filter_update_children (filter, level, elt);
    }

done:
  if (path)
    gtk_tree_path_free (path);

  if (free_c_path)
    gtk_tree_path_free (c_path);
}

static void
increase_offset_iter (gpointer data,
                      gpointer user_data)
{
  FilterElt *elt = data;
  gint offset = GPOINTER_TO_INT (user_data);

  if (elt->offset >= offset)
    elt->offset++;
}

static void
gtk_tree_model_filter_row_inserted (GtkTreeModel *c_model,
                                    GtkTreePath  *c_path,
                                    GtkTreeIter  *c_iter,
                                    gpointer      data)
{
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (data);
  GtkTreePath *path = NULL;
  GtkTreePath *real_path = NULL;
  GtkTreeIter iter;

  GtkTreeIter real_c_iter;

  FilterLevel *level;
  FilterLevel *parent_level;
  GSequenceIter *siter;
  FilterElt dummy;

  gint i = 0, offset;

  gboolean free_c_path = FALSE;

  g_return_if_fail (c_path != NULL || c_iter != NULL);

  if (!c_path)
    {
      c_path = gtk_tree_model_get_path (c_model, c_iter);
      free_c_path = TRUE;
    }

  if (c_iter)
    real_c_iter = *c_iter;
  else
    gtk_tree_model_get_iter (c_model, &real_c_iter, c_path);

  /* the row has already been inserted. so we need to fixup the
   * virtual root here first
   */
  if (filter->priv->virtual_root)
    {
      if (gtk_tree_path_get_depth (filter->priv->virtual_root) >=
          gtk_tree_path_get_depth (c_path))
        {
          gint level;
          gint *v_indices, *c_indices;
          gboolean common_prefix = TRUE;

          level = gtk_tree_path_get_depth (c_path) - 1;
          v_indices = gtk_tree_path_get_indices (filter->priv->virtual_root);
          c_indices = gtk_tree_path_get_indices (c_path);

          for (i = 0; i < level; i++)
            if (v_indices[i] != c_indices[i])
              {
                common_prefix = FALSE;
                break;
              }

          if (common_prefix && v_indices[level] >= c_indices[level])
            (v_indices[level])++;
        }
    }

  if (!filter->priv->root)
    {
      /* No point in building the level if this node is not visible. */
      if (!filter->priv->virtual_root
          && !gtk_tree_model_filter_visible (filter, c_iter))
        goto done;

      /* build level will pull in the new child */
      gtk_tree_model_filter_build_level (filter, NULL, NULL, FALSE);

      if (filter->priv->root
          && g_sequence_get_length (FILTER_LEVEL (filter->priv->root)->visible_seq))
        goto done_and_emit;
      else
        goto done;
    }

  parent_level = level = FILTER_LEVEL (filter->priv->root);

  /* subtract virtual root if necessary */
  if (filter->priv->virtual_root)
    {
      real_path = gtk_tree_model_filter_remove_root (c_path,
                                                     filter->priv->virtual_root);
      /* not our child */
      if (!real_path)
        goto done;
    }
  else
    real_path = gtk_tree_path_copy (c_path);

  if (gtk_tree_path_get_depth (real_path) - 1 >= 1)
    {
      /* find the parent level */
      while (i < gtk_tree_path_get_depth (real_path) - 1)
        {
          FilterElt *elt;

          if (!level)
            /* we don't cover this signal */
            goto done;

          elt = bsearch_elt_with_offset (level->seq,
              gtk_tree_path_get_indices (real_path)[i], NULL);

          if (!elt)
            /* parent is probably being filtered out */
            goto done;

          if (!elt->children)
            {
              GtkTreePath *tmppath;
              GtkTreeIter  tmpiter;

              tmpiter.stamp = filter->priv->stamp;
              tmpiter.user_data = level;
              tmpiter.user_data2 = elt;

              tmppath = gtk_tree_model_get_path (GTK_TREE_MODEL (data),
                                                 &tmpiter);

              if (tmppath)
                {
                  gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (data),
                                                        tmppath, &tmpiter);
                  gtk_tree_path_free (tmppath);
                }

              /* not covering this signal */
              goto done;
            }

          level = elt->children;
          parent_level = level;
          i++;
        }
    }

  if (!parent_level)
    goto done;

  /* let's try to insert the value */
  offset = gtk_tree_path_get_indices (real_path)[gtk_tree_path_get_depth (real_path) - 1];

  /* update the offsets, yes if we didn't insert the node above, there will
   * be a gap here. This will be filled with the node (via fetch_child) when
   * it becomes visible
   */
  dummy.offset = offset;
  siter = g_sequence_search (level->seq, &dummy, filter_elt_cmp, NULL);
  siter = g_sequence_iter_prev (siter);
  g_sequence_foreach_range (siter, g_sequence_get_end_iter (level->seq),
      increase_offset_iter, GINT_TO_POINTER (offset));

  /* only insert when visible */
  if (gtk_tree_model_filter_visible (filter, &real_c_iter))
    {
      FilterElt *felt;

      felt = filter_elt_new ();
      felt->offset = offset;
      felt->zero_ref_count = 0;
      felt->ref_count = 0;
      felt->children = NULL;
      felt->visible_siter = NULL;

      if (GTK_TREE_MODEL_FILTER_CACHE_CHILD_ITERS (filter))
        felt->iter = real_c_iter;

      g_sequence_insert_sorted (level->seq, felt, filter_elt_cmp, NULL);
      felt->visible_siter = g_sequence_insert_sorted (level->visible_seq, felt, filter_elt_cmp, NULL);

      if (level->parent_level || filter->priv->virtual_root)
        {
          GtkTreeIter f_iter;

          f_iter.stamp = filter->priv->stamp;
          f_iter.user_data = level;
          f_iter.user_data2 = felt;

          gtk_tree_model_filter_ref_node (GTK_TREE_MODEL (filter), &f_iter);
        }
    }

  /* don't emit the signal if we aren't visible */
  if (!gtk_tree_model_filter_visible (filter, &real_c_iter))
    goto done;

done_and_emit:
  /* NOTE: pass c_path here and NOT real_path. This function does
   * root subtraction itself
   */
  path = gtk_real_tree_model_filter_convert_child_path_to_path (filter,
                                                                c_path,
                                                                FALSE, TRUE);

  if (!path)
    goto done;

  gtk_tree_model_filter_increment_stamp (filter);

  gtk_tree_model_filter_get_iter_full (GTK_TREE_MODEL (data), &iter, path);

  /* get a path taking only visible nodes into account */
  gtk_tree_path_free (path);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (data), &iter);

  gtk_tree_model_row_inserted (GTK_TREE_MODEL (data), path, &iter);

  gtk_tree_path_free (path);

done:
  if (real_path)
    gtk_tree_path_free (real_path);

  if (free_c_path)
    gtk_tree_path_free (c_path);
}

static void
gtk_tree_model_filter_row_has_child_toggled (GtkTreeModel *c_model,
                                             GtkTreePath  *c_path,
                                             GtkTreeIter  *c_iter,
                                             gpointer      data)
{
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (data);
  GtkTreePath *path;
  GtkTreeIter iter;
  FilterLevel *level;
  FilterElt *elt;

  g_return_if_fail (c_path != NULL && c_iter != NULL);

  /* If we get row-has-child-toggled on the virtual root, and there is
   * no root level; try to build it now.
   */
  if (filter->priv->virtual_root && !filter->priv->root
      && !gtk_tree_path_compare (c_path, filter->priv->virtual_root))
    {
      gtk_tree_model_filter_build_level (filter, NULL, NULL, TRUE);
      return;
    }

  if (!gtk_tree_model_filter_visible (filter, c_iter))
    return;

  path = gtk_real_tree_model_filter_convert_child_path_to_path (filter,
                                                                c_path,
                                                                FALSE,
                                                                TRUE);
  if (!path)
    return;

  gtk_tree_model_filter_get_iter_full (GTK_TREE_MODEL (data), &iter, path);

  gtk_tree_path_free (path);

  level = FILTER_LEVEL (iter.user_data);
  elt = FILTER_ELT (iter.user_data2);

#ifdef MAEMO_CHANGES
  /* FIXME: see NB#45767 */
  g_return_if_fail (elt->visible_siter);
#else /* !MAEMO_CHANGES */
  g_assert (elt->visible_siter);
#endif /* !MAEMO_CHANGES */

  /* If this node is referenced and has children, build the level so we
   * can monitor it for changes.
   */
  if (elt->ref_count > 1 && gtk_tree_model_iter_has_child (c_model, c_iter))
    gtk_tree_model_filter_build_level (filter, level, elt, TRUE);

  /* get a path taking only visible nodes into account */
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (data), &iter);
  gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (data), path, &iter);
  gtk_tree_path_free (path);
}

static void
decrease_offset_iter (gpointer data,
                      gpointer user_data)
{
  FilterElt *elt = data;
  gint offset = GPOINTER_TO_INT (user_data);

  if (elt->offset > offset)
    elt->offset--;
}

static void
gtk_tree_model_filter_row_deleted (GtkTreeModel *c_model,
                                   GtkTreePath  *c_path,
                                   gpointer      data)
{
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (data);
  GtkTreePath *path;
  GtkTreeIter iter;
  FilterElt *elt, *parent = NULL;
  FilterLevel *level, *parent_level = NULL;
  GSequenceIter *siter;
  gboolean emit_child_toggled = FALSE;
  gint offset;
  gint i;

  g_return_if_fail (c_path != NULL);

  /* special case the deletion of an ancestor of the virtual root */
  if (filter->priv->virtual_root &&
      (gtk_tree_path_is_ancestor (c_path, filter->priv->virtual_root) ||
       !gtk_tree_path_compare (c_path, filter->priv->virtual_root)))
    {
      GtkTreePath *path;
      FilterLevel *level = FILTER_LEVEL (filter->priv->root);

      gtk_tree_model_filter_unref_path (filter, filter->priv->virtual_root);
      filter->priv->virtual_root_deleted = TRUE;

      if (!level)
        return;

      /* remove everything in the filter model
       *
       * For now, we just iterate over the root level and emit a
       * row_deleted for each FilterElt. Not sure if this is correct.
       */

      gtk_tree_model_filter_increment_stamp (filter);
      path = gtk_tree_path_new ();
      gtk_tree_path_append_index (path, 0);

/* FIXME: how can it work? it doesn't use i
      for (i = 0; i < level->visible_nodes; i++)
        gtk_tree_model_row_deleted (GTK_TREE_MODEL (data), path);
*/
      gtk_tree_path_free (path);
      gtk_tree_model_filter_free_level (filter, filter->priv->root);

      return;
    }

  /* fixup virtual root */
  if (filter->priv->virtual_root)
    {
      if (gtk_tree_path_get_depth (filter->priv->virtual_root) >=
          gtk_tree_path_get_depth (c_path))
        {
          gint level;
          gint *v_indices, *c_indices;
          gboolean common_prefix = TRUE;

          level = gtk_tree_path_get_depth (c_path) - 1;
          v_indices = gtk_tree_path_get_indices (filter->priv->virtual_root);
          c_indices = gtk_tree_path_get_indices (c_path);

          for (i = 0; i < level; i++)
            if (v_indices[i] != c_indices[i])
              {
                common_prefix = FALSE;
                break;
              }

          if (common_prefix && v_indices[level] > c_indices[level])
            (v_indices[level])--;
        }
    }

  path = gtk_real_tree_model_filter_convert_child_path_to_path (filter,
                                                                c_path,
                                                                FALSE,
                                                                FALSE);

  if (!path)
    {
      /* The node deleted in the child model is not visible in the
       * filter model.  We will not emit a signal, just fixup the offsets
       * of the other nodes.
       */
      GtkTreePath *real_path;
      FilterElt dummy;

      if (!filter->priv->root)
        return;

      level = FILTER_LEVEL (filter->priv->root);

      /* subtract vroot if necessary */
      if (filter->priv->virtual_root)
        {
          real_path = gtk_tree_model_filter_remove_root (c_path,
                                                         filter->priv->virtual_root);
          /* we don't handle this */
          if (!real_path)
            return;
        }
      else
        real_path = gtk_tree_path_copy (c_path);

      i = 0;
      if (gtk_tree_path_get_depth (real_path) - 1 >= 1)
        {
          /* find the level where the deletion occurred */
          while (i < gtk_tree_path_get_depth (real_path) - 1)
            {
              if (!level)
                {
                  /* we don't cover this */
                  gtk_tree_path_free (real_path);
                  return;
                }

              elt = bsearch_elt_with_offset (level->seq,
                  gtk_tree_path_get_indices (real_path)[i], NULL);

              if (!elt || !elt->children)
                {
                  /* parent is filtered out, so no level */
                  gtk_tree_path_free (real_path);
                  return;
                }

              level = elt->children;
              i++;
            }
        }

      offset = gtk_tree_path_get_indices (real_path)[gtk_tree_path_get_depth (real_path) - 1];
      gtk_tree_path_free (real_path);

      if (!level)
        return;

      /* decrease offset of all nodes following the deleted node */
      dummy.offset = offset;
      siter = g_sequence_search (level->seq, &dummy, filter_elt_cmp, NULL);
      g_sequence_foreach_range (siter, g_sequence_get_end_iter (level->seq),
        decrease_offset_iter, GINT_TO_POINTER (offset));

      return;
    }

  /* a node was deleted, which was in our cache */
  gtk_tree_model_filter_get_iter_full (GTK_TREE_MODEL (data), &iter, path);

  level = FILTER_LEVEL (iter.user_data);
  elt = FILTER_ELT (iter.user_data2);
  offset = elt->offset;

  if (elt->visible_siter)
    {
      /* get a path taking only visible nodes into account */
      gtk_tree_path_free (path);
      path = gtk_tree_model_get_path (GTK_TREE_MODEL (filter), &iter);

      if (g_sequence_get_length (level->visible_seq) == 1)
        {
          emit_child_toggled = TRUE;
          parent_level = level->parent_level;
          parent = level->parent_elt;
        }

      /* emit row_deleted */
      gtk_tree_model_filter_increment_stamp (filter);
      gtk_tree_model_row_deleted (GTK_TREE_MODEL (data), path);
      iter.stamp = filter->priv->stamp;
    }

  /* The filter model's reference on the child node is released
   * below.
   */
  while (elt->ref_count > 1)
    gtk_tree_model_filter_real_unref_node (GTK_TREE_MODEL (data), &iter,
                                           FALSE);

  if (g_sequence_get_length (level->seq) == 1)
    {
      /* kill level */
      gtk_tree_model_filter_free_level (filter, level);
    }
  else
    {
      GSequenceIter *tmp;

      /* release the filter model's reference on the node */
      if (level->parent_level || filter->priv->virtual_root)
        gtk_tree_model_filter_unref_node (GTK_TREE_MODEL (filter), &iter);
      else if (elt->ref_count > 0)
        gtk_tree_model_filter_real_unref_node (GTK_TREE_MODEL (data), &iter,
                                               FALSE);

      /* remove the row */
      g_sequence_remove (elt->visible_siter);
      bsearch_elt_with_offset (level->seq, elt->offset, &siter);
      tmp = g_sequence_iter_next (siter);
      g_sequence_remove (siter);
      g_sequence_foreach_range (tmp, g_sequence_get_end_iter (level->seq),
        decrease_offset_iter, GINT_TO_POINTER (offset));
    }

  if (emit_child_toggled && parent_level)
    {
      GtkTreeIter iter;
      GtkTreePath *path;

      iter.stamp = filter->priv->stamp;
      iter.user_data = parent_level;
      iter.user_data2 = parent;

      /* We set in_row_deleted to TRUE to avoid a level build triggered
       * by row-has-child-toggled (parent model could call iter_has_child
       * for example).
       */
      filter->priv->in_row_deleted = TRUE;
      path = gtk_tree_model_get_path (GTK_TREE_MODEL (filter), &iter);
      gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (filter),
                                            path, &iter);
      gtk_tree_path_free (path);
      filter->priv->in_row_deleted = FALSE;
    }

  gtk_tree_path_free (path);
}

static void
gtk_tree_model_filter_rows_reordered (GtkTreeModel *c_model,
                                      GtkTreePath  *c_path,
                                      GtkTreeIter  *c_iter,
                                      gint         *new_order,
                                      gpointer      data)
{
  FilterElt *elt;
  FilterLevel *level;
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (data);

  GtkTreePath *path;
  GtkTreeIter iter;

  GSequence *tmp_seq;
  GSequenceIter *tmp_end_iter;
  gint *tmp_array;
  gint i, elt_count;
  gint length;

  g_return_if_fail (new_order != NULL);

  if (c_path == NULL || gtk_tree_path_get_depth (c_path) == 0)
    {
      length = gtk_tree_model_iter_n_children (c_model, NULL);

      if (filter->priv->virtual_root)
        {
          gint new_pos = -1;

          /* reorder root level of path */
          for (i = 0; i < length; i++)
            if (new_order[i] == gtk_tree_path_get_indices (filter->priv->virtual_root)[0])
              new_pos = i;

          if (new_pos < 0)
            return;

          gtk_tree_path_get_indices (filter->priv->virtual_root)[0] = new_pos;
          return;
        }

      path = gtk_tree_path_new ();
      level = FILTER_LEVEL (filter->priv->root);
    }
  else
    {
      GtkTreeIter child_iter;

      /* virtual root anchor reordering */
      if (filter->priv->virtual_root &&
	  gtk_tree_path_is_ancestor (c_path, filter->priv->virtual_root))
        {
          gint new_pos = -1;
          gint length;
          gint level;
          GtkTreeIter real_c_iter;

          level = gtk_tree_path_get_depth (c_path);

          if (c_iter)
            real_c_iter = *c_iter;
          else
            gtk_tree_model_get_iter (c_model, &real_c_iter, c_path);

          length = gtk_tree_model_iter_n_children (c_model, &real_c_iter);

          for (i = 0; i < length; i++)
            if (new_order[i] == gtk_tree_path_get_indices (filter->priv->virtual_root)[level])
              new_pos = i;

          if (new_pos < 0)
            return;

          gtk_tree_path_get_indices (filter->priv->virtual_root)[level] = new_pos;
          return;
        }

      path = gtk_real_tree_model_filter_convert_child_path_to_path (filter,
                                                                    c_path,
                                                                    FALSE,
                                                                    FALSE);

      if (!path && filter->priv->virtual_root &&
          gtk_tree_path_compare (c_path, filter->priv->virtual_root))
        return;

      if (!path && !filter->priv->virtual_root)
        return;

      if (!path)
        {
          /* root level mode */
          if (!c_iter)
            gtk_tree_model_get_iter (c_model, c_iter, c_path);
          length = gtk_tree_model_iter_n_children (c_model, c_iter);
          path = gtk_tree_path_new ();
          level = FILTER_LEVEL (filter->priv->root);
        }
      else
        {
          gtk_tree_model_filter_get_iter_full (GTK_TREE_MODEL (data),
                                               &iter, path);

          level = FILTER_LEVEL (iter.user_data);
          elt = FILTER_ELT (iter.user_data2);

          if (!elt->children)
            {
              gtk_tree_path_free (path);
              return;
            }

          level = elt->children;

          gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filter), &child_iter, &iter);
          length = gtk_tree_model_iter_n_children (c_model, &child_iter);
        }
    }

  if (!level || g_sequence_get_length (level->seq) < 1)
    {
      gtk_tree_path_free (path);
      return;
    }

  /* NOTE: we do not bail out here if level->seq->len < 2 like
   * GtkTreeModelSort does. This because we do some special tricky
   * reordering.
   */

  tmp_seq = g_sequence_new (filter_elt_free);
  tmp_end_iter = g_sequence_get_end_iter (tmp_seq);
  tmp_array = g_new (gint, g_sequence_get_length (level->visible_seq));
  elt_count = 0;
  for (i = 0; i < length; i++)
    {
      FilterElt *elt;
      GSequenceIter *siter;

      elt = bsearch_elt_with_offset (level->seq, new_order[i], &siter);
      if (elt == NULL)
        continue;

      if (elt->visible_siter)
        tmp_array[elt_count++] = g_sequence_iter_get_position (elt->visible_siter);

      /* Steal elt from level->seq and append it to tmp_seq */
      g_sequence_move (siter, tmp_end_iter);
      elt->offset = i;
    }

  g_warn_if_fail (g_sequence_get_length (level->seq) == 0);
  g_sequence_free (level->seq);
  level->seq = tmp_seq;
  g_sequence_sort (level->visible_seq, filter_elt_cmp, NULL);

  /* emit rows_reordered */
  if (!gtk_tree_path_get_indices (path))
    gtk_tree_model_rows_reordered (GTK_TREE_MODEL (data), path, NULL,
                                   tmp_array);
  else
    {
      /* get a path taking only visible nodes into account */
      gtk_tree_path_free (path);
      path = gtk_tree_model_get_path (GTK_TREE_MODEL (data), &iter);

      gtk_tree_model_rows_reordered (GTK_TREE_MODEL (data), path, &iter,
                                     tmp_array);
    }

  /* done */
  g_free (tmp_array);
  gtk_tree_path_free (path);
}

/* TreeModelIface implementation */
static GtkTreeModelFlags
gtk_tree_model_filter_get_flags (GtkTreeModel *model)
{
  GtkTreeModelFlags flags;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), 0);
  g_return_val_if_fail (GTK_TREE_MODEL_FILTER (model)->priv->child_model != NULL, 0);

  flags = gtk_tree_model_get_flags (GTK_TREE_MODEL_FILTER (model)->priv->child_model);

  if ((flags & GTK_TREE_MODEL_LIST_ONLY) == GTK_TREE_MODEL_LIST_ONLY)
    return GTK_TREE_MODEL_LIST_ONLY;

  return 0;
}

static gint
gtk_tree_model_filter_get_n_columns (GtkTreeModel *model)
{
  GtkTreeModelFilter *filter = (GtkTreeModelFilter *)model;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), 0);
  g_return_val_if_fail (filter->priv->child_model != NULL, 0);

  if (filter->priv->child_model == NULL)
    return 0;

  /* so we can't modify the modify func after this ... */
  filter->priv->modify_func_set = TRUE;

  if (filter->priv->modify_n_columns > 0)
    return filter->priv->modify_n_columns;

  return gtk_tree_model_get_n_columns (filter->priv->child_model);
}

static GType
gtk_tree_model_filter_get_column_type (GtkTreeModel *model,
                                       gint          index)
{
  GtkTreeModelFilter *filter = (GtkTreeModelFilter *)model;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), G_TYPE_INVALID);
  g_return_val_if_fail (filter->priv->child_model != NULL, G_TYPE_INVALID);

  /* so we can't modify the modify func after this ... */
  filter->priv->modify_func_set = TRUE;

  if (filter->priv->modify_types)
    {
      g_return_val_if_fail (index < filter->priv->modify_n_columns, G_TYPE_INVALID);

      return filter->priv->modify_types[index];
    }

  return gtk_tree_model_get_column_type (filter->priv->child_model, index);
}

/* A special case of _get_iter; this function can also get iters which
 * are not visible.  These iters should ONLY be passed internally, never
 * pass those along with a signal emission.
 */
static gboolean
gtk_tree_model_filter_get_iter_full (GtkTreeModel *model,
                                     GtkTreeIter  *iter,
                                     GtkTreePath  *path)
{
  GtkTreeModelFilter *filter = (GtkTreeModelFilter *)model;
  gint *indices;
  FilterLevel *level;
  FilterElt *elt;
  GSequenceIter *siter;
  gint depth, i;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), FALSE);
  g_return_val_if_fail (filter->priv->child_model != NULL, FALSE);

  indices = gtk_tree_path_get_indices (path);

  if (filter->priv->root == NULL)
    gtk_tree_model_filter_build_level (filter, NULL, NULL, FALSE);
  level = FILTER_LEVEL (filter->priv->root);

  depth = gtk_tree_path_get_depth (path);
  if (!depth)
    {
      iter->stamp = 0;
      return FALSE;
    }

  for (i = 0; i < depth - 1; i++)
    {
      if (!level)
        return FALSE;

      siter = g_sequence_get_iter_at_pos (level->seq, indices[i]);
      if (g_sequence_iter_is_end (siter))
        return FALSE;

      elt = GET_ELT (siter);
      if (!elt->children)
        gtk_tree_model_filter_build_level (filter, level, elt, FALSE);
      level = elt->children;
    }

  if (!level)
    return FALSE;

  siter = g_sequence_get_iter_at_pos (level->seq, indices[depth - 1]);
  if (g_sequence_iter_is_end (siter))
    return FALSE;

  iter->stamp = filter->priv->stamp;
  iter->user_data = level;
  iter->user_data2 = GET_ELT (siter);

  return TRUE;
}

static gboolean
gtk_tree_model_filter_get_iter (GtkTreeModel *model,
                                GtkTreeIter  *iter,
                                GtkTreePath  *path)
{
  GtkTreeModelFilter *filter = (GtkTreeModelFilter *)model;
  gint *indices;
  FilterLevel *level;
  FilterElt *elt;
  gint depth, i;
  GSequenceIter *siter;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), FALSE);
  g_return_val_if_fail (filter->priv->child_model != NULL, FALSE);

  indices = gtk_tree_path_get_indices (path);

  if (filter->priv->root == NULL)
    gtk_tree_model_filter_build_level (filter, NULL, NULL, FALSE);
  level = FILTER_LEVEL (filter->priv->root);

  depth = gtk_tree_path_get_depth (path);
  if (!depth)
    {
      iter->stamp = 0;
      return FALSE;
    }

  for (i = 0; i < depth - 1; i++)
    {
      if (!level)
        return FALSE;

      siter = g_sequence_get_iter_at_pos (level->visible_seq, indices[i]);
      if (g_sequence_iter_is_end (siter))
        return FALSE;

      elt = GET_ELT (siter);
      if (!elt->children)
        gtk_tree_model_filter_build_level (filter, level, elt, FALSE);
      level = elt->children;
    }

  if (!level)
    return FALSE;

  siter = g_sequence_get_iter_at_pos (level->visible_seq, indices[depth - 1]);
  if (g_sequence_iter_is_end (siter))
    return FALSE;

  iter->stamp = filter->priv->stamp;
  iter->user_data = level;
  iter->user_data2 = GET_ELT (siter);

  return TRUE;
}

static GtkTreePath *
gtk_tree_model_filter_get_path (GtkTreeModel *model,
                                GtkTreeIter  *iter)
{
  GtkTreePath *retval;
  FilterLevel *level;
  FilterElt *elt;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), NULL);
  g_return_val_if_fail (GTK_TREE_MODEL_FILTER (model)->priv->child_model != NULL, NULL);
  g_return_val_if_fail (GTK_TREE_MODEL_FILTER (model)->priv->stamp == iter->stamp, NULL);

  level = iter->user_data;
  elt = iter->user_data2;

  if (!elt->visible_siter)
    return NULL;

  retval = gtk_tree_path_new ();

  while (level)
    {
      gint index;

      index = g_sequence_iter_get_position (elt->visible_siter);
      gtk_tree_path_prepend_index (retval, index);

      elt = level->parent_elt;
      level = level->parent_level;
    }

  return retval;
}

static void
gtk_tree_model_filter_get_value (GtkTreeModel *model,
                                 GtkTreeIter  *iter,
                                 gint          column,
                                 GValue       *value)
{
  GtkTreeIter child_iter;
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (model);

  g_return_if_fail (GTK_IS_TREE_MODEL_FILTER (model));
  g_return_if_fail (GTK_TREE_MODEL_FILTER (model)->priv->child_model != NULL);
  g_return_if_fail (GTK_TREE_MODEL_FILTER (model)->priv->stamp == iter->stamp);

  if (filter->priv->modify_func)
    {
      g_return_if_fail (column < filter->priv->modify_n_columns);

      g_value_init (value, filter->priv->modify_types[column]);
      filter->priv->modify_func (model,
                           iter,
                           value,
                           column,
                           filter->priv->modify_data);

      return;
    }

  gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model), &child_iter, iter);
  gtk_tree_model_get_value (GTK_TREE_MODEL_FILTER (model)->priv->child_model,
                            &child_iter, column, value);
}

static gboolean
gtk_tree_model_filter_iter_next (GtkTreeModel *model,
                                 GtkTreeIter  *iter)
{
  FilterLevel *level;
  FilterElt *elt;
  GSequenceIter *siter;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), FALSE);
  g_return_val_if_fail (GTK_TREE_MODEL_FILTER (model)->priv->child_model != NULL, FALSE);
  g_return_val_if_fail (GTK_TREE_MODEL_FILTER (model)->priv->stamp == iter->stamp, FALSE);

  level = iter->user_data;
  elt = iter->user_data2;

  siter = g_sequence_iter_next (elt->visible_siter);
  if (g_sequence_iter_is_end (siter))
    return FALSE;

  iter->user_data2 = GET_ELT (siter);

  return TRUE;
}

static gboolean
gtk_tree_model_filter_iter_children (GtkTreeModel *model,
                                     GtkTreeIter  *iter,
                                     GtkTreeIter  *parent)
{
  GtkTreeModelFilter *filter = (GtkTreeModelFilter *)model;
  FilterLevel *level;
  GSequenceIter *siter;

  iter->stamp = 0;
  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), FALSE);
  g_return_val_if_fail (filter->priv->child_model != NULL, FALSE);
  if (parent)
    g_return_val_if_fail (filter->priv->stamp == parent->stamp, FALSE);

  if (!parent)
    {
      if (!filter->priv->root)
        gtk_tree_model_filter_build_level (filter, NULL, NULL, FALSE);
      if (!filter->priv->root)
        return FALSE;

      level = filter->priv->root;
      siter = g_sequence_get_begin_iter (level->visible_seq);
      if (g_sequence_iter_is_end (siter))
        return FALSE;

      iter->stamp = filter->priv->stamp;
      iter->user_data = level;
      iter->user_data2 = GET_ELT (siter);

      return TRUE;
    }
  else
    {
      if (FILTER_ELT (parent->user_data2)->children == NULL)
        gtk_tree_model_filter_build_level (filter,
                                           FILTER_LEVEL (parent->user_data),
                                           FILTER_ELT (parent->user_data2),
                                           FALSE);
      if (FILTER_ELT (parent->user_data2)->children == NULL)
        return FALSE;

      level = FILTER_ELT (parent->user_data2)->children;
      siter = g_sequence_get_begin_iter (level->visible_seq);
      if (g_sequence_iter_is_end (siter))
        return FALSE;

      iter->stamp = filter->priv->stamp;
      iter->user_data = level;
      iter->user_data2 = GET_ELT (siter);

      return TRUE;
    }

  iter->stamp = 0;
  return FALSE;
}

static gboolean
gtk_tree_model_filter_iter_has_child (GtkTreeModel *model,
                                      GtkTreeIter  *iter)
{
  GtkTreeIter child_iter;
  GtkTreeModelFilter *filter = (GtkTreeModelFilter *)model;
  FilterElt *elt;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), FALSE);
  g_return_val_if_fail (filter->priv->child_model != NULL, FALSE);
  g_return_val_if_fail (filter->priv->stamp == iter->stamp, FALSE);

  filter = GTK_TREE_MODEL_FILTER (model);

  gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model), &child_iter, iter);
  elt = FILTER_ELT (iter->user_data2);

  if (!elt->visible_siter)
    return FALSE;

  /* we need to build the level to check if not all children are filtered
   * out
   */
  if (!elt->children
      && gtk_tree_model_iter_has_child (filter->priv->child_model, &child_iter))
    gtk_tree_model_filter_build_level (filter, FILTER_LEVEL (iter->user_data),
                                       elt, FALSE);

  if (elt->children && g_sequence_get_length (elt->children->visible_seq) > 0)
    return TRUE;

  return FALSE;
}

static gint
gtk_tree_model_filter_iter_n_children (GtkTreeModel *model,
                                       GtkTreeIter  *iter)
{
  GtkTreeIter child_iter;
  GtkTreeModelFilter *filter = (GtkTreeModelFilter *)model;
  FilterElt *elt;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), 0);
  g_return_val_if_fail (filter->priv->child_model != NULL, 0);
  if (iter)
    g_return_val_if_fail (filter->priv->stamp == iter->stamp, 0);

  if (!iter)
    {
      if (!filter->priv->root)
        gtk_tree_model_filter_build_level (filter, NULL, NULL, FALSE);

      if (filter->priv->root)
        return g_sequence_get_length (FILTER_LEVEL (filter->priv->root)->visible_seq);

      return 0;
    }

  elt = FILTER_ELT (iter->user_data2);

  if (!elt->visible_siter)
    return 0;

  gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model), &child_iter, iter);

  if (!elt->children &&
      gtk_tree_model_iter_has_child (filter->priv->child_model, &child_iter))
    gtk_tree_model_filter_build_level (filter,
                                       FILTER_LEVEL (iter->user_data),
                                       elt, FALSE);

  if (elt->children)
    return g_sequence_get_length (elt->children->visible_seq);

  return 0;
}

static gboolean
gtk_tree_model_filter_iter_nth_child (GtkTreeModel *model,
                                      GtkTreeIter  *iter,
                                      GtkTreeIter  *parent,
                                      gint          n)
{
  FilterLevel *level;
  GtkTreeIter children;
  GSequenceIter *siter;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), FALSE);
  if (parent)
    g_return_val_if_fail (GTK_TREE_MODEL_FILTER (model)->priv->stamp == parent->stamp, FALSE);

  /* use this instead of has_child to force us to build the level, if needed */
  if (gtk_tree_model_filter_iter_children (model, &children, parent) == FALSE)
    {
      iter->stamp = 0;
      return FALSE;
    }

  level = children.user_data;
  siter = g_sequence_get_iter_at_pos (level->visible_seq, n);
  if (g_sequence_iter_is_end (siter))
    return FALSE;

  iter->stamp = GTK_TREE_MODEL_FILTER (model)->priv->stamp;
  iter->user_data = level;
  iter->user_data2 = GET_ELT (siter);

  return TRUE;
}

static gboolean
gtk_tree_model_filter_iter_parent (GtkTreeModel *model,
                                   GtkTreeIter  *iter,
                                   GtkTreeIter  *child)
{
  FilterLevel *level;

  iter->stamp = 0;
  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (model), FALSE);
  g_return_val_if_fail (GTK_TREE_MODEL_FILTER (model)->priv->child_model != NULL, FALSE);
  g_return_val_if_fail (GTK_TREE_MODEL_FILTER (model)->priv->stamp == child->stamp, FALSE);

  level = child->user_data;

  if (level->parent_level)
    {
      iter->stamp = GTK_TREE_MODEL_FILTER (model)->priv->stamp;
      iter->user_data = level->parent_level;
      iter->user_data2 = level->parent_elt;

      return TRUE;
    }

  return FALSE;
}

static void
gtk_tree_model_filter_ref_node (GtkTreeModel *model,
                                GtkTreeIter  *iter)
{
  GtkTreeModelFilter *filter = (GtkTreeModelFilter *)model;
  GtkTreeIter child_iter;
  FilterLevel *level;
  FilterElt *elt;

  g_return_if_fail (GTK_IS_TREE_MODEL_FILTER (model));
  g_return_if_fail (GTK_TREE_MODEL_FILTER (model)->priv->child_model != NULL);
  g_return_if_fail (GTK_TREE_MODEL_FILTER (model)->priv->stamp == iter->stamp);

  gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model), &child_iter, iter);

  gtk_tree_model_ref_node (filter->priv->child_model, &child_iter);

  level = iter->user_data;
  elt = iter->user_data2;

  elt->ref_count++;
  level->ref_count++;
  if (level->ref_count == 1)
    {
      FilterLevel *parent_level = level->parent_level;
      FilterElt *parent_elt = level->parent_elt;

      /* we were at zero -- time to decrease the zero_ref_count val */
      while (parent_level)
        {
	  parent_elt->zero_ref_count--;

	  parent_elt = parent_level->parent_elt;
	  parent_level = parent_level->parent_level;
        }

      if (filter->priv->root != level)
	filter->priv->zero_ref_count--;
    }
}

static void
gtk_tree_model_filter_unref_node (GtkTreeModel *model,
                                  GtkTreeIter  *iter)
{
  gtk_tree_model_filter_real_unref_node (model, iter, TRUE);
}

static void
gtk_tree_model_filter_real_unref_node (GtkTreeModel *model,
                                       GtkTreeIter  *iter,
                                       gboolean      propagate_unref)
{
  GtkTreeModelFilter *filter = (GtkTreeModelFilter *)model;
  FilterLevel *level;
  FilterElt *elt;

  g_return_if_fail (GTK_IS_TREE_MODEL_FILTER (model));
  g_return_if_fail (filter->priv->child_model != NULL);
  g_return_if_fail (filter->priv->stamp == iter->stamp);

  if (propagate_unref)
    {
      GtkTreeIter child_iter;
      gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model), &child_iter, iter);
      gtk_tree_model_unref_node (filter->priv->child_model, &child_iter);
    }

  level = iter->user_data;
  elt = iter->user_data2;

  g_return_if_fail (elt->ref_count > 0);

  elt->ref_count--;
  level->ref_count--;
  if (level->ref_count == 0)
    {
      FilterLevel *parent_level = level->parent_level;
      FilterElt *parent_elt = level->parent_elt;

      /* we are at zero -- time to increase the zero_ref_count val */
      while (parent_level)
        {
          parent_elt->zero_ref_count++;

          parent_elt = parent_level->parent_elt;
          parent_level = parent_level->parent_level;
        }

      if (filter->priv->root != level)
	filter->priv->zero_ref_count++;
    }
}

/* TreeDragSource interface implementation */
static gboolean
gtk_tree_model_filter_row_draggable (GtkTreeDragSource *drag_source,
                                     GtkTreePath       *path)
{
  GtkTreeModelFilter *tree_model_filter = (GtkTreeModelFilter *)drag_source;
  GtkTreePath *child_path;
  gboolean draggable;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (drag_source), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  child_path = gtk_tree_model_filter_convert_path_to_child_path (tree_model_filter, path);
  draggable = gtk_tree_drag_source_row_draggable (GTK_TREE_DRAG_SOURCE (tree_model_filter->priv->child_model), child_path);
  gtk_tree_path_free (child_path);

  return draggable;
}

static gboolean
gtk_tree_model_filter_drag_data_get (GtkTreeDragSource *drag_source,
                                     GtkTreePath       *path,
                                     GtkSelectionData  *selection_data)
{
  GtkTreeModelFilter *tree_model_filter = (GtkTreeModelFilter *)drag_source;
  GtkTreePath *child_path;
  gboolean gotten;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (drag_source), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  child_path = gtk_tree_model_filter_convert_path_to_child_path (tree_model_filter, path);
  gotten = gtk_tree_drag_source_drag_data_get (GTK_TREE_DRAG_SOURCE (tree_model_filter->priv->child_model), child_path, selection_data);
  gtk_tree_path_free (child_path);

  return gotten;
}

static gboolean
gtk_tree_model_filter_drag_data_delete (GtkTreeDragSource *drag_source,
                                        GtkTreePath       *path)
{
  GtkTreeModelFilter *tree_model_filter = (GtkTreeModelFilter *)drag_source;
  GtkTreePath *child_path;
  gboolean deleted;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (drag_source), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  child_path = gtk_tree_model_filter_convert_path_to_child_path (tree_model_filter, path);
  deleted = gtk_tree_drag_source_drag_data_delete (GTK_TREE_DRAG_SOURCE (tree_model_filter->priv->child_model), child_path);
  gtk_tree_path_free (child_path);

  return deleted;
}

/* TreeDragSource interface implementation */
static gboolean
gtk_tree_model_filter_drag_data_received (GtkTreeDragDest  *drag_dest,
					  GtkTreePath      *dest_path,
					  GtkSelectionData *selection_data)
{
  GtkTreeModelFilter *tree_model_filter = (GtkTreeModelFilter *)drag_dest;
  GtkTreePath *child_path;
  gboolean received;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (drag_dest), FALSE);
  g_return_val_if_fail (dest_path != NULL, FALSE);

  child_path = gtk_tree_model_filter_convert_path_to_child_path (tree_model_filter, dest_path);
  if (!child_path)
    {
      GtkTreePath *parent_path;

      parent_path = gtk_tree_path_copy (dest_path);
      if (gtk_tree_path_up (parent_path)
	  && gtk_tree_path_get_depth (parent_path) > 0)
        {
	  GtkTreeIter iter;

	  /* Check if the parent exists */
	  received = gtk_tree_model_get_iter (GTK_TREE_MODEL (drag_dest),
					      &iter, parent_path);
	  g_assert (received != FALSE);

	  if (gtk_tree_model_iter_has_child (GTK_TREE_MODEL (drag_dest), &iter))
	    {
	      gint n;
	      GtkTreeIter child_iter;

	      /* Parent has children, so the drop was after the last node
	       * in parent's child level.
	       */

	      gtk_tree_model_filter_convert_iter_to_child_iter (tree_model_filter, &child_iter, &iter);
	      n = gtk_tree_model_iter_n_children (tree_model_filter->priv->child_model, &child_iter);

	      child_path = gtk_tree_model_filter_convert_path_to_child_path (tree_model_filter, parent_path);
	      gtk_tree_path_append_index (child_path, n);
	    }
	  else
	    {
	      /* Parent does not have children: this was a drop on
	       * the parent.
	       */
	      child_path = gtk_tree_model_filter_convert_path_to_child_path (tree_model_filter, parent_path);
	      gtk_tree_path_append_index (child_path, 0);
	    }

	  if (gtk_tree_path_get_depth (child_path) > 1
	      && (tree_model_filter->priv->child_flags & GTK_TREE_MODEL_LIST_ONLY))
	    {
	      gtk_tree_path_free (child_path);
	      gtk_tree_path_free (parent_path);
	      return FALSE;
	    }

	  received = gtk_tree_drag_dest_drag_data_received (GTK_TREE_DRAG_DEST (tree_model_filter->priv->child_model), child_path, selection_data);
	  gtk_tree_path_free (child_path);
	  gtk_tree_path_free (parent_path);
	  return received;
	}
      else
        {
	  gint n;

	  /* There is no parent, check root level */
	  gtk_tree_path_free (parent_path);

	  n = gtk_tree_model_iter_n_children (tree_model_filter->priv->child_model, NULL);

	  child_path = gtk_tree_path_new_from_indices (n, -1);
	  received = gtk_tree_drag_dest_drag_data_received (GTK_TREE_DRAG_DEST (tree_model_filter->priv->child_model), child_path, selection_data);
	  gtk_tree_path_free (child_path);

	  return received;
	}

      g_assert_not_reached ();
    }

  received = gtk_tree_drag_dest_drag_data_received (GTK_TREE_DRAG_DEST (tree_model_filter->priv->child_model), child_path, selection_data);
  gtk_tree_path_free (child_path);

  return received;
}

static gboolean
gtk_tree_model_filter_row_drop_possible (GtkTreeDragDest  *drag_dest,
					 GtkTreePath      *dest_path,
					 GtkSelectionData *selection_data)
{
  GtkTreeModelFilter *tree_model_filter = (GtkTreeModelFilter *)drag_dest;
  GtkTreePath *child_path;
  gboolean possible;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (drag_dest), FALSE);
  g_return_val_if_fail (dest_path != NULL, FALSE);

  child_path = gtk_tree_model_filter_convert_path_to_child_path (tree_model_filter, dest_path);

  if (!child_path)
    {
      GtkTreePath *parent_path;

      parent_path = gtk_tree_path_copy (dest_path);
      if (gtk_tree_path_up (parent_path)
	  && gtk_tree_path_get_depth (parent_path) > 0)
        {
	  GtkTreeIter iter;

	  /* Check if the parent exists */
	  possible = gtk_tree_model_get_iter (GTK_TREE_MODEL (drag_dest),
					      &iter, parent_path);
	  g_assert (possible != FALSE);

	  if (gtk_tree_model_iter_has_child (GTK_TREE_MODEL (drag_dest), &iter))
	    {
	      gint n;
	      GtkTreeIter child_iter;

	      /* Parent has children, so the drop was after the last node
	       * in parent's child level.
	       */

	      gtk_tree_model_filter_convert_iter_to_child_iter (tree_model_filter, &child_iter, &iter);
	      n = gtk_tree_model_iter_n_children (tree_model_filter->priv->child_model, &child_iter);

	      child_path = gtk_tree_model_filter_convert_path_to_child_path (tree_model_filter, parent_path);
	      gtk_tree_path_append_index (child_path, n);
	    }
	  else
	    {
	      /* Parent does not have children: this was a drop on
	       * the parent.
	       */
	      child_path = gtk_tree_model_filter_convert_path_to_child_path (tree_model_filter, parent_path);
	      gtk_tree_path_append_index (child_path, 0);
	    }

	  if (gtk_tree_path_get_depth (child_path) > 1
	      && (tree_model_filter->priv->child_flags & GTK_TREE_MODEL_LIST_ONLY))
	    {
	      gtk_tree_path_free (child_path);
	      gtk_tree_path_free (parent_path);
	      return FALSE;
	    }

	  possible = gtk_tree_drag_dest_row_drop_possible (GTK_TREE_DRAG_DEST (tree_model_filter->priv->child_model), child_path, selection_data);
	  gtk_tree_path_free (child_path);
	  gtk_tree_path_free (parent_path);
	  return possible;
	}
      else
        {
	  gint n;

	  /* There is no parent, check root level */
	  gtk_tree_path_free (parent_path);

	  n = gtk_tree_model_iter_n_children (tree_model_filter->priv->child_model, NULL);

	  child_path = gtk_tree_path_new_from_indices (n, -1);
	  possible = gtk_tree_drag_dest_row_drop_possible (GTK_TREE_DRAG_DEST (tree_model_filter->priv->child_model), child_path, selection_data);
	  gtk_tree_path_free (child_path);

	  return possible;
	}

      g_assert_not_reached ();
    }

  possible = gtk_tree_drag_dest_row_drop_possible (GTK_TREE_DRAG_DEST (tree_model_filter->priv->child_model), child_path, selection_data);
  gtk_tree_path_free (child_path);

  return possible;
}

/* bits and pieces */
static void
gtk_tree_model_filter_set_model (GtkTreeModelFilter *filter,
                                 GtkTreeModel       *child_model)
{
  g_return_if_fail (GTK_IS_TREE_MODEL_FILTER (filter));

  if (filter->priv->child_model)
    {
      g_signal_handler_disconnect (filter->priv->child_model,
                                   filter->priv->changed_id);
      g_signal_handler_disconnect (filter->priv->child_model,
                                   filter->priv->inserted_id);
      g_signal_handler_disconnect (filter->priv->child_model,
                                   filter->priv->has_child_toggled_id);
      g_signal_handler_disconnect (filter->priv->child_model,
                                   filter->priv->deleted_id);
      g_signal_handler_disconnect (filter->priv->child_model,
                                   filter->priv->reordered_id);

      /* reset our state */
      if (filter->priv->root)
        gtk_tree_model_filter_free_level (filter, filter->priv->root);

      filter->priv->root = NULL;
      g_object_unref (filter->priv->child_model);
      filter->priv->visible_column = -1;

      /* FIXME: do we need to destroy more here? */
    }

  filter->priv->child_model = child_model;

  if (child_model)
    {
      g_object_ref (filter->priv->child_model);
      filter->priv->changed_id =
        g_signal_connect (child_model, "row-changed",
                          G_CALLBACK (gtk_tree_model_filter_row_changed),
                          filter);
      filter->priv->inserted_id =
        g_signal_connect (child_model, "row-inserted",
                          G_CALLBACK (gtk_tree_model_filter_row_inserted),
                          filter);
      filter->priv->has_child_toggled_id =
        g_signal_connect (child_model, "row-has-child-toggled",
                          G_CALLBACK (gtk_tree_model_filter_row_has_child_toggled),
                          filter);
      filter->priv->deleted_id =
        g_signal_connect (child_model, "row-deleted",
                          G_CALLBACK (gtk_tree_model_filter_row_deleted),
                          filter);
      filter->priv->reordered_id =
        g_signal_connect (child_model, "rows-reordered",
                          G_CALLBACK (gtk_tree_model_filter_rows_reordered),
                          filter);

      filter->priv->child_flags = gtk_tree_model_get_flags (child_model);
      filter->priv->stamp = g_random_int ();
    }
}

static void
gtk_tree_model_filter_ref_path (GtkTreeModelFilter *filter,
                                GtkTreePath        *path)
{
  int len;
  GtkTreePath *p;

  len = gtk_tree_path_get_depth (path);
  p = gtk_tree_path_copy (path);
  while (len--)
    {
      GtkTreeIter iter;

      gtk_tree_model_get_iter (GTK_TREE_MODEL (filter->priv->child_model), &iter, p);
      gtk_tree_model_ref_node (GTK_TREE_MODEL (filter->priv->child_model), &iter);
      gtk_tree_path_up (p);
    }

  gtk_tree_path_free (p);
}

static void
gtk_tree_model_filter_unref_path (GtkTreeModelFilter *filter,
                                  GtkTreePath        *path)
{
  int len;
  GtkTreePath *p;

  len = gtk_tree_path_get_depth (path);
  p = gtk_tree_path_copy (path);
  while (len--)
    {
      GtkTreeIter iter;

      gtk_tree_model_get_iter (GTK_TREE_MODEL (filter->priv->child_model), &iter, p);
      gtk_tree_model_unref_node (GTK_TREE_MODEL (filter->priv->child_model), &iter);
      gtk_tree_path_up (p);
    }

  gtk_tree_path_free (p);
}

static void
gtk_tree_model_filter_set_root (GtkTreeModelFilter *filter,
                                GtkTreePath        *root)
{
  g_return_if_fail (GTK_IS_TREE_MODEL_FILTER (filter));

  if (!root)
    filter->priv->virtual_root = NULL;
  else
    filter->priv->virtual_root = gtk_tree_path_copy (root);
}

/* public API */

/**
 * gtk_tree_model_filter_new:
 * @child_model: A #GtkTreeModel.
 * @root: A #GtkTreePath or %NULL.
 *
 * Creates a new #GtkTreeModel, with @child_model as the child_model
 * and @root as the virtual root.
 *
 * Return value: A new #GtkTreeModel.
 *
 * Since: 2.4
 */
GtkTreeModel *
gtk_tree_model_filter_new (GtkTreeModel *child_model,
                           GtkTreePath  *root)
{
  GtkTreeModel *retval;
  GtkTreeModelFilter *filter;

  g_return_val_if_fail (GTK_IS_TREE_MODEL (child_model), NULL);

  retval = g_object_new (GTK_TYPE_TREE_MODEL_FILTER, 
			 "child-model", child_model,
			 "virtual-root", root,
			 NULL);

  filter = GTK_TREE_MODEL_FILTER (retval);
  if (filter->priv->virtual_root)
    {
      gtk_tree_model_filter_ref_path (filter, filter->priv->virtual_root);
      filter->priv->virtual_root_deleted = FALSE;
    }

  return retval;
}

/**
 * gtk_tree_model_filter_get_model:
 * @filter: A #GtkTreeModelFilter.
 *
 * Returns a pointer to the child model of @filter.
 *
 * Return value: A pointer to a #GtkTreeModel.
 *
 * Since: 2.4
 */
GtkTreeModel *
gtk_tree_model_filter_get_model (GtkTreeModelFilter *filter)
{
  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (filter), NULL);

  return filter->priv->child_model;
}

/**
 * gtk_tree_model_filter_set_visible_func:
 * @filter: A #GtkTreeModelFilter.
 * @func: A #GtkTreeModelFilterVisibleFunc, the visible function.
 * @data: User data to pass to the visible function, or %NULL.
 * @destroy: Destroy notifier of @data, or %NULL.
 *
 * Sets the visible function used when filtering the @filter to be @func. The
 * function should return %TRUE if the given row should be visible and
 * %FALSE otherwise.
 *
 * If the condition calculated by the function changes over time (e.g. because
 * it depends on some global parameters), you must call 
 * gtk_tree_model_filter_refilter() to keep the visibility information of 
 * the model uptodate.
 *
 * Note that @func is called whenever a row is inserted, when it may still be
 * empty. The visible function should therefore take special care of empty
 * rows, like in the example below.
 *
 * <informalexample><programlisting>
 * static gboolean
 * visible_func (GtkTreeModel *model,
 *               GtkTreeIter  *iter,
 *               gpointer      data)
 * {
 *   /&ast; Visible if row is non-empty and first column is "HI" &ast;/
 *   gchar *str;
 *   gboolean visible = FALSE;
 *
 *   gtk_tree_model_get (model, iter, 0, &str, -1);
 *   if (str && strcmp (str, "HI") == 0)
 *     visible = TRUE;
 *   g_free (str);
 *
 *   return visible;
 * }
 * </programlisting></informalexample>
 *
 * Since: 2.4
 */
void
gtk_tree_model_filter_set_visible_func (GtkTreeModelFilter            *filter,
                                        GtkTreeModelFilterVisibleFunc  func,
                                        gpointer                       data,
                                        GDestroyNotify                 destroy)
{
  g_return_if_fail (GTK_IS_TREE_MODEL_FILTER (filter));
  g_return_if_fail (func != NULL);
  g_return_if_fail (filter->priv->visible_method_set == FALSE);

  if (filter->priv->visible_func)
    {
      GDestroyNotify d = filter->priv->visible_destroy;

      filter->priv->visible_destroy = NULL;
      d (filter->priv->visible_data);
    }

  filter->priv->visible_func = func;
  filter->priv->visible_data = data;
  filter->priv->visible_destroy = destroy;

  filter->priv->visible_method_set = TRUE;
}

/**
 * gtk_tree_model_filter_set_modify_func:
 * @filter: A #GtkTreeModelFilter.
 * @n_columns: The number of columns in the filter model.
 * @types: The #GType<!-- -->s of the columns.
 * @func: A #GtkTreeModelFilterModifyFunc
 * @data: User data to pass to the modify function, or %NULL.
 * @destroy: Destroy notifier of @data, or %NULL.
 *
 * With the @n_columns and @types parameters, you give an array of column
 * types for this model (which will be exposed to the parent model/view).
 * The @func, @data and @destroy parameters are for specifying the modify
 * function. The modify function will get called for <emphasis>each</emphasis>
 * data access, the goal of the modify function is to return the data which 
 * should be displayed at the location specified using the parameters of the 
 * modify function.
 *
 * Since: 2.4
 */
void
gtk_tree_model_filter_set_modify_func (GtkTreeModelFilter           *filter,
                                       gint                          n_columns,
                                       GType                        *types,
                                       GtkTreeModelFilterModifyFunc  func,
                                       gpointer                      data,
                                       GDestroyNotify                destroy)
{
  g_return_if_fail (GTK_IS_TREE_MODEL_FILTER (filter));
  g_return_if_fail (func != NULL);
  g_return_if_fail (filter->priv->modify_func_set == FALSE);

  if (filter->priv->modify_destroy)
    {
      GDestroyNotify d = filter->priv->modify_destroy;

      filter->priv->modify_destroy = NULL;
      d (filter->priv->modify_data);
    }

  filter->priv->modify_n_columns = n_columns;
  filter->priv->modify_types = g_new0 (GType, n_columns);
  memcpy (filter->priv->modify_types, types, sizeof (GType) * n_columns);
  filter->priv->modify_func = func;
  filter->priv->modify_data = data;
  filter->priv->modify_destroy = destroy;

  filter->priv->modify_func_set = TRUE;
}

/**
 * gtk_tree_model_filter_set_visible_column:
 * @filter: A #GtkTreeModelFilter.
 * @column: A #gint which is the column containing the visible information.
 *
 * Sets @column of the child_model to be the column where @filter should
 * look for visibility information. @columns should be a column of type
 * %G_TYPE_BOOLEAN, where %TRUE means that a row is visible, and %FALSE
 * if not.
 *
 * Since: 2.4
 */
void
gtk_tree_model_filter_set_visible_column (GtkTreeModelFilter *filter,
                                          gint column)
{
  g_return_if_fail (GTK_IS_TREE_MODEL_FILTER (filter));
  g_return_if_fail (column >= 0);
  g_return_if_fail (filter->priv->visible_method_set == FALSE);

  filter->priv->visible_column = column;

  filter->priv->visible_method_set = TRUE;
}

/* conversion */

/**
 * gtk_tree_model_filter_convert_child_iter_to_iter:
 * @filter: A #GtkTreeModelFilter.
 * @filter_iter: An uninitialized #GtkTreeIter.
 * @child_iter: A valid #GtkTreeIter pointing to a row on the child model.
 *
 * Sets @filter_iter to point to the row in @filter that corresponds to the
 * row pointed at by @child_iter.  If @filter_iter was not set, %FALSE is
 * returned.
 *
 * Return value: %TRUE, if @filter_iter was set, i.e. if @child_iter is a
 * valid iterator pointing to a visible row in child model.
 *
 * Since: 2.4
 */
gboolean
gtk_tree_model_filter_convert_child_iter_to_iter (GtkTreeModelFilter *filter,
                                                  GtkTreeIter        *filter_iter,
                                                  GtkTreeIter        *child_iter)
{
  gboolean ret;
  GtkTreePath *child_path, *path;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (filter), FALSE);
  g_return_val_if_fail (filter->priv->child_model != NULL, FALSE);
  g_return_val_if_fail (filter_iter != NULL, FALSE);
  g_return_val_if_fail (child_iter != NULL, FALSE);

  filter_iter->stamp = 0;

  child_path = gtk_tree_model_get_path (filter->priv->child_model, child_iter);
  g_return_val_if_fail (child_path != NULL, FALSE);

  path = gtk_tree_model_filter_convert_child_path_to_path (filter,
                                                           child_path);
  gtk_tree_path_free (child_path);

  if (!path)
    return FALSE;

  ret = gtk_tree_model_get_iter (GTK_TREE_MODEL (filter), filter_iter, path);
  gtk_tree_path_free (path);

  return ret;
}

/**
 * gtk_tree_model_filter_convert_iter_to_child_iter:
 * @filter: A #GtkTreeModelFilter.
 * @child_iter: An uninitialized #GtkTreeIter.
 * @filter_iter: A valid #GtkTreeIter pointing to a row on @filter.
 *
 * Sets @child_iter to point to the row pointed to by @filter_iter.
 *
 * Since: 2.4
 */
void
gtk_tree_model_filter_convert_iter_to_child_iter (GtkTreeModelFilter *filter,
                                                  GtkTreeIter        *child_iter,
                                                  GtkTreeIter        *filter_iter)
{
  g_return_if_fail (GTK_IS_TREE_MODEL_FILTER (filter));
  g_return_if_fail (filter->priv->child_model != NULL);
  g_return_if_fail (child_iter != NULL);
  g_return_if_fail (filter_iter != NULL);
  g_return_if_fail (filter_iter->stamp == filter->priv->stamp);

  if (GTK_TREE_MODEL_FILTER_CACHE_CHILD_ITERS (filter))
    {
      *child_iter = FILTER_ELT (filter_iter->user_data2)->iter;
    }
  else
    {
      GtkTreePath *path;

      path = gtk_tree_model_filter_elt_get_path (filter_iter->user_data,
                                                 filter_iter->user_data2,
                                                 filter->priv->virtual_root);
      gtk_tree_model_get_iter (filter->priv->child_model, child_iter, path);
      gtk_tree_path_free (path);
    }
}

/* The path returned can only be used internally in the filter model. */
static GtkTreePath *
gtk_real_tree_model_filter_convert_child_path_to_path (GtkTreeModelFilter *filter,
                                                       GtkTreePath        *child_path,
                                                       gboolean            build_levels,
                                                       gboolean            fetch_children)
{
  gint *child_indices;
  GtkTreePath *retval;
  GtkTreePath *real_path;
  FilterLevel *level;
  FilterElt *tmp;
  gint i;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (filter), NULL);
  g_return_val_if_fail (filter->priv->child_model != NULL, NULL);
  g_return_val_if_fail (child_path != NULL, NULL);

  if (!filter->priv->virtual_root)
    real_path = gtk_tree_path_copy (child_path);
  else
    real_path = gtk_tree_model_filter_remove_root (child_path,
                                                   filter->priv->virtual_root);

  if (!real_path)
    return NULL;

  retval = gtk_tree_path_new ();
  child_indices = gtk_tree_path_get_indices (real_path);

  if (filter->priv->root == NULL && build_levels)
    gtk_tree_model_filter_build_level (filter, NULL, NULL, FALSE);
  level = FILTER_LEVEL (filter->priv->root);

  for (i = 0; i < gtk_tree_path_get_depth (real_path); i++)
    {
      GSequenceIter *siter;
      gboolean found_child = FALSE;

      if (!level)
        {
          gtk_tree_path_free (real_path);
          gtk_tree_path_free (retval);
          return NULL;
        }

      tmp = bsearch_elt_with_offset (level->seq, child_indices[i], &siter);
      if (tmp)
        {
          gtk_tree_path_append_index (retval, g_sequence_iter_get_position (siter));
          if (!tmp->children && build_levels)
            gtk_tree_model_filter_build_level (filter, level, tmp, FALSE);
          level = tmp->children;
          found_child = TRUE;
        }

      if (!found_child && fetch_children)
        {
          int j;

          tmp = gtk_tree_model_filter_fetch_child (filter, level,
                                                   child_indices[i],
                                                   &j);

          /* didn't find the child, let's try to bring it back */
          if (!tmp || tmp->offset != child_indices[i])
            {
              /* not there */
              gtk_tree_path_free (real_path);
              gtk_tree_path_free (retval);
              return NULL;
            }

          gtk_tree_path_append_index (retval, j);
          if (!tmp->children && build_levels)
            gtk_tree_model_filter_build_level (filter, level, tmp, FALSE);
          level = tmp->children;
          found_child = TRUE;
        }
      else if (!found_child && !fetch_children)
        {
          /* no path */
          gtk_tree_path_free (real_path);
          gtk_tree_path_free (retval);
          return NULL;
        }
    }

  gtk_tree_path_free (real_path);
  return retval;
}

/**
 * gtk_tree_model_filter_convert_child_path_to_path:
 * @filter: A #GtkTreeModelFilter.
 * @child_path: A #GtkTreePath to convert.
 *
 * Converts @child_path to a path relative to @filter. That is, @child_path
 * points to a path in the child model. The rerturned path will point to the
 * same row in the filtered model. If @child_path isn't a valid path on the
 * child model or points to a row which is not visible in @filter, then %NULL
 * is returned.
 *
 * Return value: A newly allocated #GtkTreePath, or %NULL.
 *
 * Since: 2.4
 */
GtkTreePath *
gtk_tree_model_filter_convert_child_path_to_path (GtkTreeModelFilter *filter,
                                                  GtkTreePath        *child_path)
{
  GtkTreeIter iter;
  GtkTreePath *path;

  /* this function does the sanity checks */
  path = gtk_real_tree_model_filter_convert_child_path_to_path (filter,
                                                                child_path,
                                                                TRUE,
                                                                TRUE);

  if (!path)
      return NULL;

  /* get a new path which only takes visible nodes into account.
   * -- if this gives any performance issues, we can write a special
   *    version of convert_child_path_to_path immediately returning
   *    a visible-nodes-only path.
   */
  gtk_tree_model_filter_get_iter_full (GTK_TREE_MODEL (filter), &iter, path);

  gtk_tree_path_free (path);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (filter), &iter);

  return path;
}

/**
 * gtk_tree_model_filter_convert_path_to_child_path:
 * @filter: A #GtkTreeModelFilter.
 * @filter_path: A #GtkTreePath to convert.
 *
 * Converts @filter_path to a path on the child model of @filter. That is,
 * @filter_path points to a location in @filter. The returned path will
 * point to the same location in the model not being filtered. If @filter_path
 * does not point to a location in the child model, %NULL is returned.
 *
 * Return value: A newly allocated #GtkTreePath, or %NULL.
 *
 * Since: 2.4
 */
GtkTreePath *
gtk_tree_model_filter_convert_path_to_child_path (GtkTreeModelFilter *filter,
                                                  GtkTreePath        *filter_path)
{
  gint *filter_indices;
  GtkTreePath *retval;
  FilterLevel *level;
  gint i;

  g_return_val_if_fail (GTK_IS_TREE_MODEL_FILTER (filter), NULL);
  g_return_val_if_fail (filter->priv->child_model != NULL, NULL);
  g_return_val_if_fail (filter_path != NULL, NULL);

  /* convert path */
  retval = gtk_tree_path_new ();
  filter_indices = gtk_tree_path_get_indices (filter_path);
  if (!filter->priv->root)
    gtk_tree_model_filter_build_level (filter, NULL, NULL, FALSE);
  level = FILTER_LEVEL (filter->priv->root);

  for (i = 0; i < gtk_tree_path_get_depth (filter_path); i++)
    {
      FilterElt *elt;
      GSequenceIter *siter;

      if (!level)
        {
          gtk_tree_path_free (retval);
          return NULL;
        }

      siter = g_sequence_get_iter_at_pos (level->visible_seq, filter_indices[i]);
      if (g_sequence_iter_is_end (siter))
        {
          gtk_tree_path_free (retval);
          return NULL;
        }

      elt = GET_ELT (siter);
      if (elt->children == NULL)
        gtk_tree_model_filter_build_level (filter, level, elt, FALSE);

      gtk_tree_path_append_index (retval, elt->offset);
      level = elt->children;
    }

  /* apply vroot */

  if (filter->priv->virtual_root)
    {
      GtkTreePath *real_retval;

      real_retval = gtk_tree_model_filter_add_root (retval,
                                                    filter->priv->virtual_root);
      gtk_tree_path_free (retval);

      return real_retval;
    }

  return retval;
}

static gboolean
gtk_tree_model_filter_refilter_helper (GtkTreeModel *model,
                                       GtkTreePath  *path,
                                       GtkTreeIter  *iter,
                                       gpointer      data)
{
  /* evil, don't try this at home, but certainly speeds things up */
  gtk_tree_model_filter_row_changed (model, path, iter, data);

  return FALSE;
}

/**
 * gtk_tree_model_filter_refilter:
 * @filter: A #GtkTreeModelFilter.
 *
 * Emits ::row_changed for each row in the child model, which causes
 * the filter to re-evaluate whether a row is visible or not.
 *
 * Since: 2.4
 */
void
gtk_tree_model_filter_refilter (GtkTreeModelFilter *filter)
{
  g_return_if_fail (GTK_IS_TREE_MODEL_FILTER (filter));

  /* S L O W */
  gtk_tree_model_foreach (filter->priv->child_model,
                          gtk_tree_model_filter_refilter_helper,
                          filter);
}

/**
 * gtk_tree_model_filter_clear_cache:
 * @filter: A #GtkTreeModelFilter.
 *
 * This function should almost never be called. It clears the @filter
 * of any cached iterators that haven't been reffed with
 * gtk_tree_model_ref_node(). This might be useful if the child model
 * being filtered is static (and doesn't change often) and there has been
 * a lot of unreffed access to nodes. As a side effect of this function,
 * all unreffed iters will be invalid.
 *
 * Since: 2.4
 */
void
gtk_tree_model_filter_clear_cache (GtkTreeModelFilter *filter)
{
  g_return_if_fail (GTK_IS_TREE_MODEL_FILTER (filter));

  if (filter->priv->zero_ref_count > 0)
    gtk_tree_model_filter_clear_cache_helper (filter,
                                              FILTER_LEVEL (filter->priv->root));
}

#define __GTK_TREE_MODEL_FILTER_C__
#include "gtkaliasdef.c"
