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
 * @short_description: Implementation of the ATK interfaces for a #HailHomeArea.
 * @see_also: #HildonHomeArea
 *
 * #HailHomeArea implements the required ATK interfaces of #HailHomeArea.
 * In particular it exposes:
 * <itemizedlist>
 * <listitem>The embedded control. It gets a default name and a default description.</listitem>
 * </itemizedlist>
 */

#include <libhildondesktop/hildon-home-area.h>
#include "hailhomearea.h"

static void                  hail_home_area_class_init       (HailHomeAreaClass *klass);
static void                  hail_home_area_object_init      (HailHomeArea      *home_area);
static void                  hail_home_area_finalize         (GObject           *object);


static G_CONST_RETURN gchar *hail_home_area_get_name         (AtkObject *obj);
static G_CONST_RETURN gchar *hail_home_area_get_description  (AtkObject *obj);

#define HAIL_HOME_AREA_DEFAULT_NAME "Hildon Home Area"
#define HAIL_HOME_AREA_DEFAULT_DESCRIPTION "Area used as space for home applets and applications"

static GType parent_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_home_area_get_type:
 * 
 * Initialises, and returns the type of a #HailHomeArea.
 * 
 * Return value: GType of #HailHomeArea.
 **/
GType
hail_home_area_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailHomeAreaClass),
        (GBaseInitFunc) NULL,      /* base init */
        (GBaseFinalizeFunc) NULL,  /* base finalize */
        (GClassInitFunc) hail_home_area_class_init,     /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL,                      /* class data */
        (guint16) sizeof (HailHomeArea),                /* instance size */
        0,                         /* nb preallocs */
        (GInstanceInitFunc) hail_home_area_object_init, /* instance init */
        NULL                       /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), HILDON_TYPE_HOME_AREA);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailHomeArea", &tinfo, 0);
    }

  return type;
}

static void
hail_home_area_class_init (HailHomeAreaClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_home_area_finalize;

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods */
  class->get_name        = hail_home_area_get_name;
  class->get_description = hail_home_area_get_description;
}

static void
hail_home_area_object_init (HailHomeArea *home_area)
{
}

static void
hail_home_area_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * hail_home_area_new:
 * @widget: a #HildonHomeArea casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonHomeArea.
 * 
 * Return value: An #AtkObject
 **/
AtkObject *
hail_home_area_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_HOME_AREA(widget), NULL);

  object = g_object_new (HAIL_TYPE_HOME_AREA, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * Implementation of AtkObject method get_name()
 */
static G_CONST_RETURN gchar *
hail_home_area_get_name (AtkObject *obj)
{
/*REMOVE*/fprintf(stderr, "hail_home_area_get_name\n");
  G_CONST_RETURN gchar *name = NULL;

  g_return_val_if_fail (HAIL_IS_HOME_AREA (obj), NULL);

  /* parent name */
  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);

  /* HildonHomeArea name */
  if (name == NULL)
    {
      GtkWidget *widget = NULL;
      
      widget = GTK_ACCESSIBLE(obj)->widget;

      /* Since we are in the top class,
       * we'll use the class name for the a11y name
       * and not test for the type of the widget.
       */
/*       name = G_OBJECT_CLASS_NAME(G_OBJECT_CLASS(GTK_WIDGET_GET_CLASS(widget))); */
      name = HAIL_HOME_AREA_DEFAULT_NAME;
    }

  return name;
}

/*
 * Implementation of AtkObject method get_description()
 */
static G_CONST_RETURN gchar *
hail_home_area_get_description (AtkObject *obj)
{
  G_CONST_RETURN gchar *description = NULL;

  g_return_val_if_fail (HAIL_IS_HOME_AREA (obj), NULL);

  /* parent description */
  description = ATK_OBJECT_CLASS (parent_class)->get_description (obj);

  /* HildonHomeArea description */
  if (description == NULL)
    {
      /* get the default description */
      description = HAIL_HOME_AREA_DEFAULT_DESCRIPTION;
    }
  
  return description;
}
