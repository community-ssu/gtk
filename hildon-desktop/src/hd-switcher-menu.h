/*
 * Copyright (C) 2007 Nokia Corporation.
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

#ifndef __HD_SWITCHER_MENU_H__
#define __HD_SWITCHER_MENU_H__

#include <libhildondesktop/libhildondesktop.h>
#include <libhildondesktop/hildon-desktop-notification-manager.h>
#include <libhildonwm/hd-wm.h>

G_BEGIN_DECLS

#define HD_TYPE_SWITCHER_MENU            (hd_switcher_menu_get_type ())
#define HD_SWITCHER_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_SWITCHER_MENU, HDSwitcherMenu))
#define HD_IS_SWITCHER_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_SWITCHER_MENU))
#define HD_SWITCHER_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HD_TYPE_SWITCHER_MENU, HDSwitcherMenuClass))
#define HD_IS_SWITCHER_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HD_TYPE_SWITCHER_MENU))
#define HD_SWITCHER_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HD_TYPE_SWITCHER_MENU, HDSwitcherMenuClass))

typedef struct _HDSwitcherMenu HDSwitcherMenu;
typedef struct _HDSwitcherMenuPrivate HDSwitcherMenuPrivate;
typedef struct _HDSwitcherMenuClass   HDSwitcherMenuClass;

typedef gboolean (*HDSwitcherMenuForeachFunc) (HDEntryInfo *info,
					      gpointer     data);

struct _HDSwitcherMenu
{
  TaskNavigatorItem parent;

  HDWM *hdwm;
  HildonDesktopNotificationManager *nm;
  
  HDSwitcherMenuPrivate *priv;
};

struct _HDSwitcherMenuClass
{
  TaskNavigatorItemClass parent_class;
  
   
};

GType      hd_switcher_menu_get_type      (void) G_GNUC_CONST;

G_END_DECLS

#endif/*__HD_SWITCHER_MENU_H__*/
