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

#ifndef __STATUSBAR_ITEM_BINDING_H__
#define __STATUSBAR_ITEM_BINDING_H__

#include <libhildondesktop/hildon-desktop-item-plug.h>
#include <libhildondesktop/statusbar-item.h>
#include <gtk/gtkplug.h>
#include <gdk/gdktypes.h>

G_BEGIN_DECLS

#define STATUSBAR_TYPE_ITEM_BINDING ( statusbar_item_binding_get_type() )
#define STATUSBAR_ITEM_BINDING(obj) (GTK_CHECK_CAST (obj, STATUSBAR_TYPE_ITEM_BINDING, StatusbarItemBinding))
#define STATUSBAR_ITEM_BINDING_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), STATUSBAR_TYPE_ITEM_BINDING, StatusbarItemBindingClass))
#define STATUSBAR_IS_ITEM_BINDING(obj) (GTK_CHECK_TYPE (obj, STATUSBAR_TYPE_ITEM_BINDING))
#define STATUSBAR_IS_ITEM_BINDING_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), STATUSBAR_TYPE_ITEM_BINDING))
#define STATUSBAR_ITEM_BINDING_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), STATUSBAR_TYPE_ITEM_BINDING, StatusbarItemBindingPrivate));

typedef struct _StatusbarItemBinding StatusbarItemBinding; 
typedef struct _StatusbarItemBindingClass StatusbarItemBindingClass;

struct _StatusbarItemBinding
{
  StatusbarItem parent;

  GtkPlug      *plug;
};

struct _StatusbarItemBindingClass
{
  StatusbarItemClass parent_class;
};

GType statusbar_item_binding_get_type (void);

StatusbarItemBinding *
statusbar_item_binding_new (GdkNativeWindow wid);

G_END_DECLS

#endif/*__STATUSBAR_ITEM_BINDING_H__*/
