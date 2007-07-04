/*
 * Copyright (C) 2006,2007 Nokia Corporation.
 *
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

/* Hildon includes */
#include "hd-switcher-menu-item.h"
#include "hn-app-pixbuf-anim-blinker.h"
#include "hd-config.h"

#include <libhildondesktop/hildon-desktop-notification-manager.h>


/* GLib include */
#include <glib.h>
#include <glib/gi18n.h>

/* GTK includes */
#include <gtk/gtkwidget.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtkseparatormenuitem.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmisc.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

/* GDK includes */
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

/* X includes */
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* Menu item strings */
#define AS_HOME_ITEM 		_("tana_fi_home")
#define AS_HOME_ITEM_ICON 	"qgn_list_home"

#define AS_MENU_DEFAULT_APP_ICON "qgn_list_gene_default_app"

#define ANIM_DURATION 5000 	/* 5 Secs for blinking icons */
#define ANIM_FPS      2

/* Hardcoded pixel perfecting values */
#define AS_BUTTON_BORDER_WIDTH  0
#define AS_MENU_BORDER_WIDTH    20
#define AS_TIP_BORDER_WIDTH 	20
#define AS_BUTTON_HEIGHT        38
#define AS_ROW_HEIGHT 		30
#define AS_ICON_SIZE            26
#define AS_CLOSE_BUTTON_SIZE    16
#define AS_CLOSE_BUTTON_THUMB_SIZE 40
#define AS_TOOLTIP_WIDTH        360
#define AS_MENU_ITEM_WIDTH      360
#define AS_INTERNAL_PADDING     10
#define AS_SEPARATOR_HEIGHT     10
#define AS_BUTTON_BOX_PADDING   10

/*
 * HDSwitcherMenuItem
 */

enum
{
  MENU_PROP_0,
  MENU_PROP_ENTRY_INFO,
  MENU_PROP_SHOW_CLOSE,
  MENU_PROP_IS_BLINKING,
  MENU_PROP_NOT_ID,
  MENU_PROP_NOT_IDS,
  MENU_PROP_NOT_SUMMARY,
  MENU_PROP_NOT_BODY,
  MENU_PROP_NOT_ICON,
  MENU_PROP_DBUS_CALLBACK
};

#define HD_SWITCHER_MENU_ITEM_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HD_TYPE_SWITCHER_MENU_ITEM, HDSwitcherMenuItemPrivate))

struct _HDSwitcherMenuItemPrivate
{
  GtkWidget *icon;
  GtkWidget *label;
  GtkWidget *label2;
  GtkWidget *close;

  GtkIconTheme *icon_theme;
  
  guint show_close  : 1;
  guint is_blinking : 1;

  GdkPixbufAnimation *pixbuf_anim;
  
  HDWMEntryInfo *info;

  HNAppPixbufAnimBlinker *blinker;

  gint       notification_id;
  GList     *notification_ids;
  gchar     *notification_summary;
  gchar     *notification_body;
  GdkPixbuf *notification_icon;

  gchar     *dbus_callback;
  
  gboolean   was_topped;

  HildonDesktopNotificationManager *nm;
};

G_DEFINE_TYPE (HDSwitcherMenuItem, hd_switcher_menu_item, GTK_TYPE_MENU_ITEM);

static void
hd_switcher_menu_item_finalize (GObject *gobject)
{
  HDSwitcherMenuItemPrivate *priv = HD_SWITCHER_MENU_ITEM (gobject)->priv;
  
  if (priv->pixbuf_anim)
    g_object_unref (priv->pixbuf_anim); 

  if (priv->notification_icon)
    g_object_unref (priv->notification_icon);

  if (priv->notification_summary)
    g_free (priv->notification_summary);

  if (priv->notification_body)
    g_free (priv->notification_body);	  

  if (priv->notification_ids)
    g_list_free (priv->notification_ids);
  
  if (priv->dbus_callback)
    g_free (priv->dbus_callback);	  

  G_OBJECT_CLASS (hd_switcher_menu_item_parent_class)->finalize (gobject);
}

