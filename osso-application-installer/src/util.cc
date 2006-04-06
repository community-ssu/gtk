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

#define DBUS_API_SUBJECT_TO_CHANGE

#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <libintl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/gtk-infoprint.h>
#include <osso-ic.h>
#include <libgnomevfs/gnome-vfs.h>

#include "util.h"
#include "details.h"
#include "log.h"
#include "settings.h"
#include "menu.h"
#include "apt-worker-client.h"

#define _(x) gettext (x)

struct ayn_closure {
  package_info *pi;
  bool installed;
  void (*cont) (bool res, void *data);
  void *data;
};
  
static void
yes_no_response (GtkDialog *dialog, gint response, gpointer clos)
{
  ayn_closure *c = (ayn_closure *)clos;
  
  if (response == 1)
    {
      if (c->pi)
	show_package_details (c->pi, c->installed);
      return;
    }

  void (*cont) (bool res, void *data) = c->cont; 
  void *data = c->data;
  if (c->pi)
    c->pi->unref ();
  delete c;

  gtk_widget_destroy (GTK_WIDGET (dialog));
  cont (response == GTK_RESPONSE_OK, data);
}

void
ask_yes_no (const gchar *question,
	    void (*cont) (bool res, void *data),
	    void *data)
{
  GtkWidget *dialog;
  ayn_closure *c = new ayn_closure;
  c->pi = NULL;
  c->cont = cont;
  c->data = data;

  dialog = hildon_note_new_confirmation (get_main_window (), question);

  g_signal_connect (dialog, "response",
		    G_CALLBACK (yes_no_response), c);
  gtk_widget_show_all (dialog);
}

void
ask_yes_no_with_details (const gchar *title,
			 const gchar *question,
			 package_info *pi, bool installed,
			 void (*cont) (bool res, void *data),
			 void *data)
{
  GtkWidget *dialog;
  ayn_closure *c = new ayn_closure;
  c->pi = pi;
  pi->ref ();
  c->installed = installed;
  c->cont = cont;
  c->data = data;

  dialog = gtk_dialog_new_with_buttons
    (title,
     get_main_window (),
     GTK_DIALOG_MODAL,
     _("ai_bd_confirm_ok"),      GTK_RESPONSE_OK,
     _("ai_bd_confirm_details"), 1,
     _("ai_bd_confirm_cancel"),  GTK_RESPONSE_CANCEL,
     NULL);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
		     gtk_label_new (question));

  g_signal_connect (dialog, "response",
		    G_CALLBACK (yes_no_response), c);
  gtk_widget_show_all (dialog);
}

static void
annoy_user_response (GtkDialog *dialog, gint response, gpointer data)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
annoy_user (const gchar *text)
{
  GtkWidget *dialog;

  dialog = hildon_note_new_information (NULL, text);
  g_signal_connect (dialog, "response", 
		    G_CALLBACK (annoy_user_response), NULL);
  gtk_widget_show_all (dialog);
}

struct auwd_closure {
  package_info *pi;
  bool installed;
};

static void
annoy_user_with_details_response (GtkDialog *dialog, gint response,
				  gpointer data)
{
  auwd_closure *c = (auwd_closure *)data;
  package_info *pi = c->pi;
  bool installed = c->installed;
  delete c;

  gtk_widget_destroy (GTK_WIDGET (dialog));
  if (response == 1)
    show_package_details (pi, installed);
  pi->unref ();
}

void
annoy_user_with_details (const gchar *text,
			 package_info *pi, bool installed)
{
  GtkWidget *dialog, *action_area;
  GtkWidget *details_button;
  auwd_closure *c = new auwd_closure;
  gint response;

  // XXX - the buttons should be "Details" "Close", but this gives
  //       "Ok" "Details".

  dialog = hildon_note_new_information (get_main_window (), text);
  gtk_dialog_add_button (GTK_DIALOG (dialog), _("ai_ni_bd_details"), 1);

  pi->ref ();
  c->pi = pi;
  c->installed = installed;
  g_signal_connect (dialog, "response", 
		    G_CALLBACK (annoy_user_with_details_response), c);
  gtk_widget_show_all (dialog);
}

