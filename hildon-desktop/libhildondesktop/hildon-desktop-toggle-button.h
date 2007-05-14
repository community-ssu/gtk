/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan bilien@nokia.com>
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

#ifndef __HILDON_DESKTOP_TOGGLE_BUTTON_H__
#define __HILDON_DESKTOP_TOGGLE_BUTTON_H__

#include <gtk/gtktogglebutton.h>

G_BEGIN_DECLS

typedef struct _HildonDesktopToggleButton HildonDesktopToggleButton;
typedef struct _HildonDesktopToggleButtonClass HildonDesktopToggleButtonClass;
typedef struct _HildonDesktopToggleButtonPrivate HildonDesktopToggleButtonPrivate;

#define HILDON_DESKTOP_TYPE_TOGGLE_BUTTON            (hildon_desktop_toggle_button_get_type ())
#define HILDON_DESKTOP_TOGGLE_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_DESKTOP_TYPE_TOGGLE_BUTTON, HildonDesktopToggleButton))
#define HILDON_DESKTOP_TOGGLE_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HILDON_DESKTOP_TYPE_TOGGLE_BUTTON, HildonDesktopToggleButtonClass))
#define HILDON_DESKTOP_IS_TOGGLE_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_DESKTOP_TYPE_TOGGLE_BUTTON))
#define HILDON_DESKTOP_IS_TOGGLE_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HILDON_DESKTOP_TYPE_TOGGLE_BUTTON))
#define HILDON_DESKTOP_TOGGLE_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HILDON_DESKTOP_TYPE_TOGGLE_BUTTON, HildonDesktopToggleButtonClass))

struct _HildonDesktopToggleButton
{
  GtkToggleButton                       parent;

  HildonDesktopToggleButtonPrivate     *priv;
};

struct _HildonDesktopToggleButtonClass
{
  GtkToggleButtonClass                  parent_class;
};

GType           hildon_desktop_toggle_button_get_type (void);

GtkWidget*      hildon_desktop_toggle_button_new      (void);

G_END_DECLS

#endif /* __HILDON_DESKTOP_TOGGLE_BUTTON_H__ */
