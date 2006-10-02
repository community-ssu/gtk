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
 * SECTION:hailvolumebar
 * @short_description: Implementation of the ATK interfaces for #HildonVolumebar and its children
 * (#HildonHVolumebar and #HildonVVolumebar).
 * @see_also: #GailContainer, #GailRange, #GailToggleButton, #HildonVolumebar
 *
 * #HailVolumebar implements the required ATK interfaces of #HildonVolumebar and its
 * children (#HildonHVolumebar and #HildonVVolumebar). It exposes:
 * <itemizedlist>
 * <listitem>The internal toggle button for muting the sound, as a #GailToggleButton
 * referable as child of it.</listitem>
 * <listitem>The internal range to set the volume, as a #GailRange referable as child of
 * it.</listitem>
 * <listitem>The horizontal or vertical representation of the widget as an AtkState</listitem>
 * </itemizedlist>
 */

#include <gtk/gtkrange.h>
#include <gtk/gtktogglebutton.h>
#include <hildon-widgets/hildon-hvolumebar.h>
#include <hildon-widgets/hildon-vvolumebar.h>
#include "hailvolumebar.h"

#define HAIL_VOLUME_BAR_DEFAULT_NAME "Volume bar"
#define HAIL_VOLUME_BAR_MUTE_BUTTON_DEFAULT_NAME "Toggle mute"
#define HAIL_VOLUME_BAR_RANGE_DEFAULT_NAME "Volume bar range"

typedef struct _HailVolumeBarPrivate HailVolumeBarPrivate;

struct _HailVolumeBarPrivate {
  AtkObject * mute_toggle;     /* the mute toggle accessible object */
  AtkObject * volume_range;    /* volume range accessible object */
};

#define HAIL_VOLUME_BAR_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_VOLUME_BAR, HailVolumeBarPrivate))

static void                  hail_volume_bar_class_init       (HailVolumeBarClass *klass);
static void                  hail_volume_bar_object_init      (HailVolumeBar      *volume_bar);

static G_CONST_RETURN gchar* hail_volume_bar_get_name         (AtkObject       *obj);
static AtkStateSet*          hail_volume_bar_ref_state_set    (AtkObject       *obj);
static gint                  hail_volume_bar_get_n_children   (AtkObject       *obj);
static AtkObject*            hail_volume_bar_ref_child        (AtkObject       *obj,
							       gint            i);
static void                  hail_volume_bar_real_initialize  (AtkObject       *obj,
							       gpointer        data);

static void                  add_internal_widgets             (GtkWidget * widget,
						               gpointer data);

/* AtkValue.h */
static void	    hail_volume_bar_atk_value_interface_init	 (AtkValueIface  *iface);
static void	    hail_volume_bar_get_current_value (AtkValue       *obj,
						       GValue         *value);
static void	    hail_volume_bar_get_maximum_value (AtkValue       *obj,
						       GValue         *value);
static void	    hail_volume_bar_get_minimum_value (AtkValue       *obj,
						       GValue         *value);
static gboolean	    hail_volume_bar_set_current_value (AtkValue       *obj,
						       const GValue   *value);


static GType parent_type;
static GtkAccessibleClass* parent_class = NULL;

/**
 * hail_volume_bar_get_type:
 * 
 * Initialises, and returns the type of a hail volume_bar.
 * 
 * Return value: GType of #HailVolumeBar.
 **/
GType
hail_volume_bar_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
	{
	  (guint16) sizeof (HailVolumeBarClass),
	  (GBaseInitFunc) NULL, /* base init */
	  (GBaseFinalizeFunc) NULL, /* base finalize */
	  (GClassInitFunc) hail_volume_bar_class_init, /* class init */
	  (GClassFinalizeFunc) NULL, /* class finalize */
	  NULL, /* class data */
	  (guint16) sizeof (HailVolumeBar), /* instance size */
	  0, /* nb preallocs */
	  (GInstanceInitFunc) hail_volume_bar_object_init, /* instance init */
	  NULL /* value table */
	};

      static const GInterfaceInfo atk_value_info =
      {
        (GInterfaceInitFunc) hail_volume_bar_atk_value_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_CONTAINER);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;


      type = g_type_register_static (parent_type,
                                     "HailVolumeBar", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_VALUE,
                                   &atk_value_info);

    }

  return type;
}

