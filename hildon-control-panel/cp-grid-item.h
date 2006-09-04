/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005 Nokia Corporation.
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

/*
 * @file cp-grid-item.h
 *
 * This file is a header file for he implementation of CPGridItem.
 * CPGridItem is an item mainly used in CPGrid. It has an icon,
 * emblem and a label.
 */

#ifndef CP_GRID_ITEM_H_
#define CP_GRID_ITEM_H_

#include <gtk/gtkcontainer.h>

G_BEGIN_DECLS

#define CP_TYPE_GRID_ITEM       (cp_grid_item_get_type ())
#define CP_GRID_ITEM(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                            CP_TYPE_GRID_ITEM, \
                                            CPGridItem))
#define CP_GRID_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                                CP_TYPE_GRID_ITEM, \
                                                CPGridItemClass))
#define CP_OBJECT_IS_GRID_ITEM(obj)    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                            CP_TYPE_GRID_ITEM))
#define CP_OBJECT_IS_GRID_ITEM_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), CP_TYPE_GRID_ITEM))
#define CP_GRID_ITEM_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
         CP_TYPE_GRID_ITEM, CPGridItemClass))

typedef enum {
    CP_GRID_ITEM_LABEL_POS_BOTTOM = 1,
    CP_GRID_ITEM_LABEL_POS_RIGHT
} CPGridPositionType;

typedef enum {
    CP_GRID_ITEM_ICON_27x27 = 1,
    CP_GRID_ITEM_ICON_128x128
} CPGridItemIconSizeType;


typedef struct _CPGridItem CPGridItem;
typedef struct _CPGridItemClass CPGridItemClass;


struct _CPGridItem {
    GtkContainer parent;
};

struct _CPGridItemClass {
    GtkContainerClass parent_class;

    void (*activate) (CPGridItem *item);
};



GType cp_grid_item_get_type(void) G_GNUC_CONST;
GtkWidget *cp_grid_item_new(const gchar * icon_basename);
GtkWidget *cp_grid_item_new_with_label(const gchar * icon_basename,
                                       const gchar * label);

void cp_grid_item_set_emblem_type(CPGridItem * item,
                                  const gchar * emblem_basename);
const gchar *cp_grid_item_get_emblem_type(CPGridItem * item);


G_END_DECLS
#endif /* ifndef CP_GRID_ITEM_H_ */
