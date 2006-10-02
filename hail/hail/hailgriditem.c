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
 * SECTION:hailgriditem
 * @short_description: Implementation of the ATK interfaces for #HildonGridItem
 * @see_also: #GailContainer, #HildonGridItem
 *
 * #HailGridItem implements the required ATK interfaces of #HildonGridItem,
 * hiding its children (the icon and the label). It also shows the label
 * and emblem in the #AtkImage description.
 */

#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <hildon-widgets/hildon-grid-item.h>
#include <hildon-widgets/hildon-grid.h>
#include "hailgriditem.h"

typedef struct _HailGridItemPrivate HailGridItemPrivate;

struct _HailGridItemPrivate {
  GtkWidget * label;    /* widget label */
  GtkWidget * icon;     /* widget icon */
  gchar * description;  /* exposed description */
};

#define HAIL_GRID_ITEM_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_GRID_ITEM, HailGridItemPrivate))

#define HAIL_GRID_ITEM_ACTION_NAME_ACTIVATE "activate"
#define HAIL_GRID_ITEM_ACTION_DESCRIPTION_ACTIVATE "activate"

static void                  hail_grid_item_class_init       (HailGridItemClass *klass);
static void                  hail_grid_item_object_init      (HailGridItem      *grid_item);

static G_CONST_RETURN gchar* hail_grid_item_get_name         (AtkObject       *obj);
static AtkStateSet*          hail_grid_item_ref_state_set    (AtkObject       *obj);
static gint                  hail_grid_item_get_n_children   (AtkObject       *obj);
static AtkObject*            hail_grid_item_ref_child        (AtkObject       *obj,
							       gint            i);
static void                  hail_grid_item_real_initialize  (AtkObject       *obj,
							       gpointer        data);

/* AtkAction */
static void                  hail_grid_item_action_interface_init (AtkActionIface * iface);
static gboolean              hail_grid_item_action_do_action (AtkAction       *action,
							      gint            index);
static gint                  hail_grid_item_action_get_n_actions (AtkAction   *action);
static G_CONST_RETURN gchar* hail_grid_item_action_get_name  (AtkAction          *action,
							      gint index);
static G_CONST_RETURN gchar* hail_grid_item_action_get_description  (AtkAction          *action,
								     gint index);
static G_CONST_RETURN gchar* hail_grid_item_action_get_keybinding  (AtkAction          *action,
								    gint index);

/* AtkImage.h */
static void                  hail_grid_item_atk_image_interface_init   (AtkImageIface  *iface);
static G_CONST_RETURN gchar* hail_grid_item_get_image_description (AtkImage       *image);
static void	             hail_grid_item_get_image_position (AtkImage       *image,
								gint	        *x,
								gint	        *y,
								AtkCoordType   coord_type);
static void                  hail_grid_item_get_image_size (AtkImage       *image,
							    gint           *width,
							    gint           *height);

static GType parent_type;
static GtkAccessibleClass* parent_class = NULL;

/**
 * hail_grid_item_get_type:
 * 
 * Initialises, and returns the type of a hail grid_item.
 * 
 * Return value: GType of #HailGridItem.
 **/
GType
hail_grid_item_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
	{
	  (guint16) sizeof (HailGridItemClass),
	  (GBaseInitFunc) NULL, /* base init */
	  (GBaseFinalizeFunc) NULL, /* base finalize */
	  (GClassInitFunc) hail_grid_item_class_init, /* class init */
	  (GClassFinalizeFunc) NULL, /* class finalize */
	  NULL, /* class data */
	  (guint16) sizeof (HailGridItem), /* instance size */
	  0, /* nb preallocs */
	  (GInstanceInitFunc) hail_grid_item_object_init, /* instance init */
	  NULL /* value table */
	};

      static const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) hail_grid_item_action_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      static const GInterfaceInfo atk_image_info =
      {
        (GInterfaceInitFunc) hail_grid_item_atk_image_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_CONTAINER);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;


      type = g_type_register_static (parent_type,
                                     "HailGridItem", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);

      g_type_add_interface_static (type, ATK_TYPE_IMAGE,
                                   &atk_image_info);

    }

  return type;
}

static void
hail_grid_item_class_init (HailGridItemClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailGridItemPrivate));

  /* bind virtual methods for AtkObject */
  class->get_name = hail_grid_item_get_name;
  class->get_n_children = hail_grid_item_get_n_children;
  class->ref_child = hail_grid_item_ref_child;
  class->ref_state_set = hail_grid_item_ref_state_set;
  class->initialize = hail_grid_item_real_initialize;

}

static void
hail_grid_item_object_init (HailGridItem      *grid_item)
{
  HailGridItemPrivate * priv = NULL;

  priv = HAIL_GRID_ITEM_GET_PRIVATE(grid_item);

  priv->icon = NULL;
  priv->label = NULL;
  priv->description = NULL;
}


