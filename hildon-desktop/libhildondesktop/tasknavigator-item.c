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

#include "tasknavigator-item.h"

enum
{
  TN_PROP_0,
  TN_PROP_DUMMY
};


/* static declarations */

static void tasknavigator_item_class_init      (TaskNavigatorItemClass *item_class);
static void tasknavigator_item_init            (TaskNavigatorItem *item);
static void tasknavigator_item_get_property    (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void tasknavigator_item_set_property    (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
/*
static void tasknavigator_finalize		   (GObject *object);
static void tasknavigator_destroy		   (GtkObject *object);*/

/*static DesktopItem *parent_class;*/

GType tasknavigator_item_get_type (void)
{
    static GType tn_item_type = 0;

    if (!tn_item_type)
    {
        static const GTypeInfo tn_item_info =
        {
            sizeof (TaskNavigatorItemClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) tasknavigator_item_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (TaskNavigatorItem),
            0,    /* n_preallocs */
            (GInstanceInitFunc) tasknavigator_item_init,
        };
        tn_item_type = g_type_register_static (HILDON_DESKTOP_TYPE_PANEL_ITEM,
                                               "TaskNavigatorItem",
                                               &tn_item_info,
                                               0);
    }
    
    return tn_item_type;
}

static void 
tasknavigator_item_class_init (TaskNavigatorItemClass *item_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (item_class);
  
  object_class->get_property  = tasknavigator_item_get_property;
  object_class->set_property  = tasknavigator_item_set_property;

  /*
  g_object_class_install_property (object_class,
                                   SB_PROP_MANDATORY,
                                   g_param_spec_boolean("mandatory",
					   		"mandatory",
                                                        "mandaa plugin that cant'be destroyed",
                                                        FALSE,
                                                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
*/
}

static void 
tasknavigator_item_init (TaskNavigatorItem *item)
{
  item->menu   = NULL;
}

static void 
tasknavigator_item_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  /*TaskNavigatorItem *item = TASKNAVIGATOR_ITEM (object);*/

  switch (prop_id)
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }	
}

static void 
tasknavigator_item_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  /*TaskNavigatorItem *sbitem = TASKNAVIGATOR_ITEM (object);*/

  switch (prop_id)
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

