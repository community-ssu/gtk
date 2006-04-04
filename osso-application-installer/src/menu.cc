/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <stdlib.h>
#include <libintl.h>

#include <gtk/gtk.h>

#include "menu.h"
#include "util.h"
#include "main.h"
#include "log.h"
#include "settings.h"
#include "repo.h"
#include "search.h"

#define _(x) gettext (x)

static GtkWidget *
add_item (GtkMenu *menu, const gchar *label, void (*func)())
{
  GtkWidget *item = gtk_menu_item_new_with_label (label);
  gtk_menu_append (menu, item);
  if (func)
    g_signal_connect (item, "activate", G_CALLBACK (func), NULL);
  return item;
}

static GtkWidget *
add_check (GtkMenu *menu, const gchar *label, void (*func)())
{
  GtkWidget *item = gtk_check_menu_item_new_with_label (label);
  gtk_menu_append (menu, item);
  if (func)
    g_signal_connect (item, "activate", G_CALLBACK (func), NULL);
  return item;
}

static GtkWidget *
add_radio (GtkMenu *menu, const gchar *label, void (*func)(), GtkWidget *group)
{
  GtkWidget *item;
  if (group)
    item = gtk_radio_menu_item_new_with_label_from_widget 
      (GTK_RADIO_MENU_ITEM (group), label);
  else
    item = gtk_radio_menu_item_new_with_label (NULL, label);

  gtk_menu_append (menu, item);
  if (func)
    g_signal_connect (item, "activate", G_CALLBACK (func), NULL);
  return item;
}

static GtkWidget *
add_sep (GtkMenu *menu)
{
  GtkWidget *item = gtk_separator_menu_item_new ();
  gtk_menu_append (menu, item);
  return item;
}

static GtkMenu *
add_menu (GtkMenu *menu, const gchar *label)
{
  GtkWidget *sub = gtk_menu_new ();
  GtkWidget *item = add_item (menu, label, NULL);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), sub);
  return GTK_MENU (sub);
}

static void
menu_close ()
{
  exit (0);
}

static GtkWidget *details_menu_item = NULL;
static GtkWidget *operation_menu_item = NULL;

void
set_details_menu_sensitive (bool flag)
{
  if (details_menu_item)
    gtk_widget_set_sensitive (details_menu_item, flag);
}

void
set_operation_menu_label (const char *label, bool sensitive)
{
  if (operation_menu_item)
    {
      gtk_label_set 
	(GTK_LABEL (gtk_bin_get_child (GTK_BIN (operation_menu_item))),
	 label);
      gtk_widget_set_sensitive (operation_menu_item, sensitive);
    }
}

static void
fullscreen_activated (GtkWidget *item)
{
  bool active =
    gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));
  set_fullscreen (active);
}

static GtkWidget *fullscreen_item;

void
set_fullscreen_menu_check (bool f)
{
  if (fullscreen_item)
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (fullscreen_item), f);
}

static void
normal_toolbar_activated (GtkWidget *item)
{
  bool active =
    gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));
  set_toolbar_visibility (false, active);
}

static void
fullscreen_toolbar_activated (GtkWidget *item)
{
  bool active =
    gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));
  set_toolbar_visibility (true, active);
}

void
create_menu (GtkMenu *main)
{
  GtkMenu *packages = add_menu (main, _("ai_me_package"));
  GtkMenu *view = add_menu (main, _("ai_me_view"));
  GtkMenu *tools = add_menu (main, _("ai_me_tools"));
  GtkWidget *fullscreen_group, *item;

  operation_menu_item = add_item (packages, "", do_current_operation);
  add_item (packages, _("ai_me_package_install_file"), install_from_file);
  details_menu_item = add_item (packages, _("ai_me_package_details"),
				show_current_details);

  add_item (view, _("ai_me_view_sort"), show_sort_settings_dialog);
  add_sep (view);
  fullscreen_item = add_check (view, _("ai_me_view_fullscreen"), NULL);
  g_signal_connect (fullscreen_item, "activate",
		    G_CALLBACK (fullscreen_activated), NULL);

  GtkMenu *toolbar = add_menu (view, _("ai_me_view_show_toolbar"));

  item = add_check (toolbar,
		    _("ai_me_view_show_toolbar_normal"), NULL);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
  g_signal_connect (item, "activate",
		    G_CALLBACK (normal_toolbar_activated), NULL);

  item = add_check (toolbar,
		    _("ai_me_view_show_toolbar_fullscreen"), NULL);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
  g_signal_connect (item, "activate",
		    G_CALLBACK (fullscreen_toolbar_activated), NULL);

  add_item (tools, _("ai_me_tools_refresh"), refresh_package_cache);
  add_item (tools, _("ai_me_tools_settings"), show_settings_dialog);
  add_item (tools, _("ai_me_tools_repository"), show_repo_dialog);
  add_item (tools, _("ai_me_tools_search"), show_search_dialog);
  add_item (tools, _("ai_me_tools_log"), show_log);
  add_item (tools, _("ai_me_tools_help"), show_help);

  add_item (main, _("ai_me_close"), menu_close);

  gtk_widget_show_all (GTK_WIDGET (main));
}
