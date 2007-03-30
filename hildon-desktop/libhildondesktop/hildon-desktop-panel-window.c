/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "hildon-desktop-panel-window.h"
#include "hildon-desktop-panel.h"

#define HILDON_DESKTOP_PANEL_WINDOW_GET_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), HILDON_DESKTOP_TYPE_PANEL_WINDOW, HildonDesktopPanelWindowPrivate))

G_DEFINE_TYPE (HildonDesktopPanelWindow, hildon_desktop_panel_window, HILDON_DESKTOP_TYPE_WINDOW);

enum
{
  PROP_0,
  PROP_HOR_W,
  PROP_HOR_H,
  PROP_VER_W,
  PROP_VER_H,
  PROP_STRETCH,
  PROP_GRAB_KEYB,
  PROP_ORIENTATION,
  PROP_X,
  PROP_Y,
  PROP_MOVE,
  PROP_MAG_WIDTH,
  PROP_MAG_HEIGHT
};

enum
{
  SIGNAL_ORIENTATION_CHANGED,
  N_SIGNALS
};

static gint signals[N_SIGNALS];  

typedef enum
{
  HILDON_DESKTOP_PANEL_WINDOW_GRAB_NONE = 0,
  HILDON_DESKTOP_PANEL_WINDOW_GRAB_MOVE
} HildonDesktopPanelWindowState;

struct _HildonDesktopPanelWindowPrivate
{
  HildonDesktopMultiscreen            *ms;
  HildonDesktopPanelWindowOrientation  orientation;
  HildonDesktopPanelWindowState        state;
  GdkRectangle                         geometry;
  GdkRectangle		               horiz_geometry;
  GdkRectangle		               vert_geometry;
  GdkRectangle	 	               magic_geometry;
  gboolean		               stretch;
  gboolean 		               grab_keyboard;
  gint			               drag_offset_x; /* For sure, this will be deleted */ 
  gint			               drag_offset_y; /* For sure, this will be deleted */
  gboolean		               move;
};

#define IS_HORIZONTAL(w) (w->priv->orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_HORIZONTAL)
#define IS_VERTICAL(w)   (w->priv->orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_VERTICAL)
#define IS_TOP(w)        (w->priv->orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP)
#define IS_BOTTOM(w)     (w->priv->orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM)
#define IS_LEFT(w)       (w->priv->orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT)
#define IS_RIGHT(w)      (w->priv->orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT)

#define MAGIC_GEOMETRY(w) (w->priv->magic_geometry.width != -1 && w->priv->magic_geometry.height != -1)

#define HANDLE_SIZE 0

#define CHECK_MULTISCREEN_HANDLER(o)                                \
	if (!o)                                                     \
	{                                                           \
	  o = g_object_new (HILDON_DESKTOP_TYPE_MULTISCREEN, NULL); \
	  g_object_ref (G_OBJECT (o));                              \
	}

/*static void set_focus_forall_cb (GtkWidget *widget, gpointer data);*/
static void hildon_desktop_panel_window_finalize (GObject *self);

static GObject *hildon_desktop_panel_window_constructor (GType gtype, 
                                                         guint n_params, 
                                                         GObjectConstructParam *params);

static void hildon_desktop_panel_window_get_property (GObject *object, 
                                                      guint prop_id, 
                                                      GValue *value, 
                                                      GParamSpec *pspec);		   

static void hildon_desktop_panel_window_set_property (GObject *object, 
                                                      guint prop_id, 
                                                      const GValue *value, 
                                                      GParamSpec *pspec);

static void hildon_desktop_panel_window_check_resize (GtkContainer *container);

static gboolean hildon_desktop_panel_press_button_event (GtkWidget *widget, 
                                                         GdkEventButton *event);

static gboolean hildon_desktop_panel_release_button_event (GtkWidget *widget, 
                                                           GdkEventButton *event);

static gboolean hildon_desktop_panel_motion_notify_event (GtkWidget *widget, 
                                                          GdkEventMotion *event);

static void hildon_desktop_panel_size_request (GtkWidget *widget, 
                                               GtkRequisition *requisition);

static void hildon_desktop_panel_size_allocate (GtkWidget *widget, 
                                                GtkAllocation *allocation);

static void hildon_desktop_panel_stop_grab (HildonDesktopPanelWindow *window, 
                                            guint32 timestamp);

