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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkwindow.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef MAEMO_CHANGES
#ifdef HAVE_XTEST
#include <X11/extensions/XTest.h>
#endif
#endif

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
  SIGNAL_POPUP_SHOW,
  SIGNAL_POPUP_CLOSE,
  POPUP_N_SIGNALS
};

static gint signals[POPUP_N_SIGNALS];

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

static void hildon_desktop_popup_finalize (GObject *object);
static void hildon_desktop_popup_window_realize (GtkWidget *widget);
static void hildon_desktop_popup_window_unrealize (GtkWidget *widget);
static void hildon_desktop_popup_window_show (GtkWidget *widget);
static void hildon_desktop_popup_window_hide (GtkWidget *widget);
static void hildon_desktop_popup_window_show_all (GtkWidget *widget);
static void hildon_desktop_popup_window_hide_all (GtkWidget *widget);

static gboolean hildon_desktop_popup_window_motion_notify (GtkWidget *widget, GdkEventMotion *event);
static gboolean hildon_desktop_popup_window_leave_notify (GtkWidget *widget, GdkEventCrossing *event);
#ifdef MAEMO_CHANGES
static gboolean hildon_desktop_popup_window_visibility_notify (GtkWidget          *widget,
				                               GdkEventVisibility *event,
			        		               gpointer            data);
static gboolean hildon_desktop_popup_window_delete_event (GtkWidget *widget,
							  GdkEvent  *event,
							  gpointer   data);
#endif
static gboolean hildon_desktop_popup_window_composited_leave_notify (GtkWidget *widget,
						     		     GdkEventCrossing *event,
						     		     HildonDesktopPopupWindow *popup);

static gboolean hildon_desktop_popup_window_composited_button_release (GtkWidget *widget,
		                                                       GdkEventButton *event,
	                                                               HildonDesktopPopupWindow *popup);

static gboolean popup_grab_on_window (GdkWindow *window, guint32 activate_time, gboolean grab_keyboard);

static gboolean hildon_desktop_popup_window_button_release_event (GtkWidget *widget, GdkEventButton *event);
#if defined (MAEMO_CHANGES) && defined(HAVE_XTEST)
static void hildon_desktop_popup_menu_fake_button_event (GdkEventButton *event, gboolean press);
#endif
struct _HildonDesktopPopupWindowPrivate 
{
  GtkWidget	   	          **extra_panes;
  guint				    n_extra_panes;
  GtkOrientation 		    orientation;
  HildonDesktopPopupWindowDirection direction;

  HDPopupWindowPositionFunc	    position_func;
  gpointer			    position_func_data;

  gboolean 			    have_xgrab;
  gboolean			    open;
  GtkWidget 			   *pane_with_grab;

  GtkWidget			   *attached_widget;
};

static void 
hildon_desktop_popup_window_init (HildonDesktopPopupWindow *popup)
{
  popup->priv = HILDON_DESKTOP_POPUP_WINDOW_GET_PRIVATE (popup);

  popup->priv->extra_panes   = NULL;
  popup->priv->n_extra_panes = 0;

  popup->priv->open = 
  popup->priv->have_xgrab = FALSE; 

  popup->priv->pane_with_grab  =
  popup->priv->attached_widget = NULL;
}

static gboolean 
hildon_desktop_popup_window_key_press_event (GtkWidget *widget, GdkEventKey *event)
{
  if (GTK_BIN (widget)->child)
    return gtk_widget_event (GTK_BIN (widget)->child,(GdkEvent *)event);	

  return
    GTK_WIDGET_CLASS 
      (hildon_desktop_popup_window_parent_class)->key_press_event (widget, event);	  
}  

static gboolean 
hildon_desktop_popup_window_key_press_event_cb (GtkWidget *widget, 
						GdkEventKey *event,
						HildonDesktopPopupWindow *popup)
{
  if (GTK_BIN (widget)->child)
    return gtk_widget_event (GTK_BIN (widget)->child,(GdkEvent *)event);

  return FALSE;
}

