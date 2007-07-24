/* -*- mode:C; c-file-style:"gnu"; -*- */
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <sys/resource.h>
#include <string.h>
#include <fcntl.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <libgnomevfs/gnome-vfs.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#ifdef HAVE_LIBHILDON
#define ENABLE_UNSTABLE_API
#include <hildon/hildon-helper.h>
#undef ENABLE_UNSTABLE_API
#else
#include <hildon-widgets/hildon-finger.h>
#endif

#include <libhildondesktop/libhildonmenu.h>
#include <libhildondesktop/hildon-desktop-popup-menu.h>
#include <libhildondesktop/hildon-desktop-popup-window.h>
#include <libhildondesktop/hildon-desktop-panel-window.h>
#include <libhildondesktop/hildon-thumb-menu-item.h>
#include <libhildondesktop/hildon-desktop-toggle-button.h>
#include <libhildonwm/hd-wm.h>

#include "hd-applications-menu.h"
#include "hn-app-switcher.h"

#define HD_APPLICATIONS_MENU_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HD_TYPE_APPLICATIONS_MENU, HDApplicationsMenuPrivate))

G_DEFINE_TYPE (HDApplicationsMenu, hd_applications_menu, TASKNAVIGATOR_TYPE_ITEM);

#define HD_APPS_MENU_POPUP_WINDOW_NAME    "hildon-apps-menu"
#define HD_APPS_MENU_CAT_MENU_NAME        "hildon-apps-menu-pane1"
#define HD_APPS_MENU_CAT_MENU_ITEM_NAME   "hildon-apps-menu-pane1-item"
#define HD_APPS_MENU_CAT_BUTTON_UP_NAME   "hildon-apps-menu-pane1-button"
#define HD_APPS_MENU_CAT_BUTTON_DOWN_NAME "hildon-apps-menu-pane1-button"
#define HD_APPS_MENU_APP_MENU_NAME        "hildon-apps-menu-pane2"
#define HD_APPS_MENU_APP_MENU_ITEM_NAME   "hildon-apps-menu-pane2-item"
#define HD_APPS_MENU_APP_BUTTON_UP_NAME   "hildon-apps-menu-pane2-button"
#define HD_APPS_MENU_APP_BUTTON_DOWN_NAME "hildon-apps-menu-pane2-button"

#define OTHERS_MENU_ICON_NAME  "qgn_grid_tasknavigator_others"
#define OTHERS_MENU_ICON_SIZE  64

#define MENU_ITEM_DEFAULT_ICON        ""
#define MENU_ITEM_SUBMENU_ICON        "qgn_list_gene_fldr_cls"
#define MENU_ITEM_SUBMENU_ICON_DIMMED "qgn_list_gene_nonreadable_fldr"
#define MENU_ITEM_DEFAULT_APP_ICON    "qgn_list_gene_default_app"
#define MENU_ITEM_ICON_SIZE           26

#define MENU_ITEM_EMPTY_SUBMENU_STRING _("tana_li_of_noapps")

#define CATEGORY_SUB_ITEMS "category-sub-items"

#define WORKAREA_ATOM "_NET_WORKAREA"

struct _HDApplicationsMenuPrivate
{
  GtkWidget                *button;
  HildonDesktopPopupWindow *popup_window;
  HildonDesktopPopupMenu   *menu_categories;
  HildonDesktopPopupMenu   *menu_applications;
  guint                     collapse_id;
  GnomeVFSMonitorHandle    *system_dir_monitor;
  GnomeVFSMonitorHandle    *home_dir_monitor;
  GnomeVFSMonitorHandle    *desktop_dir_monitor;
  guint                     monitor_update_timeout;
  gboolean                  focus_applications;
};

static void hd_applications_menu_register_monitors (HDApplicationsMenu *button);
static void hd_applications_menu_create_menu (HDApplicationsMenu *button);
static void hd_applications_menu_button_toggled (GtkWidget *widget, HDApplicationsMenu *button);
static gboolean hd_applications_menu_button_key_press (GtkWidget* widget, GdkEventKey *event, HDApplicationsMenu *button);

static void
hd_applications_menu_finalize (GObject *gobject)
{
  HDApplicationsMenu *button = HD_APPLICATIONS_MENU (gobject);

  if (button->priv->collapse_id)
    g_source_remove (button->priv->collapse_id);

  if (button->priv->monitor_update_timeout)
    g_source_remove (button->priv->monitor_update_timeout);
  
  if (button->priv->system_dir_monitor)
    gnome_vfs_monitor_cancel (button->priv->system_dir_monitor);
  
  if (button->priv->home_dir_monitor)
    gnome_vfs_monitor_cancel (button->priv->home_dir_monitor);
  
  if (button->priv->desktop_dir_monitor)
    gnome_vfs_monitor_cancel (button->priv->desktop_dir_monitor);
  
  if (TASKNAVIGATOR_ITEM (button)->menu)
    gtk_widget_destroy (GTK_WIDGET (TASKNAVIGATOR_ITEM (button)->menu));
  
  G_OBJECT_CLASS (hd_applications_menu_parent_class)->finalize (gobject);
}

