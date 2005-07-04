/*
 * This file is part of hildon-libs
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Luc Pionchon <luc.pionchon@nokia.com>
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

/*
 * @file hildon-caption-control.c
 *
 * This file contains the implementation of Hildon Caption Control
 *
 */

#include <math.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "hildon-controlbar.h"

#include <libintl.h>
#define _(String) gettext(String)

#define HILDON_CONTROLBAR_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
     HILDON_TYPE_CONTROLBAR, HildonControlbarPrivate));

#define DEFAULT_WIDTH 234
#define DEFAULT_HEIGHT 30
#define DEFAULT_BORDER_WIDTH 2

#define HILDON_CONTROLBAR_STEP_INCREMENT 1
#define HILDON_CONTROLBAR_PAGE_INCREMENT 1
#define HILDON_CONTROLBAR_PAGE_SIZE 0
#define HILDON_CONTROLBAR_UPPER_VALUE  10
#define HILDON_CONTROLBAR_LOWER_VALUE 0.0
#define HILDON_CONTROLBAR_INITIAL_VALUE 0

static GtkScaleClass *parent_class;

typedef struct _HildonControlbarPrivate HildonControlbarPrivate;

static void
hildon_controlbar_class_init(HildonControlbarClass * controlbar_class);
static void hildon_controlbar_init(HildonControlbar * controlbar);
static GObject *
hildon_controlbar_constructor(GType type, guint n_construct_properties, 
  GObjectConstructParam *construct_properties);

static gint hildon_controlbar_button_press_event(GtkWidget * widget,
                                                 GdkEventButton * event);
static gint hildon_controlbar_button_release_event(GtkWidget * widget,
                                                   GdkEventButton * event);
static gint
hildon_controlbar_expose_event(GtkWidget * widget, GdkEventExpose * event);
static void
hildon_controlbar_size_request(GtkWidget * self, GtkRequisition * req);
static void
hildon_controlbar_paint(HildonControlbar * self, GdkRectangle * area);
static gboolean
hildon_controlbar_keypress(GtkWidget * widget, GdkEventKey * event);

static void
hildon_controlbar_value_changed( GtkAdjustment *adj, GtkRange *range );

GType hildon_controlbar_get_type(void)
{
    static GType controlbar_type = 0;

    if (!controlbar_type) {
        static const GTypeInfo controlbar_info = {
            sizeof(HildonControlbarClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) hildon_controlbar_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof(HildonControlbar),
            0,  /* n_preallocs */
            (GInstanceInitFunc) hildon_controlbar_init,
        };
        controlbar_type = g_type_register_static(GTK_TYPE_SCALE,
                                                 "HildonControlbar",
                                                 &controlbar_info, 0);
    }
    return controlbar_type;
}

struct _HildonControlbarPrivate {
    gboolean button_press;
    gint old_value;
};

static void
hildon_controlbar_class_init(HildonControlbarClass * controlbar_class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(controlbar_class);

  parent_class = g_type_class_peek_parent(controlbar_class);

  g_type_class_add_private(controlbar_class,
                           sizeof(HildonControlbarPrivate));

  widget_class->size_request = hildon_controlbar_size_request;
  widget_class->button_press_event = hildon_controlbar_button_press_event;
  widget_class->button_release_event = hildon_controlbar_button_release_event;
  widget_class->expose_event = hildon_controlbar_expose_event;
  widget_class->key_press_event = hildon_controlbar_keypress;
  G_OBJECT_CLASS(controlbar_class)->constructor = hildon_controlbar_constructor;

  gtk_widget_class_install_style_property(widget_class,
				  g_param_spec_uint("inner_border_width",
						    "Inner border width",
						    "The border spacing between the controlbar border and controlbar blocks.",
						    0, G_MAXINT,
						    DEFAULT_BORDER_WIDTH,
						    G_PARAM_READABLE));
}