static void 
hildon_desktop_popup_window_class_init (HildonDesktopPopupWindowClass *popup_class)
{
  GObjectClass *object_class   = G_OBJECT_CLASS (popup_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (popup_class);

  object_class->constructor  = hildon_desktop_popup_window_constructor;
  object_class->set_property = hildon_desktop_popup_window_set_property;
  object_class->get_property = hildon_desktop_popup_window_get_property;
  object_class->finalize     = hildon_desktop_popup_finalize;

  widget_class->motion_notify_event     = hildon_desktop_popup_window_motion_notify;
  widget_class->leave_notify_event      = hildon_desktop_popup_window_leave_notify;
  widget_class->button_release_event    = hildon_desktop_popup_window_button_release_event;
  widget_class->key_press_event 	= hildon_desktop_popup_window_key_press_event;
	  
  widget_class->realize    = hildon_desktop_popup_window_realize;
  widget_class->unrealize  = hildon_desktop_popup_window_unrealize;
  widget_class->show       = hildon_desktop_popup_window_show;
  widget_class->hide       = hildon_desktop_popup_window_hide;
  widget_class->show_all   = hildon_desktop_popup_window_show_all;
  widget_class->hide_all   = hildon_desktop_popup_window_hide_all;
  
  g_type_class_add_private (object_class, sizeof (HildonDesktopPopupWindowPrivate));

  signals[SIGNAL_POPUP_SHOW] =
        g_signal_new ("popup-window",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (HildonDesktopPopupWindowClass,popup_window),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
 
  signals[SIGNAL_POPUP_SHOW] =
        g_signal_new ("popdown-window",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (HildonDesktopPopupWindowClass,popdown_window),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0); 

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
#ifndef MAEMO_CHANGES
  GTK_WINDOW (popup)->type = GTK_WINDOW_POPUP;
#else
  GTK_WINDOW (popup)->type = GTK_WINDOW_TOPLEVEL;

  gtk_window_set_is_temporary (GTK_WINDOW (popup), TRUE);

  gtk_window_set_decorated (GTK_WINDOW (popup), FALSE);
  gtk_widget_add_events (GTK_WIDGET (popup), GDK_VISIBILITY_NOTIFY_MASK);
#endif
	  
  gtk_window_set_type_hint (GTK_WINDOW (popup), GDK_WINDOW_TYPE_HINT_MENU);

#ifdef MAEMO_CHANGES
  g_signal_connect (popup,
		    "visibility-notify-event",
		    G_CALLBACK (hildon_desktop_popup_window_visibility_notify),
		    NULL);

  g_signal_connect (popup,
		    "delete-event",
		    G_CALLBACK (hildon_desktop_popup_window_delete_event),
		    NULL);
#endif
  gtk_widget_push_composite_child ();

  popup->priv->extra_panes = g_new0 (GtkWidget *, popup->priv->n_extra_panes);

  for (i=0; i < popup->priv->n_extra_panes; i++)
  {	  
#ifndef MAEMO_CHANGES	  
    popup->priv->extra_panes[i] = gtk_window_new (GTK_WINDOW_POPUP);
#else
    popup->priv->extra_panes[i] = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    gtk_window_set_decorated (GTK_WINDOW (popup->priv->extra_panes[i]), FALSE);

    gtk_widget_add_events (GTK_WIDGET (popup), GDK_VISIBILITY_NOTIFY_MASK);
#endif
    g_object_ref (G_OBJECT (popup->priv->extra_panes[i]));
    gtk_object_sink (GTK_OBJECT (popup->priv->extra_panes[i]));		 

    gtk_window_set_type_hint (GTK_WINDOW (popup->priv->extra_panes[i]),
		    	      GDK_WINDOW_TYPE_HINT_MENU);

    g_signal_connect (popup->priv->extra_panes[i],
		      "button-release-event",
		      G_CALLBACK (hildon_desktop_popup_window_composited_button_release),
		      (gpointer)popup); 
    
    g_signal_connect (popup->priv->extra_panes[i],
		      "leave-notify-event",
		      G_CALLBACK (hildon_desktop_popup_window_composited_leave_notify),
		      (gpointer)popup);

    g_signal_connect (popup->priv->extra_panes[i],
		      "key-press-event",
		      G_CALLBACK (hildon_desktop_popup_window_key_press_event_cb),
		      (gpointer)popup);

    /*FIXME: NO FOCUS FOR ANY WINDOW!!!!!!!!!!!!!!!!!! */
  }

  gtk_widget_pop_composite_child ();

  return object;
}

static void 
hildon_desktop_popup_finalize (GObject *object)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (object);
  register gint i;
  
  if (popup->priv->extra_panes)
   for (i=0; i < popup->priv->n_extra_panes; i++)
     g_object_unref (popup->priv->extra_panes[i]);

  G_OBJECT_CLASS (hildon_desktop_popup_window_parent_class)->finalize (object);
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

  gdk_window_set_back_pixmap (widget->window, NULL, FALSE);
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
  {
    if (popup->priv->extra_panes[i]->requisition.height > 1)	  
      gtk_widget_show (popup->priv->extra_panes[i]);
  }
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
hildon_desktop_popup_window_motion_notify (GtkWidget      *widget,
                                           GdkEventMotion *event)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (widget);
  gboolean in_panes_area = FALSE;
  gint w,h,x,y,i;

  w = widget->allocation.width;
  h = widget->allocation.height;

  gtk_widget_get_pointer (widget, &x, &y);

  if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
    in_panes_area = TRUE;

  if (!in_panes_area)
  {	  
    for (i=0; i < popup->priv->n_extra_panes; i++)
    {
      w = popup->priv->extra_panes[i]->allocation.width;
      h = popup->priv->extra_panes[i]->allocation.height;

      gtk_widget_get_pointer (popup->priv->extra_panes[i], &x, &y);

      if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
      {	       
        gtk_grab_remove (widget);		 
	 
        popup_grab_on_window
         (popup->priv->extra_panes[i]->window, GDK_CURRENT_TIME, TRUE);

        gtk_grab_add (popup->priv->extra_panes[i]);

        popup->priv->pane_with_grab = popup->priv->extra_panes[i];
	popup->priv->have_xgrab = FALSE;
	
	break;
      }
    }
  }
  
  return TRUE; 
}

static gboolean 
hildon_desktop_popup_window_leave_notify (GtkWidget        *widget,
					  GdkEventCrossing *event)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (widget);
  gint w,h,x,y,i;
  gboolean in_panes_area = FALSE;
 
  /* We have to ungrab the pointer in here, we should get this when we go to
  * a composited window
  */

  for (i=0; i < popup->priv->n_extra_panes; i++)
  {
    if (!GTK_WIDGET_VISIBLE (popup->priv->extra_panes[i]))
      continue; 

    w = popup->priv->extra_panes[i]->allocation.width;
    h = popup->priv->extra_panes[i]->allocation.height;

    gtk_widget_get_pointer (popup->priv->extra_panes[i], &x, &y);

    if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
    {	    
      in_panes_area = TRUE;
      break;
    }
  }

  if (in_panes_area)
  {	  
    gtk_grab_remove (GTK_WIDGET (popup));
 
    popup_grab_on_window 
      (popup->priv->extra_panes[i]->window, GDK_CURRENT_TIME, TRUE);

    gtk_grab_add (popup->priv->extra_panes[i]);

    popup->priv->pane_with_grab = popup->priv->extra_panes[i];
    popup->priv->have_xgrab = FALSE;
  }
  
  return 
    GTK_WIDGET_CLASS (hildon_desktop_popup_window_parent_class)->leave_notify_event (widget, event);	
}