static void
hd_switcher_menu_item_set_property (GObject      *gobject,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  HDSwitcherMenuItemPrivate *priv = HD_SWITCHER_MENU_ITEM (gobject)->priv;

  switch (prop_id)
  {
    case MENU_PROP_ENTRY_INFO:
      priv->info = g_value_get_pointer (value);
      break;
      
    case MENU_PROP_SHOW_CLOSE:
      priv->show_close = g_value_get_boolean (value);
      break;
      
    case MENU_PROP_IS_BLINKING:
      priv->is_blinking = g_value_get_boolean (value);
      break;
    
    case MENU_PROP_NOT_ID:
      priv->notification_id = g_value_get_int (value);
      break;
      
    case MENU_PROP_NOT_IDS:
      priv->notification_ids = g_value_get_pointer (value);
      break;
      
    case MENU_PROP_NOT_SUMMARY:
      priv->notification_summary = g_strdup (g_value_get_string (value));
      break;
      
    case MENU_PROP_NOT_BODY:
      priv->notification_body = g_strdup (g_value_get_string (value));
      break; 
      
    case MENU_PROP_NOT_ICON:
      priv->notification_icon = g_value_get_object (value);
      break;
      
    case MENU_PROP_DBUS_CALLBACK:
      priv->dbus_callback = g_strdup (g_value_get_string (value));
      break; 
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
hd_switcher_menu_item_get_property (GObject    *gobject,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  HDSwitcherMenuItemPrivate *priv = HD_SWITCHER_MENU_ITEM (gobject)->priv;

  switch (prop_id)
  {
    case MENU_PROP_ENTRY_INFO:
      g_value_set_pointer (value, priv->info);
      break;
      
    case MENU_PROP_SHOW_CLOSE:
      g_value_set_boolean (value, priv->show_close);
      break;
      
    case MENU_PROP_IS_BLINKING:
      g_value_set_boolean (value, priv->is_blinking);
      break;
    
    case MENU_PROP_NOT_ID:
      g_value_set_int (value, priv->notification_id);
      break;
      
    case MENU_PROP_NOT_IDS:
      g_value_set_pointer (value, priv->notification_ids);
      break;
      
    case MENU_PROP_NOT_SUMMARY:
      g_value_set_string (value, priv->notification_summary);
      break;
      
    case MENU_PROP_NOT_BODY:
      g_value_set_string (value, priv->notification_body);
      break; 
      
    case MENU_PROP_NOT_ICON:
      g_value_set_object (value, priv->notification_icon);
      break;
      
    case MENU_PROP_DBUS_CALLBACK:
      g_value_set_string (value, priv->dbus_callback);
      break; 
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
hd_switcher_menu_item_icon_animation (GtkWidget *icon,
	       			 gboolean   is_on)
{
  GdkPixbuf *pixbuf;
  GdkPixbufAnimation *pixbuf_anim;
  
  g_return_if_fail (GTK_IS_IMAGE (icon));

  if (is_on)
  {
    pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (icon));
              
    pixbuf_anim = hn_app_pixbuf_anim_blinker_new (pixbuf,
                                                  1000 / ANIM_FPS,
					          -1,
						  10);

    gtk_image_set_from_animation (GTK_IMAGE(icon), pixbuf_anim);

    g_object_unref (pixbuf_anim);
  }
  else
  {
    pixbuf_anim = gtk_image_get_animation (GTK_IMAGE (icon));
    g_debug ("Turning animation off");      
    /* grab static image from menu item and reset */
    pixbuf = gdk_pixbuf_animation_get_static_image (pixbuf_anim);

    gtk_image_set_from_pixbuf (GTK_IMAGE (icon), pixbuf);
    g_object_unref (pixbuf);
  }
}

static GObject *
hd_switcher_menu_item_constructor (GType                  type,
			      	   guint                  n_construct_params,
			           GObjectConstructParam *construct_params)
{
  GObject *gobject;
  HDSwitcherMenuItem *menuitem;
  HDSwitcherMenuItemPrivate *priv;
  GtkWidget *hbox, *vbox = NULL;
  
  gobject = G_OBJECT_CLASS (hd_switcher_menu_item_parent_class)->constructor (type,
		  							 n_construct_params,
									 construct_params);

  menuitem = HD_SWITCHER_MENU_ITEM (gobject);
  priv = menuitem->priv;

  gtk_widget_push_composite_child ();
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_composite_name (GTK_WIDGET (hbox), "as-app-menu-hbox");
  gtk_container_add (GTK_CONTAINER (menuitem), hbox);
  gtk_widget_show (hbox);

  priv->icon = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (hbox), priv->icon, FALSE, FALSE, 0);
  gtk_widget_show (priv->icon);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_composite_name (GTK_WIDGET (vbox), "as-app-menu-vbox");
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);
      
  priv->label2 = gtk_label_new (NULL);
  gtk_widget_set_composite_name (GTK_WIDGET (priv->label2), "as-app-menu-label-desc");
  gtk_misc_set_alignment (GTK_MISC (priv->label2), 0.0, 0.5);
  gtk_box_pack_end (GTK_BOX (vbox), priv->label2, TRUE, TRUE, 0);
  gtk_widget_show (priv->label2);
  
  priv->label = gtk_label_new (NULL);
  gtk_widget_set_composite_name (GTK_WIDGET (priv->label), "as-app-menu-label");
  gtk_misc_set_alignment (GTK_MISC (priv->label), 0.0, 0.5);
  gtk_box_pack_end (GTK_BOX (vbox), priv->label, TRUE, TRUE, 0);
  gtk_widget_show (priv->label);

  if (priv->info)
  { 
    GdkPixbuf *app_pixbuf;
      
    priv->is_blinking = hd_wm_entry_info_is_urgent (priv->info);
    app_pixbuf = hd_wm_entry_info_get_icon (priv->info);
    
    if (!app_pixbuf)
    {
      GError *error = NULL;

      app_pixbuf = hd_wm_entry_info_get_app_icon (priv->info,
                                                  AS_ICON_THUMB_SIZE,
                                                  &error);

      if (error)
      {
        g_warning ("Could not load icon %s from theme: %s.",
                    hd_wm_entry_info_get_app_icon_name (priv->info),
                    error->message);
        g_error_free (error);
        error = NULL;
	
        app_pixbuf = gtk_icon_theme_load_icon (priv->icon_theme,
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
      
    if (app_pixbuf)
    {
      GdkPixbuf *compose = NULL;
      
      if (hd_wm_entry_info_is_hibernating (priv->info))
        compose = gtk_icon_theme_load_icon (priv->icon_theme,
 		      			    "qgn_indi_bkilled",
					     16,
					     0,
					     NULL);

      if (compose)
      {
        GdkPixbuf *tmp;
        gint dest_width = gdk_pixbuf_get_width (app_pixbuf);
        gint dest_height = gdk_pixbuf_get_height (app_pixbuf);
        gint off_x, off_y;

        off_x = dest_width - gdk_pixbuf_get_width (compose);
        off_y = dest_height - gdk_pixbuf_get_height (compose);

        /* Copy the pixbuf and make sure we have an alpha channel */
        tmp = gdk_pixbuf_add_alpha (app_pixbuf, FALSE, 255, 255, 255);
        if (tmp)
        {
          g_object_unref (app_pixbuf);
          app_pixbuf = tmp;
        }

        gdk_pixbuf_composite (compose, app_pixbuf,
                              0, 0,
                              dest_width, dest_height,
                              off_x, off_y,
                              1.0, 1.0,
                              GDK_INTERP_BILINEAR,
                              0xff);
        g_object_unref (compose);
      }
      
      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->icon), app_pixbuf);
      g_object_unref (app_pixbuf);

      if (priv->is_blinking)
        hd_switcher_menu_item_icon_animation (priv->icon, TRUE);

      gtk_widget_show (GTK_WIDGET (priv->icon));
    }

    gchar *app_name = hd_wm_entry_info_get_app_name (priv->info);
    gchar *win_name = hd_wm_entry_info_get_window_name (priv->info);

    if (win_name)
      gtk_label_set_text (GTK_LABEL (priv->label2), win_name);
	  
    gtk_label_set_text (GTK_LABEL (priv->label), app_name);

    g_free (app_name);
    g_free (win_name);
  }
  else
  if (priv->notification_id != -1)
  {
    if (priv->notification_icon)	 
      gtk_image_set_from_pixbuf 
        (GTK_IMAGE (priv->icon), priv->notification_icon);

    g_debug ("pixbuf: %p Id: %d Summary of notification: %s", priv->notification_icon, priv->notification_id, priv->notification_summary);
    
    gtk_label_set_text (GTK_LABEL (priv->label), 
		        priv->notification_summary); /* TODO: Insert timestamp */
    gtk_label_set_text (GTK_LABEL (priv->label2),
		    	priv->notification_body);
  }
  else
  if (priv->notification_ids != NULL)
  {
    if (priv->notification_icon)	 
      gtk_image_set_from_pixbuf 
        (GTK_IMAGE (priv->icon), priv->notification_icon);

    gtk_label_set_text (GTK_LABEL (priv->label), 
		        priv->notification_summary);
 
    gtk_widget_hide (priv->label2); 
  }  

  if (priv->show_close)
  {
    HDSwitcherMenuItemClass *klass = HD_SWITCHER_MENU_ITEM_CLASS (G_OBJECT_GET_CLASS (gobject));
    priv->close = gtk_image_new ();

    if (klass->thumb_close_button)
      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->close),
                                 klass->thumb_close_button);
    else
      g_warning ("Icon missing for close button");

    gtk_box_pack_end (GTK_BOX (hbox), priv->close, FALSE, FALSE, 0);
    gtk_widget_show (priv->close);
  }
  
  return gobject;
}

