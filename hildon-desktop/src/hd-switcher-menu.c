 /*
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
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

/* Hildon includes */
#include "hd-switcher-menu.h"
#include "hd-switcher-menu-item.h"
#include "hn-app-pixbuf-anim-blinker.h"

#include <gdk/gdkx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <gtk/gtkimage.h>
#include <libhildondesktop/hildon-desktop-popup-window.h>
#include <libhildondesktop/hildon-desktop-popup-menu.h>

/* Menu item strings */
#define AS_HOME_ITEM 		_("tana_fi_home")
#define AS_HOME_ITEM_ICON 	"qgn_list_home"

/* Defined workarea atom */
#define WORKAREA_ATOM "_NET_WORKAREA"

#define AS_MENU_BUTTON_ICON      "qgn_list_tasknavigator_appswitcher"
#define AS_MENU_DEFAULT_APP_ICON "qgn_list_gene_default_app"

#define ANIM_DURATION 5000 	/* 5 Secs for blinking icons */
#define ANIM_FPS      2

#define AS_MENU_BUTTON_NAME "hildon-navigator-small-button5"

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

enum 
{
  PROP_MENU_NITEMS=1,
};

#define HD_SWITCHER_MENU_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HD_TYPE_SWITCHER_MENU, HDSwitcherMenuPrivate))

G_DEFINE_TYPE (HDSwitcherMenu, hd_switcher_menu, TASKNAVIGATOR_TYPE_ITEM);

struct _HDSwitcherMenuPrivate
{
  HildonDesktopPopupMenu *menu_applications;
  HildonDesktopPopupMenu *menu_notifications;

  HildonDesktopPopupWindow *popup_window;
  GtkWidget		   *notifications_window;

  GtkImage		   *image_button;

  GtkWidget 		   *active_menu_item;
};

static GObject *hd_switcher_menu_constructor (GType gtype,
                                              guint n_params,
                                              GObjectConstructParam *params);

static void hd_switcher_menu_finalize (GObject *object);

static void hd_switcher_menu_toggled_cb (GtkWidget *button, HDSwitcherMenu *switcher);

static void hd_switcher_menu_scroll_to (HildonDesktopPopupWindow *window,
					HDSwitcherMenu *switcher);

static void hd_switcher_menu_add_info_cb (HDWM *hdwm, HDEntryInfo *info, HDSwitcherMenu *switcher);
static void hd_switcher_menu_remove_info_cb (HDWM *hdwm, gboolean removed_app, HDEntryInfo *info, HDSwitcherMenu *switcher);
static void hd_switcher_menu_changed_info_cb (HDWM *hdwm, HDEntryInfo *info, HDSwitcherMenu *switcher);
static void hd_switcher_menu_changed_stack_cb (HDWM *hdwm, HDEntryInfo *info, HDSwitcherMenu *switcher);
static void hd_switcher_menu_show_menu_cb (HDWM *hdwm, HDSwitcherMenu *switcher);
static void hd_switcher_menu_long_press_cb (HDWM *hdwm, HDSwitcherMenu *switcher);

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

static void 
hd_switcher_menu_init (HDSwitcherMenu *switcher)
{
  switcher->priv = HD_SWITCHER_MENU_GET_PRIVATE (switcher);

  switcher->priv->menu_applications  = 
  switcher->priv->menu_notifications = NULL;

  switcher->priv->popup_window = NULL;

  switcher->priv->active_menu_item = NULL;

  switcher->hdwm = hd_wm_get_singleton ();

  switcher->nm = 
    HILDON_DESKTOP_NOTIFICATION_MANAGER 
     (hildon_desktop_notification_manager_get_singleton ());

  g_object_ref (switcher->hdwm);
}

static void 
hd_switcher_menu_class_init (HDSwitcherMenuClass *switcher)
{
  GObjectClass *object_class = G_OBJECT_CLASS (switcher);

  object_class->constructor = hd_switcher_menu_constructor;
  object_class->finalize    = hd_switcher_menu_finalize;

  g_type_class_add_private (object_class, sizeof (HDSwitcherMenuPrivate));
}

