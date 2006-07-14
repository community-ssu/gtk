/* hn-app-tooltip.h
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
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

/**
 * @file hn-app-tooltip.h
 *
 * @brief Definitions of the application tooltip
 *        used by the Application Switcher
 *
 */

#ifndef HN_APP_TOOLTIP_H
#define HN_APP_TOOLTIP_H

#include <gtk/gtkwidget.h>
#include "hn-wm-types.h"

G_BEGIN_DECLS

#define HN_TYPE_APP_TOOLTIP		(hn_app_tooltip_get_type ())
#define HN_APP_TOOLTIP(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), HN_TYPE_APP_TOOLTIP, HNAppTooltip))
#define HN_IS_APP_TOOLTIP(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), HN_TYPE_APP_TOOLTIP))

typedef struct _HNAppTooltip HNAppTooltip;

GType        hn_app_tooltip_get_type      (void) G_GNUC_CONST;

GtkWidget *  hn_app_tooltip_new           (GtkWidget    *widget);
void         hn_app_tooltip_set_widget    (HNAppTooltip *tip,
					   GtkWidget    *widget);
GtkWidget *  hn_app_tooltip_get_widget    (HNAppTooltip *tip);
void         hn_app_tooltip_set_text      (HNAppTooltip *tip,
										   gchar  *text);
const gchar *hn_app_tooltip_get_text      (HNAppTooltip *tip);

void         hn_app_tooltip_install_timer (HNAppTooltip *tip,
					   GtkCallback   show_cb,
					   GtkCallback   hide_cb,
					   gpointer      data);
void         hn_app_tooltip_remove_timers  (HNAppTooltip *tip);
void         hn_app_tooltip_remove_show_timer  (HNAppTooltip *tip);

G_END_DECLS

#endif /* HN_APP_TOOLTIP_H */
