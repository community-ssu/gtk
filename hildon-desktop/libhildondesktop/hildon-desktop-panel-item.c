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

#include "hildon-desktop-panel-item.h"

/* static declarations */

static void hildon_desktop_panel_item_class_init (HildonDesktopPanelItemClass *item_class);
static void hildon_desktop_panel_item_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void hildon_desktop_panel_item_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

enum
{
  PROP_0,
  DESKTOP_P_ITEM_POS_PROP,
  DESKTOP_P_ITEM_POS_ORIENTATION,
  D_ITEM_PROPS
};

GType hildon_desktop_panel_item_get_type (void)
{
    static GType panel_item_type = 0;

    if ( !panel_item_type )
    {
        static const GTypeInfo panel_item_info =
        {
            sizeof (HildonDesktopPanelItemClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) hildon_desktop_panel_item_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (HildonDesktopPanelItem),
            0,    /* n_preallocs */
            NULL,
        };
        panel_item_type = g_type_register_static (HILDON_DESKTOP_TYPE_ITEM,
                                                  "HildonDesktopPanelItem",
                                                  &panel_item_info,0);
    }
    
    return panel_item_type;
}

static void 
hildon_desktop_panel_item_class_init (HildonDesktopPanelItemClass *item_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (item_class);

  object_class->get_property = hildon_desktop_panel_item_get_property;
  object_class->set_property = hildon_desktop_panel_item_set_property;
  
  item_class->get_position = hildon_desktop_panel_item_get_position;

  g_object_class_install_property (object_class,
                                   DESKTOP_P_ITEM_POS_PROP,
                                   g_param_spec_int("position",
                                                       "position",
                                                       "Position in the panel",
                                                       -1,
						       G_MAXINT,
						       -1,
                                                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   DESKTOP_P_ITEM_POS_ORIENTATION,
                                   g_param_spec_uint("orientation",
                                                     "orientation",
                                                     "Orientation",
                                                     GTK_ORIENTATION_HORIZONTAL,
						     GTK_ORIENTATION_VERTICAL,
						     GTK_ORIENTATION_VERTICAL,
                                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

}

static void 
hildon_desktop_panel_item_set_property (GObject *object,
                                        guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
  HildonDesktopPanelItem *item;
  GtkOrientation old_orientation;

  g_assert (object && HILDON_DESKTOP_IS_PANEL_ITEM (object));

  item = HILDON_DESKTOP_PANEL_ITEM (object);

  switch (prop_id)
  {
    case DESKTOP_P_ITEM_POS_PROP:	   
      item->position = g_value_get_int (value);
      break;

    case DESKTOP_P_ITEM_POS_ORIENTATION:
      old_orientation = item->orientation;
      item->orientation = g_value_get_uint (value);
      if (old_orientation == item->orientation)
	g_object_notify (object,"orientation");
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  } 
}

static void 
hildon_desktop_panel_item_get_property (GObject *object,
                                        guint prop_id,
                                        GValue *value,
                                        GParamSpec *pspec)
{
  HildonDesktopPanelItem *item;

  g_assert (object && HILDON_DESKTOP_IS_PANEL_ITEM (object));

  item = HILDON_DESKTOP_PANEL_ITEM (object);

  switch (prop_id)
  {
    case DESKTOP_P_ITEM_POS_PROP:
      g_value_set_int (value,item->position);
      break;
    case DESKTOP_P_ITEM_POS_ORIENTATION:
      g_value_set_uint (value,item->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;						
  }
}

gint 
hildon_desktop_panel_item_get_position (HildonDesktopPanelItem *item)
{
  g_return_val_if_fail (item && HILDON_DESKTOP_IS_PANEL_ITEM (item),-1);

  return item->position;
}

