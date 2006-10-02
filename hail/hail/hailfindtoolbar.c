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
 * SECTION:hailfindtoolbar
 * @short_description: Implementation of the ATK interfaces for a #HildonFindToolbar.
 * @see_also: #GailContainer, #HildonFindToolbar
 * 
 * #HailFindToolbar implements the required ATK interfaces of #HildonFindToolbar,
 * delegating in its parent class #GailContainer for most of the behaviour.But
 * it redefines some things:
 * <itemizedlist>
 * <listitem>It gives ATK significative names to the toolbar buttons close and find.</listitem>
 * </itemizedlist>
 */

#include <string.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktoolbutton.h>
#include <hildon-widgets/hildon-find-toolbar.h>
#include "hailfindtoolbar.h"

static void                  hail_find_toolbar_class_init       (HailFindToolbarClass *klass);

static G_CONST_RETURN gchar* hail_find_toolbar_get_name         (AtkObject       *obj);
static void                  hail_find_toolbar_real_initialize  (AtkObject       *obj,
								 gpointer        data);

#define HAIL_FIND_TOOLBAR_DEFAULT_NAME "Find toolbar"


static GType parent_type;
static GtkAccessibleClass* parent_class = NULL;

/**
 * hail_find_toolbar_get_type:
 *
 * Initialises, and returns the type of a #HailFindToolbar.
 * Return value: GType of #HailFindToolbar.
 */
GType
hail_find_toolbar_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailFindToolbarClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) hail_find_toolbar_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        (guint16) sizeof (HailFindToolbar), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) NULL, /* instance init */
        NULL /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_TOOLBAR);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailFindToolbar", &tinfo, 0);

    }

  return type;
}

static void
hail_find_toolbar_class_init (HailFindToolbarClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods for AtkObject */
  class->get_name = hail_find_toolbar_get_name;
  class->initialize = hail_find_toolbar_real_initialize;

}


/**
 * hail_find_toolbar_new:
 * @widget: a #HailFindToolbar casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonFindToolbar.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_find_toolbar_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_FIND_TOOLBAR (widget), NULL);

  object = g_object_new (HAIL_TYPE_FIND_TOOLBAR, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_find_toolbar_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_FIND_TOOLBAR (obj), NULL);

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

      g_return_val_if_fail (HILDON_IS_FIND_TOOLBAR (widget), NULL);

      name = HAIL_FIND_TOOLBAR_DEFAULT_NAME;

    }
  return name;
}

/* Implementation of AtkObject method initialize()
 * It sets the role, and then assigns a name for the toolbars which
 * are not showing a significant identifier in ATK (the buttons).
 */
static void hail_find_toolbar_real_initialize (AtkObject *obj,
					       gpointer data)
{
  GtkWidget * toolbar = NULL;
  GList * children = NULL;

  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  toolbar = GTK_ACCESSIBLE (obj)->widget;
  g_return_if_fail (HILDON_IS_FIND_TOOLBAR(toolbar));

  /* set up role */
  obj->role =  ATK_ROLE_TOOL_BAR;

  /* iterate over the widgets */
  for (children = gtk_container_get_children(GTK_CONTAINER(toolbar)); 
       children != NULL; 
       children = g_list_next(children)) {
    GtkWidget * child = children->data;

    /* it gives significant names to the internal widgets */
    if (GTK_IS_TOOL_BUTTON(child)) {
      AtkObject * child_accessible = NULL;
      GtkWidget * button = NULL;
      AtkObject * button_accessible = NULL;
      gchar * label = NULL;
      
      label = (gchar *) gtk_tool_button_get_label(GTK_TOOL_BUTTON(child));
      if ((label != NULL)&&(strcmp(label, "Find")==0)) {
	child_accessible = gtk_widget_get_accessible(child);
	if (ATK_IS_OBJECT(child_accessible)&&(atk_object_get_name(child_accessible)==NULL)) {
	  atk_object_set_name(child_accessible,label);
	}
	button = gtk_bin_get_child(GTK_BIN(child));
	if (GTK_IS_BUTTON(button)) {
	  button_accessible = gtk_widget_get_accessible(button);
	  if (ATK_IS_OBJECT(button_accessible)&&(atk_object_get_name(child_accessible)==NULL)) {
	    atk_object_set_name(button_accessible,label);
	  }
	}
      } else if ((label != NULL) && (strcmp(label, "Close")==0)) {
	child_accessible = gtk_widget_get_accessible(child);
	if (ATK_IS_OBJECT(child_accessible)&&(atk_object_get_name(child_accessible)==NULL)) {
	  atk_object_set_name(child_accessible,label);
	}
	button = gtk_bin_get_child(GTK_BIN(child));
	if (GTK_IS_BUTTON(button)) {
	  button_accessible = gtk_widget_get_accessible(button);
	  if (ATK_IS_OBJECT(button_accessible)&&(atk_object_get_name(child_accessible)==NULL)) {
	    atk_object_set_name(button_accessible,label);
	  }
	}
      }
    }
  } /* for */
}
