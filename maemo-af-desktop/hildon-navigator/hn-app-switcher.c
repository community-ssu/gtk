/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/* hn-app-switcher.c
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

/* Hildon includes */
#include "hn-app-switcher.h"
#include "hn-app-button.h"
#include "hn-app-tooltip.h"
#include "hn-app-menu-item.h"
#include "hn-app-sound.h"
#include "hn-wm.h"
#include "hn-entry-info.h"
#include "hn-wm-watchable-app.h"
#include "hildon-pixbuf-anim-blinker.h"

#include <stdlib.h>
#include <string.h>

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

#include <libosso.h>

#include <hildon-widgets/gtk-infoprint.h>
#include "hildon-pixbuf-anim-blinker.h"


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

/* log include */
#include <log-functions.h>

/* Menu item strings */
#define AS_HOME_ITEM 		_("tana_fi_home")
#define AS_HOME_ITEM_ICON 	"qgn_list_home"

/* Defined workarea atom */
#define WORKAREA_ATOM "_NET_WORKAREA"

#define AS_MENU_BUTTON_ICON      "qgn_list_tasknavigator_appswitcher"
#define AS_MENU_DEFAULT_APP_ICON "qgn_list_gene_default_app"

#define ANIM_DURATION 5000 	/* 5 Secs for blinking icons */
#define ANIM_FPS      2

/* Themed widget names */
static const gchar *as_button_names[] = {
  "hildon-navigator-small-button1",
  "hildon-navigator-small-button2",
  "hildon-navigator-small-button3",
  "hildon-navigator-small-button4",
  
  NULL,
};

static const gchar *as_button_pressed_names[] = {
  "hildon-navigator-small-button1-pressed",
  "hildon-navigator-small-button2-pressed",
  "hildon-navigator-small-button3-pressed",
  "hildon-navigator-small-button4-pressed",
  NULL,
};

#define AS_MENU_BUTTON_NAME "hildon-navigator-small-button5"

#define AS_UPPER_SEPARATOR "hildon-navigator-upper-separator"
#define AS_LOWER_SEPARATOR "hildon-navigator-lower-separator"

#define AS_BOX_NAME         "hildon-navigator-app-switcher"

/* Hardcoded pixel perfecting values */
#define AS_BUTTON_BORDER_WIDTH  0
#define AS_MENU_BORDER_WIDTH    20
#define AS_TIP_BORDER_WIDTH 	20
#define AS_BUTTON_HEIGHT        38
#define AS_MENU_BUTTON_HEIGHT   58
#define AS_ROW_HEIGHT 		    30
#define AS_ICON_SIZE            26
#define AS_TOOLTIP_WIDTH        360
#define AS_MENU_ITEM_WIDTH      360
#define AS_INTERNAL_PADDING     10
#define AS_SEPARATOR_HEIGHT     10

/* Needed for catching the MCE D-BUS messages */
#define MCE_SERVICE          "com.nokia.mce"
#define MCE_SIGNAL_INTERFACE "com.nokia.mce.signal"
#define MCE_SIGNAL_PATH      "/com/nokia/mce/signal"

/* hardware signals */
#define HOME_LONG_PRESS "sig_home_key_pressed_long_ind"
#define HOME_PRESS      "sig_home_key_pressed_ind"
#define SHUTDOWN_IND    "shutdown_ind"

/* lowmem signals */
#define LOWMEM_ON_SIGNAL_INTERFACE  "com.nokia.ke_recv.lowmem_on"
#define LOWMEM_ON_SIGNAL_PATH       "/com/nokia/ke_recv/lowmem_on"
#define LOWMEM_ON_SIGNAL_NAME       "lowmem_on"

#define LOWMEM_OFF_SIGNAL_INTERFACE "com.nokia.ke_recv.lowmem_off"
#define LOWMEM_OFF_SIGNAL_PATH      "/com/nokia/ke_recv/lowmem_off"
#define LOWMEM_OFF_SIGNAL_NAME      "lowmem_off"

/* bgkill signals */
#define BGKILL_ON_SIGNAL_INTERFACE  "com.nokia.ke_recv.bgkill_on"
#define BGKILL_ON_SIGNAL_PATH       "/com/nokia/ke_recv/bgkill_on"
#define BGKILL_ON_SIGNAL_NAME       "bgkill_on"

#define BGKILL_OFF_SIGNAL_INTERFACE "com.nokia.ke_recv.bgkill_off"
#define BGKILL_OFF_SIGNAL_PATH      "/com/nokia/ke_recv/bgkill_off"
#define BGKILL_OFF_SIGNAL_NAME      "bgkill_off"

/* sound samples */
#define HN_WINDOW_OPEN_SOUND        DATADIR"/sounds/ui-window_open.wav"
#define HN_WINDOW_CLOSE_SOUND       DATADIR"/sounds/ui-window_close.wav"

#define TOOLTIP_SHOW_TIMEOUT 500
#define TOOLTIP_HIDE_TIMEOUT 1500


static void
hn_app_image_animation (GtkWidget *icon,
		       gboolean   is_on)
{
  GdkPixbuf *pixbuf;
  GdkPixbufAnimation *pixbuf_anim;

  g_return_if_fail (GTK_IS_IMAGE (icon));

  if (is_on)
    {
      pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (icon));
      pixbuf_anim = hildon_pixbuf_anim_blinker_new (pixbuf,
		     				    1000 / ANIM_FPS,
						    -1,
						    10);
     
      gtk_image_set_from_animation (GTK_IMAGE(icon), pixbuf_anim);
      g_object_unref (pixbuf_anim);
    }
  else
    {
      pixbuf_anim = gtk_image_get_animation (GTK_IMAGE (icon));

      /* grab static image from menu item and reset */
      pixbuf = gdk_pixbuf_animation_get_static_image (pixbuf_anim);

      gtk_image_set_from_pixbuf (GTK_IMAGE (icon), pixbuf);

      /*
       * unrefing the pixbuf here causes SIGSEGV
       */
      
      /*g_object_unref (pixbuf);*/
    }
}

static void
refresh_app_button (HNAppSwitcher *app_switcher, HNEntryInfo *entry, gint pos);

static void
queue_refresh_buttons (HNAppSwitcher *app_switcher);

/*
 * HNAppSwitcher
 */

enum
{
  AS_APP1_BUTTON,
  AS_APP2_BUTTON,
  AS_APP3_BUTTON,
  AS_APP4_BUTTON,

  N_BUTTONS
};

enum
{
  ADD_INFO,
  REMOVE_INFO,
  CHANGED_INFO,
  CHANGED_STACK,
  SHUTDOWN,
  LOWMEM,
  BGKILL,

  LAST_SIGNAL
};

#define HN_APP_SWITCHER_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HN_TYPE_APP_SWITCHER, HNAppSwitcherPrivate))

struct _HNAppSwitcherPrivate
{
  GtkWidget *buttons[N_BUTTONS];
  GSList *buttons_group;

  GtkWidget *main_button;
  GtkWidget *main_menu;
  GtkWidget *main_home_item;

  GtkWidget *tooltip;
  
  GtkWidget *current_toggle;
  GtkWidget *app_menu;

  /* flags */
  guint menu_icon_is_blinking : 1;
  guint tooltip_visible : 1;
  guint switched_to_desktop : 1;
  guint is_long_press : 1;
  guint is_dimming_on : 1;
  guint system_inactivity : 1;
  
  /* pointer location */
  guint pointer_on_button : 1;
  guint is_thumbable : 1;

  GList *applications;
  
  GtkIconTheme *icon_theme;
  osso_context_t *osso;

  guint queue_refresh_id;

  /* sound samples data */
  gint esd_socket;
  gint start_sample;
  gint end_sample;
};

G_DEFINE_TYPE (HNAppSwitcher, hn_app_switcher, GTK_TYPE_VBOX);


static HNAppSwitcher *singleton = NULL;

static guint app_switcher_signals[LAST_SIGNAL] = { 0 };