static gboolean
hildon_desktop_popup_window_composited_leave_notify (GtkWidget *widget,
                                                     GdkEventCrossing *event,
                                                     HildonDesktopPopupWindow *popup)
{
  gint w,h,x,y,i;
  gboolean in_panes_area = FALSE;
  
  for (i=0; i < popup->priv->n_extra_panes; i++)
  {
    w = popup->priv->extra_panes[i]->allocation.width;
    h = popup->priv->extra_panes[i]->allocation.height;
    
    gtk_widget_get_pointer (popup->priv->extra_panes[i], &x, &y);
       
    if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
    {	    
      in_panes_area = TRUE;
      break;
    }
  }
  
  if (in_panes_area)
  {
    gtk_grab_remove (widget);

    popup_grab_on_window
      (popup->priv->extra_panes[i]->window, GDK_CURRENT_TIME, TRUE);

    gtk_grab_add (popup->priv->extra_panes[i]);

    popup->priv->pane_with_grab = popup->priv->extra_panes[i];
    popup->priv->have_xgrab = FALSE;
  }
  else
  {
    gtk_widget_get_pointer (GTK_WIDGET (popup), &x, &y);

    w = GTK_WIDGET (popup)->allocation.width;
    h = GTK_WIDGET (popup)->allocation.height;

    if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
    {
     gtk_grab_remove (widget);

     popup_grab_on_window (GTK_WIDGET (popup)->window, GDK_CURRENT_TIME, TRUE);

     gtk_grab_add (GTK_WIDGET (popup));

     popup->priv->pane_with_grab = NULL;
     popup->priv->have_xgrab = TRUE;
    }
  }

  return TRUE;
}
#ifdef MAEMO_CHANGES
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
  GdkScreen *screen;
  GList *stack;
  gboolean deactivate;

  if (event->state == GDK_VISIBILITY_UNOBSCURED)
    return FALSE;

  screen = gtk_widget_get_screen (widget);

  deactivate = FALSE;

  /* Inspect windows above us */
  stack = gdk_screen_get_window_stack (screen);
  
  if (stack != NULL)
  {
     GList *iter;

     iter = g_list_last (stack);

     while (iter)
     { 
       GdkWindow *win = iter->data;
       GdkWindowTypeHint type;

       if (win == widget->window)
         break;

       gdk_error_trap_push ();

       type = gdk_window_get_type_hint (win);
 
      if (!gdk_error_trap_pop () && 
 	  type != GDK_WINDOW_TYPE_HINT_NOTIFICATION && 
          type != GDK_WINDOW_TYPE_HINT_MENU && 
	  type != GDK_WINDOW_TYPE_HINT_POPUP_MENU &&
          type != GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU)
      {
        /* A non-message and non-menu window above us; close. */

#if 0	      
	/* This code is to check whether if we receive the visibility-notify from
	 * the virtual keyboard. In that case we don't close but the menu window
	 * is gonna be behind the virtual keyboard
	 */
	      
        Atom    type_ret;
        gint    format_ret;
        gulong  items_ret;
        gulong  after_ret;
        union
        {
           Atom *a;
           guchar *c;
        } window_type;
        gint    status;
 
        window_type.c = NULL;
 
        gdk_error_trap_push ();
 
        status = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (gdk_drawable_get_display (GDK_DRAWABLE (win))),
                                     GDK_WINDOW_XID (win),
                                     gdk_x11_get_xatom_by_name ("_NET_WM_WINDOW_TYPE"),
                                     0, G_MAXLONG,
                                     False,
                                     XA_ATOM,
                                     &type_ret,
                                      &format_ret,
                                      &items_ret,
                                      &after_ret,
                                      &window_type.c);
 
        if (!gdk_error_trap_pop () &&
            status == Success &&
            window_type.c != NULL &&
            items_ret == 1 &&
            window_type.a[0] == gdk_x11_get_xatom_by_name ("_NET_WM_WINDOW_TYPE_INPUT"))
        {
           break;
        }
#endif	

        deactivate = TRUE;
        break;
      }

      iter = iter->prev;
    }

    g_list_foreach (stack, (GFunc) g_object_unref, NULL);
    g_list_free (stack);
  }

  if (deactivate)
  {
    hildon_desktop_popup_window_popdown (popup);
    return TRUE;
  }

  return FALSE;
}

