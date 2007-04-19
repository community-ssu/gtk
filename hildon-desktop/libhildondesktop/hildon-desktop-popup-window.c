/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
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

#include "hildon-desktop-popup-window.h" 

#define HILDON_DESKTOP_POPUP_WINDOW_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_POPUP_WINDOW, HildonDesktopPopupWindowPrivate))

G_DEFINE_TYPE (HildonDesktopPopupWindow, hildon_desktop_popup_window, GTK_TYPE_WINDOW);

enum
{
  PROP_POPUP_N_PANES=1,
  PROP_POPUP_ORIENTATION,
  PROP_POPUP_DIRECTION
};

enum
{
  POPUP_N_SIGNALS;
};

static GObject *hildon_desktop_popup_window_constructor (GType gtype, 
                                                         guint n_params, 
                                                         GObjectConstructParam *params);

static void hildon_desktop_popup_window_get_property (GObject *object, 
                                                      guint prop_id, 
                                                      GValue *value, 
                                                      GParamSpec *pspec);		   

static void hildon_desktop_popup_window_set_property (GObject *object, 
                                                      guint prop_id, 
                                                      const GValue *value, 
                                                      GParamSpec *pspec);

static void hildon_desktop_popup_window_realize (GtkWidget *widget);
static void hildon_desktop_popup_window_unrealize (GtkWidget *widget);
static void hildon_desktop_popup_window_show (GtkWidget *widget);
static void hildon_desktop_popup_window_hide (GtkWidget *widget);
static void hildon_desktop_popup_window_show_all (GtkWidget *widget);
static void hildon_desktop_popup_window_hide_all (GtkWidget *widget);

static gboolean hildon_desktop_popup_window_focus (GtkWidget *widget);

static gboolean hildon_desktop_popup_window_visibility_notify (GtkWidget          *widget,
				                               GdkEventVisibility *event,
			        		               gpointer            data);

struct _HildonDesktopPopupWindow 
{
  GtkWidget	   	          **extra_panes;
  guint				    n_extra_panes;
  GtkOrientation 		    orientation;
  HildonDesktopPopupWindowDirection direction;
}

static void 
hildon_desktop_popup_window_init (GObject *object)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (object);

  popup->priv = HILDON_DESKTOP_POPUP_WINDOW_GET_PRIVATE (object);

  popup->priv->extra_panes   = NULL;
  popup->priv->n_extra_panes = 0;


}