static void 
hd_switcher_menu_resize_menu (HildonDesktopPopupMenu *menu, HDSwitcherMenu *switcher)
{ 
  hildon_desktop_popup_recalculate_position (switcher->priv->popup_window);  
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

  object = G_OBJECT_CLASS (hd_switcher_menu_parent_class)->constructor (gtype,
           	                                                        n_params,
                                                                        params);

  switcher = HD_SWITCHER_MENU (object);

  gtk_widget_push_composite_child ();

  button = gtk_toggle_button_new ();
  
  gtk_container_add (GTK_CONTAINER (object), button);
  gtk_widget_show (button);

  switcher->priv->popup_window =
    HILDON_DESKTOP_POPUP_WINDOW 
      (hildon_desktop_popup_window_new (1,GTK_ORIENTATION_HORIZONTAL,HD_POPUP_WINDOW_DIRECTION_RIGHT_BOTTOM));	     
  
  switcher->priv->notifications_window = 
    hildon_desktop_popup_window_get_pane (switcher->priv->popup_window, 0);

  gtk_widget_set_size_request (GTK_WIDGET (switcher->priv->popup_window),
			       340,100);
  
  gtk_widget_set_size_request (GTK_WIDGET (switcher->priv->notifications_window),
			       340,100);  

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
		    G_CALLBACK (hd_switcher_menu_scroll_to),
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
                    "show_menu",
                    G_CALLBACK (hd_switcher_menu_show_menu_cb),
                    (gpointer)switcher);

  g_signal_connect (switcher->hdwm,
                    "long-key-press",
                    G_CALLBACK (hd_switcher_menu_long_press_cb),
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
 
  hildon_desktop_popup_menu_add_item
   (switcher->priv->menu_applications, GTK_MENU_ITEM (menu_item));

  return object;
}

static void 
hd_switcher_menu_finalize (GObject *object)
{
  HDSwitcherMenu *switcher = HD_SWITCHER_MENU (object);	
	
  g_object_unref (switcher->hdwm);

  gtk_widget_destroy (GTK_WIDGET (switcher->priv->popup_window));
	
  G_OBJECT_CLASS (hd_switcher_menu_parent_class)->finalize (object);
}	

