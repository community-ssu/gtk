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

#ifndef __HBM_BACKGROUND_H__
#define __HBM_BACKGROUND_H__

#include <libhildondesktop/hildon-desktop-background.h>
#include <X11/extensions/Xrender.h>

G_BEGIN_DECLS

typedef struct _HBMBackground HBMBackground;
typedef struct _HBMBackgroundClass HBMBackgroundClass;
typedef struct _HBMBackgroundPrivate HBMBackgroundPrivate;

#define HBM_TYPE_BACKGROUND                 (hbm_background_get_type ())
#define HBM_BACKGROUND(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), HBM_TYPE_BACKGROUND, HBMBackground))
#define HBM_BACKGROUND_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass),  HBM_TYPE_BACKGROUND, HBMBackgroundClass))
#define HBM_IS_BACKGROUND(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HBM_TYPE_BACKGROUND))
#define HBM_IS_BACKGROUND_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass),  HBM_TYPE_BACKGROUND))
#define HBM_BACKGROUND_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj),  HBM_TYPE_BACKGROUND, HBMBackgroundClass))

#define HBM_BACKGROUND_NO_IMAGE             "no-image"

struct _HBMBackground
{
  HildonDesktopBackground       parent;

  HBMBackgroundPrivate         *priv;

};

struct _HBMBackgroundClass
{
  HildonDesktopBackgroundClass  parent_class;

};


GType           hbm_background_get_type         (void) G_GNUC_CONST;
Picture         hbm_background_render           (HBMBackground *background,
                                                 GError       **error);

G_END_DECLS

#endif /* __HBM_BACKGROUND_H__ */
