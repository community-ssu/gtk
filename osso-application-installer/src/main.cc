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
#include <assert.h>
#include <iostream>

#include <gtk/gtk.h>

#include "apt-worker-client.h"
#include "apt-worker-proto.h"

#include "main.h"
#include "util.h"
#include "details.h"
#include "menu.h"
#include "log.h"
#include "settings.h"

#define _(x) x

extern "C" {
  #include "hildonbreadcrumbtrail.h"
  #include <hildon-widgets/hildon-app.h>
  #include <hildon-widgets/hildon-appview.h>
  #include <libosso.h>
}

using namespace std;

static guint apt_source_id;

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
view *cur_view_struct;
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
  cur_view_struct = v;

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
GtkWidget *make_search_results_view (view *v);

view main_view = {
  NULL,
  "Main",
  make_main_view,
  NULL
};

view install_applications_view = {
  &main_view,
  "Install new",
  make_install_applications_view,
  NULL
};

view upgrade_applications_view = {
  &main_view,
  "Update",
  make_upgrade_applications_view,
  NULL
};

view uninstall_applications_view = {
  &main_view,
  "Installed applications",
  make_uninstall_applications_view,
  NULL
};

view install_section_view = {
  &install_applications_view,
  NULL,
  make_install_section_view,
  NULL
};

view search_results_view = {
  &main_view,
  "Search results",
  make_search_results_view,
  NULL
};

GtkWidget *
make_main_view (view *v)
{
  GtkWidget *view;
  GtkWidget *vbox;
  GtkWidget *btn;
  
  view = gtk_hbox_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 10);
  gtk_box_pack_start (GTK_BOX (view), vbox, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox),
		      gtk_label_new ("My Device Nokia 770"),
		      FALSE, FALSE, 5);
  
  btn = gtk_button_new_with_label ("List installed applications");
  gtk_box_pack_start (GTK_BOX (vbox), btn, FALSE, FALSE, 5);
  g_signal_connect (G_OBJECT (btn), "clicked",
		    G_CALLBACK (show_view_callback),
		    &uninstall_applications_view);

  gtk_box_pack_start (GTK_BOX (vbox),
		      gtk_label_new ("Repository"),
		      FALSE, FALSE, 5);

  btn = gtk_button_new_with_label ("Install new applications");
  gtk_box_pack_start (GTK_BOX (vbox), btn, FALSE, FALSE, 5);
  g_signal_connect (G_OBJECT (btn), "clicked",
		    G_CALLBACK (show_view_callback),
		    &install_applications_view);
  
  btn = gtk_button_new_with_label ("Check for updates");
  gtk_box_pack_start (GTK_BOX (vbox), btn, FALSE, FALSE, 5);
  g_signal_connect (G_OBJECT (btn), "clicked",
		    G_CALLBACK (show_view_callback),
		    &upgrade_applications_view);

  gtk_widget_show_all (view);

  get_package_list_info (NULL);

  return view;
}

static GList *install_sections = NULL;
static GList *upgradeable_packages = NULL;
static GList *installed_packages = NULL;
static GList *search_result_packages = NULL;

static section_info *cur_section;

package_info::package_info ()
{
  ref_count = 1;
  name = NULL;
  installed_version = NULL;
  available_version = NULL;
  available_section = NULL;
  have_info = false;
  installed_short_description = NULL;
  available_short_description = NULL;
  model = NULL;
}

package_info::~package_info ()
{
  g_free (name);
  g_free (installed_version);
  g_free (available_version);
  g_free (available_section);
  g_free (installed_short_description);
  g_free (available_short_description);
}

void
package_info::ref ()
{
  ref_count += 1;
}

void
package_info::unref ()
{
  ref_count -= 1;
  if (ref_count == 0)
    delete this;
}

static void
free_packages (GList *list)
{
  for (GList *p = list; p; p = p->next)
    ((package_info *)p->data)->unref ();
  g_list_free (list);
}

