/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005 Nokia Corporation.
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

/*
 * @file cp-grid-item-private.h
 *
 * This file is a private header file for he implementation of
 * CPGridItem. CPGridItem is an item mainly used in CPGrid. It
 * has an icon, emblem and a label. This private header file exists so that
 * grid can call semi-public functions of an item.
 */

#ifndef CP_GRID_ITEM_PRIVATE_H_
#define CP_GRID_ITEM_PRIVATE_H_

#include "cp-grid-item.h"


G_BEGIN_DECLS


void _cp_grid_item_set_label(CPGridItem *item,
                             const gchar *label);
void _cp_grid_item_set_emblem_size(CPGridItem *item,
                                   const gint emblem_size);
void _cp_grid_item_set_icon_size(CPGridItem *item,
                                 CPGridItemIconSizeType icon_size);
void _cp_grid_item_set_focus_margin(CPGridItem *item,
                                    const gint focus_margin);
void _cp_grid_item_set_label_height(CPGridItem *item,
                                    const gint label_height);
void _cp_grid_item_set_label_icon_margin(CPGridItem *item,
                                         const gint label_icon_margin);
void _cp_grid_item_set_icon_width(CPGridItem *item,
                                  const gint icon_width);
void _cp_grid_item_set_icon_height(CPGridItem *item,
                                   const gint icon_height);
void _cp_grid_item_set_label_height(CPGridItem *item,
		                    const gint label_height);
void _cp_grid_item_done_updating_settings(CPGridItem *item);


G_END_DECLS

#endif /* ifndef CP_GRID_ITEM_PRIVATE_H_ */