struct auwe_closure {
  void (*func) (void *, void *);
  void *data1, *data2;
};

static void
annoy_user_with_log_response (GtkDialog *dialog, gint response,
			      gpointer data)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
  if (response == 1)
    show_log ();
}

void
annoy_user_with_log (const gchar *text)
{
  GtkWidget *dialog, *action_area;
  GtkWidget *details_button;

  dialog = hildon_note_new_information (get_main_window (), text);
#if 0
  gtk_dialog_add_button (GTK_DIALOG (dialog), _("ai_ni_bd_log"), 1);
#endif

  g_signal_connect (dialog, "response", 
		    G_CALLBACK (annoy_user_with_log_response), NULL);
  gtk_widget_show_all (dialog);
}

void
irritate_user (const gchar *text)
{
  gtk_infoprintf (NULL, "%s", text);
}

void
scare_user_with_legalese (bool sure,
			  void (*cont) (bool res, void *data),
			  void *data)
{
  ayn_closure *c = new ayn_closure;
  c->pi = NULL;
  c->cont = cont;
  c->data = data;

  GtkWidget *dialog;

  dialog = gtk_dialog_new_with_buttons
    (_("ai_ti_notice"),
     get_main_window (),
     GTK_DIALOG_MODAL,
     _("ai_bd_notice_ok"),      GTK_RESPONSE_OK,
     _("ai_bd_notice_cancel"),  GTK_RESPONSE_CANCEL,
     NULL);

  const char *text = (sure
		      ? _("ai_nc_non_verified_package")
		      : _("ai_nc_unsure_package"));

  GtkWidget *label = gtk_label_new (text);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), label);

  g_signal_connect (dialog, "response",
		    G_CALLBACK (yes_no_response), c);
  gtk_widget_show_all (dialog);
}

static GtkWidget *progress_dialog = NULL;
static GtkWidget *progress_cancel_button;
static bool progress_cancelled;
static GtkProgressBar *progress_bar;
static const gchar *general_title;
static apt_proto_operation current_status_operation = op_general;
static gint pulse_id = -1;

static gboolean
pulse_progress (gpointer unused)
{
  if (progress_bar)
    gtk_progress_bar_pulse (progress_bar);
  return TRUE;
}

static void
start_pulsing ()
{
  if (pulse_id < 0)
    pulse_id = gtk_timeout_add (500, pulse_progress, NULL);
}

static void
stop_pulsing ()
{
  if (pulse_id >= 0)
    {
      g_source_remove (pulse_id);
      pulse_id = -1;
    }
}

static void
cancel_response (GtkDialog *dialog, gint response, gpointer data)
{
  if (current_status_operation == op_downloading)
    {
      progress_cancelled = true;
      cancel_apt_worker ();
    }
  else
    irritate_user (_("ai_ib_unable_cancel"));
}

bool
progress_was_cancelled ()
{
  return progress_cancelled;
}

void
show_progress (const char *title)
{
  if (progress_dialog == NULL)
    {
      progress_bar = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
      progress_dialog =
	hildon_note_new_cancel_with_progress_bar (get_main_window (),
						  NULL,
						  progress_bar);
      g_signal_connect (progress_dialog, "response",
			G_CALLBACK (cancel_response), NULL);
      GtkWidget *box = GTK_DIALOG (progress_dialog)->action_area;
      GList *kids = gtk_container_get_children (GTK_CONTAINER (box));
      if (kids)
	progress_cancel_button = GTK_WIDGET (kids->data);
      else
	progress_cancel_button = NULL;
      g_list_free (kids);
    }

  general_title = title;
  set_progress (op_general, -1, 0);

  gtk_widget_show (progress_dialog);

  progress_cancelled = false;
}