static void
hd_applications_menu_class_init (HDApplicationsMenuClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
 
  gobject_class->finalize = hd_applications_menu_finalize;
    
  g_type_class_add_private (gobject_class, sizeof (HDApplicationsMenuPrivate));
}

static void
hd_applications_menu_init (HDApplicationsMenu *button)
{
  HDWM *hdwm;
  GtkWidget *icon;
  
  HDApplicationsMenuPrivate *priv = HD_APPLICATIONS_MENU_GET_PRIVATE (button);

  button->priv = priv;

  button->priv->system_dir_monitor = NULL;
  button->priv->home_dir_monitor = NULL;
  button->priv->desktop_dir_monitor = NULL;
  
  priv->button = hildon_desktop_toggle_button_new ();

  icon = gtk_image_new_from_pixbuf (get_icon (OTHERS_MENU_ICON_NAME,
				 	      OTHERS_MENU_ICON_SIZE));

  if (icon)
  {
    gtk_container_add (GTK_CONTAINER (priv->button), icon);
  }

  gtk_widget_show_all (priv->button);

  hdwm = hd_wm_get_singleton ();
  hd_wm_set_all_menu_button (hdwm, G_OBJECT (priv->button));

  g_signal_connect (G_OBJECT (priv->button), 
		    "toggled",
      	            G_CALLBACK (hd_applications_menu_button_toggled), 
		    button);

  g_signal_connect (G_OBJECT (priv->button), 
		    "key-press-event",
 		     G_CALLBACK (hd_applications_menu_button_key_press), 
		     button);
  
  priv->monitor_update_timeout = 0;

  priv->focus_applications = TRUE;
  
  priv->popup_window = NULL;
  
  gtk_container_add (GTK_CONTAINER (button), priv->button);

  hd_applications_menu_register_monitors (button);
}

GtkWidget *
hd_applications_menu_new ()
{
  HDApplicationsMenu *button;

  button = g_object_new (HD_TYPE_APPLICATIONS_MENU, NULL);

  return GTK_WIDGET (button);
}

static gboolean
hd_applications_menu_has_focus (HildonDesktopPopupMenu *menu)
{
  GList *items, *i;

  items = hildon_desktop_popup_menu_get_children (menu);

  for (i = g_list_first (items); i; i = i->next)
  {
    GtkWidget *item = (GtkWidget *) i->data;

    if (gtk_widget_is_focus (item))
      return TRUE;
  }

  return FALSE;
}

static void
hd_applications_menu_popdown (GtkWidget *menu, HDApplicationsMenu *button)
{
  g_return_if_fail (button);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button->priv->button),
			        FALSE);

  button->priv->focus_applications = TRUE;
}

