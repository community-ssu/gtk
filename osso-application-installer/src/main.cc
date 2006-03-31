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
#include <libintl.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "apt-worker-client.h"
#include "apt-worker-proto.h"

#include "main.h"
#include "util.h"
#include "details.h"
#include "menu.h"
#include "log.h"
#include "settings.h"
#include "search.h"

#define _(x) gettext (x)

extern "C" {
  #include "hildonbreadcrumbtrail.h"
  #include <hildon-widgets/hildon-window.h>
  #include <libosso.h>
  #include <osso-helplib.h>
}

using namespace std;

static guint apt_source_id;

static void set_details_callback (void (*func) (gpointer), gpointer data);
static void set_operation_label (const char *label);
static void set_operation_callback (void (*func) (gpointer), gpointer data);
static void enable_search (bool f);
static void set_current_help_topic (const char *topic);

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
  set_operation_label (NULL);
  set_operation_callback (NULL, NULL);

  cur_view = v->maker (v);
  cur_view_struct = v;

  if (v->path == NULL)
    v->path = make_view_path (v);
  hildon_bread_crumb_trail_set_path (main_trail, v->path);
  
  gtk_box_pack_start (GTK_BOX (main_vbox), cur_view, TRUE, TRUE, 10);
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
  return gettext (((view *)node->data)->label);
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
  "ai_ti_main",
  make_main_view,
  NULL
};

view install_applications_view = {
  &main_view,
  "ai_li_install",
  make_install_applications_view,
  NULL
};

view upgrade_applications_view = {
  &main_view,
  "ai_li_update",
  make_upgrade_applications_view,
  NULL
};

view uninstall_applications_view = {
  &main_view,
  "ai_li_uninstall",
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
  "ai_ti_search_results",
  make_search_results_view,
  NULL
};

void
show_main_view ()
{
  show_view (&main_view);
}

void
show_parent_view ()
{
  if (cur_view_struct && cur_view_struct->parent)
    show_view (cur_view_struct->parent);
}

static GtkWidget *
make_padded_button (const char *label)
{
  GtkWidget *l = gtk_label_new (label);
  gtk_misc_set_padding (GTK_MISC (l), 15, 15);
  GtkWidget *btn = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (btn), l);
  return btn;
}

GtkWidget *
make_main_view (view *v)
{
  GtkWidget *view;
  GtkWidget *table, *vbox;
  GtkWidget *btn, *label, *image;
  
  view = gtk_hbox_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 0);
  table = gtk_table_new (5, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (view), vbox, FALSE, FALSE, 0);

  gtk_table_set_row_spacings (GTK_TABLE (table), 10);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  image = gtk_image_new_from_icon_name ("qgn_list_filesys_divc_cls",
					HILDON_ICON_SIZE_26);
  gtk_table_attach_defaults (GTK_TABLE (table), image,
			     0, 1, 0, 1);

  label = gtk_label_new ("My Device Nokia 770");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     1, 2, 0, 1);
  
  btn = make_padded_button (_("ai_li_uninstall"));
  g_signal_connect (G_OBJECT (btn), "clicked",
		    G_CALLBACK (show_view_callback),
		    &uninstall_applications_view);
  gtk_table_attach_defaults (GTK_TABLE (table), btn,
			     1, 2, 1, 2);

  image = gtk_image_new_from_icon_name ("qgn_list_browser",
					HILDON_ICON_SIZE_26);
  gtk_table_attach_defaults (GTK_TABLE (table), image,
			     0, 1, 2, 3);
  label = gtk_label_new (_("ai_li_repository"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     1, 2, 2, 3);

  btn = make_padded_button (_("ai_li_install"));
  g_signal_connect (G_OBJECT (btn), "clicked",
		    G_CALLBACK (show_view_callback),
		    &install_applications_view);
  gtk_table_attach_defaults (GTK_TABLE (table), btn,
			     1, 2, 3, 4);
  
  btn = make_padded_button (_("ai_li_update"));
  g_signal_connect (G_OBJECT (btn), "clicked",
		    G_CALLBACK (show_view_callback),
		    &upgrade_applications_view);
  gtk_table_attach_defaults (GTK_TABLE (table), btn,
			     1, 2, 4, 5);

  // XXX - find the proper way to load this image.
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (view), vbox, FALSE, FALSE, 0);
  image = gtk_image_new_from_file ("/usr/share/themes/default/images/qgn_plat_application_installer_image.png");
  gtk_box_pack_end (GTK_BOX (vbox), image, FALSE, FALSE, 0);
  
  gtk_widget_show_all (view);

  get_package_list_info (NULL);

  enable_search (false);
  set_current_help_topic (AI_TOPIC ("mainview"));

  return view;
}

static GList *install_sections = NULL;
static GList *upgradeable_packages = NULL;
static GList *installed_packages = NULL;
static GList *search_result_packages = NULL;