static gboolean hildon_desktop_panel_grab_motion_event (HildonDesktopPanelWindow *window, 
                                                        GdkEventMotion *event);

static void hildon_desktop_panel_calc_orientation (HildonDesktopPanelWindow *window, 
                                                   gint px, 
                                                   gint py);

static void hildon_desktop_panel_begin_grab (HildonDesktopPanelWindow *window,
                                             HildonDesktopPanelWindowState state,
                                             gboolean grab_keyboard,
                                             guint32 timestamp);
static void hildon_desktop_panel_win_move_resize (HildonDesktopPanelWindow *window,gboolean move,gboolean resize);

GType
hildon_desktop_panel_window_orientation_get_type (void)
{
  static GType etype = 0;

  if (etype == 0) {
    static const GFlagsValue values[] = {
        { HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP,
          "HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP",
          "top" },
        { HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT,
          "HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT",
          "left" },
        { HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT,
          "HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT",
          "right" },
        { HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM,
          "HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM",
          "bottom" },
        { HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_VERTICAL,
          "HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_VERTICAL",
          "vertical" },
        { HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_HORIZONTAL,
          "HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_HORIZONTAL",
          "horizontal" },
        { 0, NULL, NULL }
    };

    etype = g_flags_register_static ("HildonDesktopPanelWindowOrientationType", values);
  }

  return etype;
}

