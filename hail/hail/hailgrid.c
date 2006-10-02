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
 * SECTION:hailgrid
 * @short_description: Implementation of the ATK interfaces for #HildonGrid
 * @see_also: #GailContainer, #HailGridItem, #HildonGrid, #HildonGridItem
 *
 * #HailGrid implements the required ATK interfaces of #HildonGrid,
 * showing its children. It also exposes the popup event as an action.
 * It reports the children from their position through #AtkComponent.
 */

#include <hildon-widgets/hildon-grid.h>
#include "hailgrid.h"

#define HAIL_GRID_DEFAULT_NAME "Grid"

#define HAIL_GRID_ACTION_NAME_POPUP "Contextual menu"
#define HAIL_GRID_ACTION_DESCRIPTION_POPUP "Pop up contextual menu"

static void                  hail_grid_class_init       (HailGridClass *klass);
static void                  hail_grid_object_init      (HailGrid      *grid);

static G_CONST_RETURN gchar* hail_grid_get_name         (AtkObject       *obj);
static void                  hail_grid_real_initialize  (AtkObject       *obj,
							 gpointer        data);

/* AtkAction */
static void                  hail_grid_action_interface_init (AtkActionIface * iface);
static gboolean              hail_grid_action_do_action (AtkAction       *action,
							 gint            index);
static gint                  hail_grid_action_get_n_actions (AtkAction   *action);
static G_CONST_RETURN gchar* hail_grid_action_get_name  (AtkAction          *action,
							 gint index);
static G_CONST_RETURN gchar* hail_grid_action_get_description  (AtkAction          *action,
								gint index);
static G_CONST_RETURN gchar* hail_grid_action_get_keybinding  (AtkAction          *action,
							       gint index);

/* atkcomponent.h */
static void             hail_grid_atk_component_interface_init(AtkComponentIface      *iface);

static AtkObject*       hail_grid_ref_accessible_at_point     (AtkComponent           *component,
							       gint                   x,
							       gint                   y,
							       AtkCoordType           coord_type);

static GType parent_type;
static GtkAccessibleClass* parent_class = NULL;

/**
 * hail_grid_get_type:
 * 
 * Initialises, and returns the type of a hail grid.
 * 
 * Return value: GType of #HailGrid.
 **/
GType
hail_grid_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
	{
	  (guint16) sizeof (HailGridClass),
	  (GBaseInitFunc) NULL, /* base init */
	  (GBaseFinalizeFunc) NULL, /* base finalize */
	  (GClassInitFunc) hail_grid_class_init, /* class init */
	  (GClassFinalizeFunc) NULL, /* class finalize */
	  NULL, /* class data */
	  (guint16) sizeof (HailGrid), /* instance size */
	  0, /* nb preallocs */
	  (GInstanceInitFunc) hail_grid_object_init, /* instance init */
	  NULL /* value table */
	};

      static const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) hail_grid_action_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      static const GInterfaceInfo atk_component_info =
      {
        (GInterfaceInitFunc) hail_grid_atk_component_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_CONTAINER);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;


      type = g_type_register_static (parent_type,
                                     "HailGrid", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);
      g_type_add_interface_static (type, ATK_TYPE_COMPONENT,
                                   &atk_component_info);
    }

  return type;
}

static void
hail_grid_class_init (HailGridClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods for AtkObject */
  class->get_name = hail_grid_get_name;
  class->initialize = hail_grid_real_initialize;

}

static void
hail_grid_object_init (HailGrid      *grid)
{
}


/**
 * hail_grid_new:
 * @widget: a #HildonGrid casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonGrid.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_grid_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_GRID (widget), NULL);

  object = g_object_new (HAIL_TYPE_GRID, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_grid_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_GRID (obj), NULL);

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

      g_return_val_if_fail (HILDON_IS_GRID (widget), NULL);

      /* It gets the name from the grid label */
      name = HAIL_GRID_DEFAULT_NAME;

    }
  return name;
}

/* Implementation of AtkObject method initialize() */
static void
hail_grid_real_initialize (AtkObject *obj,
                             gpointer   data)
{
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_PANEL;

}


