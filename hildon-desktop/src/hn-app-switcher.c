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
#include "hn-app-switcher.h"
#include "hn-app-button.h"
#include "hn-app-tooltip.h"
#include "hn-app-menu-item.h"
#include "hn-app-sound.h"
#include "hn-app-pixbuf-anim-blinker.h"

#include <stdlib.h>
#include <string.h>

/* X include */
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

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

#ifdef HAVE_LIBOSSO
#include <libosso.h>
#endif

#ifdef HAVE_LIBHILDON
#else
#include <hildon-widgets/hildon-finger.h>
#endif 


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

/* Defined workarea atom */
#define WORKAREA_ATOM "_NET_WORKAREA"

#define AS_MENU_BUTTON_ICON      "qgn_list_tasknavigator_appswitcher"
#define AS_MENU_DEFAULT_APP_ICON "qgn_list_gene_default_app"

#define ANIM_DURATION 5000 	/* 5 Secs for blinking icons */
#define ANIM_FPS      2

#define AS_BUTTON_NAME "hildon-navigator-small-button%d"
#define AS_BUTTON_NAME_PRESSED "hildon-navigator-small-button%d-pressed"

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

/* sound samples */
#define HN_WINDOW_OPEN_SOUND        DATADIR"/sounds/ui-window_open.wav"
#define HN_WINDOW_CLOSE_SOUND       DATADIR"/sounds/ui-window_close.wav"

#define TOOLTIP_SHOW_TIMEOUT 500
#define TOOLTIP_HIDE_TIMEOUT 1500

enum 
{
  AS_PROP_NITEMS=1,
};

static void 
hn_app_switcher_add_info_cb (HDWM *hdwm, HDEntryInfo *entry_info, gpointer data);

static void 
hn_app_switcher_remove_info_cb (HDWM *hdwm, gboolean removed_app, HDEntryInfo *entry_info, gpointer data);

static void 
hn_app_switcher_changed_info_cb (HDWM *hdwm, HDEntryInfo *entry_info, gpointer data);

static void 
hn_app_switcher_changed_stack_cb (HDWM *hdwm, HDEntryInfo *entry_info, gpointer data);

static void
hn_app_switcher_show_menu_cb (HDWM *hdwm, gpointer data);

static void 
hn_app_switcher_orientation_changed_cb (HNAppSwitcher *app_switcher, GParamSpec *pspec);

static void 
hn_app_switcher_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void 
hn_app_switcher_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

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
    pixbuf_anim = 
      hn_app_pixbuf_anim_blinker_new (pixbuf, 1000 / ANIM_FPS, -1, 10);
     
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
refresh_app_button (HNAppSwitcher *app_switcher, HDEntryInfo *entry, gint pos);

static void
queue_refresh_buttons (HNAppSwitcher *app_switcher);

/*
 * HNAppSwitcher
 */

enum
{
  ADD_INFO,
  REMOVE_INFO,
  CHANGED_INFO,
  CHANGED_STACK,
  LOWMEM,
  BGKILL,

  LAST_SIGNAL
};

#define HN_APP_SWITCHER_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HN_TYPE_APP_SWITCHER, HNAppSwitcherPrivate))

struct _HNAppSwitcherPrivate
{
  gint nitems;
	
  GtkWidget **buttons;
  GSList *buttons_group;

  GtkWidget *main_button;
  GtkWidget *main_menu;
  GtkWidget *main_home_item;
  GtkWidget *active_menu_item;
  HDEntryInfo *home_info;

  GtkWidget *tooltip;
  
  GtkWidget *current_toggle;
  GtkWidget *app_menu;

  /* flags */
  guint menu_icon_is_blinking : 1;
  guint tooltip_visible : 1;
  guint switched_to_desktop : 1;
  guint is_dimming_on : 1;
  guint system_inactivity : 1;
  
  /* pointer location */
  guint pointer_on_button : 1;
  guint is_thumbable : 1;
  guint was_thumbable : 1;

  guint menu_button_timeout;

  GList *applications;
  
  GtkIconTheme *icon_theme;

#ifdef HAVE_LIBOSSO
  osso_context_t *osso;
#endif
  
  guint queue_refresh_id;

  /* sound samples data */
  gint esd_socket;
  gint start_sample;
  gint end_sample;
};

G_DEFINE_TYPE (HNAppSwitcher, hn_app_switcher, TASKNAVIGATOR_TYPE_ITEM);

static gint
get_app_button_pos (GtkWidget *button)
{
  gint pos;
  
  g_return_val_if_fail (GTK_IS_TOGGLE_BUTTON (button), 0);

  pos = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
					    "app-button-pos"));
  return pos;  
}