/**
 * hail_grid_item_new:
 * @widget: a #HildonGridItem casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonGridItem.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_grid_item_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_GRID_ITEM (widget), NULL);

  object = g_object_new (HAIL_TYPE_GRID_ITEM, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_grid_item_get_name (AtkObject *obj)
{
  HailGridItemPrivate * priv = NULL;
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_GRID_ITEM (obj), NULL);
  priv = HAIL_GRID_ITEM_GET_PRIVATE(HAIL_GRID_ITEM(obj));

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

      g_return_val_if_fail (HILDON_IS_GRID_ITEM (widget), NULL);

      /* It gets the name from the grid_item label */
      name = gtk_label_get_text(GTK_LABEL(priv->label));

    }
  return name;
}

/* callback for gtk_container_forall. It's used from atk object initialize
 * to get the references to the AtkObjects of the internal widgets */
static void
add_internal_widgets (GtkWidget * widget,
		     gpointer data)
{
  HailGridItem * bar = NULL;
  HailGridItemPrivate * priv = NULL;

  g_return_if_fail (HAIL_IS_GRID_ITEM(data));
  bar = HAIL_GRID_ITEM(data);
  priv = HAIL_GRID_ITEM_GET_PRIVATE(bar);
  if (GTK_IS_LABEL(widget)) {
    priv->label = widget;
  } else if (GTK_IS_IMAGE(widget)) {
    priv->icon = widget;
  }
}

/* Implementation of AtkObject method initialize() */
static void
hail_grid_item_real_initialize (AtkObject *obj,
                             gpointer   data)
{
  GtkWidget * grid_item = NULL;
  HailGridItemPrivate * priv = NULL;

  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_LABEL;

  grid_item = GTK_ACCESSIBLE(obj)->widget;

  gtk_container_forall(GTK_CONTAINER(grid_item), add_internal_widgets, obj);

  priv = HAIL_GRID_ITEM_GET_PRIVATE(HAIL_GRID_ITEM(obj));

  g_return_if_fail (GTK_IS_LABEL(priv->label));
  g_return_if_fail (GTK_IS_IMAGE(priv->icon));

}

/* Implementation of AtkObject method ref_state_set() */
static AtkStateSet*
hail_grid_item_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set = NULL;
  GtkWidget *widget = NULL;

  state_set = ATK_OBJECT_CLASS (parent_class)->ref_state_set (obj);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    return state_set;

  atk_state_set_add_state(state_set, ATK_STATE_MANAGES_DESCENDANTS);
  return state_set;
}

/* Implementation of AtkObject method get_n_children() */
static gint                  
hail_grid_item_get_n_children   (AtkObject       *obj)
{
  GtkWidget *widget = NULL;

  g_return_val_if_fail (HAIL_IS_GRID_ITEM (obj), 0);

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  return 0;

}

/* Implementation of AtkObject method ref_child() */
static AtkObject*            
hail_grid_item_ref_child        (AtkObject       *obj,
				  gint            i)
{
  GtkWidget *widget = NULL;
  HailGridItemPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_GRID_ITEM (obj), 0);
  priv = HAIL_GRID_ITEM_GET_PRIVATE(HAIL_GRID_ITEM(obj));
  g_return_val_if_fail ((i >= 0), NULL);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  return NULL;
}

/* AtkAction initialization */
void                  
hail_grid_item_action_interface_init (AtkActionIface * iface)
{
  g_return_if_fail (iface != NULL);

  iface->do_action = hail_grid_item_action_do_action;
  iface->get_n_actions = hail_grid_item_action_get_n_actions;
  iface->get_description = hail_grid_item_action_get_description;
  iface->get_keybinding = hail_grid_item_action_get_keybinding;
  iface->get_name = hail_grid_item_action_get_name;

}

/* Implementation of AtkAction method do_action() */
gboolean              
hail_grid_item_action_do_action (AtkAction       *action,
				 gint            index)
{
  GtkWidget *widget = NULL;
  GtkWidget * grid = NULL;
  HailGridItem *item = NULL;
  HailGridItemPrivate * priv = NULL;
  gboolean return_value = TRUE;

  widget = GTK_ACCESSIBLE (action)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!GTK_WIDGET_IS_SENSITIVE (widget) || !GTK_WIDGET_VISIBLE (widget))
    return FALSE;

  grid = gtk_widget_get_parent(widget);
  
  item = HAIL_GRID_ITEM (action); 
  priv = HAIL_GRID_ITEM_GET_PRIVATE(item);

  switch (index)
    {
    case 0:
      /* forwards the activate to the grid */
      if (HILDON_IS_GRID(grid)) {
	  hildon_grid_activate_child (HILDON_GRID(grid), HILDON_GRID_ITEM(widget));
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
hail_grid_item_action_get_n_actions (AtkAction   *action)
{
  GtkWidget * grid_item = NULL;

  g_return_val_if_fail (HAIL_IS_GRID_ITEM (action), 0);

  grid_item = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_GRID_ITEM(grid_item), 0);

  return 1;

}

/* Implementation of AtkAction method get_name() */
G_CONST_RETURN gchar* 
hail_grid_item_action_get_name  (AtkAction          *action,
				 gint index)
{
  GtkWidget * grid_item = NULL;

  g_return_val_if_fail (HAIL_IS_GRID_ITEM (action), 0);

  grid_item = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_GRID_ITEM(grid_item), 0);

  switch (index) {
  case 0:
    return HAIL_GRID_ITEM_ACTION_NAME_ACTIVATE;
    break;
  default:
    return NULL;
  }

}