/* MCE signals handler: listens to HOME and SHUTDOWN events; in case of
 * SHUTDOWN event, relays it through the corresponding signal of the
 * HNAppSwitcher widget.
 */
static DBusHandlerResult
mce_handler (DBusConnection *conn,
             DBusMessage    *msg,
             void           *data)
{
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (data);
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  const gchar *member;
  
  if (dbus_message_get_type (msg) != DBUS_MESSAGE_TYPE_SIGNAL)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  
  member = dbus_message_get_member (msg);
  if (!member || member[0] == '\0')
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  
  if (strcmp (HOME_LONG_PRESS, member) == 0)
    {
      if (GTK_WIDGET_IS_SENSITIVE (priv->main_button))
        {
          priv->is_long_press = TRUE;
          
	  hn_wm_top_desktop ();
	}
      
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
  
  if (strcmp (HOME_PRESS, member) == 0)
    {
      gboolean was_long_press = priv->is_long_press;
      
      if (was_long_press)
        priv->is_long_press = FALSE;
      else
        {
          if (GTK_WIDGET_IS_SENSITIVE (priv->main_button))
            {
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->main_button),
			      		    TRUE);
	      gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON (priv->main_button));
	    }
	}

      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
  
  if (strcmp (SHUTDOWN_IND, member) == 0)
    {
      g_signal_emit (app_switcher, app_switcher_signals[SHUTDOWN], 0);
      
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
  
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* LOWMEM signals handler: listen to LOWMEM_ON and LOWMEM_OFF events,
 * and relays them through the HNAppSwitcher "lowmem" signal.
 */
static DBusHandlerResult
lowmem_handler (DBusConnection *conn,
                DBusMessage    *msg,
                void           *data)
{
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (data);
  const gchar *member;
  
  if (dbus_message_get_type (msg) != DBUS_MESSAGE_TYPE_SIGNAL)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  
  member = dbus_message_get_member (msg);
  if (!member || member[0] == '\0')
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (strcmp (LOWMEM_ON_SIGNAL_NAME, member) == 0)
    {
      g_signal_emit (app_switcher, app_switcher_signals[LOWMEM], 0, TRUE);
      
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
  
  if (strcmp (LOWMEM_OFF_SIGNAL_NAME, member) == 0)
    {
      g_signal_emit (app_switcher, app_switcher_signals[LOWMEM], 0, FALSE);
      
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
  
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* BGKILL signals handler: listens to the BGKILL_ON and BGKILL_OFF
 * events, and relays them through the HNAppSwitcher "bgkill"
 * signal
 */
static DBusHandlerResult
bgkill_handler (DBusConnection *conn,
                DBusMessage    *msg,
                void           *data)
{
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (data);
  const gchar *member;

  if (dbus_message_get_type (msg) != DBUS_MESSAGE_TYPE_SIGNAL)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  
  member = dbus_message_get_member (msg);
  if (!member || member[0] == '\0')
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (strcmp (BGKILL_ON_SIGNAL_NAME, member) == 0)
    {
      g_signal_emit (app_switcher, app_switcher_signals[BGKILL], 0, TRUE);
      
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
  
  if (strcmp (BGKILL_OFF_SIGNAL_NAME, member) == 0)
    {
      g_signal_emit (app_switcher, app_switcher_signals[BGKILL], 0, FALSE);
      
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
  
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gint
get_app_button_pos (GtkWidget *button)
{
  gint pos;
  
  g_return_val_if_fail (GTK_IS_TOGGLE_BUTTON (button), AS_APP1_BUTTON);

  pos = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
					    "app-button-pos"));

  return pos;  
}

static void
set_app_button_pos (GtkWidget *button,
		    gint       pos)
{
  g_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
  g_return_if_fail (pos >= AS_APP1_BUTTON && pos < N_BUTTONS);

  g_object_set_data (G_OBJECT (button),
		     "app-button-pos",
		     GINT_TO_POINTER (pos));
}

static gboolean
get_main_button_is_blinking (GtkWidget *button)
{
  gint is_blinking;

  g_return_val_if_fail (GTK_IS_TOGGLE_BUTTON (button), FALSE);

  is_blinking = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
			  			    "button-is-blinking"));

  return is_blinking;
}

static void
set_main_button_is_blinking (GtkWidget *button,
			     gboolean   is_blinking)
{
  g_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));

  g_object_set_data (G_OBJECT (button),
                     "button-is-blinking",
                     GINT_TO_POINTER (is_blinking));
}

static GdkPixbuf *
hn_app_switcher_get_icon_from_theme (HNAppSwitcher *app_switcher,
				     const gchar   *icon_name,
				     gint           size)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GError *error;
  GdkPixbuf *retval;

  if (!icon_name)
    return NULL;

  if (!priv->icon_theme)
    priv->icon_theme = gtk_icon_theme_get_default ();

  g_return_val_if_fail (priv->icon_theme, NULL);

  if (!icon_name || icon_name[0] == '\0')
    return NULL;

  error = NULL;
  retval = gtk_icon_theme_load_icon (priv->icon_theme,
		  		     icon_name,
				     size == -1 ? AS_ICON_SIZE : size,
				     0,
				     &error);
  if (error)
    {
      osso_log (LOG_ERR, "Could not load icon '%s': %s\n",
		icon_name,
		error->message);

      g_error_free (error);

      return NULL;
    }

  return retval;  
}

#if 0
static GdkPixbuf *
hn_app_switcher_get_app_icon_for_entry_info (HNAppSwitcher *app_switcher,
                                             HNEntryInfo   *info)
{
  GdkPixbuf *retval;
  const gchar *icon_name;
 
  HN_DBG("New object does not have an icon -- using default");
  
  icon_name = hn_entry_info_get_app_icon_name (info);
  if (!icon_name)
    icon_name = AS_MENU_DEFAULT_APP_ICON;

  retval = hn_app_switcher_get_icon_from_theme (app_switcher, icon_name, -1);

  if(!retval)
    {
      /*perhaps the specified icon is missing -- try the default */
      retval = hn_app_switcher_get_icon_from_theme (app_switcher,
                                                    AS_MENU_DEFAULT_APP_ICON,
                                                    -1);
    }
  
  return retval;
}
#endif

static void
main_menu_detach (GtkWidget *attach_widget,
		  GtkMenu   *menu)
{
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (attach_widget);

  app_switcher->priv->main_menu = NULL;
  app_switcher->priv->main_home_item = NULL;
  app_switcher->priv->is_thumbable = FALSE;
}

static void
main_home_item_activate_cb (GtkMenuItem   *menuitem,
			    HNAppSwitcher *app_switcher)
{
  HN_DBG ("Raising desktop");

  hn_wm_top_desktop ();
}