static void
set_app_button_pos (GtkWidget *button,
		    gint       pos,
		    gint       maxpos)
{
  g_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
  g_return_if_fail (pos >= 0 && pos < maxpos);

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
    g_warning ("Could not load icon '%s': %s\n",
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
                                             HDEntryInfo   *info)
{
  GdkPixbuf *retval;
  const gchar *icon_name;
 
  g_debug("New object does not have an icon -- using default");
  
  icon_name = hd_entry_info_get_app_icon_name (info);
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
#if 0
main_menu_detach  (HNAppSwitcher *app_switcher,
#else
main_menu_destroy (HNAppSwitcher *app_switcher,
#endif
                  GtkMenu   *menu)
{
  app_switcher->priv->main_menu = NULL;
  app_switcher->priv->main_home_item = NULL;
  app_switcher->priv->active_menu_item = NULL;

  GtkToggleButton *button = GTK_TOGGLE_BUTTON(app_switcher->priv->main_button);
  gtk_toggle_button_set_active (button, FALSE);
}

static void
main_home_item_activate_cb (GtkMenuItem   *menuitem,
			    HNAppSwitcher *app_switcher)
{
  g_debug ("Raising desktop");

  hd_wm_top_desktop ();
}

static void
main_menu_ensure_state (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GList *menu_children, *l;
  GtkWidget *separator;
  
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

  priv->active_menu_item = NULL;
  /* rebuild the menu */
  for (l = hd_wm_get_applications (app_switcher->hdwm); l != NULL; l = l->next)
  {
    GtkWidget *menu_item;
    const GList * children = hd_entry_info_get_children(l->data);
    const GList * child;

    for (child = children; child != NULL; child = child->next)
    {
      HDEntryInfo *entry = child->data;
	  
      g_debug ("Creating new app menu item (thumb:%s) for %s",
 	       priv->is_thumbable ? "on" : "off",
	       hd_entry_info_peek_title (entry));

      menu_item = hn_app_menu_item_new (entry, TRUE, priv->is_thumbable);

      if (hd_entry_info_is_active (entry))
        priv->active_menu_item = menu_item;
          
      gtk_menu_shell_append (GTK_MENU_SHELL (priv->main_menu), menu_item);
      gtk_widget_show (menu_item);
    }

    /* append the separator for this app*/
    separator = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (priv->main_menu), separator);

    gtk_widget_show (separator);
  }

  /* append home menu item */
  if (priv->main_home_item)
  {
    gtk_widget_destroy (priv->main_home_item);
    priv->main_home_item = NULL;
  }
  
  g_assert (priv->home_info); 
  priv->main_home_item = hn_app_menu_item_new (priv->home_info,
		                               FALSE,
					       priv->is_thumbable);
  g_assert (HN_IS_APP_MENU_ITEM (priv->main_home_item));
      
  g_signal_connect (priv->main_home_item, "activate",
		    G_CALLBACK (main_home_item_activate_cb),
		    app_switcher);
      
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->main_menu),
		 	 priv->main_home_item);
  gtk_widget_show (priv->main_home_item);
  
  g_object_ref (priv->main_home_item);

  priv->was_thumbable = priv->is_thumbable;
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
      hd_wm_focus_active_window (app_switcher->hdwm);
    }
    else
    {
      g_debug ("%s: %d, hd_wm_activate (HN_TN_ACTIVATE_KEY_FOCUS);",__FILE__,__LINE__);
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

  g_debug ("Main menu button released\n"
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
      g_debug ("deactivating menu");
      
      hd_wm_activate (HN_TN_ACTIVATE_LAST_APP_WINDOW);
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
#if 0
  gtk_menu_attach_to_widget (GTK_MENU (priv->main_menu),
                             GTK_WIDGET (app_switcher),
                             (GtkMenuDetachFunc)main_menu_detach);
#else
  g_signal_connect_swapped (priv->main_menu, "destroy",
                            G_CALLBACK (main_menu_destroy),
                            app_switcher);
#endif

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
  GtkRequisition req;

  if (!GTK_WIDGET_REALIZED (widget))
    return;

  if (hd_wm_fullscreen_mode())
    *x = 0;
  else
    gdk_window_get_origin (widget->window, x, y);
    *x += widget->allocation.width;

  gtk_widget_size_request ((HN_APP_SWITCHER (widget)->priv->main_menu), &req);
  *y = (*y + widget->allocation.y);/*FIXME: + req.height;*/

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
    HDEntryInfo       *child;
    GtkWidget         *app_image = gtk_bin_get_child (GTK_BIN (toggle));
    gint                i;
      
    hn_app_image_animation (app_image, FALSE);
    set_main_button_is_blinking (toggle, FALSE);

    /*
     * set the ignore flag on any children that were causing the blinking
     * we skip the first four apps, which cause blinking of the app buttons,
     * not the menu button.
     */
    for (k = hd_wm_get_applications (app_switcher->hdwm), i = 0; k != NULL; k = k->next, ++i)
    {
      if (i < priv->nitems)
        continue;
          
      for (l = hd_entry_info_get_children (k->data); l != NULL; l = l->next)
      {
        child = l->data;
        if (hd_entry_info_is_urgent(child))
          hd_entry_info_set_ignore_urgent(child, TRUE);
      }
    }
  }
  
  main_menu_build (app_switcher);

  gtk_widget_realize (priv->main_menu);

  if (priv->is_thumbable)
    gtk_widget_set_name (gtk_widget_get_toplevel (priv->main_menu),
                         "hildon-menu-window-thumb");
  else
    gtk_widget_set_name (gtk_widget_get_toplevel (priv->main_menu),
                         "hildon-menu-window-normal");
  
  if (priv->active_menu_item)
    gtk_menu_shell_select_item (GTK_MENU_SHELL (priv->main_menu),
                                priv->active_menu_item);
  else
    gtk_menu_shell_select_item (GTK_MENU_SHELL (priv->main_menu),
                                priv->main_home_item);


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
  else
  if (event->keyval == GDK_Left || event->keyval == GDK_KP_Left)
  {
    g_debug ("%s: %d, hd_wm_activate (HN_TN_ACTIVATE_LAST_APP_WINDOW);",__FILE__,__LINE__);
  }
  
  return FALSE;
}

