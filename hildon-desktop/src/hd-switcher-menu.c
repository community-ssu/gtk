 /*
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
 *          Lucas Rocha <lucas.rocha@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
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

#include "hd-config.h"

/* Hildon includes */
#include "hd-switcher-menu.h"
#include "hd-switcher-menu-item.h"
#include "hn-app-sound.h"
#include "hn-app-pixbuf-anim-blinker.h"

#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <dbus/dbus.h>

#ifdef HAVE_LIBHILDON
#include <hildon/hildon-note.h>
#else
#include <hildon-widgets/hildon-note.h>
#endif

#ifdef HAVE_MCE
#include <mce/dbus-names.h>
#endif

#include <gtk/gtkimage.h>
#include <libhildondesktop/hildon-desktop-popup-window.h>
#include <libhildondesktop/hildon-desktop-popup-menu.h>
#include <libhildondesktop/hildon-desktop-toggle-button.h>
#include <libhildondesktop/hildon-desktop-panel-window-dialog.h>

/* Menu item strings */
#define AS_HOME_ITEM 		_("tana_fi_home")
#define AS_HOME_ITEM_ICON 	"qgn_list_home"

#define AS_CLEAR_ITEM          _("tana_fi_clear_received_events")
#define AS_CLEAR_ITEM_ICON     "qgn_list_home"
#define AS_CLEAR_CONFIRM       _("tana_nc_clear_events")
#define AS_CLEAR_CONFIRM_ICON  "qgn_note_confirm" 

#define AS_ITEMS_LIMIT  7

/* Defined workarea atom */
#define WORKAREA_ATOM "_NET_WORKAREA"

#define AS_MENU_BUTTON_ICON       "qgn_list_tasknavigator_appswitcher"
#define AS_MENU_DEFAULT_APP_ICON  "qgn_list_gene_default_app"
#define ANIM_DURATION 5000 	/* 5 Secs for blinking icons */
#define ANIM_FPS      2

#define AS_MENU_BUTTON_NAME "hildon-navigator-small-button4"

#define AS_MENU_BUTTON_HIGHLIGHTED "qgn_some_theme_to_be_defined"

/* Hardcoded pixel perfecting values */
#define AS_BUTTON_BORDER_WIDTH  0
#define AS_MENU_BORDER_WIDTH    20
#define AS_TIP_BORDER_WIDTH 	20
#define AS_BUTTON_HEIGHT        38
#define AS_MENU_BUTTON_HEIGHT  116
#define AS_ROW_HEIGHT 		    30
#define AS_ICON_SIZE            26
#define AS_TOOLTIP_WIDTH        360
#define AS_MENU_ITEM_WIDTH      360
#define AS_INTERNAL_PADDING     10
#define AS_SEPARATOR_HEIGHT     10

#define SWITCHER_HIGHLIGHTING_FREQ 1000
#define SWITCHER_HIGHLIGHTED_NAME ""

#define SWITCHER_BUTTON_SIZE 32
#define SWITCHER_BUTTON_UP "qgn_list_tasknavigator_appswitcher"
#define SWITCHER_BUTTON_DOWN "qgn_list_tasknavigator_appswitcher"

#define SWITCHER_BUTTON_NAME ""

#define SWITCHER_TOGGLE_BUTTON GTK_BIN(switcher)->child

#define SWITCHER_DETTACHED_TIMEOUT 5000

#define SWITCHER_N_SLOTS 3	

#define HD_SWITCHER_MENU_APP_WINDOW_NAME      "hildon-switcher-menu-app-window"
#define HD_SWITCHER_MENU_APP_MENU_NAME        "hildon-switcher-menu-app-menu"
#define HD_SWITCHER_MENU_APP_MENU_ITEM_NAME   "hildon-switcher-menu-app-item"
#define HD_SWITCHER_MENU_APP_BUTTON_UP_NAME   ""
#define HD_SWITCHER_MENU_APP_BUTTON_DOWN_NAME ""
#define HD_SWITCHER_MENU_NOT_WINDOW_NAME      "hildon-switcher-menu-not-window"
#define HD_SWITCHER_MENU_NOT_MENU_NAME        "hildon-switcher-menu-not-menu"
#define HD_SWITCHER_MENU_NOT_MENU_ITEM_NAME   "hildon-switcher-menu-not-item"
#define HD_SWITCHER_MENU_NOT_BUTTON_UP_NAME   ""
#define HD_SWITCHER_MENU_NOT_BUTTON_DOWN_NAME ""

enum 
{
  PROP_MENU_NITEMS = 1,
};

#define HD_SWITCHER_MENU_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HD_TYPE_SWITCHER_MENU, HDSwitcherMenuPrivate))

G_DEFINE_TYPE (HDSwitcherMenu, hd_switcher_menu, TASKNAVIGATOR_TYPE_ITEM);

typedef struct 
{
  gchar     *icon;
  gchar     *category;
  gchar     *label;
  gchar     *dbus_callback;
  gboolean   is_active;
  GList     *notifications;
} HDSwitcherMenuNotificationGroup;

struct _HDSwitcherMenuPrivate
{
  HildonDesktopPopupMenu *menu_applications;
  HildonDesktopPopupMenu *menu_notifications;

  HildonDesktopPopupWindow *popup_window;
  GtkWidget		   *notifications_window;

  GtkImage		   *image_button;

  GtkWidget 		   *active_menu_item;
  GtkWidget		   *clear_events_menu_item;

  gboolean 		    is_open;
  gboolean 		    fullscreen;

  gint			    timeout;

  GtkIconTheme		   *icon_theme;
  GtkWidget		   *icon;

  GtkTreeIter		   *last_iter_added;

  HDWMEntryInfo		   *last_urgent_info;

  GtkWidget		   *window_dialog;
  GtkWidget		   *toggle_button;

  GHashTable               *notification_groups;
  
  gint                      esd_socket;
};

static GObject *hd_switcher_menu_constructor (GType gtype,
                                              guint n_params,
                                              GObjectConstructParam *params);

static void hd_switcher_menu_finalize (GObject *object);

static void hd_switcher_menu_style_set (GtkWidget *widget, GtkStyle *style, gpointer data);

static void hd_switcher_menu_toggled_cb (GtkWidget *button, HDSwitcherMenu *switcher);

static void hd_switcher_menu_scroll_to (HildonDesktopPopupWindow *window,
					HDSwitcherMenu *switcher);

static void hd_switcher_menu_add_clear_notifications_button (HDSwitcherMenu *switcher);

static void hd_switcher_menu_add_info_cb (HDWM *hdwm, HDWMEntryInfo *info, HDSwitcherMenu *switcher);
static void hd_switcher_menu_remove_info_cb (HDWM *hdwm, gboolean removed_app, HDWMEntryInfo *info, HDSwitcherMenu *switcher);
static void hd_switcher_menu_changed_info_cb (HDWM *hdwm, HDWMEntryInfo *info, HDSwitcherMenu *switcher);
static void hd_switcher_menu_changed_stack_cb (HDWM *hdwm, HDWMEntryInfo *info, HDSwitcherMenu *switcher);
static void hd_switcher_menu_show_menu_cb (HDWM *hdwm, HDSwitcherMenu *switcher);
static void hd_switcher_menu_long_press_cb (HDWM *hdwm, HDSwitcherMenu *switcher);
static void hd_switcher_menu_fullscreen_cb (HDWM *hdwm, gboolean fullscreen, HDSwitcherMenu *switcher);

static void hd_switcher_menu_notification_added_cb (GtkTreeModel   *tree_model,
                                                    GtkTreePath    *path,
                                                    GtkTreeIter    *iter,
                                                    HDSwitcherMenu *switcher);

static void hd_switcher_menu_notification_deleted_cb (HildonDesktopNotificationManager *nm,
                                                      gint 	      id,
                                                      HDSwitcherMenu *switcher);

static void hd_switcher_menu_notification_changed_cb (GtkTreeModel   *tree_model,
                                                      GtkTreePath    *path,
                                                      GtkTreeIter    *iter,
                                                      HDSwitcherMenu *switcher);

static void hd_switcher_menu_create_notifications_menu (HDSwitcherMenu *switcher);

static void hd_switcher_menu_reset_main_icon (HDSwitcherMenu *switcher);

static void hd_switcher_menu_check_content (HDSwitcherMenu *switcher);

static void hd_switcher_menu_item_activated (GtkMenuItem *menuitem, HDSwitcherMenu *switcher);

static void hd_switcher_menu_attach_button (HDSwitcherMenu *switcher);

static void hd_switcher_menu_dettach_button (HDSwitcherMenu *switcher);

