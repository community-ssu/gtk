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
 * SECTION:hailhometitlebar
 * @short_description: Implementation of the ATK interfaces for a #HailHomeTitlebar.
 * @see_also: #HildonHomeTitlebar
 *
 * #HailHomeTitlebar implements the required ATK interfaces of #AtkObject.
 * In particular it exposes:
 * <itemizedlist>
 * <listitem>The embedded control. It gives a default name and description.
 * <listitem>Added the 'press' action within the titlebar for showing the menu.</listitem>
 * </itemizedlist>
 */

#include <gdk/gdk.h>
#include <libhildondesktop/hildon-home-titlebar.h>
#include "hailhometitlebar.h"

static void                  hail_home_titlebar_class_init       (HailHomeTitlebarClass *klass);
static void                  hail_home_titlebar_object_init      (HailHomeTitlebar      *home_titlebar);
static void                  hail_home_titlebar_finalize         (GObject               *object);

static G_CONST_RETURN gchar *hail_home_titlebar_get_name         (AtkObject *obj);
static G_CONST_RETURN gchar *hail_home_titlebar_get_description  (AtkObject *obj);

/* AtkAction.h */
static void                  hail_home_titlebar_atk_action_interface_init
                                                                 (AtkActionIface *iface);
static gint                  hail_home_titlebar_action_get_n_actions
                                                                 (AtkAction      *action);
static gboolean              hail_home_titlebar_action_do_action (AtkAction      *action,
                                                                  gint            index);
static const gchar *         hail_home_titlebar_action_get_name  (AtkAction      *action,
								  gint            index);
static const gchar *         hail_home_titlebar_action_get_description
                                                                 (AtkAction      *action,
								  gint            index);
static const gchar *         hail_home_titlebar_action_get_keybinding
                                                                 (AtkAction      *action,
								  gint            index);

#define HAIL_HOME_TITLEBAR_DEFAULT_NAME "Home Titlebar"
#define HAIL_HOME_TITLEBAR_DEFAULT_DESCRIPTION "Bar for showing titles and layout menu"
#define HAIL_HOME_TITLEBAR_ACTION_PRESS_NAME "press"
#define HAIL_HOME_TITLEBAR_ACTION_PRESS_DESCRIPTION "Synthesize press signal for showing the menu"

static GType parent_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_home_titlebar_get_type:
 * 
 * Initialises, and returns the type of a #HailHomeTitlebar.
 * 
 * Return value: GType of #HailHomeTitlebar.
 **/
GType
hail_home_titlebar_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailHomeTitlebarClass),
        (GBaseInitFunc) NULL,      /* base init */
        (GBaseFinalizeFunc) NULL,  /* base finalize */
        (GClassInitFunc) hail_home_titlebar_class_init,     /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL,                      /* class data */
        (guint16) sizeof (HailHomeTitlebar),                /* instance size */
        0,                         /* nb preallocs */
        (GInstanceInitFunc) hail_home_titlebar_object_init, /* instance init */
        NULL                       /* value table */
      };

      static const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) hail_home_titlebar_atk_action_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), HILDON_TYPE_HOME_TITLEBAR);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailHomeTitlebar", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);

    }

  return type;
}

static void
hail_home_titlebar_class_init (HailHomeTitlebarClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_home_titlebar_finalize;

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods for AtkObject */
  class->get_name        = hail_home_titlebar_get_name;
  class->get_description = hail_home_titlebar_get_description;
}

static void
hail_home_titlebar_object_init (HailHomeTitlebar *home_titlebar)
{
}

static void
hail_home_titlebar_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * hail_home_titlebar_new:
 * @widget: a #HildonHomeTitlebar casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #GtkHomeTitlebar.
 * 
 * Return value: An #AtkObject
 **/
AtkObject *
hail_home_titlebar_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_HOME_TITLEBAR(widget), NULL);

  object = g_object_new (HAIL_TYPE_HOME_TITLEBAR, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * Implementation of AtkObject method get_name()
 */
static G_CONST_RETURN gchar *
hail_home_titlebar_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar *name = NULL;
  GtkWidget *home_titlebar = NULL;

  g_return_val_if_fail (HAIL_IS_HOME_TITLEBAR (obj), NULL);
  home_titlebar = GTK_ACCESSIBLE(obj)->widget;

  /* parent name */
  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);

  if (name == NULL)
    {
      name = HAIL_HOME_TITLEBAR_DEFAULT_NAME;
    }

  return name;
}