static char *cur_section_name;

package_info::package_info ()
{
  ref_count = 1;
  name = NULL;
  installed_version = NULL;
  installed_section = NULL;
  available_version = NULL;
  available_section = NULL;
  installed_short_description = NULL;
  available_short_description = NULL;
  installed_icon = NULL;
  available_icon = NULL;

  have_info = false;

  have_details = false;
  maintainer = NULL;
  description = NULL;
  summary = NULL;

  model = NULL;
  
  filename = NULL;
}

package_info::~package_info ()
{
  g_free (name);
  g_free (installed_version);
  g_free (installed_section);
  g_free (available_version);
  g_free (available_section);
  g_free (installed_short_description);
  g_free (available_short_description);
  if (installed_icon)
    g_object_unref (installed_icon);
  if (available_icon)
    g_object_unref (available_icon);
  g_free (maintainer);
  g_free (description);
  g_free (summary);
  g_free (filename);
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

section_info::section_info ()
{
  ref_count = 1;
  name = NULL;
  packages = NULL;
}

section_info::~section_info ()
{
  g_free (name);
  free_packages (packages);
}

void
section_info::ref ()
{
  ref_count += 1;
}

void
section_info::unref ()
{
  ref_count -= 1;
  if (ref_count == 0)
    delete this;
}

static void
free_sections (GList *list)
{
  for (GList *s = list; s; s = s->next)
    ((section_info *)s->data)->unref ();
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

const char *
nicify_section_name (const char *name)
{
  if (name == NULL || red_pill_mode)
    return name;

  // XXX
  if (!strncmp (name, "maemo/", 6))
    return name + 6;

  if (!strncmp (name, "user/", 5))
    {
      const char *subsection = name + 5;
      char *logical_id = g_strdup_printf ("ai_category_%s", subsection);
      const char *translated_name = gettext (logical_id);
      if (translated_name != logical_id)
	subsection = translated_name;
      g_free (logical_id);
      return subsection;
    }
  else
    return name;
}

static section_info *
find_section_info (GList **list_ptr, const char *name, bool create)
{
  if (name == NULL)
    name = "other";

  for (GList *ptr = *list_ptr; ptr; ptr = ptr->next)
    if (!strcmp (((section_info *)ptr->data)->name, name))
      return (section_info *)ptr->data;

  if (!create)
    return NULL;
	    
  section_info *si = new section_info;
  si->name = g_strdup (name);
  si->packages = NULL;
  *list_ptr = g_list_prepend (*list_ptr, si);
  return si;
}

static gint
compare_section_names (gconstpointer a, gconstpointer b)
{
  section_info *si_a = (section_info *)a;
  section_info *si_b = (section_info *)b;

  // The sorting of sections can not be configured.
  
  return g_ascii_strcasecmp (si_a->name, si_b->name);
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

  if (search_results_view.parent == &install_applications_view
      || search_results_view.parent == &upgrade_applications_view)
    search_result_packages = g_list_sort (search_result_packages,
					  compare_packages_avail);
  else
    search_result_packages = g_list_sort (search_result_packages,
					  compare_packages_inst);

  show_view (cur_view_struct);
}

static void
get_package_list_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  int count = 0, new_p = 0, upg_p = 0, inst_p = 0;

  clear_global_package_list ();
  clear_global_section_list ();
  free_all_packages ();
  
  if (dec == NULL)
    ;
  else if (dec->decode_int () == 0)
    annoy_user_with_log ("Operation failed");
  else
    {
      while (!dec->at_end ())
	{
	  const char *installed_icon, *available_icon;
	  package_info *info = new package_info;

	  info->name = dec->decode_string_dup ();
	  info->installed_version = dec->decode_string_dup ();
	  info->installed_size = dec->decode_int ();
	  info->installed_section = dec->decode_string_dup ();
	  info->installed_short_description = dec->decode_string_dup ();
	  installed_icon = dec->decode_string_in_place ();
	  info->available_version = dec->decode_string_dup ();
	  info->available_section = dec->decode_string_dup ();
	  info->available_short_description = dec->decode_string_dup ();
	  available_icon = dec->decode_string_in_place ();

	  info->installed_icon = pixbuf_from_base64 (installed_icon);
	  if (available_icon)
	    info->available_icon = pixbuf_from_base64 (available_icon);
	  else
	    {
	      info->available_icon = info->installed_icon;
	      if (info->available_icon)
		g_object_ref (info->available_icon);
	    }

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
						     info->available_section,
						     true);
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

  if (dec)
    {
      dec->decode_mem (&(pi->info), sizeof (pi->info));
      if (!dec->corrupted ())
	{
	  pi->have_info = true;
	  func (pi, data, true);
	}
    }

  pi->unref ();
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
  /* The updating might have been 'cancelled' or it might have failed,
     and we want to distinguish between those two situations in the
     message shown to the user.

     It would be good to consider all explicit user actions as
     cancelling, but we don't get enough information for that.  Thus,
     the updating is considered cancelled only when the user hits our
     own cancel button, and not for example the cancel button in the
     "Select connection" dialog, or when the user explicitely
     disconnects the network.
  */

  hide_progress ();

  if (dec == NULL)
    {
      /* Network connection failed or apt-worker crashed.  An error
	 message has already been displayed.
      */
      return;
    }
     
  if (progress_was_cancelled ())
    {
      /* The user hit cancel.  We don't care whether the operation was
	 successful or not.
      */
      annoy_user (_("ai_ni_update_list_cancelled"));
      return;
    }

  int success = dec->decode_int ();

  if (!success)
    {
      annoy_user_with_log (_("ai_ni_update_list_not_successful"));
      return;
    }

  get_package_list ();
}

static bool refreshed_this_session = false;

static void
refresh_package_cache_cont (bool res, void *unused)
{
  refreshed_this_session = true;
  last_update = time (NULL);
  save_settings ();

  if (res)
    {
      show_progress (_("ai_nw_updating_list"));
      apt_worker_update_cache (refresh_package_cache_reply, NULL);
    }
}

void
refresh_package_cache ()
{
  ask_yes_no (_("ai_nc_confirm_update"), refresh_package_cache_cont, NULL);
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
  char download_buf[20];
  
  if (pi->info.installable)
    {
      size_string_general (download_buf, 20, pi->info.download_size);
      g_string_append_printf (text, "\n%s", download_buf);
    }
  else
    strcpy (download_buf, "-");

  g_string_printf (text,
		   (pi->installed_version
		    ? _("ai_nc_update")
		    : _("ai_nc_install")),
		   pi->name, pi->available_version, download_buf);
  
  ask_yes_no_with_details ((pi->installed_version
			    ? _("ai_ti_update")
			    : _("ai_ti_install")),
			   text->str, pi, false, cont, data);
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
  package_info *pi = (package_info *)data;

  hide_progress ();

  if (dec == NULL)
    {
      pi->unref ();
      return;
    }

  int success = dec->decode_int ();

  get_package_list ();

  if (clean_after_install)
    apt_worker_clean (clean_reply, NULL);

  if (success)
    {
      char *str = g_strdup_printf (_("ai_ni_install_successful"),
				   pi->name);
      annoy_user (str);
      g_free (str);
    }
  else
    {
      if (progress_was_cancelled ())
	annoy_user (_("ai_ni_install_cancelled"));
      else
	{
	  char *str = g_strdup_printf (_("ai_ni_error_installation_failed"),
				       pi->name);
	  annoy_user_with_log (str);
	  g_free (str);
	}
    }

  pi->unref ();
}

struct ip_closure {
  package_info *pi;
  GSList *upgrade_names;
  GSList *upgrade_versions;
  char *cur_name;
};

static void install_package_cont3 (bool res, void *data);

static void
install_package_cont4 (int status, void *data)
{
  ip_closure *c = (ip_closure *)data;

  if (status != -1 && WIFEXITED (status) && WEXITSTATUS (status) == 111)
    {
      char *str =
	g_strdup_printf (_("ai_ni_error_uninstall_applicationrunning"),
			 c->cur_name);
      annoy_user (str);
      g_free (str);

      install_package_cont3 (false, data);
    }
  else
    install_package_cont3 (true, data);
}

static void
install_package_cont3 (bool res, void *data)
{
  ip_closure *c = (ip_closure *)data;

  if (c->upgrade_names == NULL)
    {
      if (res)
	{
	  show_progress (_("ai_nw_installing"));
	  apt_worker_install_package (c->pi->name,
				      install_package_reply, c->pi);
	}
      else
	c->pi->unref ();

      g_free (c->cur_name);
      delete c;
    }
  else
    {
      char *name = (char *)pop (c->upgrade_names);
      char *version = (char *)pop (c->upgrade_versions);

      if (res)
	{
	  char *cmd =
	    g_strdup_printf ("/var/lib/osso-application-installer/info/%s.checkrm",
			     name);
      
	  add_log ("Checking %s\n", cmd);
	  char *argv[] = { cmd, "upgrade", version, NULL };
	  run_cmd (argv, install_package_cont4, c);
	  g_free (cmd);
	}
      else
	install_package_cont3 (false, data);

      g_free (c->cur_name);
      c->cur_name = name;
      g_free (version);
    }
}

static void
install_package_cont5 (bool res, void *data)
{
  if (!res)
    annoy_user (_("ai_ni_install_cancelled"));
  else
    install_package_cont3 (true, data);
}


static void
install_check_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  GList *notauth = NULL, *notcert = NULL;

  ip_closure *c = new ip_closure;
  c->pi = (package_info *)data;
  c->upgrade_names = c->upgrade_versions = NULL;
  c->cur_name = NULL;

  if (dec == NULL)
    {
      hide_progress ();
      c->pi->unref ();
      delete c;
      return;
    }

  while (!dec->corrupted ())
    {
      apt_proto_preptype prep = apt_proto_preptype (dec->decode_int ());
      if (prep == preptype_end)
	break;

      const char *string = dec->decode_string_in_place ();
      if (prep == preptype_notauth)
	notauth = g_list_append (notauth, (void*)string);
      else if (prep == preptype_notcert)
	notcert = g_list_append (notcert, (void*)string);
    }

  while (!dec->corrupted ())
    {
      char *name = dec->decode_string_dup ();
      if (name == NULL)
	break;

      char *version = dec->decode_string_dup ();
      fprintf (stderr, "upgrade %s %s\n", name, version);

      push (c->upgrade_names, name);
      push (c->upgrade_versions, version);
    }

  int success = dec->decode_int ();

  if (success)
    {
      // A non-authenticated package can never be certified.
      // Apt-worker does not implement that rule, tho, so we do it
      // here.

      if (notcert || notauth)
	{
	  hide_progress ();
	  scare_user_with_legalese (true, install_package_cont5, c);
	}
      else
	install_package_cont3 (true, c);
    }
  else
    {
      hide_progress ();
      char *str = g_strdup_printf (_("ai_ni_error_installation_failed"),
				   c->pi->name);
      annoy_user_with_log ("ai_ni_error_installation_failed");
      g_free (str);
      install_package_cont3 (false, c);
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
	  
	  show_progress (_("ai_nw_installing"));
	  apt_worker_install_check (pi->name, install_check_reply, pi);
	}
      else
	{
	  char *str = g_strdup_printf (_("ai_ni_error_installation_failed"),
				       pi->name);
	  annoy_user_with_details (str, pi, false);
	  g_free (str);
	  pi->unref ();
	}
    }
  else
    annoy_user (_("ai_ni_install_cancelled"));
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
      set_operation_callback ((void (*)(void*))install_package, pi);
      get_intermediate_package_info (pi, NULL, NULL);
    }
  else
    {
      set_details_callback (NULL, NULL);
      set_operation_callback (NULL, NULL);
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
      set_operation_callback ((void (*)(void*))uninstall_package, pi);
      get_intermediate_package_info (pi, NULL, NULL);
    }
  else
    {
      set_details_callback (NULL, NULL);
      set_operation_callback (NULL, NULL);
    }
}