static void 
hd_switcher_menu_init (HDSwitcherMenu *switcher)
{
  switcher->priv = HD_SWITCHER_MENU_GET_PRIVATE (switcher);

  switcher->priv->menu_applications  = 
  switcher->priv->menu_notifications = NULL;

  switcher->priv->popup_window = NULL;

  switcher->priv->active_menu_item       =
  switcher->priv->clear_events_menu_item = NULL;

  switcher->priv->is_open     =  
  switcher->priv->fullscreen  = FALSE;

  switcher->priv->timeout = 0;
  
  switcher->priv->last_iter_added = NULL;

  switcher->priv->last_urgent_info = NULL;

  switcher->hdwm = hd_wm_get_singleton ();

  switcher->nm = 
    HILDON_DESKTOP_NOTIFICATION_MANAGER 
     (hildon_desktop_notification_manager_get_singleton ());

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (switcher->nm),
		  			HD_NM_COL_ID,
					GTK_SORT_DESCENDING);
  
  switcher->priv->icon_theme = gtk_icon_theme_get_default ();

  g_object_ref (switcher->hdwm);

  switcher->priv->esd_socket = hn_as_sound_init ();
}

static void 
hd_switcher_menu_class_init (HDSwitcherMenuClass *switcher)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (switcher);
  
  object_class->constructor = hd_switcher_menu_constructor;
  object_class->finalize    = hd_switcher_menu_finalize;

  g_type_class_add_private (object_class, sizeof (HDSwitcherMenuPrivate));
}

static void 
hd_switcher_menu_resize_menu (HildonDesktopPopupMenu *menu, HDSwitcherMenu *switcher)
{ 
  hildon_desktop_popup_recalculate_position (switcher->priv->popup_window);  
}	

static GdkPixbuf *
hd_switcher_menu_get_icon_from_theme (HDSwitcherMenu *switcher,
                                     const gchar     *icon_name,
                                     gint             size)
{
  GError *error;
  GdkPixbuf *retval;

  if (!icon_name)
    return NULL;

  switcher->priv->icon_theme = gtk_icon_theme_get_default ();

  g_return_val_if_fail (switcher->priv->icon_theme, NULL);

  if (!icon_name || icon_name[0] == '\0')
    return NULL;

  error = NULL;
  retval = gtk_icon_theme_load_icon (switcher->priv->icon_theme,
                                     icon_name,
                                     size == -1 ? AS_ICON_SIZE : size,
                                     0,
                                     &error);
  if (error)
  {
    g_warning ("Could not load icon '%s': %s\n",
              icon_name,
              error->message);

    g_error_free (error);

    return NULL;
  }

  return retval;
}

static gboolean  
hd_switcher_menu_popup_window_keypress_cb (GtkWidget      *widget,
			                   GdkEventKey    *event,
			                   HDSwitcherMenu *switcher)
{
  HildonDesktopPopupWindow *window =
    HILDON_DESKTOP_POPUP_WINDOW (widget);

  if (event->keyval == GDK_Left ||
      event->keyval == GDK_KP_Left ||
      event->keyval == GDK_Escape)
  {
    hildon_desktop_popup_window_popdown (window);

    switcher->priv->is_open = FALSE;

    if (event->keyval == GDK_Escape)
    {
      /* pass focus to the last active application */
      hd_wm_focus_active_window (switcher->hdwm);
    }
    else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (switcher->priv->toggle_button), FALSE);

      GdkWindow *window = gtk_widget_get_parent_window (switcher->priv->toggle_button);
      gtk_widget_grab_focus (GTK_WIDGET (switcher->priv->toggle_button));
      hd_wm_activate_window (HD_TN_ACTIVATE_KEY_FOCUS,window);
    }

    return TRUE;
  }
  else
  if (event->keyval == GDK_Right ||
      event->keyval == GDK_KP_Right)
  {
    GList *notifications =
     hildon_desktop_popup_menu_get_children 
       (switcher->priv->menu_notifications);

    if (notifications)    
    {	  
      hildon_desktop_popup_window_jump_to_pane 
        (switcher->priv->popup_window, 0);	    

      hildon_desktop_popup_menu_select_item
        (switcher->priv->menu_applications, NULL);

      hildon_desktop_popup_menu_select_first_item 
        (switcher->priv->menu_notifications);	    

      g_list_free (notifications);
      
      return TRUE;
    }

    g_list_free (notifications);
  }	  

  return FALSE;
}

static gboolean 
hd_switcher_menu_popup_window_pane_keypress_cb (GtkWidget      *widget,
	                                        GdkEventKey    *event,
        	                                HDSwitcherMenu *switcher)
{
  if (event->keyval == GDK_Left ||
      event->keyval == GDK_KP_Left)
  {
    hildon_desktop_popup_window_jump_to_pane
      (switcher->priv->popup_window, -1);

    hildon_desktop_popup_menu_select_item
      (switcher->priv->menu_notifications, NULL);

    hildon_desktop_popup_menu_select_first_item
      (switcher->priv->menu_applications);

    return TRUE;
  } 

  return FALSE;
}

static gboolean 
hd_switcher_menu_switcher_keypress_cb (GtkWidget      *widget,
				       GdkEventKey    *event,
				       HDSwitcherMenu *switcher)
{
  if (event->keyval == GDK_Right ||
      event->keyval == GDK_KP_Right)
  {
    gtk_toggle_button_set_active
      (GTK_TOGGLE_BUTTON (switcher->priv->toggle_button), TRUE);
	  
    g_signal_emit_by_name (switcher->priv->toggle_button, "toggled");
  }	  

  return TRUE;
}

#ifdef HAVE_MCE
static void
hd_switcher_menu_set_led_pattern (const gchar *pattern, gboolean activate)
{
  DBusConnection *sys_conn = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);

  g_return_if_fail (pattern != NULL);
  
  DBusMessage *message = 
    dbus_message_new_method_call (MCE_SERVICE,
                                  MCE_REQUEST_PATH,
                                  MCE_REQUEST_IF,
				  (activate ? MCE_ACTIVATE_LED_PATTERN :
				              MCE_DEACTIVATE_LED_PATTERN));

  dbus_message_append_args (message,
                            DBUS_TYPE_STRING, &pattern,
                            DBUS_TYPE_INVALID);

  dbus_message_set_no_reply (message, TRUE);

  dbus_connection_send (sys_conn, message, NULL);
  dbus_connection_flush (sys_conn);

  dbus_message_unref (message);
  dbus_connection_unref (sys_conn);
}
#endif

static void
hd_switcher_menu_update_open (HildonDesktopPopupWindow *window,
			      HDSwitcherMenu *switcher)
{
#ifdef HAVE_MCE  
  GList *children, *l; 
	
  children = hildon_desktop_popup_menu_get_children (switcher->priv->menu_notifications);

  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (HD_IS_SWITCHER_MENU_ITEM (l->data))
    {
      GtkTreeIter iter;
      GHashTable *hints;
      GValue *hint;

      guint id = 
        hd_switcher_menu_item_get_notification_id (HD_SWITCHER_MENU_ITEM (l->data));

      if (hildon_desktop_notification_manager_find_by_id (switcher->nm, id, &iter))
      {
        gtk_tree_model_get (GTK_TREE_MODEL (switcher->nm),
                            &iter,
                            HD_NM_COL_HINTS, &hints,
                            -1);

	hint = g_hash_table_lookup (hints, "led-pattern");

	if (hint)
          hd_switcher_menu_set_led_pattern (g_value_get_string (hint), FALSE);
      }
    }
  }
#endif

  hd_switcher_menu_scroll_to (window, switcher);
}

static void 
hd_switcher_menu_update_close (HildonDesktopPopupWindow *window, HDSwitcherMenu *switcher)
{
  GList *children = NULL, *l;
	
  switcher->priv->is_open = FALSE;

  children = hildon_desktop_popup_menu_get_children (switcher->priv->menu_notifications);

  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (HD_IS_SWITCHER_MENU_ITEM (l->data) &&
	hd_switcher_menu_item_is_blinking (HD_SWITCHER_MENU_ITEM (l->data)))
    {
      GtkTreeIter iter;
      guint id = 
        hd_switcher_menu_item_get_notification_id (HD_SWITCHER_MENU_ITEM (l->data));

      if (hildon_desktop_notification_manager_find_by_id (switcher->nm, id, &iter))
      {
        g_signal_handlers_block_by_func (switcher->nm,
					 hd_switcher_menu_notification_changed_cb, 
					 switcher);
	      
        gtk_list_store_set (GTK_LIST_STORE (switcher->nm),
                            &iter,
                            HD_NM_COL_ACK, FALSE,
                            -1);

        g_signal_handlers_unblock_by_func (switcher->nm,
					   hd_switcher_menu_notification_changed_cb, 
					   switcher);
      }

      hd_switcher_menu_item_set_blinking (HD_SWITCHER_MENU_ITEM (l->data), FALSE);
    }
  }

  gtk_toggle_button_set_active 
    (GTK_TOGGLE_BUTTON (switcher->priv->toggle_button), FALSE);

  g_list_free (children);
}

