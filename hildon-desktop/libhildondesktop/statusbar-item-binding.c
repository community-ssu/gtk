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

#include "statusbar-item-binding.h"

enum
{
  PROP_0,
  SB_I_BINDING_PROP_ID
};

static StatusbarItem *parent_class;

GObject *statusbar_item_binding_constructor (GType gtype, guint n_params, GObjectConstructParam *params);

static void statusbar_item_binding_class_init (StatusbarItemBindingClass *itembinding_class);

static void statusbar_item_binding_add (GtkContainer *container, GtkWidget *widget);

static void statusbar_item_binding_proxy_embedded (GtkPlug *binding, StatusbarItemBinding *itembinding);

static void statusbar_item_binding_iface_init (HildonDesktopItemPlugIface *iface);

static GdkNativeWindow statusbar_item_binding_get_id (HildonDesktopItemPlug *desktop_plug);

static void statusbar_item_binding_embedded (HildonDesktopItemPlug *itembinding);

GType statusbar_item_binding_get_type (void)
{
    static GType item_type = 0;

    if ( !item_type )
    {
        static const GTypeInfo item_info =
        {
            sizeof (StatusbarItemBindingClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) statusbar_item_binding_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (StatusbarItemBinding),
            0,    /* n_preallocs */
            NULL,
        };

        static const GInterfaceInfo item_binding_info =
	{
	  (GInterfaceInitFunc) statusbar_item_binding_iface_init,
	  NULL,
	  NULL 
	};

        item_type = g_type_register_static (STATUSBAR_TYPE_ITEM,
                                            "StatusbarItemBinding",
                                            &item_info,
                                            0);

        g_type_add_interface_static (item_type,
                                     HILDON_DESKTOP_TYPE_ITEM_PLUG,
                                     &item_binding_info);
    }
    
    return item_type;
}

static void 
statusbar_item_binding_iface_init (HildonDesktopItemPlugIface *iface)
{
  iface->get_id   = statusbar_item_binding_get_id;
  iface->embedded = statusbar_item_binding_embedded;
}

static void 
statusbar_item_binding_proxy_embedded (GtkPlug *binding, StatusbarItemBinding *itembinding)
{
  g_signal_emit_by_name (itembinding, "embedded");
}

GObject *
statusbar_item_binding_constructor (GType gtype, 
				    guint n_params, 
				    GObjectConstructParam *params)
{
  GObject *self;
  StatusbarItemBinding *itembinding;
  GdkNativeWindow wid;

  self = G_OBJECT_CLASS (parent_class)->constructor (gtype,
                                                     n_params,
                                                     params);

  g_object_get (self, "wid", &wid, NULL);

  itembinding = STATUSBAR_ITEM_BINDING (self);
	
  itembinding->plug = GTK_PLUG (gtk_plug_new (wid));
 
  gtk_container_add (GTK_CONTAINER (itembinding), GTK_WIDGET (itembinding->plug));
  gtk_widget_show (GTK_WIDGET (itembinding->plug));

  g_signal_connect (G_OBJECT (itembinding->plug), 
		    "embedded",
		    G_CALLBACK (statusbar_item_binding_proxy_embedded),
		    (gpointer)itembinding);

  return self;
}

static void 
statusbar_item_binding_class_init (StatusbarItemBindingClass *itembinding_class)
{
  GObjectClass	    *object_class    = G_OBJECT_CLASS (itembinding_class);	
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (itembinding_class);

  object_class->constructor = statusbar_item_binding_constructor;

  container_class->add = statusbar_item_binding_add;

  parent_class = g_type_class_peek_parent (itembinding_class);

  g_object_class_install_property (object_class,
                                   SB_I_BINDING_PROP_ID,
                                   g_param_spec_uint("wid",
					             "nwid",
                                                     "GdkNativeWindow ID",
						     0,
						     G_MAXUINT,
                                                     0,
                                                     G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

}

static void 
statusbar_item_binding_add (GtkContainer *container, GtkWidget *widget)
{
  g_assert (GTK_IS_CONTAINER (container) && 
	    GTK_IS_WIDGET (widget)       &&
	    STATUSBAR_IS_ITEM_BINDING (container));

  gtk_container_add (GTK_CONTAINER (STATUSBAR_ITEM_BINDING (container)->plug),widget);
}

static GdkNativeWindow 
statusbar_item_binding_get_id (HildonDesktopItemPlug *desktop_plug)
{
  StatusbarItemBinding *itembinding = (StatusbarItemBinding *) desktop_plug;

  return gtk_plug_get_id (itembinding->plug);
}

static void 
statusbar_item_binding_embedded (HildonDesktopItemPlug *desktop_plug)
{ 
  StatusbarItemBinding *itembinding = (StatusbarItemBinding *) desktop_plug;

  if (GTK_PLUG_GET_CLASS (itembinding->plug)->embedded)
    GTK_PLUG_GET_CLASS (itembinding->plug)->embedded (itembinding->plug);
}

/*
 * Public declarations
 */

StatusbarItemBinding *
statusbar_item_binding_new (GdkNativeWindow wid)
{
  return g_object_new (STATUSBAR_TYPE_ITEM_BINDING,
		       "wid",wid,
		       NULL);
}

