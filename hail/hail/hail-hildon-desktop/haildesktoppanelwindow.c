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
 * SECTION:haildesktoppanelwindow
 * @short_description: Implementation of the ATK interfaces for a #HailDesktopPanelwindow.
 * @see_also: #HildonDesktopPanelWindow
 *
 * #HailDesktopPanelWindow implements the required ATK interfaces of #HailDesktopPanelWindow.
 * In particular it exposes:
 * <itemizedlist>
 * <listitem>The orientation.</listitem>
 * </itemizedlist>
 */

#include <libhildondesktop/hildon-desktop-panel-window.h>
#include "haildesktoppanelwindow.h"

static void                  hail_desktop_panel_window_class_init       (HailDesktopPanelWindowClass *klass);
static void                  hail_desktop_panel_window_object_init      (HailDesktopPanelWindow      *desktop_panel_window);
static void                  hail_desktop_panel_window_finalize         (GObject                     *object);


static AtkStateSet *         hail_desktop_panel_window_ref_state_set    (AtkObject *obj);


static GType parent_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_desktop_panel_window_get_type:
 * 
 * Initialises, and returns the type of a #HailDesktopPanelWindow.
 * 
 * Return value: GType of #HailDesktopPanelWindow.
 **/
GType
hail_desktop_panel_window_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailDesktopPanelWindowClass),
        (GBaseInitFunc) NULL,      /* base init */
        (GBaseFinalizeFunc) NULL,  /* base finalize */
        (GClassInitFunc) hail_desktop_panel_window_class_init,     /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL,                      /* class data */
        (guint16) sizeof (HailDesktopPanelWindow),                 /* instance size */
        0,                         /* nb preallocs */
        (GInstanceInitFunc) hail_desktop_panel_window_object_init, /* instance init */
        NULL                       /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), HILDON_DESKTOP_TYPE_PANEL_WINDOW);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailDesktopPanelWindow", &tinfo, 0);
    }

  return type;
}

static void
hail_desktop_panel_window_class_init (HailDesktopPanelWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_desktop_panel_window_finalize;

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods */
  class->ref_state_set   = hail_desktop_panel_window_ref_state_set;
}

static void
hail_desktop_panel_window_object_init (HailDesktopPanelWindow *desktop_panel_window)
{
}

static void
hail_desktop_panel_window_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * hail_desktop_panel_window_new:
 * @widget: a #HildonDesktopPanelWindow casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonDesktopPanelWindow.
 * 
 * Return value: An #AtkObject
 **/
AtkObject *
hail_desktop_panel_window_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_DESKTOP_IS_PANEL_WINDOW(widget), NULL);

  object = g_object_new (HAIL_TYPE_DESKTOP_PANEL_WINDOW, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * Implementation of AtkObject method ref_state_set()
 */
static AtkStateSet *
hail_desktop_panel_window_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set = NULL;
  GtkWidget *widget = NULL;
  gint orientation;

  state_set = ATK_OBJECT_CLASS (parent_class)->ref_state_set (obj);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    {
      return state_set;
    }

  g_return_val_if_fail (HILDON_DESKTOP_IS_PANEL_WINDOW (widget), state_set);

  /* get from hildon-desktop-panel */
  g_object_get (G_OBJECT (widget),
		"orientation", &orientation,
		NULL);

  if (orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_HORIZONTAL)
    {
      atk_state_set_add_state(state_set, ATK_STATE_HORIZONTAL);
    }
  else if (orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_VERTICAL)
    {
      atk_state_set_add_state(state_set, ATK_STATE_VERTICAL);
    }

  return state_set;
}
