/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright 2006 Nokia Corporation, all rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __HAIL_GRID_H__
#define __HAIL_GRID_H__

#include <gtk/gtkaccessible.h>

G_BEGIN_DECLS

#define HAIL_TYPE_GRID                     (hail_grid_get_type ())
#define HAIL_GRID(obj)                     (G_TYPE_CHECK_INSTANCE_CAST ((obj), HAIL_TYPE_GRID, HailGrid))
#define HAIL_GRID_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), HAIL_TYPE_GRID, HailGridClass))
#define HAIL_IS_GRID(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HAIL_TYPE_GRID))
#define HAIL_IS_GRID_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), HAIL_TYPE_GRID))
#define HAIL_GRID_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), HAIL_TYPE_GRID, HailGridClass))
  

  /**
   * HailGrid:
   *
   * Contains only private date.
   */
typedef struct _HailGrid                   HailGrid;
typedef struct _HailGridClass              HailGridClass;
  
struct _HailGrid
{
  GtkAccessible parent;
};

GType hail_grid_get_type (void);

struct _HailGridClass
{
  GtkAccessibleClass parent_class;
};

AtkObject* hail_grid_new (GtkWidget *widget);

G_END_DECLS


#endif /* __HAIL_GRID_H__ */
