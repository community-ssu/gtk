/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

/*
 *
 * Implementation of close application dialog
 *
 */

/* Gtk+ includes */
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

/* Log macro includes */
#include <osso-log.h>

/* Hildon headers */
#include "hn-wm.h"


/* Include our own header */
#include "close-application-dialog.h"

typedef struct _CADItem CADItem;
struct _CADItem
{
  HNWMWatchedWindow* win;
  GtkToggleButton *button;
  gint vmdata;
};


void item_toggled (GtkToggleButton *button,
                   gpointer user_data);
void kill_selected_items (GList *items);


void item_toggled (GtkToggleButton *button,
                   gpointer user_data)
{
  gboolean selected;
  GList *l;
  GList *items;
  GtkDialog *dialog;
  CADItem *item;
  
  item = NULL;
  selected = FALSE;

  items = (GList*) user_data;

  for (l = items; l; l = l->next)
    {
      item = (CADItem *) l->data;
      if (gtk_toggle_button_get_active(item->button))
        {
          selected = TRUE;
        }
    }

  dialog = GTK_DIALOG(gtk_widget_get_toplevel(GTK_WIDGET(item->button)));

  gtk_dialog_set_response_sensitive(dialog,
                                    GTK_RESPONSE_ACCEPT,
                                    selected);

  
}

void kill_selected_items (GList *items)
{
  GList *l;
  
  for (l = items; l; l = l->next)
    {
      CADItem *item = (CADItem *) l->data;
      if (gtk_toggle_button_get_active(item->button))
        {
          hn_wm_watched_window_attempt_signal_kill (item->win, SIGTERM);
        }
    }
}

gint compare_items (gconstpointer a, gconstpointer b);

gint compare_items (gconstpointer a, gconstpointer b)
{
  CADItem *ia;
  CADItem *ib;
  
  ia = (CADItem *)a;
  ib = (CADItem *)b;
  
  return (ib->vmdata - ia->vmdata);
} 