static gboolean 
hildon_desktop_popup_window_delete_event (GtkWidget *widget,
					  GdkEvent *event,
					  gpointer data)
{ 
  hildon_desktop_popup_window_popdown (HILDON_DESKTOP_POPUP_WINDOW (widget));
  
  return TRUE;
}
#endif
static gboolean
hildon_desktop_popup_window_composited_button_release (GtkWidget *widget,
                                                       GdkEventButton *event,
                                                       HildonDesktopPopupWindow *popup)
{
  gboolean in_panes_area  = FALSE,
           in_window_area = FALSE;
  gint x,y,w,h,i;

  if (!event)
    return FALSE;

  gtk_widget_get_pointer (widget, &x, &y);

  w = widget->allocation.width;
  h = widget->allocation.height;

  if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
    in_panes_area = TRUE;
  else
  {
     w = GTK_WIDGET (popup)->allocation.width;
     h = GTK_WIDGET (popup)->allocation.height;
      
     gtk_widget_get_pointer (widget, &x, &y);

     if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
       in_window_area = TRUE;
	  
     for (i=0; i < popup->priv->n_extra_panes; i++)
     {
       if (widget != popup->priv->extra_panes[i])
       {
         w = popup->priv->extra_panes[i]->allocation.width;
         h = popup->priv->extra_panes[i]->allocation.height;

         gtk_widget_get_pointer (popup->priv->extra_panes[i], &x, &y);

         if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
         {		 
           in_panes_area = TRUE;
           break;
	 }
       }
     }
  }

  /* Event outside of popup or in button area, close in clean way */
  if (!in_panes_area || in_window_area)
  {	  
    hildon_desktop_popup_window_popdown (popup);
#if defined(MAEMO_CHANGES) && defined(HAVE_XTEST)    
    /* This hack sends an extra button-event in order to not lose the event
     * when closing outside the menu so another button could receive it and
     * act consequently.
     */
    
    if (popup->priv->attached_widget)
    {	    
      gtk_widget_get_pointer (popup->priv->attached_widget, &x, &y);

      w = popup->priv->attached_widget->allocation.width;
      h = popup->priv->attached_widget->allocation.height;
      
      if ((x < 0) || (x > w) || (y < 0) || (y > h))
      {	      
        hildon_desktop_popup_menu_fake_button_event (event, TRUE);	      
        hildon_desktop_popup_menu_fake_button_event (event, FALSE);
      }
    }
#endif    
  }
  
  return TRUE;
}

