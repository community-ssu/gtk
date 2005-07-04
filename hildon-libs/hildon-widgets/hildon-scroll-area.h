/*
 * This file is part of hildon-libs
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

/*
 * The reason why this is not made as a widget:
 *  We can not create a widget which could return the correct child.
 *  (ie. by gtk_bin_get_child)
 */

#ifndef HILDON_SCROLL_AREA_H
#define HILDON_SCROLL_AREA_H

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS
    GtkWidget * hildon_scroll_area_new(GtkWidget * sw, GtkWidget * child);

G_END_DECLS
#endif /* HILDON_SCROLL_AREA_H */
