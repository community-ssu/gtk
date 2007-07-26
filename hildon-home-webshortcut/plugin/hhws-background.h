/*
 * This file is part of hildon-home-webshortcut
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

#ifndef __HHWS_BACKGROUND_H__
#define __HHWS_BACKGROUND_H__

#include <libhildondesktop/hildon-desktop-background.h>

G_BEGIN_DECLS

typedef struct _HHWSBackground HHWSBackground;
typedef struct _HHWSBackgroundClass HHWSBackgroundClass;
typedef struct _HHWSBackgroundPrivate HHWSBackgroundPrivate;

#define HHWS_TYPE_BACKGROUND                 (hhws_background_get_type ())
#define HHWS_BACKGROUND(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), HHWS_TYPE_BACKGROUND, HHWSBackground))
#define HHWS_BACKGROUND_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass),  HHWS_TYPE_BACKGROUND, HHWSBackgroundClass))
#define HHWS_IS_BACKGROUND(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HHWS_TYPE_BACKGROUND))
#define HHWS_IS_BACKGROUND_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass),  HHWS_TYPE_BACKGROUND))
#define HHWS_BACKGROUND_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj),  HHWS_TYPE_BACKGROUND, HHWSBackgroundClass))


struct _HHWSBackground
{
  HildonDesktopBackground        parent;

  HHWSBackgroundPrivate       *priv;

};

struct _HHWSBackgroundClass
{
  HildonDesktopBackgroundClass  parent_class;

};


void        hhws_background_register_type    (GTypeModule *module);
GType       hhws_background_get_type         (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __HHWS_BACKGROUND_H__ */
