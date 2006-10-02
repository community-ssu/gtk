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
 * SECTION:hailcaption
 * @short_description: Implementation of the ATK interfaces for a #HildonCaption.
 * @see_also: #HildonCaption
 *
 * #HailCaption implements the required ATK interfaces of #HildonCaption. In particular it
 * exposes:
 * <itemizedlist>
 * <listitem>The embedded control. It gets a default name (<literal>control</literal>) in case
 * the developer does not provide a name to it.</listitem>
 * <listitem>The relationship with its label.</listitem>
 * <listitem>Whether it's mandatory or optional, through #ATK_STATE_REQUIRED</listitem>
 * <listitem>Action of clicking the label</listitem>
 * </itemizedlist>
 */

#include <gtk/gtkimage.h>
#include <hildon-widgets/hildon-caption.h>
#include "hailcaption.h"

static void                  hail_caption_class_init       (HailCaptionClass *klass);
static void                  hail_caption_object_init      (HailCaption      *caption);

static G_CONST_RETURN gchar* hail_caption_get_name         (AtkObject       *obj);
static AtkStateSet*          hail_caption_ref_state_set    (AtkObject       *obj);
static AtkRelationSet*       hail_caption_ref_relation_set (AtkObject       *accessible);

static void                  hail_caption_real_initialize  (AtkObject       *obj,
							    gpointer        data);
static void                  hail_caption_finalize         (GObject        *object);
static void                  hail_caption_gtk_container_add(GtkContainer   *container,
							    GtkWidget      *widget,
							    gpointer user_data);
static void                  hail_caption_child_set_name   (AtkObject     *accessible);



/* AtkImage.h */
static void                  atk_image_interface_init   (AtkImageIface  *iface);
static G_CONST_RETURN gchar* hail_caption_get_image_description 
                                                        (AtkImage       *image);
static void	             hail_caption_get_image_position
                                                        (AtkImage       *image,
                                                         gint	        *x,
                                                         gint	        *y,
                                                         AtkCoordType   coord_type);
static void                  hail_caption_get_image_size (AtkImage       *image,
                                                         gint           *width,
                                                         gint           *height);
static gboolean              hail_caption_set_image_description 
                                                        (AtkImage       *image,
                                                         const gchar    *description);

/* AtkAction.h */
static void                  hail_caption_atk_action_interface_init   (AtkActionIface  *iface);
static gint                  hail_caption_action_get_n_actions    (AtkAction       *action);
static gboolean              hail_caption_action_do_action        (AtkAction       *action,
								   gint            index);
static const gchar*          hail_caption_action_get_name         (AtkAction       *action,
								   gint            index);
static const gchar*          hail_caption_action_get_description  (AtkAction       *action,
								   gint            index);
static const gchar*          hail_caption_action_get_keybinding   (AtkAction       *action,
								   gint            index);



static GtkImage*             get_image_from_caption      (GtkWidget      *caption);
static void                  set_role_for_caption        (AtkObject      *accessible,
                                                         GtkWidget      *caption);

#define HAIL_CAPTION_CLICK_ACTION_NAME "click"
#define HAIL_CAPTION_CLICK_ACTION_DESCRIPTION "Click the caption"
#define HAIL_CAPTION_DEFAULT_CHILD_NAME "control"

static GType parent_type;
static GtkAccessibleClass* parent_class = NULL;

/**
 * hail_caption_get_type:
 * 
 * Initialises, and returns the type of a hail caption.
 * 
 * Return value: GType of #HailCaption.
 **/
GType
hail_caption_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailCaptionClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) hail_caption_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        (guint16) sizeof (HailCaption), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) hail_caption_object_init, /* instance init */
        NULL /* value table */
      };

      static const GInterfaceInfo atk_image_info =
      {
        (GInterfaceInitFunc) atk_image_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      static const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) hail_caption_atk_action_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_EVENT_BOX);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailCaption", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_IMAGE,
                                   &atk_image_info);

      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);

    }

  return type;
}

static void
hail_caption_class_init (HailCaptionClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_caption_finalize;

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods for AtkObject */
  class->get_name = hail_caption_get_name;
  class->ref_relation_set = hail_caption_ref_relation_set;
  class->ref_state_set = hail_caption_ref_state_set;
  class->initialize = hail_caption_real_initialize;

}

static void
hail_caption_object_init (HailCaption      *caption)
{
}


/**
 * hail_caption_new:
 * @widget: a #HildonCaption casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonCaption.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_caption_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_CAPTION (widget), NULL);

  object = g_object_new (HAIL_TYPE_CAPTION, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method ref_relation_set() */