static gboolean
hd_applications_menu_key_press (GtkWidget *menu,
			        GdkEventKey *event,
			        HDApplicationsMenu *button)
{
  HDWM *hdwm = hd_wm_get_singleton ();

  if (event->keyval == GDK_Left    ||
      event->keyval == GDK_KP_Left ||
      event->keyval == GDK_Escape)
  {
    if (hd_applications_menu_has_focus (button->priv->menu_applications))
    {
      GtkMenuItem *selected_item = 
        hildon_desktop_popup_menu_get_selected_item (button->priv->menu_applications);

      hildon_desktop_popup_menu_deselect_item (button->priv->menu_applications,
		      			       selected_item);
      
      selected_item = 
        hildon_desktop_popup_menu_get_selected_item (button->priv->menu_categories);

      gtk_widget_grab_focus (GTK_WIDGET (selected_item));
      
      return TRUE;
    }
  
    hildon_desktop_popup_window_popdown (button->priv->popup_window);

    if (event->keyval == GDK_Escape)
    {
      /* pass focus to the last active application */
      hd_wm_focus_active_window (hdwm);
    }
    else
    {
      GtkWidget *window = gtk_widget_get_toplevel (button->priv->button);
      
      gtk_widget_grab_focus (GTK_WIDGET (button->priv->button));
      
      g_object_set (window, "can-focus", TRUE, NULL);
      
      hd_wm_activate_window (HD_TN_ACTIVATE_KEY_FOCUS, window->window);
    }
    
    return TRUE;
  }
  else if (event->keyval == GDK_Return    ||
	   event->keyval == GDK_KP_Enter  ||
	   event->keyval == GDK_ISO_Enter ||
	   event->keyval == GDK_Right     ||
           event->keyval == GDK_KP_Right)
  {
    if (hd_applications_menu_has_focus (button->priv->menu_categories))
    {
      GtkMenuItem *selected_item;

      hildon_desktop_popup_menu_select_first_item (button->priv->menu_applications);

      selected_item = 
        hildon_desktop_popup_menu_get_selected_item (button->priv->menu_applications);

      if (GTK_WIDGET_IS_SENSITIVE (selected_item))
        gtk_widget_grab_focus (GTK_WIDGET (selected_item));
      else
	hildon_desktop_popup_menu_deselect_item (button->priv->menu_applications,
			                         selected_item);
      
      return TRUE;
    }
    else if (hd_applications_menu_has_focus (button->priv->menu_applications))
    {
      return gtk_widget_event (GTK_WIDGET (button->priv->menu_applications), (GdkEvent *) event);
    }
  }
  else
  {
    if (hd_applications_menu_has_focus (button->priv->menu_categories))
    {
      gtk_widget_event (GTK_WIDGET (button->priv->menu_categories), (GdkEvent *) event);

      if (event->keyval == GDK_Up     ||
          event->keyval == GDK_KP_Up  ||
          event->keyval == GDK_Down   ||
          event->keyval == GDK_KP_Down)
      {
        GtkMenuItem *selected_item;

	selected_item = 
          hildon_desktop_popup_menu_get_selected_item (button->priv->menu_categories);

	gtk_menu_item_activate (selected_item);
      }
    }
    else if (hd_applications_menu_has_focus (button->priv->menu_applications))
    {
      return gtk_widget_event (GTK_WIDGET (button->priv->menu_applications), (GdkEvent *) event);
    }
  }
  
  return FALSE;
}

static gboolean 
hd_applications_menu_button_release_category (GtkMenuItem *item, 
					    GdkEventButton *event,
					    HDApplicationsMenu *button)
{
  button->priv->focus_applications = TRUE;

  gtk_menu_item_activate (item);

  return TRUE;
}

static void
hd_applications_menu_activate_category (GtkMenuItem *item, HDApplicationsMenu *button)
{
  GList *sub_items, *i;

  sub_items = hildon_desktop_popup_menu_get_children 
	  (button->priv->menu_applications);
  
  for (i = g_list_first (sub_items); i; i = i->next)
  {
    GtkMenuItem *child = (GtkMenuItem *) i->data;

    g_object_ref (child);

    hildon_desktop_popup_menu_remove_item
      (button->priv->menu_applications, child);
  }

  g_list_free (sub_items);

  sub_items = (GList *) g_object_get_data (G_OBJECT (item),
		                           CATEGORY_SUB_ITEMS);

  for (i = g_list_first (sub_items); i; i = i->next)
  {
    GtkMenuItem *child = (GtkMenuItem *) i->data;

    hildon_desktop_popup_menu_add_item
      (button->priv->menu_applications, child);
  }

  if (button->priv->focus_applications &&
      GTK_WIDGET_IS_SENSITIVE (sub_items->data))
  {
    hildon_desktop_popup_menu_select_first_item (button->priv->menu_applications);
    gtk_widget_grab_focus (GTK_WIDGET (sub_items->data));
  }
  else
  {
    gtk_widget_grab_focus (GTK_WIDGET (item));
  }

  button->priv->focus_applications = FALSE;
}

