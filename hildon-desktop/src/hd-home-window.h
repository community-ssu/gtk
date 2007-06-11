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

#ifndef __HD_HOME_WINDOW_H__
#define __HD_HOME_WINDOW_H__

#include <glib-object.h>
#include <libhildondesktop/hildon-home-window.h>

G_BEGIN_DECLS

typedef struct _HDHomeWindow HDHomeWindow;
typedef struct _HDHomeWindowClass HDHomeWindowClass;
typedef struct _HDHomeWindowPrivate HDHomeWindowPrivate;

#define HD_TYPE_HOME_WINDOW            (hd_home_window_get_type ())
#define HD_HOME_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_HOME_WINDOW, HDHomeWindow))
#define HD_HOME_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_HOME_WINDOW, HDHomeWindowClass))
#define HD_IS_HOME_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_HOME_WINDOW))
#define HD_IS_HOME_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_HOME_WINDOW))
#define HD_HOME_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_HOME_WINDOW, HDHomeWindowClass))

struct _HDHomeWindow
{
  HildonHomeWindow parent;
};

struct _HDHomeWindowClass
{
  HildonHomeWindowClass parent_class;

  void    (* background)        (HDHomeWindow *window, gboolean is_background);
  void    (* lowmem)            (HDHomeWindow *window, gboolean is_lowmem);
  void    (* screen_off       ) (HDHomeWindow *window, gboolean is_off);
  void    (* layout_mode_accept)(HDHomeWindow *window);
  void    (* layout_mode_cancel)(HDHomeWindow *window);
  void    (* io_error)          (HDHomeWindow *window, GError *error);

};

GType  hd_home_window_get_type  (void);

G_END_DECLS

#endif /*__HD_HOME_WINDOW_H__*/
