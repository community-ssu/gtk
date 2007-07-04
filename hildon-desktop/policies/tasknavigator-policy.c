/*
 * This file is part of python-hildondesktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
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

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libhildondesktop/hildon-desktop-item.h>
#include <libhildondesktop/tasknavigator-item.h>
#include <libhildondesktop/hildon-desktop-toggle-button.h>

#define HD_TN_BROWSER_PLUGIN       "hildon-task-navigator-bookmarks.desktop"
#define HD_TN_CONTACT_PLUGIN       "osso-contact-plugin.desktop"
#define HD_TN_APPS_MENU_PLUGIN     "applications-menu.desktop"
#define HD_TN_APP_SWITCHER_PLUGIN  "app-switcher.desktop"
#define HD_TN_SWITCHER_MENU_PLUGIN "switcher-menu.desktop"

G_MODULE_EXPORT GList             *hd_ui_policy_module_filter_plugin_list (GList *plugin_list);
G_MODULE_EXPORT gchar             *hd_ui_policy_module_get_default_item   (gint position);
G_MODULE_EXPORT HildonDesktopItem *hd_ui_policy_module_get_failure_item   (gint position);

static gboolean
is_gap_plugin (const gchar *plugin_id)
{
  if (!g_str_equal (plugin_id, HD_TN_ENTRY_PATH "/" HD_TN_APP_SWITCHER_PLUGIN) &&
      !g_str_equal (plugin_id, HD_TN_ENTRY_PATH "/" HD_TN_SWITCHER_MENU_PLUGIN))
  {
    return TRUE;
  }

  return FALSE;
}

GList *
hd_ui_policy_module_filter_plugin_list (GList *plugin_list)
{
  GList *i, *f_plugin_list = NULL;
  gboolean used_browser_plugin = FALSE;
  gboolean used_contact_plugin = FALSE;
  gboolean used_appsmenu_plugin = FALSE;
  gint position = 0;
  gint n_items;

  for (i = plugin_list; i && position < 3; i = i->next)
  {
    const gchar *plugin_id = (const gchar *) i->data;

    if (position == 2 && !used_appsmenu_plugin)
    {
      f_plugin_list = g_list_append (f_plugin_list, 
		                     g_strdup (HD_TN_ENTRY_PATH "/" HD_TN_APPS_MENU_PLUGIN));
      break;
    }
 
    if (is_gap_plugin (plugin_id))
    {
      f_plugin_list = g_list_append (f_plugin_list, g_strdup (plugin_id));
    }
    else
    {
      break;
    }

    if (g_str_equal (plugin_id, HD_TN_ENTRY_PATH "/" HD_TN_BROWSER_PLUGIN))
      used_browser_plugin = TRUE;

    if (g_str_equal (plugin_id, HD_TN_ENTRY_PATH "/" HD_TN_CONTACT_PLUGIN))
      used_contact_plugin = TRUE;
 
    if (g_str_equal (plugin_id, HD_TN_ENTRY_PATH "/" HD_TN_APPS_MENU_PLUGIN))
      used_appsmenu_plugin = TRUE;
    
    position++;
  }

  n_items = g_list_length (f_plugin_list);
  
  if (n_items < 3 && !used_browser_plugin)
  {
    f_plugin_list = g_list_prepend (f_plugin_list, 
		    		    g_strdup (HD_TN_ENTRY_PATH "/" HD_TN_BROWSER_PLUGIN));
    n_items++;
  }
  
  if (n_items < 3 && !used_contact_plugin)
  {
    f_plugin_list = g_list_append (f_plugin_list, 
		    		   g_strdup (HD_TN_ENTRY_PATH "/" HD_TN_CONTACT_PLUGIN));
  }

  if (n_items < 3 && !used_appsmenu_plugin)
  {
    f_plugin_list = g_list_append (f_plugin_list, 
		    		   g_strdup (HD_TN_ENTRY_PATH "/" HD_TN_APPS_MENU_PLUGIN));
  }

  f_plugin_list = g_list_append (f_plugin_list, 
		  		 g_strdup (HD_TN_ENTRY_PATH "/" HD_TN_APP_SWITCHER_PLUGIN));

  f_plugin_list = g_list_append (f_plugin_list, 
		  		 g_strdup (HD_TN_ENTRY_PATH "/" HD_TN_SWITCHER_MENU_PLUGIN));

  return f_plugin_list;
}

gchar *
hd_ui_policy_module_get_default_item (gint position)
{
  gchar *plugin_id = NULL;

  switch (position)
  {
    case 0:
      plugin_id = g_strdup (HD_TN_ENTRY_PATH "/" HD_TN_BROWSER_PLUGIN);
      break;

    case 1:
      plugin_id = g_strdup (HD_TN_ENTRY_PATH "/" HD_TN_CONTACT_PLUGIN);
      break;

    case 2:
      plugin_id = g_strdup (HD_TN_ENTRY_PATH "/" HD_TN_APPS_MENU_PLUGIN);
      break;

    case 3:
      plugin_id = g_strdup (HD_TN_ENTRY_PATH "/" HD_TN_APP_SWITCHER_PLUGIN);
      break;

    case 4:
      plugin_id = g_strdup (HD_TN_ENTRY_PATH "/" HD_TN_SWITCHER_MENU_PLUGIN);
      break;
  }

  return plugin_id;
}

HildonDesktopItem *
hd_ui_policy_module_get_failure_item (gint position)
{
  GObject *tnitem;
  GtkWidget *fake_button;

  tnitem = g_object_new (TASKNAVIGATOR_TYPE_ITEM, NULL);

  fake_button = hildon_desktop_toggle_button_new ();

  gtk_container_add (GTK_CONTAINER (tnitem), fake_button);
  gtk_widget_show (fake_button);

  return HILDON_DESKTOP_ITEM (tnitem);
}
