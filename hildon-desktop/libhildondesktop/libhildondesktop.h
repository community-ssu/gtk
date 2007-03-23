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

#ifndef __LIBDESKTOP_H__
#define __LIBDESKTOP_H__

#include <libhildondesktop/hildon-desktop-panel-window.h>
#include <libhildondesktop/hildon-desktop-panel-window-dialog.h>
#include <libhildondesktop/hildon-desktop-multiscreen.h>
#include <libhildondesktop/hildon-desktop-panel.h>
#include <libhildondesktop/hildon-desktop-panel-expandable.h>

#include <libhildondesktop/hildon-desktop-item.h>
#include <libhildondesktop/hildon-desktop-panel-item.h>
#include <libhildondesktop/hildon-desktop-home-item.h>

#include <libhildondesktop/hildon-desktop-plugin.h>

#include <libhildondesktop/statusbar-item.h>
#include <libhildondesktop/statusbar-item-socket.h>

#include <libhildondesktop/tasknavigator-item.h>
#include <libhildondesktop/tasknavigator-item-socket.h>

#include <libhildondesktop/hildon-home-area.h>

#include <libhildondesktop/hildon-desktop-notification-manager.h>

#include <libhildondesktop/libhildonmenu.h>
#include <libhildondesktop/hildon-thumb-menu-item.h>

#define HD_DEFINE_PLUGIN_EXTENDED(TN, t_n, T_P, CODE_LOAD, CODE_UNLOAD)      \
HD_DEFINE_TYPE_MODULE_EXTENDED (TN, t_n, T_P, 0, {})       		     \
HILDON_DESKTOP_PLUGIN_SYMBOLS_CODE (TN, t_n, T_P, 0, CODE_LOAD, CODE_UNLOAD)

#define HD_DEFINE_PLUGIN(TN, t_n, T_P)			\
HD_DEFINE_TYPE_MODULE_EXTENDED (TN, t_n, T_P, 0, {})	\
HILDON_DESKTOP_PLUGIN_SYMBOLS (t_n)

#define HD_DEFINE_PLUGIN_WITH_CODE(TN, t_n, T_P, CODE)  \
HD_DEFINE_TYPE_MODULE_EXTENDED (TN, t_n, T_P, 0, CODE)	\
HILDON_DESKTOP_PLUGIN_SYMBOLS (t_n)

#define HD_DEFINE_TYPE_MODULE_EXTENDED(TypeName, type_name, TYPE_PARENT, flags, CODE)	\
											\
static GType    type_name##_type_id = 0;                                              	\
											\
static void     type_name##_init (TypeName *self); 					\
static void     type_name##_class_init (TypeName##Class *klass); 			\
static gpointer type_name##_parent_class = NULL; 					\
											\
static void     type_name##_class_intern_init (gpointer klass) 				\
{ 											\
  type_name##_parent_class = g_type_class_peek_parent (klass); 				\
  type_name##_class_init ((TypeName##Class*) klass); 					\
} 											\
											\
GType 											\
type_name##_get_type (void) 								\
{ 											\
  return type_name##_type_id;								\
}											\
											\
static GType 										\
type_name##_register_type (GTypeModule *module) 					\
{ 											\
  if (G_UNLIKELY (type_name##_type_id == 0)) 						\
  { 											\
    static const GTypeInfo g_define_type_info = 					\
    { 											\
      sizeof (TypeName##Class), 							\
      (GBaseInitFunc) NULL, 								\
      (GBaseFinalizeFunc) NULL, 							\
      (GClassInitFunc) type_name##_class_intern_init, 					\
      (GClassFinalizeFunc) NULL, 							\
      NULL,   /* class_data */ 								\
      sizeof (TypeName), 								\
      0,      /* n_preallocs */ 							\
      (GInstanceInitFunc) type_name##_init, 						\
      NULL    /* value_table */ 							\
    }; 											\
											\
    type_name##_type_id = 								\
	g_type_module_register_type (module, 						\
                                     TYPE_PARENT, 					\
                                     #TypeName, 					\
                                     &g_define_type_info, 				\
                                     (GTypeFlags) flags); 				\
    { CODE } 										\
  } 											\
											\
  return type_name##_type_id; 								\
}

#endif /*__LIBDESKTOP_H__*/
