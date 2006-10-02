/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2005 Nokia Corporation.
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
 * SECTION:hailnumbereditor
 * @short_description: Implementation of the ATK interfaces for #HildonNumberEditor.
 * @see_also: #HildonNumberEditor
 *
 * #HailNumberEditor implements the required ATK interfaces of #HildonNumberEditor and its
 * children (the buttons inside the widget). It exposes the internal plus and minus buttons,
 * and the number entry. Through the internal number entry you can set or check the value
 * using the #AtkText interface.
 */

#include <gtk/gtkbutton.h>
#include <gtk/gtkentry.h>
#include <hildon-widgets/hildon-number-editor.h>
#include "hailnumbereditor.h"

#define HAIL_NUMBER_EDITOR_DEFAULT_NAME "Number editor"
#define HAIL_NUMBER_EDITOR_NUM_ENTRY_DEFAULT_NAME "Number entry"
#define HAIL_NUMBER_EDITOR_PLUS_BUTTON_DEFAULT_NAME "Plus button"
#define HAIL_NUMBER_EDITOR_MINUS_BUTTON_DEFAULT_NAME "Minus button"

typedef struct _HailNumberEditorPrivate HailNumberEditorPrivate;

struct _HailNumberEditorPrivate {
  AtkObject * num_entry; /* accessible of the number entry */
  AtkObject * plus; /* accessible of the plus button */
  AtkObject * minus; /* accessible of the minus button */
};

#define HAIL_NUMBER_EDITOR_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_NUMBER_EDITOR, HailNumberEditorPrivate))

static void                  hail_number_editor_class_init       (HailNumberEditorClass *klass);
static void                  hail_number_editor_object_init      (HailNumberEditor      *number_editor);

static G_CONST_RETURN gchar* hail_number_editor_get_name         (AtkObject       *obj);
static gint                  hail_number_editor_get_n_children   (AtkObject       *obj);
static AtkObject*            hail_number_editor_ref_child        (AtkObject       *obj,
								  gint            i);
static void                  hail_number_editor_real_initialize  (AtkObject       *obj,
								  gpointer        data);

static void                  add_internal_widgets             (GtkWidget * widget,
						               gpointer data);

static GType parent_type;
static GtkAccessibleClass* parent_class = NULL;

/**
 * hail_number_editor_get_type:
 * 
 * Initialises, and returns the type of a hail number editor.
 * 
 * Return value: GType of #HailNumberEditor.
 **/
GType
hail_number_editor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
	{
	  (guint16) sizeof (HailNumberEditorClass),
	  (GBaseInitFunc) NULL, /* base init */
	  (GBaseFinalizeFunc) NULL, /* base finalize */
	  (GClassInitFunc) hail_number_editor_class_init, /* class init */
	  (GClassFinalizeFunc) NULL, /* class finalize */
	  NULL, /* class data */
	  (guint16) sizeof (HailNumberEditor), /* instance size */
	  0, /* nb preallocs */
	  (GInstanceInitFunc) hail_number_editor_object_init, /* instance init */
	  NULL /* value table */
	};

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_CONTAINER);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailNumberEditor", &tinfo, 0);

    }

  return type;
}

static void
hail_number_editor_class_init (HailNumberEditorClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailNumberEditorPrivate));

  /* bind virtual methods for AtkObject */
  class->get_name = hail_number_editor_get_name;
  class->get_n_children = hail_number_editor_get_n_children;
  class->ref_child = hail_number_editor_ref_child;
  class->initialize = hail_number_editor_real_initialize;
}

static void
hail_number_editor_object_init (HailNumberEditor      *number_editor)
{
}


/**
 * hail_number_editor_new:
 * @widget: a #HildonNumberEditor casted as a #AtkObject
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonNumberEditor.
 * 
 * Return value: An #AtkObject
 */