static gboolean
hildon_desktop_popup_window_button_release_event (GtkWidget *widget,
                                                  GdkEventButton *event)
{
  HildonDesktopPopupWindow *popup = HILDON_DESKTOP_POPUP_WINDOW (widget);  	
  gboolean in_panes_area  = FALSE,
           in_window_area = FALSE;
  gint x,y,w,h,i;

  if (!event)
    return FALSE;

  gtk_widget_get_pointer (widget, &x, &y);

  w = widget->allocation.width;
  h = widget->allocation.height;

  if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
    in_window_area = TRUE;
  else
  {
     for (i=0; i < popup->priv->n_extra_panes; i++)
     {	     
       w = popup->priv->extra_panes[i]->allocation.width;
       h = popup->priv->extra_panes[i]->allocation.height;

       gtk_widget_get_pointer (popup->priv->extra_panes[i], &x, &y);

       if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
         in_panes_area = TRUE;

       break;
     }
  }
  
  /* Event outside of popup or in button area, close in clean way */
  if (!in_panes_area || !in_window_area)
  {	  
    hildon_desktop_popup_window_popdown (popup);
#if defined(MAEMO_CHANGES) && defined(HAVE_XTEST)
    /* This hack sends an extra button-event in order to not lose the event
     * when closing outside the menu so another button could receive it and
     * act consequently.
     */
    
    if (popup->priv->attached_widget)
    {	    
      gtk_widget_get_pointer (popup->priv->attached_widget, &x, &y);

      w = popup->priv->attached_widget->allocation.width;
      h = popup->priv->attached_widget->allocation.height;
      
      if ((x < 0) || (x > w) || (y < 0) || (y > h))
      {	      
        hildon_desktop_popup_menu_fake_button_event (event, TRUE);	      
        hildon_desktop_popup_menu_fake_button_event (event, FALSE);
      }
    }
#endif
  }
  return TRUE;
}

