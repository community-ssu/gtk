/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
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

#ifndef HN_ITEM_DUMMY_H
#define HN_ITEM_DUMMY_H

#include "hildon-navigator-item.h"

G_BEGIN_DECLS

#define HN_TYPE_ITEM_DUMMY ( hn_item_dummy_get_type() )
#define HN_ITEM_DUMMY(obj) (GTK_CHECK_CAST (obj, \
            HN_TYPE_ITEM_DUMMY, \
            HNItemDummy))
#define HN_ITEM_DUMMY_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), \
            HN_TYPE_ITEM_DUMMY, HNItemDummyClass))
#define HN_IS_ITEM_DUMMY(obj) (GTK_CHECK_TYPE (obj, \
            HN_TYPE_ITEM_DUMMY))
#define HILDON_IS_ITEM_DUMMY_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
            HN_TYPE_ITEM_DUMMY_ITEM))

typedef struct _HNItemDummy HNItemDummy; 
typedef struct _HNItemDummyClass HNItemDummyClass;

struct _HNItemDummy
{
    HildonNavigatorItem parent;

    GtkWidget *button;
};

struct _HNItemDummyClass
{
    HildonNavigatorItemClass parent_class;

};


GType hn_item_dummy_get_type (void);

HNItemDummy *hn_item_dummy_new (const gchar *name);

G_END_DECLS

#endif /* HN_ITEM_DUMMY_H */