static void
hd_switcher_menu_free_notification_group (gpointer user_data)
{
  HDSwitcherMenuNotificationGroup *ngroup;

  ngroup = (HDSwitcherMenuNotificationGroup *) user_data;

  if (ngroup->icon)
    g_free (ngroup->icon);

  if (ngroup->category)
    g_free (ngroup->category);

  if (ngroup->label)
    g_free (ngroup->label);

  if (ngroup->dbus_callback)
    g_free (ngroup->dbus_callback);

  if (ngroup->notifications)
    g_list_free (ngroup->notifications);

  g_free (ngroup);
}

static void
hd_switcher_menu_add_notification_group (gpointer key,
	             	                 gpointer value, 
					 gpointer user_data)
{
  HDSwitcherMenuNotificationGroup *ngroup;
  HDSwitcherMenu *switcher;
  
  ngroup = (HDSwitcherMenuNotificationGroup *) value;
  switcher = (HDSwitcherMenu *) user_data;	
 
  if (ngroup->is_active)
  {
    GtkWidget *menu_item;
    GdkPixbuf *icon;
    gchar *summary;
	  
    summary = g_strdup_printf (_(ngroup->label), 
		               g_list_length (ngroup->notifications));
    
    icon = hd_switcher_menu_get_icon_from_theme (switcher, ngroup->icon, -1);
    
    hildon_desktop_popup_menu_add_item
      (switcher->priv->menu_notifications,
      GTK_MENU_ITEM (gtk_separator_menu_item_new ()));

    menu_item = hd_switcher_menu_item_new_from_notification_group
      (ngroup->notifications, icon, summary, ngroup->dbus_callback, TRUE);

    gtk_widget_set_name (GTK_WIDGET (menu_item), 
      		         HD_SWITCHER_MENU_NOT_MENU_ITEM_NAME);

    hildon_desktop_popup_menu_add_item
     (switcher->priv->menu_notifications, GTK_MENU_ITEM (menu_item));
  }
}

static void
hd_switcher_menu_add_notification_groups (HDSwitcherMenu *switcher)
{
  g_hash_table_foreach (switcher->priv->notification_groups, 
		        hd_switcher_menu_add_notification_group,
			switcher);
}

static void
hd_switcher_menu_reset_notification_group (gpointer key,
	             	                   gpointer value, 
					   gpointer user_data)
{
  HDSwitcherMenuNotificationGroup *ngroup;
  
  ngroup = (HDSwitcherMenuNotificationGroup *) value;

  ngroup->notifications = NULL;
  ngroup->is_active = FALSE;
}

static void
hd_switcher_menu_reset_notification_groups (HDSwitcherMenu *switcher)
{
  g_hash_table_foreach (switcher->priv->notification_groups, 
		        hd_switcher_menu_reset_notification_group,
			switcher);
}

static void
hd_switcher_menu_load_notification_groups (HDSwitcherMenu *switcher)
{
  GKeyFile *keyfile;
  gchar **groups;
  GError *error = NULL;
  gint i;

  keyfile = g_key_file_new ();

  g_key_file_load_from_file (keyfile,
                             HD_DESKTOP_CONFIG_PATH "/notification-groups.conf",
                             G_KEY_FILE_NONE,
                             &error);

  if (error)
  {
    g_warning ("Error loading notification groups file: %s", error->message);
    g_error_free (error);
    
    return;
  }

  groups = g_key_file_get_groups (keyfile, NULL);

  for (i = 0; groups[i]; i++)
  {
    HDSwitcherMenuNotificationGroup *ngroup;

    ngroup = g_new0 (HDSwitcherMenuNotificationGroup, 1);

    ngroup->icon = g_key_file_get_string (keyfile,
		    			  groups[i],
					  "Icon",
					  &error);

    if (error)
    {
      g_warning ("Error loading notification groups file: %s", error->message);
      hd_switcher_menu_free_notification_group (ngroup);
      g_error_free (error);
      error = NULL;
      continue;
    }

    ngroup->category = g_key_file_get_string (keyfile,
		    			      groups[i],
					      "Category",
					      &error);

    if (error)
    {
      g_warning ("Error loading notification groups file: %s", error->message);
      hd_switcher_menu_free_notification_group (ngroup);
      g_error_free (error);
      error = NULL;
      continue;
    }      

    ngroup->label = g_key_file_get_string (keyfile,
		    			   groups[i],
					   "Label",
					   &error);

    if (error)
    {
      g_warning ("Error loading notification groups file: %s", error->message);
      hd_switcher_menu_free_notification_group (ngroup);
      g_error_free (error);
      error = NULL;
      continue;
    }

    ngroup->dbus_callback = g_key_file_get_string (keyfile,
		    			           groups[i],
					           "DBus-Call",
					           &error);

    if (error)
    {
      g_warning ("Error loading notification groups file: %s", error->message);
      hd_switcher_menu_free_notification_group (ngroup);
      g_error_free (error);
      error = NULL;
      continue;
    }

    ngroup->is_active = FALSE;
    ngroup->notifications = NULL;
    
    g_hash_table_insert (switcher->priv->notification_groups, 
		         ngroup->category, 
			 ngroup); 
  }

  g_free (groups);
  g_key_file_free (keyfile);
}
	
