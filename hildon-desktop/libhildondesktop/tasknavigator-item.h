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

#ifndef __TASKNAVIGATOR_ITEM_H__
#define __TASKNAVIGATOR_ITEM_H__

#include <libhildondesktop/hildon-desktop-panel-item.h>

#include <gtk/gtkmenu.h>
#include <gtk/gtkimage.h>

G_BEGIN_DECLS

#define TASKNAVIGATOR_TYPE_ITEM ( tasknavigator_item_get_type() )
#define TASKNAVIGATOR_ITEM(obj) (GTK_CHECK_CAST (obj, TASKNAVIGATOR_TYPE_ITEM, TaskNavigatorItem))
#define TASKNAVIGATOR_ITEM_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), TASKNAVIGATOR_TYPE_ITEM, TaskNavigatorItemClass))
#define TASKNAVIGATOR_IS_ITEM(obj) (GTK_CHECK_TYPE (obj, TASKNAVIGATOR_TYPE_ITEM))
#define TASKNAVIGATOR_IS_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), TASKNAVIGATOR_TYPE_ITEM))


typedef struct _TaskNavigatorItem TaskNavigatorItem; 
typedef struct _TaskNavigatorItemClass TaskNavigatorItemClass;

struct _TaskNavigatorItem
{
    HildonDesktopPanelItem parent;

    GtkMenu	    *menu;
    
    gboolean mandatory;
};

struct _TaskNavigatorItemClass
{
    HildonDesktopPanelItemClass parent_class;

};

GType tasknavigator_item_get_type (void);

G_END_DECLS

#endif
