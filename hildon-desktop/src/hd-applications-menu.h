/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author: Lucas Rocha <lucas.rocha@nokia.com>
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

#ifndef __HD_APPLICATIONS_MENU_H__
#define __HD_APPLICATIONS_MENU_H__

#include <glib.h>
#include <glib-object.h>
#include <libhildonwm/hd-wm.h>
#include <libhildondesktop/libhildondesktop.h>

G_BEGIN_DECLS

typedef struct _HDApplicationsMenu HDApplicationsMenu;
typedef struct _HDApplicationsMenuClass HDApplicationsMenuClass;
typedef struct _HDApplicationsMenuPrivate HDApplicationsMenuPrivate;

#define HD_TYPE_APPLICATIONS_MENU            (hd_applications_menu_get_type ())
#define HD_APPLICATIONS_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_APPLICATIONS_MENU, HDApplicationsMenu))
#define HD_IS_APPLICATIONS_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_APPLICATIONS_MENU))
#define HD_APPLICATIONS_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_APPLICATIONS_MENU, HDApplicationsMenuClass))
#define HD_IS_APPLICATIONS_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_APPLICATIONS_MENU))
#define HD_APPLICATIONS_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_APPLICATIONS_MENU, HDApplicationsMenuClass))

struct _HDApplicationsMenu
{
  TaskNavigatorItem parent_instance;
  
  HDApplicationsMenuPrivate *priv;
};

struct _HDApplicationsMenuClass
{
  TaskNavigatorItemClass parent_class;
};

GType        hd_applications_menu_get_type             (void);

GtkWidget   *hd_applications_menu_new                  (void);

void         hd_applications_menu_close_menu	   (HDApplicationsMenu *button);

G_END_DECLS

#endif /* __HD_APPLICATIONS_MENU_H__ */
