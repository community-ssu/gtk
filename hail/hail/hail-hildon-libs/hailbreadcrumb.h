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

#ifndef __HAIL_BREAD_CRUMB_H__
#define __HAIL_BREAD_CRUMB_H__

#include <gtk/gtkaccessible.h>

G_BEGIN_DECLS

#define HAIL_TYPE_BREAD_CRUMB                     (hail_bread_crumb_get_type ())
#define HAIL_BREAD_CRUMB(obj)                     (G_TYPE_CHECK_INSTANCE_CAST ((obj), HAIL_TYPE_BREAD_CRUMB, HailBreadCrumb))
#define HAIL_BREAD_CRUMB_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass),  HAIL_TYPE_BREAD_CRUMB, HailBreadCrumbClass))
#define HAIL_IS_BREAD_CRUMB(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HAIL_TYPE_BREAD_CRUMB))
#define HAIL_IS_BREAD_CRUMB_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass),  HAIL_TYPE_BREAD_CRUMB))
#define HAIL_BREAD_CRUMB_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj),  HAIL_TYPE_BREAD_CRUMB, HailBreadCrumbClass))
  

/**
 * HailBreadCrumb:
 *
 * Contains only private date.
 */
typedef struct _HailBreadCrumb                     HailBreadCrumb;
typedef struct _HailBreadCrumbClass                HailBreadCrumbClass;
  
struct _HailBreadCrumb
{
  GtkAccessible parent;
};

GType hail_bread_crumb_get_type (void);

struct _HailBreadCrumbClass
{
  GtkAccessibleClass parent_class;
};

AtkObject *hail_bread_crumb_new (GtkWidget *widget);

G_END_DECLS


#endif /* __HAIL_BREAD_CRUMB_H__ */