static void
free_sections (GList *list)
{
  for (GList *s = list; s; s = s->next)
    {
      section_info *si = (section_info *)s->data;
      free_packages (si->packages);
      delete si;
    }
  g_list_free (list);
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
      free_packages (upgradeable_packages);
      upgradeable_packages = NULL;
    }
    
  if (installed_packages)
    {
      free_packages (installed_packages);
      installed_packages = NULL;
    }

  if (search_result_packages)
    {
      free_packages (search_result_packages);
      search_result_packages = NULL;
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

static void set_global_lists_for_view (view *);

static gint
compare_section_names (gconstpointer a, gconstpointer b)
{
  section_info *si_a = (section_info *)a;
  section_info *si_b = (section_info *)b;

  return package_sort_sign * g_ascii_strcasecmp (si_a->name, si_b->name);
}

static gint
compare_package_names (gconstpointer a, gconstpointer b)
{
  package_info *pi_a = (package_info *)a;
  package_info *pi_b = (package_info *)b;

  return package_sort_sign * g_ascii_strcasecmp (pi_a->name, pi_b->name);
}

static gint
compare_versions (const gchar *a, const gchar *b)
{
  // XXX - wrong, of course
  return package_sort_sign * g_ascii_strcasecmp (a, b);
}

static gint
compare_package_installed_versions (gconstpointer a, gconstpointer b)
{
  package_info *pi_a = (package_info *)a;
  package_info *pi_b = (package_info *)b;

  return compare_versions (pi_a->installed_version, pi_b->installed_version);
}

static gint
compare_package_available_versions (gconstpointer a, gconstpointer b)
{
  package_info *pi_a = (package_info *)a;
  package_info *pi_b = (package_info *)b;

  return compare_versions (pi_a->available_version, pi_b->available_version);
}

static gint
compare_package_installed_sizes (gconstpointer a, gconstpointer b)
{
  package_info *pi_a = (package_info *)a;
  package_info *pi_b = (package_info *)b;

  return (package_sort_sign *
	  (pi_a->installed_size - pi_b->installed_size));
}

static gint
compare_package_download_sizes (gconstpointer a, gconstpointer b)
{
  package_info *pi_a = (package_info *)a;
  package_info *pi_b = (package_info *)b;

  // Download size might not be known when we sort so we sort by name
  // instead in that case.
  
  if (pi_a->have_info && pi_b->have_info)
    return (package_sort_sign * 
	    (pi_a->info.download_size - pi_b->info.download_size));
  else
    return compare_package_names (a, b);
}

void
sort_all_packages ()
{
  install_sections = g_list_sort (install_sections,
				  compare_section_names);

  GCompareFunc compare_packages_inst = compare_package_names;
  GCompareFunc compare_packages_avail = compare_package_names;
  if (package_sort_key == SORT_BY_VERSION)
    {
      compare_packages_inst = compare_package_installed_versions;
      compare_packages_avail = compare_package_available_versions;
    }
  else if (package_sort_key == SORT_BY_SIZE)
    {
      compare_packages_inst = compare_package_installed_sizes;
      compare_packages_avail = compare_package_download_sizes;
    }

  for (GList *s = install_sections; s; s = s->next)
    {
      section_info *si = (section_info *)s->data;
      si->packages = g_list_sort (si->packages,
				  compare_packages_avail);
    }

  installed_packages = g_list_sort (installed_packages,
				    compare_packages_inst);

  upgradeable_packages = g_list_sort (upgradeable_packages,
				      compare_packages_avail);

  set_global_lists_for_view (cur_view_struct);
}

static void
get_package_list_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  int count = 0, new_p = 0, upg_p = 0, inst_p = 0;

  set_global_package_list (NULL, false, NULL, NULL);
  set_global_section_list (NULL, NULL);
  free_all_packages ();
  
  while (!dec->at_end ())
    {
      package_info *info = new package_info;
      info->name = dec->decode_string_dup ();
      info->installed_version = dec->decode_string_dup ();
      info->installed_size = dec->decode_int ();
      info->available_version = dec->decode_string_dup ();
      info->available_section = dec->decode_string_dup ();
      
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

  sort_all_packages ();

  if (cur_view_struct == &search_results_view)
    show_view (search_results_view.parent);
}

void
get_package_list ()
{
  apt_worker_get_package_list (!red_pill_mode,
			       false, 
			       false, 
			       NULL,
			       get_package_list_reply, NULL);
}

struct gpi_closure {
  void (*func) (package_info *, void *, bool);
  void *data;
  package_info *pi;
};

static void
gpi_reply  (int cmd, apt_proto_decoder *dec, void *clos)
{
  gpi_closure *c = (gpi_closure *)clos;
  void (*func) (package_info *, void *, bool) = c->func;
  void *data = c->data;
  package_info *pi = c->pi;
  delete c;
  
  dec->decode_mem (&(pi->info), sizeof (pi->info));
  pi->installed_short_description = dec->decode_string_dup ();
  pi->available_short_description = dec->decode_string_dup ();
  if (!dec->corrupted ())
    {
      pi->have_info = true;
      func (pi, data, true);
      pi->unref ();
    }
}

void
call_with_package_info (package_info *pi,
			void (*func) (package_info *, void *, bool),
			void *data)
{
  if (pi->have_info)
    func (pi, data, false);
  else
    {
      gpi_closure *c = new gpi_closure;
      c->func = func;
      c->data = data;
      c->pi = pi;
      pi->ref ();
      apt_worker_get_package_info (pi->name, gpi_reply, c);
    }
}

static GList *cur_packages_for_info;
static GList *next_packages_for_info;

void row_changed (GtkTreeModel *model, GtkTreeIter *iter);

static package_info *intermediate_info;
static void (*intermediate_callback) (package_info *, void*, bool);
static void *intermediate_data;

static void
get_next_package_info (package_info *pi, void *unused, bool changed)
{
  if (pi && changed)
    global_package_info_changed (pi);

  if (pi && pi == intermediate_info)
    {
      intermediate_info = NULL;
      if (intermediate_callback)
	intermediate_callback (pi, intermediate_data, changed);
    }

  if (intermediate_info)
    call_with_package_info (intermediate_info, get_next_package_info, NULL);
  else
    {
      cur_packages_for_info = next_packages_for_info;
      if (cur_packages_for_info)
	{
	  next_packages_for_info = cur_packages_for_info->next;
	  pi = (package_info *)cur_packages_for_info->data;
	  call_with_package_info (pi, get_next_package_info, NULL);
	}
    }
}

void
get_package_list_info (GList *packages)
{
  next_packages_for_info = packages;
  if (cur_packages_for_info == NULL && intermediate_info == NULL)
    get_next_package_info (NULL, NULL, false);
}

void
get_intermediate_package_info (package_info *pi,
			       void (*callback) (package_info *, void *, bool),
			       void *data)
{
  package_info *old_intermediate_info = intermediate_info;

  if (pi->have_info)
    {
      if (callback)
	callback (pi, data, false);
    }
  else if (intermediate_info == NULL || intermediate_callback == NULL)
    {
      intermediate_info = pi;
      intermediate_callback = callback;
      intermediate_data = data;
    }
  else
    printf ("package info request already pending.\n");

  if (cur_packages_for_info == NULL && old_intermediate_info == NULL)
    get_next_package_info (NULL, NULL, false);
}

static void
refresh_package_cache_reply (int cmd, apt_proto_decoder *dec, void *clos)
{
  hide_progress ();

  int success = dec->decode_int ();
  get_package_list ();

  if (!dec->corrupted () && success)
    annoy_user ("Done.");
  else
    annoy_user_with_log ("Failed, see log.");
}

static bool refreshed_this_session = false;

static void
refresh_package_cache_cont (bool res, void *unused)
{
  if (res)
    {
      refreshed_this_session = true;
      last_update = time (NULL);
      save_settings ();

      show_progress ("Refreshing");
      apt_worker_update_cache (refresh_package_cache_reply, NULL);
    }
}

void
refresh_package_cache ()
{
  ask_yes_no ("Refresh package list?", refresh_package_cache_cont, NULL);
}

static int
days_elapsed_since (time_t past)
{
  time_t now = time (NULL);
  if (now == (time_t)-1)
    return 0;

  /* Truncating is the right rounding mode here.
   */
  return (now - past) / (24*60*60);
}

void
maybe_refresh_package_cache ()
{
  if (update_interval_index == UPDATE_INTERVAL_NEVER)
    return;

  if (update_interval_index == UPDATE_INTERVAL_SESSION
      && refreshed_this_session)
    return;

  if (update_interval_index == UPDATE_INTERVAL_WEEK
      && days_elapsed_since (last_update) < 7)
    return;

  if (update_interval_index == UPDATE_INTERVAL_MONTH
      && days_elapsed_since (last_update) < 30)
    return;

  refresh_package_cache ();
}

static bool
confirm_install (package_info *pi,
		 void (*cont) (bool res, void *data), void *data)
{
  GString *text = g_string_new ("");

  g_string_printf (text,
		   "%s\n"
		   "%s %s",
		   (pi->installed_version
		    ? "Do you want to update?"
		    : "Do you want to install?"),
		   pi->name, pi->available_version);
  
  if (pi->info.installable)
    {
      char download_buf[20];
      size_string_general (download_buf, 20, pi->info.download_size);
      g_string_append_printf (text, "\n%s", download_buf);
    }

  ask_yes_no (text->str, cont, data);
  g_string_free (text, 1);
}

static void
clean_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  /* Failure messages are in the log.  We don't annoy the user here.
     However, if cleaning takes really long, the user might get
     confused since apt-worker is not responding.
   */
}