static void
main_menu_ensure_state (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GList *menu_children, *l;
  GtkWidget *separator;
  GtkWidget *active_menu_item = NULL;
  
  /* we must dispose the old contents of the menu first */
  menu_children = gtk_container_get_children (GTK_CONTAINER (priv->main_menu));
  for (l = menu_children; l != NULL; l = l->next)
    {
      GtkWidget *child = GTK_WIDGET (l->data);
      
      /* remove the home menu item too, but keep a reference so
       * that it stays around until we can re-add it later
       */
      if (child == priv->main_home_item)
        g_object_ref (child);	
      
      gtk_container_remove (GTK_CONTAINER (priv->main_menu), child);
    }
  g_list_free (menu_children);

  /* rebuild the menu */
  for (l = priv->applications; l != NULL; l = l->next)
    {
      GtkWidget *menu_item;
      const GList * children = hn_entry_info_get_children(l->data);
      const GList * child;

      for(child = children; child != NULL; child = child->next)
        {
          HN_DBG ("Creating new app menu item (thumb:%s)",
	          priv->is_thumbable ? "on" : "off");

          menu_item = hn_app_menu_item_new (child->data,
			 		    TRUE,
					    priv->is_thumbable);

          if(hn_entry_info_is_active(child->data))
            active_menu_item = menu_item;
          
          gtk_menu_shell_append (GTK_MENU_SHELL (priv->main_menu), menu_item);
          gtk_widget_show (menu_item);
        }

      /* append the separator for this app*/
      separator = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (priv->main_menu),
                             separator);
      gtk_widget_show (separator);
    }

  /* append home menu item */
  if (!priv->main_home_item)
    {
      GdkPixbuf *home_icon;
      
      priv->main_home_item = gtk_image_menu_item_new_with_label (AS_HOME_ITEM);
      
      home_icon = hn_app_switcher_get_icon_from_theme (app_switcher,
		      				       AS_HOME_ITEM_ICON,
						       -1);
      if (home_icon)
        {
          GtkWidget *image = gtk_image_new_from_pixbuf (home_icon);
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (priv->main_home_item),
			  		 image);

	  g_object_unref (home_icon);
	}

      g_signal_connect (priv->main_home_item, "activate",
		        G_CALLBACK (main_home_item_activate_cb),
			app_switcher);
      
      gtk_menu_shell_append (GTK_MENU_SHELL (priv->main_menu),
		 	     priv->main_home_item);
    }
  else
    gtk_menu_shell_append (GTK_MENU_SHELL (priv->main_menu),
		           priv->main_home_item);

  gtk_widget_show (priv->main_home_item);
  
  g_object_ref (priv->main_home_item);

  if (active_menu_item)
    gtk_menu_shell_select_item (GTK_MENU_SHELL (priv->main_menu),
                                active_menu_item);
  else
    gtk_menu_shell_select_item (GTK_MENU_SHELL (priv->main_menu),
                                priv->main_home_item);
  
  g_object_unref (priv->main_home_item);
}

static void
main_menu_unmap_cb (GtkWidget     *menu,
		    HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GtkToggleButton *button = GTK_TOGGLE_BUTTON (priv->main_button);
  
  gtk_toggle_button_set_active (button, FALSE);
}

static gboolean
main_menu_keypress_cb (GtkWidget * menu, GdkEventKey *event,
                       HNAppSwitcher *app_switcher)
{
  g_return_val_if_fail(event && menu && app_switcher, FALSE);

  if (event->keyval == GDK_Left ||
      event->keyval == GDK_KP_Left ||
      event->keyval == GDK_Escape)
    {
      gtk_menu_shell_deactivate (GTK_MENU_SHELL(menu));

      if (event->keyval == GDK_Escape)
        {
          /* pass focus to the last active application */
	  hn_wm_focus_active_window ();
        }
      else
        {
	  hn_wm_activate (HN_TN_ACTIVATE_KEY_FOCUS);
	  gtk_widget_grab_focus (app_switcher->priv->main_button);
        }
        
      return TRUE;
    }

  return FALSE;
}

gboolean
hn_app_switcher_menu_button_release_cb (GtkWidget      *widget,
                                        GdkEventButton *event)
{
  gint x, y;
  
  gtk_widget_get_pointer(widget, &x, &y);

  HN_DBG ("Main menu button released\n"
	  "\tpointer    [%d, %d],\n"
          "\tallocation [%d, %d, %d, %d]",
          x, y,
          widget->allocation.x,
          widget->allocation.y,
          widget->allocation.width,
          widget->allocation.height);

#if 0
  /* XXX - Is this really necessary? It breaks thumb menu support. EB */
  if ((x > widget->allocation.width)  || (x < 0) ||
      (y > widget->allocation.height) || (y < 0))
    {
      HN_DBG ("deactivating menu");
      
      hn_wm_activate (HN_TN_ACTIVATE_LAST_APP_WINDOW);
      gtk_menu_shell_deactivate(GTK_MENU_SHELL(widget));
      
      return TRUE;
    }
#endif

  return FALSE;
}

static void
main_menu_build (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  
  if (priv->main_menu)
    {
      main_menu_ensure_state (app_switcher);

      return;
    }

  priv->main_menu = gtk_menu_new ();
  g_signal_connect (priv->main_menu, "unmap",
		    G_CALLBACK (main_menu_unmap_cb),
		    app_switcher);
  g_signal_connect (priv->main_menu, "key-press-event",
		    G_CALLBACK (main_menu_keypress_cb),
		    app_switcher);
  g_signal_connect (priv->main_menu, "button-release-event",
		    G_CALLBACK (hn_app_switcher_menu_button_release_cb),
		    NULL);
  gtk_menu_attach_to_widget (GTK_MENU (priv->main_menu),
		             GTK_WIDGET (app_switcher),
			     main_menu_detach);

  main_menu_ensure_state (app_switcher);
}

static void
main_menu_position_func (GtkMenu  *menu,
			 gint     *x,
			 gint     *y,
			 gboolean *push_in,
			 gpointer  data)
{
  GtkWidget *widget = GTK_WIDGET (data);
  GdkScreen *screen = gtk_widget_get_screen (widget);
  GtkRequisition req;

  if (!GTK_WIDGET_REALIZED (widget))
    return;

  if (hn_wm_fullscreen_mode())
    {
      *x = 0;
    }
  else
    {
      gdk_window_get_origin (widget->window, x, y);
      *x += widget->allocation.width;
    }

  gtk_widget_size_request (GTK_WIDGET (menu), &req);
  *y = gdk_screen_get_height (screen) - req.height;
  
  *push_in = FALSE;
}


static void
main_menu_pop (HNAppSwitcher *app_switcher,
	       GtkWidget     *toggle)
{
  HNAppSwitcherPrivate *priv;
  gboolean was_blinking;

  g_return_if_fail (HN_IS_APP_SWITCHER (app_switcher));
  g_return_if_fail (GTK_IS_WIDGET (toggle));

  priv = app_switcher->priv;
  
  was_blinking = get_main_button_is_blinking (toggle);
  if (was_blinking)
    {
      /* need to stop the button blinking */
      const GList       *k, *l;
      HNEntryInfo       *child;
      GtkWidget         *app_image = gtk_bin_get_child (GTK_BIN (toggle));
      int                i;
      
      hn_app_image_animation (app_image, FALSE);
      set_main_button_is_blinking (toggle, FALSE);

      /*
       * set the ignore flag on any children that were causing the blinking
       * we skip the first four apps, which cause blinking of the app buttons,
       * not the menu button.
       */
      for (k = priv->applications, i = 0; k != NULL; k = k->next, ++i)
        {
          if (i < N_BUTTONS)
            continue;
          
          for (l = hn_entry_info_get_children (k->data); l != NULL; l = l->next)
            {
              child = l->data;
              if (hn_entry_info_is_urgent(child))
                {
                  hn_entry_info_set_ignore_urgent(child, TRUE);
                }
            }
        }
    }
  
  main_menu_build (app_switcher);

  gtk_menu_popup (GTK_MENU (priv->main_menu), NULL, NULL,
                  main_menu_position_func, app_switcher,
                  0, GDK_CURRENT_TIME);
}

static gboolean
main_menu_button_keypress_cb (GtkWidget     *toggle,
	       		      GdkEventKey   *event,
                              HNAppSwitcher *app_switcher)
{
  if (event->keyval == GDK_Right ||
      event->keyval == GDK_KP_Right ||
      event->keyval == GDK_KP_Enter)
    {
      hn_app_switcher_toggle_menu_button (app_switcher);
      
      return TRUE;
    }
  else if (event->keyval == GDK_Left ||
           event->keyval == GDK_KP_Left)
    {
      hn_wm_activate (HN_TN_ACTIVATE_LAST_APP_WINDOW);
    }
  
  return FALSE;
}

static void
main_menu_button_toggled_cb (HNAppSwitcher *app_switcher,
			     GtkWidget     *toggle)
{
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)))
    return;

  HN_DBG("Main menu button toggled");
  
  main_menu_pop (app_switcher, toggle);
}