static void
hd_applications_menu_activate_app (GtkMenuItem *item, HDApplicationsMenu *button)
{
  gchar *service_field;
  gchar *exec_field;
  gchar *program = NULL;
  GError *error = NULL;
  
  g_return_if_fail (button);

  if ((service_field = g_object_get_data (G_OBJECT (item),
      				          DESKTOP_ENTRY_SERVICE_FIELD)))
  {
    /* Launch the app or if it's already running move it to the top */
    hd_wm_top_service (service_field);
  }
  else
  {
    /* No way to track hibernation (if any), so call the dialog
       unconditionally when in lowmem state */
    if (hd_wm_is_lowmem_situation ())
    {
      /*
      if (!tn_close_application_dialog (CAD_ACTION_OPENING))
      {
        return;
      }
      */
    }
    
    exec_field = g_object_get_data (G_OBJECT (item),
    			            DESKTOP_ENTRY_EXEC_FIELD);

    if (exec_field)
    {
      gchar *space = strchr(exec_field, ' ');

      if (space)
      {
        gchar *cmd = g_strdup (exec_field);
        cmd[space - exec_field] = 0;

        gchar *exc = g_find_program_in_path (cmd);

        program = g_strconcat (exc, space, NULL);

        g_free (exc);
        g_free (cmd);
      }
      else
      {
        program = g_find_program_in_path (exec_field);
      }
    }
    
    if (program)
    {
      gint argc;
      gchar **argv;
      GPid child_pid;
      
      if (g_shell_parse_argv (program, &argc, &argv, &error))
      {
        g_spawn_async (
    	      /* Child's current working directory,
    	         or NULL to inherit parent's */
    	      NULL,
    	      /* Child's argument vector. [0] is the path of
    	         the program to execute */
    	      argv,
    	      /* Child's environment, or NULL to inherit
    	         parent's */
    	      NULL,
    	      /* Flags from GSpawnFlags */
    	      0,
    	      /* Function to run in the child just before
    	         exec() */
    	      NULL,
    	      /* User data for child_setup */
    	      NULL,
    	      /* Return location for child process ID or NULL */
    	      &child_pid,
    	      /* Return location for error */
    	      &error);
      }
      
      if (error)
      {
        g_warning ("Others_menu_activate_app: failed to execute %s: %s.",
                   exec_field, error->message);
    
        g_clear_error (&error);
      }
      else
      {
        int priority;
        errno = 0;
        gchar *oom_filename;
        int fd;

        /* If the child process inherited desktop's high priority,
         * give child default priority */
        priority = getpriority (PRIO_PROCESS, child_pid);
    
        if (!errno && priority < 0)
        {
          setpriority (PRIO_PROCESS, child_pid, 0);
        }

        /* Unprotect from OOM */
        oom_filename = g_strdup_printf ("/proc/%i/oom_adj",
                                        child_pid);
        fd = open (oom_filename, O_WRONLY);
        g_free (oom_filename);

        if (fd >= 0)
          {
            write (fd, "0", sizeof (char));
            close (fd);
          }

      }
    }
    else
    {
      /* We don't have the service name and we don't
       * have a path to the executable so it would be
       * quite difficult to launch the app.
       */
      g_warning ("hd_applications_menu_activate_app: "
                 "both service name and binary path missing. "
                 "Unable to launch.");
    }
  }
  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button->priv->button), 
		                FALSE);

  g_free(program);
}

static void
hd_applications_menu_free_menu_items (GList *menu_items)
{
  g_list_foreach (menu_items, (GFunc) gtk_widget_destroy, NULL);
  g_list_free (menu_items);
}