static void
hd_switcher_menu_item_size_request (GtkWidget      *widget,
			            GtkRequisition *requisition)
{
  HDSwitcherMenuItemPrivate *priv = HD_SWITCHER_MENU_ITEM (widget)->priv;
  GtkRequisition child_req = {0};
  GtkRequisition child_req2 = {0};
  gint child_width, child_height;

  /* if the width of icon + label (+ close) is too big,
   * we clamp it to AS_MENU_ITEM_WIDTH and ellipsize the
   * label text; the height is set by the icons
   */

  gtk_widget_size_request (priv->icon, &child_req);
  child_width = child_req.width;
  child_height = child_req.height;  

  child_req.width = child_req.height = 0;
  gtk_widget_size_request (priv->label,  &child_req);    
  
  gtk_widget_size_request (priv->label2, &child_req2);    
  child_width += MAX (child_req.width, child_req2.width);

  if (priv->show_close)
  {
    child_req.width = child_req.height = 0;
    gtk_widget_size_request (priv->close, &child_req);
    child_width += child_req.width;
    child_height = child_req.height;
  }

  child_width = MAX (child_width, requisition->width);
  
  GTK_WIDGET_CLASS (hd_switcher_menu_item_parent_class)->size_request (widget,
		 						       requisition);

  GtkWidget *parent = gtk_widget_get_parent (widget);
  gint label_width_ellipsize = (parent) ? parent->requisition.width : AS_MENU_ITEM_WIDTH;

  if (child_width > label_width_ellipsize)
  {
    requisition->width = label_width_ellipsize;

    gtk_label_set_ellipsize (GTK_LABEL (priv->label), PANGO_ELLIPSIZE_END);
    gtk_label_set_ellipsize (GTK_LABEL (priv->label2), PANGO_ELLIPSIZE_END);
  }
  else
    requisition->width = MAX (requisition->width, child_width);

  requisition->height = MAX (requisition->height, child_height);

  g_debug ("menu-item requisition: (%d, %d)",
	  requisition->width, requisition->height);
}