static void
install_package_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  int success = dec->decode_int ();

  hide_progress ();
  get_package_list ();

  if (clean_after_install)
    apt_worker_clean (clean_reply, NULL);

  if (!dec->corrupted () && success)
    annoy_user ("Done.");
  else
    annoy_user_with_log ("Failed, see log.");
}

static void
install_package_cont3 (bool res, void *data)
{
  package_info *pi = (package_info *)data;
  if (res)
    {
      show_progress ("Installing");
      apt_worker_install_package (pi->name, install_package_reply, NULL);
    }
  pi->unref ();
}

static void
format_string_list (GString *str, const char *title,
		    GList *list)
{
  if (list)
    g_string_append (str, title);

  while (list)
    {
      //g_string_append_printf (str, "- %s\n", (char *)list->data);
      list = list->next;
    }
}

static void
install_check_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  package_info *pi = (package_info *)data;
  GList *notauth = NULL, *notcert = NULL;

  while (true)
    {
      apt_proto_preptype prep = apt_proto_preptype (dec->decode_int ());
      if (dec->corrupted () || prep == preptype_end)
	break;

      const char *string = dec->decode_string_in_place ();
      if (prep == preptype_notauth)
	notauth = g_list_append (notauth, (void*)string);
      else if (prep == preptype_notcert)
	notcert = g_list_append (notcert, (void*)string);
    }

  int success = dec->decode_int ();

  if (!dec->corrupted () && success)
    {
      if (notauth || notcert)
	{
	  GString *text = g_string_new ("");
	  format_string_list (text, 
			      "This software is neither verified nor delivered to you by Nokia. Please note that some software may harm your device and installation will be at your own risk.\n",
			      notauth);
	  format_string_list (text, 
			      "The following packages are not\n"
			      "certified by Nokia:\n",
			      notcert);
	  g_string_append (text, "Continue anyway?");
	  hide_progress ();
	  ask_yes_no (text->str, install_package_cont3, pi);
	  g_string_free (text, 1);
	}
      else
	install_package_cont3 (true, pi);
    }
  else
    {
      hide_progress ();
      annoy_user_with_log ("Failed, see log.");
      pi->unref ();
    }

  g_list_free (notauth);
  g_list_free (notcert);
}