static void 
hildon_desktop_popup_window_class_init (HildonDesktopPopupWindow *popup_class)
{
  GObjectClass *object_class   = G_OBJECT_CLASS (popup_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (popup_class);

  object_class->constructor  = hildon_desktop_popup_window_constructor;
  object_class->set_property = hildon_desktop_popup_window_set_property;
  object_class->get_property = hildon_desktop_popup_window_get_property;

  widget_class->enter_notify_event      = hildon_desktop_popup_window_enter_notify;
  widget_class->leave_notify_event      = hildon_desktop_popup_window_leave_notify;

  widget_class->focus = hildon_desktop_popup_window_focus;
	  
  widget_class->realize    = hildon_desktop_popup_window_realize;
  widget_class->unrealize  = hildon_desktop_popup_window_unrealize;
  widget_class->show       = hildon_desktop_popup_show;
  widget_class->hide       = hildon_desktop_popup_hide;
  widget_class->show_all   = hildon_desktop_popup_show_all;
  widget_class->hide_all   = hildon_desktop_popup_hide_all;
  
  g_type_class_add_private (g_object_class, sizeof (HildonDesktopPopupWindowPrivate));

  g_object_class_install_property (object_class,
                                   PROP_POPUP_N_PANES,
                                   g_param_spec_uint(
                                           "n-panes",
                                           "npanes",
                                           "Number of extra panes",
                                            0,
                                            20,
                                            0,
                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_POPUP_ORIENTATION,
                                   g_param_spec_int(
                                           "orientation",
                                           "orientation",
                                           "Stack panels horizontally or vertically",
                                            GTK_ORIENTATION_HORIZONTAL,
                                            GTK_ORIENTATION_VERTICAL,
                                            GTK_ORIENTATION_HORIZONTAL,
                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_POPUP_DIRECTION,
                                   g_param_spec_int(
                                           "direction",
                                           "orientation",
                                           "Stack panels to left/top or right/bottom",
                                            HD_POPUP_WINDOW_DIRECTION_LEFT_TOP,
                                            HD_POPUP_WINDOW_DIRECTION_RIGHT_BOTTOM,
                                            HD_POPUP_WINDOW_DIRECTION_RIGHT_BOTTOM,
                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static GObject *
hildon_desktop_popup_window_constructor (GType gtype,
					 guint n_params,
					 GObjectConstructParam *params)
{
  GObject *object;
  HildonDesktopPopupWindow *popup;
  gint i;

  object = G_OBJECT_CLASS (hildon_desktop_popup_window_parent_class)->constructor (gtype,
                                                                                   n_params,
                                                                                   params);

  popup = HILDON_DESKTOP_POPUP_WINDOW (object);

  GTK_WINDOW (popup)->type = GTK_WINDOW_POPUP;

  gtk_window_set_type_hint (GTK_WINDOW (popup), GDK_WINDOW_TYPE_HINT_POPUP);

  g_signal_connect (popup,
		    "visibility_notify_envent",
		    G_CALLBACK (hildon_desktop_popup_window_visibility_notify),
		    NULL);

  gtk_widget_push_composite_child ();

  popup->priv->extra_panes = g_new0 (GtkWindow *, popup->priv->n_extra_panes);

  for (i=0; i < popup->priv->n_extra_panes; i++)
  {	  
    popup->priv->extra_panes[i] = gtk_window_new (GTK_WINDOW_POPUP);

    g_signal_connect (popup->priv->extra_panes[i],
		      "enter-notify-event",
		      G_CALLBACK (hildon_desktop_popup_window_composited_enter_notify),
		      (gpointer)popup);

    g_signal_connect (popup->priv->extra_panes[i],
		      "leave-notify-event",
		      G_CALLBACK (hildon_desktop_popup_window_composited_leave_notify),
		      (gpointer)popup);

    /*FIXME: NO FOCUS FOR ANY WINDOW!!!!!!!!!!!!!!!!!! */
  }

  gtk_widget_pop_composite_child ();

  return object;
}

/* At some point I will use macros :) */

static void 
hildon_desktop_popup_window_realize (GtkWidget *widget)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (widget);
  register gint i;
  
  GTK_WIDGET_CLASS (hildon_desktop_popup_window_parent_class)->realize (widget);

  for (i=0; i < popup->priv->n_extra_panes; i++)
    gtk_widget_realize (popup->priv->extra_panes[i]);
}	

static void 
hildon_desktop_popup_window_unrealize (GtkWidget *widget)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (widget);
  register gint i;
  
  GTK_WIDGET_CLASS (hildon_desktop_popup_window_parent_class)->unrealize (widget);

  for (i=0; i < popup->priv->n_extra_panes; i++)
    gtk_widget_unrealize (popup->priv->extra_panes[i]);
}

static void 
hildon_desktop_popup_window_show (GtkWidget *widget)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (widget);
  register gint i;
  
  GTK_WIDGET_CLASS (hildon_desktop_popup_window_parent_class)->show (widget);

  for (i=0; i < popup->priv->n_extra_panes; i++)
    gtk_widget_show (popup->priv->extra_panes[i]);
}

static void 
hildon_desktop_popup_window_hide (GtkWidget *widget)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (widget);
  register gint i;
  
  GTK_WIDGET_CLASS (hildon_desktop_popup_window_parent_class)->hide (widget);

  for (i=0; i < popup->priv->n_extra_panes; i++)
    gtk_widget_hide (popup->priv->extra_panes[i]);
}

static void 
hildon_desktop_popup_window_show_all (GtkWidget *widget)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (widget);
  register gint i;
  
  GTK_WIDGET_CLASS (hildon_desktop_popup_window_parent_class)->show_all (widget);

  for (i=0; i < popup->priv->n_extra_panes; i++)
    gtk_widget_show_all (popup->priv->extra_panes[i]);
}

static void 
hildon_desktop_popup_window_hide_all (GtkWidget *widget)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (widget);
  register gint i;
  
  GTK_WIDGET_CLASS (hildon_desktop_popup_window_parent_class)->hide_all (widget);

  for (i=0; i < popup->priv->n_extra_panes; i++)
    gtk_widget_hide_all (popup->priv->extra_panes[i]);
}

