/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2005 Nokia Corporation.
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

#ifndef __HAIL_COLORSELECTOR_H__
#define __HAIL_COLORSELECTOR_H__

#include <gtk/gtkaccessible.h>

G_BEGIN_DECLS

#define HAIL_TYPE_COLOR_SELECTOR                     (hail_color_selector_get_type ())
#define HAIL_COLOR_SELECTOR(obj)                     (G_TYPE_CHECK_INSTANCE_CAST ((obj), HAIL_TYPE_COLOR_SELECTOR, HailColorSelector))
#define HAIL_COLOR_SELECTOR_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), HAIL_TYPE_COLOR_SELECTOR, HailColorSelectorClass))
#define HAIL_IS_COLOR_SELECTOR(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HAIL_TYPE_COLOR_SELECTOR))
#define HAIL_IS_COLOR_SELECTOR_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), HAIL_TYPE_COLOR_SELECTOR))
#define HAIL_COLOR_SELECTOR_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), HAIL_TYPE_COLOR_SELECTOR, HailColorSelectorClass))
  

  /**
   * HailColorSelector:
   *
   * Contains only private date.
   */
typedef struct _HailColorSelector                   HailColorSelector;
typedef struct _HailColorSelectorClass              HailColorSelectorClass;
  
struct _HailColorSelector
{
  GtkAccessible parent;
};

GType hail_color_selector_get_type (void);

struct _HailColorSelectorClass
{
  GtkAccessibleClass parent_class;
};

AtkObject* hail_color_selector_new (GtkWidget *widget);

G_END_DECLS


#endif /* __HAIL_COLORSELECTOR_H__ */