static gboolean
menu_button_release_cb (GtkWidget      *widget,
                        GdkEventButton *event,
                        HNAppSwitcher  *app_switcher)
{
  gint x,y;
  HNAppSwitcherPrivate *priv;

  g_return_val_if_fail (event && app_switcher, FALSE);

  priv = app_switcher->priv;
  
  /* if the relase is outside the button, reset the thumbable flag, to avoid
   * problems if the menu is raised some other way than via pointer
   */
  gtk_widget_get_pointer (widget, &x, &y);

  HN_DBG ("Main menu button released:\n"
          "\tpointer    [%d, %d],\n"
          "\tallocation [%d, %d, %d, %d]",
          x, y,
          widget->allocation.x,
          widget->allocation.y,
          widget->allocation.width,
          widget->allocation.height);

  if (((x > widget->allocation.width)  || (x < 0)) ||
      ((y > widget->allocation.height) || (y < 0)))
    {
      HN_DBG ("Release outside button -- resetting thumbable flag");
      priv->is_thumbable = FALSE;

      hn_wm_activate (HN_TN_ACTIVATE_LAST_APP_WINDOW);
    }

  return FALSE;
}

static gboolean
menu_button_pressed_cb (GtkWidget      *widget,
                        GdkEventButton *event,
                        HNAppSwitcher  *app_switcher)
{
  g_return_val_if_fail (event && app_switcher, FALSE);
  
  HNAppSwitcherPrivate *priv = app_switcher->priv;

  /* remember which button was used to press this button */
  HN_DBG("Main menu button pressed using button %d", event->button);

  hn_wm_activate (HN_TN_DEACTIVATE_KEY_FOCUS);

  if (event->button == APP_BUTTON_THUMBABLE || event->button == 2)
    {
      priv->is_thumbable = TRUE;
      hn_app_switcher_toggle_menu_button (app_switcher);
      
      return TRUE;
    }
  else
    priv->is_thumbable = FALSE;
  
  return FALSE;
}

static GtkWidget *
create_menu_button (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GdkPixbuf *pixbuf;
  GtkWidget *button;
  GtkWidget *icon;
  
  button = gtk_toggle_button_new ();
  
  gtk_widget_set_name (button, AS_MENU_BUTTON_NAME);
  gtk_widget_set_sensitive (button, FALSE);
  gtk_widget_set_size_request (button, -1, AS_MENU_BUTTON_HEIGHT);

  g_object_set (G_OBJECT (button),
                "can-focus", TN_DEFAULT_FOCUS,
                NULL);
  
  pixbuf = hn_app_switcher_get_icon_from_theme (app_switcher, AS_MENU_BUTTON_ICON, -1);
  icon = gtk_image_new_from_pixbuf (pixbuf);
  gtk_container_add (GTK_CONTAINER (button), icon);

  g_signal_connect_swapped (button, "toggled",
		  	    G_CALLBACK (main_menu_button_toggled_cb),
			    app_switcher);

  g_signal_connect (button, "key-press-event",
                    G_CALLBACK (main_menu_button_keypress_cb),
                    app_switcher);
 
  g_signal_connect (button, "button-press-event",
                    G_CALLBACK (menu_button_pressed_cb),
                    app_switcher);

  g_signal_connect (button, "button-release-event",
                    G_CALLBACK (menu_button_release_cb),
                    app_switcher);
  
  priv->main_button = button;
  
  return priv->main_button;
}

static void
app_button_toggled_cb (GtkToggleButton *toggle,
		       gpointer         user_data)
{
  GtkWidget *widget = GTK_WIDGET (toggle);
  gint pos = get_app_button_pos (widget);
  gboolean is_active = gtk_toggle_button_get_active (toggle);
  gboolean is_inconsistent = gtk_toggle_button_get_inconsistent (toggle);

  if (is_inconsistent)
    gtk_widget_set_name (widget, as_button_names[pos]);
  else
    {
      if (is_active)
	gtk_widget_set_name (widget, as_button_pressed_names[pos]);
      else
	gtk_widget_set_name (widget, as_button_names[pos]);
    }

  HN_DBG ("setting button (pos=%d) (inconsistent='<%s>', active='<%s>') name: %s",
	  pos,
	  is_inconsistent ? "true" : "false",
	  is_active ? "true" : "false",
	  gtk_widget_get_name (widget));
}

static GtkWidget *
create_app_button (HNAppSwitcher *app_switcher,
	           gint           pos)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GtkWidget *retval;

  g_assert (priv->buttons[pos] == NULL);

  HN_DBG ("Creating app button at pos %d (name %s)",
	  pos, as_button_names[pos]);
 
  retval = hn_app_button_new (NULL);

  set_app_button_pos (retval, pos);
  hn_app_button_set_is_blinking (HN_APP_BUTTON (retval), FALSE);

  gtk_widget_set_name (retval, as_button_names[pos]);

  g_signal_connect (retval, "toggled",
		    G_CALLBACK (app_button_toggled_cb),
		    NULL);

  priv->buttons[pos] = retval;
  
  return priv->buttons[pos];
}

static void
hn_app_switcher_build (HNAppSwitcher *app_switcher)
{
  GtkWidget *vbox;
  GtkWidget *button;

  g_assert (app_switcher && hn_wm_init (app_switcher));

  gtk_widget_push_composite_child ();
  
  /* inner box, used for padding */
  HN_DBG ("Adding inner VBox");
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_composite_name (vbox, "application-switcher-button-box");
  gtk_box_pack_start (GTK_BOX (app_switcher), vbox,
		      TRUE, TRUE, 0 /*padding*/);
  gtk_widget_show (vbox);

  /* most recent applications buttons */
  HN_DBG ("Adding buttons");
  button = create_app_button (app_switcher, AS_APP1_BUTTON);
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  
  button = create_app_button (app_switcher, AS_APP2_BUTTON);
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  
  button = create_app_button (app_switcher, AS_APP3_BUTTON);
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  
  button = create_app_button (app_switcher, AS_APP4_BUTTON);
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);  
  gtk_widget_show (button);
  
  /* menu button */
  HN_DBG ("Adding menu button");
  button = create_menu_button (app_switcher);
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  
  gtk_widget_pop_composite_child ();
}

/*
 * initializes esd cache of sound samples
 */
static void
hn_app_switcher_init_sound_samples (HNAppSwitcher *app_switcher)
{
  /* initialize the sound samples data */
  app_switcher->priv->esd_socket = hn_as_sound_init ();
  app_switcher->priv->start_sample = hn_as_sound_register_sample (
          app_switcher->priv->esd_socket,
          HN_WINDOW_OPEN_SOUND);
  app_switcher->priv->end_sample = hn_as_sound_register_sample (
          app_switcher->priv->esd_socket,
          HN_WINDOW_CLOSE_SOUND);
}

/*
 * callback for system HW events
 * -- currently we are only interested in the system inactivity event
 */
static
void hn_app_osso_hw_cb (osso_hw_state_t *state, gpointer data)
{
  g_return_if_fail(state && data);
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (data);
  HNAppSwitcherPrivate *priv = app_switcher->priv;

  if (state->system_inactivity_ind != priv->system_inactivity)
    {
      priv->system_inactivity = state->system_inactivity_ind;
      queue_refresh_buttons (app_switcher);
    }
}

