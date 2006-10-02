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
 * SECTION:hailtimepicker
 * @short_description: Implementation of the ATK interfaces for #HildonTimePicker.
 * @see_also: #HildonTimePicker
 *
 * #HailTimePicker implements the required ATK interfaces of #HildonTimePicker and its
 * children (the buttons inside the widget). It exposes:
 * <itemizedlist>
 * <listitem>The action button of the dialog</listitem>
 * <listitem>The up/down buttons.</listitem>
 * <listitem>The labels.</listitem>
 * </itemizedlist>
 */

#include <gtk/gtkalignment.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkframe.h>
#include <gtk/gtktable.h>
#include <hildon-widgets/hildon-time-picker.h>
#include "hailtimepicker.h"

/* list of widgets holded inside the time picker */
enum {
  WIDGET_HOURS,
  WIDGET_HOURS_UP,
  WIDGET_HOURS_DOWN,
  WIDGET_MMINUTES,
  WIDGET_MMINUTES_UP,
  WIDGET_MMINUTES_DOWN,
  WIDGET_LMINUTES,
  WIDGET_LMINUTES_UP,
  WIDGET_LMINUTES_DOWN,
  WIDGET_AM_PM,
  WIDGET_ACTIONS_PANEL,
  WIDGET_COUNT,
};    

typedef struct _HailTimePickerPrivate HailTimePickerPrivate;

struct _HailTimePickerPrivate {
  AtkObject *accessible[WIDGET_COUNT]; /* holds the accessibles of all the widgets */
};

#define HAIL_TIME_PICKER_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_TIME_PICKER, HailTimePickerPrivate))

/* Widget names */
static char * time_picker_widget_names[WIDGET_COUNT+1] = {
  "Hours", "Hours up", "Hours down",
  "MMinutes", "MMinutes up", "MMinutes down",
  "LMinutes", "LMinutes up", "LMinutes down",
  "Set AM/PM",NULL,
  NULL
};

static void                  hail_time_picker_class_init       (HailTimePickerClass *klass);
static void                  hail_time_picker_object_init      (HailTimePicker      *time_picker);

static G_CONST_RETURN gchar* hail_time_picker_get_name         (AtkObject       *obj);
static gint                  hail_time_picker_get_n_children   (AtkObject       *obj);
static AtkObject*            hail_time_picker_ref_child        (AtkObject       *obj,
								gint i);
static void                  hail_time_picker_real_initialize  (AtkObject       *obj,
								gpointer        data);

static GType parent_type;
static GtkAccessibleClass* parent_class = NULL;

#define HAIL_TIME_PICKER_DEFAULT_NAME "Time selection dialog"




/**
 * hail_time_picker_get_type:
 * 
 * Initialises, and returns the type of a hail time_picker.
 * 
 * Return value: GType of #HailTimePicker.
 **/
GType
hail_time_picker_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
	{
	  (guint16) sizeof (HailTimePickerClass),
	  (GBaseInitFunc) NULL, /* base init */
	  (GBaseFinalizeFunc) NULL, /* base finalize */
	  (GClassInitFunc) hail_time_picker_class_init, /* class init */
	  (GClassFinalizeFunc) NULL, /* class finalize */
	  NULL, /* class data */
	  (guint16) sizeof (HailTimePicker), /* instance size */
	  0, /* nb preallocs */
	  (GInstanceInitFunc) hail_time_picker_object_init, /* instance init */
	  NULL /* value table */
	};

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_DIALOG);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;


      type = g_type_register_static (parent_type,
                                     "HailTimePicker", &tinfo, 0);

    }

  return type;
}

static void
hail_time_picker_class_init (HailTimePickerClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailTimePickerPrivate));

  /* bind virtual methods for AtkObject */
  class->get_name = hail_time_picker_get_name;
  class->ref_child = hail_time_picker_ref_child;
  class->get_n_children = hail_time_picker_get_n_children;
  class->initialize = hail_time_picker_real_initialize;

}


static void
hail_time_picker_object_init (HailTimePicker      *time_picker)
{
  HailTimePickerPrivate * priv = NULL;
  int i;
  priv = HAIL_TIME_PICKER_GET_PRIVATE(time_picker);

  for (i = 0; i < WIDGET_COUNT; i++) {
    priv->accessible[i] = NULL;
  }
}

/**
 * hail_time_picker_new:
 * @widget: a #HildonTimePicker casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonTimePicker.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_time_picker_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_TIME_PICKER (widget), NULL);

  object = g_object_new (HAIL_TYPE_TIME_PICKER, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_time_picker_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_PICKER (obj), NULL);

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

      g_return_val_if_fail (HILDON_IS_TIME_PICKER (widget), NULL);

      /* It gets the name from the time_picker label */
      name = HAIL_TIME_PICKER_DEFAULT_NAME;

    }
  return name;
}

