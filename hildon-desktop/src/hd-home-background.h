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

#ifndef __HD_HOME_BACKGROUND_H__
#define __HD_HOME_BACKGROUND_H__

#include <glib-object.h>
#include <gdk/gdkwindow.h>

G_BEGIN_DECLS

typedef struct _HDHomeBackground HDHomeBackground;
typedef struct _HDHomeBackgroundClass HDHomeBackgroundClass;
typedef struct _HDHomeBackgroundPrivate HDHomeBackgroundPrivate;

#define HD_TYPE_HOME_BACKGROUND                 (hd_home_background_get_type ())
#define HD_HOME_BACKGROUND(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_HOME_BACKGROUND, HDHomeBackground))
#define HD_HOME_BACKGROUND_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_HOME_BACKGROUND, HDHomeBackgroundClass))
#define HD_IS_HOME_BACKGROUND(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_HOME_BACKGROUND))
#define HD_IS_HOME_BACKGROUND_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_HOME_BACKGROUND))
#define HD_HOME_BACKGROUND_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_HOME_BACKGROUND, HDHomeBackgroundClass))

#define HD_HOME_BACKGROUND_NO_IMAGE             "no-image"

struct _HDHomeBackground
{
  GObject                       parent;

  HDHomeBackgroundPrivate  *priv;

};

struct _HDHomeBackgroundClass
{
  GObjectClass                  parent_class; 

};

typedef void    (*HDHomeBackgroundApplyCallback)  (HDHomeBackground *bg,
                                                   gint              pixmap_xid,
                                                   GError           *error,
                                                   gpointer          user_data);

GType       hd_home_background_get_type         (void) G_GNUC_CONST;
void        hd_home_background_save             (HDHomeBackground *background,
                                                 const gchar      *filename,
                                                 GError          **error);
void        hd_home_background_load             (HDHomeBackground *background,
                                                 const gchar      *filename,
                                                 GError          **error);
void        hd_home_background_apply            (HDHomeBackground *background,
                                                 GdkWindow        *window,
                                                 GdkRectangle     *region,
                                                 GError          **error);
void        hd_home_background_apply_async      (HDHomeBackground *background,
                                                 GdkWindow        *window,
                                                 GdkRectangle     *region,
                                                 HDHomeBackgroundApplyCallback
                                                                   cb,
                                                 gpointer          user_data);

HDHomeBackground *
            hd_home_background_copy             (const HDHomeBackground *src);

G_END_DECLS

#endif /* __HD_HOME_BACKGROUND_H__ */
