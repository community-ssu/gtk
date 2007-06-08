/*
 * This file is part of maemo-af-statusbar
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

#ifndef __STATUSBAR_ITEM_H__
#define __STATUSBAR_ITEM_H__

#include <libhildondesktop/hildon-desktop-panel-item.h>

#include <gtk/gtkwindow.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkimage.h>

G_BEGIN_DECLS

#define STATUSBAR_TYPE_ITEM ( statusbar_item_get_type() )
#define STATUSBAR_ITEM(obj) (GTK_CHECK_CAST (obj, STATUSBAR_TYPE_ITEM, StatusbarItem))
#define STATUSBAR_ITEM_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), STATUSBAR_TYPE_ITEM, StatusbarItemClass))
#define STATUSBAR_IS_ITEM(obj) (GTK_CHECK_TYPE (obj, STATUSBAR_TYPE_ITEM))
#define STATUSBAR_IS_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), STATUSBAR_TYPE_ITEM))


typedef struct _StatusbarItem StatusbarItem; 
typedef struct _StatusbarItemClass StatusbarItemClass;

struct _StatusbarItem
{
    HildonDesktopPanelItem parent;

    GtkWindow	    *window;
    GtkVBox	    *vbox;

    gint position;
    
    gboolean condition;
};

struct _StatusbarItemClass
{
    HildonDesktopPanelItemClass parent_class;

    void (*condition_update) (StatusbarItem *item, gboolean condition);
};

GType statusbar_item_get_type (void);

G_END_DECLS

#endif
