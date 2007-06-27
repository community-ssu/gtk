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
 * SECTION:haildesktoppopupwindow
 * @short_description: Implementation of the ATK interfaces for a #HailDesktopPopupWindow.
 * @see_also: #HildonDesktopPopupWindow.
 *
 * #HailDesktopPopupWindow implements the required ATK interfaces of #HailDesktopPopupWindow.
 * In particular it exposes:
 * <itemizedlist>
 * <listitem>The embedded control. It gets a default name and description.</listitem>
 * <listitem>The panes for the menus as a children (they are GtkWindows).</listitem>
 * <listitem>The orientation.</listitem>
 * </itemizedlist>
 */

#include <libhildondesktop/hildon-desktop-popup-window.h>
#include "haildesktoppopupwindow.h"

static void       hail_desktop_popup_window_class_init     (HailDesktopPopupWindowClass *klass);
static void       hail_desktop_popup_window_object_init    (HailDesktopPopupWindow      *popup_window);
static void       hail_desktop_popup_window_finalize       (GObject                     *object);

static G_CONST_RETURN gchar *hail_desktop_popup_window_get_name         (AtkObject      *obj);
static G_CONST_RETURN gchar *hail_desktop_popup_window_get_description  (AtkObject      *obj);

static gint       hail_desktop_popup_window_get_n_children (AtkObject *obj);
static AtkObject *hail_desktop_popup_window_ref_child      (AtkObject *obj,
							    gint       i);

static AtkStateSet *hail_desktop_popup_window_ref_state_set(AtkObject *obj);

static void       hail_desktop_popup_window_real_initialize(AtkObject *obj,
							    gpointer data);
static void       add_internal_widgets                     (GtkWidget *widget,
							    gpointer data);

#define HAIL_DESKTOP_POPUP_WINDOW_DEFAULT_NAME "Hildon Desktop Popup Window"
#define HAIL_DESKTOP_POPUP_WINDOW_DEFAULT_DESCRIPTION "The window which shows the popup menus"

typedef struct _HailDesktopPopupWindowPrivate HailDesktopPopupWindowPrivate;
struct _HailDesktopPopupWindowPrivate
{
  AtkObject **extra_panes;   /* the widgets */
  guint	      n_extra_panes; /* number of widget */
  guint	      index;         /* index to scan the array */
};

#define HAIL_DESKTOP_POPUP_WINDOW_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_DESKTOP_POPUP_WINDOW, HailDesktopPopupWindowPrivate))

static GType parent_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_desktop_popup_window_get_type:
 * 
 * Initialises, and returns the type of a #HailDesktopPopupWindow.
 * 
 * Return value: GType of #HailDesktopPopupWindow.
 **/
GType
hail_desktop_popup_window_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailDesktopPopupWindowClass),
        (GBaseInitFunc) NULL,      /* base init */
        (GBaseFinalizeFunc) NULL,  /* base finalize */
        (GClassInitFunc) hail_desktop_popup_window_class_init,     /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL,                      /* class data */
        (guint16) sizeof (HailDesktopPopupWindow),                 /* instance size */
        0,                         /* nb preallocs */
        (GInstanceInitFunc) hail_desktop_popup_window_object_init, /* instance init */
        NULL                       /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (),
					  HILDON_DESKTOP_TYPE_POPUP_WINDOW);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailDesktopPopupWindow", &tinfo, 0);
    }

  return type;
}

static void
hail_desktop_popup_window_class_init (HailDesktopPopupWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_desktop_popup_window_finalize;

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailDesktopPopupWindowPrivate));

  /* bind virtual methods */
  class->get_name        = hail_desktop_popup_window_get_name;
  class->get_description = hail_desktop_popup_window_get_description;
  class->get_n_children  = hail_desktop_popup_window_get_n_children;
  class->ref_child       = hail_desktop_popup_window_ref_child;
  class->ref_state_set   = hail_desktop_popup_window_ref_state_set;
  class->initialize      = hail_desktop_popup_window_real_initialize;
}

static void
hail_desktop_popup_window_object_init (HailDesktopPopupWindow *popup_window)
{
  HailDesktopPopupWindowPrivate *priv = NULL;

  g_return_if_fail (HAIL_IS_DESKTOP_POPUP_WINDOW (popup_window));
  priv = HAIL_DESKTOP_POPUP_WINDOW_GET_PRIVATE (popup_window);

  priv->extra_panes = NULL;
  priv->n_extra_panes = 0;
  priv->index = 0;
}

static void
hail_desktop_popup_window_finalize (GObject *object)
{
  HailDesktopPopupWindow *popup_window = NULL;
  HailDesktopPopupWindowPrivate *priv  = NULL;
  register int i;

  g_return_if_fail (HAIL_IS_DESKTOP_POPUP_WINDOW (object));
  popup_window = HAIL_DESKTOP_POPUP_WINDOW (object);
  priv = HAIL_DESKTOP_POPUP_WINDOW_GET_PRIVATE (popup_window);

  if (priv->extra_panes)
    {
      for (i = 0; i < priv->n_extra_panes; i++)
	g_object_unref (priv->extra_panes[i]);
      g_free (priv->extra_panes);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * hail_desktop_popup_window_new:
 * @widget: a #HildonDesktopPopupWindow casted as a #GtkWidget.
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonDesktopPopupWindow.
 * 
 * Return value: An #AtkObject.
 **/
AtkObject *
hail_desktop_popup_window_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_DESKTOP_IS_POPUP_WINDOW(widget), NULL);

  object = g_object_new (HAIL_TYPE_DESKTOP_POPUP_WINDOW, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * Implementation of AtkObject method get_name()
 */
static G_CONST_RETURN gchar *
hail_desktop_popup_window_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar *name = NULL;
  g_return_val_if_fail (HAIL_IS_DESKTOP_POPUP_WINDOW (obj), NULL);

  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);

  if (name == NULL)
    {
      /* popup window does not have title, so default name */
      name = HAIL_DESKTOP_POPUP_WINDOW_DEFAULT_NAME;
    }

  return name;
}

