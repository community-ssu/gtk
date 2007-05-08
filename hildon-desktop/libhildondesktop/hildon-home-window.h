/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
 *
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

#ifndef __HILDON_HOME_WINDOW_H__
#define __HILDON_HOME_WINDOW_H__

#include <libhildondesktop/hildon-desktop-window.h>

G_BEGIN_DECLS

#define HILDON_TYPE_HOME_WINDOW			(hildon_home_window_get_type ())
#define HILDON_HOME_WINDOW(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_HOME_WINDOW, HildonHomeWindow))
#define HILDON_IS_HOME_WINDOW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_HOME_WINDOW))
#define HILDON_HOME_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_HOME_WINDOW, HildonHomeWindowClass))
#define HILDON_IS_HOME_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_HOME_WINDOW))
#define HILDON_HOME_WINDOW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_HOME_WINDOW, HildonHomeWindowClass))

typedef struct _HildonHomeWindow		    HildonHomeWindow;
typedef struct _HildonHomeWindowPrivate		HildonHomeWindowPrivate;
typedef struct _HildonHomeWindowClass		HildonHomeWindowClass;

struct _HildonHomeWindow
{
  HildonDesktopWindow parent_instance;

  HildonHomeWindowPrivate *priv;
};

struct _HildonHomeWindowClass
{
  HildonDesktopWindowClass parent_class;

  void (*button1_clicked)       (HildonHomeWindow *window);
  void (*button2_clicked)       (HildonHomeWindow *window);

};

GType hildon_home_window_get_type                  (void) G_GNUC_CONST;

GtkWidget *hildon_home_window_new                  (void);


GtkWidget *hildon_home_window_get_menu             (HildonHomeWindow *window);
void       hildon_home_window_set_menu             (HildonHomeWindow *window,
                                                    GtkMenu          *menu);
void       hildon_home_window_toggle_menu          (HildonHomeWindow *window);

G_END_DECLS

#endif /* __HILDON_HOME_WINDOW_H__ */