static void hildon_controlbar_init(HildonControlbar * controlbar)
{
    GtkRange *range;
    HildonControlbarPrivate *priv;

    priv = HILDON_CONTROLBAR_GET_PRIVATE(controlbar);
    priv->button_press = FALSE;
    priv->old_value = 0;
    range = GTK_RANGE(controlbar);

    range->has_stepper_a = TRUE;
    range->has_stepper_d = TRUE;
    range->round_digits = -1;
    
    gtk_widget_set_size_request( GTK_WIDGET(controlbar), DEFAULT_WIDTH,
                                 DEFAULT_HEIGHT );
}

static GObject *hildon_controlbar_constructor(GType type, 
  guint n_construct_properties, GObjectConstructParam *construct_properties)
{
  GObject *obj;
  GtkAdjustment *adj;  

  obj = G_OBJECT_CLASS(parent_class)->constructor(type, 
    n_construct_properties, construct_properties);
   
  gtk_scale_set_draw_value (GTK_SCALE (obj), FALSE);

  adj = GTK_RANGE(obj)->adjustment;
  adj->step_increment = HILDON_CONTROLBAR_STEP_INCREMENT;
  adj->page_increment = HILDON_CONTROLBAR_PAGE_INCREMENT;
  adj->page_size = HILDON_CONTROLBAR_PAGE_SIZE;
  adj->upper = HILDON_CONTROLBAR_UPPER_VALUE;
  adj->lower = HILDON_CONTROLBAR_LOWER_VALUE;
  adj->value = HILDON_CONTROLBAR_INITIAL_VALUE;
  g_signal_connect( adj, "value-changed", 
                    G_CALLBACK(hildon_controlbar_value_changed), obj );
  return obj;
}

static void
hildon_controlbar_value_changed( GtkAdjustment *adj, GtkRange *range )
{
  HildonControlbarPrivate *priv = HILDON_CONTROLBAR_GET_PRIVATE(range);

  if( ABS(ceil(range->adjustment->value) - priv->old_value) >= 1 )
  {
    priv->old_value = ceil(range->adjustment->value);
    range->adjustment->value = priv->old_value;
  }
  else
    g_signal_stop_emission_by_name( range->adjustment, "value-changed" );
  gtk_adjustment_set_value( range->adjustment, priv->old_value );
}

/**
 * hildon_controlbar_new:
 * 
 * Creates a new #HildonControlbar widget.
 *
 * Return value: A #GtkWidget pointer of newly created #HildonControlbar
 * widget.
 */
GtkWidget *hildon_controlbar_new(void)
{
    return GTK_WIDGET(g_object_new(HILDON_TYPE_CONTROLBAR, NULL));
}

static gboolean
hildon_controlbar_keypress(GtkWidget * widget, GdkEventKey * event)
{
    if (event->keyval == GDK_Up || event->keyval == GDK_Down)
        return FALSE;
    return ((GTK_WIDGET_CLASS(parent_class)->key_press_event) (widget, event));
}

static void
hildon_controlbar_size_request(GtkWidget * self, GtkRequisition * req)
{
    if (GTK_WIDGET_CLASS(parent_class)->size_request)
        GTK_WIDGET_CLASS(parent_class)->size_request(self, req);

    req->width = DEFAULT_WIDTH;
    req->height = DEFAULT_HEIGHT;
}

/**
 * hildon_controlbar_set_value:
 * @self: Pointer to #HildonControlbar
 * @value: Value in range of >= 0 && < G_MAX_INT
 *
 * This method will change #HildonControlbar current value
 * to the given value.
 */
void hildon_controlbar_set_value(HildonControlbar * self, gint value)
{
    GtkAdjustment *adj;
    g_return_if_fail (HILDON_IS_CONTROLBAR (self));
    adj = GTK_RANGE(self)->adjustment;
 
    g_return_if_fail(value >= 0);

    if (value >= adj->upper)
        value = adj->upper;
    else if (value <= adj->lower)
        value = adj->lower;

    adj->value = value;
    gtk_adjustment_value_changed(adj);
}

/**
 * hildon_controlbar_get_value:
 * @self: Pointer to #HildonControlbar
 *
 * Accessor method for getting controlbars current value
 *
 * Return value: Current value as gint.
 */
