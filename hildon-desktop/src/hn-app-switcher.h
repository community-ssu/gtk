/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/* hn-app-switcher.h
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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

/**
 * @file hn-app-switcher.h
 *
 * @brief Definitions of Application Switcher
 *
 */

#ifndef __HN_APP_SWITCHER_H__
#define __HN_APP_SWITCHER_H__

#include <libhildondesktop/libhildondesktop.h>
#include <libhildonwm/hd-wm.h>

G_BEGIN_DECLS

#define HN_TYPE_APP_SWITCHER            (hn_app_switcher_get_type ())
#define HN_APP_SWITCHER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HN_TYPE_APP_SWITCHER, HNAppSwitcher))
#define HN_IS_APP_SWITCHER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HN_TYPE_APP_SWITCHER))
#define HN_APP_SWITCHER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HN_TYPE_APP_SWITCHER, HNAppSwitcherClass))
#define HN_IS_APP_SWITCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HN_TYPE_APP_SWITCHER))
#define HN_APP_SWITCHER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HN_TYPE_APP_SWITCHER, HNAppSwitcherClass))

typedef struct _HNAppSwitcher HNAppSwitcher;
typedef struct _HNAppSwitcherPrivate HNAppSwitcherPrivate;
typedef struct _HNAppSwitcherClass   HNAppSwitcherClass;

typedef gboolean (*HNAppSwitcherForeachFunc) (HDEntryInfo *info,
					      gpointer     data);

struct _HNAppSwitcher
{
  TaskNavigatorItem parent_instance;

  GtkBox *box;

  HDWM *hdwm;
  
  HNAppSwitcherPrivate *priv;
};

struct _HNAppSwitcherClass
{
  TaskNavigatorItemClass parent_class;
  
   
  /* relay signals from the bus */
  void (*lowmem)   (HNAppSwitcher *app_switcher,
                    gboolean       is_active);
  void (*bgkill)   (HNAppSwitcher *app_switcher,
                    gboolean       is_active);
};

GType      hn_app_switcher_get_type      (void) G_GNUC_CONST;

GtkWidget *hn_app_switcher_new           (gint nitems);

gboolean   hn_app_switcher_get_system_inactivity (HNAppSwitcher *app_switcher);

gboolean   hn_app_switcher_menu_button_release_cb (GtkWidget      *widget,
                                                 GdkEventButton *event);


G_END_DECLS

#endif/*__HN_APP_SWITCHER_H__*/