static GtkWidget *
make_last_update_label ()
{
  char text[1024];
  time_t t = last_update;
  struct tm *broken = localtime (&t);
  strftime (text, 1024, _("ai_li_updated_%x"), broken);
  return gtk_label_new (text);
}

static GtkWidget *
make_package_list_view (GtkWidget *list_widget,
			bool with_updated_label)
{
  GtkWidget *view;

  if (with_updated_label)
    {
      GtkWidget *label = make_last_update_label ();
      view = gtk_vbox_new (FALSE, 10);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_box_pack_start (GTK_BOX (view), label, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (view), list_widget, TRUE, TRUE, 0);
    }
  else
    view = list_widget;
  
  return view;
}

static void
set_install_section_name (const char *name)
{
  g_free ((gchar *)install_section_view.label);
  install_section_view.label = g_strdup (nicify_section_name (name));
}

GtkWidget *
make_install_section_view (view *v)
{
  GtkWidget *view;
  GList *packages;

  set_operation_label (_("ai_me_package_install"));

  section_info *si = find_section_info (&install_sections,
					cur_section_name, false);

  GtkWidget *list =
    make_global_package_list (si? si->packages : NULL,
			      false,
			      _("ai_li_no_applications_available"),
			      available_package_selected, 
			      install_package);

  view = make_package_list_view (list, true);
  gtk_widget_show_all (view);

  if (si)
    get_package_list_info (si->packages);
  maybe_refresh_package_cache ();

  enable_search (true);
  set_current_help_topic (AI_TOPIC ("sectionsview"));

  return view;
}

