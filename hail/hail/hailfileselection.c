/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2005, 2006 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:hailfileselection
 * @short_description: Implementation of the ATK interfaces for #HildonFileSelection.
 * @see_also: #GailContainer, #HildonFileSelection
 *
 * #HailFileSelection implements the required ATK interfaces of #HildonFileSelection and its
 * children (the buttons inside the widget). It exposes:
 * <itemizedlist>
 * <listitem>The folder treeview, as its gail default implementation. The 
 * <literal>activate</literal> signal in the cells has an implementation which selects
 * another folder and shows it.</listitem>
 * <listitem>The notebook holding the different views.</listitem>
 * <listitem>The actions to get the contextual menus</listitem>
 * </itemizedlist>
 */

#include <gtk/gtkbin.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkhpaned.h>
#include <hildon-widgets/hildon-file-selection.h>
#include "hailfileselection.h"

#define HAIL_FILE_SELECTION_DEFAULT_NAME "File selection"

typedef struct _HailFileSelectionPrivate HailFileSelectionPrivate;

struct _HailFileSelectionPrivate {
  AtkObject * folder_tree;        /* the folder treeview accessible object */
  AtkObject * views_notebook;     /* the notebook holds the 3 views available (list, 
				   *  thumbnail and no view */
  AtkObject * hpaned;             /* the pane in hpaned mode */
  gboolean    hpaned_mode;        /* TRUE if the widget is in paned mode */
};

#define HAIL_FILE_SELECTION_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_FILE_SELECTION, HailFileSelectionPrivate))

static void                  hail_file_selection_class_init       (HailFileSelectionClass *klass);
static void                  hail_file_selection_object_init      (HailFileSelection      *file_selection);

static G_CONST_RETURN gchar* hail_file_selection_get_name         (AtkObject       *obj);
static gint                  hail_file_selection_get_n_children   (AtkObject       *obj);
static AtkObject*            hail_file_selection_ref_child        (AtkObject       *obj,
							       gint            i);
static void                  hail_file_selection_real_initialize  (AtkObject       *obj,
							    gpointer        data);

/* AtkAction.h */
static void                  hail_file_selection_atk_action_interface_init   (AtkActionIface  *iface);
static gint                  hail_file_selection_action_get_n_actions (AtkAction       *action);
static gboolean              hail_file_selection_action_do_action    (AtkAction       *action,
								      gint            index);
static const gchar*          hail_file_selection_action_get_name     (AtkAction       *action,
								      gint            index);
static const gchar*          hail_file_selection_action_get_description (AtkAction       *action,
									 gint            index);
static const gchar*          hail_file_selection_action_get_keybinding  (AtkAction       *action,
									 gint            index);

static void                  add_internal_widgets (GtkWidget * widget,
						   gpointer data);
static void hail_file_selection_folders_tree_row_activated (GtkTreeView *tree_view,
							    GtkTreePath *path,
							    GtkTreeViewColumn *column,
							    gpointer user_data);

#define HAIL_FILE_SELECTION_TREE_CONTEXT_ACTION_NAME "tree context menu"
#define HAIL_FILE_SELECTION_CONTENT_CONTEXT_ACTION_NAME "files context menu"
#define HAIL_FILE_SELECTION_TREE_CONTEXT_ACTION_DESCRIPTION "Show the context menu for the folder tree"
#define HAIL_FILE_SELECTION_CONTENT_CONTEXT_ACTION_DESCRIPTION "Show the context menu for the files list"

static GType parent_type;
static GtkAccessible* parent_class = NULL;

/**
 * hail_file_selection_get_type:
 * 
 * Initialises, and returns the type of a hail file_selection.
 * 
 * Return value: GType of #HailFileSelection.
 **/
GType
hail_file_selection_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailFileSelectionClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) hail_file_selection_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        (guint16) sizeof (HailFileSelection), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) hail_file_selection_object_init, /* instance init */
        NULL /* value table */
      };

      static const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) hail_file_selection_atk_action_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_CONTAINER);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailFileSelection", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);
    }

  return type;
}

static void
hail_file_selection_class_init (HailFileSelectionClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailFileSelectionPrivate));

  /* bind virtual methods for AtkObject */
  class->get_name = hail_file_selection_get_name;
  class->get_n_children = hail_file_selection_get_n_children;
  class->ref_child = hail_file_selection_ref_child;
  class->initialize = hail_file_selection_real_initialize;

}

static void
hail_file_selection_object_init (HailFileSelection      *file_selection)
{
  HailFileSelectionPrivate * priv = NULL;

  priv = HAIL_FILE_SELECTION_GET_PRIVATE(file_selection);

  priv->folder_tree = NULL;
  priv->views_notebook = NULL;
  priv->hpaned = NULL;
  priv->hpaned_mode = FALSE;
}


