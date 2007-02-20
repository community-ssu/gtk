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

#include "hildon-desktop-item-plug.h"
#include <hildon-desktop-item.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkmain.h>            /* For _gtk_boolean_handled_accumulator */

static void hildon_desktop_item_plug_base_init (gpointer g_class);

GType 
hildon_desktop_item_plug_get_type (void)
{
  static GType item_plug_type = 0;

  if (!item_plug_type)
  {
    static const GTypeInfo item_plug_info =
    {
      sizeof (HildonDesktopItemPlugIface), /* class_size */
      hildon_desktop_item_plug_base_init,   /* base_init */
      NULL,		/* base_finalize */
      NULL,
      NULL,		/* class_finalize */
      NULL,		/* class_data */
      0,
      0,
      NULL
    };

    item_plug_type =
      g_type_register_static (G_TYPE_INTERFACE, "HildonDesktopItemPlug",
			      &item_plug_info, 0);

    g_type_interface_add_prerequisite (item_plug_type, HILDON_DESKTOP_TYPE_ITEM);
    
  }

  return item_plug_type;
}

static void
hildon_desktop_item_plug_base_init (gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (!initialized)
  {
    g_signal_new ("embedded",
                  HILDON_DESKTOP_TYPE_ITEM_PLUG,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (HildonDesktopItemPlugIface, embedded),
                  NULL, NULL,
                  gtk_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

    initialized = TRUE;
  }
}

GdkNativeWindow 
hildon_desktop_item_plug_get_id (HildonDesktopItemPlug *itemplug)
{
  HildonDesktopItemPlugIface *iface;

  g_assert (HILDON_DESKTOP_IS_ITEM_PLUG (itemplug));

  iface = HILDON_DESKTOP_ITEM_PLUG_GET_IFACE (itemplug);

  g_assert (iface != NULL);
  g_return_val_if_fail (iface->get_id != NULL,0); /* FIXME: are you sure you can return this? */
  
  return (* iface->get_id) (itemplug);
}