static void
install_package_cont2 (bool res, void *data)
{
  package_info *pi = (package_info *)data;

  if (res)
    {
      if (pi->info.installable)
	{
	  add_log ("-----\n");
	  if (pi->installed_version)
	    add_log ("Upgrading %s %s to %s\n", pi->name,
		     pi->installed_version, pi->available_version);
	  else
	    add_log ("Installing %s %s\n", pi->name, pi->available_version);
	  
	  show_progress ("Installing");
	  apt_worker_install_check (pi->name, install_check_reply, pi);
	}
      else
	{
	  annoy_user_with_details ("Impossible, see details.", pi, false);
	  pi->unref ();
	}
    }
}

static void
install_package_cont (package_info *pi, void *data, bool changed)
{
  confirm_install (pi, install_package_cont2, pi);
}

static void
install_package (package_info *pi)
{
  pi->ref ();
  get_intermediate_package_info (pi, install_package_cont, NULL);
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
      set_operation_callback ((pi->installed_version
			       ? "Update"
			       : "Install"),
			      (void (*)(void*))install_package, pi);
      get_intermediate_package_info (pi, NULL, NULL);
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
      get_intermediate_package_info (pi, NULL, NULL);
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

  if (v->label)
    g_free ((gchar *)v->label);
  v->label = g_strdup (cur_section->name);

  view = get_global_package_list_widget ();
  set_global_lists_for_view (v);
  gtk_widget_show_all (view);

  maybe_refresh_package_cache ();

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
  view = get_global_section_list_widget ();
  set_global_lists_for_view (v);
  gtk_widget_show_all (view);

  return view;
}