void
set_progress (apt_proto_operation op, int already, int total)
{
  const char *title;

  if (op == op_downloading)
    {
      static int last_total;
      static char *dynlabel = NULL;

      if (dynlabel == NULL || total != last_total)
	{
	  char size_buf[20];
	  size_string_detailed (size_buf, 20, total);
	  g_free (dynlabel);
	  dynlabel =  g_strdup_printf (_("ai_nw_downloading"), size_buf);
	}
      title = dynlabel;
    }
  else if (op == op_updating_cache)
    title = _("ai_nw_updating_list");
  else
    title = general_title;

  current_status_operation = op;

#if 0
  if (progress_cancel_button)
    gtk_widget_set_sensitive (progress_cancel_button, op == op_downloading);
#endif

  // printf ("STATUS: %s -- %f\n", title, fraction);

  if (progress_dialog)
    {
      g_object_set (progress_dialog, "description", title, NULL);
      
      if (already >= 0)
	{
	  stop_pulsing ();
	  gtk_progress_bar_set_fraction (progress_bar,
					 ((double)already)/total);
	}
      else
	start_pulsing ();
    }
}

void
hide_progress ()
{
  if (progress_dialog)
    gtk_widget_hide (progress_dialog);
  current_status_operation = op_general;
}

static PangoFontDescription *
get_small_font ()
{
  static PangoFontDescription *small_font = NULL;

  if (small_font == NULL)
    {
      // XXX - do it nicer...
      small_font = pango_font_description_from_string ("Nokia Sans 13");
    }
  return small_font;
}

GtkWidget *
make_small_text_view (const char *text)
{
  GtkWidget *scroll;
  GtkWidget *view;
  GtkTextBuffer *buffer;
  
  scroll = gtk_scrolled_window_new (NULL, NULL);
  view = gtk_text_view_new ();
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_set_text (buffer, text, -1);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), 0);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), 0);
  gtk_container_add (GTK_CONTAINER (scroll), view);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_widget_modify_font (view, get_small_font ());

  return scroll;
}

void
set_small_text_view_text (GtkWidget *scroll, const char *text)
{
  GtkWidget *view = gtk_bin_get_child (GTK_BIN (scroll));
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_set_text (buffer, text, -1);
}

GtkWidget *
make_small_label (const char *text)
{
  GtkWidget *label = gtk_label_new (text);
  gtk_widget_modify_font (label, get_small_font ());
  return label;
}

static GtkListStore *global_list_store = NULL;
static bool global_installed;

static GdkPixbuf *default_icon = NULL;

static void
global_icon_func (GtkTreeViewColumn *column,
		  GtkCellRenderer *cell,
		  GtkTreeModel *model,
		  GtkTreeIter *iter,
		  gpointer data)
{
  package_info *pi;
  gtk_tree_model_get (model, iter, 0, &pi, -1);
  if (!pi)
    return;

  GdkPixbuf *icon = (global_installed
		     ? pi->installed_icon
		     : pi->available_icon);

  if (default_icon == NULL)
    {
      GtkIconTheme *icon_theme;

      icon_theme = gtk_icon_theme_get_default ();
      default_icon = gtk_icon_theme_load_icon (icon_theme,
					       "qgn_list_gene_default_app",
					       26,
					       GtkIconLookupFlags(0),
					       NULL);
    }

  g_object_set (cell, "pixbuf", icon? icon : default_icon, NULL);
}

static void
global_name_func (GtkTreeViewColumn *column,
		  GtkCellRenderer *cell,
		  GtkTreeModel *model,
		  GtkTreeIter *iter,
		  gpointer data)
{
  GtkTreeView *tree = (GtkTreeView *)data;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  package_info *pi;

  gtk_tree_model_get (model, iter, 0, &pi, -1);
  if (!pi)
    return;

  if (gtk_tree_selection_iter_is_selected (selection, iter))
    {
      const gchar *desc;
      gchar *markup;
      if (global_installed)
	desc = pi->installed_short_description;
      else
	{
	  desc = pi->available_short_description;
	  if (desc == NULL)
	    desc = pi->installed_short_description;
	}
      markup = g_markup_printf_escaped ("%s\n<small>%s</small>",
					pi->name, desc);
      g_object_set (cell, "markup", markup, NULL);
      g_free (markup);
    }
  else
    g_object_set (cell, "text", pi->name, NULL);
}

static void
global_row_changed (GtkTreeIter *iter)
{
  GtkTreePath *path;
  GtkTreeModel *model = GTK_TREE_MODEL (global_list_store);

  path = gtk_tree_model_get_path (model, iter);
  g_signal_emit_by_name (model, "row-changed", path, iter);
  gtk_tree_path_free (path);
}

