/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/* hn-app-button.c
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

/* Hildon includes */
#include "hn-app-button.h"
#include "hn-app-menu-item.h"
#include "hn-app-tooltip.h"
#include "hn-wm.h"
#include "hn-entry-info.h"
#include "hn-wm-watchable-app.h"
#include "hildon-pixbuf-anim-blinker.h"
#include "hn-app-switcher.h"

#include <stdlib.h>
#include <string.h>

/* GLib include */
#include <glib.h>
#include <glib/gi18n.h>

/* GTK includes */
#include <gtk/gtkwidget.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtkseparatormenuitem.h>
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

/* application button compose icon names */
static const gchar *app_group_icons[] = {
  NULL,                    /* single instance: no icon */
  "qgn_indi_grouped2",
  "qgn_indi_grouped3",
  "qgn_indi_grouped4",
  "qgn_indi_grouped5",
  "qgn_indi_grouped6",
  "qgn_indi_grouped7",
  "qgn_indi_grouped8",
  "qgn_indi_grouped9",
  
  "qgn_indi_grouped_more", /* 9+ instances: keep last! */
};
static guint app_group_n_icons = G_N_ELEMENTS (app_group_icons);

/* fast mnemonic value for the "more" icon name */
#define APP_GROUP_ICON_MORE 	(app_group_icons[app_group_n_icons - 1])
#define APP_GROUP_ICON_SIZE     16

/* Hardcoded pixel perfecting values */
#define BUTTON_HEIGHT      38

#define AS_BUTTON_BORDER_WIDTH  0
#define AS_MENU_BORDER_WIDTH    20
#define AS_TIP_BORDER_WIDTH 	20
#define AS_ROW_HEIGHT 		30
#define AS_ICON_SIZE            26
#define AS_TOOLTIP_WIDTH        360
#define AS_MENU_ITEM_WIDTH      360
#define AS_INTERNAL_PADDING     10
#define AS_SEPARATOR_HEIGHT     10
#define AS_BUTTON_BOX_PADDING   10

enum
{
  BUTTON_PROP_0,
  BUTTON_PROP_ENTRY_INFO,
  BUTTON_PROP_IS_BLINKING
};

#define HN_APP_BUTTON_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HN_TYPE_APP_BUTTON, HNAppButtonPrivate))

struct _HNAppButtonPrivate
{
  HNEntryInfo *info;
  GtkWidget *icon;

  GtkToggleButton *prev_button;
  
  GtkWidget *tooltip;
  GtkWidget *menu;

  guint is_blinking  : 1;
  guint is_thumbable : 1;
};


G_DEFINE_TYPE (HNAppButton, hn_app_button, GTK_TYPE_TOGGLE_BUTTON);


#if 0
/* TODO - uncomment to handle keyboard focus */
static gboolean
hn_app_button_focus (GtkWidget        *widget,
		     GtkDirectionType  direction)
{
  return TRUE;
}
#endif

static void
hn_app_button_icon_animation (GtkWidget *icon, gboolean   turn_on);

static void
hn_app_button_finalize (GObject *gobject)
{
  HNAppButton *app_button = HN_APP_BUTTON (gobject);

  if (app_button->priv->tooltip)
    gtk_widget_destroy (app_button->priv->tooltip);
  
  G_OBJECT_CLASS (hn_app_button_parent_class)->finalize (gobject);
}

static void
hn_app_button_destroy (GtkObject *object)
{
  HNAppButton *app_button = HN_APP_BUTTON (object);
  HNAppButton *tmp_button;
  GSList *l;
  
  app_button->group = g_slist_remove (app_button->group, app_button);

  for (l = app_button->group; l != NULL; l = l->next)
    {
      tmp_button = l->data;

      tmp_button->group = app_button->group;
    }
  
  app_button->group = NULL;
  
  if (GTK_OBJECT_CLASS (hn_app_button_parent_class)->destroy)
    GTK_OBJECT_CLASS (hn_app_button_parent_class)->destroy (object);
}

/* static implementation of the _gtk_button_set_depressed semi-private
 * function inside GTK+; keep in sync with that
 */
static void
hn_app_button_set_depressed (HNAppButton *app_button,
			     gboolean     depressed)
{
  GtkWidget *widget = GTK_WIDGET (app_button);

  depressed = depressed != FALSE;

  if (depressed != GTK_BUTTON (app_button)->depressed)
    {
      GTK_BUTTON (app_button)->depressed = depressed;
      gtk_widget_queue_resize (widget);
    }
}