static GObject *
hd_switcher_menu_constructor (GType gtype,
			      guint n_params,
			      GObjectConstructParam *params)
{
  GObject *object;
  HDSwitcherMenu *switcher;
  GtkWidget *button;
  HDWM *hdwm = hd_wm_get_singleton ();
  GdkPixbuf *pixbuf, *icon_up = NULL, *icon_down = NULL;

  object = G_OBJECT_CLASS (hd_switcher_menu_parent_class)->constructor (gtype,
           	                                                        n_params,
                                                                        params);
  switcher = HD_SWITCHER_MENU (object);

  gtk_widget_push_composite_child ();

  button = hildon_desktop_toggle_button_new ();

  switcher->priv->toggle_button = button;

  gtk_widget_set_name (button, AS_MENU_BUTTON_NAME);
  gtk_widget_set_size_request (button, -1, AS_MENU_BUTTON_HEIGHT);

  pixbuf = hd_switcher_menu_get_icon_from_theme (switcher, AS_MENU_BUTTON_ICON, -1);
  switcher->priv->icon = gtk_image_new_from_pixbuf (pixbuf);
  gtk_container_add (GTK_CONTAINER (button), switcher->priv->icon);
  gtk_widget_show (switcher->priv->icon);
  g_object_unref (pixbuf);
  
  gtk_container_add (GTK_CONTAINER (object), button);
  gtk_widget_show (button);

  switcher->priv->popup_window =
    HILDON_DESKTOP_POPUP_WINDOW 
      (hildon_desktop_popup_window_new (1,GTK_ORIENTATION_HORIZONTAL,HD_POPUP_WINDOW_DIRECTION_RIGHT_BOTTOM));	     

  /* We don't attach the widget because if we do it, we cannot be on top of 
   * virtual keyboard. Anyway it should be transient to button
   */

  hildon_desktop_popup_window_attach_widget
    (switcher->priv->popup_window, NULL);	  
  
  switcher->priv->notifications_window = 
    hildon_desktop_popup_window_get_pane (switcher->priv->popup_window, 0);

  gtk_widget_set_size_request (GTK_WIDGET (switcher->priv->popup_window),
			       340, 100);
  
  gtk_widget_set_size_request (GTK_WIDGET (switcher->priv->notifications_window),
			       340, 100);  

  switcher->priv->menu_applications =
    HILDON_DESKTOP_POPUP_MENU (g_object_new (HILDON_DESKTOP_TYPE_POPUP_MENU,
		  		    	     "item-height", 72,
				    	     NULL));	    

  switcher->priv->menu_notifications =
    HILDON_DESKTOP_POPUP_MENU (g_object_new (HILDON_DESKTOP_TYPE_POPUP_MENU,
		  			     "item-height", 72,
					     NULL));

  gtk_container_add (GTK_CONTAINER (switcher->priv->popup_window),
		     GTK_WIDGET (switcher->priv->menu_applications));
  gtk_container_add (GTK_CONTAINER (switcher->priv->notifications_window),
		     GTK_WIDGET (switcher->priv->menu_notifications));

  gtk_widget_show (GTK_WIDGET (switcher->priv->menu_applications));
  gtk_widget_show (GTK_WIDGET (switcher->priv->menu_notifications));

  icon_up = 
    hd_switcher_menu_get_icon_from_theme (switcher,
		                          SWITCHER_BUTTON_UP,
		    			  -1);

  icon_down = 
    hd_switcher_menu_get_icon_from_theme (switcher,
		    			  SWITCHER_BUTTON_DOWN,
		    			  -1);
	  
  if (icon_up && icon_down)
  {
    const GtkWidget *button_up, *button_down;
    GtkWidget *image_up, *image_down;

    button_up = 
      hildon_desktop_popup_menu_get_scroll_button_up (switcher->priv->menu_applications);
    button_down =
      hildon_desktop_popup_menu_get_scroll_button_down (switcher->priv->menu_applications);

    gtk_widget_set_name (GTK_WIDGET (button_up), SWITCHER_BUTTON_NAME);
    gtk_widget_set_name (GTK_WIDGET (button_down), SWITCHER_BUTTON_NAME);
    
    image_up   = gtk_image_new_from_pixbuf (icon_up);
    image_down = gtk_image_new_from_pixbuf (icon_down);
 
    gtk_container_add (GTK_CONTAINER (button_up),   image_up);
    gtk_container_add (GTK_CONTAINER (button_down), image_down);
    gtk_widget_show (image_up);
    gtk_widget_show (image_down);
    
    button_up = 
      hildon_desktop_popup_menu_get_scroll_button_up (switcher->priv->menu_notifications);
    button_down =
      hildon_desktop_popup_menu_get_scroll_button_down (switcher->priv->menu_notifications);

    gtk_widget_set_name (GTK_WIDGET (button_up), SWITCHER_BUTTON_NAME);
    gtk_widget_set_name (GTK_WIDGET (button_down), SWITCHER_BUTTON_NAME);

    image_up   = gtk_image_new_from_pixbuf (icon_up);
    image_down = gtk_image_new_from_pixbuf (icon_down);

    gtk_container_add (GTK_CONTAINER (button_up),   image_up);
    gtk_container_add (GTK_CONTAINER (button_down), image_down);
    gtk_widget_show (image_up);
    gtk_widget_show (image_down);
  } 

  gtk_widget_set_name (GTK_WIDGET (switcher->priv->popup_window), 
		       HD_SWITCHER_MENU_APP_WINDOW_NAME);
  gtk_widget_set_name (GTK_WIDGET (switcher->priv->notifications_window), 
		       HD_SWITCHER_MENU_NOT_WINDOW_NAME);

  gtk_widget_set_name (GTK_WIDGET (switcher->priv->menu_applications), 
		       HD_SWITCHER_MENU_APP_MENU_NAME);
  gtk_widget_set_name (GTK_WIDGET (switcher->priv->menu_notifications), 
		       HD_SWITCHER_MENU_NOT_MENU_NAME);
  
  
  g_signal_connect (switcher->priv->popup_window,
		    "key-press-event",
		    G_CALLBACK (hd_switcher_menu_popup_window_keypress_cb),
		    (gpointer)switcher);

  g_signal_connect (hildon_desktop_popup_window_get_pane (switcher->priv->popup_window, 0),
		    "key-press-event",
		    G_CALLBACK (hd_switcher_menu_popup_window_pane_keypress_cb),
		    (gpointer)switcher);

  g_signal_connect (button,
		    "key-press-event",
		    G_CALLBACK (hd_switcher_menu_switcher_keypress_cb),
		    (gpointer)switcher);
  
  g_signal_connect (switcher,
		    "style-set",
		    G_CALLBACK (hd_switcher_menu_style_set),
		    NULL);

  g_signal_connect (switcher->priv->popup_window,
		    "popdown-window",
		    G_CALLBACK (hd_switcher_menu_update_close),
		    (gpointer)switcher);
  
  g_signal_connect (switcher->priv->menu_applications,
		    "popup-menu-resize",
		    G_CALLBACK (hd_switcher_menu_resize_menu),
		    (gpointer)switcher);

  g_signal_connect (switcher->priv->menu_notifications,
		    "popup-menu-resize",
		    G_CALLBACK (hd_switcher_menu_resize_menu),
		    (gpointer)switcher);

  g_signal_connect (button,
		    "toggled",
		    G_CALLBACK (hd_switcher_menu_toggled_cb),
		    (gpointer)switcher);

  g_signal_connect (switcher->priv->popup_window,
		    "popup-window",
		    G_CALLBACK (hd_switcher_menu_update_open),
		    (gpointer)switcher);

  g_signal_connect (switcher->hdwm,
                    "entry_info_added",
                    G_CALLBACK (hd_switcher_menu_add_info_cb),
                    (gpointer)switcher);

  g_signal_connect (switcher->hdwm,
                    "entry_info_removed",
                    G_CALLBACK (hd_switcher_menu_remove_info_cb),
                    (gpointer)switcher);

  g_signal_connect (switcher->hdwm,
                    "entry_info_changed",
                    G_CALLBACK (hd_switcher_menu_changed_info_cb),
                    (gpointer)switcher);

  g_signal_connect (switcher->hdwm,
                    "entry_info_stack_changed",
                    G_CALLBACK (hd_switcher_menu_changed_stack_cb),
                    (gpointer)switcher);

  g_signal_connect (switcher->hdwm,
                    "show-menu",
                    G_CALLBACK (hd_switcher_menu_show_menu_cb),
                    (gpointer)switcher);

  g_signal_connect (switcher->hdwm,
                    "long-key-press",
                    G_CALLBACK (hd_switcher_menu_long_press_cb),
                    (gpointer)switcher);

  g_signal_connect (switcher->hdwm,
                    "fullscreen",
                    G_CALLBACK (hd_switcher_menu_fullscreen_cb),
                    (gpointer)switcher);

  g_signal_connect_after (switcher->nm,
		          "row-inserted",
		          G_CALLBACK (hd_switcher_menu_notification_added_cb),
		          (gpointer)switcher);

  g_signal_connect (switcher->nm,
                    "notification-closed",
                    G_CALLBACK (hd_switcher_menu_notification_deleted_cb),
                    (gpointer)switcher);

  g_signal_connect (switcher->nm,
		    "row-changed",
                    G_CALLBACK (hd_switcher_menu_notification_changed_cb),
		    (gpointer)switcher);
  
  gtk_widget_pop_composite_child ();

  GtkWidget *menu_item = 
    hd_switcher_menu_item_new_from_entry_info 
      (hd_wm_get_home_info (hdwm), FALSE);

  gtk_widget_set_name (GTK_WIDGET (menu_item), 
      		       HD_SWITCHER_MENU_APP_MENU_ITEM_NAME);

  hildon_desktop_popup_menu_add_item
   (switcher->priv->menu_applications, GTK_MENU_ITEM (menu_item));

  g_signal_connect_after (menu_item,
                          "activate",
                          G_CALLBACK (hd_switcher_menu_item_activated),
                          (gpointer)switcher); 

  switcher->priv->notification_groups = 
          g_hash_table_new_full (g_str_hash, 
	  		         g_str_equal,
			         (GDestroyNotify) g_free,
			         (GDestroyNotify) hd_switcher_menu_free_notification_group);

  hd_switcher_menu_load_notification_groups (switcher);
  
  hd_switcher_menu_create_notifications_menu (switcher);

  hd_switcher_menu_check_content (switcher);

  switcher->priv->window_dialog = NULL;

  return object;
}

static gboolean 
hd_switcher_menu_force_move_window (GtkWidget *widget, HDSwitcherMenu *switcher)
{
  gint x,y;

  g_object_get (widget, "x", &x, "y", &y, NULL);  
	
  gdk_window_move (widget->window, x, y);

  return FALSE;
}	

static void 
hd_switcher_create_external_window (HDSwitcherMenu *switcher)
{
  HildonDesktopPanelWindowOrientation orientation;
  GtkWidget *top_level;
	
  if (switcher->priv->window_dialog)
    return;	  

  top_level = gtk_widget_get_toplevel (GTK_WIDGET (switcher));

  if (HILDON_DESKTOP_IS_PANEL_WINDOW (top_level))
    g_object_get (top_level, "orientation", &orientation, NULL);
  else
    orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT;	  
  
  switcher->priv->window_dialog =
    GTK_WIDGET (g_object_new (HILDON_DESKTOP_TYPE_PANEL_WINDOW_DIALOG,
                              "x", 0,
                              "y", gdk_screen_height () - AS_BUTTON_HEIGHT*2,
                              "width", GTK_WIDGET (switcher)->allocation.width,
                              "height", AS_BUTTON_HEIGHT*2,
                              "move", TRUE,
                              "use-old-titlebar", FALSE,
			      "orientation", orientation,
                              NULL));

  gtk_container_remove (GTK_CONTAINER (switcher->priv->window_dialog),
                        GTK_BIN (switcher->priv->window_dialog)->child);

  g_signal_connect (switcher->priv->window_dialog,
		    "map-event",
		    G_CALLBACK (hd_switcher_menu_force_move_window),
		    (gpointer)switcher);
}	

static void 
hd_switcher_menu_finalize (GObject *object)
{
  HDSwitcherMenu *switcher = HD_SWITCHER_MENU (object);	
	
  g_object_unref (switcher->hdwm);

  gtk_widget_destroy (GTK_WIDGET (switcher->priv->popup_window));

  if (switcher->priv->window_dialog)
    gtk_widget_destroy (switcher->priv->window_dialog);	  

  g_object_unref (switcher->priv->icon_theme);

  hn_as_sound_deinit (switcher->priv->esd_socket); 

  if (switcher->priv->notification_groups)
  {
    g_hash_table_destroy (switcher->priv->notification_groups);
    switcher->priv->notification_groups = NULL;
  }
	  
  G_OBJECT_CLASS (hd_switcher_menu_parent_class)->finalize (object);
}