static void
view_section (section_info *si)
{
  g_free (cur_section_name);
  cur_section_name = g_strdup (si->name);

  install_section_view.label = nicify_section_name (cur_section_name);

  show_view (&install_section_view);
}

GtkWidget *
make_install_applications_view (view *v)
{
  GtkWidget *list, *view, *label;

  set_operation_label (_("ai_me_package_install"));
  
  // XXX - provide "All" section, don't show sections when there are
  //       only few packages.

  list = make_global_section_list (install_sections, view_section);
  label = make_last_update_label ();
  view = gtk_vbox_new (FALSE, 10);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (view), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (view), list, TRUE, TRUE, 0);

  gtk_widget_show_all (view);

  enable_search (true);
  set_current_help_topic (AI_TOPIC ("packagesview"));

  return view;
}

GtkWidget *
make_upgrade_applications_view (view *v)
{
  GtkWidget *view;

  set_operation_label (_("ai_me_package_update"));

  GtkWidget *list =
    make_global_package_list (upgradeable_packages,
			      false,
			      _("ai_li_no_updates_available"),
			      available_package_selected,
			      install_package);

  view = make_package_list_view (list, true);
  gtk_widget_show_all (view);

  get_package_list_info (upgradeable_packages);

  enable_search (true);
  set_current_help_topic (AI_TOPIC ("updateview"));

  return view;
}