gint hildon_controlbar_get_value(HildonControlbar * self)
{
    GtkAdjustment *adj;
    g_return_val_if_fail (HILDON_IS_CONTROLBAR (self), 0);
    adj = GTK_RANGE(self)->adjustment;
  
    return (gint) ceil(adj->value);
}

/**
 * hildon_controlbar_set_max:
 * @self: Pointer to #HildonControlbar
 * @max: Maximum value to set. The value needs to be
 * in range of > 0 && <= G_MAX_INT.
 * If the new maximum is smaller than current value,
 * the value will be adjusted so that
 * it equals the new maximum.
 *
 * Accessor method for setting #HildonControlbar maximum to the
 * given value.
 **/
void hildon_controlbar_set_max(HildonControlbar * self, gint max)
{
    GtkAdjustment *adj;
    g_return_if_fail (HILDON_IS_CONTROLBAR (self));
    adj = GTK_RANGE(self)->adjustment;

    if (max < adj->lower)
        return;
    else if( max == adj->lower )
      gtk_widget_set_sensitive( GTK_WIDGET(self), FALSE );
    else
      gtk_widget_set_sensitive( GTK_WIDGET(self), TRUE );
    
    if (adj->value > max)
        adj->value = max;

    adj->upper = max;
    gtk_adjustment_changed(adj);
}

/**
 * hildon_controlbar_set_min:
 * @self: Pointer to #HildonControlbar
 * @min: Minimum value to set. The value needs to be
 * in range of => 0 && <= G_MAX_INT.
 * If the new minimum is smaller than current value,
 * the value will be adjusted so that
 * it equals the new minimum.
 *
 * Accessor method for setting #HildonControlbar minimum to the
 * given value.
 **/
void hildon_controlbar_set_min(HildonControlbar * self, gint min)
{
    GtkAdjustment *adj;
    g_return_if_fail (HILDON_IS_CONTROLBAR (self));
    adj = GTK_RANGE(self)->adjustment;

    if (min < 0 || min >= adj->upper)
        return;
    else if( min == adj->upper )
      gtk_widget_set_sensitive( GTK_WIDGET(self), FALSE );
    else
      gtk_widget_set_sensitive( GTK_WIDGET(self), TRUE );
    
    if (adj->value > min)
        adj->value = min;

    adj->lower = min;
    gtk_adjustment_changed(adj);
}

/**
 * hildon_controlbar_set_range:
 * @self: Pointer to #HildonControlbar
 * @max: Maximum value to set. The value needs to be
 * in range of > 0 && <= G_MAX_INT.
 * @min: Minimum value to set. The value needs to be
 * in range of => 0 && <= G_MAX_INT.
 * If the new maximum is smaller than current value,
 * the value will be adjusted so that
 * it equals the new maximum.
 * If the new minimum is smaller than current value,
 * the value will be adjusted so that
 * it equals the new minimum.
 *
 * Accessor method for setting #HildonControlbar maximum to the
 * given value.
 **/
void hildon_controlbar_set_range(HildonControlbar * self, gint min,
                                 gint max)
{
    GtkAdjustment *adj;
    g_return_if_fail (HILDON_IS_CONTROLBAR (self));
    adj = GTK_RANGE(self)->adjustment;

    if (min < 0 || min >= max)
        return;

    if (adj->value > max)
        adj->value = max;

    adj->upper = max;
    adj->lower = min;
    
    if( adj->upper == adj->lower )
       gtk_widget_set_sensitive( GTK_WIDGET(self), FALSE );
    else
      gtk_widget_set_sensitive( GTK_WIDGET(self), TRUE );

    gtk_adjustment_changed(adj);
}

/**
 * hildon_controlbar_get_max:
 * @self: A pointer to #HildonControlbar
 *
 * Accessor method for getting controlbars current maximum value
 *
 * Return value: Maximum value as gint
 */
gint hildon_controlbar_get_max(HildonControlbar * self)
{
    GtkAdjustment *adj;
    g_return_val_if_fail (HILDON_IS_CONTROLBAR (self), 0);
    adj = GTK_RANGE(self)->adjustment;

    return (gint) adj->upper;
}