/*
 * Implementation of AtkObject method get_description()
 */
static G_CONST_RETURN gchar *
hail_home_titlebar_get_description (AtkObject *obj)
{
  G_CONST_RETURN gchar *description = NULL;
  GtkWidget *home_titlebar = NULL;

  g_return_val_if_fail (HAIL_IS_HOME_TITLEBAR (obj), NULL);
  home_titlebar = GTK_ACCESSIBLE(obj)->widget;

  /* parent description */
  description = ATK_OBJECT_CLASS (parent_class)->get_description (obj);

  if (description == NULL)
    {
      description = HAIL_HOME_TITLEBAR_DEFAULT_DESCRIPTION;
    }

  return description;
}

/*
 * Initializes the AtkAction interface, and binds the virtual methods
 */
static void
hail_home_titlebar_atk_action_interface_init (AtkActionIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->get_n_actions   = hail_home_titlebar_action_get_n_actions;
  iface->do_action       = hail_home_titlebar_action_do_action;
  iface->get_name        = hail_home_titlebar_action_get_name;
  iface->get_description = hail_home_titlebar_action_get_description;
  iface->get_keybinding  = hail_home_titlebar_action_get_keybinding;
}

/*
 * Implementation of AtkAction method get_n_actions()
 */
static gint
hail_home_titlebar_action_get_n_actions (AtkAction *action)
{
  g_return_val_if_fail (HAIL_IS_HOME_TITLEBAR (action), 0);
  GtkWidget *home_titlebar = NULL;

  home_titlebar = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_HOME_TITLEBAR(home_titlebar), 0);

  /* expose the "press" */
  return 1;
}

/*
 * Implementation of AtkAction method do_action()
 */
static gboolean
hail_home_titlebar_action_do_action (AtkAction *action,
				     gint       index)
{
  GtkWidget *home_titlebar = NULL;

  g_return_val_if_fail (HAIL_IS_HOME_TITLEBAR (action), FALSE);

  home_titlebar = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_HOME_TITLEBAR (home_titlebar), FALSE);

  if (index == 0)
    {
      /* we are going to synthesize the "button-press-event"
       * over the HildonHomeTitlebar
       */
      HildonHomeTitlebarClass *klass = HILDON_HOME_TITLEBAR_GET_CLASS (home_titlebar);
      GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
      GdkEvent *event = gdk_event_new (GDK_BUTTON_PRESS);

      /*
       * need to assign button.[xy] for the GdkEventButton
       * as we are generating an event for this
       * widget GdkWindow itself we can use x=1 and y=1
       */
      event->button.x = event->button.y = 1;

      /* begin the process for showing the menu */
      widget_class->button_press_event(home_titlebar, event);
    }

  return TRUE;
}

/*
 * Implementation of AtkAction method get_name()
 */
static const gchar *
hail_home_titlebar_action_get_name (AtkAction *action,
				    gint       index)
{
  GtkWidget *home_titlebar = NULL;
  g_return_val_if_fail (HAIL_IS_HOME_TITLEBAR (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  home_titlebar = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_HOME_TITLEBAR(home_titlebar), NULL);

  return HAIL_HOME_TITLEBAR_ACTION_PRESS_NAME;
}

/*
 * Implementation of AtkAction method get_description()
 */
static const gchar *
hail_home_titlebar_action_get_description (AtkAction *action,
					   gint       index)
{
  GtkWidget *home_titlebar = NULL;

  g_return_val_if_fail (HAIL_IS_HOME_TITLEBAR (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);
  
  home_titlebar = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_HOME_TITLEBAR(home_titlebar), NULL);

  return HAIL_HOME_TITLEBAR_ACTION_PRESS_DESCRIPTION;
}

/* Implementation of AtkAction method get_keybinding */
static const gchar *
hail_home_titlebar_action_get_keybinding (AtkAction *action,
					  gint       index)
{
  GtkWidget *home_titlebar = NULL;

  g_return_val_if_fail (HAIL_IS_HOME_TITLEBAR (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  home_titlebar = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_HOME_TITLEBAR(home_titlebar), NULL);

  return NULL;
}