static bool
confirm_uninstall (package_info *pi,
		   void (*cont) (bool res, void *data), void *data)
{
  GString *text = g_string_new ("");
  char size_buf[20];
  
  size_string_general (size_buf, 20, pi->installed_size);
  g_string_printf (text, _("ai_nc_uninstall"),
		   pi->name, pi->installed_version, size_buf);

  ask_yes_no_with_details (_("ai_ti_confirm_uninstall"), text->str,
			   pi, true, cont, data);
  g_string_free (text, 1);
}

static void
uninstall_package_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  package_info *pi = (package_info *)data;

  hide_progress ();

  if (dec == NULL)
    {
      pi->unref ();
      return;
    }

  int success = dec->decode_int ();
  get_package_list ();

  if (success)
    {
      char *str = g_strdup_printf (_("ai_ni_uninstall_successful"),
				   pi->name);
      annoy_user (str);
      g_free (str);
    }
  else
    {
      char *str = g_strdup_printf (_("ai_ni_error_uninstallation_failed"),
				   pi->name);
      annoy_user_with_log (str);
      g_free (str);
    }

  pi->unref ();
}

static void
uninstall_package_doit (package_info *pi)
{
  if (pi->info.removable)
    {
      add_log ("-----\n");
      add_log ("Uninstalling %s %s", pi->name, pi->installed_version);
      
      show_progress (_("ai_nw_uninstalling"));
      apt_worker_remove_package (pi->name, uninstall_package_reply, pi);
    }
  else
    {
      char *str = g_strdup_printf (_("ai_ni_error_uninstallation_failed"),
				   pi->name);
      annoy_user_with_details (str, pi, true);
      g_free (str);
      pi->unref ();
    }
}

struct uip_closure {
  package_info *pi;
  GSList *to_remove;
  char *cur_name;
};

static void
check_uninstall_scripts2 (int status, void *data)
{
  uip_closure *c = (uip_closure *)data;

  if (status != -1 && WIFEXITED (status) && WEXITSTATUS (status) == 111)
    {
      char *str =
	g_strdup_printf (_("ai_ni_error_uninstall_applicationrunning"),
			 c->cur_name);
      annoy_user (str);
      g_free (str);

      c->pi->unref ();
      g_free (c->cur_name);
      // XXX delete c->to_remove list
      delete c;
    }
  else if (c->to_remove)
    {
      GSList *next = c->to_remove->next;
      char *name = (char *)c->to_remove->data;
      g_slist_free_1 (c->to_remove);
      c->to_remove = next;

      g_free (c->cur_name);
      c->cur_name = name;

      char *cmd =
	g_strdup_printf ("/var/lib/osso-application-installer/info/%s.checkrm",
			 name);
      
      add_log ("Checking %s\n", cmd);
      char *argv[] = { cmd, "remove", NULL };
      run_cmd (argv, check_uninstall_scripts2, c);
      g_free (cmd);
    }
  else
    {
      uninstall_package_doit (c->pi);
      g_free (c->cur_name);
      delete c;
    }
}

static void
get_packages_to_remove_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  package_info *pi = (package_info *)data;
  GSList *names = NULL;
  
  if (dec == NULL)
    {
      pi->unref ();
      return;
    }

  while (true)
    {
      char *name = dec->decode_string_dup ();
      if (name == NULL)
	break;
      names = g_slist_prepend (names, name);
    }

  uip_closure *c = new uip_closure;
  c->pi = pi;
  c->to_remove = names;
  c->cur_name = NULL;
  check_uninstall_scripts2 (0, c);
}

static void
check_uninstall_scripts (package_info *pi)
{
  apt_worker_get_packages_to_remove (pi->name,
				     get_packages_to_remove_reply, pi);
}

static void
uninstall_package_cont2 (bool res, void *data)
{
  package_info *pi = (package_info *)data;

  if (res)
    check_uninstall_scripts (pi);
  else
    {
      annoy_user (_("ai_ni_uninstall_cancelled"));
      pi->unref ();
    }
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

  set_operation_label (_("ai_me_package_uninstall"));

  view = make_global_package_list (installed_packages,
				   true,
				   _("ai_li_no_installed_applications"),
				   installed_package_selected,
				   uninstall_package);
  gtk_widget_show_all (view);

  get_package_list_info (installed_packages);
  
  enable_search (true);
  set_current_help_topic (AI_TOPIC ("uninstallview"));

  return view;
}

