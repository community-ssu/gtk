/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "hildon-desktop-window.h" 

#define HILDON_DESKTOP_WINDOW_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_WINDOW, HildonDesktopWindowPrivate))

G_DEFINE_TYPE (HildonDesktopWindow, hildon_desktop_window, GTK_TYPE_WINDOW);

enum {
  PROP_0,
  PROP_PADDING_LEFT,
  PROP_PADDING_RIGHT,
  PROP_PADDING_TOP,
  PROP_PADDING_BOTTOM
};

enum 
{
  SIGNAL_SELECT_PLUGINS,
  SIGNAL_SAVE,
  SIGNAL_LOAD,
  N_SIGNALS
};

static gint signals[N_SIGNALS];  

struct _HildonDesktopWindowPrivate 
{
  gint   padding_left;
  gint   padding_right;
  gint   padding_top;
  gint   padding_bottom;
};

static void 
hildon_desktop_window_finalize (GObject *object)
{
  HildonDesktopWindow *desktop_window;
  HildonDesktopWindowPrivate *priv;

  desktop_window = HILDON_DESKTOP_WINDOW (object);

  priv = HILDON_DESKTOP_WINDOW_GET_PRIVATE (desktop_window);

  if (desktop_window->container)
  {
    g_object_unref (desktop_window->container);
    desktop_window = NULL;
  }

  G_OBJECT_CLASS (hildon_desktop_window_parent_class)->finalize (object);
}

static void
hildon_desktop_window_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  HildonDesktopWindowPrivate *priv;
  
  priv = HILDON_DESKTOP_WINDOW_GET_PRIVATE (object);

  switch (prop_id)
  {
    case PROP_PADDING_LEFT:
      priv->padding_left = g_value_get_int (value);
      break;

    case PROP_PADDING_RIGHT:
      priv->padding_right = g_value_get_int (value);
      break;

    case PROP_PADDING_TOP:
      priv->padding_top = g_value_get_int (value);
      break;

    case PROP_PADDING_BOTTOM:
      priv->padding_bottom = g_value_get_int (value);
      break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hildon_desktop_window_get_property (GObject      *object,
                                    guint         prop_id,
                                    GValue       *value,
                                    GParamSpec   *pspec)
{
  HildonDesktopWindowPrivate *priv;
 
  priv = HILDON_DESKTOP_WINDOW_GET_PRIVATE (object);

  switch (prop_id)
  {
    case PROP_PADDING_LEFT:
      g_value_set_int (value, priv->padding_left);
      break;

    case PROP_PADDING_RIGHT:
      g_value_set_int (value, priv->padding_right);
      break;

    case PROP_PADDING_TOP:
      g_value_set_int (value, priv->padding_top);
      break;

    case PROP_PADDING_BOTTOM:
      g_value_set_int (value, priv->padding_bottom);
      break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void 
hildon_desktop_window_init (HildonDesktopWindow *desktop_window)
{
  desktop_window->priv = HILDON_DESKTOP_WINDOW_GET_PRIVATE (desktop_window);

  desktop_window->container = NULL;

  desktop_window->priv->padding_left = 0;
  desktop_window->priv->padding_right = 0;
  desktop_window->priv->padding_top = 0;
  desktop_window->priv->padding_bottom = 0;
}

static void
hildon_desktop_window_class_init (HildonDesktopWindowClass *desktopwindow_class)
{
  GObjectClass *g_object_class;
  GParamSpec *pspec;
  
  g_object_class = G_OBJECT_CLASS (desktopwindow_class);

  g_object_class->finalize     = hildon_desktop_window_finalize;
  g_object_class->set_property = hildon_desktop_window_set_property;
  g_object_class->get_property = hildon_desktop_window_get_property;

  signals[SIGNAL_SELECT_PLUGINS] =
        g_signal_new ("select-plugins",
                      G_OBJECT_CLASS_TYPE (g_object_class),
                      G_SIGNAL_RUN_FIRST,
		      G_STRUCT_OFFSET (HildonDesktopWindowClass, select_plugins),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

  signals[SIGNAL_SAVE] =
        g_signal_new ("save",
                      G_OBJECT_CLASS_TYPE (g_object_class),
                      G_SIGNAL_RUN_FIRST,
		      G_STRUCT_OFFSET (HildonDesktopWindowClass, save),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

  signals[SIGNAL_LOAD] =
        g_signal_new ("load",
                      G_OBJECT_CLASS_TYPE (g_object_class),
                      G_SIGNAL_RUN_FIRST,
		      G_STRUCT_OFFSET (HildonDesktopWindowClass, load),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

  pspec = g_param_spec_int ("padding-left",
                            "Padding Left",
                            "Padding Left",
                            0,
                            G_MAXINT,
                            0,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_property (g_object_class,
                                   PROP_PADDING_LEFT,
                                   pspec);

  pspec = g_param_spec_int ("padding-right",
                            "Padding Right",
                            "Padding Right",
                            0,
                            G_MAXINT,
                            0,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_property (g_object_class,
                                   PROP_PADDING_RIGHT,
                                   pspec);

  pspec = g_param_spec_int ("padding-top",
                            "Padding Top",
                            "Padding Top",
                            0,
                            G_MAXINT,
                            0,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_property (g_object_class,
                                   PROP_PADDING_TOP,
                                   pspec);

  pspec = g_param_spec_int ("padding-bottom",
                            "Padding Bottom",
                            "Padding Bottom",
                            0,
                            G_MAXINT,
                            0,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_property (g_object_class,
                                   PROP_PADDING_BOTTOM,
                                   pspec);

  g_type_class_add_private (g_object_class, sizeof (HildonDesktopWindowPrivate));
}

void 
hildon_desktop_window_set_sensitive (HildonDesktopWindow *window, gboolean sensitive)
{
/* TODO: */
}	

void 
hildon_desktop_window_set_focus (HildonDesktopWindow *window, gboolean focus)
{
/* TODO: */
}