void                  
hail_grid_action_interface_init (AtkActionIface * iface)
{
  g_return_if_fail (iface != NULL);

  iface->do_action = hail_grid_action_do_action;
  iface->get_n_actions = hail_grid_action_get_n_actions;
  iface->get_description = hail_grid_action_get_description;
  iface->get_keybinding = hail_grid_action_get_keybinding;
  iface->get_name = hail_grid_action_get_name;

}

/* Implementation of AtkAction method do_action() */
gboolean              
hail_grid_action_do_action (AtkAction       *action,
			    gint            index)
{
  GtkWidget *widget = NULL;
  gboolean return_value = TRUE;

  g_return_val_if_fail(HAIL_IS_GRID(action), FALSE);

  widget = GTK_ACCESSIBLE (action)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!GTK_WIDGET_IS_SENSITIVE (widget) || !GTK_WIDGET_VISIBLE (widget))
    return FALSE;

  switch (index)
    {
    case 0:
      /* emits the context menu signal. It should get a better support through
       * some kind of tap and hold support from hail */
      if (HILDON_IS_GRID(widget)) {
	g_signal_emit_by_name(widget, "popup-context-menu", NULL);
	return_value = TRUE;
      } else {
	return FALSE;
      }
      break;
    default:
      return_value = FALSE;
      break;
    }
  return return_value; 
}

/* Implementation of AtkAction method get_n_actions() */
gint                  
hail_grid_action_get_n_actions (AtkAction   *action)
{
  GtkWidget * widget = NULL;

  g_return_val_if_fail (HAIL_IS_GRID (action), 0);

  widget = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_GRID(widget), 0);

  return 1;

}

/* Implementation of AtkAction method get_name() */
G_CONST_RETURN gchar* 
hail_grid_action_get_name  (AtkAction          *action,
				 gint index)
{
  GtkWidget * widget = NULL;

  g_return_val_if_fail (HAIL_IS_GRID (action), 0);

  widget = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_GRID(widget), 0);

  switch (index) {
  case 0:
    return HAIL_GRID_ACTION_NAME_POPUP;
    break;
  default:
    return NULL;
  }

}

/* Implementation of AtkAction method get_description() */
G_CONST_RETURN gchar* 
hail_grid_action_get_description  (AtkAction          *action,
					gint index)
{
  GtkWidget * widget = NULL;

  g_return_val_if_fail (HAIL_IS_GRID (action), 0);

  widget = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_GRID(widget), 0);

  switch (index) {
  case 0:
    return HAIL_GRID_ACTION_DESCRIPTION_POPUP;
    break;
  default:
    return NULL;
  }

}

/* Implementation of AtkAction method get_keybinding() */
G_CONST_RETURN gchar* 
hail_grid_action_get_keybinding  (AtkAction          *action,
				       gint index)
{
  GtkWidget * widget = NULL;

  g_return_val_if_fail (HAIL_IS_GRID (action), 0);

  widget = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_GRID(widget), 0);


  return NULL;
}

/* atkcomponent.h */

static void
hail_grid_atk_component_interface_init (AtkComponentIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->ref_accessible_at_point = hail_grid_ref_accessible_at_point;
}

static AtkObject*
hail_grid_ref_accessible_at_point (AtkComponent           *component,
				   gint                   x,
				   gint                   y,
				   AtkCoordType           coord_type)
{
  AtkObject * child = NULL;
  gint i;

  g_return_val_if_fail (HAIL_IS_GRID(component), NULL);
  for (i = 0; i < atk_object_get_n_accessible_children (ATK_OBJECT(component)); i++) {
    child = atk_object_ref_accessible_child(ATK_OBJECT(component), i);
    if (child == NULL)
      continue;
    if ((ATK_IS_COMPONENT(child)) && 
	(atk_component_contains(ATK_COMPONENT(child),x,y,coord_type)) ) {
      break;
    } else {
      g_object_unref(child);
      child = NULL;
    }
  }

  return child;
}