static void
hn_app_switcher_osso_initialize (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  DBusConnection *conn = NULL;
  DBusObjectPathVTable mce_vtable, lowmem_vtable, bgkill_vtable;
  gboolean res;
  osso_hw_state_t hs = {0};
  
  priv->osso = osso_initialize ("AS_DIMMED_infoprint", "0.1", FALSE, NULL);
  if (!priv->osso)
    {
      osso_log(LOG_ERR, "Failed to initialize libOSSO");
      
      return;
    }

  /* register stystem inactivity handler */
  hs.system_inactivity_ind = TRUE;
  osso_hw_set_event_cb(priv->osso, &hs,
                       hn_app_osso_hw_cb, app_switcher);
  
  
  /* Set up the monitoring of MCE events in order to be able
   * to top Home or open the menu
   */
  conn = osso_get_sys_dbus_connection (priv->osso);
  if (!conn)
    {
      osso_log (LOG_ERR, "Failed getting connection to system bus");
      
      return;
    }
  
  mce_vtable.message_function = mce_handler;
  mce_vtable.unregister_function = NULL;

#define MAKE_SIGNAL_RULE(_iface)	"type=\'signal\', interface=\'" _iface "\'"
  
  res = dbus_connection_register_object_path (conn, MCE_SIGNAL_PATH,
                                              &mce_vtable,
                                              app_switcher);
  if (res)
    {
      dbus_bus_add_match (conn, MAKE_SIGNAL_RULE (MCE_SIGNAL_INTERFACE), NULL);
      dbus_connection_flush (conn);
    }
  else
    osso_log (LOG_ERR, "Failed registering MCE handler");

  lowmem_vtable.message_function = lowmem_handler;
  lowmem_vtable.unregister_function = NULL;
  
  res = dbus_connection_register_object_path (conn, LOWMEM_ON_SIGNAL_PATH,
                                              &lowmem_vtable,
                                              app_switcher);
  if (res)
    {
      dbus_bus_add_match (conn, MAKE_SIGNAL_RULE (LOWMEM_ON_SIGNAL_INTERFACE), NULL);
      dbus_connection_flush (conn);
    }
  else
    osso_log(LOG_ERR, "Failed registering LOWMEM_ON handler");

  res = dbus_connection_register_object_path (conn, LOWMEM_OFF_SIGNAL_PATH,
                                              &lowmem_vtable,
                                              app_switcher);
  if (res)
    {
      dbus_bus_add_match (conn, MAKE_SIGNAL_RULE (LOWMEM_OFF_SIGNAL_INTERFACE), NULL);
      dbus_connection_flush (conn);
    }
  else
    osso_log(LOG_ERR, "Failed registering LOWMEM_OFF handler");

  bgkill_vtable.message_function = bgkill_handler;
  bgkill_vtable.unregister_function = NULL;
  
  res = dbus_connection_register_object_path (conn, BGKILL_ON_SIGNAL_PATH,
                                              &bgkill_vtable,
                                              app_switcher);
  if (res)
    {
      dbus_bus_add_match (conn, MAKE_SIGNAL_RULE (BGKILL_ON_SIGNAL_INTERFACE), NULL);
      dbus_connection_flush (conn);
    }
  else
    osso_log(LOG_ERR, "Failed registering BGKILL_ON handler");

  res = dbus_connection_register_object_path (conn, BGKILL_OFF_SIGNAL_PATH,
                                              &bgkill_vtable,
                                              app_switcher);
  if (res)
    {
      dbus_bus_add_match (conn, MAKE_SIGNAL_RULE (BGKILL_OFF_SIGNAL_INTERFACE), NULL);
      dbus_connection_flush (conn);
    }
  else
    osso_log(LOG_ERR, "Failed registering BGKILL_OFF handler");
}

/* We must override the "show-all" method, as we may have stuff we don't want
 * to show, like the main button or the application buttons
 */
static void
hn_app_switcher_show_all (GtkWidget *widget)
{
  HNAppSwitcherPrivate *priv = HN_APP_SWITCHER (widget)->priv;
  gint i;

  gtk_widget_show (widget);

  /* show the main menu button only if there is at least
   * one application on the switcher
   */
  if (priv->applications)
    gtk_widget_show (priv->main_button);

  /* show only the buttons linked to an application */
  for (i = AS_APP1_BUTTON; i < N_BUTTONS; i++)
    {
      GtkWidget *button = priv->buttons[i];
      HNEntryInfo *info;
     
      info = hn_app_button_get_entry_info (HN_APP_BUTTON (button));
      if (info)
        gtk_widget_show (button);
    }
}

static HNEntryInfo *
hn_app_switcher_find_app_for_child(HNAppSwitcher *app_switcher,
                                   HNEntryInfo *entry_info)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GList * l = priv->applications;
  HNWMWatchableApp *app = hn_entry_info_get_app(entry_info);
        
  while(l)
    {
      HNEntryInfo *e = (HNEntryInfo *)l->data;
            
      if(app == hn_entry_info_get_app(e))
          return e;
      l = g_list_next(l);
    }

  return NULL;
}

static void
remove_entry_from_app_button (HNAppSwitcher *app_switcher,
			      HNEntryInfo   *entry_info)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  gint pos;

  for (pos = AS_APP1_BUTTON; pos < N_BUTTONS; pos++)
    {
      GtkWidget *button = priv->buttons[pos];
      HNEntryInfo *e;

      e = hn_app_button_get_entry_info (HN_APP_BUTTON (button));
      if (e == entry_info)
        {
          hn_app_button_set_group (HN_APP_BUTTON (button), NULL);
          hn_app_button_set_entry_info (HN_APP_BUTTON (button), NULL);
          priv->buttons_group = hn_app_button_get_group (HN_APP_BUTTON (button));
	  break;
	}
    }
}

static void
refresh_app_button (HNAppSwitcher *app_switcher,
                    HNEntryInfo   *entry,
                    gint           pos)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  const GList          *l, *children = hn_entry_info_get_children(entry);
  gboolean              urgent = FALSE;
  HNAppButton          *app_button = HN_APP_BUTTON (priv->buttons[pos]);
  
  /* deal with urgency flags */
  for (l = children; l != NULL; l = l->next)
    {
      /*
       * If the entry is urgent and the ignore flag is not set, the button
       * should blink
       */
      if (hn_entry_info_is_urgent(l->data) &&
          !hn_entry_info_get_ignore_urgent(l->data))
        {
          HN_DBG("Found an urgent button");
          urgent = TRUE;
        }

      /*
       * if the info is not urgent, we need to clear any leftover
       * ignore_urgent flag
       */
      if(!hn_entry_info_is_urgent(l->data) &&
         hn_entry_info_get_ignore_urgent(l->data))
        {
          hn_entry_info_set_ignore_urgent(l->data, FALSE);
        }
    }

  /* bind the entry info to the widget, so that we can
   * use it later when the user toggles the button
   *
   * NB: this recreates the icon respecting system inactivity
   */
  hn_app_button_set_entry_info (app_button, entry);
  hn_app_button_set_group (app_button, priv->buttons_group);
  priv->buttons_group = hn_app_button_get_group (app_button);

  HN_DBG ("buttons_group.size := %d",
	  g_slist_length (priv->buttons_group));

  hn_app_button_set_is_blinking (app_button, urgent);

  if (hn_entry_info_is_active (entry))
    {
      GtkToggleButton *button;

      button = GTK_TOGGLE_BUTTON (priv->buttons[pos]);
      gtk_toggle_button_set_inconsistent (button, FALSE);
      gtk_toggle_button_set_active (button, TRUE);
      gtk_toggle_button_toggled (button);
    }
  
  gtk_widget_set_sensitive (priv->buttons[pos], TRUE);
  g_object_set (G_OBJECT (priv->buttons[pos]),
                "can-focus", TRUE,
                NULL);
}

