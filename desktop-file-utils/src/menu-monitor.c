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

#include <config.h>

#include "menu-monitor.h"

#include <string.h> /* for memset() */

struct MenuMonitor
{
  gpointer              *handle;
  MenuMonitorCallback    callback;
  gpointer               user_data;
};

static MenuMonitorAddFunc    monitor_add_func    = NULL;
static MenuMonitorRemoveFunc monitor_remove_func = NULL;

static MenuMonitor *
do_monitor_add (const char          *path,
		gboolean             monitor_dir,
		MenuMonitorCallback  callback,
		gpointer             user_data)
{
  MenuMonitor *monitor;

  monitor = NULL;

  if (monitor_add_func != NULL)
    {
      MenuMonitor *monitor;
      gpointer     handle;

      monitor = g_new0 (MenuMonitor, 1);

      handle = (* monitor_add_func) (monitor, path, monitor_dir);
      if (handle == NULL)
	{
	  g_free (monitor);
	  return NULL;
	}

      monitor->handle    = handle;
      monitor->callback  = callback;
      monitor->user_data = user_data;
    }

  /* FIXME: perhaps fall back to polling? */

  return monitor;
}

MenuMonitor *
menu_monitor_add_dir  (const char          *dir_path,
		       MenuMonitorCallback  callback,
		       gpointer             user_data)
{
  g_return_val_if_fail (dir_path != NULL, NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  return do_monitor_add (dir_path, TRUE, callback, user_data);
}

MenuMonitor *
menu_monitor_add_file (const char          *file_path,
		       MenuMonitorCallback  callback,
		       gpointer             user_data)
{
  g_return_val_if_fail (file_path != NULL, NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  return do_monitor_add (file_path, FALSE, callback, user_data);
}

void
menu_monitor_remove (MenuMonitor *monitor)
{
  g_return_if_fail (monitor != NULL);
  g_return_if_fail (monitor->handle != NULL);

  if (monitor_remove_func != NULL)
    {
      (* monitor_remove_func) (monitor->handle);
    }

  /* FIXME: race condition here; gnome-vfs queues up monitors
   * so it can co-alesce them; however when a monitor is cancelled
   * it doesn't remove all pending notifications but processes
   * them later. Therefore, we may still get notifications of file
   * changes after this point
   */
  memset (monitor, 0xaa, sizeof (MenuMonitor));
  g_free (monitor);
}

void
menu_monitor_do_callback (MenuMonitor      *monitor,
			  const char       *path,
			  MenuMonitorEvent  event)
{
  g_assert (monitor != NULL);
  /* See the menu_monitor_remove() FIXME */
  g_assert (((guchar *) monitor) [0] != 0xaa);
  g_assert (monitor->callback != NULL);

  monitor->callback (monitor, path, event, monitor->user_data);
}

void
menu_monitor_init (MenuMonitorAddFunc    add_func,
		   MenuMonitorRemoveFunc remove_func)
{
  g_return_if_fail (monitor_add_func == NULL);
  g_return_if_fail (monitor_remove_func == NULL);

  monitor_add_func    = add_func;
  monitor_remove_func = remove_func;
}