static gboolean
hd_switcher_menu_confirm_clear_events ()
{
	GtkWidget *note;
	gint response;
	
	note = hildon_note_new_confirmation_with_icon_name (NULL,
					                    AS_CLEAR_CONFIRM,
							    AS_CLEAR_CONFIRM_ICON);

	response = gtk_dialog_run (GTK_DIALOG (note));

	gtk_widget_destroy (note);
	
	return (response == GTK_RESPONSE_OK); 
}

static void 
hd_switcher_menu_clear_item_activated (GtkMenuItem *menuitem, HDSwitcherMenu *switcher)
{
  GtkTreeModel *nm;
  GtkTreeIter iter;
  gboolean need_confirmation = FALSE;

  nm = GTK_TREE_MODEL (switcher->nm);

  if (gtk_tree_model_get_iter_first (nm, &iter))
  {
    do
    {
      GHashTable *hints;
      GValue *hint;
      const gchar *category;
      
      gtk_tree_model_get (nm,
                          &iter,
                          HD_NM_COL_HINTS, &hints,
                          -1);

      hint = g_hash_table_lookup (hints, "category");

      if (hint)
      {	  
        category = g_value_get_string (hint);

        if (g_str_has_prefix (category, "im."))
        {
          need_confirmation = TRUE;
	  break; 
	}
      }
    }
    while (gtk_tree_model_iter_next (nm, &iter));
  }	 

  if (!need_confirmation ||
      hd_switcher_menu_confirm_clear_events ())
  {
    hildon_desktop_notification_manager_close_all (switcher->nm);
    gtk_widget_destroy (switcher->priv->clear_events_menu_item);
    switcher->priv->clear_events_menu_item = NULL;
  }
}
	
static void 
hd_switcher_menu_add_clear_notifications_button (HDSwitcherMenu *switcher)
{
  GtkWidget *hbox, *image, *label;
  GdkPixbuf *icon;

  switcher->priv->clear_events_menu_item = gtk_menu_item_new ();
    
  hbox = gtk_hbox_new (FALSE, 0);

  icon = hd_switcher_menu_get_icon_from_theme (switcher,
        	  			         AS_CLEAR_ITEM_ICON, 
      					 -1);
  
  image = gtk_image_new_from_pixbuf (icon);
  
  gtk_box_pack_start (GTK_BOX (hbox),
  		        image, 
  		        FALSE, 
  		        FALSE, 
  		        15);
  
  label = gtk_label_new (AS_CLEAR_ITEM); 
  
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  
  gtk_box_pack_start (GTK_BOX (hbox),
  		        label,
  		        TRUE, 
  		        TRUE, 
  		        0);
  
  gtk_container_add (GTK_CONTAINER (switcher->priv->clear_events_menu_item), 
      	       hbox);

  gtk_widget_show_all (hbox);

  g_signal_connect_after (G_OBJECT (switcher->priv->clear_events_menu_item),
                          "activate",
                          G_CALLBACK (hd_switcher_menu_clear_item_activated),
                          switcher);

  hildon_desktop_popup_menu_add_item
   (switcher->priv->menu_notifications, 
    GTK_MENU_ITEM (switcher->priv->clear_events_menu_item));
}

static void
hd_switcher_menu_replace_blinking_icon (HDSwitcherMenu *switcher, GdkPixbuf *icon)
{
  if (!switcher->priv->is_open && icon)
  {
    GdkPixbufAnimation *icon_blinking;
    GtkWidget *image_blinking;

    icon_blinking =
      hn_app_pixbuf_anim_blinker_new (icon,1000/ANIM_FPS,-1,10);

    image_blinking = gtk_image_new ();
    gtk_image_set_from_animation (GTK_IMAGE (image_blinking), icon_blinking);

    if (image_blinking)
    {    
      if (GTK_BIN (switcher->priv->toggle_button)->child == switcher->priv->icon)   
      {	      
        g_object_ref (G_OBJECT (switcher->priv->icon));
        gtk_container_remove (GTK_CONTAINER (switcher->priv->toggle_button), GTK_WIDGET (switcher->priv->icon));
      }
      else
       gtk_container_remove (GTK_CONTAINER (switcher->priv->toggle_button), GTK_BIN (switcher->priv->toggle_button)->child);	      
    }

    gtk_container_add (GTK_CONTAINER (switcher->priv->toggle_button), image_blinking);
    gtk_widget_show (image_blinking);
  }
}

static void 
hd_switcher_menu_item_activated (GtkMenuItem *menuitem, HDSwitcherMenu *switcher)
{
  if (switcher->priv->is_open)
    hildon_desktop_popup_window_popdown
      (switcher->priv->popup_window);

  switcher->priv->is_open = FALSE;
  
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (switcher->priv->toggle_button), FALSE);
}	

static void 
hd_switcher_menu_style_set (GtkWidget *widget, GtkStyle *style, gpointer data)
{
  static gboolean first_run = FALSE; /* FIXME: WTF????? Do it better. Highlighting the item will call this handler */
  
  if (!first_run)
  {
    gtk_widget_set_name (widget, AS_MENU_BUTTON_NAME);	
    gtk_widget_set_name (GTK_BIN (widget)->child, AS_MENU_BUTTON_NAME);
    first_run = TRUE;
  }
}

static void
hd_switcher_menu_update_highlighting (HDSwitcherMenu *switcher, gboolean state)
{
  if (state)
  {
    gtk_widget_set_name (GTK_WIDGET (switcher), SWITCHER_HIGHLIGHTED_NAME);
    gtk_widget_set_name (switcher->priv->toggle_button, SWITCHER_HIGHLIGHTED_NAME);    
  }	  
  else
  {
    gtk_widget_set_name (GTK_WIDGET (switcher), AS_MENU_BUTTON_NAME);
    gtk_widget_set_name (switcher->priv->toggle_button, AS_MENU_BUTTON_NAME);
  }	  
}

static void 
hd_switcher_menu_check_content (HDSwitcherMenu *switcher)
{
  GList *children = NULL;
  GList *apps = NULL;
  guint n_apps = 0;

  children = 
    hildon_desktop_popup_menu_get_children (switcher->priv->menu_notifications);

  apps = hd_wm_get_applications (switcher->hdwm);
  
  if (apps || children)
  {    
    g_object_set (switcher->priv->toggle_button, "can-focus", TRUE, NULL);
    gtk_widget_grab_focus (switcher->priv->toggle_button);
    gtk_widget_set_sensitive (switcher->priv->toggle_button, TRUE);

    gtk_widget_show (GTK_BIN (switcher->priv->toggle_button)->child);

    if (children)
    {	     
      if (GTK_BIN (switcher->priv->toggle_button)->child != switcher->priv->icon)
      {
        if (switcher->priv->fullscreen)
          hd_switcher_menu_dettach_button (switcher);
        else
          hd_switcher_menu_attach_button (switcher);	     
      }	       
	     
      hd_switcher_menu_update_highlighting (switcher, TRUE);
    }
    else
    {	     
      hd_switcher_menu_update_highlighting (switcher, FALSE); 
    }

    if (!switcher->priv->fullscreen)
      hd_switcher_menu_attach_button (switcher);

    n_apps = g_list_length (apps);

    if (n_apps <= SWITCHER_N_SLOTS && switcher->priv->last_urgent_info) 
    {	    
      hd_switcher_menu_reset_main_icon (switcher);	    
      switcher->priv->last_urgent_info = NULL;
    }
  }
  else
  {	  
     hd_switcher_menu_update_highlighting (switcher, FALSE);
	  
     gtk_widget_hide (GTK_BIN (switcher->priv->toggle_button)->child);	  
     g_object_set (switcher->priv->toggle_button, "can-focus", FALSE, NULL);
     gtk_widget_set_sensitive (switcher->priv->toggle_button, FALSE);
     
     if (switcher->priv->is_open)
     {	     
       hildon_desktop_popup_window_popdown 
         (switcher->priv->popup_window);	       
 
       switcher->priv->is_open = FALSE;
     }
     
     gtk_toggle_button_set_active 
       (GTK_TOGGLE_BUTTON (switcher->priv->toggle_button), FALSE);
  }

  if (!children && switcher->priv->is_open)
  {
    g_debug ("This was necessary!!!!!!!!!!!!!!!!");	     
    hildon_desktop_popup_window_jump_to_pane (switcher->priv->popup_window, 0);
  }  	     

  g_list_free (children);
}	