static void
global_version_func (GtkTreeViewColumn *column,
		     GtkCellRenderer *cell,
		     GtkTreeModel *model,
		     GtkTreeIter *iter,
		     gpointer data)
{
  package_info *pi;
  gtk_tree_model_get (model, iter, 0, &pi, -1);
  if (!pi)
    return;

  g_object_set (cell, "text", (global_installed
			       ? pi->installed_version
			       : pi->available_version),
		NULL);
}

static void
global_size_func (GtkTreeViewColumn *column,
		  GtkCellRenderer *cell,
		  GtkTreeModel *model,
		  GtkTreeIter *iter,
		  gpointer data)
{
  package_info *pi;
  char buf[20];

  gtk_tree_model_get (model, iter, 0, &pi, -1);
  if (!pi)
    return;

  if (global_installed)
    size_string_general (buf, 20, pi->installed_size);
  else if (pi->have_info)
    size_string_general (buf, 20, pi->info.download_size);
  else
    strcpy (buf, "-");
  g_object_set (cell, "text", buf, NULL);
}

static bool global_have_last_selection;
static GtkTreeIter global_last_selection;
static package_info_callback *global_selection_callback;

static void
global_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (global_have_last_selection)
    global_row_changed (&global_last_selection);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      package_info *pi;

      assert (model == GTK_TREE_MODEL (global_list_store));

      global_row_changed (&iter);
      global_last_selection = iter;
      global_have_last_selection = true;
      
      if (global_selection_callback)
	{
	  gtk_tree_model_get (model, &iter, 0, &pi, -1);
	  if (pi)
	    global_selection_callback (pi);
	}
    }
  else
    {
      if (global_selection_callback)
	global_selection_callback (NULL);
    }
}

static package_info_callback *global_activation_callback;

static void
global_row_activated (GtkTreeView *treeview,
		      GtkTreePath *path,
		      GtkTreeViewColumn *column,
		      gpointer data)
{
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;

  assert (model == GTK_TREE_MODEL (global_list_store));

  if (global_activation_callback &&
      gtk_tree_model_get_iter (model, &iter, path))
    {
      package_info *pi;
      gtk_tree_model_get (model, &iter, 0, &pi, -1);
      if (pi)
	global_activation_callback (pi);
    }
}

static void set_global_package_list (GList *packages,
				     bool installed,
				     package_info_callback *selected,
				     package_info_callback *activated);

static GList *global_packages;

GtkWidget *
make_global_package_list (GList *packages,
			  bool installed,
			  const char *empty_label,
			  const char *op_label,
			  package_info_callback *selected,
			  package_info_callback *activated)
{
  if (global_list_store == NULL)
    {
      global_list_store = gtk_list_store_new (1, GTK_TYPE_POINTER);
      g_object_ref (global_list_store);
    }

  if (packages == NULL)
    {
      GtkWidget *label = gtk_label_new (empty_label);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.0);
      return label;
    }

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkWidget *tree, *scroller;

  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (global_list_store));

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), TRUE);

  renderer = gtk_cell_renderer_pixbuf_new ();
  g_object_set (renderer, "yalign", 0.0, NULL);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
					      -1,
					      NULL,
					      renderer,
					      global_icon_func,
					      tree,
					      NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), 0);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_expand (column, FALSE);
  gtk_tree_view_column_set_fixed_width (column, 30);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "yalign", 0.0, NULL);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
					      -1,
					      NULL,
					      renderer,
					      global_name_func,
					      tree,
					      NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), 1);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_expand (column, TRUE);
  gtk_tree_view_column_set_fixed_width (column, 400);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "yalign", 0.0, NULL);
  g_object_set (renderer, "xalign", 1.0, NULL);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
					      -1,
					      "Version",
					      renderer,
					      global_version_func,
					      tree,
					      NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), 2);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_expand (column, FALSE);
  gtk_tree_view_column_set_fixed_width (column, 140);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "yalign", 0.0, NULL);
  g_object_set (renderer, "xalign", 1.0, NULL);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
					      -1,
					      "Size",
					      renderer,
					      global_size_func,
					      tree,
					      NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), 3);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_expand (column, FALSE);
  gtk_tree_view_column_set_fixed_width (column, 80);

  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scroller), tree);

  global_have_last_selection = false;
  g_signal_connect
    (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree))),
     "changed",
     G_CALLBACK (global_selection_changed), NULL);

  g_signal_connect (tree, "row-activated", 
		    G_CALLBACK (global_row_activated), NULL);

  GtkWidget *menu = create_package_menu (op_label);
  gtk_widget_show_all (menu);
  gtk_widget_tap_and_hold_setup (tree, menu, NULL,
				 GtkWidgetTapAndHoldFlags (0));

  set_global_package_list (packages, installed, selected, activated);

  return scroller;
}