/* taken from gtkradiobutton.c */
static void
hn_app_button_clicked (GtkButton *button)
{
  HNAppButton *app_button = HN_APP_BUTTON (button);
  GtkToggleButton *toggle_button = GTK_TOGGLE_BUTTON (button);
  GtkToggleButton *tmp_button;
  GtkStateType new_state;
  gboolean toggled = FALSE;
  gboolean depressed;

  g_object_ref (button);

  if (toggle_button->active)
    {
      GSList *l;
      
      gboolean found = FALSE;
      tmp_button = NULL;
      
      for (l = app_button->group; l != NULL; l = l->next)
        {
          tmp_button = l->data;

	  if (tmp_button->active && tmp_button != toggle_button)
	    {
              found = TRUE;
	      break;
	    }
        }

      if (!found && app_button->priv->info != NULL)
	new_state = (button->in_button ? GTK_STATE_PRELIGHT : GTK_STATE_ACTIVE);
      else
        {
          toggled = TRUE;
	  toggle_button->active = !toggle_button->active;
          new_state = (button->in_button ? GTK_STATE_PRELIGHT : GTK_STATE_NORMAL);
	}
    }
  else
    {
      if (app_button->priv->info != NULL)
        {
          GSList *l;
	  
          toggled = TRUE;
	  toggle_button->active = !toggle_button->active;

	  for (l = app_button->group; l != NULL; l = l->next)
            {
              tmp_button = l->data;

	      if (tmp_button->active && (tmp_button != toggle_button))
	        {
                  gtk_button_clicked (GTK_BUTTON (tmp_button));
	          break;
	        }
            }

	  new_state = (button->in_button ? GTK_STATE_PRELIGHT : GTK_STATE_ACTIVE);
        }
      else
        new_state = (button->in_button ? GTK_STATE_PRELIGHT : GTK_STATE_NORMAL);
    }

  if (toggle_button->inconsistent)
    depressed = FALSE;
  else if (button->in_button && button->button_down)
    depressed = !toggle_button->active;
  else
    depressed = toggle_button->active;

  if (GTK_WIDGET_STATE (button) != new_state)
    gtk_widget_set_state (GTK_WIDGET (button), new_state);

  if (toggled)
    {
      gtk_toggle_button_toggled (toggle_button);

      g_object_notify (G_OBJECT (toggle_button), "active");
    }

  hn_app_button_set_depressed (app_button, depressed);

  gtk_widget_queue_draw (GTK_WIDGET (button));

  g_object_unref (button);
}

static void
menu_position_func (GtkMenu  *menu,
		    gint     *x,
		    gint     *y,
		    gboolean *push_in,
		    gpointer  user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  GdkScreen *screen = gtk_widget_get_screen (widget);
  GtkRequisition req;
  gint out_y;

  if (!GTK_WIDGET_REALIZED (widget))
    return;

  gdk_window_get_origin (widget->window, x, y);

  gtk_widget_size_request (GTK_WIDGET (menu), &req);

  /* align with the current toggle button, but clamp on
   * the screen size
   */
  *x += widget->allocation.width;
  out_y = widget->allocation.y + req.height;
  if (out_y > gdk_screen_get_height (screen))
    *y += widget->allocation.y - (out_y - gdk_screen_get_height (screen));
  else
    *y += widget->allocation.y;

  *push_in = FALSE;
}

static GtkWidget *
hn_app_button_create_menu (HNAppButton *app_button)
{
  GtkWidget *menu;
  GtkWidget *active_item = NULL;
  HNEntryInfo *info;
  const GList *children, *l;
  gint width;
  GtkRequisition req;

  info = app_button->priv->info;
  g_assert (info != NULL);

  menu = gtk_menu_new ();

  children = hn_entry_info_get_children (info);
  for (l = children; l != NULL; l = l->next)
    {
      GtkWidget *menu_item;

      menu_item =
        hn_app_menu_item_new(l->data, FALSE, app_button->priv->is_thumbable);

      /* the G spec says the first item should be selected */
      if (!active_item)
        active_item = menu_item;

      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      gtk_widget_show (menu_item);
    }

  if (active_item)
    gtk_menu_shell_select_item (GTK_MENU_SHELL (menu), active_item);

  width = MIN (menu->allocation.width, AS_MENU_ITEM_WIDTH);
  gtk_widget_set_size_request (menu, -1, -1);
  gtk_widget_size_request (menu, &req);
  gtk_widget_set_size_request (menu,
		  	       MAX (width, req.width),
			       -1);

  return menu;
}