static void
hd_switcher_menu_item_activate (GtkMenuItem *menu_item)
{
  HDWMEntryInfo *info;
  
  info = hd_switcher_menu_item_get_entry_info (HD_SWITCHER_MENU_ITEM (menu_item));

  if (info != NULL)
  {
    g_debug ("Raising application '%s'", hd_wm_entry_info_peek_title (info));
  
    hd_wm_top_item (info);
  }
  else
  if (HD_SWITCHER_MENU_ITEM (menu_item)->priv->notification_id != -1)
  {
    hildon_desktop_notification_manager_call_action 
      (HD_SWITCHER_MENU_ITEM (menu_item)->priv->nm, 
       HD_SWITCHER_MENU_ITEM (menu_item)->priv->notification_id,
       "default");

    GError *error = NULL;

    hildon_desktop_notification_manager_close_notification
      (HD_SWITCHER_MENU_ITEM (menu_item)->priv->nm,
       HD_SWITCHER_MENU_ITEM (menu_item)->priv->notification_id,
       &error);

    if (error)
    {
      g_warning ("We cannot close the notification!?!?!");
      g_error_free (error);
    } 
  }	  
  else
  if (HD_SWITCHER_MENU_ITEM (menu_item)->priv->notification_ids != NULL)
  {
    GList *l;
    GError *error = NULL;

    for (l = HD_SWITCHER_MENU_ITEM (menu_item)->priv->notification_ids; l; l = g_list_next (l))
    {
      gint id = GPOINTER_TO_INT (l->data);

      hildon_desktop_notification_manager_close_notification
        (HD_SWITCHER_MENU_ITEM (menu_item)->priv->nm, id, &error);

      if (error)
      {
        g_warning ("We cannot close the notification!?!?!");
        g_error_free (error);
      } 
    }

    hildon_desktop_notification_manager_call_dbus_callback 
      (HD_SWITCHER_MENU_ITEM (menu_item)->priv->nm, 
       HD_SWITCHER_MENU_ITEM (menu_item)->priv->dbus_callback); 
  }	  
}

