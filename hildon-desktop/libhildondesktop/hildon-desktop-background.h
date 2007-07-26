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

#ifndef __HildonDesktop_BACKGROUND_H__
#define __HildonDesktop_BACKGROUND_H__

#include <glib-object.h>
#include <gdk/gdkwindow.h>

G_BEGIN_DECLS

typedef struct _HildonDesktopBackground HildonDesktopBackground;
typedef struct _HildonDesktopBackgroundClass HildonDesktopBackgroundClass;
typedef struct _HildonDesktopBackgroundPrivate HildonDesktopBackgroundPrivate;

#define HILDON_DESKTOP_TYPE_BACKGROUND                 (hildon_desktop_background_get_type ())
#define HILDON_DESKTOP_BACKGROUND(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_DESKTOP_TYPE_BACKGROUND, HildonDesktopBackground))
#define HILDON_DESKTOP_BACKGROUND_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass),  HILDON_DESKTOP_TYPE_BACKGROUND, HildonDesktopBackgroundClass))
#define HILDON_DESKTOP_IS_BACKGROUND(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_DESKTOP_TYPE_BACKGROUND))
#define HILDON_DESKTOP_IS_BACKGROUND_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass),  HILDON_DESKTOP_TYPE_BACKGROUND))
#define HILDON_DESKTOP_BACKGROUND_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj),  HILDON_DESKTOP_TYPE_BACKGROUND, HildonDesktopBackgroundClass))

#define HILDON_DESKTOP_TYPE_BACKGROUND_MODE (hildon_desktop_background_mode_get_type ())

#define HILDON_DESKTOP_BACKGROUND_NO_IMAGE              "no-image"

typedef void    (*HildonDesktopBackgroundApplyCallback)  (HildonDesktopBackground *bg,
                                                   gint              pixmap_xid,
                                                   GError           *error,
                                                   gpointer          user_data);

typedef enum
{
  HILDON_DESKTOP_BACKGROUND_CENTERED,
  HILDON_DESKTOP_BACKGROUND_SCALED,
  HILDON_DESKTOP_BACKGROUND_STRETCHED,
  HILDON_DESKTOP_BACKGROUND_TILED,
  HILDON_DESKTOP_BACKGROUND_CROPPED

} HildonDesktopBackgroundMode;

struct _HildonDesktopBackground
{
  GObject                       parent;

  HildonDesktopBackgroundPrivate  *priv;

};

struct _HildonDesktopBackgroundClass
{
  GObjectClass          parent_class;

  void                  (*load) (HildonDesktopBackground       *bg,
                                 const gchar                   *file,
                                 GError                       **error);

  void                  (*save) (HildonDesktopBackground       *bg,
                                 const gchar                   *file,
                                 GError                       **error);

  void                  (*cancel) (HildonDesktopBackground       *bg);

  void                  (*apply) (HildonDesktopBackground      *bg,
                                  GdkWindow                    *window,
                                  GError                      **error);

  void                  (*apply_async) (HildonDesktopBackground      *bg,
                                  GdkWindow                    *window,
                                  HildonDesktopBackgroundApplyCallback cb,
                                  gpointer                      user_data);
  HildonDesktopBackground *
                        (*copy) (HildonDesktopBackground *src);

};

GType       hildon_desktop_background_get_type         (void) G_GNUC_CONST;
GType       hildon_desktop_background_mode_get_type    (void) G_GNUC_CONST;
void        hildon_desktop_background_save      (HildonDesktopBackground *bg,
                                                 const gchar      *filename,
                                                 GError          **error);
void        hildon_desktop_background_load      (HildonDesktopBackground *bg,
                                                 const gchar      *filename,
                                                 GError          **error);
void        hildon_desktop_background_apply     (HildonDesktopBackground *bg,
                                                 GdkWindow        *window,
                                                 GError          **error);
void        hildon_desktop_background_apply_async
                                                (HildonDesktopBackground *bg,
                                                 GdkWindow        *window,
                                                 HildonDesktopBackgroundApplyCallback
                                                                   cb,
                                                 gpointer          user_data);

HildonDesktopBackground *
            hildon_desktop_background_copy      (HildonDesktopBackground *src);

gboolean    hildon_desktop_background_equal     (const HildonDesktopBackground *bg1,
                                                 const HildonDesktopBackground *bg2);

void        hildon_desktop_background_cancel    (HildonDesktopBackground *bg);

const gchar *hildon_desktop_background_get_filename
                                                (HildonDesktopBackground *bg);

G_END_DECLS

#endif /* __HildonDesktop_BACKGROUND_H__ */
