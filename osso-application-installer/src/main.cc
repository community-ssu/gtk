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

#include "main.h"
#include "operations.h"
#include "util.h"
#include "details.h"
#include "menu.h"
#include "log.h"

#define _(x) x

extern "C" {
  #include "hildonbreadcrumbtrail.h"
  #include <hildon-widgets/hildon-app.h>
  #include <hildon-widgets/hildon-appview.h>
}

using namespace std;

static void set_details_callback (void (*func) (gpointer), gpointer data);
static void set_operation_callback (const char *label,
				    void (*func) (gpointer), gpointer data);

struct view {
  view *parent;
  const gchar *label;
  GtkWidget *(*maker) (view *);
  GList *path;
};

GtkWidget *main_vbox;
GtkWidget *main_trail;
GtkWidget *cur_view = NULL;
GList *cur_path;

static GList *
make_view_path (view *v)
{
  if (v == NULL)
    return NULL;
  else
    return g_list_append (make_view_path (v->parent), v);
}

void
show_view (view *v)
{
  if (cur_view)
    gtk_widget_destroy (cur_view);

  set_details_callback (NULL, NULL);
  set_operation_callback (NULL, NULL, NULL);

  cur_view = v->maker (v);

  if (v->path == NULL)
    v->path = make_view_path (v);
  hildon_bread_crumb_trail_set_path (main_trail, v->path);
  
  gtk_box_pack_start (GTK_BOX (main_vbox), cur_view, TRUE, TRUE, 0);
}

static void
show_view_callback (GtkWidget *btn, gpointer data)
{
  view *v = (view *)data;
  
  show_view (v);
}

static const gchar *
get_view_label (GList *node)
{
  return ((view *)node->data)->label;
}

static void
view_clicked (GList *node)
{
  show_view ((view *)node->data);
}

GtkWidget *make_main_view (view *v);
GtkWidget *make_install_applications_view (view *v);
GtkWidget *make_install_section_view (view *v);
GtkWidget *make_upgrade_applications_view (view *v);
GtkWidget *make_uninstall_applications_view (view *v);
 
view main_view = {
  NULL,
  "Main",
  make_main_view,
  NULL
};

view install_applications_view = {
  &main_view,
  "Install applications",
  make_install_applications_view,
  NULL
};

view upgrade_applications_view = {
  &main_view,
  "Upgrade applications",
  make_upgrade_applications_view,
  NULL
};

view uninstall_applications_view = {
  &main_view,
  "Uninstall applications",
  make_uninstall_applications_view,
  NULL
};

view install_section_view = {
  &install_applications_view,
  "XXX",
  make_install_section_view,
  NULL
};

GtkWidget *
make_main_view (view *v)
{
  GtkWidget *view;
  GtkWidget *btn;

  view = gtk_vbox_new (TRUE, 10);

  btn = gtk_button_new_with_label ("Install Applications");
  gtk_box_pack_start (GTK_BOX (view), btn, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (btn), "clicked",
		    G_CALLBACK (show_view_callback),
		    &install_applications_view);
  
  btn = gtk_button_new_with_label ("Upgrade Applications");
  gtk_box_pack_start (GTK_BOX (view), btn, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (btn), "clicked",
		    G_CALLBACK (show_view_callback),
		    &upgrade_applications_view);

  btn = gtk_button_new_with_label ("Uninstall Applications");
  gtk_box_pack_start (GTK_BOX (view), btn, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (btn), "clicked",
		    G_CALLBACK (show_view_callback),
		    &uninstall_applications_view);

  gtk_widget_show_all (view);

  return view;
}

static GList *sections;
static section_info *cur_section;

static void
append_user_size_delta (GString *str, gint delta,
			bool zero_is_positive)
{
  if (delta == 0)
    {
      if (zero_is_positive)
	g_string_append_printf (str, "No additional space will be used.\n");
      else
	g_string_append_printf (str, "No space will be freed.\n");
    }
  else if (delta > 0)
    g_string_append_printf (str, "%d additional bytes will be used.\n", delta);
  else
    g_string_append_printf (str, "%d bytes will be freed.\n", -delta);
}
    