AtkObject* 
hail_number_editor_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_NUMBER_EDITOR (widget), NULL);

  object = g_object_new (HAIL_TYPE_NUMBER_EDITOR, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_number_editor_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_NUMBER_EDITOR (obj), NULL);

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

      g_return_val_if_fail (HILDON_IS_NUMBER_EDITOR (widget), NULL);

      name = HAIL_NUMBER_EDITOR_DEFAULT_NAME;

    }
  return name;
}

/* callback for gtk_container_forall. It's used from atk object initialize
 * to get the references to the AtkObjects of the internal widgets */
static void
add_internal_widgets (GtkWidget * widget,
		      gpointer data)
{
  HailNumberEditor * editor = NULL;
  HailNumberEditorPrivate * priv = NULL;

  g_return_if_fail (HAIL_IS_NUMBER_EDITOR(data));
  editor = HAIL_NUMBER_EDITOR(data);
  priv = HAIL_NUMBER_EDITOR_GET_PRIVATE(editor);
  if (GTK_IS_BUTTON(widget)) {
    if (!g_strcasecmp(gtk_widget_get_name(widget), "minus-button")) {
      priv->minus = gtk_widget_get_accessible(widget);
    } else if (!g_strcasecmp(gtk_widget_get_name(widget), "plus-button")) {
      priv->plus = gtk_widget_get_accessible(widget);
    }
  } else if (GTK_IS_ENTRY(widget)) {
    priv->num_entry = gtk_widget_get_accessible(widget);
  }
}

/* Implementation of AtkObject method initialize() */
static void
hail_number_editor_real_initialize (AtkObject *obj,
				    gpointer   data)
{
  GtkWidget * number_editor = NULL;
  HailNumberEditorPrivate * priv = NULL;

  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_PANEL;

  number_editor = GTK_ACCESSIBLE(obj)->widget;

  /* search internal widgets */
  gtk_container_forall(GTK_CONTAINER(number_editor), add_internal_widgets, obj);

  priv = HAIL_NUMBER_EDITOR_GET_PRIVATE(HAIL_NUMBER_EDITOR(obj));

  g_return_if_fail (ATK_IS_OBJECT(priv->num_entry));
  g_return_if_fail (ATK_IS_OBJECT(priv->plus));
  g_return_if_fail (ATK_IS_OBJECT(priv->minus));

  /* establish significant names for the inner accessibles */
  if (atk_object_get_name(priv->plus)==NULL) {
    atk_object_set_name(priv->plus, HAIL_NUMBER_EDITOR_PLUS_BUTTON_DEFAULT_NAME);
  }
  if (atk_object_get_name(priv->minus)==NULL) {
    atk_object_set_name(priv->minus, HAIL_NUMBER_EDITOR_MINUS_BUTTON_DEFAULT_NAME);
  }
  if (atk_object_get_name(priv->num_entry)==NULL) {
    atk_object_set_name(priv->num_entry, HAIL_NUMBER_EDITOR_NUM_ENTRY_DEFAULT_NAME);
  }
}

/* Implementation of AtkObject method get_n_children() */
static gint                  
hail_number_editor_get_n_children   (AtkObject       *obj)
{
  GtkWidget *widget = NULL;

  g_return_val_if_fail (HAIL_IS_NUMBER_EDITOR (obj), 0);

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
hail_number_editor_ref_child        (AtkObject       *obj,
				     gint            i)
{
  GtkWidget *widget = NULL;
  AtkObject * accessible_child = NULL;
  HailNumberEditorPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_NUMBER_EDITOR (obj), 0);
  priv = HAIL_NUMBER_EDITOR_GET_PRIVATE(HAIL_NUMBER_EDITOR(obj));
  g_return_val_if_fail ((i >= 0), NULL);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  switch (i) {
  case 0: 
    accessible_child = priv->num_entry;
    break;
  case 1:
    accessible_child = priv->plus;
    break;
  case 2:
    accessible_child = priv->minus;
    break;
  default:
    return NULL;
    break;
  }

  g_return_val_if_fail (ATK_IS_OBJECT(accessible_child), NULL);

  g_object_ref(accessible_child);
  return accessible_child;
}
