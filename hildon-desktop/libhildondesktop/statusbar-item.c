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

#include "statusbar-item.h"

typedef enum
{
  STATUSBAR_ITEM_CONDITION,
  STATUSBAR_ITEM_CONDITION_LEGACY,
  SB_ITEM_SIGNALS
}
SBItemSignals;

enum
{
  SB_PROP_0,
  SB_PROP_CONDITION,
};

static gint statusbar_signals[SB_ITEM_SIGNALS];

/* static declarations */

static void statusbar_item_class_init      (StatusbarItemClass *item_class);
static void statusbar_item_init            (StatusbarItem *item);
static void statusbar_item_update_condition (StatusbarItem *item, gboolean condition);
static void statusbar_item_get_property    (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void statusbar_item_set_property    (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
/*
static void statusbar_finalize		   (GObject *object);
static void statusbar_destroy		   (GtkObject *object);*/

/*static DesktopItem *parent_class;*/

GType statusbar_item_get_type (void)
{
    static GType item_type = 0;

    if ( !item_type )
    {
        static const GTypeInfo item_info =
        {
            sizeof (StatusbarItemClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            ( GClassInitFunc ) statusbar_item_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof ( StatusbarItem ),
            0,    /* n_preallocs */
            (GInstanceInitFunc) statusbar_item_init,
        };
        item_type = g_type_register_static ( HILDON_DESKTOP_TYPE_PANEL_ITEM,
                                             "StatusbarItem",
                                             &item_info,
                                             0);
    }
    
    return item_type;
}

static void 
statusbar_item_class_init (StatusbarItemClass *item_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (item_class);
  
  item_class->condition_update = statusbar_item_update_condition;

  object_class->get_property  = statusbar_item_get_property;
  object_class->set_property  = statusbar_item_set_property;

  statusbar_signals[STATUSBAR_ITEM_CONDITION_LEGACY] = 
	g_signal_new("hildon-status-bar-update-conditional",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET (StatusbarItemClass,condition_update),
		     NULL, NULL,
		     g_cclosure_marshal_VOID__BOOLEAN, 
		     G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

  g_object_class_install_property (object_class,
                                   SB_PROP_CONDITION,
                                   g_param_spec_boolean("condition",
					   		"condition",
                                                        "plugin that cant'be destroyed",
                                                        TRUE,
                                                        G_PARAM_READWRITE));

}

static void 
statusbar_item_init (StatusbarItem *item)
{
  item->window = NULL;
  item->vbox   = NULL;

  item->condition = TRUE;
}

static void 
statusbar_item_update_condition (StatusbarItem *item, gboolean condition)
{
  g_object_set (G_OBJECT (item), "condition", condition, NULL);
}

static void 
statusbar_item_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  StatusbarItem *sbitem = STATUSBAR_ITEM (object);

  switch (prop_id)
  {
    case SB_PROP_CONDITION:
      g_value_set_boolean (value,sbitem->condition);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }	
}

static void 
statusbar_item_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  StatusbarItem *sbitem = STATUSBAR_ITEM (object);

  switch (prop_id)
  {
    case SB_PROP_CONDITION:
      sbitem->condition = g_value_get_boolean (value);
      g_object_notify (object, "condition");
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