static bool
confirm_install (package_info *pi)
{
  GString *text = g_string_new ("");
  bool response;

  simulate_install (pi);
  g_string_printf (text,
		   "Do you want to %s?\n%s\n%s\n",
		   pi->installed? "upgrade" : "install",
		   pi->name, pi->available->version);
  if (pi->install_possible)
    {
      g_string_append_printf (text, "%d bytes need to be downloaded.\n",
			      pi->download_size);
      append_user_size_delta (text, pi->install_user_size_delta, false);
    }

  response = ask_yes_no (text->str);
  g_string_free (text, 1);
  return response;
}

static void
install_package (package_info *pi)
{
  if (confirm_install (pi))
    {
      simulate_install (pi);
      if (pi->install_possible)
	{
	  bool success;

	  add_log ("\n------\n");
	  add_log ("Installing %s %s.\n", pi->name, pi->available->version);

	  /* XXX - We are going to invalidate the package db at one
	     point and need to make sure that no pointers into it are
	     used.  For example, redrawing the list of packages must
	     not happen.  That's why we stop using it before actually
	     invalidating it.
	  */

	  progress_with_cancel *pc = new progress_with_cancel ();
	  pc->set ("Installing", 0.0);
	  setup_install (pi);
	  success = do_operations (pc);
	  log_errors ();
	  operations_init ();
	  pc->close ();
	  delete pc;
	  
	  package_db_invalidated ();

	  if (success)
	    annoy_user ("Done");
	  else
	    annoy_user ("Failed, see log.");
	}
      else
	annoy_user ("Impossible, see details.");
    }
}

static void
available_package_details (gpointer data)
{
  package_info *pi = (package_info *)data;
  show_package_details (pi, false);
}

void
available_package_selected (package_info *pi)
{
  if (pi)
    {
      set_details_callback (available_package_details, pi);
      set_operation_callback ("Install", (void (*)(void*))install_package, pi);
    }
  else
    {
      set_details_callback (NULL, NULL);
      set_operation_callback (NULL, NULL, NULL);
    }
}

static void
installed_package_details (gpointer data)
{
  package_info *pi = (package_info *)data;
  show_package_details (pi, true);
}

static void uninstall_package (package_info *);

void
installed_package_selected (package_info *pi)
{
  if (pi)
    {
      set_details_callback (installed_package_details, pi);
      set_operation_callback ("Uninstall",
			      (void (*)(void*))uninstall_package, pi);
    }
  else
    {
      set_details_callback (NULL, NULL);
      set_operation_callback (NULL, NULL, NULL);
    }
}

GtkWidget *
make_install_section_view (view *v)
{
  GtkWidget *view;
  GList *packages;

  v->label = cur_section->name;

  view = make_package_list (cur_section->packages,
			    false,
			    available_package_selected, 
			    install_package);
  gtk_widget_show_all (view);

  return view;
}

static void
view_section (section_info *si)
{
  cur_section = si;
  show_view (&install_section_view);
}

GtkWidget *
make_install_applications_view (view *v)
{
  GtkWidget *view;

  set_operation_callback ("Install", NULL, NULL);
  update_package_cache ();
  sections = sectionize_packages (get_package_list (false, false),
				  false);
  cur_section = NULL;

  view = make_section_list (sections, view_section);
  gtk_widget_show_all (view);

  return view;
}

GtkWidget *
make_upgrade_applications_view (view *v)
{
  GtkWidget *view;
  GList *packages;

  update_package_cache ();
  packages = get_package_list (true, true);

  view = make_package_list (packages,
			    false,
			    available_package_selected,
			    install_package);
  gtk_widget_show_all (view);

  return view;
}

static bool
confirm_uninstall (package_info *pi)
{
  GString *text = g_string_new ("");
  bool response;

  simulate_uninstall (pi);
  g_string_printf (text,
		   "Do you want to uninstall?\n%s\n%s\n",
		   pi->name, pi->installed->version);
  if (pi->uninstall_possible)
    append_user_size_delta (text, pi->uninstall_user_size_delta, false);
  
  response = ask_yes_no (text->str);
  g_string_free (text, 1);
  return response;
}