GtkWidget *
make_search_results_view (view *v)
{
  GtkWidget *view;

  if (v->parent == &install_applications_view
      || v->parent == &upgrade_applications_view)
    {
      view = make_global_package_list (search_result_packages,
				       false,
				       _("ai_li_no_packages_found"),
				       available_package_selected,
				       install_package);
    }
  else
    {
      view = make_global_package_list (search_result_packages,
				       true,
				       _("ai_ib_no_matches"),
				       installed_package_selected,
				       uninstall_package);
    }
  gtk_widget_show_all (view);

  get_package_list_info (search_result_packages);

  enable_search (true);
  set_current_help_topic (AI_TOPIC ("searchresultsview"));

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

  if (dec == NULL)
    return;

  while (!dec->at_end ())
    {
      const char *name = dec->decode_string_in_place ();
      dec->decode_string_in_place (); // installed_version
      dec->decode_int ();             // installed_size
      dec->decode_string_in_place (); // installed_section
      dec->decode_string_in_place (); // installed_short_description
      dec->decode_string_in_place (); // installed_icon
      dec->decode_string_in_place (); // available_version
      dec->decode_string_in_place (); // available_section
      dec->decode_string_in_place (); // available_short_description
      dec->decode_string_in_place (); // available_icon

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
static const char *operation_label;

void
do_current_operation ()
{
  if (operation_func)
    operation_func (operation_data);
}

static void set_operation_toolbar_label (const char *label, bool enable);

static void
set_operation_label (const char *label)
{
  if (label == NULL)
    label = _("ai_me_package_install");

  operation_label = label;
  set_operation_menu_label (label, operation_func != NULL);
  set_operation_toolbar_label (label, operation_func != NULL);
}

static void
set_operation_callback (void (*func) (gpointer), gpointer data)
{
  operation_data = data;
  operation_func = func;
  set_operation_label (operation_label);
}

static void
install_from_file_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  package_info *pi = (package_info *)data;

  hide_progress ();

  if (dec == NULL)
    {
      pi->unref ();
      return;
    }

  int success = dec->decode_int ();

  get_package_list ();

  if (success)
    {
      char *str = g_strdup_printf (_("ai_ni_install_successful"),
				   pi->name);
      annoy_user (str);
      g_free (str);
    }
  else
    {
      char *str = g_strdup_printf (_("ai_ni_error_installation_failed"),
				   pi->name);
      annoy_user_with_log (str);
      g_free (str);
    }

  pi->unref ();
}

void
install_from_file_cont4 (bool res, void *data)
{
  package_info *pi = (package_info *)data;

  if (res)
    {
      show_progress (_("ai_nw_installing"));
      apt_worker_install_file (pi->filename,
			       install_from_file_reply, pi);
    }
  else
    {
      annoy_user (_("ai_ni_install_cancelled"));
      pi->unref ();
    }
}

void
install_from_file_cont3 (bool res, void *data)
{
  package_info *pi = (package_info *)data;

  if (res)
    scare_user_with_legalese (false, install_from_file_cont4, pi);
  else
    pi->unref ();
}

void
install_from_file_fail (bool res, void *data)
{
  package_info *pi = (package_info *)data;

  if (res)
    {
      char *str = g_strdup_printf (_("ai_ni_error_installation_failed"),
				   pi->name);
      annoy_user_with_details (str, pi, false);
      g_free (str);
    }

  pi->unref ();
}

static bool
is_user_section (const char *section)
{
  if (section == NULL)
    return false;

  // XXX
  if (!strncmp (section, "maemo/", 6))
    return true;

  return !strncmp (section, "user/", 5);
}

static char *
first_line_of (const char *text)
{
  const char *end = strchr (text, '\n');
  if (end == NULL)
    return g_strdup (text);
  else
    return g_strndup (text, end-text);
}

static void
file_details_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  char *filename = (char *)data;

  if (dec == NULL)
    return;

  package_info *pi = new package_info;

  pi->name = dec->decode_string_dup ();
  pi->filename = filename;
  pi->installed_version = dec->decode_string_dup ();
  pi->installed_size = dec->decode_int ();;
  pi->available_version = dec->decode_string_dup ();
  pi->maintainer = dec->decode_string_dup ();
  pi->available_section = dec->decode_string_dup ();
  pi->info.installable = dec->decode_int ();
  pi->info.install_user_size_delta = dec->decode_int ();
  pi->info.removable = false; // not used
  pi->info.remove_user_size_delta = 0;
  pi->info.download_size = 0;
  pi->description = dec->decode_string_dup ();
  pi->available_short_description = first_line_of (pi->description);
  pi->available_icon = pixbuf_from_base64 (dec->decode_string_in_place ());

  pi->have_info = true;
  pi->have_details = true;

  pi->summary = decode_summary (dec, pi, false);

  if (!red_pill_mode && !is_user_section (pi->available_section))
    {
      pi->info.installable = false;
      g_free (pi->summary);
      if (pi->installed_version)
	pi->summary = g_strdup (_("ai_ni_error_update_incompatible"));
      else
	pi->summary = g_strdup (_("ai_ni_error_install_incompatible"));
    }

  GString *text = g_string_new ("");

  char size_buf[20];
  size_string_general (size_buf, 20, pi->info.install_user_size_delta);
  if (pi->installed_version)
    g_string_printf (text, _("ai_nc_update"),
		     pi->name, pi->available_version, size_buf);
  else
    g_string_printf (text, _("ai_nc_install"),
		     pi->name, pi->available_version, size_buf);

  void (*cont) (bool res, void *);

  if (pi->info.installable)
    cont = install_from_file_cont3;
  else
    cont = install_from_file_fail;

  ask_yes_no_with_details ((pi->installed_version
			    ? _("ai_ti_confirm_update")
			    : _("ai_ti_confirm_install")),
			   text->str,
			   pi, false, cont, pi);
    
  g_string_free (text, 1);
}