GtkWidget *
make_upgrade_applications_view (view *v)
{
  GtkWidget *view;
  GList *packages;

  view = get_global_package_list_widget ();
  set_global_lists_for_view (v);
  gtk_widget_show_all (view);

  return view;
}

static bool
confirm_uninstall (package_info *pi,
		   void (*cont) (bool res, void *data), void *data)
{
  GString *text = g_string_new ("");
  char size_buf[20];
  
  size_string_general (size_buf, 20, pi->installed_size);
  g_string_printf (text,
		   "Do you want to uninstall?\n"
		   "%s %s\n"
		   "%s",
		   pi->name, pi->installed_version, size_buf);

  ask_yes_no (text->str, cont, data);
  g_string_free (text, 1);
}

static void
uninstall_package_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  hide_progress ();

  int success = dec->decode_int ();
  get_package_list ();

  if (!dec->corrupted() && success)
    annoy_user ("Done.");
  else
    annoy_user ("Failed, see log.");
}

static void
uninstall_package_cont2 (bool res, void *data)
{
  package_info *pi = (package_info *)data;

  if (res)
    {
      if (pi->info.removable)
	{
	  add_log ("-----\n");
	  add_log ("Uninstalling %s %s", pi->name, pi->installed_version);

	  show_progress ("Uninstalling");
	  apt_worker_remove_package (pi->name, uninstall_package_reply, NULL);
	}
      else
	annoy_user_with_details ("Impossible, see details.", pi, true);
    }

  pi->unref ();
}

static void
uninstall_package_cont (package_info *pi, void *data, bool changed)
{
  confirm_uninstall (pi, uninstall_package_cont2, pi);
}

static void
uninstall_package (package_info *pi)
{
  pi->ref ();
  get_intermediate_package_info (pi, uninstall_package_cont, NULL);
}

GtkWidget *
make_uninstall_applications_view (view *v)
{
  GtkWidget *view;
  GList *packages;

  view = get_global_package_list_widget ();
  set_global_lists_for_view (v);
  gtk_widget_show_all (view);

  return view;
}

GtkWidget *
make_search_results_view (view *v)
{
  GtkWidget *view;

  view = get_global_package_list_widget ();
  set_global_lists_for_view (v);
  gtk_widget_show_all (view);

  return view;
}

static void
search_package_list (GList **result,
		     GList *packages, const char *pattern)
{
  while (packages)
    {
      package_info *pi = (package_info *)packages->data;
      
      // XXX
      if (strcasestr (pi->name, pattern))
	{
	  pi->ref ();
	  *result = g_list_append (*result, pi);
	}

      packages = packages->next;
    }
}