static void
set_global_package_list (GList *packages,
			 bool installed,
			 package_info_callback *selected,
			 package_info_callback *activated)
{
  global_have_last_selection = false;

  for (GList *p = global_packages; p; p = p->next)
    {
      package_info *pi = (package_info *)p->data;
      pi->model = NULL;
    }
  if (global_list_store)
    gtk_list_store_clear (global_list_store);

  global_installed = installed;
  global_selection_callback = selected;
  global_activation_callback = activated;
  global_packages = packages;
  
  for (GList *p = global_packages; p; p = p->next)
    {
      GtkTreeIter iter;
      package_info *pi = (package_info *)p->data;
      pi->model = GTK_TREE_MODEL (global_list_store);
      gtk_list_store_append (global_list_store, &pi->iter);
      gtk_list_store_set (global_list_store, &pi->iter,
			  0, pi,
			  -1);
    }
}

void
clear_global_package_list ()
{
  set_global_package_list (NULL, false, NULL, NULL);
}

void
global_package_info_changed (package_info *pi)
{
  if (pi->model == GTK_TREE_MODEL (global_list_store))
    global_row_changed (&pi->iter);
}

static GtkWidget *global_section_list;
static section_activated *global_section_activated;

static void
section_clicked (GtkWidget *widget, gpointer data)
{
  section_info *si = (section_info *)data;
  if (global_section_activated)
    global_section_activated (si);
}

GtkWidget *
make_global_section_list (GList *sections, section_activated *act)
{
  global_section_activated = act;

  if (sections == NULL)
    {
      GtkWidget *label = gtk_label_new (_("ai_li_no_applications_available"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.0);
      return label;
    }

  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 5);
  GtkWidget *scroller;

  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 10);

  for (GList *s = sections; s; s = s ->next)
    {
      section_info *si = (section_info *)s->data;
      GtkWidget *label = gtk_label_new (si->name);
      gtk_misc_set_padding (GTK_MISC (label), 15, 15);
      GtkWidget *btn = gtk_button_new ();
      gtk_container_add (GTK_CONTAINER (btn), label);
      gtk_box_pack_start (GTK_BOX (vbox), btn, FALSE, FALSE, 0);
      g_signal_connect (btn, "clicked",
			G_CALLBACK (section_clicked), si);
    }

  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroller),
					 hbox);

  global_section_list = scroller;

  return scroller;
}

void
clear_global_section_list ()
{
  if (global_section_list)
    gtk_widget_destroy (global_section_list);
  global_section_list = NULL;
}

#define KILO 1000

void
size_string_general (char *buf, size_t n, int bytes)
{
  size_t num = bytes;

  if (num == 0)
    snprintf (buf, n, _("ai_li_size_max_99kb"), 0);
  else if (num < 1*KILO)
    snprintf (buf, n, _("ai_li_size_max_99kb"), 1);
  else
    {
      // round to nearest KILO
      // bytes ~ num * KILO
      num = (bytes + KILO/2) / KILO;
      if (num < 100)
	snprintf (buf, n, _("ai_li_size_max_99kb"), num);
      else
	{
	  // round to nearest 100 KILO
	  // bytes ~ num * 100 * KILO
	  num = (bytes + 50*KILO) / (100*KILO);
	  if (num < 100)
	    snprintf (buf, n, _("ai_li_size_100kb_10mb"), num/10.0);
	  else
	    {
	      // round to nearest KILO KILO
	      // bytes ~ num * KILO * KILO
	      num = (bytes + KILO*KILO/2) / (KILO*KILO);
	      if (num < KILO)
		snprintf (buf, n, _("ai_li_size_10mb_1gb"), num);
	      else
		snprintf (buf, n, _("ai_li_size_larger_than_1gb"),
			  ((float)num)/KILO);
	    }
	}
    }
}