void
hn_app_button_make_active (HNAppButton *button)
{
  g_return_if_fail (button);

  gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON (button));

  /* reset the previous button to NULL -- this function is called by
     AS in response to MB stacking order changes
   */
  button->priv->prev_button = NULL;
}

static void
menu_unmap_cb (GtkWidget   *widget,
	       HNAppButton *app_button)
{
  gtk_widget_destroy (widget);
  app_button->priv->menu = NULL;

  /* we always deactivate the button when the menu closes, and wait for MB
   * notification of change in stacking order to ensure that the button
   * selection always matches the stacking order (i.e., if the app fails to
   * start or top, we do not want the button selected)
   */
  if (app_button->priv->prev_button &&
      app_button->priv->prev_button != GTK_TOGGLE_BUTTON(app_button))
    {
      HN_DBG ("Retoggling previously toggled button");
	 
      gtk_toggle_button_set_active (app_button->priv->prev_button, TRUE);
      gtk_toggle_button_toggled (app_button->priv->prev_button);
    }
  
}

static gboolean
menu_button_release_cb (GtkWidget      *widget,
                        GdkEventButton *event,
                        HNAppButton *app_button)
{
  HNAppButtonPrivate *priv = app_button->priv;
  gint x, y;
  
  if (app_button && priv->tooltip)
    {
      hn_app_tooltip_remove_show_timer (HN_APP_TOOLTIP (priv->tooltip));
      
      gtk_widget_destroy (priv->tooltip);
      priv->tooltip = NULL;
    }

  /*
   * if the button is released outside of the menu, we need to deactivate
   * the button
   */
  gtk_widget_get_pointer(widget, &x, &y);

  HN_DBG ("pointer [%d,%d],\n"
          "allocation [%d, %d, %d, %d]",
          x, y,
          widget->allocation.x,
          widget->allocation.y,
          widget->allocation.width,
          widget->allocation.height);

  if(x > widget->allocation.width  || x < 0 ||
     y > widget->allocation.height || y < 0)
    {
      /* pointer outside the menu, i.e., menu cancelled */
      gtk_menu_shell_deactivate(GTK_MENU_SHELL(widget));

      if (app_button->priv->prev_button)
        {
          HN_DBG ("Retoggling previously toggled button");
	 
          gtk_toggle_button_set_active (app_button->priv->prev_button, TRUE);
          gtk_toggle_button_toggled (app_button->priv->prev_button);
        }

      hn_wm_activate(HN_TN_ACTIVATE_LAST_APP_WINDOW);
      return TRUE;
    }
  
  return FALSE;
}

static gboolean
menu_keypress_cb (GtkWidget *menu, GdkEventKey *event, HNAppButton * app_button)
{
  g_return_val_if_fail(event, FALSE);

  if (event->keyval == GDK_Left ||
      event->keyval == GDK_KP_Left ||
      event->keyval == GDK_Escape)
    {
      gtk_menu_shell_deactivate (GTK_MENU_SHELL (menu));
      
      if (event->keyval == GDK_Escape)
        {
          /* pass focus to the last active application */
		  hn_wm_focus_active_window ();
        }
      else
        {
          /* pointer outside the button, i.e., press canceled */
          if (app_button->priv->prev_button)
            {
              HN_DBG ("Retoggling previously toggled button");
	 
              gtk_toggle_button_set_active (app_button->priv->prev_button, TRUE);
              gtk_toggle_button_toggled (app_button->priv->prev_button);
            }

		  hn_wm_activate(HN_TN_ACTIVATE_KEY_FOCUS);
		  gtk_widget_grab_focus (GTK_WIDGET (app_button));
        }
      
      return TRUE;
    }

  return FALSE;
}