AtkRelationSet *
hail_caption_ref_relation_set (AtkObject * obj)
{
  GtkWidget * widget = NULL;
  AtkRelationSet *relation_set = NULL;

  g_return_val_if_fail (HAIL_IS_CAPTION (obj), NULL);

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    return NULL;

  relation_set = ATK_OBJECT_CLASS (parent_class)->ref_relation_set (obj);

  /* adds the relation "the caption labels the embedded control" */
  if (!atk_relation_set_contains (relation_set, ATK_RELATION_LABEL_FOR)) {
    AtkObject * accessible_array[1];
    AtkRelation * relation = NULL;
    GtkWidget * child = NULL;

    child = gtk_bin_get_child(GTK_BIN(widget));
    if (child) {
      accessible_array[0] = gtk_widget_get_accessible (child);
      relation = atk_relation_new (accessible_array, 1, 
				   ATK_RELATION_LABEL_FOR);
      atk_relation_set_add (relation_set, relation);
      g_object_unref(relation);
    }
  }

  return relation_set;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_caption_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_CAPTION (obj), NULL);

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

      g_return_val_if_fail (HILDON_IS_CAPTION (widget), NULL);

      /* It gets the name from the caption label */
      name = hildon_caption_get_label(HILDON_CAPTION(widget));

    }
  return name;
}

/* Implementation of AtkObject method initialize() */
static void
hail_caption_real_initialize (AtkObject *obj,
                             gpointer   data)
{
  GtkWidget * widget = NULL;
  GtkWidget * child = NULL;
  AtkObject * child_a = NULL;
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  set_role_for_caption (obj, data);

  g_return_if_fail(HAIL_IS_CAPTION(obj));
  widget = GTK_ACCESSIBLE(obj)->widget;
  g_return_if_fail(GTK_IS_WIDGET(widget));

  g_signal_connect(widget, "add", (GCallback) hail_caption_gtk_container_add, obj);

  /* set a default accessible name to the caption child, if it does not provide
   * one */
  g_return_if_fail(GTK_IS_BIN(widget));
  child = gtk_bin_get_child (GTK_BIN(widget));
  child_a = gtk_widget_get_accessible(child);
  if(ATK_IS_OBJECT(child_a)) {
    hail_caption_child_set_name(child_a);
  }
  
}

/* Implementation of AtkObject method ref_state_set() */
static AtkStateSet*
hail_caption_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set = NULL;
  GtkWidget *widget = NULL;
  HildonCaption *caption = NULL;

  state_set = ATK_OBJECT_CLASS (parent_class)->ref_state_set (obj);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    return state_set;

  caption = HILDON_CAPTION (widget);

  if (GTK_WIDGET_STATE (widget) == GTK_STATE_ACTIVE) {
    atk_state_set_add_state (state_set, ATK_STATE_ARMED);
  }

  if (hildon_caption_get_status(caption)==HILDON_CAPTION_MANDATORY) {
    atk_state_set_add_state (state_set, ATK_STATE_REQUIRED);
  }

  return state_set;
}
/* AtkImage interface initialization. It binds the virtual methods */
static void
atk_image_interface_init (AtkImageIface *iface)
{
  g_return_if_fail (iface != NULL);

  /* Binding of virtual methods of AtkImage */
  iface->get_image_description = hail_caption_get_image_description;
  iface->get_image_position = hail_caption_get_image_position;
  iface->get_image_size = hail_caption_get_image_size;
  iface->set_image_description = hail_caption_set_image_description;
}

/* internal utility method to get the icon image of the caption */
static GtkImage*
get_image_from_caption (GtkWidget *caption)
{
  GtkWidget *image = NULL;

  image = hildon_caption_get_icon_image(HILDON_CAPTION(caption));
  if (GTK_IS_IMAGE (image))
    return GTK_IMAGE(image);
  else
    return NULL;
}

/* Implementation of AtkImage method get_image_description() */
static G_CONST_RETURN gchar* 
hail_caption_get_image_description (AtkImage *image) {

  GtkWidget *widget = NULL;
  GtkImage  *caption_image = NULL;
  AtkObject *obj = NULL;

  widget = GTK_ACCESSIBLE (image)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  caption_image = get_image_from_caption (widget);

  if (caption_image != NULL)
    {
      obj = gtk_widget_get_accessible (GTK_WIDGET (caption_image));
      return atk_image_get_image_description (ATK_IMAGE (obj));
    }
  else 
    return NULL;
}

/* Implementation of AtkImage method get_image_position() */
static void
hail_caption_get_image_position (AtkImage     *image,
				 gint	     *x,
				 gint	     *y,
				 AtkCoordType coord_type)
{
  GtkWidget *widget = NULL;
  GtkImage  *caption_image = NULL;
  AtkObject *obj = NULL;

  widget = GTK_ACCESSIBLE (image)->widget;

  if (widget == NULL)
    {
    /*
     * State is defunct
     */
      *x = G_MININT;
      *y = G_MININT;
      return;
    }

  caption_image = get_image_from_caption (widget);

  /* Extracts position from the image accessible object */
  if (caption_image != NULL)
    {
      obj = gtk_widget_get_accessible (GTK_WIDGET (caption_image));
      atk_component_get_position (ATK_COMPONENT (obj), x, y, coord_type); 
    }
  else
    {
      *x = G_MININT;
      *y = G_MININT;
    }
}