static gboolean
refresh_buttons (gpointer user_data)
{
  HNAppSwitcher        *app_switcher = HN_APP_SWITCHER (user_data);
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GList                *l;
  gint                  pos;
  gint                  last_active;
  gboolean              was_blinking;
  gboolean              is_urgent = FALSE;
  GtkWidget            *app_image;

  /* first we reset all the buttons icons */
  for (pos = AS_APP1_BUTTON; pos < N_BUTTONS; pos++)
    {
      HNAppButton *button = HN_APP_BUTTON (priv->buttons[pos]);

      hn_app_button_set_group (button, NULL);
      hn_app_button_set_entry_info (button, NULL);
    }

  priv->buttons_group = NULL;
  
  /* then refresh the icons of the application buttons */
  for (l = priv->applications, pos = AS_APP1_BUTTON;
       l != NULL && pos < N_BUTTONS;
       l = l->next, pos++)
    {
      HNEntryInfo *entry = l->data;
      
      /* we just want the most recently used top-level applications */
      if (entry->type != HN_ENTRY_WATCHED_APP)
        {
          HN_DBG("Object is not an application");
          continue;
        }

      refresh_app_button (app_switcher, entry, pos);
      
      HN_DBG ("Showing object");
    }

  /*
   * now we need to process the menu button to ensure that it blinks
   * if needed
   */
  was_blinking = get_main_button_is_blinking (priv->main_button);
  
  is_urgent = FALSE;
  last_active = 0;
  for (l = priv->applications, pos = 0;
       l != NULL;
       l = l->next, ++pos)
    {
      const GList       *k;
      HNEntryInfo       *child;

      if (pos < N_BUTTONS)
        {
          if (hn_entry_info_is_active (l->data))
	    last_active = pos;
	  
          continue;
	}

      if (hn_entry_info_is_active (l->data))
        {
          GtkToggleButton *active_button;

	  HN_DBG ("Unsetting the previously active button %d",
		  last_active);
	  
	  active_button = GTK_TOGGLE_BUTTON (priv->buttons[pos]);
	  gtk_toggle_button_set_inconsistent (active_button, TRUE);
	  gtk_toggle_button_set_active (active_button, FALSE);
	  gtk_toggle_button_toggled (active_button);
	}

      /* set the ignore flag on any children that were causing the blinking
       * we skip the first four apps, which cause blinking of the app buttons,
       * not the menu button.
       */
      for (k = hn_entry_info_get_children (l->data); k != NULL; k = k->next)
        {
          child = k->data;

          /* If the entry is urgent and the ignore flag is not set, the
	   * button should blink
           */
          if(hn_entry_info_is_urgent(child) &&
             !hn_entry_info_get_ignore_urgent(child))
            {
              is_urgent = TRUE;
            }

          /*
           * if the info is not urgent, we need to clear any leftover
           * ignore_urgent flag
           */
          if(!hn_entry_info_is_urgent(child) &&
             hn_entry_info_get_ignore_urgent(child))
            {
              hn_entry_info_set_ignore_urgent(child, FALSE);
            }
        }
    }

  app_image = gtk_bin_get_child (GTK_BIN (priv->main_button));
  g_assert (GTK_IS_IMAGE (app_image));

  if (is_urgent && !was_blinking && !priv->system_inactivity)
    {
      HN_DBG("Setting menu button urgency to '%s'",
	     is_urgent ? "true" : "false");
      hn_app_image_animation (app_image, is_urgent);
      set_main_button_is_blinking (priv->main_button, is_urgent);
    }
  else if (was_blinking && priv->system_inactivity)
    {
      HN_DBG("Setting menu button urgency to 'false'", FALSE);
      
      hn_app_image_animation (app_image, FALSE);
      set_main_button_is_blinking (priv->main_button, FALSE);
    }

  /* hide the main button if no more applications are left */
  if (!priv->applications)
    {
      HN_DBG ("Hiding main button icon");
      
      gtk_widget_hide (app_image);
      gtk_widget_set_sensitive (priv->main_button, FALSE);
    }
  else
    {
      HN_DBG ("Showing main button icon");
      
      gtk_widget_set_sensitive (priv->main_button, TRUE);
      gtk_widget_show (app_image);
    }

  return FALSE;
}

static void
queue_refresh_done (gpointer user_data)
{
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (user_data);

  app_switcher->priv->queue_refresh_id = 0;
}

static void
queue_refresh_buttons (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;

  if (!priv->queue_refresh_id)
    priv->queue_refresh_id = g_timeout_add_full (G_PRIORITY_HIGH,
		    				 200,
		                                 refresh_buttons,
					         app_switcher,
					         queue_refresh_done);
}

/* Class closure for the "add" signal; this is called each time an
 * entry has been added to the applications list.
 */
static void
hn_app_switcher_real_add_info (HNAppSwitcher *app_switcher,
                               HNEntryInfo   *entry_info)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  HNWMWatchableApp *app;
  HNEntryInfo * e;
  
  HN_DBG ("In hn_app_switcher_real_add_info");

  if (!entry_info)
    {
      g_warning ("No entry info provided!");

      return;
    }

  switch(entry_info->type)
    {
      case HN_ENTRY_WATCHED_WINDOW:
      case HN_ENTRY_WATCHED_VIEW:
        /*
         * because initial windows get created before we have a chance to add
         * the application item, we have to store orphan windows in temporary
         * list and process them when the application item is added
         */
        HN_DBG ("Adding new child to AS ...");
        app = hn_entry_info_get_app (entry_info);
        e = hn_app_switcher_find_app_for_child (app_switcher, entry_info);

        if (!e)
          {
            e = hn_wm_watchable_app_get_info(app);
            if(!e)
              {
                g_warning ("Could not create HNEntryInfo for app.");
                return;
              }
            
            priv->applications = g_list_prepend (priv->applications, e);
            
          }
        
        hn_entry_info_add_child (e, entry_info);
        break;
      case HN_ENTRY_WATCHED_APP:
        /* we handle adding of applications internally in AS */
        g_warning("asked to append HN_ENTRY_WATCHED_APP "
                  "-- this should not happen");
        return;
      default:
        g_warning("Unknown info type");
        return;
    }

  /* Play a sound */
  if (hn_as_sound_play_sample (priv->esd_socket, priv->start_sample) == -1)
    {
      /* Connection to esd was probably closed */
      hn_app_switcher_init_sound_samples (app_switcher);
      hn_as_sound_play_sample (priv->esd_socket, priv->start_sample);
    }

  queue_refresh_buttons (app_switcher);
  
  /* we must explicitely ensure that the main menu is rebuilt,
   * now that the application list is tainted by a new entry;
   * destroying the menu widget will call the detach function
   */
  if (priv->main_menu)
    gtk_widget_destroy (priv->main_menu);
}

/* Class closure for the "remove" signal; this is called each time
 * an entry has been remove from the application list
 */
static void
hn_app_switcher_real_remove_info (HNAppSwitcher *app_switcher,
			          HNEntryInfo   *entry_info)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  HNEntryInfo * info_parent = NULL;
  gboolean removed_app = FALSE;
  
  switch (entry_info->type)
    {
      case HN_ENTRY_WATCHED_WINDOW:
      case HN_ENTRY_WATCHED_VIEW:
        HN_DBG ("removing child from AS ...");
        info_parent = hn_entry_info_get_parent(entry_info);

        if(!info_parent)
          {
            g_warning("An orphan HNEntryInfo !!!");
            return;
          }

        if(!hn_entry_info_remove_child(info_parent, entry_info))
          {
            HN_DBG ("... no more children, removing app.");
            priv->applications = g_list_remove (priv->applications,
                                                info_parent);
            removed_app = TRUE;
          }
        
        break;
      case HN_ENTRY_WATCHED_APP:
        /* we handle adding/removing of applications internally in AS */
        g_warning("asked to remove HN_ENTRY_WATCHED_APP "
                  "-- this should not happen");
        return;
      default:
        g_warning("Unknown info type");
        return;
    }
  
  /* Play a sound */
  if (hn_as_sound_play_sample (priv->esd_socket, priv->end_sample) == -1)
    {
      /* Connection to esd was probably closed */
      hn_app_switcher_init_sound_samples (app_switcher);
      hn_as_sound_play_sample (priv->esd_socket, priv->start_sample);
    }
  
  if (removed_app)
    remove_entry_from_app_button (app_switcher, info_parent);

  queue_refresh_buttons (app_switcher);

  /* we must explicitely ensure that the main menu is rebuilt,
   * now that the application list is tainted by delete entry;
   * destroying the menu widget will call the detach function
   */
  if (priv->main_menu)
    gtk_widget_destroy (priv->main_menu);

  if (removed_app)
    {
      /* we need to check that not all of the remaining apps are
       * hibernating, and if they are, wake one of them up, because
       * we will not receive current window msg from MB
       */
      GList * l;
      gboolean all_asleep = TRUE;
      
      for(l = priv->applications; l != NULL; l = l->next)
        {
          HNEntryInfo * entry = l->data;
          HNWMWatchableApp * app = hn_entry_info_get_app (entry);

          if (app && !hn_wm_watchable_app_is_hibernating (app))
            {
              all_asleep = FALSE;
              break;
            }
        }

      if(all_asleep && priv->applications)
        {
          /*
           * Unfortunately, we do not know which application is the
           * most recently used one, so we just wake up the first one
           */
          hn_wm_top_item((HNEntryInfo*)priv->applications->data);
        }
    }
}