static GList *
hd_applications_menu_get_items (HDApplicationsMenu *button, 
		                GtkTreeModel *model,
		                GtkTreeIter *iter,
			        gint level)
{
  GList *menu_items = NULL;
  GList *sub_items = NULL;
  GtkWidget *menu_item = NULL;

  gchar *item_name = NULL;
  gchar *item_comment = NULL;
  GdkPixbuf *item_icon  = NULL;
  gchar *item_exec = NULL;
  gchar *item_service = NULL;
  gchar *item_desktop_id = NULL;
  gchar *item_text_domain = NULL;

  if (model == NULL)
  {
    model = get_menu_contents ();
    
    if (!gtk_tree_model_get_iter_first (model, iter))
    {
      g_object_unref (G_OBJECT (model));

      return NULL;
    }
  }
  else
  {
    g_object_ref (model);
  }

  do  
  {
    sub_items = NULL; 
    menu_item = NULL;
    item_name = NULL;
    item_icon = NULL;
    item_exec = NULL;
    item_service = NULL;
    item_desktop_id = NULL;
    item_text_domain = NULL;
     
    gtk_tree_model_get (model, iter,
		        TREE_MODEL_LOCALIZED_NAME, &item_name,
		        TREE_MODEL_ICON, &item_icon,
		        TREE_MODEL_EXEC, &item_exec,
		        TREE_MODEL_SERVICE, &item_service,
		        TREE_MODEL_DESKTOP_ID, &item_desktop_id,
		        TREE_MODEL_COMMENT, &item_comment,
		        TREE_MODEL_TEXT_DOMAIN, &item_text_domain,
		        -1);

    /* If the item has children. */
    if (!item_desktop_id || strlen (item_desktop_id) == 0)
    {
      GtkTreeIter child_iter;
      GtkWidget *hbox, *label;

#if 0
      menu_item = gtk_image_menu_item_new_with_label (
                                (item_text_domain && *item_text_domain) ?
                                dgettext(item_text_domain, item_name) : _(item_name));
#else
      menu_item = gtk_menu_item_new ();
      
      hbox = gtk_hbox_new (FALSE, 0);
      
      label = gtk_label_new (item_name);
      
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      
      gtk_box_pack_start (GTK_BOX (hbox),
      		          label,
      		          TRUE, 
      		          TRUE, 
      		          15);
      
      gtk_container_add (GTK_CONTAINER (menu_item), hbox);
      
      gtk_widget_show_all (hbox);
#endif

      GTK_WIDGET_SET_FLAGS (menu_item, GTK_CAN_FOCUS);

      gtk_widget_set_name (GTK_WIDGET (menu_item), HD_APPS_MENU_CAT_MENU_ITEM_NAME);
      
      if (gtk_tree_model_iter_children (model, &child_iter, iter))
      {
        sub_items = hd_applications_menu_get_items (button, model, &child_iter, level + 1);
      }
      else
      {
        /* Add "no applications" item */
        GtkWidget *no_app_item;

        no_app_item = gtk_image_menu_item_new_with_label (MENU_ITEM_EMPTY_SUBMENU_STRING);

        gtk_widget_set_sensitive (no_app_item, FALSE);
	
        sub_items = g_list_prepend (sub_items, no_app_item);
      }

      g_object_set_data_full (G_OBJECT (menu_item),
      		              CATEGORY_SUB_ITEMS,
      		              sub_items, 
			      (GDestroyNotify) hd_applications_menu_free_menu_items);

      g_signal_connect (G_OBJECT (menu_item), 
		        "button-release-event",
      		        G_CALLBACK (hd_applications_menu_button_release_category),
      		        button);

      g_signal_connect (G_OBJECT (menu_item), 
		        "activate",
      		        G_CALLBACK (hd_applications_menu_activate_category),
      		        button);
    }
    else if (g_str_equal (item_desktop_id, SEPARATOR_STRING))
    {
      /* Separator */
      menu_item = gtk_separator_menu_item_new ();
    }
    else if (level > 0)
    {
#if 0
      /* Application */
      menu_item = gtk_image_menu_item_new_with_label (
                                (item_text_domain && *item_text_domain) ?
                                dgettext(item_text_domain, item_name) : _(item_name));
#else
      menu_item = gtk_menu_item_new ();
#endif
      
      GTK_WIDGET_SET_FLAGS (menu_item, GTK_CAN_FOCUS);

      gtk_widget_set_name (GTK_WIDGET (menu_item), HD_APPS_MENU_APP_MENU_ITEM_NAME);

      if (!item_icon)
      {
        item_icon = get_icon (MENU_ITEM_DEFAULT_APP_ICON,
      		              MENU_ITEM_ICON_SIZE);
      }

      if (item_icon)
      {
#if 0
      /* FIXME: disable icons on application while we don't fix the 
       * crash related to toggle area sizing in the menu items. */
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
            		               gtk_image_new_from_pixbuf (item_icon));
#else
        GtkWidget *hbox, *image, *label;
	
	hbox = gtk_hbox_new (FALSE, 0);

	image = gtk_image_new_from_pixbuf (item_icon);
	
	gtk_box_pack_start (GTK_BOX (hbox),
			    image, 
			    FALSE, 
			    FALSE, 
			    15);

        label = gtk_label_new ((item_text_domain && *item_text_domain) ?
			        dgettext(item_text_domain, item_name) : _(item_name)), 

        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	
	gtk_box_pack_start (GTK_BOX (hbox),
			    label,
			    TRUE, 
			    TRUE, 
			    0);

	gtk_container_add (GTK_CONTAINER (menu_item), hbox);

	gtk_widget_show_all (hbox);
#endif
      }

      g_object_set_data_full (G_OBJECT (menu_item),
      		              DESKTOP_ENTRY_EXEC_FIELD,
      		              g_strdup (item_exec), 
			      g_free);

      g_object_set_data_full (G_OBJECT(menu_item),
      		              DESKTOP_ENTRY_SERVICE_FIELD,
      		              g_strdup (item_service), 
			      g_free);

      g_signal_connect (G_OBJECT (menu_item), 
		        "activate",
      		        G_CALLBACK (hd_applications_menu_activate_app),
      		        button);
    }

    if (menu_item)
    {
      menu_items = g_list_prepend (menu_items, menu_item);
    }
      
    g_free (item_name);
    
    if (item_icon)
      g_object_unref (G_OBJECT (item_icon));
    
    g_free (item_exec);
    g_free (item_service);
    g_free (item_desktop_id);
    g_free (item_comment);
    g_free (item_text_domain);

  } while (gtk_tree_model_iter_next (model, iter));

  g_object_unref (model);

  return menu_items;
}

static void
hd_applications_menu_populate (HDApplicationsMenu *button)
{
  GList *menu_items, *i;
  GtkTreeIter iter;
  
  menu_items = hd_applications_menu_get_items (button, NULL, &iter, 0);

  for (i = g_list_first (menu_items); i; i = i->next)
  {
    GtkMenuItem *menu_item = (GtkMenuItem *) i->data;

    hildon_desktop_popup_menu_add_item
      (button->priv->menu_categories, menu_item);
  }

  g_list_free (menu_items);
}

