/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
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

#include <libhildondesktop/hildon-home-area.h>
#include <libhildondesktop/hildon-home-applet.h>

#include "hildon-home-select-applets-dialog.h"

#include <hildon-widgets/hildon-window.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkmain.h>
#include <glib/gmem.h>

#define MAD_GTKRC ".osso/current-gtk-theme.maemo_af_desktop"


typedef struct _Home {
  GtkWidget *window;
  GtkWidget *area;
  HildonPluginList *list;
} Home;


static void
home_select_applets (Home *home)
{
  GtkWidget *dialog;
  HildonHomeArea *area = HILDON_HOME_AREA (home->area);
  gint response;

  hildon_home_area_sync_to_list (area, home->list);
  dialog = hildon_home_select_applets_dialog_new_with_model (
                                        GTK_TREE_MODEL (home->list));

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  if (response == GTK_RESPONSE_OK)
    {
      hildon_home_area_sync_from_list (area, home->list);
    }

  gtk_widget_destroy (dialog);
}

static void
home_switch_layout_mode (Home *home)
{
  HildonHomeArea *area = HILDON_HOME_AREA (home->area);
  gboolean layout_mode;

  layout_mode = hildon_home_area_get_layout_mode (area);
  hildon_home_area_set_layout_mode (area, !layout_mode);
}

static void
home_create_menu (Home *home)
{
  GtkWidget *menu;
  GtkWidget *item;

  menu = gtk_menu_new ();

  item = gtk_menu_item_new_with_label ("Select applets");
  g_signal_connect_swapped (item, "activate",
                            G_CALLBACK (home_select_applets),
                            home);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  
  item = gtk_menu_item_new_with_label ("Layout mode");
  g_signal_connect_swapped (item, "activate",
                            G_CALLBACK (home_switch_layout_mode),
                            home);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  gtk_widget_show_all (menu);

  hildon_window_set_menu (HILDON_WINDOW (home->window), GTK_MENU (menu));

}




int
main (int argc, char **argv)
{
  Home *home;
  gchar *gtkrc;
  HDDesktopLoaderIface *loader;
  GKeyFile *keyfile;

  gtkrc = g_build_filename (g_getenv ("HOME"),
                            MAD_GTKRC,
                            NULL);
  if (gtkrc)
    gtk_rc_add_default_file (gtkrc);

  g_free (gtkrc);

  gtk_init (&argc, &argv);

  home = g_new0 (Home, 1);

  home->area = hildon_home_area_new ();
  home->window = hildon_window_new ();
  home->list = g_object_new (HILDON_TYPE_PLUGIN_LIST,
                             "name-key",     HH_APPLET_KEY_NAME,
                             "library-key",  HH_APPLET_KEY_LIBRARY,
                             "group",        HH_APPLET_GROUP,
                             NULL);

  hildon_plugin_list_set_directory (home->list,
                                    "/usr/share/applications/hildon-home");

  home_create_menu (home);

  gtk_container_add (GTK_CONTAINER (home->window), home->area);
  gtk_widget_show_all (home->window);

  loader = HD_PLUGIN_LOADER_IFACE (g_object_new (HD_PLUGIN_LOADER_LEGACY,
                                                 NULL));


  keyfile = g_key_file_new ();
  g_key_file_load_from_file (keyfile,
                             "/usr/share/applications/hildon-home/isearch-applet.desktop", NULL);

  hd_plugin_loader_load (loader, keyfile, NULL);

  gtk_main ();
  
  return 0;
}
