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
 * SECTION:hailappview
 * @short_description: Implementation of the ATK interfaces for a #HildonAppView.
 * @see_also: #HildonAppView
 * 
 * #HailAppView implements the required ATK interfaces of #HildonAppView. In particular it
 * exposes:
 * <itemizedlist>
 * <listitem>The embedded control, the toolbar and the menu.</listitem>
 * <listitem>The fullscreen button through #AtkAction</listitem>
 * </itemizedlist>
 */

#include <gdk/gdkkeysyms.h>
#include <gtk/gtkbin.h>
#include <gtk/gtkwidget.h>
#include <hildon-widgets/hildon-appview.h>
#include <hildon-widgets/hildon-defines.h>
#include "hailappview.h"

static void                  hail_app_view_class_init       (HailAppViewClass *klass);

static G_CONST_RETURN gchar* hail_app_view_get_name         (AtkObject       *obj);
static gint                  hail_app_view_get_n_children   (AtkObject       *obj);
static AtkObject*            hail_app_view_ref_child        (AtkObject       *obj,
							     gint            i);
static void                  hail_app_view_real_initialize  (AtkObject       *obj,
							     gpointer        data);

/* AtkAction */
static void                  hail_app_view_atk_action_interface_init   (AtkActionIface  *iface);
static gint                  hail_app_view_action_get_n_actions    (AtkAction       *action);
static gboolean              hail_app_view_action_do_action        (AtkAction       *action,
							            gint            index);
static const gchar*          hail_app_view_action_get_name         (AtkAction       *action,
								    gint            index);
static const gchar*          hail_app_view_action_get_description  (AtkAction       *action,
								    gint            index);
static const gchar*          hail_app_view_action_get_keybinding   (AtkAction       *action,
								    gint            index);

#define HAIL_APP_VIEW_FULLSCREEN_ACTION_NAME "fullscreen"
#define HAIL_APP_VIEW_UNFULLSCREEN_ACTION_NAME "unfullscreen"
#define HAIL_APP_VIEW_FULLSCREEN_ACTION_DESCRIPTION "Go full screen"
#define HAIL_APP_VIEW_UNFULLSCREEN_ACTION_DESCRIPTION "Return from full screen";

static GType parent_type;
static GtkAccessibleClass* parent_class = NULL;

/**
 * hail_app_view_get_type:
 *
 * Initialises, and returns the type of a hail app_view.
 * Return value: GType of #HailAppView.
 */
GType
hail_app_view_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailAppViewClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) hail_app_view_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        (guint16) sizeof (HailAppView), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) NULL, /* instance init */
        NULL /* value table */
      };

      static const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) hail_app_view_atk_action_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_BIN);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailAppView", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);


    }

  return type;
}

static void
hail_app_view_class_init (HailAppViewClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods for AtkObject */
  class->get_name = hail_app_view_get_name;
  class->get_n_children = hail_app_view_get_n_children;
  class->ref_child = hail_app_view_ref_child;
  class->initialize = hail_app_view_real_initialize;

}


/**
 * hail_app_view_new:
 * @widget: a #HailAppView casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonAppView.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_app_view_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_APPVIEW (widget), NULL);

  object = g_object_new (HAIL_TYPE_APP_VIEW, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_app_view_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_APP_VIEW (obj), NULL);

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

      g_return_val_if_fail (HILDON_IS_APPVIEW (widget), NULL);

      name = hildon_appview_get_title(HILDON_APPVIEW(widget));

    }
  return name;
}

/* Implementation of AtkObject method initialize()
 * It sets the role
 */
static void hail_app_view_real_initialize (AtkObject *obj,
					   gpointer data)
{
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role =  ATK_ROLE_FRAME;

}

/* Implementation of AtkObject method get_n_children() 
 * At least the widget has the toolbar and the menu (2 widgets).
 * If there's a contained widget, there are 3 children then */
static gint
hail_app_view_get_n_children (AtkObject* obj)
{
  GtkWidget *widget = NULL;
  gint n_children_bin = 0;

  g_return_val_if_fail (HAIL_IS_APP_VIEW (obj), 0);

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  if (gtk_bin_get_child(GTK_BIN(widget)))
    n_children_bin = 1;
  else
    n_children_bin = 0;

  return n_children_bin + 2;
}

