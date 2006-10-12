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
#include <sys/types.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <signal.h>
#include <X11/Xatom.h>

/* Log macro includes */
#include <osso-log.h>


/* L10n comes from the ke-recv domain */
#include <libintl.h>

#define _KE(a) dgettext("ke-recv", a)
#define HN_CAD_TITLE            _KE("memr_ti_close_applications")
#define HN_CAD_LABEL_OPENING    _KE("memr_ia_close_applications_opening")
#define HN_CAD_LABEL_SWITCHING  _KE("memr_ia_close_applications_switching")
#define HN_CAD_LABEL_APP_LIST   _KE("memr_ia_close_applications_application_list")
#define HN_CAD_OK               _KE("memr_bd_close_applications_ok")
#define HN_CAD_CANCEL           _KE("memr_bd_close_applications_cancel")

#define AS_MENU_DEFAULT_APP_ICON "qgn_list_gene_default_app"

/* Hildon headers */
#include <hildon-widgets/hildon-defines.h>

#include "hn-wm.h"
#include "hn-wm-watched-window.h"
#include "hn-wm-watchable-app.h"
#include "hildon-navigator.h"
#include "hn-app-switcher.h"
#include "hn-entry-info.h"

/* Include our own header */
#include "close-application-dialog.h"

#define ICON_SIZE        26



typedef struct _CADItem CADItem;
struct _CADItem
{
  HNWMWatchedWindow* win;
  GtkToggleButton *button;
  gint vmdata;
  guint pid;
};


void item_toggled (GtkToggleButton *button,
                   gpointer user_data);
void kill_selected_items (GList *items);

gint compare_items (gconstpointer a, gconstpointer b);


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
      if (gtk_toggle_button_get_active(item->button)
          && item->win != NULL)
        {
          hn_wm_watched_window_attempt_signal_kill (item->win, SIGTERM, TRUE);
        }
    }
}

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
  
  items = NULL;
  check = NULL;
  req.height = 0;

  /* Get the open applications 
     FIXME: Is there a better way to get them?
   */
  for (l = hn_app_switcher_get_entries(hn_wm_get_app_switcher()); l; l = l->next)
    {
      guint32 *pid_result = NULL;
      pid_t pid;
      GList *p;
      gboolean pid_exists;
      HNWMWatchableApp *app;
      HNWMWatchedWindow* win;
      GdkAtom a;
      HNEntryInfo *info;

      info = (HNEntryInfo*)l->data;
      win = hn_entry_info_get_window(info);

      if (win == NULL)
        {
          gchar * title = hn_entry_info_get_title(info);
          ULOG_WARN("No win found for item %s",title);
          g_free(title);
          continue;
        }

      app = hn_wm_watched_window_get_app(win);
      
      if (app == NULL)
        {
          gchar * title = hn_entry_info_get_title(info);
          ULOG_WARN("No app found for item %s",title);
          g_free(title);
          continue;
        }

      /* Skip hibernating apps */
      if (hn_wm_watchable_app_is_hibernating(app))
        {
          continue;
        }

      /* FIXME: This is interned here since we don't want to rely on extern
       * global structures (which are slated for removal anyway).
       */
      a = gdk_atom_intern("_NET_WM_PID", FALSE);
      pid_result = hn_wm_util_get_win_prop_data_and_validate (
                hn_wm_watched_window_get_x_win(win),
							  gdk_x11_atom_to_xatom (a),
							  XA_CARDINAL,
							  32,
							  0,
							  NULL);
      if (pid_result == NULL)
        {
          gchar * title = hn_entry_info_get_title(info);
          ULOG_WARN("No pid found for item %s",title);
          g_free(title);
          continue;
        }

      /*
        sanity check -- pid_t can be less than 32 bits, and has to allow
        for a sign
      */
      if(sizeof(pid_t) <= sizeof(guint32) &&
         pid_result[0] > (1 << (sizeof(pid_t)*8 - 1)))
        {
          /*
            something is very wrong; the PID we were given does not fit into
            pid_t
          */
          g_warning("PID (%d) out of bounds", pid_result[0]);
          XFree(pid_result);
          continue;
        }

      
      pid = pid_result[0];

      /* If we already have the pid, skip item creation */
      pid_exists = FALSE;
      for (p = items; p != NULL; p = p->next)
        {
          CADItem *item = (CADItem *) p->data;
          
          if (item->pid == pid)
            {
              pid_exists = TRUE;
              break;
            }
          
        }

      if (pid_exists)
        {
          continue;
        }

      ULOG_DEBUG ("%s(): %s is %s, Pid:%i, VmData: %ikB\n", __FUNCTION__,
        hn_wm_watchable_app_get_name(app),
        hn_wm_watchable_app_is_hibernating(app) ? "hibernating" : "awake",
        pid,
        hn_wm_get_vmdata_for_pid(pid));

      item = g_new0(CADItem, 1);
      
      item->win = win;
      item->vmdata = hn_wm_get_vmdata_for_pid(pid);
      item->pid = pid;
      
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
  dialog = gtk_dialog_new_with_buttons (HN_CAD_TITLE,
                                        NULL,
                                        GTK_DIALOG_MODAL
                                        | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        HN_CAD_OK,
                                        GTK_RESPONSE_ACCEPT,
                                        HN_CAD_CANCEL,
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

  label = gtk_label_new(HN_CAD_LABEL_APP_LIST);

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
      GdkPixbuf *icon = NULL;
      GtkIconTheme *icon_theme;
      HNWMWatchableApp *app;
      CADItem *item = (CADItem *) l->data;

      if (item->win == NULL)
        {
          continue;
        }

      app = hn_wm_watched_window_get_app(item->win);
      
      if (app == NULL)
        {
          continue;
        }
      
      label = gtk_label_new(_(hn_wm_watchable_app_get_name(app)));
      icon_name = hn_wm_watchable_app_get_icon_name(app);
      icon_theme = gtk_icon_theme_get_default ();
      if (icon_name)
          icon = gtk_icon_theme_load_icon (icon_theme,
                                           icon_name,
                                           ICON_SIZE,
                                           GTK_ICON_LOOKUP_NO_SVG,
                                           NULL);


      if (!icon)
          icon = gtk_icon_theme_load_icon (icon_theme,
                                           AS_MENU_DEFAULT_APP_ICON,
                                           ICON_SIZE,
                                           GTK_ICON_LOOKUP_NO_SVG,
                                           NULL);
      

      image = gtk_image_new_from_pixbuf (icon);
      if (icon)
        gdk_pixbuf_unref (icon);
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
        label = gtk_label_new(HN_CAD_LABEL_OPENING);
        break;
      case CAD_ACTION_SWITCHING:
        label = gtk_label_new(HN_CAD_LABEL_SWITCHING);
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
