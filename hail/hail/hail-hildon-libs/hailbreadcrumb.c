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
 * SECTION:hailbreadcrumb
 * @short_description: Implementation of the ATK interfaces for #HildonBreadCrumbTrail
 * @see_also: #HildonBreadCrumb, #HildonBreadCrumbTrail, #HildonBreadCrumbWidget
 *
 * #HailBreadCrumb implements the required ATK interfaces of #HildonBreadCrumbTrail,
 * exposing a name and description and redefining the children methods to expose
 * the hildon-bread-crumb-widgets and the back button.
 */

#include <hildon/hildon-bread-crumb-trail.h>
#include "hailbreadcrumb.h"

#define HAIL_BREAD_CRUMB_DEFAULT_NAME "Hail Bread Crumb"
#define HAIL_BREAD_CRUMB_DEFAULT_DESCRIPTION "It represents a specific path in a hierarchical tree."


static void                  hail_bread_crumb_class_init      (HailBreadCrumbClass *klass);
static void                  hail_bread_crumb_object_init     (HailBreadCrumb      *bread_crumb);

static G_CONST_RETURN gchar *hail_bread_crumb_get_name        (AtkObject *obj);
static G_CONST_RETURN gchar *hail_bread_crumb_get_description (AtkObject *obj);

static gint                  hail_bread_crumb_get_n_children  (AtkObject *obj);
static AtkObject *           hail_bread_crumb_ref_child       (AtkObject *obj,
							       gint       i);

static void                  hail_bread_crumb_real_initialize (AtkObject *obj,
							       gpointer data);
static void                  add_internal_widgets             (GtkWidget *widget,
							       gpointer data);

typedef struct _HailBreadCrumbPrivate HailBreadCrumbPrivate;
struct _HailBreadCrumbPrivate
{
  AtkObject *back_button;
};

#define HAIL_BREAD_CRUMB_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_BREAD_CRUMB, HailBreadCrumbPrivate))

static GType parent_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_bread_crumb_get_type:
 * 
 * Initialises, and returns the type of a hail bread crumb.
 * 
 * Return value: GType of #HailBreadCrumb.
 **/
GType
hail_bread_crumb_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
	{
	  (guint16) sizeof (HailBreadCrumbClass),
	  (GBaseInitFunc) NULL,      /* base init */
	  (GBaseFinalizeFunc) NULL,  /* base finalize */
	  (GClassInitFunc) hail_bread_crumb_class_init, /* class init */
	  (GClassFinalizeFunc) NULL, /* class finalize */
	  NULL,                      /* class data */
	  (guint16) sizeof (HailBreadCrumb), /* instance size */
	  0,                         /* nb preallocs */
	  (GInstanceInitFunc) hail_bread_crumb_object_init, /* instance init */
	  NULL                       /* value table */
	};

      factory = atk_registry_get_factory (atk_get_default_registry (),
					  HILDON_TYPE_BREAD_CRUMB_TRAIL);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailBreadCrumb", &tinfo, 0);
    }

  return type;
}

static void
hail_bread_crumb_class_init (HailBreadCrumbClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailBreadCrumbPrivate));

  /* bind virtual methods for AtkObject */
  class->get_name   = hail_bread_crumb_get_name;
  class->get_description = hail_bread_crumb_get_description;
  class->get_n_children  = hail_bread_crumb_get_n_children;
  class->ref_child  = hail_bread_crumb_ref_child;
  class->initialize = hail_bread_crumb_real_initialize;
}

static void
hail_bread_crumb_object_init (HailBreadCrumb *bread_crumb)
{
  HailBreadCrumbPrivate *priv = NULL;

  priv = HAIL_BREAD_CRUMB_GET_PRIVATE (bread_crumb);

  priv->back_button = NULL;
}

/**
 * hail_bread_crumb_new:
 * @widget: a #HildonBreadCrumbTrail casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonBreadCrumbTrail.
 * 
 * Return value: An #AtkObject
 *
 */
AtkObject *
hail_bread_crumb_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_BREAD_CRUMB_TRAIL (widget), NULL);

  object = g_object_new (HAIL_TYPE_BREAD_CRUMB, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * Implementation of AtkObject method get_name()
 */
static G_CONST_RETURN gchar *
hail_bread_crumb_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar *name = NULL;

  g_return_val_if_fail (HAIL_IS_BREAD_CRUMB (obj), NULL);

  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);
  if (name == NULL)
    {
      /* It gets the default name */
      name = HAIL_BREAD_CRUMB_DEFAULT_NAME;
    }

  return name;
}