/* Class closure for the "changed" signal; this is called each time
 * an entry inside the applications list has been changed by the WM,
 * for instance when the icon or the title changes.
 *
 * NB: entry_info can be NULL for global update
 */
static void
hn_app_switcher_real_changed_info (HNAppSwitcher *app_switcher,
			           HNEntryInfo   *entry_info)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  HN_DBG ("In hn_app_switcher_real_changed_info");

  /* all changes have potential impact on the the main menu; app menus are
   * created on the fly, so we do not have to worry about menu changes there
   */
  if (priv->main_menu)
    gtk_widget_destroy (priv->main_menu);

  /*
   * If we are given an entry info and it of the app type, we just need to
   * update at most one button
   */
  if(entry_info && entry_info->type == HN_ENTRY_WATCHED_APP)
    {
      gint                  pos;
      GList *               l;

      HN_DBG ("HNEntryInfo present and of type WATCHED_APP");
      
      for (l = priv->applications, pos = AS_APP1_BUTTON;
           l != NULL && pos < N_BUTTONS;
           l = l->next, pos++)
        {
          HNEntryInfo *entry = l->data;
          
          if (entry->type != HN_ENTRY_WATCHED_APP)
            {
              HN_DBG("Object is not an application");
              continue;
            }

          if (entry_info == entry)
            {
              refresh_app_button (app_switcher, entry_info, pos);
              return;
            }
        }
      
      return;
    }
  else if (entry_info)
    {
      /* this is a change in a child entry; in addition to the main menu impact
       * we already dealt with, child changes affect the blinking of the
       * associated buttons
       */
      HNEntryInfo *parent;
      GtkWidget   *button = NULL;
      GList       *l;
      gint         pos;
      
      g_return_if_fail (entry_info->type != HN_ENTRY_INVALID);
      HN_DBG ("HNEntryInfo present, with child entry");

      parent = hn_entry_info_get_parent(entry_info);

      if (!parent)
        {
          g_critical("Attempting to change orphan child item.");
          return;
        }

      /* if the info is urgent but has an ingore flag, we know that it was not
       * causing the button to blink, and will not cause the button to blink,
       * so we do not need to update the buttons
       */
      if(hn_entry_info_is_urgent(entry_info) &&
         hn_entry_info_get_ignore_urgent(entry_info))
        {
          return;
        }

      /* if the info is not urgent, but has the ignore flag set, we have to
       * clear that flag; we also know that it was not causing the associated
       * button to blink (it was being ingored), so we do not need to update
       * the button.
       */
      if(!hn_entry_info_is_urgent(entry_info) &&
         hn_entry_info_get_ignore_urgent(entry_info))
        {
          hn_entry_info_set_ignore_urgent(entry_info, FALSE);
          return;
        }

      /* the remaining cases are more complicated and require that we know
       * the state of the associated button
       */
      for (l = priv->applications, pos = AS_APP1_BUTTON;
           l != NULL && pos < N_BUTTONS;
           l = l->next, pos++)
        {
          HNEntryInfo *entry = l->data;
      
          /* we just want the most recently used top-level applications */
          if (entry->type != HN_ENTRY_WATCHED_APP)
            {
              HN_DBG("Object is not an application");
              continue;
            }

          if(entry == parent)
            {
              button = priv->buttons[pos];
              break;
            }
        }

      HN_DBG ("Force the button's icon to update itself");
      hn_app_button_force_update_icon (HN_APP_BUTTON (button));

      HN_MARK();

      /* if the current info is urgent and not to be ignored, we know the app
       * button should blink; make sure it does
       */
      if(hn_entry_info_is_urgent(entry_info) &&
         !hn_entry_info_get_ignore_urgent(entry_info))
        {
          if(button)
            {
              /* child of one of the app buttons */
              if(!hn_app_button_get_is_blinking(HN_APP_BUTTON(button)))
                {
                  hn_app_button_set_is_blinking(HN_APP_BUTTON(button),TRUE);
                }
            }
          else
            {
              /* the child belongs to the main menu button */
              if(!get_main_button_is_blinking(priv->main_button))
                {
                  GtkWidget *app_image
                    = gtk_bin_get_child (GTK_BIN (priv->main_button));

                  HN_DBG("Setting menu button urgency to %d", TRUE);
                  hn_app_image_animation(app_image, TRUE);
                  set_main_button_is_blinking(priv->main_button, TRUE);
                }
            }
          
          return;
        }

      /* if the info is not urgent and it is not being ignored and the
       * associated button is not blinking, we know that it was not urgent
       * previously, so we do not need to update the button
       */
      if(!hn_entry_info_is_urgent(entry_info) &&
         !hn_entry_info_get_ignore_urgent(entry_info))
        {
          if((button && !hn_app_button_get_is_blinking(HN_APP_BUTTON(button))) ||
             (!button && !get_main_button_is_blinking(priv->main_button)))
            {
              return;
            }
        }
      
      HN_MARK();

      /* we are left with the case where the info is not urgent, is not being
       * ignored, and the associated button is blinking -- this is probably a
       * change urgent -> !urgent, but we do not know if this button was the
       * sole cause of the blinking, so we have to update the whole shebang
       * -- fall through to refresh_buttons();
       */
    }
  
  HN_DBG("Queuing a refresh cycle of the buttons (info: %s)",
         entry_info != NULL ? "yes" : "no");

  /* either global update (no entry_info) or a more complicated case that
   * was not handled above
   */
  queue_refresh_buttons (app_switcher);
}


/* Class closure for the "changed-stack" signal; this is called each time
 * The TN receives notification from MB about window/view being topped. 
 */
static void
hn_app_switcher_real_changed_stack (HNAppSwitcher *app_switcher,
				    HNEntryInfo   *entry_info)
{
  gint                   pos, active_pos;
  GList                * l;
  HNAppSwitcherPrivate * priv = app_switcher->priv;
  HNEntryInfo          * parent;
  gboolean               active_found = FALSE;
  
  HN_DBG ("In hn_app_switcher_real_changed_stack");
  
  if (!entry_info || !hn_entry_info_is_active (entry_info))
    {
      /* rebuild everything, since we were not told what has been topped
       * issue warning, as this is not how this function is meant to be
       * called
       */
      g_warning ("No entry_info provided");
      
      queue_refresh_buttons (app_switcher);
      return;
    }
  

  if(entry_info->type == HN_ENTRY_WATCHED_APP)
    {
      /* we only accept entries for windows and views */
      g_warning ("Cannot handle HN_ENTRY_WATCHED_APP");
      return;
    }
  
  parent = hn_entry_info_get_parent (entry_info);

  if (!parent)
    {
      g_warning ("Orphan entry info");
      return;
    }
  
  /* locate the associated button, and toggle it */
  active_pos = 0;
  for (l = priv->applications, pos = AS_APP1_BUTTON;
       l != NULL && pos < N_BUTTONS;
       l = l->next, pos++)
    {
      HNEntryInfo *entry = l->data;
          
      if (parent == entry)
	{
	  hn_app_button_make_active (HN_APP_BUTTON(priv->buttons[pos]));
	  active_pos = pos;
	  active_found = TRUE;
	  break;
	}
    }

  /* no active window in the buttons, make sure that
   * no button is active
   */  
  if (!active_found)
    {
      for (pos = AS_APP1_BUTTON; pos < N_BUTTONS; pos++)
        {
          GtkToggleButton *app_button;

	  HN_DBG ("Setting inconsistent state for pos %d", pos);

	  app_button = GTK_TOGGLE_BUTTON (priv->buttons[pos]);
	  gtk_toggle_button_set_inconsistent (app_button, TRUE);
	  gtk_toggle_button_set_active (app_button, FALSE);
	  gtk_toggle_button_toggled (app_button);
	}
    }  
  

  /* we do not worry about the urgency hint here, as we will receive a
   * notification when it is cleared from the WM
   */
}