static void
main_menu_button_toggled_cb (HNAppSwitcher *app_switcher,
			     GtkWidget     *toggle)
{
  HNAppSwitcherPrivate *priv;

  g_return_if_fail (app_switcher);

  priv = app_switcher->priv;

  if (priv->menu_button_timeout)
  {
    g_source_remove (priv->menu_button_timeout);
    priv->menu_button_timeout = 0;
  }

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)))
  {
    if (app_switcher->priv->main_menu)
      gtk_menu_popdown (GTK_MENU (app_switcher->priv->main_menu));
    return;
  }

  g_debug("Main menu button toggled");

  g_debug ("%s: %d, hd_wm_others_open ();",__FILE__,__LINE__);
 
  main_menu_pop (app_switcher, toggle);
}

static gboolean
menu_button_release_cb (GtkWidget      *widget,
                        GdkEventButton *event,
                        HNAppSwitcher  *app_switcher)
{
/*  gint x,y;*/
  HNAppSwitcherPrivate *priv;

  g_return_val_if_fail (event && app_switcher, FALSE);

  priv = app_switcher->priv;
#if 0
  
  /* if the relase is outside the button, reset the thumbable flag, to avoid
   * problems if the menu is raised some other way than via pointer
   */
  gtk_widget_get_pointer (widget, &x, &y);

  g_debug ("Main menu button released:\n"
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
      g_debug ("Release outside button -- resetting thumbable flag");
      priv->is_thumbable = FALSE;

      hd_wm_activate (HN_TN_ACTIVATE_LAST_APP_WINDOW);
    }
#endif

  return FALSE;
}

static gboolean
menu_button_pressed_timeout (HNAppSwitcher *app_switcher)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;

  g_source_remove (priv->menu_button_timeout);
  priv->menu_button_timeout = 0;

  /* If the thumbable status has changed since last time, we need to
   * rebuild the menu :( */
  if (priv->main_menu && priv->is_thumbable != priv->was_thumbable)
  {
    gtk_widget_destroy (priv->main_menu);
    priv->main_menu = NULL;
  }
  
  hn_app_switcher_toggle_menu_button (app_switcher);
  
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
  g_debug("Main menu button pressed using button %d", event->button);

/*  hd_wm_activate (HN_TN_DEACTIVATE_KEY_FOCUS);*/
#ifndef HILDON_LIBS
  if (event->button == APP_BUTTON_THUMBABLE || event->button == 2)
#else
  if (hildon_button_event_is_finger (event))
#endif
    priv->is_thumbable = TRUE;
  else 
  if (!priv->menu_button_timeout)
    priv->is_thumbable = FALSE;

  if (!priv->menu_button_timeout)
    priv->menu_button_timeout = g_timeout_add (100,
                                               (GSourceFunc)
                                                 menu_button_pressed_timeout,
                                               app_switcher);
                                               
  
  return TRUE;
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

  pixbuf = hn_app_switcher_get_icon_from_theme (app_switcher, AS_MENU_BUTTON_ICON, -1);
  icon = gtk_image_new_from_pixbuf (pixbuf);
  gtk_container_add (GTK_CONTAINER (button), icon);
  g_object_unref (pixbuf);

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
  gchar *name,*name_pressed;

  name =
   g_strdup_printf (AS_BUTTON_NAME,pos+1);

  name_pressed =
   g_strdup_printf (AS_BUTTON_NAME_PRESSED,pos+1);

  if (is_inconsistent)
    gtk_widget_set_name (widget, name);
  else
  {
    if (is_active)
      gtk_widget_set_name (widget, name_pressed);
    else
      gtk_widget_set_name (widget, name);
  }

  g_free (name);
  g_free (name_pressed);

  g_debug ("setting button (pos=%d) (inconsistent='<%s>', active='<%s>') name: %s",
	   pos,
	   is_inconsistent ? "true" : "false",
	   is_active ? "true" : "false",
	   gtk_widget_get_name (widget));
}

static void 
hn_app_switcher_show_menu_cb (HDWM *hdwm, gpointer data)
{
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (data);

  hn_app_switcher_toggle_menu_button (app_switcher);
}