static gboolean
hd_switcher_menu_item_button_release_event (GtkWidget      *widget,
                                            GdkEventButton *event)
{
  HDSwitcherMenuItem *menuitem = HD_SWITCHER_MENU_ITEM(widget);
  gint x, y;
  
  g_debug ("menu item clicked ended");
  
  if(!menuitem->priv->show_close ||
     !menuitem->priv->close)
    return FALSE;

  /*
   * the pointer value is relative to the menuitem, but for some reason,
   * the close button allocation is relative to the menu as whole
   */
  
  gtk_widget_get_pointer(widget, &x, &y);

  /* only test x here; y is always withing the button range */
  if (x >  menuitem->priv->close->allocation.x &&
      x <= menuitem->priv->close->allocation.x +
           menuitem->priv->close->allocation.width)
  {
    if (menuitem->priv->info != NULL)
      hd_wm_entry_info_close (menuitem->priv->info);
    else
    if (menuitem->priv->notification_id != -1)
    {
      GError *error = NULL;

      hildon_desktop_notification_manager_close_notification
        (menuitem->priv->nm,
	 menuitem->priv->notification_id,
	 &error);

      if (error)
      {
        g_warning ("We cannot close the notification!?!?!");
	g_error_free (error);
      }	     	 
    }	    
    else
    if (menuitem->priv->notification_ids != NULL)
    {
      GList *l;
      GError *error = NULL;

      for (l = HD_SWITCHER_MENU_ITEM (menuitem)->priv->notification_ids; l; l = g_list_next (l))
      {
        gint id = GPOINTER_TO_INT (l->data);

        hildon_desktop_notification_manager_close_notification
          (HD_SWITCHER_MENU_ITEM (menuitem)->priv->nm, id, &error);

        if (error)
        {
          g_warning ("We cannot close the notification!?!?!");
          g_error_free (error);
        } 
      } 
    } 
    return TRUE;
  }
  
  return FALSE;
}

static gboolean
hd_switcher_menu_item_button_press_event (GtkWidget      *widget,
                                          GdkEventButton *event)
{
  HDSwitcherMenuItem *menuitem = HD_SWITCHER_MENU_ITEM(widget);
  gint x, y;
  
  g_debug ("menu item clicked");
  
  if(!menuitem->priv->show_close ||
     !menuitem->priv->close)
    return FALSE;

  /*
   * the pointer value is relative to the menuitem, but for some reason,
   * the close button allocation is relative to the menu as whole
   */
  
  gtk_widget_get_pointer(widget, &x, &y);

  g_debug ("pointer [%d,%d],\n"
          "close allocation [%d, %d, %d, %d]",
          x, y,
          menuitem->priv->close->allocation.x,
          menuitem->priv->close->allocation.y,
          menuitem->priv->close->allocation.width,
          menuitem->priv->close->allocation.height);

  /* only test x here; y is always withing the button range */
  if(x >  menuitem->priv->close->allocation.x &&
     x <= menuitem->priv->close->allocation.x +
          menuitem->priv->close->allocation.width)
    {
      return TRUE;
    }
  
  return FALSE;
}

static gboolean 
hd_switcher_menu_item_leave_notify (GtkWidget        *widget,
				    GdkEventCrossing *event)
{
  gtk_item_deselect (GTK_ITEM (widget));
	
  return TRUE;
}