static void
hn_app_switcher_real_bgkill (HNAppSwitcher *app_switcher,
			     gboolean       is_on)
{
  hn_app_switcher_changed (app_switcher, NULL);  
}

static void
hn_app_switcher_real_lowmem (HNAppSwitcher *app_switcher,
                             gboolean       is_on)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  priv->is_dimming_on = is_on;

  /* TODO - update the sensitivity of the items depending on the
   * lowmem state
   */
  hn_app_switcher_changed (app_switcher, NULL);
}

static void
hn_app_switcher_finalize (GObject *gobject)
{
  HNAppSwitcher *app_switch = HN_APP_SWITCHER (gobject);
  HNAppSwitcherPrivate *priv = app_switch->priv;
  
  osso_deinitialize (priv->osso);

  g_list_free (priv->applications);

  HN_DBG ("Destroying HNAppSwitcher");

  G_OBJECT_CLASS (hn_app_switcher_parent_class)->finalize (gobject);
}

static GObject *
hn_app_switcher_constructor (GType                  type,
			     guint                  n_construct_params,
			     GObjectConstructParam *construct_params)
{
  GObject *object;

  HN_DBG ("inside hn_app_switcher_constructor...");

  if (!singleton)
    {
      object = G_OBJECT_CLASS (hn_app_switcher_parent_class)->constructor (type,
		      n_construct_params, construct_params);
      singleton = HN_APP_SWITCHER (object);
  
      HN_DBG ("building HNAppSwitcher widget");
      
      hn_app_switcher_build (singleton);
      hn_app_switcher_osso_initialize (singleton);
    }
  else
    {
      /* is this even possible? anyway, let's play along. EB */
      osso_log (LOG_WARNING, "Requested another HNAppSwitcher instance");
      
      object = g_object_ref (singleton);
    }
  
  return object;
}

static void
hn_app_switcher_class_init (HNAppSwitcherClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  gobject_class->finalize = hn_app_switcher_finalize;
  gobject_class->constructor = hn_app_switcher_constructor;

  widget_class->show_all = hn_app_switcher_show_all;

  app_switcher_signals[ADD_INFO] =
    g_signal_new ("add-info",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (HNAppSwitcherClass, add_info),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);
  
  app_switcher_signals[REMOVE_INFO] = 
    g_signal_new ("remove-info",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (HNAppSwitcherClass, remove_info),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);
  
  app_switcher_signals[CHANGED_INFO] =
    g_signal_new ("changed-info",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (HNAppSwitcherClass, changed_info),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  app_switcher_signals[CHANGED_STACK] =
    g_signal_new ("changed-stack",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (HNAppSwitcherClass, changed_stack),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);
  
  app_switcher_signals[SHUTDOWN] =
    g_signal_new ("shutdown",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (HNAppSwitcherClass, shutdown),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  app_switcher_signals[LOWMEM] =
    g_signal_new ("lowmem",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (HNAppSwitcherClass, lowmem),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);

  app_switcher_signals[BGKILL] =
    g_signal_new ("bgkill",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (HNAppSwitcherClass, bgkill),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);
  
  klass->add_info = hn_app_switcher_real_add_info;
  klass->remove_info = hn_app_switcher_real_remove_info;
  klass->changed_info = hn_app_switcher_real_changed_info;
  klass->changed_stack = hn_app_switcher_real_changed_stack;
 
  klass->bgkill = hn_app_switcher_real_bgkill; 
  klass->lowmem = hn_app_switcher_real_lowmem;
  
  g_type_class_add_private (klass, sizeof (HNAppSwitcherPrivate));
}

static void
hn_app_switcher_init (HNAppSwitcher *app_switcher)
{
  app_switcher->priv = HN_APP_SWITCHER_GET_PRIVATE (app_switcher);
  
  app_switcher->priv->buttons_group = NULL;

  gtk_widget_set_name (GTK_WIDGET (app_switcher), AS_BOX_NAME);
  
  /* set base properties of the app-switcher widget */
  gtk_box_set_spacing (GTK_BOX (app_switcher), 0);
  gtk_box_set_homogeneous (GTK_BOX (app_switcher), FALSE);
  
  gtk_container_set_border_width (GTK_CONTAINER (app_switcher), 0);

  hn_app_switcher_init_sound_samples (app_switcher);
}


/* Public API */

GtkWidget *
hn_app_switcher_new (void)
{
  return g_object_new (HN_TYPE_APP_SWITCHER, NULL);
}

void
hn_app_switcher_add (HNAppSwitcher *app_switcher,
		     HNEntryInfo   *entry_info)
{
  g_return_if_fail (HN_IS_APP_SWITCHER (app_switcher));
  g_return_if_fail (entry_info != NULL && HN_ENTRY_INFO_IS_VALID_TYPE (entry_info->type));

  HN_DBG ("Emitting the ADD_INFO signal");
  
  g_signal_emit (app_switcher, app_switcher_signals[ADD_INFO], 0, entry_info);  
}

void
hn_app_switcher_remove (HNAppSwitcher *app_switcher,
			HNEntryInfo   *entry_info)
{
  g_return_if_fail (HN_IS_APP_SWITCHER (app_switcher));
  g_return_if_fail (entry_info != NULL && HN_ENTRY_INFO_IS_VALID_TYPE (entry_info->type));
  
  HN_DBG ("Emitting the REMOVE_INFO signal");
  
  g_signal_emit (app_switcher, app_switcher_signals[REMOVE_INFO], 0, entry_info);  
}

/* allow entry_info == NULL to carry out global changes */
void
hn_app_switcher_changed (HNAppSwitcher *app_switcher,
			 HNEntryInfo   *entry_info)
{
  g_return_if_fail (HN_IS_APP_SWITCHER (app_switcher));
  g_return_if_fail (entry_info == NULL ||
                    HN_ENTRY_INFO_IS_VALID_TYPE (entry_info->type));
  
  HN_DBG ("Emitting the CHANGED_INFO signal");
  
  g_signal_emit (app_switcher, app_switcher_signals[CHANGED_INFO], 0, entry_info);
}

void
hn_app_switcher_changed_stack (HNAppSwitcher *app_switcher,
			       HNEntryInfo   *entry_info)
{
  g_return_if_fail (HN_IS_APP_SWITCHER (app_switcher));
  g_return_if_fail (HN_ENTRY_INFO_IS_VALID_TYPE (entry_info->type));

  HN_DBG ("Emitting the CHANGED_STACK signal");

  g_signal_emit (app_switcher, app_switcher_signals[CHANGED_STACK], 0, entry_info);
}

GList *
hn_app_switcher_get_entries (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv;
  GList *retlist, *l;

  g_return_val_if_fail (HN_IS_APP_SWITCHER (app_switcher), NULL);
  priv = app_switcher->priv;

  retlist = NULL;
  for (l = priv->applications; l != NULL; l = l->next)
    retlist = g_list_prepend (retlist, l->data);

  return g_list_reverse (retlist);
}

void
hn_app_switcher_foreach_entry (HNAppSwitcher            *app_switcher,
			       HNAppSwitcherForeachFunc  func,
			       gpointer                  data)
{
  HNAppSwitcherPrivate *priv;
  GList *entries, *l;

  g_return_if_fail (HN_IS_APP_SWITCHER (app_switcher));
  g_return_if_fail (func != NULL);

  priv = app_switcher->priv;
  entries = priv->applications;

  for (l = entries; l != NULL; l = l->next)
    {
      HNEntryInfo *info = l->data;

      if (!(* func) (info, data))
	break;
    }
}


void
hn_app_switcher_toggle_menu_button (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv;

  g_return_if_fail (HN_IS_APP_SWITCHER (app_switcher));
  priv = app_switcher->priv;
  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->main_button), TRUE);
  g_signal_emit_by_name (priv->main_button, "toggled");
}

gboolean
hn_app_switcher_get_system_inactivity (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv;

  g_return_val_if_fail (HN_IS_APP_SWITCHER (app_switcher), FALSE);
  priv = app_switcher->priv;

  return priv->system_inactivity;
}