/* Implementation of AtkAction method get_description() */
G_CONST_RETURN gchar* 
hail_grid_item_action_get_description  (AtkAction          *action,
					gint index)
{
  GtkWidget * grid_item = NULL;

  g_return_val_if_fail (HAIL_IS_GRID_ITEM (action), 0);

  grid_item = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_GRID_ITEM(grid_item), 0);

  switch (index) {
  case 0:
    return HAIL_GRID_ITEM_ACTION_DESCRIPTION_ACTIVATE;
    break;
  default:
    return NULL;
  }

}

/* Implementation of AtkAction method get_keybinding */
G_CONST_RETURN gchar* 
hail_grid_item_action_get_keybinding  (AtkAction          *action,
				       gint index)
{
  GtkWidget * grid_item = NULL;

  g_return_val_if_fail (HAIL_IS_GRID_ITEM (action), 0);

  grid_item = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_GRID_ITEM(grid_item), 0);


  return NULL;
}

/* AtkImage interface initialization. It binds the virtual methods */
static void
hail_grid_item_atk_image_interface_init (AtkImageIface *iface)
{
  g_return_if_fail (iface != NULL);

  /* Binding of virtual methods of AtkImage */
  iface->get_image_description = hail_grid_item_get_image_description;
  iface->get_image_position = hail_grid_item_get_image_position;
  iface->get_image_size = hail_grid_item_get_image_size;
}

/* Implementation of AtkImage method get_image_description() */
static G_CONST_RETURN gchar* 
hail_grid_item_get_image_description (AtkImage *image)
{
  HailGridItemPrivate * priv = NULL;
  GtkWidget *widget = NULL;
  G_CONST_RETURN gchar* description = NULL;

  g_return_val_if_fail (HAIL_IS_GRID_ITEM (image), NULL);
  priv = HAIL_GRID_ITEM_GET_PRIVATE(HAIL_GRID_ITEM(image));

  if (priv->description) {
    g_free(priv->description);
    priv->description = NULL;
  }
  /*
   * Get the text on the label
   */
  
  widget = GTK_ACCESSIBLE (image)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;
  
  g_return_val_if_fail (HILDON_IS_GRID_ITEM (widget), NULL);
  
  /* It gets the description from the grid_item label and emblem */
  if (hildon_grid_item_get_emblem_type(HILDON_GRID_ITEM(widget))) {
    priv->description = 
      g_strconcat(gtk_label_get_text(GTK_LABEL(priv->label)),"(",
		  hildon_grid_item_get_emblem_type(HILDON_GRID_ITEM(widget)),
		  ")", NULL);
  } else {
    priv->description = g_strdup(gtk_label_get_text(GTK_LABEL(priv->label)));
  }
  
  description = priv->description;
  
  return description;
}

/* Implementation of AtkImage method get_image_position() */
static void
hail_grid_item_get_image_position (AtkImage     *image,
				      gint	     *x,
				      gint	     *y,
				      AtkCoordType coord_type)
{
  HailGridItemPrivate * priv = NULL;
  AtkObject * icon_a = NULL;

  g_return_if_fail (HAIL_IS_GRID_ITEM(image));
  priv = HAIL_GRID_ITEM_GET_PRIVATE(image);
  g_return_if_fail (GTK_IS_WIDGET(priv->icon));

  icon_a = gtk_widget_get_accessible(priv->icon);
  atk_component_get_position(ATK_COMPONENT(icon_a), x, y, coord_type);
}

/* Implementation of AtkImage method get_image_size() */
static void
hail_grid_item_get_image_size (AtkImage *image,
			       gint     *width,
			       gint     *height)
{
  HailGridItemPrivate * priv = NULL;
  AtkObject * icon_a = NULL;

  g_return_if_fail (HAIL_IS_GRID_ITEM(image));
  priv = HAIL_GRID_ITEM_GET_PRIVATE(image);
  g_return_if_fail (GTK_IS_WIDGET(priv->icon));

  icon_a = gtk_widget_get_accessible(priv->icon);
  atk_component_get_size(ATK_COMPONENT(icon_a), width, height);
}