/* Implementation of AtkObject method initialize() */
static void
hail_time_picker_real_initialize (AtkObject *obj,
                             gpointer   data)
{
  GtkWidget * time_picker = NULL;
  GtkWidget * internal_table = NULL;
  GList * vbox_children = NULL;
  GList * table_children = NULL;
  HailTimePickerPrivate * priv = NULL;
  gint i, limit;

  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_DIALOG;

  time_picker = GTK_ACCESSIBLE(obj)->widget;
  priv = HAIL_TIME_PICKER_GET_PRIVATE(HAIL_TIME_PICKER(obj));

  /* retrieve the internal table of the dialog */
  g_return_if_fail (GTK_IS_CONTAINER(GTK_DIALOG(time_picker)->vbox));
  vbox_children = gtk_container_get_children (GTK_CONTAINER(GTK_DIALOG(time_picker)->vbox));
  g_return_if_fail (vbox_children != NULL);

  g_return_if_fail (GTK_IS_ALIGNMENT(vbox_children->data));
  g_return_if_fail (GTK_IS_TABLE(gtk_bin_get_child(GTK_BIN(vbox_children->data))));
  internal_table = gtk_bin_get_child(GTK_BIN(vbox_children->data));

  /* go through the table to get the buttons and frames of the dialog box */
  for (table_children = gtk_container_get_children(GTK_CONTAINER(internal_table)); 
       table_children != NULL; 
       table_children = g_list_next(table_children)) {

    GtkWidget * internal_widget = NULL;
    guint x, y;

    internal_widget = GTK_WIDGET(table_children->data);
    gtk_container_child_get (GTK_CONTAINER(internal_table), internal_widget, "left-attach", &x, "top-attach", &y, NULL);
    switch (y) {
    case 0:
      switch (x) {
      case 1: /* hours up */
	if (GTK_IS_BUTTON(internal_widget)) {
	  priv->accessible[WIDGET_HOURS_UP] = gtk_widget_get_accessible(internal_widget);
	}
	break;
      case 3:
	if (GTK_IS_BUTTON(internal_widget)) {
	  priv->accessible[WIDGET_MMINUTES_UP] = gtk_widget_get_accessible(internal_widget);
	}
	break;
      case 4:
	if (GTK_IS_BUTTON(internal_widget)) {
	  priv->accessible[WIDGET_LMINUTES_UP] = gtk_widget_get_accessible(internal_widget);
	}
	break;
      default:
	break;
      }
      break;
    case 1:
      switch (x) {
      case 1: /* hours frame */
	if (GTK_IS_FRAME(internal_widget)) {
	  priv->accessible[WIDGET_HOURS] = gtk_widget_get_accessible(internal_widget);
	}
	break;
      case 3:
	if (GTK_IS_FRAME(internal_widget)) {
	  priv->accessible[WIDGET_MMINUTES] = gtk_widget_get_accessible(internal_widget);
	}
	break;
      case 4:
	if (GTK_IS_FRAME(internal_widget)) {
	  priv->accessible[WIDGET_LMINUTES] = gtk_widget_get_accessible(internal_widget);
	}
	break;
      case 0:
      case 5:
	priv->accessible[WIDGET_AM_PM] = gtk_widget_get_accessible(internal_widget);
	break;
      default:
	break;
      }
      break;
    case 2:
      switch (x) {
      case 1: /* hours down */
	if (GTK_IS_BUTTON(internal_widget)) {
	  priv->accessible[WIDGET_HOURS_DOWN] = gtk_widget_get_accessible(internal_widget);
	}
	break;
      case 3:
	if (GTK_IS_BUTTON(internal_widget)) {
	  priv->accessible[WIDGET_MMINUTES_DOWN] = gtk_widget_get_accessible(internal_widget);
	}
	break;
      case 4:
	if (GTK_IS_BUTTON(internal_widget)) {
	  priv->accessible[WIDGET_LMINUTES_DOWN] = gtk_widget_get_accessible(internal_widget);
	}
	break;
      default:
	break;
      }
      break;
    default:
      break;
    }
    
  } /* for widgets in table */

  priv->accessible[WIDGET_ACTIONS_PANEL] = gtk_widget_get_accessible(GTK_DIALOG(time_picker)->action_area);

  limit = WIDGET_COUNT;
  if (priv->accessible[WIDGET_AM_PM] == NULL)
    limit--;

  /* set names and parent */
  for (i = 0; i < limit; i++) {
    if (time_picker_widget_names[i])
      atk_object_set_name(priv->accessible[i], time_picker_widget_names[i]);
    atk_object_set_parent(priv->accessible[i], obj);
  }
}

/* Implementation of AtkObject method get_n_children */
static gint                  
hail_time_picker_get_n_children   (AtkObject       *obj)
{
  GtkWidget *widget = NULL;
  HailTimePickerPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_PICKER (obj), 0);
  priv = HAIL_TIME_PICKER_GET_PRIVATE(HAIL_TIME_PICKER(obj));

  ATK_OBJECT_CLASS (parent_class)->get_n_children (obj);

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  /* exclude one child if
   there's no am/pm part */

  if (priv->accessible[WIDGET_AM_PM] == NULL) {
    return WIDGET_COUNT - 1;
  } else {
    return WIDGET_COUNT ;
  }
      

}

/* Implementation of AtkObject method ref_child() */
static AtkObject*            
hail_time_picker_ref_child        (AtkObject       *obj,
				  gint            i)
{
  GtkWidget *widget = NULL;
  AtkObject * accessible_child = NULL;
  HailTimePickerPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_PICKER (obj), 0);
  priv = HAIL_TIME_PICKER_GET_PRIVATE(HAIL_TIME_PICKER(obj));
  g_return_val_if_fail ((i >= 0)&&(i < WIDGET_COUNT), NULL);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  accessible_child = priv->accessible[i];

  g_return_val_if_fail (ATK_IS_OBJECT(accessible_child), NULL);

  g_object_ref(accessible_child);
  return accessible_child;
}