static void
hd_applications_menu_create_menu (HDApplicationsMenu *button)
{
  HildonDesktopPopupWindow *popup_window;
  const GtkWidget *button_up, *button_down;
  GtkWidget *box, *alignment, *arrow;
  
  g_return_if_fail (button);

  box = gtk_hbox_new (TRUE, 0);
  
  popup_window =
    HILDON_DESKTOP_POPUP_WINDOW 
      (hildon_desktop_popup_window_new (0, 
					GTK_ORIENTATION_HORIZONTAL,
					HD_POPUP_WINDOW_DIRECTION_RIGHT_BOTTOM));

  alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 4, 4, 4, 4);
  gtk_widget_show (alignment);
  
  gtk_widget_set_name (GTK_WIDGET (popup_window), HD_APPS_MENU_POPUP_WINDOW_NAME);

  gtk_widget_set_size_request (GTK_WIDGET (popup_window), 650, 480);

  /* We don't attach the widget because if we do it, we cannot be on top of 
   * virtual keyboard. Anyway it should be transient to button->priv->button
   */
  
  hildon_desktop_popup_window_attach_widget (popup_window, NULL);
  
  button->priv->menu_categories =
    HILDON_DESKTOP_POPUP_MENU (g_object_new (HILDON_DESKTOP_TYPE_POPUP_MENU,
		  		    	     "item-height", 68,
		  		    	     "resize-parent", FALSE,
				    	     NULL));

  gtk_widget_show (GTK_WIDGET (button->priv->menu_categories));

  gtk_widget_set_name (GTK_WIDGET (button->priv->menu_categories), 
  		       HD_APPS_MENU_CAT_MENU_NAME);

  gtk_box_pack_start (GTK_BOX (box),
		      GTK_WIDGET (button->priv->menu_categories),
		      TRUE,
		      TRUE,
		      0); 
  
  button_up =  
    hildon_desktop_popup_menu_get_scroll_button_up (button->priv->menu_categories);

  gtk_widget_set_name (GTK_WIDGET (button_up), HD_APPS_MENU_CAT_BUTTON_UP_NAME);

  arrow = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (button_up), arrow);
  gtk_widget_show (arrow);
  
  button_down =
    hildon_desktop_popup_menu_get_scroll_button_down (button->priv->menu_categories);

  gtk_widget_set_name (GTK_WIDGET (button_down), HD_APPS_MENU_CAT_BUTTON_DOWN_NAME);

  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (button_down), arrow);
  gtk_widget_show (arrow);

  button->priv->menu_applications =
    HILDON_DESKTOP_POPUP_MENU (g_object_new (HILDON_DESKTOP_TYPE_POPUP_MENU,
		  		    	     "item-height", 68,
		  		    	     "resize-parent", FALSE,
				    	     NULL));

  gtk_widget_show (GTK_WIDGET (button->priv->menu_applications));

  gtk_widget_set_name (GTK_WIDGET (button->priv->menu_applications), 
  		       HD_APPS_MENU_APP_MENU_NAME);

  gtk_box_pack_start (GTK_BOX (box),
		      GTK_WIDGET (button->priv->menu_applications),
		      TRUE,
		      TRUE,
		      0); 

  button_up = 
    hildon_desktop_popup_menu_get_scroll_button_up (button->priv->menu_applications);

  gtk_widget_set_name (GTK_WIDGET (button_up), HD_APPS_MENU_APP_BUTTON_UP_NAME);

  arrow = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (button_up), arrow);
  gtk_widget_show (arrow);
  
  button_down =
    hildon_desktop_popup_menu_get_scroll_button_down (button->priv->menu_applications);

  gtk_widget_set_name (GTK_WIDGET (button_down), HD_APPS_MENU_APP_BUTTON_DOWN_NAME);

  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (button_down), arrow);
  gtk_widget_show (arrow);

  g_signal_connect (G_OBJECT (popup_window), 
		    "key-press-event",
		    G_CALLBACK (hd_applications_menu_key_press),
		    button);
  
  g_signal_connect (G_OBJECT (popup_window), 
		    "popdown-window",
		    G_CALLBACK (hd_applications_menu_popdown),
		    button);
  
  gtk_widget_show (box);

  gtk_container_add (GTK_CONTAINER (alignment), box);
  gtk_container_add (GTK_CONTAINER (popup_window), alignment);

  if (button->priv->popup_window)
  {
    /* Destroy the previous version of the menu */
    gtk_widget_destroy (GTK_WIDGET (button->priv->popup_window));
  }

  button->priv->popup_window = popup_window;

  /* Now populate the menu */
  hd_applications_menu_populate (button);
}