static void 
hildon_desktop_panel_window_class_init (HildonDesktopPanelWindowClass *dskwindow_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (dskwindow_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (dskwindow_class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (dskwindow_class);

  g_type_class_add_private (dskwindow_class, sizeof (HildonDesktopPanelWindowPrivate));

  object_class->constructor  = hildon_desktop_panel_window_constructor;
  object_class->finalize     = hildon_desktop_panel_window_finalize;
  object_class->set_property = hildon_desktop_panel_window_set_property;
  object_class->get_property = hildon_desktop_panel_window_get_property;

  widget_class->button_press_event   = hildon_desktop_panel_press_button_event; 
  widget_class->button_release_event = hildon_desktop_panel_release_button_event;
  widget_class->motion_notify_event  = hildon_desktop_panel_motion_notify_event;
  widget_class->size_request         = hildon_desktop_panel_size_request;
  widget_class->size_allocate	     = hildon_desktop_panel_size_allocate;

  container_class->check_resize = hildon_desktop_panel_window_check_resize;
  
  signals[SIGNAL_ORIENTATION_CHANGED] =
        g_signal_new ("orientation-changed",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_FIRST,
		      G_STRUCT_OFFSET (HildonDesktopPanelWindowClass, orientation_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE, 1,
                      G_TYPE_INT);

  g_object_class_install_property (object_class,
		  		   PROP_HOR_W,
				   g_param_spec_int(
					   "horizontal_width",
					   "horiz_width",
					   "Max width when horizontal",
					    0,
					    G_MAXINT,
					    0,
					    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_HOR_H,
				   g_param_spec_int(
					   "horizontal_height",
					   "horiz_height",
					   "Max height when vertical",
					    0,
					    G_MAXINT,
					    0,
					    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
				   
  g_object_class_install_property (object_class,
				   PROP_VER_W,
				   g_param_spec_int(
					   "vertical_width",
					   "vert_width",
					   "Max width when vertical",
					    0,
					    G_MAXINT,
					    0,
					    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
						    
  g_object_class_install_property (object_class,
				   PROP_VER_H,
				   g_param_spec_int(
					   "vertical_height",
					   "vert_height",
					   "Max height when vertical",
					    0,
					    G_MAXINT,
					    0,
					    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	  
  g_object_class_install_property (object_class,
  				   PROP_STRETCH,
				   g_param_spec_boolean(
					   "stretch",
					   "stretch",
					   "To stretch the dock panel"
					   " to whole width or height",
					   FALSE,
					   G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
							
	  
  g_object_class_install_property (object_class,
 				   PROP_GRAB_KEYB,
				   g_param_spec_boolean(
					   "grab_keyboard",
					   "grab_keyb",
					   "Grab keyboard when dragging "
					   "dock panel",
					    FALSE,
					    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
												
  g_object_class_install_property (object_class,
		  		   PROP_X,
				   g_param_spec_int(
					   "x",
					   "X position horizontal",
					   "The X position when horizontal",
					    0,
					    G_MAXINT,
					    0,
					    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_Y,
				   g_param_spec_int(
					   "y",
					   "Y position vertical",
					   "Y position when vertical",
					    0,
					    G_MAXINT,
					    0,
					    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	
  g_object_class_install_property (object_class,
				   PROP_ORIENTATION,
				   g_param_spec_int(
					   "orientation",
					   "orientation",
					   "Panel orientation",
					    HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP,
					    HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM,
					    HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP,
					    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
 				   PROP_MOVE,
				   g_param_spec_boolean(
					   "move",
					   "move_panel",
					   "Allow movement of panel",
					    TRUE,
					    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));  

  g_object_class_install_property (object_class,
				   PROP_MAG_WIDTH,
				   g_param_spec_int(
					   "width",
					   "magic_width",
					   "Max width",
					    0,
					    G_MAXINT,
					    0,
					    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
						    
  g_object_class_install_property (object_class,
				   PROP_MAG_HEIGHT,
				   g_param_spec_int(
					   "height",
					   "magic_height",
					   "Max height",
					    0,
					    G_MAXINT,
					    0,
					    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void 
hildon_desktop_panel_window_init (HildonDesktopPanelWindow *window)
{
  g_return_if_fail (window);

  window->priv = HILDON_DESKTOP_PANEL_WINDOW_GET_PRIVATE (window);

  HILDON_DESKTOP_WINDOW (window)->container = NULL;

  window->priv->ms		      = NULL;
  window->priv->orientation           = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP;
  window->priv->state                 = HILDON_DESKTOP_PANEL_WINDOW_GRAB_NONE;
  window->priv->stretch		      = FALSE;
  window->priv->geometry.x            = -1;
  window->priv->geometry.y            = -1;
  window->priv->geometry.width        = -1;
  window->priv->geometry.height       = -1;
  window->priv->horiz_geometry.x      = -1;
  window->priv->horiz_geometry.y      =  0; /* this won't change */ 
  window->priv->horiz_geometry.width  = -1;
  window->priv->horiz_geometry.height = -1;
  window->priv->vert_geometry.x       =  0; /* this won't change */
  window->priv->vert_geometry.y       = -1;
  window->priv->vert_geometry.width   = -1;
  window->priv->vert_geometry.height  = -1;
  window->priv->magic_geometry.x      =  0; /* this won't change */
  window->priv->magic_geometry.y      = -1;
  window->priv->magic_geometry.width  = -1;
  window->priv->magic_geometry.height = -1;
  window->priv->drag_offset_x = window->priv->drag_offset_y = 0; /* For sure, this will be deleted */
  window->priv->move		      = TRUE;
  
  gtk_widget_add_events (GTK_WIDGET (window),
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK);
 
  /* FIXME: what do we do with focus??? */
}

static void 
hildon_desktop_panel_window_force_move (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{ 
  HildonDesktopPanelWindow *window = HILDON_DESKTOP_PANEL_WINDOW (widget);

  if (window->priv->move)
    hildon_desktop_panel_win_move_resize (window,TRUE,TRUE);
}

static GObject *  
hildon_desktop_panel_window_constructor (GType gtype,
			                 guint n_params,
			                 GObjectConstructParam *params)
{
  GObject *object;
  HildonDesktopPanelWindow *window;
  GtkWidget *widget, *alignment;
  gint padding_left = 0, padding_right = 0, padding_top = 0, padding_bottom = 0;
  
  object = G_OBJECT_CLASS (hildon_desktop_panel_window_parent_class)->constructor (gtype,
		  				                                   n_params,
						                                   params);

  widget = GTK_WIDGET (object);
  window = HILDON_DESKTOP_PANEL_WINDOW (object);

  
  GTK_WINDOW (window)->type = GTK_WINDOW_TOPLEVEL;

  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DOCK);

  gtk_widget_push_composite_child ();

  HILDON_DESKTOP_WINDOW (window)->container = g_object_new (HILDON_DESKTOP_TYPE_PANEL, NULL);

  if (window->priv->orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_HORIZONTAL)
    g_object_set (G_OBJECT (HILDON_DESKTOP_WINDOW (window)->container),
		  "orientation",
		  GTK_ORIENTATION_HORIZONTAL,
		  NULL);
  else
    g_object_set (G_OBJECT (HILDON_DESKTOP_WINDOW (window)->container),
		  "orientation",
		  GTK_ORIENTATION_VERTICAL,
		  NULL);

  g_object_get (G_OBJECT (window),
                "padding-left", &padding_left,
                "padding-right", &padding_right,
                "padding-top", &padding_top,
                "padding-bottom", &padding_bottom,
                NULL);

  alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);

  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 
                             padding_top,
                             padding_bottom,
                             padding_left,
                             padding_right);
  
  gtk_widget_show (alignment);

  gtk_container_add (GTK_CONTAINER (alignment),
		     GTK_WIDGET (HILDON_DESKTOP_WINDOW (window)->container));

  gtk_container_add (GTK_CONTAINER (window), alignment);

  gtk_widget_show (GTK_WIDGET (HILDON_DESKTOP_WINDOW (window)->container));

  g_signal_connect_after (object,
		          "map",
			  G_CALLBACK (hildon_desktop_panel_window_force_move),
			  NULL);

  gtk_widget_pop_composite_child ();

  return object;
}

static void 
hildon_desktop_panel_window_finalize (GObject *object)
{
  HildonDesktopPanelWindow *window;

  window = HILDON_DESKTOP_PANEL_WINDOW (object);

  if (window->priv->ms)
  {
    g_object_unref (G_OBJECT (window->priv->ms));
    window->priv->ms = NULL;
  }

  G_OBJECT_CLASS (hildon_desktop_panel_window_parent_class)->finalize (object);
}

static void 
hildon_desktop_panel_begin_grab (HildonDesktopPanelWindow *window,
                                 HildonDesktopPanelWindowState state,
                                 gboolean grab_keyboard,
                                 guint32 timestamp)
{
  gtk_grab_add (GTK_WIDGET (window));
  
  GdkCursor *cursor = gdk_cursor_new (GDK_CROSS);

  if (gdk_pointer_grab (GTK_WIDGET (window)->window, FALSE,
                    	GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    	NULL, cursor, timestamp) == GDK_GRAB_SUCCESS)
  {
    window->priv->state = state;
  }

  gdk_cursor_unref (cursor);
  
  if (grab_keyboard)
    gdk_keyboard_grab (GTK_WIDGET (window)->window, 
		       FALSE, timestamp);
}

static void 
hildon_desktop_panel_stop_grab (HildonDesktopPanelWindow *window,
			 guint32	     timestamp)
{
  g_return_if_fail (window->priv->state != HILDON_DESKTOP_PANEL_WINDOW_GRAB_NONE);
	
  gtk_grab_remove (GTK_WIDGET (window));

  gdk_pointer_ungrab (timestamp);
  gdk_keyboard_ungrab (timestamp);
}

static gboolean 
hildon_desktop_panel_grab_motion_event (HildonDesktopPanelWindow *window,
				 GdkEventMotion *event)
{
  /* add cases in order to add new features as resizing */
  switch (window->priv->state)
  {
    case HILDON_DESKTOP_PANEL_WINDOW_GRAB_MOVE: 
      hildon_desktop_panel_calc_orientation (window,
		      		      event->x_root,
		      		      event->y_root);
      return TRUE;
      break;

    default:
      break;
  }

  return FALSE;
}

static gboolean 
hildon_desktop_panel_press_button_event (GtkWidget *widget, 
				  GdkEventButton *event)
{
  HildonDesktopPanelWindow *window;
  GtkWidget *event_widget;

  g_return_val_if_fail (widget && HILDON_DESKTOP_IS_PANEL_WINDOW (widget),FALSE);

  window = HILDON_DESKTOP_PANEL_WINDOW (widget);

  if (!window->priv->move) return TRUE;

  g_return_val_if_fail (event->button == 1 || event->button == 2, FALSE);

  gdk_window_get_user_data (event->window, (gpointer) &event_widget);
  
  g_assert (GTK_IS_WIDGET (event_widget));

  gtk_widget_translate_coordinates (event_widget,
                                    widget,
                                    event->x,
                                    event->y,
                                    &window->priv->drag_offset_x,
                                    &window->priv->drag_offset_y);

  hildon_desktop_panel_begin_grab (window,
	  	            HILDON_DESKTOP_PANEL_WINDOW_GRAB_MOVE,
			    FALSE,
			    event->time);

  return TRUE;
}

static gboolean 
hildon_desktop_panel_release_button_event (GtkWidget *widget, 
				    GdkEventButton *event)
{
   HildonDesktopPanelWindow *window;

   window = HILDON_DESKTOP_PANEL_WINDOW (widget);
	
   if (window->priv->state == HILDON_DESKTOP_PANEL_WINDOW_GRAB_NONE)
     return FALSE;

   if (window->priv->grab_keyboard)
     return FALSE;   

   hildon_desktop_panel_stop_grab (window,event->time);

   return TRUE;
}

static void 
hildon_desktop_panel_win_set_orientation (HildonDesktopPanelWindow *window,
				          HildonDesktopPanelWindowOrientation orientation)
{
  gboolean rotate = FALSE, change_position = FALSE;

  if ((orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_HORIZONTAL) && IS_VERTICAL (window))
  {
    rotate = TRUE;  
  }
  else if ((orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_VERTICAL) && IS_HORIZONTAL (window))
  {
    rotate = TRUE;
  } 

  if (orientation != window->priv->orientation)
    change_position = TRUE;
    
  if (change_position)  
  {
    window->priv->orientation = orientation;
    g_signal_emit (window, signals[SIGNAL_ORIENTATION_CHANGED], 0, orientation);
  }

  if (HILDON_DESKTOP_WINDOW (window)->container && rotate)
  {
    if IS_HORIZONTAL (window)
      g_object_set (G_OBJECT (HILDON_DESKTOP_WINDOW (window)->container),
		    "orientation",
		    GTK_ORIENTATION_HORIZONTAL,
		    NULL);
    else
      g_object_set (G_OBJECT (HILDON_DESKTOP_WINDOW (window)->container),
  		    "orientation",
		    GTK_ORIENTATION_VERTICAL,
		    NULL);
  
  }

  if (change_position)
    gtk_widget_queue_resize (GTK_WIDGET (window));
}

static gboolean 
hildon_desktop_panel_motion_notify_event (GtkWidget *widget, 
			 	   GdkEventMotion *event)
{ 
  if (gdk_event_get_screen ((GdkEvent *)event) ==
      gtk_window_get_screen (GTK_WINDOW (widget)))
  { 
    return 
      hildon_desktop_panel_grab_motion_event (HILDON_DESKTOP_PANEL_WINDOW (widget), event);
  }
  else
    return FALSE;
}

static void 
hildon_desktop_panel_calc_orientation (HildonDesktopPanelWindow *window,
			        gint px,
				gint py)
{
 
  HildonDesktopPanelWindowOrientation new_orientation = window->priv->orientation;
  GdkScreen            *screen;
  gint                  hborder, 
  			vborder,
			monitor,
                        monitor_width,
  			monitor_height,
			new_x,
			new_y;

  screen  = gtk_window_get_screen (GTK_WINDOW (window));
  monitor = gdk_screen_get_monitor_at_point (screen, px, py); 

  CHECK_MULTISCREEN_HANDLER (window->priv->ms);

  if IS_HORIZONTAL (window)
    vborder = hborder = (3 * window->priv->geometry.height) >> 1;
  else
    vborder = hborder = (3 * window->priv->geometry.width)  >> 1;

  new_x = px - hildon_desktop_ms_get_x (window->priv->ms, screen, monitor);
  new_y = py - hildon_desktop_ms_get_y (window->priv->ms, screen, monitor);
  monitor_width  = hildon_desktop_ms_get_width (window->priv->ms, screen, monitor);
  monitor_height = hildon_desktop_ms_get_height (window->priv->ms, screen, monitor);
  
  switch (window->priv->orientation)
  {
    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP:
      if (new_y > (monitor_height - hborder))
	new_orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM;
      else 
      if (new_y > hborder)
      {
	if (new_x > (monitor_width - vborder))
	  new_orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT;
	else
	if (new_x < vborder)
	  new_orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT;
      } 
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM:
      if (new_y < hborder)
	new_orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP;
      else
      if (new_y < (monitor_height - hborder))
      {
	if (new_x > (monitor_width - vborder))
	  new_orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT;
	else
	if (new_x < vborder)
	  new_orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT;
      } 
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT:
      if (new_x > (monitor_width - vborder))
	new_orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT;
      else 
      if (new_x > vborder)
      {
	if (new_y > (monitor_height - hborder))
	  new_orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM;
	else
	if (new_y < hborder)
	  new_orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP;
      } 
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT:
      if (new_x < vborder)
	new_orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT;
      else 
      if (new_x < (monitor_width - vborder))
      {
	if (new_y > (monitor_height - hborder))
	  new_orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM;
	else 
	if (new_y < hborder)
	  new_orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP;
      }
      break;

    default:
      g_debug ("Unreachable");
      break;
  }

  hildon_desktop_panel_win_set_orientation (window,new_orientation);
}

static void
hildon_desktop_panel_update_size (HildonDesktopPanelWindow *window,
                           GdkScreen          *screen, 
                           gint                monitor)
{
  if (window->priv->stretch)
  {
    if IS_HORIZONTAL (window)
    {
      window->priv->geometry.width = hildon_desktop_ms_get_width (window->priv->ms, 
                                                           screen, 
                                                           monitor);

      if (MAGIC_GEOMETRY (window))
        window->priv->geometry.height = window->priv->magic_geometry.height;
      else
        window->priv->geometry.height = window->priv->horiz_geometry.height;
    }
    else if IS_VERTICAL (window)
    {
      window->priv->geometry.height = hildon_desktop_ms_get_height (window->priv->ms, 
                                                             screen, 
                                                             monitor);
      
      if (MAGIC_GEOMETRY (window))
        window->priv->geometry.width = window->priv->magic_geometry.height;
      else
        window->priv->geometry.width = window->priv->vert_geometry.height;
    }  
  } 
  else
  {
    if IS_HORIZONTAL (window) 
    {
      if (MAGIC_GEOMETRY (window))
      {
        window->priv->geometry.width  = window->priv->magic_geometry.width;
        window->priv->geometry.height = window->priv->magic_geometry.height;
      }
      else
      {
        window->priv->geometry.width  = window->priv->horiz_geometry.width;
        window->priv->geometry.height = window->priv->horiz_geometry.height;
      }
    }
    else if IS_VERTICAL (window)
    {
      if (MAGIC_GEOMETRY (window))
      {
	window->priv->geometry.width  = window->priv->magic_geometry.height;
	window->priv->geometry.height = window->priv->magic_geometry.width;
      }
      else
      {
        window->priv->geometry.width  = window->priv->vert_geometry.width;
        window->priv->geometry.height = window->priv->vert_geometry.height;
      }
    }
  }  
}

static void
hildon_desktop_panel_update_position (HildonDesktopPanelWindow *window, 
                               GdkScreen          *screen, 
                               gint                monitor)
{
  if IS_TOP (window)
  {
    window->priv->geometry.y = 0;

    if (window->priv->stretch)
      window->priv->geometry.x = 0;
    else
      window->priv->geometry.x = window->priv->horiz_geometry.x;
  }

  if IS_BOTTOM (window)
  { 
    gint height;

    if (MAGIC_GEOMETRY (window))
      height = window->priv->magic_geometry.height;
    else
      height = window->priv->horiz_geometry.height;

    window->priv->geometry.y = hildon_desktop_ms_get_height (window->priv->ms, 
                                                      screen, 
                                                      monitor) - height;

    if (window->priv->stretch)
      window->priv->geometry.x = 0;
    else
      window->priv->geometry.x = window->priv->horiz_geometry.x;
  }

  if IS_LEFT(window)
  {
    window->priv->geometry.x = window->priv->geometry.y = 0;
    
    if (window->priv->stretch)
      window->priv->geometry.y = 0;
    else
      window->priv->geometry.y = (hildon_desktop_ms_get_height (window->priv->ms, screen, monitor) * window->priv->horiz_geometry.x) / 
                                  hildon_desktop_ms_get_width (window->priv->ms, screen, monitor);/*FIXME: This is not well implemented */
  }									

  if IS_RIGHT (window)
  {
    gint width;

    if (MAGIC_GEOMETRY (window))
      width = window->priv->magic_geometry.height;
    else
      width = window->priv->vert_geometry.width;

    window->priv->geometry.x = hildon_desktop_ms_get_width (window->priv->ms, 
                                                     screen, 
                                                     monitor) - width;

    if (window->priv->stretch)
      window->priv->geometry.y = 0;
    else
      window->priv->geometry.y = window->priv->vert_geometry.y;/*FIXME: This is not well implemented */ 
  }
}

static void
hildon_desktop_panel_update_geometry (HildonDesktopPanelWindow *window)
{
  GdkScreen *screen;
  gint monitor = 0;
 
  CHECK_MULTISCREEN_HANDLER (window->priv->ms);

  screen = gtk_window_get_screen (GTK_WINDOW (window));

  if (GTK_WIDGET_REALIZED (window))
    monitor = gdk_screen_get_monitor_at_window (screen, 
                                                GTK_WIDGET (window)->window); 

  hildon_desktop_panel_update_size (window, screen, monitor);
  hildon_desktop_panel_update_position (window, screen, monitor);
}

static void 
hildon_desktop_panel_win_move_resize (HildonDesktopPanelWindow *window,
			       gboolean move,
			       gboolean resize)
{
  GtkWidget *widget;

  widget = GTK_WIDGET (window);

  g_return_if_fail (GTK_WIDGET_REALIZED (widget));

  g_debug ("Move: %s to x:%d y:%d & Resize: %s",
	   move ? "TRUE" : "FALSE", window->priv->geometry.x, window->priv->geometry.y,
	   resize ? "TRUE" : "FALSE");
  
  if (move && resize)
    gdk_window_move_resize (widget->window,
		    	    window->priv->geometry.x,
			    window->priv->geometry.y,
			    window->priv->geometry.width,
			    window->priv->geometry.height);

  if (resize)
    gdk_window_resize (widget->window,
		       window->priv->geometry.width,
		       window->priv->geometry.height);

  if (move)
    gdk_window_move (widget->window,
		     window->priv->geometry.x,
		     window->priv->geometry.y);
}

static void 
hildon_desktop_panel_window_check_resize (GtkContainer *container)
{
  GtkAllocation   allocation;
  GtkRequisition  requisition;
  GtkWidget      *widget;

  widget = GTK_WIDGET (container);

  if (!GTK_WIDGET_VISIBLE (widget))
    return;

  requisition.width  = -1;
  requisition.height = -1;

  gtk_widget_size_request (widget, &requisition);

  if (widget->allocation.width  != requisition.width ||
      widget->allocation.height != requisition.height)
    return;

  allocation = widget->allocation;
 
  gtk_widget_size_allocate (widget, &allocation);
}

static void 
hildon_desktop_panel_size_request (GtkWidget *widget, 
	 		    GtkRequisition *requisition)
{
  HildonDesktopPanelWindow *window;
  GtkBin	     *bin;
  GdkRectangle old_geometry;
  gboolean position_changed = FALSE;
  gboolean size_changed     = FALSE;

  window = HILDON_DESKTOP_PANEL_WINDOW (widget);
  bin	 = GTK_BIN (widget);

  old_geometry = window->priv->geometry;

  hildon_desktop_panel_update_geometry (window);

  requisition->width  = window->priv->geometry.width;
  requisition->height = window->priv->geometry.height;

  if (!GTK_WIDGET_REALIZED (widget))
    return;

  if (old_geometry.width  != window->priv->geometry.width ||
      old_geometry.height != window->priv->geometry.height)
  {
    size_changed = TRUE;
  }
  
  if (old_geometry.x != window->priv->geometry.x || 
      old_geometry.y != window->priv->geometry.y)
  {
    position_changed = TRUE;	
  }

  if (window->priv->move)
    hildon_desktop_panel_win_move_resize (window,position_changed,size_changed);
}

static void 
hildon_desktop_panel_size_allocate (GtkWidget *widget, 
			     GtkAllocation  *allocation)
{
  HildonDesktopPanelWindow *window = HILDON_DESKTOP_PANEL_WINDOW (widget);
  GtkBin *bin = GTK_BIN (widget);
  GtkAllocation challoc;

  widget->allocation = *allocation;

  if (window->priv->orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_HORIZONTAL)
  {
    challoc.x      = HANDLE_SIZE;
    challoc.y      = 0;
    challoc.width  = allocation->width - 2 * HANDLE_SIZE;
    challoc.height = allocation->height;
  } 
  else 
  {
    challoc.x      = 0;
    challoc.y      = HANDLE_SIZE;
    challoc.width  = allocation->width;
    challoc.height = allocation->height - 2 * HANDLE_SIZE;
  }

  if (GTK_WIDGET_MAPPED (widget) &&
      (challoc.x != bin->child->allocation.x ||
       challoc.y != bin->child->allocation.y ||
       challoc.width  != bin->child->allocation.width ||
       challoc.height != bin->child->allocation.height))
  {
    gdk_window_invalidate_rect (widget->window, &widget->allocation, FALSE);
  }
   
  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    gtk_widget_size_allocate (bin->child, &challoc);
}

static void 
hildon_desktop_panel_window_get_property (GObject *object,
                            	   guint prop_id,
                            	   GValue *value,
                            	   GParamSpec *pspec)
{
  HildonDesktopPanelWindow *window = HILDON_DESKTOP_PANEL_WINDOW (object);
	
  switch (prop_id)
  {
    case PROP_HOR_W:
      g_value_set_int     (value,
		      	   window->priv->horiz_geometry.width);
      break;			
    case PROP_HOR_H:
      g_value_set_int     (value,
		      	   window->priv->horiz_geometry.height);
      break;			
    case PROP_VER_W:
      g_value_set_int     (value,
		      	   window->priv->vert_geometry.width);
      break;			
    case PROP_VER_H:
      g_value_set_int     (value,
		      	   window->priv->vert_geometry.height);
      break;			
    case PROP_STRETCH:
      g_value_set_boolean (value,
		      	   window->priv->stretch);
      break;			
    case PROP_GRAB_KEYB:
      g_value_set_boolean (value,
		      	   window->priv->grab_keyboard);
      break;			
    case PROP_ORIENTATION:
      g_value_set_int     (value,
		           window->priv->orientation);
      break;			
    case PROP_X:
      g_value_set_int     (value,
		           window->priv->horiz_geometry.x);
      break;			
    case PROP_Y:
      g_value_set_int     (value,
		      	   window->priv->vert_geometry.y);
      break;
    case PROP_MOVE:
      g_value_set_boolean (value,
			   window->priv->move);      
      break;
    case PROP_MAG_WIDTH:
      g_value_set_int     (value,
		      	   window->priv->magic_geometry.width);
      break;			
    case PROP_MAG_HEIGHT:
      g_value_set_int     (value,
		      	   window->priv->magic_geometry.height);
      break;			
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void 
hildon_desktop_panel_window_set_property (GObject *object,
                            	   guint prop_id,
                            	   const GValue *value,
                            	   GParamSpec *pspec)
{
  HildonDesktopPanelWindow *window = HILDON_DESKTOP_PANEL_WINDOW (object);

  switch (prop_id)
  {
    case PROP_HOR_W:
      window->priv->horiz_geometry.width =
	      g_value_get_int (value);
      break;			
    case PROP_HOR_H:
      window->priv->horiz_geometry.height =
	      g_value_get_int (value);
      break;			
    case PROP_VER_W:
      window->priv->vert_geometry.width = 
	      g_value_get_int (value);
      break;			
    case PROP_VER_H:
      window->priv->vert_geometry.height =
	      g_value_get_int (value);
      break;			
    case PROP_STRETCH:
      window->priv->stretch =
	      g_value_get_boolean (value);
      break;			
    case PROP_GRAB_KEYB:
      window->priv->grab_keyboard =
	      g_value_get_boolean (value);
      break;			
    case PROP_ORIENTATION:
      window->priv->orientation =
	      g_value_get_int (value);
      break;			
    case PROP_X:
      window->priv->horiz_geometry.x =
	      g_value_get_int (value);
      break;			
    case PROP_Y:
      window->priv->vert_geometry.y =
	      g_value_get_int (value);
      break;		
    case PROP_MOVE:
      window->priv->move =
	      g_value_get_boolean (value);     
      break;
    case PROP_MAG_WIDTH:
      window->priv->magic_geometry.width = 
	      g_value_get_int (value);
      break;			
    case PROP_MAG_HEIGHT:
      window->priv->magic_geometry.height =
	      g_value_get_int (value);
      break;			 
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

void 
hildon_desktop_panel_window_set_multiscreen_handler (HildonDesktopPanelWindow *window, 
                                                     HildonDesktopMultiscreen *ms)
{
  g_assert (window && ms);
  g_assert (HILDON_DESKTOP_IS_PANEL_WINDOW (window));
  g_assert (HILDON_DESKTOP_IS_MULTISCREEN (ms));

  if (window->priv->ms)
  {
    g_warning ("There is already an assigned multiscreen handler");
    return;
  }

  g_object_ref (G_OBJECT (ms));

  window->priv->ms = ms;
  
}

GtkWidget *
hildon_desktop_panel_window_new ()
{
  GtkWidget *window;
  window = g_object_new (HILDON_DESKTOP_TYPE_PANEL_WINDOW, NULL);
  return window;
}
