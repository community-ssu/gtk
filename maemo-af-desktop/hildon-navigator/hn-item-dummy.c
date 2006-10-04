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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "hn-item-dummy.h"

#include <gtk/gtktogglebutton.h>


/*
#include <libintl.h>
#include <locale.h>
*/

#include "hn-wm.h"

static HildonNavigatorItemClass *parent_class = NULL;

/* static declarations */

static void hn_item_dummy_finalize (GObject *object);

static void hn_item_dummy_class_init (HNItemDummyClass *item_class);

static void hn_item_dummy_init (HNItemDummy *item);

/* end static declarations */

static void 
hn_item_dummy_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void 
hn_item_dummy_class_init (HNItemDummyClass *item_class)
{
  GObjectClass      *object_class = G_OBJECT_CLASS   (item_class);

  parent_class = g_type_class_peek_parent (item_class);

  object_class->finalize = hn_item_dummy_finalize;

}

static void 
hn_item_dummy_init (HNItemDummy *dummy)
{
  g_return_if_fail (dummy); 

  gtk_widget_push_composite_child ();

  dummy->button = gtk_toggle_button_new ();  

  gtk_container_add (GTK_CONTAINER (dummy),dummy->button);
  
  GTK_BIN (dummy)->child = dummy->button;

  gtk_widget_pop_composite_child ();
}

GType hn_item_dummy_get_type (void)
{
    static GType dummy_type = 0;

    if ( !dummy_type )
    {
        static const GTypeInfo dummy_info =
        {
            sizeof ( HNItemDummyClass ),
            NULL, /* base_init */
            NULL, /* base_finalize */
            ( GClassInitFunc ) hn_item_dummy_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof ( HNItemDummy ),
            0,    /* n_preallocs */
            ( GInstanceInitFunc ) hn_item_dummy_init,
        };
        dummy_type = g_type_register_static ( HILDON_TYPE_NAVIGATOR_ITEM,
                                             "HildonNavigatorItemDummy",
                                             &dummy_info,
                                             0 );
    }
    
    return dummy_type;
}


HNItemDummy *
hn_item_dummy_new (const gchar *name)
{
  g_return_val_if_fail (name,NULL);

  return g_object_new (HN_TYPE_ITEM_DUMMY,"name",name,NULL);
}