static void 
hd_switcher_menu_position_func (HildonDesktopPopupWindow  *menu,
                                gint     *x,
                                gint     *y,
                                gpointer  data)
{
  HDSwitcherMenu *switcher = HD_SWITCHER_MENU (data);
  GtkRequisition req;
  GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (menu));
  gint menu_height = 0;
  gint main_height = 0;
  GtkAllocation workarea = { 0, 0, 0, 0 };
  GtkWidget *top_level;
  HildonDesktopPanelWindowOrientation orientation =
      HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT;
  GtkWidget *button = GTK_BIN (switcher)->child;
  gboolean dettached = FALSE;
  
  if (!button)
  {	  
    button = GTK_BIN (switcher->priv->window_dialog)->child;	  
    dettached = TRUE;
  }
  
  if (!GTK_WIDGET_REALIZED (GTK_WIDGET (data)))
    return;

  hd_wm_get_work_area (switcher->hdwm, &workarea);

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
      if (switcher->priv->fullscreen)
        *x = 0;	
      else      
        *x = workarea.x;		
      
      if (main_height - button->allocation.y < menu_height)
        *y = MAX (0, (main_height - menu_height));
      else
      {
        if (!dettached)	      
	  *y = button->allocation.y;
	else
	  *y = main_height - menu_height;
      }
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT:
      if (switcher->priv->fullscreen)
        *x = gdk_screen_get_width (screen) - req.width;
      else      
        *x = workarea.x + workarea.width - req.width;

      if (main_height - button->allocation.y < menu_height)
        *y = MAX (0, (main_height - menu_height));
      else
        *y = button->allocation.y;
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP:
      *x = button->allocation.x;

      if (switcher->priv->fullscreen)
        *y = req.height;
      else
	*y = workarea.y;	      
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM:
      *x = button->allocation.x;

      if (switcher->priv->fullscreen)
        *y = main_height - req.height;
      else
        *y = workarea.y + workarea.height - req.height;	      
      break;

    default:
      g_assert_not_reached ();
  }
}

static void 
hd_switcher_menu_toggled_cb (GtkWidget *button, HDSwitcherMenu *switcher)
{
  if (!GTK_WIDGET_VISIBLE (GTK_BIN (switcher->priv->toggle_button)->child))
    return;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (switcher->priv->toggle_button)))
  {
    hildon_desktop_popup_window_popdown (switcher->priv->popup_window);
    switcher->priv->is_open = FALSE;
    return;
  }   

  if (switcher->priv->is_open)
  {	  
    hildon_desktop_popup_menu_select_next_item (switcher->priv->menu_applications);
    hd_wm_debug ("selecting next item");
  }
    
  hildon_desktop_popup_window_popup 
   (switcher->priv->popup_window,
    hd_switcher_menu_position_func,
    (gpointer)switcher,
    GDK_CURRENT_TIME);

  if (!hildon_desktop_popup_menu_get_children (switcher->priv->menu_notifications))
    gtk_widget_hide (switcher->priv->notifications_window);
  else
    gtk_widget_show (switcher->priv->notifications_window);  

  switcher->priv->is_open = TRUE;

  hd_switcher_menu_reset_main_icon (switcher);

  hd_switcher_menu_check_content (switcher);

  if (GTK_BIN (switcher->priv->toggle_button)->child == switcher->priv->icon)
    hd_switcher_menu_attach_button (switcher);	  
}

static void 
hd_switcher_menu_scroll_to (HildonDesktopPopupWindow *window,
			    HDSwitcherMenu *switcher)
{
  GtkAdjustment *adj;
       
  if (!switcher->priv->menu_applications ||
      !switcher->priv->menu_notifications)
  {
    return;	   
  }	   
  hildon_desktop_popup_menu_scroll_to_selected
   (switcher->priv->menu_applications);

  adj = hildon_desktop_popup_menu_get_adjustment (switcher->priv->menu_notifications);

  gtk_adjustment_set_value (adj, adj->upper - adj->page_size);
}

static void 
hd_switcher_menu_reset_main_icon (HDSwitcherMenu *switcher)
{
  if (GTK_BIN (switcher->priv->toggle_button)->child != switcher->priv->icon)
  {
    gtk_container_remove (GTK_CONTAINER (switcher->priv->toggle_button),
		    	  GTK_BIN (switcher->priv->toggle_button)->child);

    gtk_container_add (GTK_CONTAINER (switcher->priv->toggle_button),
	               GTK_WIDGET (switcher->priv->icon));
    
    g_object_unref (G_OBJECT (switcher->priv->icon)); 
  }
}	

static void 
hd_switcher_menu_create_notifications_menu (HDSwitcherMenu *switcher)
{
  GtkWidget *menu_item = NULL, *first_item = NULL;
  GtkTreeModel *nm = GTK_TREE_MODEL (switcher->nm);
  GList *children = NULL, *l;
  GtkTreeIter  iter;	

  children =
    hildon_desktop_popup_menu_get_children (switcher->priv->menu_notifications);

  if (gtk_tree_model_iter_n_children (nm, NULL) >= AS_ITEMS_LIMIT)
  {
    hd_switcher_menu_add_clear_notifications_button (switcher);
    first_item = switcher->priv->clear_events_menu_item;
  }
  else
  {
    if (switcher->priv->clear_events_menu_item)
    {
      gtk_widget_destroy (switcher->priv->clear_events_menu_item);
      switcher->priv->clear_events_menu_item = NULL;
    }
  }
  
  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (l->data != switcher->priv->clear_events_menu_item)
      gtk_widget_destroy (GTK_WIDGET (l->data));
  }

  g_list_free (children);

  if (gtk_tree_model_get_iter_first (nm, &iter))
  {
    GdkPixbuf *icon;
    HDSwitcherMenuNotificationGroup *ngroup;
    GHashTable *hints;
    GValue *hint;
    gchar *summary, *body;
    gboolean ack;
    gint id;

    if (switcher->priv->clear_events_menu_item)
    {
      do
      {
	ngroup = NULL;
	      
        gtk_tree_model_get (nm,
                            &iter,
                            HD_NM_COL_ID, &id,
                            HD_NM_COL_HINTS, &hints,
                            -1);

	hint = g_hash_table_lookup (hints, "category");

	if (hint)
	  ngroup = 
	    g_hash_table_lookup (switcher->priv->notification_groups, 
	                         g_value_get_string (hint));

	if (ngroup != NULL)
        {
	  ngroup->notifications = 
            g_list_prepend (ngroup->notifications, GINT_TO_POINTER (id));

	  ngroup->is_active = 
            (g_list_length (ngroup->notifications) > 1);
	}
      }
      while (gtk_tree_model_iter_next (nm, &iter));

      hd_switcher_menu_add_notification_groups (switcher);

      gtk_tree_model_get_iter_first (nm, &iter);
    }
   
    do
    {
      ngroup = NULL;
	    
      gtk_tree_model_get (nm,
                          &iter,
                          HD_NM_COL_ID, &id,
                          HD_NM_COL_ICON, &icon,
                          HD_NM_COL_SUMMARY, &summary,
                          HD_NM_COL_BODY, &body,
                          HD_NM_COL_HINTS, &hints,
                          HD_NM_COL_ACK, &ack,
                          -1); 

      hint = g_hash_table_lookup (hints, "category");
      
      if (hint)
        ngroup = 
          g_hash_table_lookup (switcher->priv->notification_groups, 
                               g_value_get_string (hint));

      if (ngroup == NULL || !ngroup->is_active)
      {
        if (first_item)
          hildon_desktop_popup_menu_add_item
            (switcher->priv->menu_notifications,
            GTK_MENU_ITEM (gtk_separator_menu_item_new ()));

        menu_item =
          hd_switcher_menu_item_new_from_notification
           (id, icon, summary, body, TRUE);

        gtk_widget_set_name (GTK_WIDGET (menu_item), 
      		             HD_SWITCHER_MENU_NOT_MENU_ITEM_NAME);

        hd_switcher_menu_item_set_blinking (HD_SWITCHER_MENU_ITEM (menu_item), !ack);

        hildon_desktop_popup_menu_add_item
         (switcher->priv->menu_notifications, GTK_MENU_ITEM (menu_item));

        if (!ack)
          hd_switcher_menu_replace_blinking_icon (switcher, icon);

        if (!first_item)
	  first_item = menu_item;
      }
    }
    while (gtk_tree_model_iter_next (nm, &iter));

    hd_switcher_menu_reset_notification_groups (switcher);
    
    hd_switcher_menu_scroll_to (NULL, switcher);
  }

  hd_switcher_menu_check_content (switcher); 
}

