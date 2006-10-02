/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2006 Nokia Corporation.
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
 * SECTION:hailrangeeditor
 * @short_description: Implementation of the ATK interfaces for #HildonRangeEditor.
 * @see_also: #HildonRangeEditor
 *
 * #HailRangeEditor implements the required ATK interfaces of #HildonRangeEditor and its
 * children (the buttons inside the widget). It exposes the two entries and the label
 * in the middle. The entries get Start and End names.
 */

#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <hildon-widgets/hildon-range-editor.h>
#include "hailrangeeditor.h"

#define HAIL_RANGE_EDITOR_DEFAULT_NAME "Edit range"
#define HAIL_RANGE_EDITOR_START_ENTRY_NAME "Start"
#define HAIL_RANGE_EDITOR_END_ENTRY_NAME "End"

typedef struct _HailRangeEditorPrivate HailRangeEditorPrivate;

struct _HailRangeEditorPrivate {
  AtkObject * start_entry;
  AtkObject * end_entry;
  AtkObject * label;
};

#define HAIL_RANGE_EDITOR_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_RANGE_EDITOR, HailRangeEditorPrivate))

static void                  hail_range_editor_class_init       (HailRangeEditorClass *klass);
static void                  hail_range_editor_object_init      (HailRangeEditor      *range_editor);

static G_CONST_RETURN gchar* hail_range_editor_get_name         (AtkObject       *obj);
static gint                  hail_range_editor_get_n_children   (AtkObject       *obj);
static AtkObject*            hail_range_editor_ref_child        (AtkObject       *obj,
								 gint            i);
static void                  hail_range_editor_real_initialize  (AtkObject       *obj,
								 gpointer        data);

static void                  add_internal_widgets                  (GtkWidget * widget,
								    gpointer data);
static GType parent_type;
static GtkAccessible * parent_class = NULL;

/**
 * hail_range_editor_get_type:
 * 
 * Initialises, and returns the type of a hail range_editor.
 * 
 * Return value: GType of #HailRangeEditor.
 **/
GType
hail_range_editor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailRangeEditorClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) hail_range_editor_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        (guint16) sizeof (HailRangeEditor), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) hail_range_editor_object_init, /* instance init */
        NULL /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_CONTAINER);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailRangeEditor", &tinfo, 0);

    }

  return type;
}

static void
hail_range_editor_class_init (HailRangeEditorClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailRangeEditorPrivate));

  /* bind virtual methods for AtkObject */
  class->get_name = hail_range_editor_get_name;
  class->get_n_children = hail_range_editor_get_n_children;
  class->ref_child = hail_range_editor_ref_child;
  class->initialize = hail_range_editor_real_initialize;

}

static void
hail_range_editor_object_init (HailRangeEditor      *range_editor)
{
  HailRangeEditorPrivate * priv = NULL;

  priv = HAIL_RANGE_EDITOR_GET_PRIVATE(range_editor);
  priv->start_entry = priv->end_entry = priv->label = NULL;
}


/**
 * hail_range_editor_new:
 * @widget: a #HildonRangeEditor casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonRangeEditor.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_range_editor_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_RANGE_EDITOR (widget), NULL);

  object = g_object_new (HAIL_TYPE_RANGE_EDITOR, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_range_editor_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_RANGE_EDITOR (obj), NULL);

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

      g_return_val_if_fail (HILDON_IS_RANGE_EDITOR (widget), NULL);

      /* It gets the name from the range_editor label */
      name = HAIL_RANGE_EDITOR_DEFAULT_NAME;

    }
  return name;
}

/* callback for gtk_container_forall. It's used from atk object initialize
 * to get the references to the AtkObjects of the internal buttons */
static void
add_internal_widgets (GtkWidget * widget,
		      gpointer data)
{
  HailRangeEditor * editor = NULL;
  AtkObject * accessible_child = NULL;
  HailRangeEditorPrivate *priv = NULL;

  g_return_if_fail (HAIL_IS_RANGE_EDITOR(data));
  editor = HAIL_RANGE_EDITOR(data);
  priv = HAIL_RANGE_EDITOR_GET_PRIVATE(editor);
  if (GTK_IS_LABEL(widget)) {
    accessible_child = gtk_widget_get_accessible(widget);
    priv->label = accessible_child;
  } else if (GTK_IS_ENTRY(widget)) {
    accessible_child = gtk_widget_get_accessible(widget);
    if (priv->start_entry == NULL) {
      priv->start_entry = accessible_child;
      if (atk_object_get_name(accessible_child) == NULL) {
	atk_object_set_name(accessible_child, HAIL_RANGE_EDITOR_START_ENTRY_NAME);
      }
    } else {
      priv->end_entry = accessible_child;
      if (atk_object_get_name(accessible_child) == NULL) {
	atk_object_set_name(accessible_child, HAIL_RANGE_EDITOR_END_ENTRY_NAME);
      }
    }
  }
}

/* Implementation of AtkObject method initialize() */
static void
hail_range_editor_real_initialize (AtkObject *obj,
                             gpointer   data)
{
  GtkWidget * widget = NULL;
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_PANEL;

  widget = GTK_ACCESSIBLE (obj)->widget;
  gtk_container_forall(GTK_CONTAINER(widget), add_internal_widgets, obj);

}

/* Implementation of AtkObject method get_n_children() */
static gint                  
hail_range_editor_get_n_children   (AtkObject       *obj)
{
  GtkWidget *widget = NULL;

  g_return_val_if_fail (HAIL_IS_RANGE_EDITOR (obj), 0);

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  return 3;

}

/* Implementation of AtkObject method ref_child() */
static AtkObject*            
hail_range_editor_ref_child        (AtkObject       *obj,
				      gint            i)
{
  GtkWidget *widget = NULL;
  AtkObject * accessible_child = NULL;
  HailRangeEditorPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_RANGE_EDITOR (obj), 0);
  priv = HAIL_RANGE_EDITOR_GET_PRIVATE(HAIL_RANGE_EDITOR(obj));
  g_return_val_if_fail ((i >= 0)&&(i<3), NULL);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  switch (i) {
  case 0:
    accessible_child = priv->start_entry;
    break;
  case 1:
    accessible_child = priv->label;
    break;
  case 2:
    accessible_child = priv->end_entry;
    break;
  }

  g_return_val_if_fail (ATK_IS_OBJECT(accessible_child), NULL);

  g_object_ref(accessible_child);
  return accessible_child;
}
