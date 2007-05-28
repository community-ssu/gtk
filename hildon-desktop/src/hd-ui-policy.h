/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
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


#ifndef __HD_UI_POLICY_H__
#define __HD_UI_POLICY_H__

#include <glib-object.h>
#include <gtk/gtkwidget.h>

#include <libhildondesktop/hildon-desktop-item.h>

G_BEGIN_DECLS

typedef struct _HDUIPolicy HDUIPolicy;
typedef struct _HDUIPolicyClass HDUIPolicyClass;
typedef struct _HDUIPolicyPrivate HDUIPolicyPrivate;

#define HD_TYPE_UI_POLICY            (hd_ui_policy_get_type ())
#define HD_UI_POLICY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_UI_POLICY, HDUIPolicy))
#define HD_UI_POLICY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_UI_POLICY, HDUIPolicyClass))
#define HD_IS_UI_POLICY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_UI_POLICY))
#define HD_IS_UI_POLICY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_UI_POLICY))
#define HD_UI_POLICY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_UI_POLICY, HDUIPolicyClass))

struct _HDUIPolicy 
{
  GObject gobject;

  HDUIPolicyPrivate *priv;
};

struct _HDUIPolicyClass 
{
  GObjectClass parent_class;
};

GType               hd_ui_policy_get_type            (void);

HDUIPolicy         *hd_ui_policy_new                 (const gchar *module);

GList*              hd_ui_policy_filter_plugin_list  (HDUIPolicy  *policy,
		                                      GList       *plugin_list);

gchar              *hd_ui_policy_get_default_item    (HDUIPolicy  *policy,
		 				      gint         position);

HildonDesktopItem  *hd_ui_policy_get_failure_item    (HDUIPolicy  *policy,
		 				      gint         position);

G_END_DECLS

#endif /* __HD_UI_POLICY_H__ */
