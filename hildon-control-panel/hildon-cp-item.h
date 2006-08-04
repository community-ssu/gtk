/*
 * This file is part of hildon-control-panel
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

#ifndef HILDON_CP_ITEM_H
#define HILDON_CP_ITEM_H

#include <gtk/gtkwidget.h>
#include <libosso.h>

#define HCP_PLUGIN_EXEC_SYMBOL          "execute"
#define HCP_PLUGIN_SAVE_STATE_SYMBOL    "save_state"


typedef osso_return_t (hcp_plugin_exec_f) (
                       osso_context_t * osso,
                       gpointer data,
                       gboolean user_activated);

typedef osso_return_t (hcp_plugin_save_state_f) (
                       osso_context_t * osso,
                       gpointer data);

typedef struct _HCPItem {
    gchar *name;
    gchar *plugin;
    gchar *icon;
    gchar *category;
    gboolean running;
    GtkWidget *grid_item;
    hcp_plugin_save_state_f *save_state;
} HCPItem;

void hcp_item_init (gpointer hcp);

void hcp_item_free (HCPItem *item);

void hcp_item_launch (HCPItem *item,
                      gboolean user_activated);

void hcp_item_save_state (HCPItem *item);

void hcp_item_focus (HCPItem *item);


#endif
