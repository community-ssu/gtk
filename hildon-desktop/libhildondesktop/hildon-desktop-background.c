/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan.bilien@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
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

#include "hildon-desktop-background.h"
#include <libhildondesktop/hildon-desktop-marshalers.h>
#include <gdk/gdk.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <gdk/gdkcolor.h>

enum
{
  PROP_FILENAME = 1,
  PROP_MODE,
  PROP_COLOR,
  PROP_CACHE
};

enum
{
  LOAD,
  SAVE,
  CANCEL,
  APPLY,
  APPLY_ASYNC,
  COPY,
  N_SIGNALS
};

static guint SIGNALS[N_SIGNALS];

struct _HildonDesktopBackgroundPrivate
{
  GdkColor                     *color;
  HildonDesktopBackgroundMode   mode;
  gchar                        *filename;
  gchar                        *cache;
};

G_DEFINE_TYPE (HildonDesktopBackground, hildon_desktop_background, G_TYPE_OBJECT);


static void hildon_desktop_background_finalize (GObject *object);

static void hildon_desktop_background_set_property (GObject       *object,
                                                    guint          property_id,
                                                    const GValue  *value,
                                                    GParamSpec    *pspec);

static void hildon_desktop_background_get_property (GObject       *object,
                                                    guint          property_id,
                                                    GValue  *value,
                                                    GParamSpec    *pspec);

static HildonDesktopBackground *
hildon_desktop_background_copy_real (HildonDesktopBackground *src);