/**
 * hildon_controlbar_get_min:
 * @self: A pointer to #HildonControlbar
 *
 * Accessor method for getting controlbars current minimum value
 *
 * Return value: Minimum value as gint
 */
gint hildon_controlbar_get_min(HildonControlbar * self)
{
    GtkAdjustment *adj = GTK_RANGE(self)->adjustment;
    return (gint) adj->lower;
}

/*
 * Event handler for button press
 * Need to change button1 to button2 before passing this event to
 * parent handler. (see specs)
 * Also updates button_press variable so that we can draw hilites
 * correctly
 */
static gint hildon_controlbar_button_press_event(GtkWidget * widget,
                                                 GdkEventButton * event)
{
    HildonControlbar *self;
    HildonControlbarPrivate *priv;
    gboolean result = FALSE;

    g_return_val_if_fail(widget, FALSE);
    g_return_val_if_fail(event, FALSE);

    self = HILDON_CONTROLBAR(widget);
    priv = HILDON_CONTROLBAR_GET_PRIVATE(self);

    priv->button_press = TRUE;
    event->button = event->button == 1 ? 2 : event->button;

    /* Ugh dirty hack. We manipulate the mouse event location to
       compensate for centering the widget in case it is taller than the
       default height. */
    if (widget->allocation.height > DEFAULT_HEIGHT) {
        gint difference = widget->allocation.height - DEFAULT_HEIGHT;

        if (difference & 1)
            difference += 1;
        difference = difference / 2;

        event->y -= difference;
    }


    /* call the parent handler */
    if (GTK_WIDGET_CLASS(parent_class)->button_press_event)
        result =
            GTK_WIDGET_CLASS(parent_class)->button_press_event(widget, event);

    return result;
}

/*
 * Event handler for button release
 * Need to change button1 to button2 before passing this event to
 * parent handler. (see specs)
 * Also updates button_press variable so that we can draw hilites
 * correctly
 */
static gint hildon_controlbar_button_release_event(GtkWidget * widget,
                                                   GdkEventButton * event)
{
    HildonControlbar *self;
    HildonControlbarPrivate *priv;
    gboolean result = FALSE;

    g_return_val_if_fail(widget, FALSE);
    g_return_val_if_fail(event, FALSE);

    self = HILDON_CONTROLBAR(widget);
    priv = HILDON_CONTROLBAR_GET_PRIVATE(self);

    priv->button_press = FALSE;
    event->button = event->button == 1 ? 2 : event->button;

    /* call the parent handler */
    if (GTK_WIDGET_CLASS(parent_class)->button_release_event)
        result =
            GTK_WIDGET_CLASS(parent_class)->button_release_event(widget, event);
    return result;
}

/*
 * Event handler for expose event
 */
static gint hildon_controlbar_expose_event(GtkWidget * widget,
                                           GdkEventExpose * event)
{
    HildonControlbar *self = NULL;

    gboolean result = FALSE;
    gint old_height = -1;
    gint old_y = -1;

    g_return_val_if_fail(widget, FALSE);
    g_return_val_if_fail(event, FALSE);
    g_return_val_if_fail(HILDON_IS_CONTROLBAR(widget), FALSE);

    self = HILDON_CONTROLBAR(widget);

    old_height = widget->allocation.height;
    old_y = widget->allocation.y;

    if (widget->allocation.height > DEFAULT_HEIGHT) {
        int difference = widget->allocation.height - DEFAULT_HEIGHT;

        if (difference & 1)
            difference += 1;
        difference = difference / 2;

        widget->allocation.y += difference;
        widget->allocation.height = DEFAULT_HEIGHT;
    }

    /* call the parent handler */
    if (GTK_WIDGET_CLASS(parent_class)->expose_event)
        result = GTK_WIDGET_CLASS(parent_class)->expose_event(widget, event);
    hildon_controlbar_paint(self, &event->area);

    widget->allocation.height = old_height;
    widget->allocation.y = old_y;

    return TRUE;
}

/*
 * Paint method.
 * This is where all the work is actually done...
 */
