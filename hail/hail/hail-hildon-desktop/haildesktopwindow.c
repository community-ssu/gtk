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
 * SECTION:haildesktopwindow
 * @short_description: Implementation of the ATK interfaces for a #HailDesktopWindow.
 * @see_also: #HildonDesktopWindow
 *
 * #HailDesktopWindow implements the required ATK interfaces of #HailDesktopWindow.
 * In particular it exposes:
 * <itemizedlist>
 * <listitem>The embedded control. It gets a default name, based on the name of the class,
 * and a default description.</listitem>
 * </itemizedlist>
 */

#include <libhildondesktop/hildon-desktop-window.h>
#include "haildesktopwindow.h"

static void                  hail_desktop_window_class_init       (HailDesktopWindowClass *klass);
static void                  hail_desktop_window_object_init      (HailDesktopWindow      *desktop_window);
static void                  hail_desktop_window_finalize         (GObject                *object);


static G_CONST_RETURN gchar *hail_desktop_window_get_name         (AtkObject *obj);
static G_CONST_RETURN gchar *hail_desktop_window_get_description  (AtkObject *obj);

#define HAIL_DESKTOP_WINDOW_DEFAULT_DESCRIPTION "Each one of the windows embedded in the desktop"

static GType parent_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_desktop_window_get_type:
 * 
 * Initialises, and returns the type of a #HailDesktopWindow.
 * 
 * Return value: GType of #HailDesktopWindow.
 **/
GType
hail_desktop_window_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailDesktopWindowClass),
        (GBaseInitFunc) NULL,      /* base init */
        (GBaseFinalizeFunc) NULL,  /* base finalize */
        (GClassInitFunc) hail_desktop_window_class_init,     /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL,                      /* class data */
        (guint16) sizeof (HailDesktopWindow),                /* instance size */
        0,                         /* nb preallocs */
        (GInstanceInitFunc) hail_desktop_window_object_init, /* instance init */
        NULL                       /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), HILDON_DESKTOP_TYPE_WINDOW);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailDesktopWindow", &tinfo, 0);
    }

  return type;
}

static void
hail_desktop_window_class_init (HailDesktopWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_desktop_window_finalize;

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods */
  class->get_name        = hail_desktop_window_get_name;
  class->get_description = hail_desktop_window_get_description;
}

static void
hail_desktop_window_object_init (HailDesktopWindow *desktop_window)
{
}

static void
hail_desktop_window_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * hail_desktop_window_new:
 * @widget: a #HildonDesktopWindow casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonDesktopWindow.
 * 
 * Return value: An #AtkObject
 **/
AtkObject *
hail_desktop_window_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_DESKTOP_IS_WINDOW(widget), NULL);

  object = g_object_new (HAIL_TYPE_DESKTOP_WINDOW, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * Implementation of AtkObject method get_name()
 */
static G_CONST_RETURN gchar *
hail_desktop_window_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar *name = NULL;

  g_return_val_if_fail (HAIL_IS_DESKTOP_WINDOW (obj), NULL);

  /* parent name */
  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);

  /* HildonDesktopWindow name */
  if (name == NULL)
    {
      GtkWidget *widget = NULL;
      
      widget = GTK_ACCESSIBLE(obj)->widget;

      /* Since we are in the top class,
       * we'll use the class name for the a11y name
       * and not test for the type of the widget.
       */
      name = G_OBJECT_CLASS_NAME(G_OBJECT_CLASS(GTK_WIDGET_GET_CLASS(widget)));
    }

  return name;
}

/*
 * Implementation of AtkObject method get_description()
 */
static G_CONST_RETURN gchar *
hail_desktop_window_get_description (AtkObject *obj)
{
  G_CONST_RETURN gchar *description = NULL;

  g_return_val_if_fail (HAIL_IS_DESKTOP_WINDOW (obj), NULL);

  /* parent description */
  description = ATK_OBJECT_CLASS (parent_class)->get_description (obj);

  /* HildonDesktopWindow description */
  if (description == NULL)
    {
      /* get the default description */
      description = HAIL_DESKTOP_WINDOW_DEFAULT_DESCRIPTION;
    }
  
  return description;
}
