/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

#ifndef __HILDON_DESKTOP_POPUP_MENU_H__
#define __HILDON_DESKTOP_POPUP_MENU_H__

#include <gtk/gtkvbox.h>
#include <gtk/gtkmenuitem.h>

G_BEGIN_DECLS

typedef struct _HildonDesktopPopupMenu HildonDesktopPopupMenu;
typedef struct _HildonDesktopPopupMenuClass HildonDesktopPopupMenuClass;
typedef struct _HildonDesktopPopupMenuPrivate HildonDesktopPopupMenuPrivate;


#define HILDON_DESKTOP_TYPE_POPUP_MENU ( hildon_desktop_popup_menu_get_type() )
#define HILDON_DESKTOP_POPUP_MENU(obj) (GTK_CHECK_CAST (obj, HILDON_DESKTOP_TYPE_POPUP_MENU, HildonDesktopPopupMenu))
#define HILDON_DESKTOP_POPUP_MENU_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), HILDON_DESKTOP_TYPE_POPUP_MENU, HildonDesktopPopupMenuClass))
#define HILDON_DESKTOP_IS_POPUP_MENU(obj) (GTK_CHECK_TYPE (obj, HILDON_DESKTOP_TYPE_POPUP_MENU))
#define HILDON_DESKTOP_IS_POPUP_MENU_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), HILDON_DESKTOP_TYPE_POPUP_MENU))

struct _HildonDesktopPopupMenu
{
  GtkVBox	                   parent;

  HildonDesktopPopupMenuPrivate   *priv;
};

struct _HildonDesktopPopupMenuClass
{
  GtkVBoxClass		           parent_class;
  /* */	
};

GType 
hildon_desktop_popup_menu_get_type (void);

void 
hildon_desktop_popup_menu_add_item (HildonDesktopPopupMenu *menu,
				    GtkMenuItem            *item);

void
hildon_desktop_popup_menu_remove_item (HildonDesktopPopupMenu *menu,
				       GtkMenuItem            *item);

GList *
hildon_desktop_popup_menu_get_children (HildonDesktopPopupMenu *menu);

void
hildon_desktop_popup_menu_select_item (HildonDesktopPopupMenu *menu, GtkMenuItem *item);

void 
hildon_desktop_popup_menu_select_first_item (HildonDesktopPopupMenu *menu);

void
hildon_desktop_popup_menu_activate_item (HildonDesktopPopupMenu *menu, GtkMenuItem *item);

void   
hildon_desktop_popup_menu_scroll_to_selected (HildonDesktopPopupMenu *menu);

const GtkWidget *
hildon_desktop_popup_menu_get_scroll_button_up (HildonDesktopPopupMenu *menu);

const GtkWidget *
hildon_desktop_popup_menu_get_scroll_button_down (HildonDesktopPopupMenu *menu);

G_BEGIN_DECLS

#endif/*__HILDON_DESKTOP_POPUP_MENU_H__*/

