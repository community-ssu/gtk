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
 * SECTION:hailwindow
 * @short_description: Implementation of the ATK interfaces for a #HildonWindow.
 * @see_also: #HildonWindow, #HildonProgram
 * 
 * #HailWindow implements the required ATK interfaces of #HildonWindow. In particular it
 * exposes, in this order, and depending on availability of them in the instance:
 * <itemizedlist>
 * <listitem>The embedded control.</listitem>
 * <listitem>The container of toolbars.</listitem>
 * <listitem>The currently active window menu (the #HildonWindow menu if there's one, or
 * the #HildonProgram common menu if not)</listitem>
 * </itemizedlist>
 */

#include <glib.h>
#include <atk/atkobject.h>
#include <gtk/gtkbin.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkbox.h>
#include <hildon-widgets/hildon-window.h>
#include <hildon-widgets/hildon-program.h>
#include "hailwindow.h"

typedef struct _HailWindowPrivate HailWindowPrivate;

struct _HailWindowPrivate {
  AtkObject *toolbars_box;  /* reference to the internal vbox */
};

#define HAIL_WINDOW_GET_PRIVATE(o)  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_WINDOW, HailWindowPrivate))

static void                  hail_window_class_init       (HailWindowClass *klass);
static void                  hail_window_object_init      (HailWindow      *window);

static G_CONST_RETURN gchar* hail_window_get_name         (AtkObject       *obj);
static gint                  hail_window_get_n_children   (AtkObject       *obj);
static AtkObject*            hail_window_ref_child        (AtkObject       *obj,
							     gint            i);
static void                  hail_window_real_initialize  (AtkObject       *obj,
							     gpointer        data);
static AtkObject *           hail_window_get_menu         (AtkObject * obj);

static GType parent_type;
static GtkAccessibleClass* parent_class = NULL;

/**
 * hail_window_get_type:
 *
 * Initialises, and returns the type of a hail window.
 * Return value: GType of #HailWindow.
 */
GType
hail_window_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailWindowClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) hail_window_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        (guint16) sizeof (HailWindow), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) hail_window_object_init, /* instance init */
        NULL /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_WINDOW);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailWindow", &tinfo, 0);


    }

  return type;
}

static void
hail_window_class_init (HailWindowClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailWindowPrivate));

  /* bind virtual methods for AtkObject */
  class->get_name = hail_window_get_name;
  class->get_n_children = hail_window_get_n_children;
  class->ref_child = hail_window_ref_child;
  class->initialize = hail_window_real_initialize;

}

/* nullify or initialize attributes */
static void
hail_window_object_init     (HailWindow        *window)
{
  HailWindowPrivate * priv = NULL;

  priv = HAIL_WINDOW_GET_PRIVATE(window);
  priv->toolbars_box = NULL;
}


/**
 * hail_window_new:
 * @widget: a #HailWindow casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonWindow.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_window_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_WINDOW (widget), NULL);

  object = g_object_new (HAIL_TYPE_WINDOW, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_window_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_WINDOW (obj), NULL);

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

      g_return_val_if_fail (HILDON_IS_WINDOW (widget), NULL);

      name = gtk_window_get_title(GTK_WINDOW(widget));

    }
  return name;
}

/* callback for gtk_container_forall. It's used from atk object initialize
 * to get the references to the AtkObjects of the internal buttons */
static void
add_internal_widgets (GtkWidget * widget,
		      gpointer data)
{
  HailWindow * accessible = NULL;
  AtkObject * accessible_child = NULL;
  HailWindowPrivate *priv = NULL;
  GtkWidget * window = NULL;
  GtkWidget * child = NULL;

  g_return_if_fail (HAIL_IS_WINDOW(data));
  accessible = HAIL_WINDOW(data);
  priv = HAIL_WINDOW_GET_PRIVATE(accessible);

  window = GTK_ACCESSIBLE(accessible)->widget;
  g_return_if_fail (HILDON_IS_WINDOW(window));
  child = gtk_bin_get_child(GTK_BIN(window));
  if ((widget != child)&&(GTK_IS_BOX(widget))) {
    accessible_child = gtk_widget_get_accessible(widget);
    g_return_if_fail (ATK_IS_OBJECT(accessible_child));
    if (priv->toolbars_box != NULL) {
      g_free(priv->toolbars_box);
    }
    priv->toolbars_box = accessible_child;
  }
}

/* Implementation of AtkObject method initialize()
 * It sets the role
 */
static void hail_window_real_initialize (AtkObject *obj,
					   gpointer data)
{
  GtkWidget * widget = NULL;
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  widget = GTK_ACCESSIBLE(obj)->widget;
  gtk_container_forall(GTK_CONTAINER(widget), add_internal_widgets, obj);
}

/* get the currently active menu for this window. It does not make a
 * reference to it, so, if used afterwards, it should be g_object_ref'ed
 */
static AtkObject *
hail_window_get_menu (AtkObject * obj)
{
  GtkWidget * window = NULL;
  GtkWidget * menu_to_use = NULL;
  AtkObject * accessible = NULL;
  g_return_val_if_fail (HAIL_IS_WINDOW(obj), NULL);

  window = GTK_ACCESSIBLE(obj)->widget;
  menu_to_use = GTK_WIDGET(hildon_window_get_menu(HILDON_WINDOW(window)));

  if (menu_to_use == NULL) {
    HildonProgram * program = NULL;

    program = hildon_program_get_instance();
    menu_to_use = GTK_WIDGET(hildon_program_get_common_menu(program));
  }

  if (menu_to_use != NULL) {
    accessible = gtk_widget_get_accessible(menu_to_use);
  }
  return accessible;
}

/* Implementation of AtkObject method get_n_children() 
 * At least the widget has the toolbar box. It can also have
 * a child and the menu.
 */
static gint
hail_window_get_n_children (AtkObject* obj)
{
  GtkWidget *widget = NULL;
  gint n_children = 0;

  g_return_val_if_fail (HAIL_IS_WINDOW (obj), 0);

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;
  
  n_children = 1;
  
  if (gtk_bin_get_child(GTK_BIN(widget)))
    n_children++;
  
  if (hail_window_get_menu(obj)!=NULL)
    n_children++;
  
  return n_children;
}
 
/* Implementation of AtkObject method ref_accessible_child() */
static AtkObject*
hail_window_ref_child (AtkObject *obj,
                       gint      i)
{
  GtkWidget *widget = NULL;
  AtkObject * accessible = NULL;
  HailWindowPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_WINDOW (obj), 0);
  g_return_val_if_fail ((i >= 0), NULL);
  widget = GTK_ACCESSIBLE (obj)->widget;
  priv = HAIL_WINDOW_GET_PRIVATE(obj);

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  /* If there's a child, of the container, it's the first returnable element. If
     it's not, then it decreases the i index in order to let the toolbar container
     and menu be accessible in correct order */
  if (gtk_bin_get_child(GTK_BIN(widget))) {
    if (i == 0) {
      accessible = gtk_widget_get_accessible(gtk_bin_get_child(GTK_BIN(widget)));
      g_object_ref (accessible);
      return (accessible);
    } else {
      i -= 1;
    }
  }
  if (i == 0){
    accessible = priv->toolbars_box;
    g_object_ref(accessible);
    return accessible;
  } else {
    accessible = hail_window_get_menu(obj);
    if (accessible != NULL) {
      g_object_ref(accessible);
      return accessible;
    }
  }
  return NULL;
}
