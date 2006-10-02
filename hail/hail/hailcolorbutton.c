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
 * SECTION:hailcolorbutton
 * @short_description: Implementation of the ATK interfaces for #HildonColorButton
 * @see_also: #GailButton, #HailColorSelector, #HildonColorButton
 *
 * #HailColorButton implements the required ATK interfaces of #HildonColorButton . #AtkValue
 * support included to let the user get the color using a 24 bit number. #AtkImage support
 * also included to show the currently selected color and the position and size of the
 * color image.
 */

#include <gtk/gtkdrawingarea.h>
#include <hildon-widgets/hildon-color-button.h>
#include "hailcolorbutton.h"


typedef struct _HailColorButtonPrivate HailColorButtonPrivate;

struct _HailColorButtonPrivate {
  gchar * exposed_text;    /* AtkImage exposed description */
  GtkWidget * drawing_area;  /* color drawing area */
};

#define HAIL_COLOR_BUTTON_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_COLOR_BUTTON, HailColorButtonPrivate))
static void   hail_color_button_class_init       (HailColorButtonClass *klass);
static void   hail_color_button_object_init      (HailColorButton      *color_button);

static void   hail_color_button_real_initialize  (AtkObject       *obj,
						    gpointer        data);

static G_CONST_RETURN gchar* hail_color_button_get_name         (AtkObject       *obj);


/* AtkValue.h */
static void	    hail_color_button_atk_value_interface_init	 (AtkValueIface  *iface);
static void	    hail_color_button_get_current_value (AtkValue       *obj,
							   GValue         *value);
static void	    hail_color_button_get_maximum_value (AtkValue       *obj,
							   GValue         *value);
static void	    hail_color_button_get_minimum_value (AtkValue       *obj,
							   GValue         *value);
static gboolean	    hail_color_button_set_current_value (AtkValue       *obj,
							   const GValue   *value);

/* AtkImage.h */
static void                  hail_color_button_atk_image_interface_init   (AtkImageIface  *iface);
static G_CONST_RETURN gchar* hail_color_button_get_image_description (AtkImage       *image);
static void	             hail_color_button_get_image_position (AtkImage       *image,
								   gint	        *x,
								   gint	        *y,
								   AtkCoordType   coord_type);
static void                  hail_color_button_get_image_size (AtkImage       *image,
							       gint           *width,
							       gint           *height);


static GType parent_type;
static GtkAccessibleClass* parent_class = NULL;

/**
 * hail_color_button_get_type:
 * 
 * Initialises, and returns the type of a hail color_button.
 * 
 * Return value: GType of #HailColorButton.
 **/
GType
hail_color_button_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailColorButtonClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) hail_color_button_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        (guint16) sizeof (HailColorButton), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) hail_color_button_object_init, /* instance init */
        NULL /* value table */
      };

      static const GInterfaceInfo atk_image_info =
      {
        (GInterfaceInitFunc) hail_color_button_atk_image_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      static const GInterfaceInfo atk_value_info =
      {
        (GInterfaceInitFunc) hail_color_button_atk_value_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_BUTTON);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailColorButton", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_IMAGE,
                                   &atk_image_info);

      g_type_add_interface_static (type, ATK_TYPE_VALUE,
                                   &atk_value_info);

    }

  return type;
}

static void
hail_color_button_class_init (HailColorButtonClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailColorButtonPrivate));

  /* bind virtual methods for AtkObject */
  class->initialize = hail_color_button_real_initialize;
  class->get_name = hail_color_button_get_name;

}

static void
hail_color_button_object_init (HailColorButton      *color_button)
{
  HailColorButtonPrivate * priv = NULL;

  priv = HAIL_COLOR_BUTTON_GET_PRIVATE(color_button);
  priv->exposed_text = NULL;
  priv->drawing_area = NULL;
}


/**
 * hail_color_button_new:
 * @widget: a #HildonColorButton casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonColorButton.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_color_button_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_COLOR_BUTTON (widget), NULL);

  object = g_object_new (HAIL_TYPE_COLOR_BUTTON, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method initialize() */
static void
hail_color_button_real_initialize (AtkObject *obj,
                             gpointer   data)
{
  HailColorButtonPrivate * priv = NULL;
  GtkWidget * widget = NULL;
  GtkWidget * alignment = NULL;

  priv = HAIL_COLOR_BUTTON_GET_PRIVATE(obj);

  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_PUSH_BUTTON;

  widget = GTK_ACCESSIBLE(obj)->widget;
  g_return_if_fail (GTK_IS_BUTTON(widget));
  alignment = gtk_bin_get_child(GTK_BIN(widget));
  g_return_if_fail (GTK_IS_BIN(alignment));
  priv->drawing_area = gtk_bin_get_child(GTK_BIN(alignment));

}

/* this internal method is used to update the value of the exposed_text
 * attribute. It's the description shown in AtkImage get_image_description
 * method
 */
void
hail_color_button_update_text (HailColorButton * selector)
{
  HailColorButtonPrivate * priv = NULL;
  GtkWidget * widget = NULL;
  GdkColor * color = NULL;

  g_return_if_fail (HAIL_COLOR_BUTTON(selector));
  widget = GTK_ACCESSIBLE(selector)->widget;
  g_return_if_fail (HILDON_COLOR_BUTTON(widget));

  priv = HAIL_COLOR_BUTTON_GET_PRIVATE(selector);
  color = (GdkColor *) hildon_color_button_get_color(HILDON_COLOR_BUTTON(widget));
  if (priv->exposed_text != NULL)
    g_free(priv->exposed_text);
  /* html format of color*/
  priv->exposed_text = g_strdup_printf("Color #%02x%02x%02x", (color->red)&0xff, (color->green)&0xff, (color->blue)&0xff);
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_color_button_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_COLOR_BUTTON (obj), NULL);

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

      g_return_val_if_fail (HILDON_IS_COLOR_BUTTON (widget), NULL);

      /* It gets the name from the button label */
      name = gtk_button_get_label(GTK_BUTTON(widget));

    }
  return name;
}