static void 
hd_switcher_menu_create_applications_menu (HDSwitcherMenu *switcher, HDWM *hdwm)
{
  GList *children = NULL, *apps = NULL, *l;
  GtkWidget *separator = NULL;

  children =
    hildon_desktop_popup_menu_get_children (switcher->priv->menu_applications);

  for (l = children; l != NULL; l = g_list_next (l))
  {
    HDWMEntryInfo *info;

    if (GTK_IS_SEPARATOR_MENU_ITEM (l->data))
      info = NULL;
    else 
      info = 
        hd_switcher_menu_item_get_entry_info 
	  (HD_SWITCHER_MENU_ITEM (l->data));
 
    if (info != hd_wm_get_home_info (switcher->hdwm))
      gtk_widget_destroy (GTK_WIDGET (l->data));
  }

  g_list_free (children);

  apps = g_list_reverse (g_list_copy (hd_wm_get_applications (hdwm)));

  for (l = apps; l != NULL; l = l->next)
  {
    GtkWidget *menu_item;
    const GList * children = hd_wm_entry_info_get_children(l->data);
    const GList * child;

    if (l == apps)
    {
      separator = gtk_separator_menu_item_new ();

      hildon_desktop_popup_menu_add_item
       (switcher->priv->menu_applications, GTK_MENU_ITEM (separator));
    }

    for (child = children; child != NULL; child = child->next)
    {
      HDWMEntryInfo *entry = child->data;

      menu_item = hd_switcher_menu_item_new_from_entry_info (entry, TRUE);

      gtk_widget_set_name (GTK_WIDGET (menu_item), 
      		           HD_SWITCHER_MENU_APP_MENU_ITEM_NAME);

      g_signal_connect_after (menu_item,
		              "activate",
			      G_CALLBACK (hd_switcher_menu_item_activated),
			      (gpointer)switcher);

      hildon_desktop_popup_menu_add_item
       (switcher->priv->menu_applications, GTK_MENU_ITEM (menu_item));

      if (hd_wm_entry_info_is_active (entry))
        hildon_desktop_popup_menu_select_item 
          (switcher->priv->menu_applications, GTK_MENU_ITEM (menu_item));		

      switcher->priv->active_menu_item = menu_item;
    }

    /* Append the separator for this app*/
    if (l->next != NULL)
    {
      separator = gtk_separator_menu_item_new ();

      hildon_desktop_popup_menu_add_item
        (switcher->priv->menu_applications, GTK_MENU_ITEM (separator));
    }
  }

  g_list_free (apps);

  hd_switcher_menu_check_content (switcher); 

  gtk_widget_queue_draw (GTK_WIDGET (switcher->priv->menu_applications));
}

static void 
hd_switcher_menu_add_info_cb (HDWM *hdwm, 
			      HDWMEntryInfo *info,
			      HDSwitcherMenu *switcher)
{
  if (!info)
    return;

  hd_switcher_menu_create_applications_menu (switcher, hdwm);
}

static void 
hd_switcher_menu_remove_info_cb (HDWM *hdwm,
				 gboolean removed_app,
				 HDWMEntryInfo *info,
				 HDSwitcherMenu *switcher)
{
  GList *children = NULL, *l;
  HDWMEntryInfo *_info;

  children =
    hildon_desktop_popup_menu_get_children (switcher->priv->menu_applications);

  for (l = children; l != NULL; l = g_list_next (l))
  {
    GtkMenuItem *separator = NULL;
	  
    if (!GTK_IS_SEPARATOR_MENU_ITEM (l->data))
    {	    
      _info = hd_switcher_menu_item_get_entry_info (HD_SWITCHER_MENU_ITEM (l->data));

      if (_info && _info == info)
      {	    
        hildon_desktop_popup_menu_remove_item (switcher->priv->menu_applications,
	  				       GTK_MENU_ITEM (l->data));

        if (l == children)
	{
          if (l->next && GTK_IS_SEPARATOR_MENU_ITEM (l->next->data))
	    separator = GTK_MENU_ITEM (l->next->data);
	}
	else
        {
	  if (l->prev && GTK_IS_SEPARATOR_MENU_ITEM (l->prev->data))
	    separator = GTK_MENU_ITEM (l->prev->data);
	}

	if (separator != NULL)
          hildon_desktop_popup_menu_remove_item (switcher->priv->menu_applications,
	  				         separator);

	if (info == switcher->priv->last_urgent_info)
        {
          hd_switcher_menu_reset_main_icon (switcher);
	  switcher->priv->last_urgent_info = NULL;
        }		
      
        break;
      }
    }
  }

  g_list_free (children);

  hd_switcher_menu_changed_stack_cb (switcher->hdwm, NULL, switcher);  

  hd_switcher_menu_check_content (switcher);
}

static GdkPixbuf *
hd_switcher_get_default_icon_from_entry_info (HDSwitcherMenu *switcher, HDWMEntryInfo *info)
{
  GdkPixbuf *app_pixbuf = hd_wm_entry_info_get_icon (info);
 
  if (!app_pixbuf)
  {
    GError *error = NULL;

    app_pixbuf = hd_wm_entry_info_get_app_icon (info,
                                             AS_ICON_THUMB_SIZE,
                                             &error);
    if (error)
    {
      g_error_free (error);
      error = NULL;
      
      app_pixbuf = gtk_icon_theme_load_icon (switcher->priv->icon_theme,
                                             AS_MENU_DEFAULT_APP_ICON,
                                             AS_ICON_THUMB_SIZE,
                                             GTK_ICON_LOOKUP_NO_SVG,
                                             &error);

      if (error)
      {
        g_warning ("Could not load icon %s from theme: %s.",
                   AS_MENU_DEFAULT_APP_ICON,
                   error->message);
        g_error_free (error);
      }
    }
  }

  return app_pixbuf;
}	

static void 
hd_switcher_menu_changed_info_cb (HDWM *hdwm,
				  HDWMEntryInfo *info,
				  HDSwitcherMenu *switcher)
{
  GtkWidget *menu_item = NULL;
  GList *children = NULL, *apps = NULL, *l;
  gint pos=0;
  gboolean make_it_blink = FALSE;

  if (!info)
    return;	  

  /* We have to guess whether it is in app switcher's slots or not*/
  
  if (HD_WM_IS_APPLICATION (info))
  {	  
     apps = hd_wm_get_applications (switcher->hdwm);

     for (l = apps; l != NULL; l = g_list_next (l))
     {
        HDWMEntryInfo *iter_info = (HDWMEntryInfo *) l->data;

	if (!HD_WM_IS_APPLICATION (iter_info))
          continue;
 	
        if (HD_WM_IS_APPLICATION (iter_info))		
        {
	  pos++;

	  if (iter_info == info)
            break;		  
        }		
     }

     if (pos >= SWITCHER_N_SLOTS)
       make_it_blink = TRUE;	    
  }     
  else
  {
     apps = hd_wm_get_applications (switcher->hdwm);

     for (l = apps; l != NULL; l = g_list_next (l))
     {
       const GList *iter_children;	     
       HDWMEntryInfo *iter_info = (HDWMEntryInfo *) l->data;
       
       if (HD_WM_IS_APPLICATION (iter_info))
       {
         pos++;	
  	 
         const GList *info_children = 
  	   hd_wm_entry_info_get_children (iter_info);
	 
	 for (iter_children = info_children; 
 	      iter_children != NULL; 
	      iter_children = g_list_next (iter_children))
         {
           if (iter_children->data == info)
	     break;	   
         }		 
       }	       
     }	     

     if (pos >= SWITCHER_N_SLOTS)
       make_it_blink = TRUE;	     
  }	  
	  
  children =
    hildon_desktop_popup_menu_get_children (switcher->priv->menu_applications);

  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (!GTK_IS_SEPARATOR_MENU_ITEM (l->data))
    {	   
      HDWMEntryInfo *_info = 
        hd_switcher_menu_item_get_entry_info (HD_SWITCHER_MENU_ITEM (l->data));
 
      if (_info == info)
      {
        menu_item = GTK_WIDGET (l->data);    
        break;
      }
    }
  }

  g_list_free (children);

  if (menu_item)
  {
    hd_switcher_menu_item_set_entry_info (HD_SWITCHER_MENU_ITEM (menu_item), info);
  }   
  
  if (!hd_wm_entry_info_is_urgent (info) &&
       hd_wm_entry_info_get_ignore_urgent (info))
  {
     hd_wm_entry_info_set_ignore_urgent (info, FALSE);
     return;
  }
  else
  if (!hd_wm_entry_info_is_urgent (info) &&      /* We were told to change appswitcher icon with */
      switcher->priv->last_urgent_info == info)	 /* application's one. Now we've been told to change it back*/
  {
    hd_switcher_menu_reset_main_icon (switcher);	  
    switcher->priv->last_urgent_info = NULL;	  
  }	  

  if (hd_wm_entry_info_is_urgent (info) &&
      !hd_wm_entry_info_get_ignore_urgent (info))
  {
    if (menu_item)
    {
      /* child of one of the app buttons */
      if (!hd_switcher_menu_item_is_blinking (HD_SWITCHER_MENU_ITEM (menu_item)))
        hd_switcher_menu_item_set_blinking (HD_SWITCHER_MENU_ITEM (menu_item), TRUE);

      if (make_it_blink)
      {	      
        hd_switcher_menu_replace_blinking_icon 
  	  (switcher, hd_switcher_get_default_icon_from_entry_info (switcher,info));
	
	switcher->priv->last_urgent_info = info;
      }
    }
  }
}