GType
hildon_desktop_background_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0)
  {
    static const GEnumValue values[] = {
    { HILDON_DESKTOP_BACKGROUND_CENTERED,
      "HILDON_DESKTOP_BACKGROUND_CENTERED",
      "centered" },
    { HILDON_DESKTOP_BACKGROUND_SCALED,
      "HILDON_DESKTOP_BACKGROUND_SCALED",
      "scaled" },
    { HILDON_DESKTOP_BACKGROUND_STRETCHED,
      "HILDON_DESKTOP_BACKGROUND_STRETCHED",
      "stretched" },
    { HILDON_DESKTOP_BACKGROUND_TILED,
      "HILDON_DESKTOP_BACKGROUND_TILED",
      "tiled" },
    { HILDON_DESKTOP_BACKGROUND_CROPPED,
      "HILDON_DESKTOP_BACKGROUND_CROPPED",
      "cropped" },
    { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("HildonDesktopBackgroundMode", values);
  }
  return etype;
}

static void
hildon_desktop_background_init (HildonDesktopBackground *background)
{
  background->priv =
      G_TYPE_INSTANCE_GET_PRIVATE ((background),
                                   HILDON_DESKTOP_TYPE_BACKGROUND,
                                   HildonDesktopBackgroundPrivate);
}

static void
hildon_desktop_background_class_init (HildonDesktopBackgroundClass *klass)
{
  GObjectClass *object_class;
  GParamSpec   *pspec;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = hildon_desktop_background_finalize;
  object_class->set_property = hildon_desktop_background_set_property;
  object_class->get_property = hildon_desktop_background_get_property;

  klass->copy = hildon_desktop_background_copy_real;

  pspec = g_param_spec_string ("filename",
                               "filename",
                               "Image filename",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_FILENAME,
                                   pspec);

  pspec = g_param_spec_string ("cache",
                               "cache",
                               "Image local cache, for remote filesystems",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_CACHE,
                                   pspec);


  pspec = g_param_spec_enum ("mode",
                             "Mode",
                             "Background stretching mode",
                             HILDON_DESKTOP_TYPE_BACKGROUND_MODE,
                             HILDON_DESKTOP_BACKGROUND_CENTERED,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_MODE,
                                   pspec);

  pspec = g_param_spec_pointer ("color",
                                "Color",
                                "Background color",
                                G_PARAM_READWRITE);
  g_object_class_install_property (object_class,
                                   PROP_COLOR,
                                   pspec);

  SIGNALS[LOAD] =
      g_signal_new ("load",
                    G_OBJECT_CLASS_TYPE (object_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (HildonDesktopBackgroundClass,
                                     load),
                    NULL,
                    NULL,
                    g_cclosure_user_marshal_VOID__STRING_POINTER,
                    G_TYPE_NONE,
                    2,
                    G_TYPE_STRING,
                    G_TYPE_POINTER);

  SIGNALS[SAVE] =
      g_signal_new ("save",
                    G_OBJECT_CLASS_TYPE (object_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (HildonDesktopBackgroundClass,
                                     save),
                    NULL,
                    NULL,
                    g_cclosure_user_marshal_VOID__STRING_POINTER,
                    G_TYPE_NONE,
                    2,
                    G_TYPE_STRING,
                    G_TYPE_POINTER);

  SIGNALS[CANCEL] =
      g_signal_new ("cancel",
                    G_OBJECT_CLASS_TYPE (object_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (HildonDesktopBackgroundClass,
                                     cancel),
                    NULL,
                    NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE,
                    0);

  SIGNALS[APPLY] =
      g_signal_new ("apply",
                    G_OBJECT_CLASS_TYPE (object_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (HildonDesktopBackgroundClass,
                                     apply),
                    NULL,
                    NULL,
                    g_cclosure_user_marshal_VOID__POINTER_POINTER,
                    G_TYPE_NONE,
                    2,
                    GDK_TYPE_WINDOW,
                    G_TYPE_POINTER);

  SIGNALS[APPLY_ASYNC] =
      g_signal_new ("apply-async",
                    G_OBJECT_CLASS_TYPE (object_class),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (HildonDesktopBackgroundClass,
                                     apply_async),
                    NULL,
                    NULL,
                    g_cclosure_user_marshal_VOID__POINTER_POINTER_POINTER,
                    G_TYPE_NONE,
                    3,
                    GDK_TYPE_WINDOW,
                    G_TYPE_POINTER,
                    G_TYPE_POINTER);

  SIGNALS[COPY] =
      g_signal_new ("copy",
                    G_OBJECT_CLASS_TYPE (object_class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (HildonDesktopBackgroundClass,
                                     copy),
                    NULL,
                    NULL,
                    g_cclosure_user_marshal_OBJECT__VOID,
                    HILDON_DESKTOP_TYPE_BACKGROUND,
                    0);

  g_type_class_add_private (klass, sizeof (HildonDesktopBackgroundPrivate));
}

static void
hildon_desktop_background_finalize (GObject *object)
{
  HildonDesktopBackgroundPrivate  *priv;

  g_return_if_fail (HILDON_DESKTOP_IS_BACKGROUND (object));
  priv = HILDON_DESKTOP_BACKGROUND (object)->priv;

  g_free (priv->filename);
  priv->filename = NULL;

  g_free (priv->cache);
  priv->cache = NULL;

  if (priv->color)
    gdk_color_free (priv->color);
  priv->color = NULL;

}

static void
hildon_desktop_background_set_property (GObject       *object,
                                        guint          property_id,
                                        const GValue  *value,
                                        GParamSpec    *pspec)
{
  HildonDesktopBackgroundPrivate  *priv =
      HILDON_DESKTOP_BACKGROUND (object)->priv;

  switch (property_id)
  {
      case PROP_FILENAME:
          g_free (priv->filename);
          if (g_str_equal (g_value_get_string (value),
                           HILDON_DESKTOP_BACKGROUND_NO_IMAGE))
            priv->filename = g_strdup ("");
          else
            priv->filename = g_strdup (g_value_get_string (value));
          break;
      case PROP_CACHE:
          g_free (priv->cache);
          priv->cache = g_strdup (g_value_get_string (value));
          break;
      case PROP_MODE:
          priv->mode = g_value_get_enum (value);
          break;
      case PROP_COLOR:
          if (priv->color)
            gdk_color_free (priv->color);
          priv->color = gdk_color_copy (g_value_get_pointer (value));
          break;
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
hildon_desktop_background_get_property (GObject       *object,
                                        guint          property_id,
                                        GValue        *value,
                                        GParamSpec    *pspec)
{
  HildonDesktopBackgroundPrivate  *priv =
      HILDON_DESKTOP_BACKGROUND (object)->priv;

  switch (property_id)
  {
      case PROP_FILENAME:
          g_value_set_string (value, priv->filename);
          break;
      case PROP_CACHE:
          g_value_set_string (value, priv->cache);
          break;
      case PROP_MODE:
          g_value_set_enum (value, priv->mode);
          break;
      case PROP_COLOR:
          g_value_set_pointer (value, priv->color);
          break;
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static HildonDesktopBackground *
hildon_desktop_background_copy_real (HildonDesktopBackground *src)
{
  HildonDesktopBackgroundPrivate  *priv;
  HildonDesktopBackground         *dest;

  g_return_val_if_fail (HILDON_DESKTOP_IS_BACKGROUND (src), NULL);

  priv = src->priv;

  dest = g_object_new (HILDON_DESKTOP_TYPE_BACKGROUND,
                       "mode", priv->mode,
                       "color", priv->color,
                       "filename", priv->filename,
                       "cache", priv->cache,
                       NULL);

  return dest;

}

void
hildon_desktop_background_save (HildonDesktopBackground *background,
                                const gchar *filename,
                                GError **error)
{
  g_return_if_fail (HILDON_DESKTOP_IS_BACKGROUND (background));

  g_signal_emit (background,
                 SIGNALS[SAVE],
                 0,
                 filename,
                 error);
}

void
hildon_desktop_background_load (HildonDesktopBackground *background,
                                const gchar *filename,
                                GError **error)
{
  g_return_if_fail (HILDON_DESKTOP_IS_BACKGROUND (background));

  g_signal_emit (background,
                 SIGNALS[LOAD],
                 0,
                 filename,
                 error);
}

void
hildon_desktop_background_apply (HildonDesktopBackground *background,
                                 GdkWindow        *window,
                                 GError          **error)
{
  g_return_if_fail (HILDON_DESKTOP_IS_BACKGROUND (background));

  g_signal_emit (background,
                 SIGNALS[APPLY],
                 0,
                 window,
                 error);
}

void
hildon_desktop_background_apply_async (HildonDesktopBackground *background,
                                       GdkWindow               *window,
                                       HildonDesktopBackgroundApplyCallback
                                                                cb,
                                       gpointer                 user_data)
{
  g_return_if_fail (HILDON_DESKTOP_IS_BACKGROUND (background));

  g_signal_emit (background,
                 SIGNALS[APPLY_ASYNC],
                 0,
                 window,
                 cb,
                 user_data);
}


gboolean
hildon_desktop_background_equal (const HildonDesktopBackground *background1,
                          const HildonDesktopBackground *background2)
{
  HildonDesktopBackgroundPrivate      *priv1;
  HildonDesktopBackgroundPrivate      *priv2;

  g_return_val_if_fail (HILDON_DESKTOP_IS_BACKGROUND (background1) &&
                        HILDON_DESKTOP_IS_BACKGROUND (background2),
                        FALSE);

  priv1 = background1->priv;
  priv2 = background2->priv;

#define equal_or_null(s, t) ((!s && !t) || ((s && t) && g_str_equal (s,t)))
  return (equal_or_null (priv1->filename,         priv2->filename)        &&
          gdk_color_equal (priv1->color,          priv2->color)           &&
          priv1->mode == priv2->mode);
#undef equal_or_null

}

void
hildon_desktop_background_cancel (HildonDesktopBackground *background)
{
  g_return_if_fail (HILDON_DESKTOP_IS_BACKGROUND (background));

  g_signal_emit (background, SIGNALS[CANCEL], 0);

}

HildonDesktopBackground *
hildon_desktop_background_copy (HildonDesktopBackground *src)
{
  HildonDesktopBackground         *dest;

  g_return_val_if_fail (HILDON_DESKTOP_IS_BACKGROUND (src), NULL);

  g_signal_emit (src, SIGNALS[COPY], 0, &dest);

  return dest;

}

const gchar *
hildon_desktop_background_get_filename (HildonDesktopBackground *background)
{
  g_return_val_if_fail (HILDON_DESKTOP_IS_BACKGROUND (background), NULL);

  return background->priv->filename;
}
