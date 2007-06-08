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

#ifndef __HILDON_DESKTOP_PLUGIN_H__
#define __HILDON_DESKTOP_PLUGIN_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _HildonDesktopPlugin HildonDesktopPlugin;
typedef struct _HildonDesktopPluginClass HildonDesktopPluginClass;
typedef struct _HildonDesktopPluginPrivate HildonDesktopPluginPrivate;

#define HILDON_DESKTOP_TYPE_PLUGIN 	   (hildon_desktop_plugin_get_type ())
#define HILDON_DESKTOP_PLUGIN(o)   	   (G_TYPE_CHECK_INSTANCE_CAST ((o), HILDON_DESKTOP_TYPE_PLUGIN, HildonDesktopPlugin))
#define HILDON_DESKTOP_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), HILDON_DESKTOP_TYPE_PLUGIN, HildonDesktopPluginClass))
#define HILDON_DESKTOP_IS_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), HILDON_DESKTOP_TYPE_PLUGIN))
#define HILDON_DESKTOP_IS_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), HILDON_DESKTOP_TYPE_PLUGIN))
#define HILDON_DESKTOP_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), HILDON_DESKTOP_TYPE_PLUGIN, HildonDesktopPluginClass))

struct _HildonDesktopPlugin
{
  GTypeModule                  parent;

  GList                       *gtypes;

  HildonDesktopPluginPrivate  *priv;
};

struct _HildonDesktopPluginClass
{
  GTypeModuleClass parent_class;
};

GType                  hildon_desktop_plugin_get_type     (void);

HildonDesktopPlugin   *hildon_desktop_plugin_new          (const gchar *path);

GList                 *hildon_desktop_plugin_get_objects  (HildonDesktopPlugin *plugin);

void                   hildon_desktop_plugin_add_type     (HildonDesktopPlugin *plugin, 
                                                           GType                type);

#define HILDON_DESKTOP_PLUGIN_SYMBOLS(t_n)					 	\
G_MODULE_EXPORT void hildon_desktop_plugin_load (HildonDesktopPlugin *plugin);	 	\
void hildon_desktop_plugin_load (HildonDesktopPlugin *plugin)			 	\
{											\
  hildon_desktop_plugin_add_type (plugin, t_n##_register_type(G_TYPE_MODULE (plugin)));	\
}										 	\
G_MODULE_EXPORT void hildon_desktop_plugin_unload (HildonDesktopPlugin *plugin); 	\
void hildon_desktop_plugin_unload (HildonDesktopPlugin *plugin)			 	\
{										 	\
  (void) plugin;									\
}

#define HILDON_DESKTOP_PLUGIN_SYMBOLS_CODE(t_n, CODE_LOAD, CODE_UNLOAD)					 	\
G_MODULE_EXPORT void hildon_desktop_plugin_load (HildonDesktopPlugin *plugin);	 	\
void hildon_desktop_plugin_load (HildonDesktopPlugin *plugin)			 	\
{											\
  hildon_desktop_plugin_add_type (plugin, t_n##_register_type(G_TYPE_MODULE (plugin)));	\
  { CODE_LOAD }										\
}										 	\
G_MODULE_EXPORT void hildon_desktop_plugin_unload (HildonDesktopPlugin *plugin); 	\
void hildon_desktop_plugin_unload (HildonDesktopPlugin *plugin)			 	\
{										 	\
  { CODE_UNLOAD }									\
}

G_END_DECLS

#endif /*__HILDON_DESKTOP_PLUGIN_H__*/