static gboolean
hn_app_button_pop_menu (HNAppButton *app_button)
{
  guint            n_children;
  HNEntryInfo     *info;

  g_return_val_if_fail(app_button, FALSE);

  info = app_button->priv->info;
  g_return_val_if_fail(info, FALSE);
  
  n_children = hn_entry_info_get_n_children (info);
  if (n_children == 1)
    {
      /* pointer released in the app button, top our app */
      const GList *child;
      gboolean was_blinking;

      child = hn_entry_info_get_children (app_button->priv->info);

      /* stop the blinking if needed */
      was_blinking = hn_app_button_get_is_blinking (app_button);
      if (was_blinking)
        {
          hn_app_button_set_is_blinking (app_button, FALSE);
	  
	  if (hn_entry_info_is_urgent (child->data))
            hn_entry_info_set_ignore_urgent (child->data, TRUE);
	}
      
      hn_wm_top_item (child->data);
    }
  else
    {
      GtkWidget *menu;
      gboolean   was_blinking;

      /* multiple instances: show the menu and let the
       * user choose which one to top; if the user unmaps
       * the menu, reset the active state to the
       * previously set button.
       */

      /* first, stop the button blinking */
      was_blinking = hn_app_button_get_is_blinking (app_button);
  
      if (was_blinking)
        {
          const GList *l;
      
          hn_app_button_set_is_blinking (app_button, FALSE);

          /*
           * set the ignore flag on any children that were causing the
           * blinking
           */
          for (l = hn_entry_info_get_children (info); l != NULL; l = l->next)
            {
              if (hn_entry_info_is_urgent(l->data))
                hn_entry_info_set_ignore_urgent(l->data, TRUE);
            }
        }
      
      menu = hn_app_button_create_menu (app_button);
      if (!menu)
        {
          g_warning ("Unable to create the menu");
          return FALSE;
        }

      gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
                      menu_position_func, app_button,
		      0, GDK_CURRENT_TIME);

      g_signal_connect (menu, "unmap",
                        G_CALLBACK (menu_unmap_cb),
                        app_button);

      g_signal_connect (menu, "button-release-event",
                        G_CALLBACK (menu_button_release_cb),
                        app_button);

      g_signal_connect (menu, "key-press-event",
                        G_CALLBACK (menu_keypress_cb),
                        app_button);
      
      app_button->priv->menu = menu;
    }

  return TRUE;
}

static gboolean
hn_app_button_key_press_event (GtkWidget * widget,
                               GdkEventKey *event)
{
  HNAppButton *app_button = HN_APP_BUTTON (widget);
  GtkToggleButton * toggle_button = GTK_TOGGLE_BUTTON (widget);
  GSList * l;
  GtkToggleButton *tmp_button = NULL;

  if (!event)
    {
      HN_DBG("no event given !!!");
      return FALSE;
    }
  
  if (event->keyval == GDK_Right    ||
      event->keyval == GDK_KP_Right ||
      event->keyval == GDK_KP_Enter ||
      event->keyval == GDK_ISO_Enter||
      event->keyval == GDK_Return)
    {
      /* search for the toggled button, so that we can re-toggle
       * it in case the user didn't top the window/application
       */
      
      app_button->priv->prev_button = NULL;
      for (l = app_button->group; l != NULL; l = l->next)
        {
          tmp_button = l->data;

          if (tmp_button->active && (tmp_button != toggle_button))
            {
              app_button->priv->prev_button = tmp_button;
              break;
            }
        }
  
      gtk_toggle_button_set_active (toggle_button, TRUE);
      gtk_toggle_button_toggled (toggle_button);
  
      hn_app_button_pop_menu (app_button);
      return TRUE;
    }
  else if(event->keyval == GDK_Left || event->keyval == GDK_KP_Left)
	{
      HN_DBG("left keypress -- passing focus to last active app");
      hn_wm_activate(HN_TN_ACTIVATE_LAST_APP_WINDOW);
	}

  return FALSE;
}

