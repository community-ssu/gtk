/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2007 Nokia Corporation.
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
 * SECTION:hailhomewindow
 * @short_description: Implementation of the ATK interfaces for a #HailHomeWindow.
 * @see_also: #HildonHomeWindow
 *
 * #HailHomeWindow implements the required ATK interfaces of #HailHomeWindow.
 * In particular it exposes:
 * <itemizedlist>
 * <listitem>The embedded control. It gets a default name (<literal>control</literal>).</listitem>
 * <listitem>The #HildonHomeTitleBar widget as a child.</listitem>
 * </itemizedlist>
 * A valid name and description is given by the superclass #HailDesktopWindow.
 */

#include <libhildondesktop/hildon-desktop-window.h>
#include <libhildondesktop/hildon-home-window.h>
#include "hailhomewindow.h"

static void                  hail_home_window_class_init       (HailHomeWindowClass *klass);
static void                  hail_home_window_object_init      (HailHomeWindow      *home_window);
static void                  hail_home_window_finalize         (GObject             *object);


static gint                  hail_home_window_get_n_children   (AtkObject *obj);
static AtkObject *           hail_home_window_ref_child        (AtkObject *obj,
                                                                gint       i);

static GType parent_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_home_window_get_type:
 * 
 * Initialises, and returns the type of a #HailHomeWindow.
 * 
 * Return value: GType of #HailHomeWindow.
 **/
GType
hail_home_window_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailHomeWindowClass),
        (GBaseInitFunc) NULL,      /* base init */
        (GBaseFinalizeFunc) NULL,  /* base finalize */
        (GClassInitFunc) hail_home_window_class_init,     /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL,                      /* class data */
        (guint16) sizeof (HailHomeWindow),                /* instance size */
        0,                         /* nb preallocs */
        (GInstanceInitFunc) hail_home_window_object_init, /* instance init */
        NULL                       /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), HILDON_TYPE_HOME_WINDOW);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailHomeWindow", &tinfo, 0);
    }

  return type;
}

static void
hail_home_window_class_init (HailHomeWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_home_window_finalize;

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods */
  class->get_n_children = hail_home_window_get_n_children;
  class->ref_child      = hail_home_window_ref_child;
}

static void
hail_home_window_object_init (HailHomeWindow *home_window)
{
}

static void
hail_home_window_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * hail_home_window_new:
 * @widget: a #HildonHomeWindow casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonHomeWindow.
 * 
 * Return value: An #AtkObject
 **/
AtkObject *
hail_home_window_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_HOME_WINDOW(widget), NULL);

  object = g_object_new (HAIL_TYPE_HOME_WINDOW, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * Implementation of AtkObject method get_name()
 */
/*
 * Implementation of AtkObject method get_n_children()
 * At least the widget has the titlebar widget (#HildonHomeTitleBar).
 */
static gint
hail_home_window_get_n_children (AtkObject *obj)
{
  GtkWidget *widget = NULL;
  gint n_children = 0;

  g_return_val_if_fail (HAIL_IS_HOME_WINDOW (obj), 0);

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    {
      /*
       * State is defunct
       */
      return 0;
    }

  /* any children from the parent */
  n_children = ATK_OBJECT_CLASS (parent_class)->get_n_children (obj);

  if (hildon_home_window_get_titlebar (HILDON_HOME_WINDOW (widget)))
    {
      n_children++;
    }

  return n_children;
}

/*
 * Implementation of AtkObject method ref_accessible_child()
 */
static AtkObject *
hail_home_window_ref_child (AtkObject *obj,
                            gint      i)
{
  AtkObject  *accessible;
  GtkWidget *widget;

  g_return_val_if_fail (HAIL_IS_HOME_WINDOW (obj), NULL);
  g_return_val_if_fail ((i >= 0), NULL);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    {
      return NULL;
    }

  if (((ATK_OBJECT_CLASS (parent_class)->get_n_children (obj)) == 0) && (i == 0))
    {
      /* take care if the parent has not the container exposed,
       * that is, the case of i equal to 0 and the accessible parent NULL.
       */
      i++;
    }

  if (i == 0)
    {
      /* the container of the superclass HildonDesktopWindow */
      accessible = ATK_OBJECT_CLASS (parent_class)->ref_child (obj, i);
    }
  else
    {
      /* the HildonHomeTitleBar widget */
      GtkWidget *titlebar =
        hildon_home_window_get_titlebar (HILDON_HOME_WINDOW (widget));

      if (titlebar != NULL)
        {
          accessible = gtk_widget_get_accessible (titlebar);
	  g_object_ref (accessible);
        }
    }

  return accessible;
}