void
size_string_detailed (char *buf, size_t n, int bytes)
{
  size_t num = bytes;

  if (num == 0)
    snprintf (buf, n, _("ai_li_de_size_max_999kb"), 0);
  else if (num < 1*KILO)
    snprintf (buf, n, _("ai_li_de_size_max_999kb"), 1);
  else
    {
      // round to nearest KILO
      // bytes ~ num * KILO
      num = (bytes + KILO/2) / KILO;
      if (num < 1000)
	snprintf (buf, n, _("ai_li_de_size_max_999kb"), num);
      else
	{
	  // round to nearest 10 KILO
	  // bytes ~ num * 10 * KILO
	  num = (bytes + 5*KILO) / (10*KILO);
	  if (num < 1000)
	    snprintf (buf, n, _("ai_li_de_size_1mb_10mb"), num/100.0);
	  else
	    {
	      if (num < 10000)
		snprintf (buf, n, _("ai_li_de_size_10mb_1gb"), num/100.0);
	      else
		snprintf (buf, n, _("ai_li_de_size_larger_than_1g"),
			  ((float)num)/(100*KILO));
	    }
	}
    }
}

struct fcd_closure {
  void (*cont) (char *filename, void *data);
  void *data;
};

static void
fcd_response (GtkDialog *dialog, gint response, gpointer clos)
{
  fcd_closure *c = (fcd_closure *)clos;
  void (*cont) (char *filename, void *data) = c->cont; 
  void *data = c->data;
  delete c;

  char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

  gtk_widget_destroy (GTK_WIDGET (dialog));

  if (response == GTK_RESPONSE_OK)
    cont (filename, data);
  else
    g_free (filename);
}

void
show_deb_file_chooser (void (*cont) (char *filename, void *data),
		       void *data)
{
  fcd_closure *c = new fcd_closure;
  c->cont = cont;
  c->data = data;

  GtkWidget *fcd;
  GtkFileFilter *filter;

  fcd = hildon_file_chooser_dialog_new_with_properties
    (get_main_window (),
     "action",            GTK_FILE_CHOOSER_ACTION_OPEN,
     "title",             _("ai_ti_select_package"),
     "empty_text",        _("ai_ia_select_package_no_packages"),
     "open_button_text",  _("ai_bd_select_package"),
     NULL);
  gtk_window_set_modal (GTK_WINDOW (fcd), TRUE);

  filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (filter, "application/x-deb");
  gtk_file_filter_add_mime_type (filter, "application/x-debian-package");
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER(fcd), filter);
  // XXX - gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(fcd), TRUE);

  g_signal_connect (fcd, "response",
		    G_CALLBACK (fcd_response), c);

  gtk_widget_show_all (fcd);
}

void
show_file_chooser_for_save (const char *title,
			    const char *default_filename,
			    void (*cont) (char *filename, void *data),
			    void *data)
{
  fcd_closure *c = new fcd_closure;
  c->cont = cont;
  c->data = data;

  GtkWidget *fcd;

  fcd = hildon_file_chooser_dialog_new_with_properties
    (get_main_window (),
     "action",            GTK_FILE_CHOOSER_ACTION_SAVE,
     "title",             title,
     NULL);
  gtk_window_set_modal (GTK_WINDOW (fcd), TRUE);

  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (fcd), default_filename);

  g_signal_connect (fcd, "response",
		    G_CALLBACK (fcd_response), c);

  gtk_widget_show_all (fcd);
}