static gboolean
hn_app_button_release_event (GtkWidget      *widget,
	                     GdkEventButton *event)
{
  HNAppButton *app_button = HN_APP_BUTTON (widget);
  HNAppButtonPrivate *priv = app_button->priv;
  gint x,y;
  gboolean force_untoggle = FALSE;
  gboolean untoggle = FALSE;
  
  HN_DBG("Button released ...");

  if (priv->tooltip)
    {
      HN_DBG("removing tooltip timer");
      
      /* have to remove the show timer, so that if the tooltip is not
       * yet showing, it does not get displayed
       */
      hn_app_tooltip_remove_show_timer (HN_APP_TOOLTIP (priv->tooltip));
      
      gtk_widget_destroy (priv->tooltip);
      priv->tooltip = NULL;
    }

  gtk_widget_get_pointer(widget, &x, &y);

  HN_DBG ("pointer [%d,%d],\n"
          "allocation [%d, %d, %d, %d]",
          x, y,
          widget->allocation.x,
          widget->allocation.y,
          widget->allocation.width,
          widget->allocation.height);

  if(x > widget->allocation.width  || x < 0 ||
     y > widget->allocation.height || y < 0)
    {
      untoggle = TRUE;
    }
  else if (priv->info && hn_entry_info_get_n_children (priv->info) == 1)
    {
      /* single window application, i.e., no submenu -- have to untogle
       * the button and wait for MB notification of change in stacking order
       * to make the toggle permanent
       */
      force_untoggle = TRUE;
    }
    
  if(untoggle || force_untoggle)
    {
      if (priv->prev_button)
        {
          HN_DBG ("Retoggling previously toggled button");
	 
	  gtk_toggle_button_set_active (priv->prev_button, TRUE);
	  gtk_toggle_button_toggled (priv->prev_button);
	}

      if (untoggle)
        {
          /* click canceled -- we are done */
          goto out;
        }
    }
  
  hn_app_button_pop_menu (app_button);

 out:
  /* reset the thumbable flag, to avoid problems if the menu is raised
   * some other way than via pointer (should really only be an issue for
   * the main menu button, but does us no harm to do it here too)
   */
  priv->is_thumbable = FALSE;

#if 0
  /*
   * must not reset here, we will need this in case the user cancels the
   * menu via keypress
   */
  priv->prev_button = NULL;
#endif
  
  /* allow the CB chain to continue */
  return FALSE;
}

static gboolean
hn_app_button_press_event (GtkWidget      *widget,
                           GdkEventButton *event)
{
  HNAppButton *app_button;
  HNAppButtonPrivate *priv;
  GtkButton *button;
  GtkToggleButton *toggle_button;
  GtkToggleButton *tmp_button = NULL;
  GSList *l;
  HNEntryInfo *info;
  
  g_return_val_if_fail (widget && event, FALSE);
  
  app_button = HN_APP_BUTTON (widget);
  priv = app_button->priv;
  
  button = GTK_BUTTON (widget);
  toggle_button = GTK_TOGGLE_BUTTON (widget);

  /* remember which button was used to press this button */
  HN_DBG("App button pressed using button %d", event->button);

  hn_wm_activate(HN_TN_DEACTIVATE_KEY_FOCUS);

  if (event->button == APP_BUTTON_THUMBABLE)
    priv->is_thumbable = TRUE;
  else
    priv->is_thumbable = FALSE;
  
  HN_DBG ("choosing");

  /* search for the toggled button, so that we can re-toggle
   * it in case the user didn't top the window/application
   */
  priv->prev_button = NULL;
  for (l = app_button->group; l != NULL; l = l->next)
    {
      tmp_button = l->data;

      if (tmp_button->active && (tmp_button != toggle_button))
        {
          priv->prev_button = tmp_button;
          break;
        }
    }

  gtk_toggle_button_set_inconsistent (toggle_button, FALSE);
  gtk_toggle_button_set_active (toggle_button, TRUE);
  gtk_toggle_button_toggled (toggle_button);
  
  info = priv->info;
  if (!info)
    {
      g_warning ("No entry info bound to this button!");
      
      return FALSE;
    }

  if (!priv->tooltip)
    priv->tooltip = hn_app_tooltip_new (GTK_WIDGET (app_button));

  hn_app_tooltip_set_text (HN_APP_TOOLTIP(app_button->priv->tooltip),
                           hn_entry_info_peek_app_name (info));

  hn_app_tooltip_install_timer (HN_APP_TOOLTIP(app_button->priv->tooltip),
                                NULL,
                                NULL,
                                NULL);
  
  return FALSE;
}