static void 
hn_app_switcher_orientation_changed_cb (HNAppSwitcher *app_switcher, 
					GParamSpec *pspec)
{
  GList *children,*iter;
  
  children = 
    gtk_container_get_children (GTK_CONTAINER (app_switcher->box));

  if (children == NULL) return;
  else g_debug ("Changing orientation app_switcher");
  
  for (iter = children; iter; iter = g_list_next (iter))
  {	  
    g_object_ref (G_OBJECT (iter->data));
    gtk_container_remove (GTK_CONTAINER (app_switcher->box),GTK_WIDGET (iter->data));
  }

  gtk_container_remove (GTK_CONTAINER (app_switcher),GTK_WIDGET (app_switcher->box)); 

  if (HILDON_DESKTOP_PANEL_ITEM (app_switcher)->orientation == GTK_ORIENTATION_HORIZONTAL) 
    app_switcher->box = GTK_BOX (gtk_hbox_new (FALSE, 0));
  else
    app_switcher->box = GTK_BOX (gtk_vbox_new (FALSE, 0));

  gtk_container_add (GTK_CONTAINER (app_switcher),GTK_WIDGET (app_switcher->box));

  for (iter = children; iter; iter = g_list_next (iter))
  {
    gtk_container_add (GTK_CONTAINER (app_switcher->box),GTK_WIDGET (iter->data));
    g_object_unref (G_OBJECT (iter->data));
  }

  gtk_widget_show_all (GTK_WIDGET (app_switcher->box));

  queue_refresh_buttons (app_switcher);

  g_list_free (children);
}

static GtkWidget *
create_app_button (HNAppSwitcher *app_switcher,
	           gint           pos)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GtkWidget *retval;
  gchar *name;

  g_assert (priv->buttons[pos] == NULL);

  name = 
    g_strdup_printf (AS_BUTTON_NAME,pos+1);
  
  g_debug ("Creating app button at pos %d (name %s)", pos, name);
 
  retval = hn_app_button_new (NULL);

  g_object_set (retval, "app-switcher", app_switcher, NULL);

  set_app_button_pos (retval, pos, priv->nitems);
  hn_app_button_set_is_blinking (HN_APP_BUTTON (retval), FALSE);

  gtk_widget_set_name (retval, name);

  g_signal_connect (retval, "toggled",
		    G_CALLBACK (app_button_toggled_cb),
		    NULL);

  priv->buttons[pos] = retval;

  g_free (name);
  
  return priv->buttons[pos];
}