static void 
hildon_desktop_popup_window_calculate_position (HildonDesktopPopupWindow *popup)
{
  gint x=0,y=0,i,d_width=0;
  GtkRequisition orig_req, req, req_pane;

  gtk_widget_size_request (GTK_WIDGET (popup), &req);

  orig_req = req;

  if (popup->priv->position_func)
  {
    (* popup->priv->position_func) (popup, &x, &y, popup->priv->position_func_data);
  }	  

  gtk_window_move (GTK_WINDOW (popup), x, y);

  if (popup->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
  {
    if (popup->priv->direction == HD_POPUP_WINDOW_DIRECTION_RIGHT_BOTTOM)
      for (i=0; i < popup->priv->n_extra_panes; i++)
      {	
	gtk_widget_size_request (popup->priv->extra_panes[i], &req_pane);

	if (i > 0)
          gtk_widget_size_request (popup->priv->extra_panes[i-1], &req);

	d_width += req.width;
	
        gtk_window_move (GTK_WINDOW (popup->priv->extra_panes[i]),
			 d_width + x,
			 y + orig_req.height - req_pane.height);
      }
    else
    if (popup->priv->direction == HD_POPUP_WINDOW_DIRECTION_LEFT_TOP)	    
      for (i=0; i < popup->priv->n_extra_panes; i++)
      {	      
        gtk_widget_size_request (popup->priv->extra_panes[i], &req_pane);

        if (i > 0)
          gtk_widget_size_request (popup->priv->extra_panes[i-1], &req);

        d_width -= req.width;

        gtk_window_move (GTK_WINDOW (popup->priv->extra_panes[i]),
                         d_width - x,
                         y + orig_req.height - req_pane.height);
      }
  }	  
  else
  if (popup->priv->orientation == GTK_ORIENTATION_VERTICAL)
  {
    if (popup->priv->direction == HD_POPUP_WINDOW_DIRECTION_RIGHT_BOTTOM)
      for (i=0; i < popup->priv->n_extra_panes; i++)
        gtk_window_move (GTK_WINDOW (popup->priv->extra_panes[i]),
			 x,
			 req.height*(i+1) + y);
    else
    if (popup->priv->direction == HD_POPUP_WINDOW_DIRECTION_LEFT_TOP)	    
      for (i=0; i < popup->priv->n_extra_panes; i++)
        gtk_window_move (GTK_WINDOW (popup->priv->extra_panes[i]),
                         x,
                         req.height*(i+1) - y);
  } 
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

#ifdef HAVE_XTEST
static void 
hildon_desktop_popup_menu_fake_button_event (GdkEventButton *event, gboolean press)
{
  XTestFakeButtonEvent (GDK_DISPLAY (), event->button, press, CurrentTime);
}	
#endif
static GdkWindow *
popup_window_grab_transfer_window_get (HildonDesktopPopupWindow *popup)
{
  GdkWindow *window = 
    g_object_get_data (G_OBJECT (popup),
		       "popup-window-transfer-window");

  if (!window)
  {
    GdkWindowAttr attributes;
    gint attributes_mask;

    attributes.x = -100;
    attributes.y = -100;
    attributes.width = 10;
    attributes.height = 10;
    attributes.window_type = GDK_WINDOW_TEMP;
    attributes.wclass = GDK_INPUT_ONLY;
    attributes.override_redirect = TRUE;
    attributes.event_mask = 0;

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_NOREDIR;

    window = gdk_window_new (gtk_widget_get_root_window (GTK_WIDGET (popup)),
                             &attributes, attributes_mask);
    gdk_window_set_user_data (window, popup);

    gdk_window_show (window);

    g_object_set_data (G_OBJECT (popup), "popup-window-transfer-window", window);
  }

  return window;
}

static void
popup_window_grab_transfer_window_destroy (HildonDesktopPopupWindow *popup)
{
  GdkWindow *window = g_object_get_data (G_OBJECT (popup), "popup-window-transfer-window");
  if (window)
  {
    gdk_window_set_user_data (window, NULL);
    gdk_window_destroy (window);
    g_object_set_data (G_OBJECT (popup), "popup-window-transfer-window", NULL);
  }
}

static gboolean
popup_grab_on_window (GdkWindow *window,
                      guint32    activate_time,
                      gboolean   grab_keyboard)
{
  if ((gdk_pointer_grab (window, TRUE,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                         GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                         GDK_POINTER_MOTION_MASK,
                         NULL, NULL, activate_time) == 0))
  {
    if (!grab_keyboard || gdk_keyboard_grab (window, TRUE, activate_time) == 0)
      return TRUE;
    else
    {
      gdk_display_pointer_ungrab 
        (gdk_drawable_get_display (window), activate_time);
      return FALSE;
    }
  }

  return FALSE;
}

GtkWidget *
hildon_desktop_popup_window_new (guint n_panes,
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

GtkWidget *
hildon_desktop_popup_window_get_grabbed_pane (HildonDesktopPopupWindow *popup)
{
  g_assert (HILDON_DESKTOP_IS_POPUP_WINDOW (popup));

  if (popup->priv->pane_with_grab)
    return popup->priv->pane_with_grab;
  else
    return GTK_WIDGET (popup->priv->pane_with_grab);	   
}	

void
hildon_desktop_popup_window_jump_to_pane (HildonDesktopPopupWindow *popup, gint pane)
{
  g_assert (HILDON_DESKTOP_IS_POPUP_WINDOW (popup));
  
  if (pane >= (gint) popup->priv->n_extra_panes)
    pane = popup->priv->n_extra_panes -1;	  
  
  if (GTK_WIDGET_VISIBLE (GTK_WIDGET (popup)))
  {
    if (pane <= -1)
    { 
      if (popup->priv->have_xgrab)	    
        return;
      else
      { 
        gtk_grab_remove (popup->priv->pane_with_grab);	      

	popup_grab_on_window (GTK_WIDGET (popup)->window, GDK_CURRENT_TIME, TRUE);

        gtk_grab_add (GTK_WIDGET (popup));

        popup->priv->pane_with_grab = NULL;
        popup->priv->have_xgrab = TRUE;
      }
    }
    else
    { 
      GtkWidget *widget_with_grab;
	    
      if (popup->priv->have_xgrab)
        widget_with_grab = GTK_WIDGET (popup);
      else
        widget_with_grab = popup->priv->pane_with_grab;	      
      	
      if (widget_with_grab == popup->priv->extra_panes[pane])
        return;
      
      gtk_grab_remove (widget_with_grab);

      popup_grab_on_window (popup->priv->extra_panes[pane]->window, GDK_CURRENT_TIME, TRUE);

      gtk_grab_add (popup->priv->extra_panes[pane]);

      popup->priv->pane_with_grab = popup->priv->extra_panes[pane];
      popup->priv->have_xgrab = FALSE;	
    }	    
  }
}	

void 
hildon_desktop_popup_window_attach_widget (HildonDesktopPopupWindow *popup, GtkWidget *widget)
{
  g_return_if_fail (widget && GTK_IS_WIDGET (widget));

  popup->priv->attached_widget = widget;
}	

void 
hildon_desktop_popup_window_popup (HildonDesktopPopupWindow *popup,
				   HDPopupWindowPositionFunc func,
				   gpointer		     func_data,
				   guint32 		     activate_time)
{
  GdkWindow *transfer_window;
  GtkWidget *parent_toplevel;
	
  g_assert (HILDON_DESKTOP_IS_POPUP_WINDOW (popup));
	
  popup->priv->position_func      = func;
  popup->priv->position_func_data = func_data;

  transfer_window = popup_window_grab_transfer_window_get (popup);

  if (popup_grab_on_window (transfer_window, activate_time, TRUE))
    popup->priv->have_xgrab = TRUE;

  if (popup->priv->attached_widget)
  {
    parent_toplevel = gtk_widget_get_toplevel (popup->priv->attached_widget);

    if (parent_toplevel && GTK_IS_WINDOW (parent_toplevel))
    {
      register gint i;
      gtk_window_set_transient_for (GTK_WINDOW (popup),
                                    GTK_WINDOW (parent_toplevel));
 
      for (i=0; i < popup->priv->n_extra_panes; i++)
        gtk_window_set_transient_for (GTK_WINDOW (popup->priv->extra_panes[i]),
				      GTK_WINDOW (parent_toplevel));
    }
  }

  hildon_desktop_popup_window_calculate_position (popup);

  gtk_widget_show (GTK_WIDGET (popup));

  popup_grab_on_window (GTK_WIDGET (popup)->window, activate_time, TRUE); /* Should always succeed */

  gtk_grab_add (GTK_WIDGET (popup));

  g_signal_emit_by_name (popup, "popup-window");

  popup->priv->open = TRUE;
}

void 
hildon_desktop_popup_window_popdown (HildonDesktopPopupWindow *popup)
{
  gint i;

  if (!popup->priv->open)
    return;	  

  gtk_widget_hide (GTK_WIDGET (popup));
  
  if (popup->priv->have_xgrab)
  {	  
    gtk_grab_remove (GTK_WIDGET (popup));
    gtk_window_set_transient_for (GTK_WINDOW (popup), NULL);  
  }
  else
  {	  
    for (i=0; i < popup->priv->n_extra_panes; i++)
    {	    
      gtk_grab_remove (popup->priv->extra_panes[i]);	  
      gtk_window_set_transient_for (GTK_WINDOW (popup->priv->extra_panes[i]), NULL);
    }
  }

  popup_window_grab_transfer_window_destroy (popup);

  g_signal_emit_by_name (popup, "popdown-window");

  popup->priv->open = FALSE;
}

void 
hildon_desktop_popup_recalculate_position (HildonDesktopPopupWindow *popup)
{
  if (!GTK_WIDGET_REALIZED (popup))
    return;	  
	
  hildon_desktop_popup_window_calculate_position (popup);
}	