static void
search_section_list (GList **result,
		     GList *sections, const char *pattern)
{
  while (sections)
    {
      section_info *si = (section_info *)sections->data;
      search_package_list (result, si->packages, pattern);

      sections = sections->next;
    }
}

static void
find_in_package_list (GList **result,
		      GList *packages, const char *name)
{
  while (packages)
    {
      package_info *pi = (package_info *)packages->data;
      
      if (!strcmp (pi->name, name))
	{
	  pi->ref ();
	  *result = g_list_append (*result, pi);
	}

      packages = packages->next;
    }
}

static void
find_in_section_list (GList **result,
		      GList *sections, const char *name)
{
  while (sections)
    {
      section_info *si = (section_info *)sections->data;
      find_in_package_list (result, si->packages, name);

      sections = sections->next;
    }
}

static void
search_packages_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  view *parent = (view *)data;

  while (!dec->at_end ())
    {
      const char *name = dec->decode_string_in_place ();
      dec->decode_string_in_place (); // installed_version
      dec->decode_int ();             // installed_size
      dec->decode_string_in_place (); // available_version
      dec->decode_string_in_place (); // available_section

      if (parent == &install_applications_view)
	find_in_section_list (&search_result_packages,
			      install_sections, name);
      else if (parent == &upgrade_applications_view)
	find_in_package_list (&search_result_packages,
			      upgradeable_packages, name);
      else if (parent == &uninstall_applications_view)
	find_in_package_list (&search_result_packages,
			      installed_packages, name);
    }

  show_view (&search_results_view);
}

void
search_packages (const char *pattern, bool in_descriptions)
{
  free_packages (search_result_packages);
  search_result_packages = NULL;

  view *parent;

  if (cur_view_struct == &search_results_view)
    parent = search_results_view.parent;
  else if (cur_view_struct == &install_section_view)
    parent = &install_applications_view;
  else if (cur_view_struct == &main_view)
    parent = &uninstall_applications_view;
  else
    parent = cur_view_struct;

  search_results_view.parent = parent;

  printf ("Searching %s in %s\n", pattern, parent->label);

  if (!in_descriptions)
    {
      if (parent == &install_applications_view)
	search_section_list (&search_result_packages,
			     install_sections, pattern);
      else if (parent == &upgrade_applications_view)
	search_package_list (&search_result_packages,
			     upgradeable_packages, pattern);
      else if (parent == &uninstall_applications_view)
	search_package_list (&search_result_packages,
			     installed_packages, pattern);

      show_view (&search_results_view);
    }
  else
    {
      bool only_installed = (parent == &uninstall_applications_view
			     || parent == &upgrade_applications_view);
      bool only_available = (parent == &install_applications_view
			     || parent == &upgrade_applications_view);
      apt_worker_get_package_list (!red_pill_mode,
				   only_installed,
				   only_available, 
				   pattern,
				   search_packages_reply, parent);
    }
}

static void
set_global_lists_for_view (view *v)
{
  GList *packages_for_info = NULL;

  if (v == &install_applications_view)
    {
      set_global_section_list (install_sections, view_section);
    }
  else if (v == &install_section_view)
    {
      cur_section = find_section_info (&install_sections,
				       install_section_view.label);
      set_global_package_list (cur_section->packages,
			       false,
			       available_package_selected, 
			       install_package);
      packages_for_info = cur_section->packages;
    }
  else if (v == &upgrade_applications_view)
    {
      set_global_package_list (upgradeable_packages,
			       false,
			       available_package_selected,
			       install_package);
      packages_for_info = upgradeable_packages;
    }
  else if (v == &uninstall_applications_view)
    {
      set_global_package_list (installed_packages,
			       true,
			       installed_package_selected,
			       uninstall_package);
      //packages_for_info = installed_packages;
    }
  else if (v == &search_results_view)
    {
      if (v->parent == &install_applications_view
	  || v->parent == &upgrade_applications_view)
	{
	  set_global_package_list (search_result_packages,
				   false,
				   available_package_selected,
				   install_package);
	  packages_for_info = search_result_packages;
	}
      else
	{
	  set_global_package_list (search_result_packages,
				   true,
				   installed_package_selected,
				   uninstall_package);
	}
    }

  get_package_list_info (packages_for_info);
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
install_from_file_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  hide_progress ();
  int success = dec->decode_int ();
  if (success)
    annoy_user ("Done.");
  else
    annoy_user_with_log ("Failed, see log.");
}