/**
 * hail_file_selection_new:
 * @widget: a #HildonFileSelection casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonFileSelection.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_file_selection_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_FILE_SELECTION (widget), NULL);

  object = g_object_new (HAIL_TYPE_FILE_SELECTION, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_file_selection_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_FILE_SELECTION (obj), NULL);

  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);
  if (name == NULL)
    {
      /*
       * Get the text on the label
       */
      GtkWidget *widget = NULL;

      widget = GTK_ACCESSIBLE (obj)->widget;
      if (widget == NULL)
        /*
         * State is defunct
         */
        return NULL;

      g_return_val_if_fail (HILDON_IS_FILE_SELECTION (widget), NULL);

      /* It gets the name from the file_selection label */
      name = HAIL_FILE_SELECTION_DEFAULT_NAME;

    }
  return name;
}

/* callback for gtk_container_forall. It's used from atk object initialize
 * to get the references to the AtkObjects of the internal widgets */
static void
add_internal_widgets (GtkWidget * widget,
		      gpointer data)
{
  HailFileSelection * file_selection = NULL;
  HailFileSelectionPrivate * priv = NULL;
  GtkWidget * folder_tree_view = NULL;

  g_return_if_fail (HAIL_IS_FILE_SELECTION(data));
  file_selection = HAIL_FILE_SELECTION(data);
  priv = HAIL_FILE_SELECTION_GET_PRIVATE(file_selection);
  /* if the child is a GtkHPaned then Hildon FM has been compiled
   * using HILDON_FM_HPANED, so we have to get the required children
   * from this.
   */
  if (GTK_IS_HPANED(widget)) {
    priv->hpaned_mode = TRUE;
    priv->hpaned = gtk_widget_get_accessible(widget);
    if (gtk_paned_get_child1(GTK_PANED(widget))) {
      priv->folder_tree = gtk_widget_get_accessible(gtk_paned_get_child1(GTK_PANED(widget)));
      folder_tree_view = gtk_bin_get_child(GTK_BIN(gtk_paned_get_child1(GTK_PANED(widget))));
    }
    if (gtk_paned_get_child2(GTK_PANED(widget))) {
      priv->views_notebook = gtk_widget_get_accessible(gtk_paned_get_child2(GTK_PANED(widget)));
    }
  } else  if (GTK_IS_SCROLLED_WINDOW(widget)) {
    priv->folder_tree = gtk_widget_get_accessible(widget);
    folder_tree_view = gtk_bin_get_child(GTK_BIN(widget));
  } else if (GTK_IS_NOTEBOOK(widget)) {
    priv->views_notebook = gtk_widget_get_accessible(widget);
  }

  if (GTK_IS_TREE_VIEW(folder_tree_view)) {
    g_signal_connect(folder_tree_view, "row-activated", 
		     (GCallback) hail_file_selection_folders_tree_row_activated,
		     file_selection);
  }

}


/* Implementation of AtkObject method initialize() */
static void
hail_file_selection_real_initialize (AtkObject *obj,
                             gpointer   data)
{
  GtkWidget * widget = NULL;
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_PANEL;

  widget = GTK_ACCESSIBLE(obj)->widget;
  gtk_container_forall(GTK_CONTAINER(widget), add_internal_widgets, obj);

}

/* Implementation of AtkObject method get_n_child() */
static gint                  
hail_file_selection_get_n_children   (AtkObject       *obj)
{
  GtkWidget *widget = NULL;
  HailFileSelectionPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_FILE_SELECTION (obj), 0);
  priv = HAIL_FILE_SELECTION_GET_PRIVATE(HAIL_FILE_SELECTION(obj));

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  /* check if file selection has been compiled with HILDON_FM_HPANED */
  if (priv->hpaned_mode)
  /* the hpaned */
    return 1;
  else 
  /* the folder treeview and the views notebook */
    return 2;
}

/* Implementation of AtkObject method ref_child() */
static AtkObject*            
hail_file_selection_ref_child        (AtkObject       *obj,
				      gint            i)
{
  GtkWidget * widget = NULL;
  AtkObject * accessible_child = NULL;
  HailFileSelectionPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_FILE_SELECTION (obj), 0);
  priv = HAIL_FILE_SELECTION_GET_PRIVATE(HAIL_FILE_SELECTION(obj));
  g_return_val_if_fail ((i >= 0), NULL);

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  if (priv->hpaned_mode) {
    switch (i) {
    case 0:
      accessible_child = priv->hpaned;
      break;
    default:
      return NULL;
    }
  } else {
    switch (i) {
    case 0: 
      accessible_child = priv->folder_tree;
      break;
    case 1:
      accessible_child = priv->views_notebook;
      break;
    default:
      return NULL;
      break;
    }
  }

  g_return_val_if_fail (ATK_IS_OBJECT(accessible_child), NULL);

  g_object_ref(accessible_child);
  return accessible_child;
}

