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

#ifndef __HILDON_DESKTOP_MULTISCREEN_H__
#define __HILDON_DESKTOP_MULTISCREEN_H__

#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

typedef struct _HildonDesktopMultiscreen HildonDesktopMultiscreen; 
typedef struct _HildonDesktopMultiscreenClass HildonDesktopMultiscreenClass;

#define HILDON_DESKTOP_TYPE_MULTISCREEN            (hildon_desktop_ms_get_type())
#define HILDON_DESKTOP_MULTISCREEN(obj)            (G_TYPE_CHECK_INSTANCE_CAST (obj, HILDON_DESKTOP_TYPE_MULTISCREEN, HildonDesktopMultiscreen))
#define HILDON_DESKTOP_MULTISCREEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_DESKTOP_TYPE_MULTISCREEN, HildonDesktopMultiscreenClass))
#define HILDON_DESKTOP_IS_MULTISCREEN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, HILDON_DESKTOP_TYPE_MULTISCREEN))
#define HILDON_DESKTOP_IS_MULTISCREEN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_DESKTOP_TYPE_MULTISCREEN))

struct _HildonDesktopMultiscreen
{
  GObject parent;

  gint screens;
  gint *monitors;	     /*NOTE: monitors[screen] */
  GdkRectangle **geometries; /*NOTE: geometries[screen][monitor]; */
  gboolean initialized;
};

struct _HildonDesktopMultiscreenClass
{
  GObjectClass parent_class;

  gint (*get_screens)           (HildonDesktopMultiscreen *ms);

  gint (*get_monitors)          (HildonDesktopMultiscreen *ms, 
				 GdkScreen                *screen);

  gint (*get_x)                 (HildonDesktopMultiscreen *ms,
				 GdkScreen                *screen,
				 gint                      monitor);

  gint (*get_y)                 (HildonDesktopMultiscreen *ms,
				 GdkScreen                *screen,
				 gint                      monitor);

  gint (*get_width)             (HildonDesktopMultiscreen *ms,
				 GdkScreen                *screen,
				 gint                      monitor);

  gint (*get_height)            (HildonDesktopMultiscreen *ms,
				 GdkScreen                *screen,
				 gint                      monitor);

  gint (*locate_widget_monitor) (HildonDesktopMultiscreen *ms,
				 GtkWidget                *widget);

  void (*is_at_visible_extreme) (HildonDesktopMultiscreen *ms,
				 GdkScreen                *screen,
				 gint                      monitor,
                                 gboolean                 *leftmost,
                                 gboolean                 *rightmost,
                                 gboolean                 *topmost,
                                 gboolean                 *bottommost);
  
};

GType    hildon_desktop_ms_get_type               (void);

void     hildon_desktop_ms_reinit                 (HildonDesktopMultiscreen *ms);

gint     hildon_desktop_ms_get_screens            (HildonDesktopMultiscreen *ms);

gint     hildon_desktop_ms_get_monitors           (HildonDesktopMultiscreen *ms,
                                                   GdkScreen                *screen);

gint     hildon_desktop_ms_get_x                  (HildonDesktopMultiscreen *ms,
                                                   GdkScreen                *screen,
                                                   gint                      monitor);

gint     hildon_desktop_ms_get_y                  (HildonDesktopMultiscreen *ms,
                                                   GdkScreen                *screen,
                                                   gint                      monitor);

gint     hildon_desktop_ms_get_width              (HildonDesktopMultiscreen *ms,
                                                   GdkScreen                *screen,
                                                   gint                      monitor);

gint     hildon_desktop_ms_get_height             (HildonDesktopMultiscreen *ms,
                                                   GdkScreen                *screen,
                                                   gint                      monitor);

gint     hildon_desktop_ms_locate_widget_monitor  (HildonDesktopMultiscreen *ms,
                                                   GtkWidget                *widget);

void     hildon_desktop_ms_is_at_visible_extreme  (HildonDesktopMultiscreen *ms,
                                                   GdkScreen                *screen,
                                                   gint                      monitor,
                                                   gboolean                 *leftmost,
                                                   gboolean                 *rightmost,
                                                   gboolean                 *topmost,
                                                   gboolean                 *bottommost);

gchar** hildon_desktop_ms_make_environment_for_screen (HildonDesktopMultiscreen *ms,
                                                       GdkScreen                *screen,
                                                       gchar                   **envp);

G_END_DECLS

#endif /* __HILDON_DESKTOP_MULTISCREEN_H__ */ 