static void
hd_switcher_menu_item_class_init (HDSwitcherMenuItemClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkMenuItemClass *menuitem_class = GTK_MENU_ITEM_CLASS (klass);
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
  
  gobject_class->finalize = hd_switcher_menu_item_finalize;
  gobject_class->set_property = hd_switcher_menu_item_set_property;
  gobject_class->get_property = hd_switcher_menu_item_get_property;
  gobject_class->constructor = hd_switcher_menu_item_constructor;

  widget_class->size_request = hd_switcher_menu_item_size_request;
  widget_class->button_press_event = hd_switcher_menu_item_button_press_event;
  widget_class->button_release_event = hd_switcher_menu_item_button_release_event;

  widget_class->leave_notify_event = hd_switcher_menu_item_leave_notify;
  
  menuitem_class->activate = hd_switcher_menu_item_activate;

  klass->thumb_close_button = gtk_icon_theme_load_icon (icon_theme,
                                                        "qgn_list_app_close",
                                                        AS_CLOSE_BUTTON_THUMB_SIZE,
                                                        0,
                                                        NULL);
  
  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_SHOW_CLOSE,
				   g_param_spec_boolean ("show-close",
					   		 "Show Close",
							 "Whether to show a close button",
							 FALSE,
							 (G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));
  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_IS_BLINKING,
				   g_param_spec_boolean ("is-blinking",
					   		 "Is blinking",
							 "Whether the menu item should blink",
							 FALSE,
							 (G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));
  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_ENTRY_INFO,
				   g_param_spec_pointer ("entry-info",
					   		 "Entry Info",
							 "The entry info object used to build the menu",
							 (G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));

  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_NOT_ID,
				   g_param_spec_int ("notification-id",
					   	     "Id of notification",
						     "The id of the notification",
						      -1,
						      G_MAXINT,
						      -1,
						     (G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));

  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_NOT_IDS,
				   g_param_spec_pointer ("notification-ids",
					   		 "List of ids of notifications",
							 "The list of ids of the notifications",
							 (G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));
 
  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_NOT_SUMMARY,
				   g_param_spec_string ("notification-summary",
					   		"Summary of notification",
							"The summary of the notification",
							"",
							(G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));
 
  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_NOT_BODY,
				   g_param_spec_string ("notification-body",
					   		"Body of notification",
							"The body of the notification",
							"",
							(G_PARAM_CONSTRUCT | G_PARAM_READWRITE))); 
 
  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_NOT_ICON,
				   g_param_spec_object ("notification-icon",
					   		"Icon notification",
							"The icon of the notification",
							GDK_TYPE_PIXBUF,
							(G_PARAM_CONSTRUCT | G_PARAM_READWRITE))); 
 
  g_object_class_install_property (gobject_class,
		  		   MENU_PROP_DBUS_CALLBACK,
				   g_param_spec_string ("dbus-callback",
					   		"D-Bus callback for notification group",
							"The DBus callback of the notification group",
							"",
							(G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));
 
  g_type_class_add_private (klass, sizeof (HDSwitcherMenuItemPrivate));
}

static void
hd_switcher_menu_item_init (HDSwitcherMenuItem *menuitem)
{
  HDSwitcherMenuItemPrivate *priv = HD_SWITCHER_MENU_ITEM_GET_PRIVATE (menuitem);

  menuitem->priv = priv;

  priv->icon_theme = gtk_icon_theme_get_default ();
  
  priv->show_close            = FALSE;
  priv->is_blinking          = FALSE;
  priv->pixbuf_anim          = NULL;
  priv->info                 = NULL;
  priv->notification_id      = -1;
  priv->notification_ids     = NULL;
  priv->notification_summary = 
  priv->notification_body    = NULL;
  priv->notification_icon    = NULL;
  priv->dbus_callback        = NULL;

  priv->nm = 
    HILDON_DESKTOP_NOTIFICATION_MANAGER 
      (hildon_desktop_notification_manager_get_singleton ());
}

GtkWidget *
hd_switcher_menu_item_new_from_entry_info (HDWMEntryInfo *info,
		                           gboolean     show_close)
{
  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (HD_WM_IS_ENTRY_INFO (info), NULL);
  
  return 
    g_object_new (HD_TYPE_SWITCHER_MENU_ITEM,
                  "entry-info", info,
                  "show-close", show_close,
                   NULL);
}