/* Initializes the AtkAction interface, and binds the virtual methods */
static void
hail_file_selection_atk_action_interface_init (AtkActionIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->do_action = hail_file_selection_action_do_action;
  iface->get_n_actions = hail_file_selection_action_get_n_actions;
  iface->get_name = hail_file_selection_action_get_name;
  iface->get_description = hail_file_selection_action_get_description;
  iface->get_keybinding = hail_file_selection_action_get_keybinding;
}

/* Implementation of AtkAction method get_n_actions */
static gint
hail_file_selection_action_get_n_actions    (AtkAction       *action)
{
  GtkWidget * file_selection = NULL;
  
  g_return_val_if_fail (HAIL_IS_FILE_SELECTION (action), 0);

  file_selection = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_FILE_SELECTION(file_selection), 0);

  /* two context menu actions: one for the tree and another for the content panes */
  return 2;
}

/* Implementation of AtkAction method do_action */
static gboolean
hail_file_selection_action_do_action        (AtkAction       *action,
					     gint            index)
{
  GtkWidget * file_selection = NULL;
  HailFileSelectionPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_FILE_SELECTION (action), FALSE);
  g_return_val_if_fail ((index >= 0)&&(index < 2), FALSE);

  file_selection = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_FILE_SELECTION(file_selection), FALSE);

  priv = HAIL_FILE_SELECTION_GET_PRIVATE(file_selection);

  switch (index) {
  case 0:
    g_signal_emit_by_name(file_selection, "navigation-pane-context-menu");
    return TRUE;
    break;
  case 1:
    g_signal_emit_by_name(file_selection, "content-pane-context-menu");
    return TRUE;
    break;
  default:
    return FALSE;
  }
}

/* Implementation of AtkAction method get_name */
static const gchar*
hail_file_selection_action_get_name         (AtkAction       *action,
					     gint            index)
{
  GtkWidget * file_selection = NULL;

  g_return_val_if_fail (HAIL_IS_FILE_SELECTION (action), NULL);
  g_return_val_if_fail ((index >= 0)&&(index < 2), NULL);

  file_selection = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_FILE_SELECTION(file_selection), NULL);

  switch (index) {
  case 0:
    return HAIL_FILE_SELECTION_TREE_CONTEXT_ACTION_NAME;
    break;
  case 1:
    return HAIL_FILE_SELECTION_CONTENT_CONTEXT_ACTION_NAME;
    break;
  default:
    return NULL;
  }
}

/* Implementation of AtkAction method get_description */
static const gchar*
hail_file_selection_action_get_description   (AtkAction       *action,
					      gint            index)
{
  GtkWidget * file_selection = NULL;

  g_return_val_if_fail (HAIL_IS_FILE_SELECTION (action), NULL);
  g_return_val_if_fail ((index >= 0)&&(index < 2), NULL);

  file_selection = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_FILE_SELECTION(file_selection), NULL);

  switch (index) {
  case 0:
    return HAIL_FILE_SELECTION_TREE_CONTEXT_ACTION_DESCRIPTION;
    break;
  case 1:
    return HAIL_FILE_SELECTION_CONTENT_CONTEXT_ACTION_DESCRIPTION;
    break;
  default:
    return NULL;
  }
}

/* Implementation of AtkAction method get_keybinding */
static const gchar*
hail_file_selection_action_get_keybinding   (AtkAction       *action,
					  gint            index)
{
  GtkWidget * file_selection = NULL;

  g_return_val_if_fail (HAIL_IS_FILE_SELECTION (action), NULL);
  g_return_val_if_fail ((index >= 0)&&(index < 2), NULL);

  file_selection = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_FILE_SELECTION(file_selection), NULL);

  return NULL;
}

/* signal handler for row-activated signal in folder tree view. It should
 * be called when the "activate" signal of the atk object tree view.
 * Then it will do a selection change in the tree view in order to let
 * "changed" activation signal be called, which is the signal the
 * file selection actually manages */
static void
hail_file_selection_folders_tree_row_activated (GtkTreeView *tree_view,
						GtkTreePath *path,
						GtkTreeViewColumn *column,
						gpointer user_data)
{
  GtkTreeSelection * selection = NULL;

  g_return_if_fail (GTK_IS_TREE_VIEW(tree_view));

  selection = gtk_tree_view_get_selection(tree_view);
  gtk_tree_selection_select_path(selection, path);
  

}