static void
hd_applications_menu_get_workarea (GtkAllocation *allocation)
{
  unsigned long n;
  unsigned long extra;
  int format;
  int status;
  Atom property = XInternAtom (GDK_DISPLAY (), WORKAREA_ATOM, FALSE);
  Atom realType;
  
  /* This is needed to get rid of the punned type-pointer 
     breaks strict aliasing warning*/
  union
  {
    unsigned char *char_value;
    int *int_value;
  } value;
    
  status = XGetWindowProperty (GDK_DISPLAY (), 
			       GDK_ROOT_WINDOW (), 
			       property, 
			       0L, 
			       4L,
			       0, 
			       XA_CARDINAL, 
			       &realType, 
			       &format,
			       &n, 
			       &extra, 
			       (unsigned char **) &value.char_value);
    
  if (status == Success &&
      realType == XA_CARDINAL &&
      format == 32 && 
      n == 4  &&
      value.char_value != NULL)
  {
    allocation->x = value.int_value[0];
    allocation->y = value.int_value[1];
    allocation->width = value.int_value[2];
    allocation->height = value.int_value[3];
  }
  else
  {
    allocation->x = 0;
    allocation->y = 0;
    allocation->width = 0;
    allocation->height = 0;
  }
    
  if (value.char_value) 
  {
    XFree(value.char_value);  
  }
}

static void
hd_applications_menu_get_menu_position (GtkMenu *menu,
                                    gint *x,
                                    gint *y,
                                    GtkWidget *button)
{
  GtkRequisition  req;
  GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(menu));
  int menu_height = 0;
  int main_height = 0;
  GtkAllocation workarea = { 0, 0, 0, 0 };
  GtkWidget *top_level;
  HildonDesktopPanelWindowOrientation orientation = 
      HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT;
    
  hd_applications_menu_get_workarea (&workarea);
  
  top_level = gtk_widget_get_toplevel (button);

  if (HILDON_DESKTOP_IS_PANEL_WINDOW (top_level))
  {
    g_object_get (top_level, "orientation", &orientation, NULL);
  }
    
  gtk_widget_size_request (GTK_WIDGET (menu), &req);

  menu_height = req.height;
  main_height = gdk_screen_get_height (screen);

  switch (orientation)
  {
    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT:
      *x =  workarea.x;

      if (main_height - button->allocation.y < menu_height)
        *y = MAX(0, ((main_height - menu_height) / 2));
      else
        *y = button->allocation.y;
      
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT:
      *x =  workarea.x + workarea.width - req.width;

      if (main_height - button->allocation.y < menu_height)
        *y = MAX(0, ((main_height - menu_height) / 2));
      else
        *y = button->allocation.y;
      
      break;
        
    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP:
      *x = button->allocation.x;
      *y = workarea.y;
      break;
        
    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM:
      *x = button->allocation.x;
      *y = workarea.y + workarea.height - req.height;
      break;

    default:
      g_assert_not_reached ();
  }
}

static void
hd_applications_menu_show (HDApplicationsMenu * button)
{
  GList *menu_items;
  
  g_return_if_fail (button);
  
  if (!button->priv->popup_window)
    hd_applications_menu_create_menu (button);

  hildon_desktop_popup_window_popup (button->priv->popup_window,
		                     (HDPopupWindowPositionFunc) hd_applications_menu_get_menu_position,
		                     button,
		                     GDK_CURRENT_TIME);

  hildon_desktop_popup_menu_select_first_item (button->priv->menu_categories);

  menu_items = hildon_desktop_popup_menu_get_children (button->priv->menu_categories);

  if (menu_items != NULL)
  {
    gtk_menu_item_activate (GTK_MENU_ITEM (menu_items->data));
  }

  g_list_free (menu_items);
}

static void
hd_applications_menu_button_toggled (GtkWidget *widget, HDApplicationsMenu *button)
{
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
  {
    hildon_desktop_popup_window_popdown (button->priv->popup_window);
    return;
  } 

  hd_applications_menu_show (button);
}

static gboolean
hd_applications_menu_button_key_press (GtkWidget *widget, 
		                       GdkEventKey *event,
			               HDApplicationsMenu *button)
{
  g_return_val_if_fail (button, FALSE);

  if (event->keyval == GDK_Right ||
      event->keyval == GDK_KP_Enter)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button->priv->button), TRUE); 

    return TRUE;
  }
  else if (event->keyval == GDK_Left || 
           event->keyval == GDK_KP_Left)
  {
    gtk_widget_grab_focus (widget);
	  
    GdkWindow *w = 
      gtk_widget_get_parent_window (GTK_WIDGET (button));

    hd_wm_activate_window (HD_TN_ACTIVATE_KEY_FOCUS, w);

    return TRUE;
  }

  return FALSE;
}

