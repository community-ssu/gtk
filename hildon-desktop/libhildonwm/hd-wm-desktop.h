/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/* 
 * This file is part of libhildonwm
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
 *
 * Author: Moises Martinez <moises.martinez@nokia.com>
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

/* A HDWMDesktop is a running watched / tracked instance of a window 
 * that references a valid HDWMApplication. A watched window *may* contain
 * a list of views ( see below ). 
 */

#ifndef __HD_WM_DESKTOP_H__
#define __HD_WM_DESKTOP_H__

#include <libhildonwm/hd-wm.h>
#include <libhildonwm/hd-wm-util.h>
#include <libhildonwm/hd-wm-types.h>

G_BEGIN_DECLS

/* For window_sync(), should go in enum */

#define HD_WM_TYPE_DESKTOP            (hd_wm_desktop_get_type ())
#define HD_WM_DESKTOP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_WM_TYPE_DESKTOP, HDWMDesktop))
#define HD_WM_DESKTOP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_WM_TYPE_DESKTOP, HDWMDesktopClass))
#define HD_WM_IS_DESKTOP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_WM_TYPE_DESKTOP))
#define HD_IS_WM_DESKTOP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_WM_TYPE_DESKTOP))
#define HD_WM_DESKTOP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_WM_TYPE_DESKTOP, HDWMDesktopClass))

typedef struct _HDWMDesktopClass HDWMDesktopClass;
typedef struct _HDWMDesktopPrivate HDWMDesktopPrivate;

struct _HDWMDesktop
{
  GObject parent;
 
  HDWMDesktopPrivate *priv; 
};

struct _HDWMDesktopClass
{
  GObjectClass parent_class;
};

GType 
hd_wm_desktop_get_type (void);

HDWMDesktop * hd_wm_desktop_new (void);

void hd_wm_desktop_set_x_window (HDWMDesktop *desktop, Window win);

G_END_DECLS

#endif