gboolean tn_close_application_dialog(CADAction action)
{
  gint response;
  gint i;
  gboolean retval;
  GList *l;
  GList *items;
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *swin;
  GtkWidget *check;
  CADItem *item;
  GtkRequisition req;
  HNWM *wm;
  
  items = NULL;
  check = NULL;
  req.height = 0;

  /* Get the open applications 
     FIXME: Is there a better way to get them?
   */
  wm = hn_wm_get_singleton();
  for (l = application_switcher_get_menuitems(wm->app_switcher); l; l = l->next)
    {
      guchar *pid_result = NULL;
      gint pid;
      HNWMWatchableApp *app;
      HNWMWatchedWindow* win;
      GdkAtom a;

      /* Skip the separator */
      if (GTK_IS_SEPARATOR_MENU_ITEM(l->data))
        {
          continue;
        }
      
      app = NULL;
      app = hn_wm_lookup_watchable_app_via_menu(GTK_WIDGET(l->data));
      if (app == NULL)
        {
          ULOG_WARN("No app found for item %s",gtk_label_get_text(GTK_LABEL(GTK_BIN(l->data)->child)));
          continue;
        }

      /* Skip hibernating apps */
      if (hn_wm_watchable_app_is_hibernating(app))
        {
          continue;
        }

      /* FIXME: this fails most of the time, why? */
      win = NULL;
      win = hn_wm_lookup_watched_window_via_menu_widget (GTK_WIDGET(l->data));
      if (win == NULL)
        {
          const gchar *service;
          service = hn_wm_watchable_app_get_service(app);
          win = hn_wm_lookup_watched_window_via_service (service);
        }
      if (win == NULL)
        {
          ULOG_WARN("No win found for item %s",gtk_label_get_text(GTK_LABEL(GTK_BIN(l->data)->child)) );
          continue;
        }

      /* FIXME: This is interned here since we don't want to rely on extern
       * global structures (which are slated for removal anyway).
       */
      a = gdk_atom_intern("_NET_WM_PID", FALSE);
      pid_result = hn_wm_util_get_win_prop_data_and_validate (hn_wm_watched_window_get_x_win(win),
							  gdk_x11_atom_to_xatom (a),
							  XA_CARDINAL,
							  32,
							  0,
							  NULL);
      if (pid_result == NULL)
        {
          ULOG_WARN("No pid found for item %s",gtk_label_get_text(GTK_LABEL(GTK_BIN(l->data)->child)) );
          continue;
        }

      pid = pid_result[0]+256*pid_result[1];

      ULOG_DEBUG ("%s(): %s is %s, Pid:%i, VmData: %ikB\n", __FUNCTION__,
        hn_wm_watchable_app_get_name(app),
        hn_wm_watchable_app_is_hibernating(app) ? "hibernating" : "awake",
        pid,
        hn_wm_get_vmdata_for_pid(pid));

      item = g_new0(CADItem, 1);
      
      item->win = win;
      item->vmdata = hn_wm_get_vmdata_for_pid(pid);
      
      items = g_list_append(items, item);
      
    }

  if (g_list_length(items) == 0)
    {
      /* If there wasn't any items we could kill, return TRUE to try the
       * action anyway without showing the dialog.
       */
      return TRUE;
    }

  /* Sort */
  items = g_list_sort(items, compare_items);

  i = 0;
  l = g_list_last(items);

  /* Truncate to 10 items */
  while (g_list_length (items) > 10)
    {
      g_free (l->data);
      items = g_list_delete_link (items, l);
      l = g_list_last(items);
    }
  
  /* Creating the UI */
  dialog = gtk_dialog_new_with_buttons (_("memr_ti_close_applications"),
                                        NULL,
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        _("memr_bd_close_applications_ok"),
                                        GTK_RESPONSE_ACCEPT,
                                        _("memr_bd_close_applications_cancel"),
                                        GTK_RESPONSE_REJECT,
                                        NULL);                                        

  gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
  /* Ok is dimmed when there is nothing selected */
  gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
                                    GTK_RESPONSE_ACCEPT,
                                    FALSE);

  swin = GTK_WIDGET(g_object_new(GTK_TYPE_SCROLLED_WINDOW,
                                 "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
                                 "hscrollbar-policy", GTK_POLICY_NEVER,
                                 NULL));

  hbox = GTK_WIDGET(g_object_new(GTK_TYPE_HBOX,
                                 "spacing", 12,
                                 NULL));
  vbox = GTK_WIDGET(g_object_new(GTK_TYPE_VBOX,
                                 NULL));

  label = gtk_label_new(_("memr_ia_close_applications_application_list"));

  /* Align to top-right */
  gtk_misc_set_alignment(GTK_MISC(label), 1, 0);

  gtk_container_add (GTK_CONTAINER(hbox), label);
  gtk_container_add (GTK_CONTAINER(hbox), vbox);

  /* Collect open applications */
  for (l = items; l; l = l->next)
    {
      const gchar *icon_name;
      GtkWidget *image;
      GtkWidget *label;
      HNWMWatchableApp *app;
      CADItem *item = (CADItem *) l->data;

      app = hn_wm_watched_window_get_app(item->win);
      
      label = gtk_label_new(_(hn_wm_watchable_app_get_name(app)));
      icon_name = hn_wm_watchable_app_get_icon_name(app);
      if (g_str_has_prefix(icon_name, "/"))
        {
          image = gtk_image_new_from_file(icon_name);
          gtk_widget_set_size_request(image,
                                      HILDON_ICON_SIZE_TOOLBAR,
                                      HILDON_ICON_SIZE_TOOLBAR);
        }
      else
        {
          image = gtk_image_new_from_icon_name(icon_name,
                                               HILDON_ICON_SIZE_TOOLBAR);
        }
      
      check = gtk_check_button_new();
      item->button = GTK_TOGGLE_BUTTON(check);

      box = GTK_WIDGET(g_object_new(GTK_TYPE_HBOX,
                                    "spacing", 12,
                                     NULL));

      gtk_container_add(GTK_CONTAINER(box), image);
      gtk_container_add(GTK_CONTAINER(box), label);
      gtk_container_add(GTK_CONTAINER(check), box);
      
      gtk_box_pack_start(GTK_BOX(vbox), check, FALSE, TRUE, 0);
      
      g_signal_connect(G_OBJECT(item->button), "toggled",
                        G_CALLBACK(item_toggled), items);
    }

  switch (action)
    {
      case CAD_ACTION_OPENING:
        label = gtk_label_new(_("memr_ia_close_applications_opening"));
        break;
      case CAD_ACTION_SWITCHING:
        label = gtk_label_new(_("memr_ia_close_applications_switching"));
        break;
      default:
        /* This isn't expected to ever happen */
        label = gtk_label_new("Unlikely internal error happened, but feel free "
                              "to close applications anyway");
        break;
    }

  gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);

  box = GTK_WIDGET(g_object_new(GTK_TYPE_VBOX,
                                "spacing", 12,
                                NULL));

  gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), gtk_hseparator_new (), FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), hbox, TRUE, TRUE, 0);

  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), box);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), swin);

  gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);

  /* ScrolledWindow won't request visibly pleasing amounts of space so we need
   * to do it for it. We ensure that there's vertical space for the whole
   * content or at least for 5 items (plus one for padding) at a time. We trust
   * the dialog to not allocate so much space that it would go offscreen.
   */
  gtk_widget_size_request (box, &req);
  i = req.height;
  gtk_widget_size_request(check, &req);
  gtk_widget_set_size_request(swin, -1, MIN(i, req.height*6));

  
  response = gtk_dialog_run (GTK_DIALOG(dialog));
  
  gtk_widget_hide(dialog);
  
  switch (response)
    {
      case GTK_RESPONSE_ACCEPT:
        
        kill_selected_items(items);
        retval = TRUE;
        
        break;
      default:
        
        retval = FALSE;
        break;  
     }

  gtk_widget_destroy(dialog);

  /* Cleanup */
  for (l = items; l; l = l->next)
    {
      if (l->data != NULL)
        {
          g_free(l->data);
        }
    }

  
  return retval;
}
