/* GTK - The GIMP Toolkit
 * Copyright (C) 2005 Nokia Corporation.
 * Author: Jorn Baayen <jbaayen@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GTK_HASH_TABLE_H__
#define __GTK_HASH_TABLE_H__

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _GtkHashTable GtkHashTable;
typedef struct _GtkHashTableClass GtkHashTableClass;

#define GTK_TYPE_HASH_TABLE              (_gtk_hash_table_get_type ())
#define GTK_HASH_TABLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GTK_TYPE_HASH_TABLE, GtkHashTable))
#define GTK_HASH_TABLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_HASH_TABLE, GtkHashTableClass))
#define GTK_IS_HASH_TABLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GTK_TYPE_HASH_TABLE))
#define GTK_IS_HASH_TABLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_HASH_TABLE))
#define GTK_HASH_TABLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_HASH_TABLE, GtkHashTableClass))

struct _GtkHashTable
{
  GObject parent_instance;

  GHashTable *hash;
};

struct _GtkHashTableClass
{
  GObjectClass parent_class;
};

GType         _gtk_hash_table_get_type (void) G_GNUC_CONST;
GtkHashTable* _gtk_hash_table_new      (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_HASH_TABLE_H__ */