static void
hail_volume_bar_class_init (HailVolumeBarClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailVolumeBarPrivate));

  /* bind virtual methods for AtkObject */
  class->get_name = hail_volume_bar_get_name;
  class->get_n_children = hail_volume_bar_get_n_children;
  class->ref_child = hail_volume_bar_ref_child;
  class->ref_state_set = hail_volume_bar_ref_state_set;
  class->initialize = hail_volume_bar_real_initialize;

}

static void
hail_volume_bar_object_init (HailVolumeBar      *volume_bar)
{
}


/**
 * hail_volume_bar_new:
 * @widget: a #HildonVolumeBar casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonVolumeBar.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_volume_bar_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_VOLUMEBAR (widget), NULL);

  object = g_object_new (HAIL_TYPE_VOLUME_BAR, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_volume_bar_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_VOLUME_BAR (obj), NULL);

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

      g_return_val_if_fail (HILDON_IS_VOLUMEBAR (widget), NULL);

      /* It gets the name from the volume_bar label */
      name = HAIL_VOLUME_BAR_DEFAULT_NAME;

    }
  return name;
}

/* initialize the AtkValue interface, binding methods */
static void	 
hail_volume_bar_atk_value_interface_init (AtkValueIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->get_current_value = hail_volume_bar_get_current_value;
  iface->get_maximum_value = hail_volume_bar_get_maximum_value;
  iface->get_minimum_value = hail_volume_bar_get_minimum_value;
  iface->set_current_value = hail_volume_bar_set_current_value;
}

/* Implementation of AtkValue method get_current_value() */
static void	 
hail_volume_bar_get_current_value (AtkValue		*obj,
				       GValue		*value)
{
  HailVolumeBarPrivate * priv = NULL;

  g_return_if_fail (HAIL_IS_VOLUME_BAR (obj));
  priv = HAIL_VOLUME_BAR_GET_PRIVATE(obj);

  g_value_init(value, G_TYPE_DOUBLE);
  atk_value_get_current_value (ATK_VALUE (priv->volume_range), value);
}

/* Implementation of AtkValue method get_maximum_value() */
static void	 
hail_volume_bar_get_maximum_value (AtkValue		*obj,
				   GValue		*value)
{
  HailVolumeBarPrivate * priv = NULL;

  g_return_if_fail (HAIL_IS_VOLUME_BAR (obj));
  priv = HAIL_VOLUME_BAR_GET_PRIVATE(obj);

  g_value_init(value, G_TYPE_DOUBLE);
  atk_value_get_maximum_value (ATK_VALUE (priv->volume_range), value);
}

/* Implementation of AtkValue method get_minimum_value() */
static void	 
hail_volume_bar_get_minimum_value (AtkValue		*obj,
				   GValue		*value)
{
  HailVolumeBarPrivate * priv = NULL;

  g_return_if_fail (HAIL_IS_VOLUME_BAR (obj));
  priv = HAIL_VOLUME_BAR_GET_PRIVATE(obj);

  g_value_init(value, G_TYPE_DOUBLE);
  atk_value_get_minimum_value (ATK_VALUE (priv->volume_range), value);
}

/* Implementation of AtkValue method set_current_value() */
static gboolean	 
hail_volume_bar_set_current_value (AtkValue		*obj,
				   const GValue	*value)
{
  HailVolumeBarPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_VOLUME_BAR (obj), FALSE);
  priv = HAIL_VOLUME_BAR_GET_PRIVATE(obj);

  atk_value_set_current_value (ATK_VALUE (priv->volume_range), value);

  return TRUE;
}