static void
hd_switcher_menu_get_workarea (GtkAllocation *allocation)
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

  if (!GTK_WIDGET_REALIZED (GTK_WIDGET (data)))
    return;

  hd_switcher_menu_get_workarea (&workarea);

  top_level = gtk_widget_get_toplevel (button);

  if (HILDON_DESKTOP_IS_PANEL_WINDOW (top_level))
  {
    g_object_get (top_level, "orientation", &orientation, NULL);
  }

  gtk_widget_size_request (GTK_WIDGET (menu), &req);

  menu_height = req.height;
  main_height = gdk_screen_get_height (screen);

  if (FALSE)
  {
    *x = 0;
    *y = MAX (0, (main_height - menu_height));
  }
  else
  {
    switch (orientation)
    {
      case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT:
        *x = workarea.x;

        if (main_height - button->allocation.y < menu_height)
          *y = MAX (0, (main_height - menu_height));
        else
          *y = button->allocation.y;
        break;

      case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT:
        *x = workarea.x + workarea.width - req.width;

        if (main_height - button->allocation.y < menu_height)
          *y = MAX (0, (main_height - menu_height));
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
}


static void 
hd_switcher_menu_toggled_cb (GtkWidget *button, HDSwitcherMenu *switcher)
{
  hildon_desktop_popup_window_popup 
   (switcher->priv->popup_window,
    hd_switcher_menu_position_func,
    (gpointer)switcher,
    GDK_CURRENT_TIME);

  g_debug ("actual size of window: w: %d h: %d", 
  	   GTK_WIDGET (switcher->priv->popup_window)->allocation.width,
	   GTK_WIDGET (switcher->priv->popup_window)->allocation.height);
}	

static void 
hd_switcher_menu_scroll_to (HildonDesktopPopupWindow *window,
			    HDSwitcherMenu *switcher)
{
   if (!switcher->priv->menu_applications ||
       !switcher->priv->menu_notifications)
   {
     return;	   
   }	   

   hildon_desktop_popup_menu_scroll_to_selected
    (switcher->priv->menu_applications);

   hildon_desktop_popup_menu_scroll_to_selected
    (switcher->priv->menu_notifications);
}

static void 
hd_switcher_menu_create_menu (HDSwitcherMenu *switcher, HDWM *hdwm)
{
  GList *children = NULL, *l;
  GtkWidget *separator;

  children =
    hildon_desktop_popup_menu_get_children (switcher->priv->menu_applications);

  for (l = children; l != NULL; l = g_list_next (l))
  {
    HDEntryInfo *info;

    if (GTK_IS_SEPARATOR_MENU_ITEM (l->data))
      info = NULL;
    else 
      info = 
        hd_switcher_menu_item_get_entry_info 
	  (HD_SWITCHER_MENU_ITEM (l->data));

    if (info != hd_wm_get_home_info (hdwm))
      gtk_widget_destroy (GTK_WIDGET (l->data));
  }

  g_list_free (children);

  for (l = hd_wm_get_applications (hdwm); l != NULL; l = l->next)
  {
    GtkWidget *menu_item;
    const GList * children = hd_entry_info_get_children(l->data);
    const GList * child;

    for (child = children; child != NULL; child = child->next)
    {
      if (l == hd_wm_get_applications (hdwm) && child == children)
      {
        separator = gtk_separator_menu_item_new ();

        hildon_desktop_popup_menu_add_item
         (switcher->priv->menu_applications, GTK_MENU_ITEM (separator));
      }	      
	    
      HDEntryInfo *entry = child->data;

      g_debug ("Creating new app menu item %s",
               hd_entry_info_peek_title (entry));

      menu_item = hd_switcher_menu_item_new_from_entry_info (entry, TRUE);

      if (hd_entry_info_is_active (entry))
        hildon_desktop_popup_menu_select_item 
          (switcher->priv->menu_applications, GTK_MENU_ITEM (menu_item));		

      hildon_desktop_popup_menu_add_item
       (switcher->priv->menu_applications, GTK_MENU_ITEM (menu_item));
    }

    /* append the separator for this app*/
    separator = gtk_separator_menu_item_new ();

    hildon_desktop_popup_menu_add_item
      (switcher->priv->menu_applications, GTK_MENU_ITEM (separator));
  }
}	

static void 
hd_switcher_menu_add_info_cb (HDWM *hdwm, 
			      HDEntryInfo *info,
			      HDSwitcherMenu *switcher)
{
/*  GtkWidget *menu_item;*/

  if (!info)
    return;	  

  hd_switcher_menu_create_menu (switcher, hdwm);
/*
  menu_item = gtk_separator_menu_item_new ();

  hildon_desktop_popup_menu_add_item
    (switcher->priv->menu_applications, GTK_MENU_ITEM (menu_item));
  
  menu_item = hd_switcher_menu_item_new_from_entry_info (info, TRUE);

  hildon_desktop_popup_menu_add_item 
    (switcher->priv->menu_applications, GTK_MENU_ITEM (menu_item));*/
}

static void 
hd_switcher_menu_remove_info_cb (HDWM *hdwm,
				 gboolean removed_app,
				 HDEntryInfo *info,
				 HDSwitcherMenu *switcher)
{
  GList *children = NULL, *l;
  HDEntryInfo *_info;

  children =
    hildon_desktop_popup_menu_get_children (switcher->priv->menu_applications);

  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (!GTK_IS_SEPARATOR_MENU_ITEM (l->data))
    {	    
      _info = hd_switcher_menu_item_get_entry_info (HD_SWITCHER_MENU_ITEM (l->data));

      if (_info && _info == info)
      {	    
        hildon_desktop_popup_menu_remove_item (switcher->priv->menu_applications,
	  				       GTK_MENU_ITEM (l->data));

        if (l->prev && GTK_IS_SEPARATOR_MENU_ITEM (l->prev->data))
          hildon_desktop_popup_menu_remove_item (switcher->priv->menu_applications,
                                                 GTK_MENU_ITEM (l->prev->data));		
      
        break;
      }
    }
  }

  g_list_free (children);  
}