static void
uninstall_package (package_info *pi)
{
  if (confirm_uninstall (pi))
    {
      simulate_uninstall (pi);
      if (pi->uninstall_possible)
	{
	  bool success;

	  add_log ("\n------\n");
	  add_log ("Removing %s %s.\n", pi->name, pi->installed->version);

	  /* XXX - We are going to invalidate the package db at one
	     point and need to make sure that no pointers into it are
	     used.  For example, redrawing the list of packages must
	     not happen.  That's why we stop using it before actually
	     invalidating it.
	  */

	  progress_with_cancel *pc = new progress_with_cancel ();
	  pc->set ("Uninstalling", 0.0);
	  setup_uninstall (pi);
	  success = do_operations (pc);
	  log_errors ();
	  operations_init ();
	  pc->close ();
	  delete pc;

	  package_db_invalidated ();

	  if (success)
	    annoy_user ("Done");
	  else
	    annoy_user ("Failed, see log.");
	}
      else
	annoy_user ("Impossible, see details.");
    }
}

GtkWidget *
make_uninstall_applications_view (view *v)
{
  GtkWidget *view;
  GList *packages;

  packages = get_package_list (true, false);

  view = make_package_list (packages,
			    true,
			    installed_package_selected,
			    uninstall_package);
  gtk_widget_show_all (view);

  return view;
}

static GtkWidget *details_button;
static void (*details_func) (gpointer);
static gpointer details_data;

void
show_current_details ()
{
  if (details_func)
    details_func (details_data);
}

static void
set_details_callback (void (*func) (gpointer), gpointer data)
{
  details_data = data;
  details_func = func;
  gtk_widget_set_sensitive (details_button, func != NULL);
  set_details_menu_sensitive (func != NULL);
}

static void (*operation_func) (gpointer);
static gpointer operation_data;

void
do_current_operation ()
{
  if (operation_func)
    operation_func (operation_data);
}

static void
set_operation_callback (const char *label, 
			void (*func) (gpointer), gpointer data)
{
  operation_data = data;
  operation_func = func;
  set_operation_menu_label (label, func != NULL);
}

static void
window_destroy (GtkWidget* widget, gpointer data)
{
  gtk_main_quit ();
}

void
package_db_invalidated ()
{
  show_view (&main_view);
}

int
main (int argc, char **argv)
{
  GtkWidget *app_view, *app;
  GtkWidget *toolbar, *image;
  GtkMenu *main_menu;

  gtk_init (&argc, &argv);
  operations_init ();

  app_view = hildon_appview_new (NULL);
  app = hildon_app_new_with_appview (HILDON_APPVIEW (app_view));
  hildon_app_set_title (HILDON_APP (app), "Applications");

  toolbar = gtk_toolbar_new ();
  image = gtk_image_new_from_icon_name ("qgn_toolb_gene_detailsbutton",
					HILDON_ICON_SIZE_26);
  details_button = GTK_WIDGET (gtk_tool_button_new (image, NULL));
  g_signal_connect (details_button, "clicked",
		    G_CALLBACK (show_current_details),
		    NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
		      GTK_TOOL_ITEM (details_button),
		      0);
  hildon_appview_set_toolbar (HILDON_APPVIEW (app_view),
			      GTK_TOOLBAR (toolbar));

  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (app_view), main_vbox);

  main_trail = hildon_bread_crumb_trail_new (get_view_label, view_clicked);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_trail, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), gtk_hseparator_new (), 
		      FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (app_view), "destroy",
		    G_CALLBACK (window_destroy), NULL);

  main_menu = hildon_appview_get_menu (HILDON_APPVIEW (app_view));
  create_menu (main_menu);

  gtk_widget_show_all (app);

  show_view (&main_view);

  // redirect_fd_to_log (1);
  redirect_fd_to_log (2);
  gtk_main ();
}
