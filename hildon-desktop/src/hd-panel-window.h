/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#ifndef __HD_PANEL_WINDOW_H__
#define __HD_PANEL_WINDOW_H__

#include <glib-object.h>
#include <libhildondesktop/hildon-desktop-panel-window-composite.h>

G_BEGIN_DECLS

typedef struct _HDPanelWindow HDPanelWindow;
typedef struct _HDPanelWindowClass HDPanelWindowClass;

#define HD_TYPE_PANEL_WINDOW            (hd_panel_window_get_type ())
#define HD_PANEL_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_PANEL_WINDOW, HDPanelWindow))
#define HD_PANEL_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_PANEL_WINDOW, HDPanelWindowClass))
#define HD_IS_PANEL_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_PANEL_WINDOW))
#define HD_IS_PANEL_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_PANEL_WINDOW))
#define HD_PANEL_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_PANEL_WINDOW, HDPanelWindowClass))

struct _HDPanelWindow 
{
  HildonDesktopPanelWindowComposite parent;

};

struct _HDPanelWindowClass
{
  HildonDesktopPanelWindowCompositeClass parent_class;

};

GType  hd_panel_window_get_type  (void);

G_END_DECLS

#endif /*__HD_PANEL_WINDOW_H__*/
