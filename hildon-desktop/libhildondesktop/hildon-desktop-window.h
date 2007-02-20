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

#ifndef __HILDON_DESKTOP_WINDOW_H__
#define __HILDON_DESKTOP_WINDOW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _HildonDesktopWindow HildonDesktopWindow;
typedef struct _HildonDesktopWindowClass HildonDesktopWindowClass;
typedef struct _HildonDesktopWindowPrivate HildonDesktopWindowPrivate;

#define HILDON_DESKTOP_TYPE_WINDOW ( hildon_desktop_window_get_type() )
#define HILDON_DESKTOP_WINDOW(obj) (GTK_CHECK_CAST (obj, HILDON_DESKTOP_TYPE_WINDOW, HildonDesktopWindow))
#define HILDON_DESKTOP_WINDOW_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), HILDON_DESKTOP_TYPE_WINDOW, HildonDesktopWindowClass))
#define HILDON_DESKTOP_IS_WINDOW(obj) (GTK_CHECK_TYPE (obj, HILDON_DESKTOP_TYPE_WINDOW))
#define HILDON_DESKTOP_IS_WINDOW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), HILDON_DESKTOP_TYPE_WINDOW))

struct _HildonDesktopWindow
{
  GtkWindow 	                parent;

  HildonDesktopWindowPrivate   *priv;

  GtkContainer                 *container;
};

struct _HildonDesktopWindowClass
{
  GtkWindowClass parent_class;

  void (*select_plugins) (HildonDesktopWindow *window);

  void (*save)           (HildonDesktopWindow *window);

  void (*load)           (HildonDesktopWindow *window);

  void (*set_sensitive)  (HildonDesktopWindow *window,
                          gboolean             sensitive);
  
  void (*set_focus)      (HildonDesktopWindow *window,
                          gboolean             focus);
};

GType          hildon_desktop_window_get_type         (void);

void           hildon_desktop_window_set_sensitive    (HildonDesktopWindow *window, 
                                                       gboolean             sensitive);

void           hildon_desktop_window_set_focus        (HildonDesktopWindow *window, 
                                                       gboolean             focus);

G_END_DECLS

#endif