GtkWidget *
hd_switcher_menu_item_new_from_notification (gint id,
					     GdkPixbuf *icon,
					     gchar     *summary,
					     gchar     *body,
					     gboolean   show_close)
{

  return 
    g_object_new (HD_TYPE_SWITCHER_MENU_ITEM,
		  "notification-id", id, 
		  "notification-icon", icon,
		  "notification-summary", summary, 
		  "notification-body", body,
		  "entry-info", NULL,
		  "show-close", show_close,
		  NULL); 
}	

GtkWidget *
hd_switcher_menu_item_new_from_notification_group (GList     *ids,
					           GdkPixbuf *icon,
					           gchar     *summary,
						   gchar     *dbus_callback,
					           gboolean   show_close)
{

  return 
    g_object_new (HD_TYPE_SWITCHER_MENU_ITEM,
		  "notification-id", -1, 
		  "notification-ids", ids, 
		  "notification-icon", icon,
		  "notification-summary", summary, 
		  "dbus-callback", dbus_callback, 
		  "show-close", show_close,
		  NULL); 
}

void
hd_switcher_menu_item_set_entry_info (HDSwitcherMenuItem *menuitem,
  		                      HDWMEntryInfo        *info)
{
  HDSwitcherMenuItemPrivate *priv;
  GdkPixbuf *pixbuf;
  
  g_return_if_fail (HD_IS_SWITCHER_MENU_ITEM (menuitem));
  g_return_if_fail (info != NULL);

  priv = menuitem->priv;

  priv->info = info;
  
  pixbuf = hd_wm_entry_info_get_icon (priv->info);
  
  if (!pixbuf)
  {
    GError *error = NULL;


    pixbuf = hd_wm_entry_info_get_app_icon (priv->info,
                                           AS_ICON_THUMB_SIZE,
                                           &error);
    if (error)
    {
      g_warning ("Could not load icon %s from theme: %s.",
                 hd_wm_entry_info_get_app_icon_name (priv->info),
                 error->message);
          
          
      g_clear_error (&error);
      
      pixbuf = gtk_icon_theme_load_icon (priv->icon_theme,
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
  
  if (pixbuf)
  {
    gtk_image_set_from_pixbuf (GTK_IMAGE (priv->icon), pixbuf);
    g_object_unref (pixbuf);
  }
  
  gtk_label_set_text (GTK_LABEL (priv->label),
		      hd_wm_entry_info_peek_title (priv->info));

  g_object_notify (G_OBJECT (menuitem), "entry-info");
}

HDWMEntryInfo *
hd_switcher_menu_item_get_entry_info (HDSwitcherMenuItem *menuitem)
{
  g_return_val_if_fail (HD_IS_SWITCHER_MENU_ITEM (menuitem), NULL);

  return menuitem->priv->info;
}

void
hd_switcher_menu_item_set_blinking (HDSwitcherMenuItem *menuitem,
				  gboolean       is_blinking)
{
  g_return_if_fail (HD_IS_SWITCHER_MENU_ITEM (menuitem));

  hd_switcher_menu_item_icon_animation (menuitem->priv->icon, is_blinking);

  menuitem->priv->is_blinking = is_blinking;
}

gboolean
hd_switcher_menu_item_is_blinking (HDSwitcherMenuItem *menuitem)
{
  g_return_val_if_fail (HD_IS_SWITCHER_MENU_ITEM (menuitem), FALSE);

  return menuitem->priv->is_blinking;
}

gint 
hd_switcher_menu_item_get_notification_id (HDSwitcherMenuItem *menuitem)
{
  g_return_val_if_fail (HD_IS_SWITCHER_MENU_ITEM (menuitem), -1);

  return menuitem->priv->notification_id;
}	

static gint
hd_switcher_menu_item_compare_ids (gconstpointer a, gconstpointer b)
{
  if (GPOINTER_TO_INT (a) == GPOINTER_TO_INT (b))
    return 0;
  else
    return 1;
}
	
gboolean 
hd_switcher_menu_item_has_id (HDSwitcherMenuItem *menuitem, gint id)
{
  g_return_val_if_fail (HD_IS_SWITCHER_MENU_ITEM (menuitem), FALSE);

  if (g_list_find_custom (menuitem->priv->notification_ids, 
			  GINT_TO_POINTER (id), 
			  hd_switcher_menu_item_compare_ids) != NULL)
    return TRUE;
  else
    return FALSE;
}	