/* callback for gtk_container_forall. It's used from atk object initialize
 * to get the references to the AtkObjects of the internal widgets */
static void
add_internal_widgets (GtkWidget * widget,
		     gpointer data)
{
  HailVolumeBar * bar = NULL;
  HailVolumeBarPrivate * priv = NULL;

  g_return_if_fail (HAIL_IS_VOLUME_BAR(data));
  bar = HAIL_VOLUME_BAR(data);
  priv = HAIL_VOLUME_BAR_GET_PRIVATE(bar);
  if (GTK_IS_TOGGLE_BUTTON(widget)) {
    priv->mute_toggle = gtk_widget_get_accessible(widget);
  } else if (GTK_IS_RANGE(widget)) {
    priv->volume_range = gtk_widget_get_accessible(widget);
  }
}

/* Implementation of AtkObject method initialize() */
static void
hail_volume_bar_real_initialize (AtkObject *obj,
                             gpointer   data)
{
  GtkWidget * volume_bar = NULL;
  HailVolumeBarPrivate * priv = NULL;

  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_PANEL;

  volume_bar = GTK_ACCESSIBLE(obj)->widget;

  gtk_container_forall(GTK_CONTAINER(volume_bar), add_internal_widgets, obj);

  priv = HAIL_VOLUME_BAR_GET_PRIVATE(HAIL_VOLUME_BAR(obj));

  g_return_if_fail (ATK_IS_OBJECT(priv->mute_toggle));
  g_return_if_fail (ATK_IS_OBJECT(priv->volume_range));

  atk_object_set_name(priv->mute_toggle, HAIL_VOLUME_BAR_MUTE_BUTTON_DEFAULT_NAME);
  atk_object_set_name(priv->volume_range, HAIL_VOLUME_BAR_RANGE_DEFAULT_NAME);

}

/* Implementation of AtkObject method ref_state_set() */
static AtkStateSet*
hail_volume_bar_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set = NULL;
  GtkWidget *widget = NULL;
  HildonVolumebar *volume_bar = NULL;

  state_set = ATK_OBJECT_CLASS (parent_class)->ref_state_set (obj);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    return state_set;

  volume_bar = HILDON_VOLUMEBAR (widget);

  if (HILDON_IS_HVOLUMEBAR(widget)) {
    atk_state_set_add_state(state_set, ATK_STATE_HORIZONTAL);
  } else if (HILDON_IS_VVOLUMEBAR(widget)) {
    atk_state_set_add_state(state_set, ATK_STATE_VERTICAL);
  }
  return state_set;
}

/* Implementation of AtkObject method get_n_children() */
static gint                  
hail_volume_bar_get_n_children   (AtkObject       *obj)
{
  GtkWidget *widget = NULL;

  g_return_val_if_fail (HAIL_IS_VOLUME_BAR (obj), 0);

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  /* the mute and the range */
  return 2;

}

/* Implementation of the AtkObject method ref_child() */
static AtkObject*            
hail_volume_bar_ref_child        (AtkObject       *obj,
				  gint            i)
{
  GtkWidget *widget = NULL;
  AtkObject * accessible_child = NULL;
  HailVolumeBarPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_VOLUME_BAR (obj), 0);
  priv = HAIL_VOLUME_BAR_GET_PRIVATE(HAIL_VOLUME_BAR(obj));
  g_return_val_if_fail ((i >= 0), NULL);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  switch (i) {
  case 0: 
    accessible_child = priv->mute_toggle;
    break;
  case 1:
    accessible_child = priv->volume_range;
    break;
  default:
    return NULL;
    break;
  }

  g_return_val_if_fail (ATK_IS_OBJECT(accessible_child), NULL);

  g_object_ref(accessible_child);
  return accessible_child;
}
