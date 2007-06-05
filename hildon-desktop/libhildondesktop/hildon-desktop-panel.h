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

#ifndef __HILDON_DESKTOP_PANEL_H__
#define __HILDON_DESKTOP_PANEL_H__

#include <gtk/gtkbox.h>
#include <libhildondesktop/hildon-desktop-panel-item.h>

G_BEGIN_DECLS

typedef struct _HildonDesktopPanel HildonDesktopPanel;
typedef struct _HildonDesktopPanelClass HildonDesktopPanelClass;

#define HILDON_DESKTOP_TYPE_PANEL            (hildon_desktop_panel_get_type())
#define HILDON_DESKTOP_PANEL(obj)            (GTK_CHECK_CAST (obj, HILDON_DESKTOP_TYPE_PANEL, HildonDesktopPanel))
#define HILDON_DESKTOP_PANEL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), HILDON_DESKTOP_TYPE_PANEL, HildonDesktopPanelClass))
#define HILDON_DESKTOP_IS_PANEL(obj)         (GTK_CHECK_TYPE (obj, HILDON_DESKTOP_TYPE_PANEL))
#define HILDON_DESKTOP_IS_PANEL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), HILDON_DESKTOP_TYPE_PANEL))
#define HILDON_DESKTOP_PANEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_DESKTOP_TYPE_PANEL, HildonDesktopPanelClass))

struct _HildonDesktopPanel
{
  GtkBox         parent;

  GtkOrientation orient;

  gint		 item_width;
  gint		 item_height;

  gboolean       pack_start;
};

struct _HildonDesktopPanelClass
{
  GtkBoxClass parent_class;

  void (*add_button)                (HildonDesktopPanel *panel, 
				     GtkWidget *widget);

  void (*set_orientation)           (HildonDesktopPanel *panel, 
		    		     GtkOrientation orientation);

  GtkOrientation (*get_orientation) (HildonDesktopPanel *panel);

  void (*flip)                      (HildonDesktopPanel *panel);

  void (*panel_flipped)		    (HildonDesktopPanel *panel);
};

GType           hildon_desktop_panel_get_type         (void);

void            hildon_desktop_panel_add_button       (HildonDesktopPanel *panel, 
                                                       GtkWidget          *widget);

void            hildon_desktop_panel_set_orientation  (HildonDesktopPanel *panel, 
                                                       GtkOrientation      orientation);

GtkOrientation  hildon_desktop_panel_get_orientation  (HildonDesktopPanel *panel);

void            hildon_desktop_panel_flip             (HildonDesktopPanel *panel);

void		hildon_desktop_panel_refresh_items_status   (HildonDesktopPanel *panel);

G_END_DECLS

#endif /* __HILDON_DESKTOP_PANEL_H__ */ 
