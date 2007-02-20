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

#ifndef __HILDON_DESKTOP_PANEL_WINDOW_H__
#define __HILDON_DESKTOP_PANEL_WINDOW_H__

#include <gtk/gtkwindow.h>

#include <libhildondesktop/hildon-desktop-multiscreen.h>
#include <libhildondesktop/hildon-desktop-window.h>
#include <libhildondesktop/hildon-desktop-panel.h>

G_BEGIN_DECLS

typedef struct _HildonDesktopPanelWindow HildonDesktopPanelWindow;
typedef struct _HildonDesktopPanelWindowClass HildonDesktopPanelWindowClass;
typedef struct _HildonDesktopPanelWindowPrivate HildonDesktopPanelWindowPrivate;

#define HILDON_DESKTOP_TYPE_PANEL_WINDOW            (hildon_desktop_panel_window_get_type())
#define HILDON_DESKTOP_PANEL_WINDOW(obj)            (GTK_CHECK_CAST (obj, HILDON_DESKTOP_TYPE_PANEL_WINDOW, HildonDesktopPanelWindow))
#define HILDON_DESKTOP_PANEL_WINDOW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), HILDON_DESKTOP_TYPE_PANEL_WINDOW, HildonDesktopPanelWindowClass))
#define HILDON_DESKTOP_IS_PANEL_WINDOW(obj)         (GTK_CHECK_TYPE (obj, HILDON_DESKTOP_TYPE_PANEL_WINDOW))
#define HILDON_DESKTOP_IS_PANEL_WINDOW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), HILDON_DESKTOP_TYPE_PANEL_WINDOW))

#define HILDON_DESKTOP_TYPE_PANEL_WINDOW_ORIENTATION (hildon_desktop_panel_window_orientation_get_type())

typedef enum
{
  HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP    = 1 << 0,
  HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT   = 1 << 1,
  HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT  = 1 << 2,
  HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM = 1 << 3
} HildonDesktopPanelWindowOrientation;

#define HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_HORIZONTAL \
	(HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP | HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM)

#define HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_VERTICAL \
	(HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT | HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT)

struct _HildonDesktopPanelWindow
{
  HildonDesktopWindow	            parent;

  GtkWidget                        *panel;

  HildonDesktopPanelWindowPrivate  *priv;
};

struct _HildonDesktopPanelWindowClass
{
  HildonDesktopWindowClass parent_class;

  void (*set_sensitive)         (HildonDesktopPanelWindow *window,
      		                 gboolean                  sensitive);
  
  void (*set_focus)             (HildonDesktopPanelWindow *window,
			         gboolean                  focus);
  
  void (*orientation_changed)   (HildonDesktopPanelWindow *window,
			         HildonDesktopPanelWindowOrientation new_orientation);
};

GType                hildon_desktop_panel_window_get_type       (void);

GType                hildon_desktop_panel_window_orientation_get_type  (void);

GtkWidget           *hildon_desktop_panel_window_new            (void);

void                 hildon_desktop_panel_window_set_multiscreen_handler (HildonDesktopPanelWindow *window, 
                                                                          HildonDesktopMultiscreen *ms);

G_END_DECLS

#endif /* __HILDON_DESKTOP_PANEL_WINDOW_H__ */