static void
b64decode (const unsigned char *str, GdkPixbufLoader *loader)
{
  unsigned const char *cur, *start;
  int d, dlast, phase;
  unsigned char c;
  static int table[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
  };

  const size_t loader_size = 2048;
  unsigned char loader_buf[loader_size], *loader_ptr;
  GError *error = NULL;

  d = dlast = phase = 0;
  start = str;
  loader_ptr = loader_buf;

  for (cur = str; *cur != '\0'; ++cur )
    {
      d = table[(int)*cur];
      if(d != -1)
        {
	  switch(phase)
            {
            case 0:
	      ++phase;
	      break;
            case 1:
	      c = ((dlast << 2) | ((d & 0x30) >> 4));
	      *loader_ptr++ = c;
	      ++phase;
	      break;
            case 2:
	      c = (((dlast & 0xf) << 4) | ((d & 0x3c) >> 2));
	      *loader_ptr++ = c;
	      ++phase;
	      break;
            case 3:
	      c = (((dlast & 0x03 ) << 6) | d);
	      *loader_ptr++ = c;
	      phase = 0;
	      break;
            }
	  dlast = d;
	  if (loader_ptr > loader_buf + loader_size)
	    {
	      gdk_pixbuf_loader_write (loader, loader_buf, loader_size,
				       &error);
	      if (error)
		{
		  fprintf (stderr, "PX: %s\n", error->message);
		  g_error_free (error);
		  return;
		}
	    }
        }
    }
  
  gdk_pixbuf_loader_write (loader, loader_buf, loader_ptr - loader_buf,
			   &error);
  if (error)
    {
      fprintf (stderr, "PX: %s\n", error->message);
      g_error_free (error);
      return;
    }
}


GdkPixbuf *
pixbuf_from_base64 (const char *base64)
{
  if (base64 == NULL)
    return NULL;

  GError *error = NULL;

  GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
  b64decode ((const unsigned char *)base64, loader);
  gdk_pixbuf_loader_close (loader, &error);
  if (error)
    {
      fprintf (stderr, "PX: %s\n", error->message);
      g_error_free (error);
      g_object_unref (loader);
      return NULL;
    }
  
  GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
  g_object_ref (pixbuf);
  g_object_unref (loader);
  return pixbuf;
}

void
localize_file (const char *uri,
	       void (*cont) (char *local, void *data),
	       void *data)
{
  if (!gnome_vfs_init ())
    {
      // XXX
      annoy_user ("Couldn't init GnomeVFS.");
      cont (NULL, data);
      return;
    }

  GnomeVFSURI *vfs_uri = gnome_vfs_uri_new (uri);

  if (vfs_uri == NULL)
    {
      fprintf (stderr, "uri: %s\n", uri);
      annoy_user ("Malformed URI");
      cont (NULL, data);
      return;
    }

  /* The app-worker can access all "file://" URIs, whether they are
     considered local by GnomeVFS or not.  (GnomeVFS considers a
     file:// URI pointing to a NFS mounted volume as remote, but we
     can read that just fine of course.)
  */

  const gchar *scheme = gnome_vfs_uri_get_scheme (vfs_uri);
  if (scheme && !strcmp (scheme, "file"))
    {
      const gchar *path = gnome_vfs_uri_get_path (vfs_uri);
      gchar *unescaped_path = gnome_vfs_unescape_string (path, NULL);
      if (unescaped_path == NULL)
	annoy_user ("Hmm.");
      cont (unescaped_path, data);
    }
  else
    {
      // XXX
      annoy_user ("Unsupported file location.");
      cont (NULL, data);
    }
}

struct rc_closure {
  void (*cont) (int status, void *data);
  void *data;
};

static void
reap_process (GPid pid, int status, gpointer raw_data)
{
  rc_closure *c = (rc_closure *)raw_data;
  void (*cont) (int status, void *data) = c->cont;
  void *data = c->data;
  delete c;

  cont (status, data);
}

void
run_cmd (char **argv,
	 void (*cont) (int status, void *data),
	 void *data)
{
  int stdout_fd, stderr_fd;
  GError *error = NULL;
  GPid child_pid;
  
  if (!g_spawn_async_with_pipes (NULL,
				 argv,
				 NULL,
				 GSpawnFlags (G_SPAWN_DO_NOT_REAP_CHILD),
				 NULL,
				 NULL,
				 &child_pid,
				 NULL,
				 &stdout_fd,
				 &stderr_fd,
				 &error))
    {
      add_log ("Can't run %s: %s\n", argv[0], error->message);
      g_error_free (error);
      cont (-1, data);
      return;
    }

  log_from_fd (stdout_fd);
  log_from_fd (stderr_fd);

  rc_closure *c = new rc_closure;
  c->cont = cont;
  c->data = data;
  g_child_watch_add (child_pid, reap_process, c);
}