void
install_from_file_cont2 (bool res, void *data)
{
  char *filename = (char *)data;
  if (res)
    {
      show_progress ("Installing");
      apt_worker_install_file (filename,
			       install_from_file_reply, NULL);
    }
  g_free (filename);
}

static bool
is_maemo_section (const char *section)
{
  return section && !strncmp (section, "maemo/", 6);
}

static void
file_details_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  char *filename = (char *)data;

  const char *package = dec->decode_string_in_place ();
  const char *version = dec->decode_string_in_place ();
  const char *maintainer = dec->decode_string_in_place ();
  const char *section = dec->decode_string_in_place ();
  const char *installed_size = dec->decode_string_in_place ();
  const char *description = dec->decode_string_in_place ();

  if (dec->corrupted ())
    {
      annoy_user_with_log ("Failed, see log.");
      g_free (filename);
      return;
    }

  if (!red_pill_mode && !is_maemo_section (section))
    {
      annoy_user ("Package not compatible");
      g_free (filename);
      return;
    }

  GString *text = g_string_new ("");

  g_string_printf (text, "Do you want to install\n%s %s",
		   package, version);

  ask_yes_no (text->str, install_from_file_cont2, filename);
  g_string_free (text, 1);
}

static void
install_from_file_cont (char *filename, void *unused)
{
  apt_worker_get_file_details (filename,
			       file_details_reply, filename);
}

void
install_from_file ()
{
  show_deb_file_chooser (install_from_file_cont, NULL);
}

static void
window_destroy (GtkWidget* widget, gpointer data)
{
  gtk_main_quit ();
}

static void
apt_status_callback (int cmd, apt_proto_decoder *dec, void *unused)
{
  const char *label = dec->decode_string_in_place ();
  int percent = dec->decode_int ();

  //printf ("STATUS: %3d %s\n", percent, label);
  set_progress (label, percent/100.0);
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
  apt_source_id = g_io_add_watch (channel,
				  GIOCondition (G_IO_IN | G_IO_HUP | G_IO_ERR),
				  handle_apt_worker, NULL);
  g_io_channel_unref (channel);
}

static void
mime_open_handler (gpointer raw_data, int argc, char **argv)
{
  for (int i = 0; i < argc; i++)
    printf ("mime-open: %s\n", argv[i]);

  if (argc > 0)
    install_from_file_cont (g_strdup (argv[0]), NULL);
}

int
main (int argc, char **argv)
{
  GtkWidget *app_view, *app;
  GtkWidget *toolbar, *image;
  GtkMenu *main_menu;
  char *apt_worker_prog = "/usr/libexec/apt-worker";

  gtk_init (&argc, &argv);

  if (argc > 1)
    apt_worker_prog = argv[1];


  app_view = hildon_appview_new (NULL);
  app = hildon_app_new_with_appview (HILDON_APPVIEW (app_view));
  hildon_app_set_title (HILDON_APP (app), "Application installer");

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

  load_settings ();

  gtk_widget_show_all (app);
  show_view (&main_view);

  /* XXX - check errors.
   */
  osso_context_t *ctxt;
  ctxt = osso_initialize ("osso_application_installer",
			  PACKAGE_VERSION, TRUE, NULL);
  osso_mime_set_cb (ctxt, mime_open_handler, NULL);

  if (start_apt_worker (apt_worker_prog))
    {
      apt_worker_set_status_callback (apt_status_callback, NULL);
      add_apt_worker_handler ();

      maybe_refresh_package_cache ();
      get_package_list ();
    }
  else
    annoy_user_with_log ("Unexpected startup problem, see log.");

  gtk_main ();
}
