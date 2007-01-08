/* Monitor abstraction */

/*
 * Copyright (C) 2004 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MENU_MONITOR_H__
#define __MENU_MONITOR_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct MenuMonitor MenuMonitor;

typedef enum
{
  MENU_MONITOR_CHANGED,
  MENU_MONITOR_CREATED,
  MENU_MONITOR_DELETED
} MenuMonitorEvent;

typedef void (* MenuMonitorCallback) (MenuMonitor      *monitor,
				      const char       *path,
				      MenuMonitorEvent  event,
				      gpointer          user_data);

MenuMonitor *menu_monitor_add_dir  (const char          *dir_path,
				    MenuMonitorCallback  callback,
				    gpointer             user_data);
MenuMonitor *menu_monitor_add_file (const char          *file_path,
				    MenuMonitorCallback  callback,
				    gpointer             user_data);
void         menu_monitor_remove   (MenuMonitor         *monitor);


/*
 * Hooks to actually implement monitoring
 */
typedef gpointer (* MenuMonitorAddFunc)    (MenuMonitor *monitor,
					    const char  *path,
					    gboolean     monitor_dir);
typedef void     (* MenuMonitorRemoveFunc) (gpointer     handle);

void menu_monitor_init        (MenuMonitorAddFunc     add_func,
			       MenuMonitorRemoveFunc  remove_func);
void menu_monitor_do_callback (MenuMonitor           *monitor,
			       const char            *path,
			       MenuMonitorEvent       event);

G_END_DECLS

#endif /* __MENU_MONITOR_H__ */