int
all_white_space (const char *text)
{
  while (*text)
    if (!isspace (*text++))
      return 0;
  return 1;
}

struct en_closure {
  void (*callback) (bool success, void *data);
  void *data;
};

static void
iap_callback (struct iap_event_t *event, void *arg)
{
  void (*callback) (bool success, void *data) = NULL;
  void *data;

  if (arg)
    {
      en_closure *c = (en_closure *)arg;
      callback = c->callback;
      data = c->data;
      delete c;
    }

  switch (event->type)
    {
    case OSSO_IAP_CONNECTED:
      add_log ("OSSO_IAP_CONNECTED: %s\n", event->iap_name);
      if (callback)
	callback (true, data);
      break;

    case OSSO_IAP_DISCONNECTED:
      add_log ("OSSO_IAP_DISCONNECTED: %s\n", event->iap_name);
      if (current_status_operation == op_downloading)
	cancel_apt_worker ();
      break;

    case OSSO_IAP_ERROR:
      add_log ("IAP Error: %x %s.\n", -event->u.error_code, event->iap_name);
      if (callback)
	callback (false, data);
      break;
    }
}

void
ensure_network (void (*callback) (bool success, void *data), void *data)
{
  if (assume_connection)
    {
      callback (true, data);
      return;
    }
  
  if (osso_iap_cb (iap_callback) == OSSO_OK)
    {
      en_closure *c = new en_closure;
      c->callback = callback;
      c->data = data;
      
      if (osso_iap_connect (OSSO_IAP_ANY, OSSO_IAP_REQUESTED_CONNECT, c)
	  == OSSO_OK)
	return;

      delete c;
    }

  // annoy_user (_("ai_ni_error_download_failed"));
  callback (false, data);
}

char *
get_http_proxy ()
{
  char *proxy;

  GConfClient *conf = gconf_client_get_default ();

  /* We clear the cache here in order to force a fresh fetch of the
     values.  Otherwise, there is a race condition with the
     iap_callback: the OSSO_IAP_CONNECTED message might come before
     the GConf cache has picked up the new proxy settings.

     At least, that's the theory.
  */
  gconf_client_clear_cache (conf);

  if (gconf_client_get_bool (conf, "/system/http_proxy/use_http_proxy",
			     NULL))
    {
      char *user = NULL;
      char *password = NULL;
      char *host;
      gint port;

      if (gconf_client_get_bool (conf, "/system/http_proxy/use_authentication",
				 NULL))
	{
	  user = gconf_client_get_string
	    (conf, "/system/http_proxy/authentication_user", NULL);
	  password = gconf_client_get_string
	    (conf, "/system/http_proxy/authentication_password", NULL);
	}

      host = gconf_client_get_string (conf, "/system/http_proxy/host", NULL);
      port = gconf_client_get_int (conf, "/system/http_proxy/port", NULL);

      if (user)
	{
	  // XXX - encoding of '@', ':' in user and password?

	  if (password)
	    proxy = g_strdup_printf ("http://%s:%s@%s:%d",
				     user, password, host, port);
	  else
	    proxy = g_strdup_printf ("http://%s@%s:%d", user, host, port);
	}
      else
	proxy = g_strdup_printf ("http://%s:%d", host, port);
    }
  else
    proxy = NULL;

  /* XXX - there is also ignore_hosts, which we ignore for now, since
           transcribing it to no_proxy is hard... mandatory,
           non-transparent proxies are evil anyway.
   */

  g_object_unref (conf);

  return proxy;
}

void
push (GSList *&ptr, void *data)
{
  ptr = g_slist_prepend (ptr, data);
}

void *
pop (GSList *&ptr)
{
  void *data = ptr->data;
  GSList *next = ptr->next;
  g_slist_free_1 (ptr);
  ptr = next;
  return data;
}
