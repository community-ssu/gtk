/* hn-app-button.h
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
 * @file hn-app-button.h
 *
 * @brief Definitions of the application button used
 *        by the Application Switcher
 *
 */

#ifndef HN_APP_BUTTON_H
#define HN_APP_BUTTON_H

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtktogglebutton.h>

#include "hn-wm-types.h"

#define APP_BUTTON_THUMBABLE 8
#define APP_BUTTON_NORMAL    1

G_BEGIN_DECLS

#define HN_TYPE_APP_BUTTON              (hn_app_button_get_type ())
#define HN_APP_BUTTON(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), HN_TYPE_APP_BUTTON, HNAppButton))
#define HN_IS_APP_BUTTON(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HN_TYPE_APP_BUTTON))
#define HN_APP_BUTTON_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), HN_TYPE_APP_BUTTON, HNAppButtonClass))
#define HN_IS_APP_BUTTON_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), HN_TYPE_APP_BUTTON))
#define HN_APP_BUTTON_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), HN_TYPE_APP_BUTTON, HNAppButtonClass))

typedef struct _HNAppButton        HNAppButton;
typedef struct _HNAppButtonPrivate HNAppButtonPrivate;
typedef struct _HNAppButtonClass   HNAppButtonClass;

struct _HNAppButton
{
  GtkToggleButton parent_instance;

  GSList *group;

  HNAppButtonPrivate *priv;
};

struct _HNAppButtonClass
{
  GtkToggleButtonClass parent_class;
};

GType hn_app_button_get_type (void) G_GNUC_CONST;

GtkWidget *  hn_app_button_new                  (GSList      *group);
void         hn_app_button_set_group            (HNAppButton *button,
						 GSList      *group);
GSList *     hn_app_button_get_group            (HNAppButton *button);

void         hn_app_button_set_icon_from_pixbuf (HNAppButton *button,
						 GdkPixbuf   *pixbuf);
GdkPixbuf *  hn_app_button_get_pixbuf_from_icon (HNAppButton *button);
HNEntryInfo *hn_app_button_get_entry_info       (HNAppButton *button);
void         hn_app_button_set_entry_info       (HNAppButton *button,
					         HNEntryInfo *info);
gboolean     hn_app_button_get_is_blinking      (HNAppButton *button);
void         hn_app_button_set_is_blinking      (HNAppButton *button,
					         gboolean     is_blinking);
void         hn_app_button_force_update_icon    (HNAppButton *button);
void         hn_app_button_make_active          (HNAppButton *button);
G_END_DECLS

#endif /* HN_APP_BUTTON_H */
