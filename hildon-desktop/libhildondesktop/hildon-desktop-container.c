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

#include "hildon-desktop-container.h"
#include <gtk/gtkcontainer.h>

static void hildon_desktop_container_base_init (gpointer g_class);

GType 
hildon_desktop_container_get_type (void)
{
  static GType container_type = 0;

  if (!container_type)
  {
    static const GTypeInfo container_info =
    {
      sizeof (HildonDesktopContainerIface), /* class_size */
      hildon_desktop_container_base_init,   /* base_init */
      NULL,		/* base_finalize */
      NULL,
      NULL,		/* class_finalize */
      NULL,		/* class_data */
      0,
      0,
      NULL
    };

    container_type =
      g_type_register_static (G_TYPE_INTERFACE, "HildonDesktopContainer",
			      &container_info, 0);

    g_type_interface_add_prerequisite (container_type, GTK_TYPE_CONTAINER);
    
  }

  return container_type;
}

static void
hildon_desktop_container_base_init (gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (!initialized)
  {
  /*FIXME: Do we have to initialize anything? */
  }
}

GList *
hildon_desktop_container_get_children (HildonDesktopContainer *container)
{
  HildonDesktopContainerIface *iface;

  g_return_val_if_fail (HILDON_DESKTOP_IS_CONTAINER (container), NULL);

  iface = HILDON_DESKTOP_CONTAINER_GET_IFACE (container);

  g_return_val_if_fail (iface != NULL, NULL);
  g_return_val_if_fail (iface->get_children != NULL, NULL);

  return (* iface->get_children) (container); 
}	

void 
hildon_desktop_container_remove (HildonDesktopContainer *container, GtkWidget *widget)
{
  HildonDesktopContainerIface *iface;

  g_return_if_fail (HILDON_DESKTOP_IS_CONTAINER (container));

  iface = HILDON_DESKTOP_CONTAINER_GET_IFACE (container);

  g_return_if_fail (iface != NULL);
  g_return_if_fail (iface->remove != NULL);

  return (* iface->remove) (container,widget); 
}
