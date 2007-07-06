/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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

#ifndef __HD_APP_MENU_TREE_H__
#define __HD_APP_MENU_TREE_H__

#include <gtk/gtkhpaned.h>
#include <gtk/gtktreemodel.h>

G_BEGIN_DECLS

typedef struct _HDAppMenuTree HDAppMenuTree;
typedef struct _HDAppMenuTreeClass HDAppMenuTreeClass;
typedef struct _HDAppMenuTreePrivate HDAppMenuTreePrivate;

#define HD_TYPE_APP_MENU_TREE            (hd_app_menu_tree_get_type ())
#define HD_APP_MENU_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_APP_MENU_TREE, HDAppMenuTree))
#define HD_APP_MENU_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_APP_MENU_TREE, HDAppMenuTreeClass))
#define HD_IS_APP_MENU_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_APP_MENU_TREE))
#define HD_IS_APP_MENU_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_APP_MENU_TREE))
#define HD_APP_MENU_TREE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_APP_MENU_TREE, HDAppMenuTreeClass))

struct _HDAppMenuTree
{
  GtkHPaned                     parent;

  HDAppMenuTreePrivate         *priv;
};

struct _HDAppMenuTreeClass
{
  GtkHPanedClass                parent_class;
  void                          (*item_selected)        (HDAppMenuTree *tree,
                                                         GtkTreeIter    iter);
};

GType   hd_app_menu_tree_get_type       (void);
void    hd_app_menu_tree_set_model      (HDAppMenuTree         *tree,
                                         GtkTreeModel          *model);
gboolean hd_app_menu_tree_get_selected_category (HDAppMenuTree *tree,
                                                 GtkTreeIter   *iter);


G_END_DECLS

#endif /* __HD_APP_MENU_TREE_H__*/