/*
 * Implementation of AtkObject method get_description()
 */
static G_CONST_RETURN gchar *
hail_desktop_popup_window_get_description (AtkObject *obj)
{
  G_CONST_RETURN gchar *description = NULL;

  g_return_val_if_fail (HAIL_IS_DESKTOP_POPUP_WINDOW (obj), NULL);

  description = ATK_OBJECT_CLASS (parent_class)->get_description (obj);
  if (description == NULL)
    {
      /*
       * Default description
       */
      description = HAIL_DESKTOP_POPUP_WINDOW_DEFAULT_DESCRIPTION;
    }

  return description;
}

/*
 * Implementation of AtkObject method get_n_children()
 * At least the widget has the titlebar widget (#HildonHomeTitleBar).
 */
static gint
hail_desktop_popup_window_get_n_children (AtkObject *obj)
{
  HailDesktopPopupWindow *popup_window = NULL;
  HailDesktopPopupWindowPrivate *priv  = NULL;
  gint n_children = 0;

  g_return_val_if_fail (HAIL_IS_DESKTOP_POPUP_WINDOW (obj), 0);

  popup_window = HAIL_DESKTOP_POPUP_WINDOW (obj);
  priv = HAIL_DESKTOP_POPUP_WINDOW_GET_PRIVATE (popup_window);

  if (priv->extra_panes)
    {
      n_children += priv->n_extra_panes;
    }

  return n_children;
}

/*
 * Implementation of AtkObject method ref_accessible_child()
 */
static AtkObject *
hail_desktop_popup_window_ref_child (AtkObject *obj,
				     gint       i)
{
  HailDesktopPopupWindow *popup_window = NULL;
  HailDesktopPopupWindowPrivate *priv  = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HAIL_IS_DESKTOP_POPUP_WINDOW (obj), NULL);
  g_return_val_if_fail ((i >= 0), NULL);

  popup_window = HAIL_DESKTOP_POPUP_WINDOW (obj);
  priv = HAIL_DESKTOP_POPUP_WINDOW_GET_PRIVATE (popup_window);

  if (priv->extra_panes != NULL)
    {
      accessible = priv->extra_panes[i];
      g_object_ref (accessible);
    }

  return accessible;
}

/*
 * Implementation of AtkObject method ref_state_set()
 */
static AtkStateSet *
hail_desktop_popup_window_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set = NULL;
  GtkWidget *widget = NULL;
  GtkOrientation orientation;

  g_return_val_if_fail (HAIL_IS_DESKTOP_POPUP_WINDOW (obj), NULL);

  state_set = ATK_OBJECT_CLASS (parent_class)->ref_state_set (obj);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    {
      return state_set;
    }

  g_return_val_if_fail (HILDON_DESKTOP_IS_POPUP_WINDOW (widget), state_set);

  /* get "orientation" property from hildon-desktop-popup-window */
  g_object_get (G_OBJECT (widget),
		"orientation", &orientation,
		NULL);

  switch (orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      atk_state_set_add_state(state_set, ATK_STATE_HORIZONTAL);
      break;

    case GTK_ORIENTATION_VERTICAL:
      atk_state_set_add_state(state_set, ATK_STATE_VERTICAL);
      break;
    }

  return state_set;
}

/*
 * Implementation of AtkObject method initialize().
 */
static void
hail_desktop_popup_window_real_initialize (AtkObject *obj, gpointer data)
{
  GtkWidget *popup_window = NULL;
  HailDesktopPopupWindowPrivate *priv  = NULL;

  g_return_if_fail (HAIL_IS_DESKTOP_POPUP_WINDOW (obj));
  priv = HAIL_DESKTOP_POPUP_WINDOW_GET_PRIVATE (obj);

  /* parent initialize */
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  popup_window = GTK_ACCESSIBLE(obj)->widget;

  if (HILDON_DESKTOP_IS_POPUP_WINDOW (popup_window))
    {
      gint panes = 0;

      g_object_get(G_OBJECT (popup_window),
		   "n-panes", &priv->n_extra_panes,
		   NULL);

      priv->extra_panes = g_new0 (AtkObject *, priv->n_extra_panes);
      priv->index = 0;

      gtk_container_forall(GTK_CONTAINER(popup_window),
			   add_internal_widgets, obj);
    }
}

/*
 * callback for gtk_container_forall.
 * It's used from atk object initialize to get the references
 * to the AtkObjects of the internal widgets.
 */
static void
add_internal_widgets (GtkWidget *widget, gpointer data)
{
  HailDesktopPopupWindow *popup_window = NULL;
  HailDesktopPopupWindowPrivate *priv  = NULL;

  popup_window = HAIL_DESKTOP_POPUP_WINDOW (data);
  priv = HAIL_DESKTOP_POPUP_WINDOW_GET_PRIVATE (popup_window);

  priv->extra_panes[priv->index] = gtk_widget_get_accessible (widget);
  priv->index++;
}