static void 
hd_switcher_menu_changed_info_cb (HDWM *hdwm,
				  HDEntryInfo *info,
				  HDSwitcherMenu *switcher)
{
  GtkWidget *menu_item = NULL;
  GList *children = NULL, *l;

  children =
    hildon_desktop_popup_menu_get_children (switcher->priv->menu_applications);
  /*TODO: Find the menu item */

  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (!GTK_IS_SEPARATOR_MENU_ITEM (l->data))
    {	   
      HDEntryInfo *_info = 
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
  
  if (!hd_entry_info_is_urgent (info) &&
       hd_entry_info_get_ignore_urgent (info))
  {
     hd_entry_info_set_ignore_urgent (info, FALSE);
     return;
  }

  if (hd_entry_info_is_urgent (info) &&
      !hd_entry_info_get_ignore_urgent (info))
  {
    if (menu_item)
    {
      /* child of one of the app buttons */
      if (!hd_switcher_menu_item_get_is_blinking (HD_SWITCHER_MENU_ITEM (menu_item)))
        hd_switcher_menu_item_set_is_blinking (HD_SWITCHER_MENU_ITEM (menu_item), TRUE);
    }
  }
}

static void 
hd_switcher_menu_changed_stack_cb (HDWM *hdwm,
				   HDEntryInfo *info,
				   HDSwitcherMenu *switcher)
{

}

static void 
hd_switcher_menu_show_menu_cb (HDWM *hdwm, HDSwitcherMenu *switcher)
{

}

static void 
hd_switcher_menu_long_press_cb (HDWM *hdwm, HDSwitcherMenu *switcher)
{

}

static void 
hd_switcher_menu_notification_changed_cb (GtkTreeModel   *tree_model,
                                        GtkTreePath    *path,
                                        GtkTreeIter    *iter,
                                        HDSwitcherMenu *switcher)
{
  GdkPixbuf *icon = NULL;
  gchar *summary = NULL, *body = NULL;
  guint id;
  GtkWidget *menu_item;
	

  gtk_tree_model_get (tree_model,
		      iter,
		      HD_NM_COL_ID, &id,
		      HD_NM_COL_ICON, &icon,
		      HD_NM_COL_SUMMARY, &summary,
		      HD_NM_COL_BODY, &body,
		      -1);
  g_debug ("Summary %s %s --------->",summary,body);
  menu_item = 
    hd_switcher_menu_item_new_from_notification 
      (id, icon, summary, body, TRUE);

  hildon_desktop_popup_menu_add_item
    (switcher->priv->menu_notifications, GTK_MENU_ITEM (menu_item));
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
    gint _id =
      hd_switcher_menu_item_get_notification_id (HD_SWITCHER_MENU_ITEM (l->data));	    
	  
    if (_id == id)      
    {
      hildon_desktop_popup_menu_remove_item (switcher->priv->menu_notifications,
	                                     GTK_MENU_ITEM (l->data));
      break;
    }	    
  }	  

  g_list_free (children);
}

static void 
hd_switcher_menu_notification_added_cb   (GtkTreeModel   *tree_model,
                                          GtkTreePath    *path,
                                          GtkTreeIter    *iter,
                                          HDSwitcherMenu *switcher)
{

}