static gboolean 
hildon_desktop_popup_window_focus (GtkWidget *widget)
{
  /* What focus? */
  return FALSE;
}

static gboolean
hildon_desktop_popup_window_enter_notify (GtkWidget        *widget,
		                          GdkEventCrossing *event)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (widget);

  /* We have to grab the pointer in here, we should get this when we come from
   * a composited window
   */ 
  
  return TRUE;	
}

static gboolean 
hildon_desktop_popup_window_leave_notify (GtkWidget        *widget,
					  GdkEventCrossing *event)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (widget);

 /* We have to ungrab the pointer in here, we should get this when we go to
  * a composited window
  */
  
  return TRUE;	
}

static gboolean 
hildon_desktop_popup_window_composited_enter_notify (GtkWidget *widget,
						     GdkEventCrossing *event,
						     HildonDesktopPopupWindow *popup)
{


  return TRUE;
}

static gboolean 
hildon_desktop_popup_window_composited_leave_notify (GtkWidget *widget,
						     GdkEventCrossing *event,
						     HildonDesktopPopupWindow *popup)
{


  return TRUE;
}

static gboolean
hildon_desktop_popup_window_visibility_notify (GtkWidget          *widget,
		                               GdkEventVisibility *event,
			                       gpointer            data)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (widget);

  /* We have to close every window when called this but also we have to 
   * track if our composited windows are not the responsibles for the 
   * visibility-notify
   */
  
  return TRUE;	
}

static void 
hildon_desktop_popup_window_get_property (GObject *object, 
                                          guint prop_id, 
                                          GValue *value, 
                                          GParamSpec *pspec)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (object);

  switch (prop_id)
  {
    case PROP_POPUP_N_PANES:
      g_value_set_uint (value, popup->priv->n_extra_panes);
      break;

   case PROP_POPUP_ORIENTATION:
      g_value_set_int (value, popup->priv->orientation);
      break;

   case PROP_POPUP_DIRECTION:
      g_value_set_int (value, popup->priv->direction);
      break;
      
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

}

static void 
hildon_desktop_popup_window_set_property (GObject *object, 
                                          guint prop_id, 
                                          const GValue *value, 
                                          GParamSpec *pspec)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (object);

  switch (prop_id)
  {
    case PROP_POPUP_N_PANES:
      popup->priv->n_extra_panes = g_value_get_uint (value);
      break;

    case PROP_POPUP_ORIENTATION:
      popup->priv->orientation = g_value_get_int (value);
      break;

    case PROP_POPUP_DIRECTION:
      popup->priv->direction = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

GtkWidget *
hildon_desktop_popup_window (guint n_panes,
			     GtkOrientation orientation,
			     HildonDesktopPopupWindowDirection direction)
{
  return GTK_WIDGET (g_object_new (HILDON_DESKTOP_TYPE_POPUP_WINDOW,
			  	   "n-panes",n_panes,
				   "orientation",orientation,
				   "direction",direction,
				   NULL));
}

GtkWidget *
hildon_desktop_popup_window_get_pane (HildonDesktopPopupWindow *popup, gint pane)
{
  g_assert (HILDON_DESKTOP_IS_POPUP_WINDOW (popup));

  if (pane <= -1)
    return GTK_WIDGET (popup);	 
  else
  if (pane >= popup->priv->n_extra_panes)
    return popup->priv->extra_panes [popup->priv->n_extra_panes-1];
  else
    return popup->priv->extra_panes [pane];
}	

void 
hildon_desktop_popup_window_popup (HildonDesktopPopupWindow *popup,
				   GtkMenuPositionFunc func)
{


}