/* Dnotify dispatches large number of events when a file is modified --
   we use a 1s timeout to wait for the flurry to pass, and then do our stuff */
static gboolean
hd_applications_menu_changed (HDApplicationsMenu *button)
{
  g_return_val_if_fail (button, FALSE);

  button->priv->monitor_update_timeout = 0;

  hd_applications_menu_create_menu (button);
  
  return FALSE;
}

static void
hd_applications_menu_dir_changed (GnomeVFSMonitorHandle *handle,
                              const gchar *monitor_uri,
                              const gchar *info_uri,
                              GnomeVFSMonitorEventType event_type,
                              HDApplicationsMenu *button)
{
  if (!button->priv->monitor_update_timeout)
  {
    button->priv->monitor_update_timeout =
            g_timeout_add (1000,
            	           (GSourceFunc) hd_applications_menu_changed,
            	           button);
  }
}

void
hd_applications_menu_register_monitors (HDApplicationsMenu * button)
{
  gchar *dir;
  gchar	*file = NULL;
  gchar *conf_file;

  conf_file = g_build_filename (g_get_home_dir (), USER_MENU_DIR, MENU_FILE, NULL);

  /* We copy SYSTEMWIDE_MENU_FILE to always track the changes */
  if (!g_file_test (conf_file, G_FILE_TEST_EXISTS))
    if (g_file_get_contents (HD_DESKTOP_MENU_PATH "/" MENU_FILE, &file, NULL, NULL))
    {
      g_file_set_contents (conf_file, file, -1, NULL);
    }
  
  g_free (file);
  
  if (gnome_vfs_monitor_add (&button->priv->system_dir_monitor, 
                             HD_DESKTOP_MENU_PATH,
                             GNOME_VFS_MONITOR_DIRECTORY,
                             (GnomeVFSMonitorCallback) hd_applications_menu_dir_changed,
                             button) != GNOME_VFS_OK)
  {
    g_warning ("Others_menu_initialize_menu: "
      	       "failed setting monitor callback "
      	       "for systemwide menu conf." );
  }
  
  /* Have to get the directory from the path because the USER_MENU_FILE
     define might contain directory (it does, in fact). */
  dir = g_path_get_dirname (conf_file);
  
  g_mkdir (dir, 0755);

  if (dir && *dir)
  {
    if (gnome_vfs_monitor_add (&button->priv->home_dir_monitor, 
                               dir,
                               GNOME_VFS_MONITOR_DIRECTORY,
                               (GnomeVFSMonitorCallback) hd_applications_menu_dir_changed,
                               button) != GNOME_VFS_OK)
    {
      g_warning ("Others_menu_initialize_menu: "
    	         "failed setting monitor callback "
    	         "for user specific menu conf." );
    }
  }
  else
  {
    g_warning ("Others_menu_initialize_menu: "
  	       "failed to create directory '%s'", dir);
  }

  g_free (dir);
      
  g_free (conf_file);

  /* Monitor the .desktop directories, so we can regenerate the menu
   * when a new application is installed */
  if (g_file_test (HD_DESKTOP_ENTRY_PATH, G_FILE_TEST_EXISTS) &&
      gnome_vfs_monitor_add (&button->priv->desktop_dir_monitor, 
                             HD_DESKTOP_ENTRY_PATH,
                             GNOME_VFS_MONITOR_DIRECTORY,
                             (GnomeVFSMonitorCallback) hd_applications_menu_dir_changed,
                             button) != GNOME_VFS_OK)
  {
    g_warning ("Others_menu_initialize_menu: "
      	       "failed setting monitor callback "
      	       "for .desktop directory." );
  }

  /* Monitor the hildon .desktop directories, so we can regenerate the 
   * menu when a new application is installed. Do this only if the default
   * directory is the standard one ($datadir/applications) */
  if (g_file_test (HD_DESKTOP_ENTRY_PATH "/hildon", G_FILE_TEST_EXISTS) &&
      gnome_vfs_monitor_add (&button->priv->desktop_dir_monitor, 
                             HD_DESKTOP_ENTRY_PATH "/hildon",
                             GNOME_VFS_MONITOR_DIRECTORY,
                             (GnomeVFSMonitorCallback) hd_applications_menu_dir_changed,
                             button) != GNOME_VFS_OK)
  {
    g_warning ("Others_menu_initialize_menu: "
      	       "failed setting monitor callback "
      	       "for .desktop directory." );
  }
}

void
hd_applications_menu_close_menu (HDApplicationsMenu *button)
{
  g_return_if_fail (button && HD_IS_APPLICATIONS_MENU (button));
  g_return_if_fail (button->priv->popup_window != NULL);

  hildon_desktop_popup_window_popdown (button->priv->popup_window);
}
