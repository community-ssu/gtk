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

#ifndef HN_APP_SWITCHER_H
#define HN_APP_SWITCHER_H

#include <gtk/gtkvbox.h>
#include "hn-wm-types.h"

G_BEGIN_DECLS

#define HN_TYPE_APP_SWITCHER            (hn_app_switcher_get_type ())
#define HN_APP_SWITCHER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HN_TYPE_APP_SWITCHER, HNAppSwitcher))
#define HN_IS_APP_SWITCHER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HN_TYPE_APP_SWITCHER))
#define HN_APP_SWITCHER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HN_TYPE_APP_SWITCHER, HNAppSwitcherClass))
#define HN_IS_APP_SWITCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HN_TYPE_APP_SWITCHER))
#define HN_APP_SWITCHER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HN_TYPE_APP_SWITCHER, HNAppSwitcherClass))

typedef struct _HNAppSwitcherPrivate HNAppSwitcherPrivate;
typedef struct _HNAppSwitcherClass   HNAppSwitcherClass;

typedef gboolean (*HNAppSwitcherForeachFunc) (HNEntryInfo *info,
					      gpointer     data);

struct _HNAppSwitcher
{
  GtkVBox parent_instance;
  
  HNAppSwitcherPrivate *priv;
};

struct _HNAppSwitcherClass
{
  GtkVBoxClass parent_class;
  
  void (*add_info)      (HNAppSwitcher *app_switcher,
		         HNEntryInfo   *entry_info);
  void (*remove_info)   (HNAppSwitcher *app_switcher,
		         HNEntryInfo   *entry_info);
  void (*changed_info)  (HNAppSwitcher *app_switcher,
		         HNEntryInfo   *entry_info);
  void (*changed_stack) (HNAppSwitcher *app_switcher,
		  	 HNEntryInfo   *entry_info);
  
  /* relay signals from the bus */
  void (*shutdown) (HNAppSwitcher *app_switcher);
  void (*lowmem)   (HNAppSwitcher *app_switcher,
                    gboolean       is_active);
  void (*bgkill)   (HNAppSwitcher *app_switcher,
                    gboolean       is_active);
};

GType      hn_app_switcher_get_type      (void) G_GNUC_CONST;

GtkWidget *hn_app_switcher_new           (void);

GList *    hn_app_switcher_get_entries   (HNAppSwitcher            *app_switcher);
void       hn_app_switcher_foreach_entry (HNAppSwitcher            *app_switcher,
					  HNAppSwitcherForeachFunc  func,
					  gpointer                  data);

void       hn_app_switcher_add           (HNAppSwitcher *app_switcher,
				          HNEntryInfo   *entry_info);
void       hn_app_switcher_remove        (HNAppSwitcher *app_switcher,
				          HNEntryInfo   *entry_info);
void       hn_app_switcher_changed       (HNAppSwitcher *app_switcher,
				          HNEntryInfo   *entry_info);
void       hn_app_switcher_changed_stack (HNAppSwitcher *app_switcher,
					  HNEntryInfo   *entry_info);

void       hn_app_switcher_toggle_menu_button (HNAppSwitcher *app_switcher);

gboolean   hn_app_switcher_get_system_inactivity (HNAppSwitcher *app_switcher);

gboolean   hn_app_switcher_menu_button_release_cb (GtkWidget      *widget,
                                                 GdkEventButton *event);

HNEntryInfo * hn_app_switcher_get_home_entry_info (HNAppSwitcher *as);

G_END_DECLS

#endif /* HN_APP_SWITCHER_H */
