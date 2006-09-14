/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/* hn-app-tooltip.c
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
#include "hn-app-tooltip.h"
#include "hn-wm.h"
#include "hn-entry-info.h"
#include "hn-wm-watchable-app.h"

#include <stdlib.h>
#include <string.h>

/* GLib include */
#include <glib.h>
#include <glib/gi18n.h>

/* GTK includes */
#include <gtk/gtkwidget.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmisc.h>
#include <gtk/gtkwindow.h>

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

#define TOOLTIP_NAME "hildon-task-navigator-tooltip"

#define TOOLTIP_WIDTH 360
#define TOOLTIP_BORDER_WIDTH 20

#define TOOLTIP_SHOW_TIMEOUT 500
#define TOOLTIP_HIDE_TIMEOUT 1500


/*
 * HNAppTooltip
 */

enum
{
  TIP_PROP_0,
  
  TIP_PROP_WIDGET,
  TIP_PROP_TEXT
};

#define HN_APP_TOOLTIP_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), HN_TYPE_APP_TOOLTIP, HNAppTooltipClass))
#define HN_IS_APP_TOOLTIP_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), HN_TYPE_APP_TOOLTIP))
#define HN_APP_TOOLTIP_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), HN_TYPE_APP_TOOLTIP, HNAppTooltipClass))

typedef struct _HNAppTooltipClass HNAppTooltipClass;

struct _HNAppTooltip
{
  GtkWindow parent_instance;

  GtkWidget *widget;
  GtkWidget *label;

  guint show_timer_id;
  guint hide_timer_id;
  
  gchar *text;

  GtkCallback show_cb;
  GtkCallback hide_cb;
  gpointer cb_data;
};

struct _HNAppTooltipClass
{
  GtkWindowClass parent_class;
};

G_DEFINE_TYPE (HNAppTooltip, hn_app_tooltip, GTK_TYPE_WINDOW);

static void
hn_app_tooltip_finalize (GObject *gobject)
{
  HNAppTooltip *tip = HN_APP_TOOLTIP (gobject);

  hn_app_tooltip_remove_timers (tip);

  if (tip->text)
    g_free (tip->text);

  G_OBJECT_CLASS (hn_app_tooltip_parent_class)->finalize (gobject);
}

