/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2005,2006 Nokia Corporation.
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
 * SECTION:hailweekdaypicker
 * @short_description: Implementation of the ATK interfaces for #HildonWeekdayPicker.
 * @see_also: #HildonWeekdayPicker
 *
 * #HailWeekdayPicker implements the required ATK interfaces of #HildonWeekdayPicker and its
 * children (the buttons inside the widget). It exposes:
 * <itemizedlist>
 * <listitem>The weekday buttons, in the locale order.</listitem>
 * </itemizedlist>
 */

#include <gtk/gtktogglebutton.h>
#include <hildon-widgets/hildon-weekday-picker.h>
#include "hailweekdaypicker.h"


#define HAIL_WEEKDAY_PICKER_DEFAULT_NAME "Weekday picker"

typedef struct _HailWeekdayPickerPrivate HailWeekdayPickerPrivate;

struct _HailWeekdayPickerPrivate {
  GPtrArray * accessible_buttons;    /* Array of accessible buttons for each day */
};

#define HAIL_WEEKDAY_PICKER_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_WEEKDAY_PICKER, HailWeekdayPickerPrivate))

static void                  hail_weekday_picker_class_init       (HailWeekdayPickerClass *klass);
static void                  hail_weekday_picker_object_init      (HailWeekdayPicker      *weekday_picker);

static G_CONST_RETURN gchar* hail_weekday_picker_get_name         (AtkObject       *obj);
static gint                  hail_weekday_picker_get_n_children   (AtkObject       *obj);
static AtkObject*            hail_weekday_picker_ref_child        (AtkObject       *obj,
								   gint            i);
static void                  hail_weekday_picker_real_initialize  (AtkObject       *obj,
								   gpointer        data);

static void                  add_weekday_buttons                  (GtkWidget * widget,
								   gpointer data);
static GType parent_type;
static GtkAccessible * parent_class = NULL;

/**
 * hail_weekday_picker_get_type:
 * 
 * Initialises, and returns the type of a hail weekday_picker.
 * 
 * Return value: GType of #HailWeekdayPicker.
 **/
GType
hail_weekday_picker_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailWeekdayPickerClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) hail_weekday_picker_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        (guint16) sizeof (HailWeekdayPicker), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) hail_weekday_picker_object_init, /* instance init */
        NULL /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_CONTAINER);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailWeekdayPicker", &tinfo, 0);

    }

  return type;
}

static void
hail_weekday_picker_class_init (HailWeekdayPickerClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailWeekdayPickerPrivate));

  /* bind virtual methods for AtkObject */
  class->get_name = hail_weekday_picker_get_name;
  class->get_n_children = hail_weekday_picker_get_n_children;
  class->ref_child = hail_weekday_picker_ref_child;
  class->initialize = hail_weekday_picker_real_initialize;

}

static void
hail_weekday_picker_object_init (HailWeekdayPicker      *weekday_picker)
{
  HailWeekdayPickerPrivate * priv = NULL;

  priv = HAIL_WEEKDAY_PICKER_GET_PRIVATE(weekday_picker);
  /* the array will store the weekdays in the expected order */
  priv->accessible_buttons = g_ptr_array_sized_new(7);
}


/**
 * hail_weekday_picker_new:
 * @widget: a #HildonWeekdayPicker casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonWeekdayPicker.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_weekday_picker_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_WEEKDAY_PICKER (widget), NULL);

  object = g_object_new (HAIL_TYPE_WEEKDAY_PICKER, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_weekday_picker_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_WEEKDAY_PICKER (obj), NULL);

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

      g_return_val_if_fail (HILDON_IS_WEEKDAY_PICKER (widget), NULL);

      /* It gets the name from the weekday_picker label */
      name = HAIL_WEEKDAY_PICKER_DEFAULT_NAME;

    }
  return name;
}

/* callback for gtk_container_forall. It's used from atk object initialize
 * to get the references to the AtkObjects of the internal buttons */
static void
add_weekday_buttons (GtkWidget * widget,
		     gpointer data)
{
  HailWeekdayPicker * picker = NULL;
  AtkObject * accessible_child = NULL;
  HailWeekdayPickerPrivate *priv = NULL;

  g_return_if_fail (HAIL_IS_WEEKDAY_PICKER(data));
  picker = HAIL_WEEKDAY_PICKER(data);
  priv = HAIL_WEEKDAY_PICKER_GET_PRIVATE(picker);
  if (GTK_IS_TOGGLE_BUTTON(widget)) {
    accessible_child = gtk_widget_get_accessible(widget);
    g_ptr_array_add(priv->accessible_buttons, accessible_child);
  }
}

/* Implementation of AtkObject method initialize() */
static void
hail_weekday_picker_real_initialize (AtkObject *obj,
                             gpointer   data)
{
  GtkWidget * widget = NULL;
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_PANEL;

  widget = GTK_ACCESSIBLE (obj)->widget;
  gtk_container_forall(GTK_CONTAINER(widget), add_weekday_buttons, obj);

}

/* Implementation of AtkObject method get_n_children() */
static gint                  
hail_weekday_picker_get_n_children   (AtkObject       *obj)
{
  GtkWidget *widget = NULL;

  g_return_val_if_fail (HAIL_IS_WEEKDAY_PICKER (obj), 0);

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  /* The seven days buttons */
  return 7;

}

/* Implementation of AtkObject method ref_child() */
static AtkObject*            
hail_weekday_picker_ref_child        (AtkObject       *obj,
				      gint            i)
{
  GtkWidget *widget = NULL;
  AtkObject * accessible_child = NULL;
  HailWeekdayPickerPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_WEEKDAY_PICKER (obj), 0);
  priv = HAIL_WEEKDAY_PICKER_GET_PRIVATE(HAIL_WEEKDAY_PICKER(obj));
  g_return_val_if_fail ((i >= 0)&&(i<7), NULL);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;


  accessible_child = g_ptr_array_index(priv->accessible_buttons,i);
  g_return_val_if_fail (ATK_IS_OBJECT(accessible_child), NULL);

  g_object_ref(accessible_child);
  return accessible_child;
}