/*
 * Implementation of AtkObject method get_description()
 */
static G_CONST_RETURN gchar *
hail_bread_crumb_get_description (AtkObject *obj)
{
  G_CONST_RETURN gchar *description = NULL;

  g_return_val_if_fail (HAIL_IS_BREAD_CRUMB (obj), NULL);

  description = ATK_OBJECT_CLASS (parent_class)->get_description (obj);
  if ((description == NULL) || (g_ascii_strcasecmp (description, "") == 0))
    {
      /* It gets the default description */
      description = HAIL_BREAD_CRUMB_DEFAULT_DESCRIPTION;
    }

  return description;
}

/*
 * Implementation of AtkObject method get_n_children()
 * At least the widget has the titlebar widget (#HildonHomeTitleBar).
 */
static gint
hail_bread_crumb_get_n_children (AtkObject *obj)
{
  HailBreadCrumb *bct = NULL;
  HailBreadCrumbPrivate *priv  = NULL;
  gint n_children = 0;

  g_return_val_if_fail (HAIL_IS_BREAD_CRUMB (obj), 0);

  bct = HAIL_BREAD_CRUMB (obj);
  priv = HAIL_BREAD_CRUMB_GET_PRIVATE (bct);

  /* any children from the parent (the bread_crumb_widget buttons, at least) */
  n_children = ATK_OBJECT_CLASS (parent_class)->get_n_children (obj);

  if (priv->back_button)
    {
      n_children += 1;
    }
  
  return n_children;
}

/*
 * Implementation of AtkObject method ref_accessible_child()
 */
static AtkObject *
hail_bread_crumb_ref_child (AtkObject *obj,
			    gint       i)
{
  HailBreadCrumb *bct = NULL;
  HailBreadCrumbPrivate *priv = NULL;
  AtkObject *accessible = NULL;
  guint parent_children = 0;
  gboolean back_button = FALSE;

  g_return_val_if_fail (HAIL_IS_BREAD_CRUMB (obj), NULL);
  g_return_val_if_fail ((i >= 0), NULL);

  bct = HAIL_BREAD_CRUMB (obj);
  priv = HAIL_BREAD_CRUMB_GET_PRIVATE (obj);

  /* take care if the parent doesn't have exposed
   * the bread_crumb_widget buttons or there aren't ones
   * and if the back button is available
   */
  parent_children = ATK_OBJECT_CLASS (parent_class)->get_n_children (obj);
  back_button = priv->back_button != NULL;

  if (i >= parent_children + (back_button ? 1 : 0))
    {
      return NULL;
    }

  /* when selecting parent children */
  if (i < parent_children)
    {
      accessible = ATK_OBJECT_CLASS (parent_class)->ref_child (obj, i);
    }
  else
    {
      /* the last one is the back button */
      if (back_button && (i == parent_children))
	{
	  accessible = priv->back_button;
	  g_object_ref (accessible);
	}
    }

  return accessible;
}

/*
 * Implementation of AtkObject method initialize()
 */
static void
hail_bread_crumb_real_initialize (AtkObject *obj,
				  gpointer   data)
{
  GtkWidget *bct = NULL;
  HailBreadCrumbPrivate *priv = NULL;

  g_return_if_fail (HAIL_IS_BREAD_CRUMB (obj));
  priv = HAIL_BREAD_CRUMB_GET_PRIVATE (obj);

  /* parent initialize */
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  bct = GTK_ACCESSIBLE(obj)->widget;

  /* get references to hidden widgets */
  gtk_container_forall(GTK_CONTAINER (bct),
		       add_internal_widgets, obj);
}

/*
 * callback for gtk_container_forall.
 * It's used from atk object initialize to get the references
 * to the AtkObjects of the internal widgets.
 */
static void
add_internal_widgets (GtkWidget *widget, gpointer data)
{
  HailBreadCrumbPrivate *priv = NULL;

  g_return_if_fail (HAIL_IS_BREAD_CRUMB (data));
  priv = HAIL_BREAD_CRUMB_GET_PRIVATE (data);

  /* the back button */
  if (GTK_IS_BUTTON(widget))
    {
      priv->back_button = gtk_widget_get_accessible (widget);
    }
}