static void
install_from_file_cont2 (char *filename, void *unused)
{
  if (filename)
    apt_worker_get_file_details (filename,
				 file_details_reply, filename);
}

static void
install_from_file_cont (char *uri, void *unused)
{
  localize_file (uri, install_from_file_cont2, NULL);
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
  if (dec == NULL)
    return;

  const char *label;
  int op = dec->decode_int ();
  int already = dec->decode_int ();
  int total = dec->decode_int ();

  set_progress ((apt_proto_operation)op, already, total);
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
  if (argc > 0)
    {
      present_main_window ();
      install_from_file_cont (g_strdup (argv[0]), NULL);
    }
}

static void
hw_state_handler (osso_hw_state_t *state, gpointer data)
{
  if (state->shutdown_ind)
    gtk_main_quit ();
}

static GtkWidget *toolbar_operation_label = NULL;
static GtkWidget *toolbar_operation_item = NULL;

static void
set_operation_toolbar_label (const char *label, bool sensitive)
{
  if (toolbar_operation_label)
    gtk_label_set_text (GTK_LABEL (toolbar_operation_label), label);
  if (toolbar_operation_item)
    gtk_widget_set_sensitive (toolbar_operation_item, sensitive);
}

static GtkWindow *main_window = NULL;
static GtkWidget *main_toolbar;

static bool fullscreen_toolbar_visibility = true;
static bool normal_toolbar_visibility = true;
static bool is_fullscreen = false;

GtkWindow *
get_main_window ()
{
  return main_window;
}

void
present_main_window ()
{
  if (main_window)
    gtk_window_present (main_window);
}

static void
set_current_toolbar_visibility (bool f)
{
  if (main_toolbar)
    {
      if (f)
	gtk_widget_show (main_toolbar);
      else
	gtk_widget_hide (main_toolbar);
    }
}

void
set_fullscreen (bool f)
{
  if (f)
    gtk_window_fullscreen (main_window);
  else
    gtk_window_unfullscreen (main_window);
}

void
toggle_fullscreen ()
{
  set_fullscreen (!is_fullscreen);
}

void
set_toolbar_visibility (bool fullscreen, bool visibility)
{
  if (fullscreen)
    {
      fullscreen_toolbar_visibility = visibility;
      if (is_fullscreen)
	set_current_toolbar_visibility (visibility);
    }
  else
    {
      normal_toolbar_visibility = visibility;
      if (!is_fullscreen)
	set_current_toolbar_visibility (visibility);
    }
}

static gboolean
window_state_event (GtkWidget *widget, GdkEventWindowState *event,
		    gpointer unused)
{
  bool f = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN);

  if (is_fullscreen != f)
    {
      is_fullscreen = f;
      set_fullscreen_menu_check (f);
      if (is_fullscreen)
	set_current_toolbar_visibility (fullscreen_toolbar_visibility);
      else
	set_current_toolbar_visibility (normal_toolbar_visibility);
    }

  return FALSE;
}

static gboolean
key_press_event (GtkWidget *widget,
		 GdkEventKey *event,
		 gpointer data)
{
  if (event->type == GDK_KEY_PRESS)
    {
      switch (event->keyval)
	{
	case HILDON_HARDKEY_FULLSCREEN:
	  toggle_fullscreen ();
	  return TRUE;
	default:
	  return FALSE;
	}
    }

  return FALSE;
}

static GtkWidget *search_button;

static void
enable_search (bool f)
{
  if (search_button)
    gtk_widget_set_sensitive (search_button, f);
}

