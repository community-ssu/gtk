/*
 * This file is part of hildon-lgpl
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Luc Pionchon <luc.pionchon@nokia.com>
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


#ifndef HILDON_APP_PRIVATE_H
#define HILDON_APP_PRIVATE_H

enum {
    TOPMOST_STATUS_ACQUIRE,
    TOPMOST_STATUS_LOSE,
    SWITCH_TO,
    IM_CLOSE,
    CLIPBOARD_COPY,
    CLIPBOARD_CUT,
    CLIPBOARD_PASTE,

    HILDON_APP_LAST_SIGNAL
};

struct _HildonAppPrivate {
    HildonAppView *appview;
    GtkWidget *focus_widget;
    gchar *title;
    HildonZoomLevel zoom;
    gint lastmenuclick;

    gulong curr_view_id;
    gulong view_id_counter;
    GSList *view_ids;
    gulong scroll_control;

    gint twoparttitle: 1;
    gint is_topmost: 1;
    gboolean killable;
    gboolean autoregistration;

    gint tag; /* source tag for the time out */
    gboolean escape_pressed;
};

typedef struct {
  gpointer view_ptr;
  unsigned long view_id;
} view_item;

#endif /* HILDON_APP_PRIVATE_H */