static void 
hd_switcher_menu_changed_stack_cb (HDWM *hdwm,
				   HDWMEntryInfo *info,
				   HDSwitcherMenu *switcher)
{
  GList *children = NULL, *l;

  children =
    hildon_desktop_popup_menu_get_children (switcher->priv->menu_applications);

  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (HD_IS_SWITCHER_MENU_ITEM (l->data))
    {
      HDWMEntryInfo *info =	    
        hd_switcher_menu_item_get_entry_info (HD_SWITCHER_MENU_ITEM (l->data));

      if (hd_wm_entry_info_is_active (info))
      {	      
        hildon_desktop_popup_menu_select_item
          (switcher->priv->menu_applications, GTK_MENU_ITEM (l->data));
	
	switcher->priv->active_menu_item = GTK_WIDGET (l->data);
	break;
      }
    }	    
  }	  
}

static void 
hd_switcher_menu_show_menu_cb (HDWM *hdwm, HDSwitcherMenu *switcher)
{
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (switcher->priv->toggle_button), TRUE);

  g_signal_emit_by_name (switcher->priv->toggle_button, "toggled");
}

static gboolean
hd_switcher_menu_auto_attach (HDSwitcherMenu *switcher)
{
  hd_switcher_menu_attach_button (switcher);

  return FALSE;
}	

static void 
hd_switcher_menu_dettach_button (HDSwitcherMenu *switcher)
{
  if (SWITCHER_TOGGLE_BUTTON != NULL)
  {
    gtk_widget_reparent (switcher->priv->toggle_button, switcher->priv->window_dialog);	    
    gtk_widget_show (switcher->priv->toggle_button);
    
    gtk_widget_show (GTK_WIDGET (switcher->priv->window_dialog));

    SWITCHER_TOGGLE_BUTTON = NULL;

    g_timeout_add (SWITCHER_DETTACHED_TIMEOUT,
		   (GSourceFunc)hd_switcher_menu_auto_attach,
		   (gpointer)switcher);
  }
}

static void 
hd_switcher_menu_attach_button (HDSwitcherMenu *switcher)
{
  if (SWITCHER_TOGGLE_BUTTON == NULL)
  {
    gtk_widget_reparent (switcher->priv->toggle_button, GTK_WIDGET (switcher));
    gtk_widget_show (switcher->priv->toggle_button);
    
    gtk_widget_hide (switcher->priv->window_dialog); 	
  }
}	

static void 
hd_switcher_menu_fullscreen_cb (HDWM *hdwm, gboolean fullscreen, HDSwitcherMenu *switcher)
{
  switcher->priv->fullscreen = fullscreen;

  GList *children =
    hildon_desktop_popup_menu_get_children (switcher->priv->menu_notifications);   	  

  hd_switcher_create_external_window (switcher);
  
  if (children)  
  {	  
    if (fullscreen && GTK_BIN (switcher->priv->toggle_button)->child != switcher->priv->icon)
      hd_switcher_menu_dettach_button (switcher);
    else
      hd_switcher_menu_attach_button (switcher);

    g_list_free (children);    
  }
  
  if (!fullscreen)
    hd_switcher_menu_attach_button (switcher);	   
}	

static void 
hd_switcher_menu_long_press_cb (HDWM *hdwm, HDSwitcherMenu *switcher)
{
  if (switcher->priv->is_open)
  {
    hildon_desktop_popup_menu_activate_item 
      (switcher->priv->menu_applications, 
       hildon_desktop_popup_menu_get_selected_item 
         (switcher->priv->menu_applications));
  }
  else
    hd_wm_top_desktop ();
}

static void 
hd_switcher_menu_notification_changed_cb (GtkTreeModel   *tree_model,
                                        GtkTreePath    *path,
                                        GtkTreeIter    *iter,
                                        HDSwitcherMenu *switcher)
{
  GHashTable *hints;
  const gchar *category = NULL, *sound_file = NULL;
  GValue *hint;
  guint id;
  
  GtkWidget *pane_notifications = 
   hildon_desktop_popup_window_get_pane 
     (switcher->priv->popup_window, 1);
  
  if (switcher->priv->is_open && !GTK_WIDGET_MAPPED (pane_notifications))
    gtk_widget_show (pane_notifications);	  

  gtk_tree_model_get (tree_model,
		      iter,
		      HD_NM_COL_ID, &id,
		      HD_NM_COL_HINTS, &hints,
		      -1);

  hint = g_hash_table_lookup (hints, "category");

  if (hint)
  {	  
    category = g_value_get_string (hint);

    if (g_str_has_prefix (category, "system.note"))
      goto out;	  
  }

  hint = g_hash_table_lookup (hints, "sound-file");

  if (hint)
  {
    gint sample_id;

    sound_file = g_value_get_string (hint);

    sample_id = hn_as_sound_register_sample (switcher->priv->esd_socket,
                                             sound_file);

    if (hn_as_sound_play_sample (switcher->priv->esd_socket, 
			         sample_id) == -1)
    {
      hn_as_sound_deinit (switcher->priv->esd_socket);
      
      switcher->priv->esd_socket = hn_as_sound_init ();

      hn_as_sound_play_sample (switcher->priv->esd_socket, 
		               sample_id);
    }

    hn_as_sound_deregister_sample (switcher->priv->esd_socket, 
		                   sample_id);
  }

#ifdef HAVE_MCE
  hint = g_hash_table_lookup (hints, "led-pattern");

  if (hint)
  {
    hd_switcher_menu_set_led_pattern (g_value_get_string (hint), TRUE);
  }
#endif
  
/*  
  if (switcher->priv->last_iter_added == NULL)
    hd_switcher_menu_notification_deleted_cb 
      (HILDON_DESKTOP_NOTIFICATION_MANAGER (tree_model), id, switcher);
*/
      
  switcher->priv->last_urgent_info = NULL;

  hd_switcher_menu_create_notifications_menu (switcher);
  
out:    	    
  switcher->priv->last_iter_added = NULL;
}

static void 
hd_switcher_menu_notification_deleted_cb (HildonDesktopNotificationManager   *nm,
                                          gint 		  id,
                                          HDSwitcherMenu *switcher)
{
  GList *children = NULL, *l;
  
  children =
    hildon_desktop_popup_menu_get_children (switcher->priv->menu_notifications);    	  
  
  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (HD_IS_SWITCHER_MENU_ITEM (l->data))	  
    {
      GtkMenuItem *menu_item = NULL, *separator = NULL;
      gint _id = 
        hd_switcher_menu_item_get_notification_id (HD_SWITCHER_MENU_ITEM (l->data));
      
      if ((_id != -1 && _id == id) ||
          (hd_switcher_menu_item_has_id (HD_SWITCHER_MENU_ITEM (l->data), id)))      
      {
        menu_item = GTK_MENU_ITEM (l->data);
      }

      if (menu_item)
      {
        hildon_desktop_popup_menu_remove_item (switcher->priv->menu_notifications,
	                                       menu_item);

        if (l == children)
	{
          if (l->next && GTK_IS_SEPARATOR_MENU_ITEM (l->next->data))
	    separator = GTK_MENU_ITEM (l->next->data);
	}
	else
        {
	  if (l->prev && GTK_IS_SEPARATOR_MENU_ITEM (l->prev->data))
	    separator = GTK_MENU_ITEM (l->prev->data);
	}

	if (separator != NULL)
          hildon_desktop_popup_menu_remove_item (switcher->priv->menu_notifications,
	  				         separator);

        break;
      }
    }
  }

  if (gtk_tree_model_iter_n_children (GTK_TREE_MODEL (switcher->nm), NULL) <= AS_ITEMS_LIMIT)
  {
    if (switcher->priv->clear_events_menu_item)
    {
      GList *clear_child;

      clear_child = g_list_last (children);
      
      gtk_widget_destroy (switcher->priv->clear_events_menu_item);
      switcher->priv->clear_events_menu_item = NULL;
      
      if (clear_child->prev && GTK_IS_SEPARATOR_MENU_ITEM (clear_child->prev->data))
        hildon_desktop_popup_menu_remove_item (switcher->priv->menu_notifications,
        				       GTK_MENU_ITEM (clear_child->prev->data));
    }
  }
 
  g_list_free (children);

  hd_switcher_menu_check_content (switcher);
}

static void 
hd_switcher_menu_notification_added_cb   (GtkTreeModel   *tree_model,
                                          GtkTreePath    *path,
                                          GtkTreeIter    *iter,
                                          HDSwitcherMenu *switcher)
{
   switcher->priv->last_iter_added = iter;	
}