static void
hn_app_button_set_property (GObject      *gobject,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  HNAppButton *app_button = HN_APP_BUTTON (gobject);

  switch (prop_id)
    {
    case BUTTON_PROP_ENTRY_INFO:
      hn_app_button_set_entry_info (app_button,
		                    g_value_get_pointer (value));
      break;
    case BUTTON_PROP_IS_BLINKING:
      hn_app_button_set_is_blinking (app_button,
		      		     g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
hn_app_button_get_property (GObject    *gobject,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{

  HNAppButton *app_button = HN_APP_BUTTON (gobject);

  switch (prop_id)
    {
    case BUTTON_PROP_ENTRY_INFO:
      g_value_set_pointer (value, app_button->priv->info);
      break;
    case BUTTON_PROP_IS_BLINKING:
      g_value_set_boolean (value, app_button->priv->is_blinking);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
hn_app_button_class_init (HNAppButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  gobject_class->finalize = hn_app_button_finalize;  
  gobject_class->set_property = hn_app_button_set_property;
  gobject_class->get_property = hn_app_button_get_property;

  object_class->destroy = hn_app_button_destroy;
  
#if 0
  /* TODO - uncomment to handle keyboard focus */
  widget_class->focus = hn_app_button_focus;
#endif
  widget_class->button_press_event = hn_app_button_press_event;
  widget_class->button_release_event = hn_app_button_release_event;
  widget_class->key_press_event = hn_app_button_key_press_event;
  
  button_class->clicked = hn_app_button_clicked;
  
  g_object_class_install_property (gobject_class,
		  		   BUTTON_PROP_ENTRY_INFO,
				   g_param_spec_pointer ("entry-info",
					   		 "Entry Info",
							 "The informations about the entry bound to the button",
							 G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
		  		   BUTTON_PROP_IS_BLINKING,
				   g_param_spec_boolean ("is-blinking",
					   		 "Is Blinking",
							 "Whether the button should be blinking or not",
							 FALSE,
							 G_PARAM_READWRITE));

  g_type_class_add_private (gobject_class, sizeof (HNAppButtonPrivate));
}

static void
hn_app_button_init (HNAppButton *app_button)
{
  HNAppButtonPrivate *priv = HN_APP_BUTTON_GET_PRIVATE (app_button);

  app_button->group = g_slist_prepend (NULL, app_button);
  app_button->priv = priv;

  gtk_widget_push_composite_child ();

  priv->icon = gtk_image_new ();
  gtk_widget_set_composite_name (priv->icon, "as-app-button-icon");
  gtk_container_add (GTK_CONTAINER (app_button), priv->icon);
  gtk_widget_show (priv->icon);

  gtk_widget_pop_composite_child ();

  priv->info = NULL;
  
  priv->is_blinking = FALSE;
  priv->is_thumbable = FALSE;

  gtk_widget_set_size_request (GTK_WIDGET (app_button), -1, BUTTON_HEIGHT);
  gtk_widget_set_sensitive (GTK_WIDGET (app_button), FALSE);
}

/* taken from gtkradiobutton.c */
void
hn_app_button_set_group (HNAppButton *app_button,
			 GSList      *group)
{
  g_return_if_fail (HN_IS_APP_BUTTON (app_button));

  if (g_slist_find (group, app_button))
    return;

  HN_DBG ("setting group for the button");

  if (app_button->group)
    {
      GSList *l;

      app_button->group = g_slist_remove (app_button->group, app_button);

      for (l = app_button->group; l != NULL; l = l->next)
        {
          HNAppButton *tmp_button = l->data;

	  tmp_button->group = app_button->group;
        }
    }

  app_button->group = g_slist_prepend (group, app_button);

  if (group)
    {
      GSList *l;

      for (l = group; l != NULL; l = l->next)
        {
          HNAppButton *tmp_button = l->data;

	  if (!tmp_button)
	    continue;

	  tmp_button->group = app_button->group;
        }
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (app_button), group == NULL);
}

GSList *
hn_app_button_get_group (HNAppButton *button)
{
  g_return_val_if_fail (HN_IS_APP_BUTTON (button), NULL);

  return button->group;
}

GtkWidget *
hn_app_button_new (GSList *group)
{
  HNAppButton *app_button;

  app_button = g_object_new (HN_TYPE_APP_BUTTON, NULL);

  if (group)
    hn_app_button_set_group (app_button, group);

  return GTK_WIDGET (app_button);
}

void
hn_app_button_set_icon_from_pixbuf (HNAppButton *button,
				    GdkPixbuf   *pixbuf)
{
  g_return_if_fail (HN_IS_APP_BUTTON (button));
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

  gtk_image_set_from_pixbuf (GTK_IMAGE (button->priv->icon), pixbuf);
}

GdkPixbuf *
hn_app_button_get_pixbuf_from_icon (HNAppButton *button)
{
  g_return_val_if_fail (HN_IS_APP_BUTTON (button), NULL);

  return gtk_image_get_pixbuf (GTK_IMAGE (button->priv->icon));
}

HNEntryInfo *
hn_app_button_get_entry_info (HNAppButton *button)
{
  g_return_val_if_fail (HN_IS_APP_BUTTON (button), NULL);

  return button->priv->info;
}

static GdkPixbuf *
get_pixbuf_for_entry_info (HNEntryInfo *info)
{
  GdkPixbuf *retval;
  GError *error;

  error = NULL;
  retval = hn_entry_info_get_app_icon (info, AS_ICON_SIZE, &error);
  if (!error)
    return retval;

  osso_log (LOG_ERR, "Could not load icon '%s': %s\n",
            hn_entry_info_get_app_icon_name (info),
            error->message);

  /* Fallback to default icon */
  g_clear_error (&error);
  retval = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
		  		     AS_MENU_DEFAULT_APP_ICON,
				     AS_ICON_SIZE,
				     0,
				     &error);
  if (!error)
    return retval;

  osso_log (LOG_ERR, "Could not load icon '%s': %s\n",
            AS_MENU_DEFAULT_APP_ICON,
            error->message);

  g_error_free (error);

  return NULL;
}

static GdkPixbuf *
compose_app_pixbuf (const GdkPixbuf *src,
		    HNEntryInfo     *info)
{
  GdkPixbuf *retval, *inst_pixbuf;
  const gchar *inst_name;
  GError *error;
  gint dest_width, dest_height;
  gint off_x, off_y;

  g_return_val_if_fail (GDK_IS_PIXBUF (src), NULL);
  g_return_val_if_fail (info != NULL, NULL);

  HN_MARK();

  /* first of all, see if this app is hibernated */
  if (hn_entry_info_is_hibernating (info))
    {
      inst_name = "qgn_indi_bkilled";
    }
  else if (hn_entry_info_has_extra_icon (info))
    {
      inst_name = hn_entry_info_get_extra_icon (info);
    }
  else
    {
      gint n_instances = hn_entry_info_get_n_children (info);
      
      if (!n_instances)
        {
          g_warning ("top-level item '%s' has no instances",
                     hn_entry_info_peek_title (info));
          return NULL;
        }

      if (n_instances == 1)
        return NULL;

      if (G_LIKELY (n_instances <= app_group_n_icons - 1))
        inst_name = app_group_icons[n_instances - 1];
      else
        inst_name = APP_GROUP_ICON_MORE;
    }
  
  HN_DBG ("Compositing icon for '%s' with icon name '%s'",
	  hn_entry_info_peek_title (info),
	  inst_name);

  error = NULL;
  inst_pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
		  			  inst_name,
					  APP_GROUP_ICON_SIZE,
					  0,
					  &error);
  if (error)
    {
      g_warning ("unable to find icon '%s' in current theme: %s",
		 inst_name,
		 error->message);

      g_error_free (error);

      return NULL;
    }

  /* make a copy of the source pixbuf, and also make
   * sure that it has an alpha channel
   */
  retval = gdk_pixbuf_add_alpha (src, FALSE, 255, 255, 255);

  dest_width = gdk_pixbuf_get_width (retval);
  dest_height = gdk_pixbuf_get_height (retval);

  off_x = dest_width - gdk_pixbuf_get_width (inst_pixbuf);
  off_y = dest_height - gdk_pixbuf_get_height (inst_pixbuf);

  gdk_pixbuf_composite (inst_pixbuf, retval,
		        0, 0,
			dest_width, dest_height,
			off_x, off_y,
			1.0, 1.0,
			GDK_INTERP_BILINEAR,
			0xFF);

  g_object_unref (inst_pixbuf);

  return retval;
}

void
hn_app_button_set_entry_info (HNAppButton *button,
			      HNEntryInfo *info)
{
  g_return_if_fail (HN_IS_APP_BUTTON (button));

  button->priv->info = NULL;

  HN_MARK();

  if (info)
    {
      GdkPixbuf *app_pixbuf;
      
      app_pixbuf = get_pixbuf_for_entry_info (info);
      if (app_pixbuf)
	{
          GdkPixbuf *pixbuf;

	  /* compose the application icon with the number of
	   * instances running
	   */
	  pixbuf = compose_app_pixbuf (app_pixbuf, info);
	  if (pixbuf)
            {
              gtk_image_set_from_pixbuf (GTK_IMAGE (button->priv->icon),
			                 pixbuf);
	      g_object_unref (pixbuf);
	    }
	  else
            gtk_image_set_from_pixbuf (GTK_IMAGE (button->priv->icon),
			               app_pixbuf);

	  g_object_unref (app_pixbuf);
	}
      else
	HN_DBG ("Unable to find the icon (even the default one)");

      /* the newly composed image is static */
      if(button->priv->is_blinking &&
         !hn_app_switcher_get_system_inactivity(hn_wm_get_app_switcher()))
        hn_app_button_icon_animation (button->priv->icon,
                                      button->priv->is_blinking);
      
      gtk_widget_show (button->priv->icon);
      gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
		                    hn_entry_info_is_active (info));
      
      g_object_set (G_OBJECT (button), "can-focus", TRUE, NULL);
      
      button->priv->info = info;
    }
  else
    {
      gtk_widget_hide (button->priv->icon);
      gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON (button));

      g_object_set (G_OBJECT (button), "can-focus", FALSE, NULL);
    }
}

void
hn_app_button_force_update_icon (HNAppButton *button)
{
  HNEntryInfo *info;
  
  g_return_if_fail (HN_IS_APP_BUTTON (button));

  HN_MARK ();

  info = button->priv->info;

  if (info)
    {
      GdkPixbuf *app_pixbuf;
      
      app_pixbuf = get_pixbuf_for_entry_info (info);
      if (app_pixbuf)
	{
          GdkPixbuf *pixbuf;

	  /* compose the application icon with the number of
	   * instances running
	   */
	  pixbuf = compose_app_pixbuf (app_pixbuf, info);
	  if (pixbuf)
            {
              gtk_image_set_from_pixbuf (GTK_IMAGE (button->priv->icon),
			                 pixbuf);
	      g_object_unref (pixbuf);
	    }
	  else
            gtk_image_set_from_pixbuf (GTK_IMAGE (button->priv->icon),
			               app_pixbuf);

	  g_object_unref (app_pixbuf);
	}
      else
	HN_DBG ("Unable to find the icon (even the default one)");

      /* the newly composed image is static */
      if(button->priv->is_blinking &&
         !hn_app_switcher_get_system_inactivity(hn_wm_get_app_switcher()))
        hn_app_button_icon_animation (button->priv->icon,
                                      button->priv->is_blinking);
      
      gtk_widget_show (button->priv->icon);
      gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
		                    hn_entry_info_is_active (info));
      
      g_object_set (G_OBJECT (button), "can-focus", TRUE, NULL);
      
      button->priv->info = info;
    }
}

static void
hn_app_button_icon_animation (GtkWidget *icon,
			      gboolean   turn_on)
{
  GdkPixbuf *pixbuf;
  GdkPixbufAnimation *pixbuf_anim;

  g_return_if_fail (GTK_IS_IMAGE (icon));

  if (turn_on &&
      gtk_image_get_storage_type(GTK_IMAGE(icon)) != GTK_IMAGE_ANIMATION)
    {
      pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (icon));
      pixbuf_anim = hildon_pixbuf_anim_blinker_new (pixbuf,
		     				    1000 / ANIM_FPS,
						    -1,
						    10);
     
      gtk_image_set_from_animation (GTK_IMAGE(icon), pixbuf_anim);
      g_object_unref (pixbuf_anim);
    }
  else if(!turn_on &&
          gtk_image_get_storage_type(GTK_IMAGE(icon)) == GTK_IMAGE_ANIMATION)
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

gboolean
hn_app_button_get_is_blinking (HNAppButton *button)
{
  g_return_val_if_fail (HN_IS_APP_BUTTON (button), FALSE);

  return button->priv->is_blinking;
}

void
hn_app_button_set_is_blinking (HNAppButton *button,
			       gboolean     is_blinking)
{
  HN_DBG("Setting is_blinking %d -> %d",
         button->priv->is_blinking,
         is_blinking);
  
  g_return_if_fail (HN_IS_APP_BUTTON (button));

  if (button->priv->is_blinking != is_blinking)
    {
      hn_app_button_icon_animation (button->priv->icon,
		      		    is_blinking);

      button->priv->is_blinking = is_blinking;
    }
}
