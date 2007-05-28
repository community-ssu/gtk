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

#ifndef __HILDON_DESKTOP_POPUP_WINDOW_H__
#define __HILDON_DESKTOP_POPUP_WINDOW_H__

#include <gtk/gtkwindow.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

typedef struct _HildonDesktopPopupWindow HildonDesktopPopupWindow;
typedef struct _HildonDesktopPopupWindowClass HildonDesktopPopupWindowClass;
typedef struct _HildonDesktopPopupWindowPrivate HildonDesktopPopupWindowPrivate;

typedef void (*HDPopupWindowPositionFunc) (HildonDesktopPopupWindow   *window,
                            	           gint      		      *x,
                                           gint      		      *y,
                                           gpointer   		      user_data);

typedef enum 
{
  HD_POPUP_WINDOW_DIRECTION_LEFT_TOP,
  HD_POPUP_WINDOW_DIRECTION_RIGHT_BOTTOM
}
HildonDesktopPopupWindowDirection;

#define HILDON_DESKTOP_TYPE_POPUP_WINDOW ( hildon_desktop_popup_window_get_type() )
#define HILDON_DESKTOP_POPUP_WINDOW(obj) (GTK_CHECK_CAST (obj, HILDON_DESKTOP_TYPE_POPUP_WINDOW, HildonDesktopPopupWindow))
#define HILDON_DESKTOP_POPUP_WINDOW_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), HILDON_DESKTOP_TYPE_POPUP_WINDOW, HildonDesktopPopupWindowClass))
#define HILDON_DESKTOP_IS_POPUP_WINDOW(obj) (GTK_CHECK_TYPE (obj, HILDON_DESKTOP_TYPE_POPUP_WINDOW))
#define HILDON_DESKTOP_IS_POPUP_WINDOW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), HILDON_DESKTOP_TYPE_POPUP_WINDOW))

struct _HildonDesktopPopupWindow
{
  GtkWindow 	                parent;

  HildonDesktopPopupWindowPrivate   *priv;
};

struct _HildonDesktopPopupWindowClass
{
  GtkWindowClass		parent_class;
  /* */	

  void (*popup_window) (HildonDesktopPopupWindow *window);
  void (*popdown_window) (HildonDesktopPopupWindow *window);
};

GType 
hildon_desktop_popup_window_get_type (void);

GtkWidget *
hildon_desktop_popup_window_new (guint n_panes,
			         GtkOrientation orientation,
			         HildonDesktopPopupWindowDirection direction);

GtkWidget *
hildon_desktop_popup_window_get_pane (HildonDesktopPopupWindow *popup, gint pane);

GtkWidget *
hildon_desktop_popup_window_get_grabbed_pane (HildonDesktopPopupWindow *popup);

void 
hildon_desktop_popup_window_jump_to_pane (HildonDesktopPopupWindow *popup, gint pane);

void 
hildon_desktop_popup_window_attach_widget (HildonDesktopPopupWindow *popup, GtkWidget *widget);

void 
hildon_desktop_popup_window_popup (HildonDesktopPopupWindow *popup,
				   HDPopupWindowPositionFunc func,
				   gpointer                  func_data,
				   guint32 		     activate_time);

void 
hildon_desktop_popup_window_popdown (HildonDesktopPopupWindow *popup);

void
hildon_desktop_popup_recalculate_position (HildonDesktopPopupWindow *popup);

G_BEGIN_DECLS

#endif/*__HILDON_DESKTOP_POPUP_WINDOW_H__*/

