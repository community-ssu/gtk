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
 * @file cp-grid.h
 *
 * This file is a header file for cp-grid.c, the implementation of
 * #CPGrid. #CPGrid is used in views like Home and Control Panel
 * which have single-tap activated items.
 */

#ifndef CP_GRID_H_
#define CP_GRID_H_

#include <gtk/gtkcontainer.h>
#include "cp-grid-item.h"

G_BEGIN_DECLS

#define GRID_MAXIMUM_NUMBER_OF_GRIDS 10

#define CP_GRID_FOCUS_FROM_ABOVE 0
#define CP_GRID_FOCUS_FIRST      1
#define CP_GRID_FOCUS_FROM_BELOW 2
#define CP_GRID_FOCUS_LAST       3

#define CP_TYPE_GRID            (cp_grid_get_type ())
#define CP_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                            CP_TYPE_GRID, \
                                            CPGrid))
#define CP_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                            CP_TYPE_GRID, \
                                            CPGridClass))
#define CP_OBJECT_IS_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                            CP_TYPE_GRID))
#define CP_OBJECT_IS_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                            CP_TYPE_GRID))
#define CP_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                            CP_TYPE_GRID, \
                                            CPGridClass))
typedef struct _CPGrid CPGrid;
typedef struct _CPGridClass CPGridClass;


struct _CPGrid {
    GtkContainer parent;
};

struct _CPGridClass {
    GtkContainerClass parent_class;

    void (*activate_child) (CPGrid * grid, CPGridItem * item);
    void (*popup_context_menu) (CPGrid * grid, CPGridItem * item);
};

GType cp_grid_get_type(void) G_GNUC_CONST;
GtkWidget *cp_grid_new(void);

/*
 * Use GtkContainer API:
 *
 * void gtk_container_set_focus_child(GtkContainer *container,
 *                                    GtkWidget *child);
 *
 * GTK_CONTAINER (grid)->focus_child can be used to get focused child.
 */

void cp_grid_set_style(CPGrid * grid, const gchar * style_name);
const gchar *cp_grid_get_style(CPGrid * grid);

void cp_grid_set_scrollbar_pos(CPGrid * grid, gint scrollbar_pos);
gint cp_grid_get_scrollbar_pos(CPGrid * grid);


/*
 * We are going to use gtk_container_add/remove, so these are internal.
 * If GridView is not visible, it won't update the view, so it should be
 * hidden when doing massive modifications.
 *
 * 
 * Use GtkContainer API:
 *
 * void gtk_container_add(GtkContainer *container,
 *                        GtkWidget *widget);
 *
 * void gtk_container_remove(GtkContainer *container,
 *                           GtkWidget *widget);
 */

void cp_grid_activate_child(CPGrid * grid, CPGridItem * item);

gboolean cp_grid_has_focus(CPGrid *grid);

void cp_grid_focus_first_item(CPGrid *grid);


G_END_DECLS
#endif /* ifndef CP_GRID_H_ */