/* initialize the AtkValue interface, binding methods */
static void	 
hail_color_button_atk_value_interface_init (AtkValueIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->get_current_value = hail_color_button_get_current_value;
  iface->get_maximum_value = hail_color_button_get_maximum_value;
  iface->get_minimum_value = hail_color_button_get_minimum_value;
  iface->set_current_value = hail_color_button_set_current_value;

}

/* Implementation of AtkValue method get_current_value() */
static void	 
hail_color_button_get_current_value (AtkValue		*obj,
				       GValue		*value)
{
  HailColorButtonPrivate * priv = NULL;
  GdkColor * color = NULL;
  GtkWidget * widget = NULL;

  g_return_if_fail (HAIL_IS_COLOR_BUTTON (obj));
  priv = HAIL_COLOR_BUTTON_GET_PRIVATE(obj);
  widget = GTK_ACCESSIBLE(obj)->widget;

  color = (GdkColor *) hildon_color_button_get_color(HILDON_COLOR_BUTTON(widget));

  g_value_init(value, G_TYPE_DOUBLE);
  /* compose the color as a 24 bits number */
  g_value_set_double(value, (gdouble) (((color->red)/256)*65536) +
		     (((color->green)/256)*256) +
		     (((color->blue)/256)));
}

/* Implementation of AtkValue method get_maximum_value() */
static void	 
hail_color_button_get_maximum_value (AtkValue		*obj,
				     GValue		*value)
{
  HailColorButtonPrivate * priv = NULL;

  g_return_if_fail (HAIL_IS_COLOR_BUTTON (obj));
  priv = HAIL_COLOR_BUTTON_GET_PRIVATE(obj);

  g_value_init(value, G_TYPE_DOUBLE);
  /* 2^24 - 1 because color is represented as a 24 bit number */
  g_value_set_double(value, (gdouble) 0xffffff);
}

/* Implementation of AtkValue method get_minimum_value() */
static void	 
hail_color_button_get_minimum_value (AtkValue		*obj,
				     GValue		*value)
{
  HailColorButtonPrivate * priv = NULL;

  g_return_if_fail (HAIL_IS_COLOR_BUTTON (obj));
  priv = HAIL_COLOR_BUTTON_GET_PRIVATE(obj);

  g_value_init(value, G_TYPE_DOUBLE);
  g_value_set_double(value, (gdouble) 0.0);
}

/* Implementation of AtkValue method set_current_value() */
static gboolean	 
hail_color_button_set_current_value (AtkValue		*obj,
				     const GValue	*value)
{
  /* you cannot set the value of the color button from atk */
  return FALSE;
}

/* AtkImage interface initialization. It binds the virtual methods */
static void
hail_color_button_atk_image_interface_init (AtkImageIface *iface)
{
  g_return_if_fail (iface != NULL);

  /* Binding of virtual methods of AtkImage */
  iface->get_image_description = hail_color_button_get_image_description;
  iface->get_image_position = hail_color_button_get_image_position;
  iface->get_image_size = hail_color_button_get_image_size;
}

/* Implementation of AtkImage method get_image_description() */
static G_CONST_RETURN gchar* 
hail_color_button_get_image_description (AtkImage *image)
{
  GtkWidget *widget = NULL;
  HailColorButtonPrivate * priv = NULL;
  gchar * description = NULL;

  g_return_val_if_fail (HAIL_IS_COLOR_BUTTON (image), NULL);
  
  priv = HAIL_COLOR_BUTTON_GET_PRIVATE(image);
  
  widget = GTK_ACCESSIBLE (image)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;
  
  g_return_val_if_fail (HAIL_IS_COLOR_BUTTON (image), NULL);
  
  /* It gets the name from the caption label */
  hail_color_button_update_text(HAIL_COLOR_BUTTON(image));
  description = priv->exposed_text;
  
  return description;
}

/* Implementation of AtkImage method get_image_position() */
static void
hail_color_button_get_image_position (AtkImage     *image,
				      gint	     *x,
				      gint	     *y,
				      AtkCoordType coord_type)
{
  HailColorButtonPrivate * priv = NULL;
  AtkObject * drawing_area_a = NULL;

  g_return_if_fail (HAIL_IS_COLOR_BUTTON(image));
  priv = HAIL_COLOR_BUTTON_GET_PRIVATE(image);
  g_return_if_fail (GTK_IS_DRAWING_AREA(priv->drawing_area));

  drawing_area_a = gtk_widget_get_accessible(priv->drawing_area);
  atk_component_get_position(ATK_COMPONENT(drawing_area_a), x, y, coord_type);
}

/* Implementation of AtkImage method get_image_size() */
static void
hail_color_button_get_image_size (AtkImage *image,
				 gint     *width,
				 gint     *height)
{
  HailColorButtonPrivate * priv = NULL;
  AtkObject * drawing_area_a = NULL;

  g_return_if_fail (HAIL_IS_COLOR_BUTTON(image));
  priv = HAIL_COLOR_BUTTON_GET_PRIVATE(image);
  g_return_if_fail (GTK_IS_DRAWING_AREA(priv->drawing_area));

  drawing_area_a = gtk_widget_get_accessible(priv->drawing_area);
  atk_component_get_size(ATK_COMPONENT(drawing_area_a), width, height);
}
