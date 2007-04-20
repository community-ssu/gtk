/* HAIL - The GNOME Accessibility Implementation Library
 * Copyright (C) 2005 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* This file includes some macros handling the Atk factory initialization
 * for Hail classes. Really it defines the accessible factory classes. These
 * are called from hail initialization in hail.c
 */

#ifndef _HAIL_FACTORY_H__
#define _HAIL_FACTORY_H__

/**
 * SECTION:hailfactory
 * @short_description: Declaration templates for factory methods in Hail
 *
 * This class stores some utility methods for initialization and definition
 * of factories of hail.
 */

#include <glib.h>
#include <gtk/gtkwidget.h>

/**
 * HAIL_ACCESSIBLE_FACTORY:
 * @type: type id of the Hail object
 * @type_as_function: left part of the factory instanciator, used in
 * HAIL_WIDGET_SET_FACTORY
 * @opt_create_accessible: constructor of the accessible object.
 *
 * Definition of a factory for a Hail object.
 */
#define HAIL_ACCESSIBLE_FACTORY(type, type_as_function, opt_create_accessible)	\
										\
static GType									\
type_as_function ## _factory_get_accessible_type (void)				\
{										\
  return type;									\
}										\
										\
static AtkObject*								\
type_as_function ## _factory_create_accessible (GObject *obj)			\
{										\
  GtkWidget *widget = NULL;								\
  AtkObject *accessible = NULL;							\
										\
  g_return_val_if_fail (GTK_IS_WIDGET (obj), NULL);				\
										\
  widget = GTK_WIDGET (obj);							\
										\
  accessible = opt_create_accessible (widget);					\
										\
  return accessible;								\
}										\
										\
static void									\
type_as_function ## _factory_class_init (AtkObjectFactoryClass *klass)		\
{										\
  klass->create_accessible   = type_as_function ## _factory_create_accessible;	\
  klass->get_accessible_type = type_as_function ## _factory_get_accessible_type;\
}										\
										\
static GType									\
type_as_function ## _factory_get_type (void)					\
{										\
  static GType t = 0;								\
										\
  if (!t)									\
  {										\
    char *name;									\
    static const GTypeInfo tinfo =						\
    {										\
      (guint16) sizeof (AtkObjectFactoryClass),					\
      NULL, NULL, (GClassInitFunc) type_as_function ## _factory_class_init,	\
      NULL, NULL, (guint16) sizeof (AtkObjectFactory), 0, NULL, NULL		\
    };										\
										\
    name = g_strconcat (g_type_name (type), "Factory", NULL);			\
    t = g_type_register_static (						\
	    ATK_TYPE_OBJECT_FACTORY, name, &tinfo, 0);				\
    g_free (name);								\
  }										\
										\
  return t;									\
}


/**
 * HAIL_WIDGET_SET_FACTORY:
 * @widget_type: type id of the widget the factory provides accessible objects
 * @type_as_function: left part of the factory methods defined.
 *
 * Method to set the hail factory for a widget
 */
#define HAIL_WIDGET_SET_FACTORY(widget_type, type_as_function)			\
	atk_registry_set_factory_type (atk_get_default_registry (),		\
				       widget_type,				\
				       type_as_function ## _factory_get_type ())

/**
 * HAIL_WIDGET_SET_EXTERNAL_FACTORY:
 * @widget_type: type id of the widget the factory provides accessible objects
 * @factory_name: object name of the factory, as registered in GObject.
 *
 * Method to set an accessible factory for a widget from the
 * object name of the factory.
 */
#define HAIL_WIDGET_SET_EXTERNAL_FACTORY(widget_type, factory_name)                \
        atk_registry_set_factory_type (atk_get_default_registry (),                \
				       widget_type,                                \
                                       g_type_from_name(factory_name))

#endif /* _HAIL_FACTORY_H__ */
