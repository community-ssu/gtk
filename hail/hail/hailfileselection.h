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

#ifndef __HAIL_FILE_SELECTION_H__
#define __HAIL_FILE_SELECTION_H__

#include <gtk/gtkaccessible.h>

G_BEGIN_DECLS

#define HAIL_TYPE_FILE_SELECTION                     (hail_file_selection_get_type ())
#define HAIL_FILE_SELECTION(obj)                     (G_TYPE_CHECK_INSTANCE_CAST ((obj), HAIL_TYPE_FILE_SELECTION, HailFileSelection))
#define HAIL_FILE_SELECTION_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), HAIL_TYPE_FILE_SELECTION, HailFileSelectionClass))
#define HAIL_IS_FILE_SELECTION(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HAIL_TYPE_FILE_SELECTION))
#define HAIL_IS_FILE_SELECTION_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), HAIL_TYPE_FILE_SELECTION))
#define HAIL_FILE_SELECTION_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), HAIL_TYPE_FILE_SELECTION, HailFileSelectionClass))
  

  /**
   * HailFileSelection:
   *
   * Contains only private date.
   */
typedef struct _HailFileSelection                   HailFileSelection;
typedef struct _HailFileSelectionClass              HailFileSelectionClass;
  
struct _HailFileSelection
{
  GtkAccessible parent;
};

GType hail_file_selection_get_type (void);

struct _HailFileSelectionClass
{
  GtkAccessibleClass parent_class;
};

AtkObject* hail_file_selection_new (GtkWidget *widget);

G_END_DECLS


#endif /* __HAIL_FILE_SELECTION_H__ */