/* Implementation of AtkImage method get_image_size() */
static void
hail_caption_get_image_size (AtkImage *image,
			     gint     *width,
			     gint     *height)
{
  GtkWidget *widget = NULL;
  GtkImage  *caption_image = NULL;
  AtkObject *obj = NULL;

  widget = GTK_ACCESSIBLE (image)->widget;

  if (widget == NULL)
    {
    /*
     * State is defunct
     */
      *width = -1;
      *height = -1;
      return;
    }

  caption_image = get_image_from_caption (widget);

  /* get the image size from the icon image accessible object */
  if (caption_image != NULL)
    {
      obj = gtk_widget_get_accessible (GTK_WIDGET (caption_image));
      atk_image_get_image_size (ATK_IMAGE (obj), width, height); 
    }
  else
    {
      *width = -1;
      *height = -1;
    }
}

/* Implementation of AtkImage method set_image_description() 
 * It sets the image description of the embedded icon accessible
 * object
 */
static gboolean
hail_caption_set_image_description (AtkImage    *image,
                                   const gchar *description)
{
  GtkWidget *widget = NULL;
  GtkImage  *caption_image = NULL;
  AtkObject *obj = NULL;

  widget = GTK_ACCESSIBLE (image)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  caption_image = get_image_from_caption (widget);

  if (caption_image != NULL) 
    {
      obj = gtk_widget_get_accessible (GTK_WIDGET (caption_image));
      return atk_image_set_image_description (ATK_IMAGE (obj), description);
    }
  else 
    return FALSE;
}

/* Internal utility method to set the role of the object.
 * It should be called from object initialization
 */
static void
set_role_for_caption (AtkObject *accessible,
                     GtkWidget *caption)
{

  accessible->role =  ATK_ROLE_PANEL;
}

static void
hail_caption_finalize (GObject            *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* Initializes the AtkAction interface, and binds the virtual methods */
static void
hail_caption_atk_action_interface_init (AtkActionIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->do_action = hail_caption_action_do_action;
  iface->get_n_actions = hail_caption_action_get_n_actions;
  iface->get_name = hail_caption_action_get_name;
  iface->get_description = hail_caption_action_get_description;
  iface->get_keybinding = hail_caption_action_get_keybinding;
}

/* Implementation of AtkAction method get_n_actions */
static gint
hail_caption_action_get_n_actions    (AtkAction       *action)
{
  GtkWidget * caption = NULL;
  
  g_return_val_if_fail (HAIL_IS_CAPTION (action), 0);

  caption = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_CAPTION(caption), 0);

  return 1;
}

/* Implementation of AtkAction method do_action */
static gboolean
hail_caption_action_do_action        (AtkAction       *action,
				      gint            index)
{
  GtkWidget * caption = NULL;

  g_return_val_if_fail (HAIL_IS_CAPTION (action), FALSE);
  g_return_val_if_fail ((index == 0), FALSE);

  caption = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_CAPTION(caption), FALSE);

  g_signal_emit_by_name(caption, "activate");

  return TRUE;
}

/* Implementation of AtkAction method get_name */
static const gchar*
hail_caption_action_get_name         (AtkAction       *action,
				      gint            index)
{
  GtkWidget * caption = NULL;

  g_return_val_if_fail (HAIL_IS_CAPTION (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  caption = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_CAPTION(caption), NULL);

  return HAIL_CAPTION_CLICK_ACTION_NAME;

}

/* Implementation of AtkAction method get_description */
static const gchar*
hail_caption_action_get_description   (AtkAction       *action,
				       gint            index)
{
  GtkWidget * caption = NULL;

  g_return_val_if_fail (HAIL_IS_CAPTION (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  caption = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_CAPTION(caption), NULL);

  return HAIL_CAPTION_CLICK_ACTION_DESCRIPTION;

}

/* Implementation of AtkAction method get_keybinding */
static const gchar*
hail_caption_action_get_keybinding   (AtkAction       *action,
				      gint            index)
{
  GtkWidget * caption = NULL;

  g_return_val_if_fail (HAIL_IS_CAPTION (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  caption = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_CAPTION(caption), NULL);

  return NULL;
}

/* Handler for adding a child. It's used to give a default atk name
 * to the children. */
static void
hail_caption_gtk_container_add       (GtkContainer   *container,
				      GtkWidget      *widget,
				      gpointer user_data)
{
  AtkObject * accessible = NULL;

  g_return_if_fail (HILDON_IS_CAPTION(container));

  g_return_if_fail (GTK_IS_WIDGET(widget));
  accessible = gtk_widget_get_accessible(widget);
  g_return_if_fail(ATK_IS_OBJECT(accessible));

  hail_caption_child_set_name(accessible);
}

/* This method sets a default name for the caption child if it
 * hasn't got anyone before */
static void
hail_caption_child_set_name          (AtkObject     *accessible)
{
  g_return_if_fail(ATK_IS_OBJECT(accessible));

  if (atk_object_get_name(accessible) == NULL) {
    atk_object_set_name(accessible, HAIL_CAPTION_DEFAULT_CHILD_NAME);
  }
}