static void
insensitive_press (GtkButton *button, gpointer data)
{
  irritate_user (_("ai_ib_not_available"));
}

static osso_context_t *osso_ctxt;

void
set_dialog_help (GtkWidget *dialog, const char *topic)
{
  if (osso_ctxt)
    {
      if (!ossohelp_dialog_help_enable (GTK_DIALOG (dialog), topic, osso_ctxt))
	add_log ("no help for %s", topic);
    }
}

static const char *current_topic;

void
show_help ()
{
  if (osso_ctxt && current_topic)
    ossohelp_show (osso_ctxt, current_topic, OSSO_HELP_SHOW_DIALOG);
}

static void
set_current_help_topic (const char *topic)
{
  current_topic = topic;
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *toolbar, *image;
  GtkMenu *main_menu;
  char *apt_worker_prog = "/usr/libexec/apt-worker";

  setlocale (LC_ALL, "");
  bind_textdomain_codeset ("osso-application-installer", "UTF-8");
  textdomain ("osso-application-installer");

  gtk_init (&argc, &argv);

  add_log ("%s %s\n", PACKAGE, VERSION);

  if (argc > 1)
    apt_worker_prog = argv[1];

  window = hildon_window_new ();
  gtk_window_set_title (GTK_WINDOW (window), _("ai_ap_application_installer"));

  main_window = GTK_WINDOW (window);

  g_signal_connect (window, "window_state_event",
		    G_CALLBACK (window_state_event), NULL);
  g_signal_connect (window, "key_press_event",
		    G_CALLBACK (key_press_event), NULL);

  toolbar = gtk_toolbar_new ();

  main_toolbar = toolbar;

  toolbar_operation_label = gtk_label_new ("");
  toolbar_operation_item =
    GTK_WIDGET (gtk_tool_button_new (toolbar_operation_label, NULL));
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (toolbar_operation_item), TRUE);
  gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (toolbar_operation_item), TRUE);
  g_signal_connect (toolbar_operation_item, "clicked",
		    G_CALLBACK (do_current_operation),
		    NULL);
  g_signal_connect (G_OBJECT (toolbar_operation_item), "insensitive_press",
		    G_CALLBACK (insensitive_press), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
		      GTK_TOOL_ITEM (toolbar_operation_item),
		      -1);

  image = gtk_image_new_from_icon_name ("qgn_toolb_gene_detailsbutton",
					HILDON_ICON_SIZE_26);
  details_button = GTK_WIDGET (gtk_tool_button_new (image, NULL));
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (details_button), TRUE);
  gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (details_button), TRUE);
  g_signal_connect (details_button, "clicked",
		    G_CALLBACK (show_current_details),
		    NULL);
  g_signal_connect (G_OBJECT (details_button), "insensitive_press",
		    G_CALLBACK (insensitive_press), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
		      GTK_TOOL_ITEM (details_button),
		      -1);

  image = gtk_image_new_from_icon_name ("qgn_toolb_gene_findbutton",
					HILDON_ICON_SIZE_26);
  search_button = GTK_WIDGET (gtk_tool_button_new (image, NULL));
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (search_button), TRUE);
  gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (search_button), TRUE);
  g_signal_connect (search_button, "clicked",
		    G_CALLBACK (show_search_dialog),
		    NULL);
  g_signal_connect (G_OBJECT (search_button), "insensitive_press",
		    G_CALLBACK (insensitive_press), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
		      GTK_TOOL_ITEM (search_button),
		      -1);

  hildon_window_add_toolbar (HILDON_WINDOW (window),
			     GTK_TOOLBAR (toolbar));

  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);

  main_trail = hildon_bread_crumb_trail_new (get_view_label, view_clicked);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_trail, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), gtk_hseparator_new (), 
		      FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (window_destroy), NULL);

  main_menu = GTK_MENU (gtk_menu_new ());
  create_menu (main_menu);
  hildon_window_set_menu (HILDON_WINDOW (window), GTK_MENU (main_menu));

  load_settings ();

  gtk_widget_show_all (window);
  show_view (&main_view);

  /* XXX - check errors.
   */
  osso_ctxt = osso_initialize ("osso_application_installer",
			       PACKAGE_VERSION, TRUE, NULL);

  osso_mime_set_cb (osso_ctxt, mime_open_handler, NULL);

  osso_hw_state_t state = { 0 };
  state.shutdown_ind = true;
  osso_hw_set_event_cb (osso_ctxt, &state, hw_state_handler, NULL);

  if (start_apt_worker (apt_worker_prog))
    {
      apt_worker_set_status_callback (apt_status_callback, NULL);
      add_apt_worker_handler ();

      maybe_refresh_package_cache ();
      get_package_list ();
    }
  else
    annoy_user_with_log (_("ai_ni_operation_failed"));

  gtk_main ();
}
