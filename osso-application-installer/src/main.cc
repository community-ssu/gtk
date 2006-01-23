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

#include "apt-worker-client.h"
#include "apt-worker-proto.h"

#include "main.h"
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

void get_package_list_info (GList *packages);

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

  get_package_list_info (NULL);

  return view;
}

static GList *install_sections = NULL;
static GList *upgradeable_packages = NULL;
static GList *installed_packages = NULL;

static section_info *cur_section;

static void
free_packages (GList *list)
{
  // XXX
}

static void
free_sections (GList *list)
{
  // XXX
}

static void
free_all_packages ()
{
  if (install_sections)
    {
      free_sections (install_sections);
      install_sections = NULL;
    }

  if (upgradeable_packages)
    {
      free_packages (install_sections);
      upgradeable_packages = NULL;
    }
    
  if (installed_packages)
    {
      free_packages (install_sections);
      installed_packages = NULL;
    }
}

static section_info *
find_section_info (GList **list_ptr, const char *name)
{
  if (name == NULL)
    name = "none";

  for (GList *ptr = *list_ptr; ptr; ptr = ptr->next)
    if (!strcmp (((section_info *)ptr->data)->name, name))
      return (section_info *)ptr->data;

  section_info *si = new section_info;
  si->name = name;
  si->packages = NULL;
  *list_ptr = g_list_prepend (*list_ptr, si);
  return si;
}

static void
get_package_list_reply (int cmd, char *response, int len, void *loop)
{
  if (response)
    {
      apt_proto_decoder dec (response, len);
      int count = 0, new_p = 0, upg_p = 0, inst_p = 0;

      while (!dec.at_end ())
	{
	  package_info *info = new package_info;
	  info->name = dec.decode_string_dup ();
	  info->installed_version = dec.decode_string_dup ();
	  info->installed_size = dec.decode_int ();
	  info->available_version = dec.decode_string_dup ();
	  info->available_section = dec.decode_string_dup ();

	  info->have_info = false;
#if 0	  
	  printf ("%s %s (%d) %s (%s)\n",
		  name,
		  installed? installed : "<none>", installed_size,
		  available? available : "<none>", section);
#endif
	  
	  count++;
	  if (info->installed_version && info->available_version)
	    {
	      upgradeable_packages = g_list_prepend (upgradeable_packages,
						     info);
	      upg_p++;
	    }
	  else if (info->available_version)
	    {
	      section_info *sec = find_section_info (&install_sections,
						     info->available_section);
	      sec->packages = g_list_prepend (sec->packages,
					      info);
	      new_p++;
	    }

	  if (info->installed_version)
	    {
	      installed_packages = g_list_prepend (installed_packages,
						   info);
	      inst_p++;
	    }
	}

      printf ("%d packages, %d new, %d upgradable, %d installed\n",
	      count, new_p, upg_p, inst_p);

      if (dec.corrupted ())
	printf ("reply corrupted.\n");
    }
  else
    printf ("bad get_package_list reply");

  if (loop)
    g_main_loop_quit ((GMainLoop *)loop);
}

static void
get_package_list ()
{
  GMainLoop *loop = g_main_loop_new (NULL, 0);
  apt_worker_get_package_list (get_package_list_reply, loop);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);
}

struct gpi_closure {
  void (*func) (package_info *, void *);
  void *data;
  package_info *pi;
};

static void
gpi_reply  (int cmd, char *response, int len, void *clos)
{
  gpi_closure *c = (gpi_closure *)clos;
  void (*func) (package_info *, void *) = c->func;
  void *data = c->data;
  package_info *pi = c->pi;
  delete c;
  
  if (response)
    {
      apt_proto_decoder dec (response, len);
      dec.decode_mem (&(pi->info), sizeof (pi->info));
      pi->installed_short_description = dec.decode_string_dup ();
      pi->available_short_description = dec.decode_string_dup ();
      if (!dec.corrupted ())
	{
	  pi->have_info = true;
	  func (pi, data);
	}
    }
}