/* Implementation of AtkObject method ref_accessible_child() */
static AtkObject*
hail_app_view_ref_child (AtkObject *obj,
                       gint      i)
{
  GtkWidget *widget = NULL;
  AtkObject * accessible = NULL;

  g_return_val_if_fail (HAIL_IS_APP_VIEW (obj), 0);
  g_return_val_if_fail ((i >= 0), NULL);
  widget = GTK_ACCESSIBLE (obj)->widget;

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
    accessible = gtk_widget_get_accessible(HILDON_APPVIEW(widget)->vbox);
    g_object_ref(accessible);
    return accessible;
  } else {
    accessible = gtk_widget_get_accessible(GTK_WIDGET(hildon_appview_get_menu(HILDON_APPVIEW(widget))));
    g_object_ref(accessible);
    return accessible;
  }
  
}

/* Initializes the AtkAction interface, and binds the virtual methods */
static void
hail_app_view_atk_action_interface_init (AtkActionIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->do_action = hail_app_view_action_do_action;
  iface->get_n_actions = hail_app_view_action_get_n_actions;
  iface->get_name = hail_app_view_action_get_name;
  iface->get_description = hail_app_view_action_get_description;
  iface->get_keybinding = hail_app_view_action_get_keybinding;
}

/* Implementation of AtkAction method get_n_actions */
static gint
hail_app_view_action_get_n_actions    (AtkAction       *action)
{
  GtkWidget * app_view = NULL;

  g_return_val_if_fail (HAIL_IS_APP_VIEW (action), 0);

  app_view = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_APPVIEW(app_view), 0);

  /* If it's allowed to go fullscreen from keyboard, show the action */
  if (hildon_appview_get_fullscreen_key_allowed(HILDON_APPVIEW(app_view))) {
    return 1;
  } else {
    return 0;
  }
}

/* Implementation of AtkAction method do_action */
static gboolean
hail_app_view_action_do_action        (AtkAction       *action,
				       gint            index)
{
  GtkWidget * app_view = NULL;

  g_return_val_if_fail (HAIL_IS_APP_VIEW (action), FALSE);
  g_return_val_if_fail ((index == 0), FALSE);

  app_view = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_APPVIEW(app_view), FALSE);

  if (!hildon_appview_get_fullscreen_key_allowed(HILDON_APPVIEW(app_view))) {
    return FALSE;
  } else if (!hildon_appview_get_fullscreen(HILDON_APPVIEW(app_view))) {
    hildon_appview_set_fullscreen(HILDON_APPVIEW(app_view), TRUE);
  } else { 
    hildon_appview_set_fullscreen(HILDON_APPVIEW(app_view), FALSE);
  }
  return TRUE;
}

/* Implementation of AtkAction method get_name */
static const gchar*
hail_app_view_action_get_name         (AtkAction       *action,
				       gint            index)
{
  GtkWidget * app_view = NULL;

  g_return_val_if_fail (HAIL_IS_APP_VIEW (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  app_view = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_APPVIEW(app_view), NULL);

  if (!hildon_appview_get_fullscreen_key_allowed(HILDON_APPVIEW(app_view))) {
    return NULL;
  } else if (hildon_appview_get_fullscreen(HILDON_APPVIEW(app_view))) {
    return HAIL_APP_VIEW_UNFULLSCREEN_ACTION_NAME;
  } else { 
    return HAIL_APP_VIEW_FULLSCREEN_ACTION_NAME;
  }
}

/* Implementation of AtkAction method get_description */
static const gchar*
hail_app_view_action_get_description   (AtkAction       *action,
					gint            index)
{
  GtkWidget * app_view = NULL;

  g_return_val_if_fail (HAIL_IS_APP_VIEW (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  app_view = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_APPVIEW(app_view), NULL);

  if (!hildon_appview_get_fullscreen_key_allowed(HILDON_APPVIEW(app_view))) {
    return NULL;
  } else if (hildon_appview_get_fullscreen(HILDON_APPVIEW(app_view))) {
    return HAIL_APP_VIEW_UNFULLSCREEN_ACTION_DESCRIPTION;
  } else { 
    return HAIL_APP_VIEW_FULLSCREEN_ACTION_DESCRIPTION;
  }
}

/* Implementation of AtkAction method get_keybinding */
static const gchar*
hail_app_view_action_get_keybinding   (AtkAction       *action,
				       gint            index)
{
  GtkWidget * app_view = NULL;

  g_return_val_if_fail (HAIL_IS_APP_VIEW (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  app_view = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_APPVIEW(app_view), NULL);

  if (!hildon_appview_get_fullscreen_key_allowed(HILDON_APPVIEW(app_view))) {
    return NULL;
  } else {
    return gdk_keyval_name(HILDON_HARDKEY_FULLSCREEN);
  }
}