static void hildon_controlbar_paint(HildonControlbar * self,
                                    GdkRectangle * area)
{
    HildonControlbarPrivate *priv;
    GtkWidget *widget = GTK_WIDGET(self);
    gint x = widget->allocation.x;
    gint y = widget->allocation.y;
    gint h = widget->allocation.height;
    gint w = widget->allocation.width;
    gint max = 0;
    gint stepper_size = 0;
    gint stepper_spacing = 0;
    gint inner_border_width = 0;
    gint block_area = 0, block_width = 0, block_x = 0, block_max = 0, block_height,block_y;
    gint block_count = 0;
    gint selected_blocks = 0;
    gint separatingpixels = 2;
    gint block_remains = 0;
    gint i, start_x, end_x, current_width;
    gint minimum_visible_bars = 0;
    GtkStateType state = GTK_STATE_NORMAL;

    g_return_if_fail(area);

    priv = HILDON_CONTROLBAR_GET_PRIVATE(self);
    if (GTK_WIDGET_SENSITIVE(self) == FALSE)
        state = GTK_STATE_INSENSITIVE;

    gtk_widget_style_get(GTK_WIDGET(self),
                         "stepper-size", &stepper_size,
                         "stepper-spacing", &stepper_spacing,
			 "inner_border_width", &inner_border_width, NULL);
    g_object_get(G_OBJECT(self), "minimum_visible_bars", &minimum_visible_bars, NULL);

    block_area = (w - 2 * stepper_size - 2 * stepper_spacing - 2 * inner_border_width);
    if (block_area <= 0)
        return;

    max = hildon_controlbar_get_max(self);
    block_max = max - GTK_RANGE(self)->adjustment->lower + minimum_visible_bars; 
    selected_blocks = priv->old_value - GTK_RANGE(self)->adjustment->lower;
 
    /* We check border width and maximum value and adjust
     * separating pixels for block width here. If the block size would
     * become too small, we make the separators smaller. Graceful fallback.
     */
    if( max == 0 )
    {
      separatingpixels = 3;
    }
    else if ((block_area - ((max - 1) * 3)) / max >= 4) {
        separatingpixels = 3;
    } else if ((block_area - ((max - 1) * 2)) / max >= 4) {
        separatingpixels = 2;
    } else if ((block_area - ((max - 1) * 1)) / max >= 4) {
        separatingpixels = 1;
    } else
        separatingpixels = 0;

    if( block_max == 0 )
    {
      /* If block max is 0 then we dim the whole control. */
      state = GTK_STATE_INSENSITIVE;
      block_width = block_area;
      block_remains = 0;
      block_max = 1;
    }
    else
    {
      block_width =
          (block_area - (separatingpixels * (block_max - 1))) / block_max;
      block_remains =
          (block_area - (separatingpixels * (block_max - 1))) % block_max;
    }

    block_x = x + stepper_size + stepper_spacing + inner_border_width;
    block_y = y + inner_border_width;
    block_height = h - 2 * inner_border_width;

    block_count = hildon_controlbar_get_value(self)/* - GTK_RANGE(self)->adjustment->lower*/;
    if (minimum_visible_bars == 0) block_count=block_count - GTK_RANGE(self)->adjustment->lower;

    /* Without this there is vertical block corruption when block_height = 
       1. This should work from 0 up to whatever */

    if (block_height < 2)
        block_height = 2;

    /* 
     * Changed the drawing of the blocks completely,
     * because of "do-not-resize-when-changing-max"-specs.
     * Now the code calculates from the block_remains when
     * it should add one pixel to the block and when not.
     */

    for (i = 1; i <= block_max; i++) {

        /* Here we calculate whether we add one pixel to current_width or
           not. */
        start_x = block_width * (i - 1) + ((i - 1) * block_remains) / block_max;
        end_x = block_width * i + (i * block_remains) / block_max;
        current_width = end_x - start_x;

        gtk_paint_box(widget->style, widget->window, state,
                      (i <= block_count) ? GTK_SHADOW_IN : GTK_SHADOW_OUT,
		      NULL, widget, "hildon_block",
                      block_x, block_y, current_width,
                      block_height);

        /* We keep the block_x separate because of the
           'separatingpixels' */
        block_x += current_width + separatingpixels;
    }

}