static void
hn_app_switcher_build (HNAppSwitcher *app_switcher)
{
  GtkWidget *button;
  GtkRequisition req;
  gint i;

  g_assert (app_switcher);

  app_switcher->priv->buttons = g_new0 (GtkWidget*,app_switcher->priv->nitems);

  app_switcher->hdwm = hd_wm_get_singleton ();

  g_object_ref (app_switcher->hdwm);

  if (!app_switcher->priv->home_info)
    app_switcher->priv->home_info = hd_entry_info_new (HD_ENTRY_DESKTOP);

  gtk_widget_push_composite_child ();

  /* inner box, used for padding */
  g_debug ("Adding inner VBox");

  if (HILDON_DESKTOP_PANEL_ITEM (app_switcher)->orientation == GTK_ORIENTATION_HORIZONTAL) 
    app_switcher->box = GTK_BOX (gtk_hbox_new (FALSE, 0));
  else
    app_switcher->box = GTK_BOX (gtk_vbox_new (FALSE, 0));

  gtk_widget_set_composite_name (GTK_WIDGET (app_switcher->box), "application-switcher-button-box");
  
  gtk_container_add (GTK_CONTAINER (app_switcher),GTK_WIDGET (app_switcher->box));
 
  gtk_widget_show (GTK_WIDGET (app_switcher->box));

  /* most recent applications buttons */
  g_debug ("Adding buttons");

  for (i=0;i < app_switcher->priv->nitems;i++)
  {
    button = create_app_button (app_switcher, i);
    gtk_box_pack_start (GTK_BOX (app_switcher->box), button, TRUE, TRUE, 0);
    gtk_widget_show (button);
  }
   
  /* menu button */
  g_debug ("Adding menu button");
  button = create_menu_button (app_switcher);
  gtk_box_pack_start (GTK_BOX (app_switcher->box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (app_switcher->hdwm,
		    "entry_info_added",
		    G_CALLBACK (hn_app_switcher_add_info_cb),
		    (gpointer)app_switcher);

  g_signal_connect (app_switcher->hdwm,
		    "entry_info_removed",
		    G_CALLBACK (hn_app_switcher_remove_info_cb),
		    (gpointer)app_switcher);
 
  g_signal_connect (app_switcher->hdwm,
		    "entry_info_changed",
		    G_CALLBACK (hn_app_switcher_changed_info_cb),
		    (gpointer)app_switcher);
 
  g_signal_connect (app_switcher->hdwm,
		    "entry_info_stack_changed",
		    G_CALLBACK (hn_app_switcher_changed_stack_cb),
		    (gpointer)app_switcher);

  g_signal_connect (app_switcher->hdwm,
		    "show_menu",
		    G_CALLBACK (hn_app_switcher_show_menu_cb),
		    (gpointer)app_switcher);

  g_signal_connect (app_switcher,
		    "notify::orientation",
		    G_CALLBACK (hn_app_switcher_orientation_changed_cb),
		    NULL);
 
  gtk_widget_pop_composite_child ();

  gtk_widget_size_request (GTK_WIDGET (app_switcher->box), &req);

  if (HILDON_DESKTOP_PANEL_ITEM (app_switcher)->orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (app_switcher), req.width, -1);
  else
    gtk_widget_set_size_request (GTK_WIDGET (app_switcher), -1, req.height);
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

#ifdef HAVE_LIBOSSO
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
  osso_hw_state_t hs = {0};
  
  priv->osso = osso_initialize ("AS_DIMMED_infoprint", "0.1", FALSE, NULL);

  if (!priv->osso)
  {
    g_warning("Failed to initialize libOSSO");
      
    return;
  }

  /* register stystem inactivity handler */
  hs.system_inactivity_ind = TRUE;
  osso_hw_set_event_cb(priv->osso, &hs,
                       hn_app_osso_hw_cb, app_switcher);
  
  
}
#endif

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
  if (hd_wm_get_applications (HN_APP_SWITCHER (widget)->hdwm) != NULL)
    gtk_widget_show (priv->main_button);
  else
    gtk_widget_hide (GTK_BIN (priv->main_button)->child);

  /* show only the buttons linked to an application */
  for (i = 0; i < priv->nitems; i++)
  {
    GtkWidget *button = priv->buttons[i];
    HDEntryInfo *info;
     
    info = hn_app_button_get_entry_info (HN_APP_BUTTON (button));
    if (info)
      gtk_widget_show (button);
  }
}
#if 0
static HDEntryInfo *
hn_app_switcher_find_app_for_child(HNAppSwitcher *app_switcher,
                                   HDEntryInfo *entry_info)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GList * l = priv->applications;
  HDWMWatchableApp *app = hd_entry_info_get_app(entry_info);
        
  while (l)
  {
    HDEntryInfo *e = (HDEntryInfo *)l->data;
            
    if (app == hd_entry_info_get_app(e))
      return e;
    l = g_list_next(l);
  }

  return NULL;
}
#endif
static void
remove_entry_from_app_button (HNAppSwitcher *app_switcher,
			      HDEntryInfo   *entry_info)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  gint pos;

  if (entry_info == NULL) return;

  for (pos = 0; pos < priv->nitems; pos++)
  {
    GtkWidget *button = priv->buttons[pos];
    HDEntryInfo *e;

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
                    HDEntryInfo   *entry,
                    gint           pos)
{
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  const GList          *l, *children = hd_entry_info_get_children(entry);
  gboolean              urgent = FALSE;
  HNAppButton          *app_button = HN_APP_BUTTON (priv->buttons[pos]);
  
  /* deal with urgency flags */
  for (l = children; l != NULL; l = l->next)
  {
    /*
     * If the entry is urgent and the ignore flag is not set, the button
     * should blink
     */
    if (hd_entry_info_is_urgent(l->data) &&
        !hd_entry_info_get_ignore_urgent(l->data))
    {
      g_debug("Found an urgent button");
      urgent = TRUE;
    }

      /*
       * if the info is not urgent, we need to clear any leftover
       * ignore_urgent flag
       */
    if(!hd_entry_info_is_urgent(l->data) &&
       hd_entry_info_get_ignore_urgent(l->data))
    {
      hd_entry_info_set_ignore_urgent(l->data, FALSE);
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

  g_debug ("buttons_group.size := %d",
	  g_slist_length (priv->buttons_group));

  hn_app_button_set_is_blinking (app_button, urgent);

  if (hd_entry_info_is_active (entry))
  {
    GtkToggleButton *button;

    button = GTK_TOGGLE_BUTTON (priv->buttons[pos]);
    gtk_toggle_button_set_inconsistent (button, FALSE);
    gtk_toggle_button_set_active (button, TRUE);
    gtk_toggle_button_toggled (button);
  }
  
  gtk_widget_set_sensitive (priv->buttons[pos], TRUE);
}

static gboolean
refresh_buttons (gpointer user_data)
{
  HNAppSwitcher        *app_switcher = HN_APP_SWITCHER (user_data);
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  GList                *l;
  gint                  pos;
  gint                  active_button = -1;
  gboolean              was_blinking;
  gboolean              is_urgent = FALSE;
  GtkWidget            *app_image;

  /* first we reset all the buttons icons */
  for (pos = 0; pos < priv->nitems; pos++)
  {
    HNAppButton *button = HN_APP_BUTTON (priv->buttons[pos]);

    hn_app_button_set_group (button, NULL);
    hn_app_button_set_entry_info (button, NULL);
  }

  priv->buttons_group = NULL;

  /* then refresh the icons of the application buttons */
  for (l = hd_wm_get_applications (app_switcher->hdwm), pos = 0;
       l != NULL && pos < priv->nitems;
       l = l->next, pos++)
  {
    HDEntryInfo *entry = l->data;

    /* we just want the most recently used top-level applications */
    if (entry->type != HD_ENTRY_WATCHED_APP)
    {
      g_debug("Object is not an application");
      continue;
    }

    if (active_button < 0 &&
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->buttons[pos]))&&
        !hd_entry_info_is_active (entry))
    {
        active_button = pos;
    }
      
    refresh_app_button (app_switcher, entry, pos);

    g_debug ("Showing object");
  }

  if (active_button >= 0)
  {
    GtkToggleButton *button;
      
    g_debug ("Unsetting the previously active button %d",
             active_button);

    button = GTK_TOGGLE_BUTTON (priv->buttons[active_button]);
    gtk_toggle_button_set_inconsistent (button, TRUE);
    gtk_toggle_button_set_active (button, FALSE);
    gtk_toggle_button_toggled (button);
  }
  
  /*
   * now we need to process the menu button to ensure that it blinks
   * if needed
   */
  was_blinking = get_main_button_is_blinking (priv->main_button);

  is_urgent = FALSE;
  for (l = hd_wm_get_applications (app_switcher->hdwm), pos = 0;
       l != NULL;
       l = l->next, ++pos)
  {
    const GList       *k;
    HDEntryInfo       *child;

    if (pos < priv->nitems)
      continue;

    /* set the ignore flag on any children that were causing the blinking
     * we skip the first four apps, which cause blinking of the app buttons,
     * not the menu button.
     */
    for (k = hd_entry_info_get_children (l->data); k != NULL; k = k->next)
    {
      child = k->data;

     /* If the entry is urgent and the ignore flag is not set, the
      * button should blink
      */
      
      if (hd_entry_info_is_urgent(child) && !hd_entry_info_get_ignore_urgent(child))
        is_urgent = TRUE;
         
      /*
       * if the info is not urgent, we need to clear any leftover
       * ignore_urgent flag
       */
      if (!hd_entry_info_is_urgent(child) && hd_entry_info_get_ignore_urgent(child))
        hd_entry_info_set_ignore_urgent(child, FALSE);
       
    }
  }

  app_image = gtk_bin_get_child (GTK_BIN (priv->main_button));
  g_assert (GTK_IS_IMAGE (app_image));

  if (is_urgent && !was_blinking && !priv->system_inactivity)
  {
    g_debug("Setting menu button urgency to '%s'",
           is_urgent ? "true" : "false");
    hn_app_image_animation (app_image, is_urgent);
    set_main_button_is_blinking (priv->main_button, is_urgent);
  }
  else 
  if (was_blinking && priv->system_inactivity)
  {
    g_debug("Setting menu button urgency to 'false'", FALSE);

    hn_app_image_animation (app_image, FALSE);
    set_main_button_is_blinking (priv->main_button, FALSE);
  }

  /* hide the main button if no more applications are left */
  if (!hd_wm_get_applications (app_switcher->hdwm))
  {
    g_debug ("Hiding main button icon");

    gtk_widget_hide (app_image);
    gtk_widget_set_sensitive (priv->main_button, FALSE);
  }
  else
  {
    g_debug ("Showing main button icon");
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
		    				 100,
		                                 refresh_buttons,
					         app_switcher,
					         queue_refresh_done);
}