void
call_with_package_info (package_info *pi,
			void (*func) (package_info *, void *), void *data)
{
  if (pi->have_info)
    func (pi, data);
  else
    {
      gpi_closure *c = new gpi_closure;
      c->func = func;
      c->data = data;
      c->pi = pi;
      apt_worker_get_package_info (pi->name, gpi_reply, c);
    }
}

static GList *cur_packages_for_info;
static GList *next_packages_for_info;

static void
get_next_package_info (package_info *pi, void *unused)
{
  if (pi)
    printf ("info: %s\n", pi->name);

  cur_packages_for_info = next_packages_for_info;
  if (cur_packages_for_info)
    {
      next_packages_for_info = cur_packages_for_info->next;
      pi = (package_info *)cur_packages_for_info->data;
      call_with_package_info (pi, get_next_package_info, NULL);
    }
}

void
get_package_list_info (GList *packages)
{
  next_packages_for_info = packages;
  if (cur_packages_for_info == NULL)
    get_next_package_info (NULL, NULL);
}

static void
append_user_size_delta (GString *str, int delta,
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

  g_string_printf (text,
		   "Do you want to %s?\n%s\n%s\n",
		   pi->installed_version? "upgrade" : "install",
		   pi->name, pi->available_version);

  response = ask_yes_no (text->str);
  g_string_free (text, 1);
  return response;
}

static void
install_package (package_info *pi)
{
  if (confirm_install (pi))
    annoy_user ("Not yet, sorry.");
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

  get_package_list_info (cur_section->packages);

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
  cur_section = NULL;
  view = make_section_list (install_sections, view_section);
  gtk_widget_show_all (view);

  get_package_list_info (NULL);

  return view;
}

GtkWidget *
make_upgrade_applications_view (view *v)
{
  GtkWidget *view;
  GList *packages;

  view = make_package_list (upgradeable_packages,
			    false,
			    available_package_selected,
			    install_package);
  gtk_widget_show_all (view);

  get_package_list_info (upgradeable_packages);

  return view;
}

static bool
confirm_uninstall (package_info *pi)
{
  GString *text = g_string_new ("");
  bool response;

  g_string_printf (text,
		   "Do you want to uninstall?\n%s\n%s\n",
		   pi->name, pi->installed_version);
  
  response = ask_yes_no (text->str);
  g_string_free (text, 1);
  return response;
}

static void
uninstall_package (package_info *pi)
{
  if (confirm_uninstall (pi))
    annoy_user ("Not yet, sorry.");
}

GtkWidget *
make_uninstall_applications_view (view *v)
{
  GtkWidget *view;
  GList *packages;

  view = make_package_list (installed_packages,
			    true,
			    installed_package_selected,
			    uninstall_package);
  gtk_widget_show_all (view);

  get_package_list_info (installed_packages);

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

static void
apt_status_callback (int cmd, char *response, int len, void *unused)
{
  if (response)
    {
      apt_proto_decoder dec (response, len);
      char *label = dec.decode_string_dup ();
      int percent = dec.decode_int ();

      printf ("STATUS: %3d %s\n", percent, label);

      free (label);
    }
}

static gboolean
handle_apt_worker (GIOChannel *channel, GIOCondition cond, gpointer data)
{
  if (apt_worker_is_running ())
    {
      handle_one_apt_worker_response ();
      return TRUE;
    }
  else
    return FALSE;
}

void
add_apt_worker_handler ()
{
  GIOChannel *channel = g_io_channel_unix_new (apt_worker_in_fd);
  g_io_add_watch (channel, GIOCondition (G_IO_IN | G_IO_HUP | G_IO_ERR),
		  handle_apt_worker, NULL);
  g_io_channel_unref (channel);
}

int
main (int argc, char **argv)
{
  GtkWidget *app_view, *app;
  GtkWidget *toolbar, *image;
  GtkMenu *main_menu;

  gtk_init (&argc, &argv);

  start_apt_worker ();
  apt_worker_set_status_callback (apt_status_callback, NULL);
  add_apt_worker_handler ();

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

  get_package_list ();
  gtk_main ();
}
