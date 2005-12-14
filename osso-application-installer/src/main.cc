/*
 * This file is part of osso-application-installer
 *
 * Parts of this file are derived from apt.  Apt is copyright 1997,
 * 1998, 1999 Jason Gunthorpe and others.
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

#include <stdio.h>
#include <iostream>

#include <gtk/gtk.h>

#include <apt-pkg/init.h>
#include <apt-pkg/error.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/pkgcachegen.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/acquire.h>
#include <apt-pkg/cachefile.h>

#include "acquire-status.h"
#include "operations.h"

#define _(x) x

extern "C" {
  #include "hildonbreadcrumbtrail.h"
}

using namespace std;

static void
window_destroy (GtkWidget* widget, gpointer data)
{
  gtk_main_quit ();
}

static const gchar *
get_label (GList *item)
{
  return (const gchar *)(item->data);
}

static void
clicked (GList *item)
{
  cerr << "clicked: " << get_label (item) << "\n";
}

GtkWidget *main_vbox;
GtkWidget *cur_view = NULL;

void
show_view (GtkWidget *w)
{
  if (cur_view != w)
    {
      if (cur_view)
	gtk_widget_hide (w);
      cur_view = w;
      gtk_widget_show (w);
    }
}

void
do_install (GtkWidget *btn, gpointer data)
{
  update_package_cache ();
}

void
main_view ()
{
  static GtkWidget *view = NULL;

  if (view == NULL)
    {
      GtkWidget *btn;

      view = gtk_vbox_new (TRUE, 10);

      gtk_box_pack_start (GTK_BOX (main_vbox), view, TRUE, TRUE, 0);
      btn = gtk_button_new_with_label ("Install Applications");
      gtk_box_pack_start (GTK_BOX (view), btn, TRUE, TRUE, 0);
      g_signal_connect (G_OBJECT (btn), "clicked",
			G_CALLBACK (do_install), NULL);

      gtk_box_pack_start
	(GTK_BOX (view),
	 gtk_button_new_with_label ("Upgrade Applications"),
	 TRUE, TRUE, 0);
      gtk_box_pack_start 
	(GTK_BOX (view),
	 gtk_button_new_with_label ("Uninstall Applications"),
	 TRUE, TRUE, 0);
      gtk_widget_show_all (view);
    }

  show_view (view);
}

int
main (int argc, char **argv)
{
  GtkWidget *window, *trail;
  GList *path;

  gtk_init (&argc, &argv);

  if (pkgInitConfig(*_config) == false ||
      pkgInitSystem(*_config,_system) == false)
   {
     _error->DumpErrors();
     return 100;
   }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (window), "Applications");
  
  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);

  trail = hildon_bread_crumb_trail_new (get_label, clicked);
  gtk_box_pack_start (GTK_BOX (main_vbox), trail, FALSE, FALSE, 0);

  path = NULL;
  path = g_list_append (path, (gpointer)"Main");
  hildon_bread_crumb_trail_set_path (trail, path);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (window_destroy), NULL);

  gtk_widget_show_all (window);

  main_view ();

  gtk_main ();
}