static void
hn_app_tooltip_set_property (GObject      *gobject,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  HNAppTooltip *tip = HN_APP_TOOLTIP (gobject);
  GtkWidget *widget = GTK_WIDGET (gobject);

  switch (prop_id)
    {
    case TIP_PROP_WIDGET:
      {
      gboolean was_visible = GTK_WIDGET_VISIBLE (widget);
      
      gtk_widget_hide (widget);
      
      tip->widget = g_value_get_object (value);
      
      if (was_visible)
        gtk_widget_show (widget);
      }
      break;
    case TIP_PROP_TEXT:
      g_free (tip->text);
      tip->text = g_strdup (g_value_get_string (value));
      gtk_label_set_text (GTK_LABEL (tip->label), tip->text);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
hn_app_tooltip_get_property (GObject    *gobject,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
  switch (prop_id)
    {
    case TIP_PROP_WIDGET:
      break;
    case TIP_PROP_TEXT:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
hn_app_tooltip_position (HNAppTooltip *tip)
{
  gint x, y;

  gdk_flush ();

  gtk_widget_realize (tip->widget);
  gdk_window_get_origin (tip->widget->window, &x, &y);

  x += tip->widget->allocation.width + 1;
  y += tip->widget->allocation.y;
  
  gtk_window_move (GTK_WINDOW (tip), x, y);
}

static void
hn_app_tooltip_show (GtkWidget *widget)
{
  hn_app_tooltip_position (HN_APP_TOOLTIP (widget));

  /* chain up */
  GTK_WIDGET_CLASS (hn_app_tooltip_parent_class)->show (widget);
}

static void
hn_app_tooltip_class_init (HNAppTooltipClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->finalize = hn_app_tooltip_finalize;
  gobject_class->set_property = hn_app_tooltip_set_property;
  gobject_class->get_property = hn_app_tooltip_get_property;

  widget_class->show = hn_app_tooltip_show;

  g_object_class_install_property (gobject_class,
		  		   TIP_PROP_WIDGET,
				   g_param_spec_object ("widget",
					   		"Widget",
							"The widget to which we align to",
							GTK_TYPE_WIDGET,
							(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  g_object_class_install_property (gobject_class,
		  		   TIP_PROP_TEXT,
				   g_param_spec_string ("text",
					   		"Text",
							"The text of the tooltip",
							NULL,
							(G_PARAM_READABLE | G_PARAM_WRITABLE)));
}

static void
hn_app_tooltip_init (HNAppTooltip *tip)
{
  GtkWindow *window = GTK_WINDOW (tip);

  tip->widget = NULL;
  tip->text = g_strdup ("Invalid text");
  
  gtk_window_set_resizable (window, FALSE);
  gtk_window_set_decorated (window, FALSE);
  gtk_window_set_type_hint (window,
		  	    GDK_WINDOW_TYPE_HINT_DIALOG);
  
  gtk_widget_set_size_request (GTK_WIDGET (tip), TOOLTIP_WIDTH, -1);
  gtk_widget_set_name (GTK_WIDGET (tip), TOOLTIP_NAME);
  gtk_container_set_border_width (GTK_CONTAINER (tip), TOOLTIP_BORDER_WIDTH);

  tip->label = gtk_label_new (tip->text);
  gtk_label_set_line_wrap (GTK_LABEL (tip->label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (tip->label), 0.5, 0.5);
  gtk_container_add (GTK_CONTAINER (tip), tip->label);
  gtk_widget_show (tip->label);
}

GtkWidget *
hn_app_tooltip_new (GtkWidget *widget)
{
  return g_object_new (HN_TYPE_APP_TOOLTIP,
		       "widget", widget,
		       NULL);
}

void
hn_app_tooltip_set_widget (HNAppTooltip *tip,
			   GtkWidget    *widget)
{
  gboolean was_visible;
  
  g_return_if_fail (HN_IS_APP_TOOLTIP (tip));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  was_visible = GTK_WIDGET_VISIBLE (GTK_WIDGET (tip));
  gtk_widget_hide (GTK_WIDGET (tip));

  tip->widget = widget;
  
  if (was_visible)
    gtk_widget_show (GTK_WIDGET (tip));

  g_object_notify (G_OBJECT (tip), "widget");
}

GtkWidget *
hn_app_tooltip_get_widget (HNAppTooltip *tip)
{
  g_return_val_if_fail (HN_IS_APP_TOOLTIP (tip), NULL);

  return tip->widget;
}

void
hn_app_tooltip_set_text (HNAppTooltip *tip,
                         const gchar  *text)
{
  g_return_if_fail (HN_IS_APP_TOOLTIP (tip));

  g_free (tip->text);
  tip->text = g_strdup (text);
  
  gtk_label_set_text (GTK_LABEL (tip->label), tip->text);

  g_object_notify (G_OBJECT (tip), "text");
}

const gchar *
hn_app_tooltip_get_text (HNAppTooltip *tip)
{
  g_return_val_if_fail (HN_IS_APP_TOOLTIP (tip), NULL);

  return tip->text;
}

static gboolean
hn_app_tooltip_hide_timeout_cb (gpointer data)
{
  HNAppTooltip *tip = HN_APP_TOOLTIP (data);

  gtk_widget_hide (GTK_WIDGET (tip));

  if (tip->hide_cb)
    tip->hide_cb (GTK_WIDGET (tip), tip->cb_data);

  return FALSE;
}

static gboolean
hn_app_tooltip_show_timeout_cb (gpointer data)
{
  HNAppTooltip *tip = HN_APP_TOOLTIP (data);

  HN_DBG ("Showing tooltip");
  
  gtk_widget_show (GTK_WIDGET (tip));

  if (tip->show_cb)
    tip->show_cb (GTK_WIDGET (tip), tip->cb_data);

  HN_DBG ("Installing tooltip hide timer");
  
  tip->show_timer_id = 0;
  tip->hide_timer_id = g_timeout_add (TOOLTIP_HIDE_TIMEOUT,
		  		      hn_app_tooltip_hide_timeout_cb,
				      tip);				      

  return FALSE;
}

void
hn_app_tooltip_install_timer (HNAppTooltip *tip,
			      GtkCallback   show_callback,
			      GtkCallback   hide_callback,
			      gpointer      callback_data)
{
  g_return_if_fail (HN_IS_APP_TOOLTIP (tip));

  if (tip->show_timer_id)
    return;

  tip->show_cb = show_callback;
  tip->hide_cb = hide_callback;
  tip->cb_data = callback_data;

  HN_DBG ("Installing tooltip show timer");

  tip->show_timer_id = g_timeout_add (TOOLTIP_SHOW_TIMEOUT,
		  		      hn_app_tooltip_show_timeout_cb,
				      tip);
}

void
hn_app_tooltip_remove_show_timer(HNAppTooltip *tip)
{
  g_return_if_fail (HN_IS_APP_TOOLTIP (tip));

  if (tip->show_timer_id)
    {
      HN_DBG ("Removing tooltip show timer");

      g_source_remove (tip->show_timer_id);
      tip->show_timer_id = 0;
    }
}


void
hn_app_tooltip_remove_timers (HNAppTooltip *tip)
{
  g_return_if_fail (HN_IS_APP_TOOLTIP (tip));

  if (tip->show_timer_id)
    {
      HN_DBG ("Removing tooltip show timer");

      g_source_remove (tip->show_timer_id);
      tip->show_timer_id = 0;
    }

  if (tip->hide_timer_id)
    {
      HN_DBG ("Removing tooltip hide timer");

      gtk_widget_hide(GTK_WIDGET(tip));
      
      g_source_remove (tip->hide_timer_id);
      tip->hide_timer_id = 0;
    }
}
