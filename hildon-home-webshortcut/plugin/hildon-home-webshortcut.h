/*
 * This file is part of hildon-home-webshortcut
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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

#ifndef __HILDON_HOME_WEBSHORTCUT_H__
#define __HILDON_HOME_WEBSHORTCUT_H__

#include "hhwsloader.h"
#include <libhildondesktop/hildon-home-applet.h>

G_BEGIN_DECLS

#define HILDON_TYPE_HHWS            (hhws_get_type ())
#define HHWS(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_HHWS, Hhws))
#define HHWS_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass),  HILDON_TYPE_HHWS, HhwsClass))
#define HILDON_IS_HHWS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_IS_HHWS))
#define HILDON_IS_HHWS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HILDON_TYPE_HHWS))
#define HHWS_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj),  HILDON_TYPE_HHWS, HhwsClass))


typedef struct _HhwsPrivate HhwsPrivate;
typedef struct
{
  HildonHomeApplet          parent;

  HhwsPrivate              *priv;

} Hhws;

typedef struct
{
  HildonHomeAppletClass     parent_class;

} HhwsClass;

GType       hhws_get_type                           (void);

G_END_DECLS
#endif
