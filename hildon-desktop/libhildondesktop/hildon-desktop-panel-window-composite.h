/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
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


#ifndef __HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE_H__
#define __HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE_H__

#include <libhildondesktop/hildon-desktop-panel-window.h>

#ifdef HAVE_X_COMPOSITE
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
#endif

G_BEGIN_DECLS

#define HILDON_DESKTOP_TYPE_PANEL_WINDOW_COMPOSITE   (hildon_desktop_panel_window_composite_get_type ())
#define HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE(obj)   (GTK_CHECK_CAST (obj, HILDON_DESKTOP_TYPE_PANEL_WINDOW_COMPOSITE, HildonDesktopPanelWindowComposite))
#define HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE_CLASS(klass) \
                                        (GTK_CHECK_CLASS_CAST ((klass), HILDON_DESKTOP_TYPE_PANEL_WINDOW_COMPOSITE, HildonDesktopPanelWindowCompositeClass))
#define HILDON_DESKTOP_IS_PANEL_WINDOW_COMPOSITE(obj) \
                                        (GTK_CHECK_TYPE (obj, HILDON_DESKTOP_TYPE_PANEL_WINDOW_COMPOSITE))
#define HILDON_IS_PANEL_WINDOW_COMPOSITE_CLASS(klass) \
                                        (GTK_CHECK_CLASS_TYPE ((klass), HILDON_DESKTOP_TYPE_PANEL_WINDOW_COMPOSITE))
#define HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HILDON_DESKTOP_TYPE_PANEL_WINDOW_COMPOSITE, HildonDesktopPanelWindowCompositeClass))



typedef struct _HildonDesktopPanelWindowComposite HildonDesktopPanelWindowComposite;
typedef struct _HildonDesktopPanelWindowCompositeClass HildonDesktopPanelWindowCompositeClass;
typedef struct _HildonDesktopPanelWindowCompositePrivate HildonDesktopPanelWindowCompositePrivate;
#ifdef HAVE_X_COMPOSITE
typedef struct _DesktopWindowData DesktopWindowData;
#endif

struct _HildonDesktopPanelWindowComposite {
  HildonDesktopPanelWindow                      parent;

  HildonDesktopPanelWindowCompositePrivate     *priv;
};

struct _HildonDesktopPanelWindowCompositeClass {
  HildonDesktopPanelWindowClass         parent_class;

#ifdef HAVE_X_COMPOSITE
  DesktopWindowData                    *desktop_window_data;
#endif

};


GType hildon_desktop_panel_window_composite_get_type (void);

G_END_DECLS
#endif /* __HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE_H__ */