/* Class closure for the "add" signal; this is called each time an
 * entry has been added to the applications list.
 */
static void
hn_app_switcher_add_info_cb (HDWM *hdwm, HDEntryInfo *entry_info, gpointer data)
{
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (data);
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  
  if (!entry_info)
  {
    g_warning ("%s: No entry info provided!",__FILE__);
    return;
  }

  /* Play a sound */
  if (hn_as_sound_play_sample (priv->esd_socket, priv->start_sample) == -1)
  {
      /* Connection to esd was probably closed */
    hn_as_sound_deinit (priv->esd_socket);
    hn_app_switcher_init_sound_samples (app_switcher);
    hn_as_sound_play_sample (priv->esd_socket, priv->start_sample);
  }

  queue_refresh_buttons (app_switcher);
  
  /* we must explicitely ensure that the main menu is rebuilt,
   * now that the application list is tainted by a new entry;
   * destroying the menu widget will call the detach function
   */
  if (priv->main_menu)
  {
    gtk_widget_destroy (priv->main_menu);
    priv->main_menu = NULL;
  }
}

/* Class closure for the "remove" signal; this is called each time
 * an entry has been remove from the application list
 */
static void
hn_app_switcher_remove_info_cb (HDWM *hdwm, 
				gboolean removed_app, 
				HDEntryInfo *entry_info, 
				gpointer data)
{
  HNAppSwitcher *app_switcher = HN_APP_SWITCHER (data);
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  HDEntryInfo *info_parent = hd_entry_info_get_parent(entry_info);
  
  /* Play a sound */
  if (hn_as_sound_play_sample (priv->esd_socket, priv->end_sample) == -1)
  {
    /* Connection to esd was probably closed */
    hn_as_sound_deinit (priv->esd_socket);
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
  {
      gtk_widget_destroy (priv->main_menu);
      priv->main_menu = NULL;
    }

}


/* Class closure for the "changed" signal; this is called each time
 * an entry inside the applications list has been changed by the WM,
 * for instance when the icon or the title changes.
 *
 * NB: entry_info can be NULL for global update
 */
static void
hn_app_switcher_changed_info_cb (HDWM *hdwm, HDEntryInfo *entry_info, gpointer data)
{
  HNAppSwitcher        *app_switcher = HN_APP_SWITCHER (data);
  HNAppSwitcherPrivate *priv = app_switcher->priv;
  g_debug ("In hn_app_switcher_real_changed_info");

  /* all changes have potential impact on the the main menu; app menus are
   * created on the fly, so we do not have to worry about menu changes there
   */
  if (priv->main_menu)
  {
    gtk_widget_destroy (priv->main_menu);
    priv->main_menu = NULL;
  }

  /*
   * If we are given an entry info and it of the app type, we just need to
   * update at most one button
   */
  if (entry_info && entry_info->type == HD_ENTRY_WATCHED_APP)
  {
    gint                  pos;
    GList *               l;

    g_debug ("HDEntryInfo present and of type WATCHED_APP");
      
    for (l = hd_wm_get_applications (hdwm), pos = 0;
         l != NULL && pos < priv->nitems;
         l = l->next, pos++)
    {
      HDEntryInfo *entry = l->data;
          
      if (entry->type != HD_ENTRY_WATCHED_APP)
      {
        g_debug("Object is not an application");
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
  else 
  if (entry_info)
  {
    /* this is a change in a child entry; in addition to the main menu impact
     * we already dealt with, child changes affect the blinking of the
     * associated buttons
     */
    HDEntryInfo *parent;
    GtkWidget   *button = NULL;
    GList       *l;
    gint         pos;
      
    g_return_if_fail (entry_info->type != HD_ENTRY_INVALID);
    g_debug ("HDEntryInfo present, with child entry");

    parent = hd_entry_info_get_parent (entry_info);

    if (!parent)
    {
      g_critical ("Attempting to change orphan child item.");
      return;
    }

    /* if the info is urgent but has an ignore flag, we know that it was not
     * causing the button to blink, and will not cause the button to blink,
     * so we do not need to update the buttons
     */
    if (hd_entry_info_is_urgent (entry_info) &&
        hd_entry_info_get_ignore_urgent (entry_info))
    {
      return;
    }

    /* if the info is not urgent, but has the ignore flag set, we have to
     * clear that flag; we also know that it was not causing the associated
     * button to blink (it was being ingored), so we do not need to update
     * the button.
     */
    if (!hd_entry_info_is_urgent (entry_info) &&
        hd_entry_info_get_ignore_urgent (entry_info))
    {
      hd_entry_info_set_ignore_urgent (entry_info, FALSE);
      return;
    }

    /* the remaining cases are more complicated and require that we know
     * the state of the associated button
     */
    for (l = hd_wm_get_applications (hdwm), pos = 0;
         l != NULL && pos < priv->nitems;
         l = l->next, pos++)
    {
      HDEntryInfo *entry = l->data;
      
      /* we just want the most recently used top-level applications */
      if (entry->type != HD_ENTRY_WATCHED_APP)
      {
        g_debug("Object is not an application");
        continue;
      }

      if (entry == parent)
      {
        button = priv->buttons[pos];
        break;
      }
    }

    if (button)
    {
      g_debug ("Force the button's icon to update itself");
      hn_app_button_force_update_icon (HN_APP_BUTTON (button));
    }

    HN_MARK();

      /* if the current info is urgent and not to be ignored, we know the app
       * button should blink; make sure it does
       */
    if (hd_entry_info_is_urgent (entry_info) &&
        !hd_entry_info_get_ignore_urgent (entry_info))
    {
      if (button)
      {
        /* child of one of the app buttons */
        if (!hn_app_button_get_is_blinking (HN_APP_BUTTON (button)))
          hn_app_button_set_is_blinking (HN_APP_BUTTON (button), TRUE);
      }
      else
      {
        /* the child belongs to the main menu button */
        if (!get_main_button_is_blinking (priv->main_button))
        {
          GtkWidget *app_image =
            gtk_bin_get_child (GTK_BIN (priv->main_button));

          g_debug("Setting menu button urgency to %d", TRUE);
                  
          hn_app_image_animation (app_image, TRUE);
          set_main_button_is_blinking (priv->main_button, TRUE);
        }
      }
          
      return;
    }

    /* if the info is not urgent and it is not being ignored and the
     * associated button is not blinking, we know that it was not urgent
     * previously, so we do not need to update the button
     */
    if (!hd_entry_info_is_urgent (entry_info) &&
        !hd_entry_info_get_ignore_urgent (entry_info))
    {
      if ((button && !hn_app_button_get_is_blinking (HN_APP_BUTTON (button))) ||
         (!button && !get_main_button_is_blinking (priv->main_button)))
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
  
  g_debug ("Queuing a refresh cycle of the buttons (info: %s)",
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
hn_app_switcher_changed_stack_cb (HDWM *hdwm, HDEntryInfo *entry_info, gpointer data)
{
  HNAppSwitcher	       * app_switcher = HN_APP_SWITCHER (data);
  gint                   pos, active_pos;
  GList                * l;
  HNAppSwitcherPrivate * priv = app_switcher->priv;
  HDEntryInfo          * parent;
  gboolean               active_found = FALSE;
  
  g_debug ("In hn_app_switcher_real_changed_stack");

  if (priv->main_menu)
  {
    gtk_widget_destroy (priv->main_menu);
    priv->main_menu = NULL;
  }
  
  if (!entry_info || !hd_entry_info_is_active (entry_info))
  {
    /* rebuild everything, since we were not told what has been topped
     * issue warning, as this is not how this function is meant to be
     * called
     */
    g_warning ("No entry_info provided");
      
    queue_refresh_buttons (app_switcher);
    return;
  }
  
  if (entry_info->type == HD_ENTRY_WATCHED_APP)
  {
    /* we only accept entries for windows and views */
    g_warning ("Cannot handle HD_ENTRY_WATCHED_APP");
    return;
  }
  
  if (entry_info->type != HD_ENTRY_DESKTOP)  
  {
     parent = hd_entry_info_get_parent (entry_info);
     
     if (!parent)
     {
       g_warning ("Orphan entry info");
       return;
     } 

     /* locate the associated button, and toggle it */
     active_pos = 0;
     for (l = hd_wm_get_applications (hdwm), pos = 0;
          l != NULL && pos < priv->nitems;
          l = l->next, pos++)
     {
       HDEntryInfo *entry = l->data;
          
       if (parent == entry)
       {
         hn_app_button_make_active (HN_APP_BUTTON(priv->buttons[pos]));
	 active_pos = pos;
	 active_found = TRUE;
	 break;
       }
     }
  }

  /* no active window in the buttons, make sure that
   * no button is active
   */  
  if (!active_found)
  {
    for (pos = 0; pos < priv->nitems; pos++)
    {
      GtkToggleButton *app_button;

      g_debug ("Setting inconsistent state for pos %d", pos);

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
hn_app_switcher_finalize (GObject *gobject)
{
  HNAppSwitcher *app_switch = HN_APP_SWITCHER (gobject);
  HNAppSwitcherPrivate *priv = app_switch->priv;

#ifdef HAVE_LIBOSSO  
  osso_deinitialize (priv->osso);
#endif

  if (priv->home_info)
    hd_entry_info_free (priv->home_info);

  g_object_unref (app_switch->hdwm);

  g_debug ("Destroying HNAppSwitcher");

  G_OBJECT_CLASS (hn_app_switcher_parent_class)->finalize (gobject);
}

static GObject *
hn_app_switcher_constructor (GType                  type,
			     guint                  n_construct_params,
			     GObjectConstructParam *construct_params)
{
  GObject *object;
  HNAppSwitcher *appswitcher;

  g_debug ("inside hn_app_switcher_constructor...");

  
  object      = G_OBJECT_CLASS (hn_app_switcher_parent_class)->constructor (type,n_construct_params, construct_params);
  appswitcher = HN_APP_SWITCHER (object);
  
  g_debug ("building HNAppSwitcher widget");
      
  hn_app_switcher_build (appswitcher);

#ifdef HAVE_LIBOSSO
  hn_app_switcher_osso_initialize (appswitcher);
#endif

  return object;
}

static void 
hn_app_switcher_set_property (GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  HNAppSwitcher *app_switcher;

  app_switcher = HN_APP_SWITCHER (object);

  switch (prop_id)
  {
    case AS_PROP_NITEMS:	   
      app_switcher->priv->nitems = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  } 
}

static void 
hn_app_switcher_get_property (GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
  HNAppSwitcher *app_switcher;

  app_switcher = HN_APP_SWITCHER (object);

  switch (prop_id)
  {
    case AS_PROP_NITEMS:
      g_value_set_int (value,app_switcher->priv->nitems);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;						
  }
}

static void
hn_app_switcher_class_init (HNAppSwitcherClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  gobject_class->finalize = hn_app_switcher_finalize;
  gobject_class->constructor = hn_app_switcher_constructor;

  gobject_class->set_property = hn_app_switcher_set_property;
  gobject_class->get_property = hn_app_switcher_get_property;

  widget_class->show_all = hn_app_switcher_show_all;
 
  g_type_class_add_private (klass, sizeof (HNAppSwitcherPrivate));

  g_object_class_install_property (gobject_class,
                                   AS_PROP_NITEMS,
                                   g_param_spec_int("n_items",
                                                    "number_of_items",
                                                    "Number of items",
                                                    1,
						    20,
						    4,
                                                    G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
hn_app_switcher_init (HNAppSwitcher *app_switcher)
{
  app_switcher->priv = HN_APP_SWITCHER_GET_PRIVATE (app_switcher);
  
  app_switcher->priv->buttons_group = NULL;

  gtk_widget_set_name (GTK_WIDGET (app_switcher), AS_BOX_NAME);
  
  /* set base properties of the app-switcher widget */
  
  gtk_container_set_border_width (GTK_CONTAINER (app_switcher), 0);

  hn_app_switcher_init_sound_samples (app_switcher);
}


/* Public API */

GtkWidget *
hn_app_switcher_new (gint nitems)
{
  return g_object_new (HN_TYPE_APP_SWITCHER,"n_items",nitems,NULL);
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
  entries = hd_wm_get_applications (app_switcher->hdwm);

  for (l = entries; l != NULL; l = l->next)
  {
    HDEntryInfo *info = l->data;

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

  if (!hd_wm_get_applications (app_switcher->hdwm))
    return;
  
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

HDEntryInfo *
hn_app_switcher_get_home_entry_info (HNAppSwitcher *app_switcher)
{
  g_return_val_if_fail (app_switcher, NULL);

  return app_switcher->priv->home_info;
}
