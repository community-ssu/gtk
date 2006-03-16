/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include <config.h>
#include "gtkcheckbutton.h"
#include "gtkintl.h"
#include "gtklabel.h"
#include "gtkprivate.h"
#include "gtkalias.h"


#define INDICATOR_SIZE     24
#define INDICATOR_SPACING  2
#define FOCUS_X_PADDING    2

/* maJiK numbers for indicator */
#define INDICATOR_SIDE_PADDING 5
#define FOCUS_TOP_PADDING 7
#define FOCUS_DOWN_PADDING 1 

/* spacing to take account of the 1 pixel 
   transparency of the widgetfocus.png 
*/
#define HILDON_SPACING 1 
 
#define TOGGLE_ON_CLICK "toggle-on-click"

static void gtk_check_button_class_init     (GtkCheckButtonClass *klass);
static void gtk_check_button_init           (GtkCheckButton     *check_button);
static void gtk_check_button_size_request   (GtkWidget         *widget,
                                             GtkRequisition    *requisition);
static void gtk_check_button_size_allocate  (GtkWidget         *widget,
                                             GtkAllocation     *allocation);
static gint gtk_check_button_expose         (GtkWidget         *widget,
                                             GdkEventExpose    *event);
static gint gtk_check_button_button_press   (GtkWidget        *widget,
                                             GdkEventButton   *event);
static void gtk_check_button_paint          (GtkWidget        *widget,
                                             GdkRectangle     *area);
static void gtk_check_button_draw_indicator (GtkCheckButton   *check_button,
					     GdkRectangle     *area);
static void gtk_real_check_button_draw_indicator 
                                             (GtkCheckButton *check_button,
					      GdkRectangle   *area);

static void gtk_check_button_calc_indicator_size( GtkCheckButton *button, 
                                                  GdkRectangle *rect );

static void gtk_check_button_clicked (GtkButton *button);
static void gtk_check_button_update_state (GtkButton *button);

static GtkToggleButtonClass *parent_class = NULL;

GType
gtk_check_button_get_type (void)
{
  static GType check_button_type = 0;
  
  if (!check_button_type)
    {
      static const GTypeInfo check_button_info =
      {
	sizeof (GtkCheckButtonClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_check_button_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkCheckButton),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gtk_check_button_init,
      };
      
      check_button_type =
	g_type_register_static (GTK_TYPE_TOGGLE_BUTTON, "GtkCheckButton",
				&check_button_info, 0);
    }
  
  return check_button_type;
}

static void
gtk_check_button_class_init (GtkCheckButtonClass *class)
{
  GtkWidgetClass *widget_class;
  GtkButtonClass *button_class;

  widget_class = (GtkWidgetClass*) class;
  button_class = (GtkButtonClass*) class;
  parent_class = g_type_class_peek_parent (class);
  
  widget_class->size_request = gtk_check_button_size_request;
  widget_class->size_allocate = gtk_check_button_size_allocate;
  widget_class->expose_event = gtk_check_button_expose;
  widget_class->button_press_event = gtk_check_button_button_press;

  button_class->clicked = gtk_check_button_clicked;

  class->draw_indicator = gtk_real_check_button_draw_indicator;

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("indicator-size",
                                                             P_("Indicator Size"),
                                                             P_("Size of check or radio indicator"),
                                                             0,
                                                             G_MAXINT,
                                                             INDICATOR_SIZE,
                                                             GTK_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("indicator-spacing",
                                                             P_("Indicator Spacing"),
                                                             P_("Spacing around check or radio indicator"),
                                                             0,
                                                             G_MAXINT,
                                                             INDICATOR_SPACING,
                                                             GTK_PARAM_READABLE));
  /**
   * GtkCheckButton:focus-x-padding:
   *
   * Since: maemo 1.0
   */
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("focus-x-padding",
                                                             P_("Horizontal focus padding"),
                                                             P_("Additional horizontal focus padding for the checkbox indicator"),
                                                             0,
                                                             G_MAXINT,
                                                             FOCUS_X_PADDING,
                                                             GTK_PARAM_READABLE));
}

static void
gtk_check_button_init (GtkCheckButton *check_button)
{
  GTK_WIDGET_SET_FLAGS (check_button, GTK_NO_WINDOW);
  GTK_WIDGET_UNSET_FLAGS (check_button, GTK_RECEIVES_DEFAULT);
  GTK_TOGGLE_BUTTON (check_button)->draw_indicator = TRUE;
  GTK_BUTTON (check_button)->depress_on_activate = FALSE;
}

GtkWidget*
gtk_check_button_new (void)
{
  return g_object_new (GTK_TYPE_CHECK_BUTTON, NULL);
}


GtkWidget*
gtk_check_button_new_with_label (const gchar *label)
{
  return g_object_new (GTK_TYPE_CHECK_BUTTON, "label", label, NULL);
}

/**
 * gtk_check_button_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *         mnemonic character
 * @returns: a new #GtkCheckButton
 *
 * Creates a new #GtkCheckButton containing a label. The label
 * will be created using gtk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the check button.
 **/
GtkWidget*
gtk_check_button_new_with_mnemonic (const gchar *label)
{
  return g_object_new (GTK_TYPE_CHECK_BUTTON, "label", label, "use_underline", TRUE, NULL);
}


/* This should only be called when toggle_button->draw_indicator
 * is true.
 */
static void
gtk_check_button_paint (GtkWidget    *widget,
			GdkRectangle *area)
{
  GtkCheckButton *check_button = GTK_CHECK_BUTTON (widget);
  
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      gint border_width = 0;
      gint interior_focus = 0;
      gint focus_x_pad = 0;
      gint focus_width = 0;
      gint focus_pad = 0;
      gint indicator_size = 0;
      gint indicator_spacing = 0;
      
      gtk_widget_style_get (widget,
			    "interior-focus", &interior_focus,
			    "focus-line-width", &focus_width,
			    "focus-padding", &focus_pad,
			    "focus-x-padding", &focus_x_pad,
			    "indicator-size", &indicator_size,
			    "indicator-spacing", &indicator_spacing,
			    NULL);
      
      border_width = GTK_CONTAINER (widget)->border_width;
      
      /* Hildon: change the focus so that it draws around the entire
       * widget - including both the indicator *and* the label 
       */
      if (GTK_WIDGET_HAS_FOCUS (widget))
	{
          GtkWidget *child = GTK_BIN (widget)->child;
          gint w, h, x, y;
	  
	  if(child)
            {
              w = indicator_size + 2 * indicator_spacing + 
                  4 * (focus_width + focus_pad + focus_x_pad) + 
                  widget->style->xthickness + child->allocation.width;
            }
          else
            {
              w = indicator_size + 2 * indicator_spacing +
                  2 * (focus_width + focus_pad) + 2 * focus_x_pad;
            }
          
	  h = indicator_size + 2 * indicator_spacing + 
	          2 * (focus_width + focus_pad);
	  x = widget->allocation.x;
	  y = widget->allocation.y + (widget->allocation.height - h) / 2;
	  
	  if (gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL)
	    x = widget->allocation.x + widget->allocation.width - 
	      (2 * HILDON_SPACING) - (indicator_size + 2) - 
	      (indicator_spacing + 2);	  

	  if (interior_focus && child && GTK_WIDGET_VISIBLE (child))
	    {
              if (gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL)
		{
		  /* Move the "x" to the left, and enlarge the width, 
		     both accounting for the child 
                   */               
		  x += - child->allocation.width - HILDON_SPACING - 
		    (widget->style->xthickness);
		  w += child->allocation.width + HILDON_SPACING + 
		    (widget->style->xthickness);
		} else {
		  w = child->allocation.x + child->allocation.width + 
		    2 * widget->style->xthickness - x;
		}
		  
	      gtk_paint_focus (widget->style, widget->window, 
			       GTK_WIDGET_STATE (widget),
			       NULL, widget, "checkbutton", x, y, w, h);
	    }
	  else
	    gtk_paint_focus (widget->style, widget->window, 
                             GTK_WIDGET_STATE (widget), 
			     NULL, widget, "checkbutton", x, y, w, h);
	}
      gtk_check_button_draw_indicator (check_button, area);
    }
}

void
_gtk_check_button_get_props (GtkCheckButton *check_button,
			     gint           *indicator_size,
			     gint           *indicator_spacing)
{
  GtkWidget *widget =  GTK_WIDGET (check_button);
  
  if (indicator_size)
    gtk_widget_style_get (widget, "indicator_size", indicator_size, NULL);
  
  if (indicator_spacing)
    gtk_widget_style_get (widget, "indicator_spacing", indicator_spacing, NULL);
}

static void
gtk_check_button_size_request (GtkWidget      *widget,
			       GtkRequisition *requisition)
{
  GtkToggleButton *toggle_button = GTK_TOGGLE_BUTTON (widget);
  
  if (toggle_button->draw_indicator)
    {
      GtkWidget *child;
      gint temp;
      gint indicator_size;
      gint indicator_spacing;
      gint border_width = GTK_CONTAINER (widget)->border_width;
      gint focus_width;
      gint focus_pad;
      gint focus_x_pad;

      gtk_widget_style_get (GTK_WIDGET (widget),
			    "focus-line-width", &focus_width,
			    "focus-padding", &focus_pad,
			    "focus-x-padding", &focus_x_pad,
			    NULL);

      requisition->width = border_width * 2;
      requisition->height = border_width * 2;

      _gtk_check_button_get_props (GTK_CHECK_BUTTON (widget),
 				   &indicator_size, &indicator_spacing);
      
      child = GTK_BIN (widget)->child;
      if (child && GTK_WIDGET_VISIBLE (child))
	{
	  GtkRequisition child_requisition;
	  
	  gtk_widget_size_request (child, &child_requisition);
	  
	  requisition->width += child_requisition.width + 
	    2 * widget->style->xthickness;
	  requisition->height += child_requisition.height;
	  requisition->width += 2 * widget->style->xthickness;
	}
      
      requisition->width += (indicator_size + indicator_spacing * 2 + 
			     2 * (focus_width + focus_pad) + 2 * focus_x_pad);
      
      temp = indicator_size + indicator_spacing * 2;
      requisition->height = MAX (requisition->height, temp) + 2 * (focus_width + focus_pad);
    }
  else
    (* GTK_WIDGET_CLASS (parent_class)->size_request) (widget, requisition);
}

static void
gtk_check_button_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation)
{
  GtkCheckButton *check_button;
  GtkToggleButton *toggle_button;
  GtkButton *button;
  GtkAllocation child_allocation;

  button = GTK_BUTTON (widget);
  check_button = GTK_CHECK_BUTTON (widget);
  toggle_button = GTK_TOGGLE_BUTTON (widget);

  if (toggle_button->draw_indicator)
    {
      gint indicator_size;
      gint indicator_spacing;
      gint focus_width;
      gint focus_pad;
      gint focus_x_pad;
      
      _gtk_check_button_get_props (check_button, &indicator_size, &indicator_spacing);
      gtk_widget_style_get (widget,
			    "focus-line-width", &focus_width,
			    "focus-padding", &focus_pad,
			    "focus-x-padding", &focus_x_pad,
			    NULL);

      widget->allocation = *allocation;
      if (GTK_WIDGET_REALIZED (widget))
	gdk_window_move_resize (button->event_window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);
      
      if (GTK_BIN (button)->child && GTK_WIDGET_VISIBLE (GTK_BIN (button)->child))
	{
	  GtkRequisition child_requisition;
 	  gint border_width = GTK_CONTAINER (widget)->border_width;
	  
	  gtk_widget_get_child_requisition (GTK_BIN (button)->child, &child_requisition);
 
	  child_allocation.width = MIN (child_requisition.width,
					allocation->width -
					((border_width + focus_width + focus_pad) * 2
					 - 2 * widget->style->xthickness + 
					 indicator_size + 
					 indicator_spacing * 2 ) );

	  child_allocation.width = MAX (child_allocation.width, 1);
	  
	  child_allocation.height = MIN (child_requisition.height,
					 allocation->height - (border_width + focus_width + focus_pad) * 2);
	  child_allocation.height = MAX (child_allocation.height, 1);
	  
	  child_allocation.x = (border_width + indicator_size + indicator_spacing * 2 +
				widget->style->xthickness + 
				widget->allocation.x + 
				focus_width + focus_pad + focus_x_pad); 
	  child_allocation.y = widget->allocation.y +
	    (allocation->height - child_allocation.height) / 2;
	  
	  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
	    child_allocation.x = allocation->x + allocation->width
	      - (child_allocation.x - allocation->x + child_allocation.width);
	  
	  gtk_widget_size_allocate (GTK_BIN (button)->child, &child_allocation);
	}
    }
  else
    (* GTK_WIDGET_CLASS (parent_class)->size_allocate) (widget, allocation);
}

static gint
gtk_check_button_expose (GtkWidget      *widget,
			 GdkEventExpose *event)
{
  GtkCheckButton *check_button;
  GtkToggleButton *toggle_button;
  GtkBin *bin;
  
  check_button = GTK_CHECK_BUTTON (widget);
  toggle_button = GTK_TOGGLE_BUTTON (widget);
  bin = GTK_BIN (widget);
  
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      if (toggle_button->draw_indicator)
	{
	  gtk_check_button_paint (widget, &event->area);
	  
	  if (bin->child)
	    gtk_container_propagate_expose (GTK_CONTAINER (widget),
					    bin->child,
					    event);
	}
      else if (GTK_WIDGET_CLASS (parent_class)->expose_event)
	(* GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);
    }
  
  return FALSE;
}

/* This event handler was introduced to fix problems with check buttons packed
   inside Hildon Caption Controls; in such cases the widget might be allocated
   a lot of blank space which isn't supposed to react to stylus taps. */
static gboolean
gtk_check_button_button_press (GtkWidget      *widget,
			       GdkEventButton *event)
{
  GtkButton *button;

  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkRectangle indicator = {0, 0, 0, 0};
      gint child_x = 0;
      gint child_y = 0;
      gint child_width = 0;
      gint child_height = 0;

      button = GTK_BUTTON (widget);
      gtk_check_button_calc_indicator_size(GTK_CHECK_BUTTON(widget),
                                           &indicator);

      if (GTK_BIN(widget)->child)
	{
	  GtkWidget *child = GTK_BIN(widget)->child;

	  child_x = child->allocation.x;
	  child_y = child->allocation.y;
	  child_width = child->allocation.width;
	  child_height = child->allocation.height;
          
          /* did the event occur above or below the actual check button? */
          if (event->y < child_y - widget->allocation.y ||
              event->y > child_y + child_height - widget->allocation.y)
            return FALSE;
        }

      /* did the event occur to the right from the actual check button? */
      if (event->x > 2 * indicator.x + indicator.width + child_width)
        return FALSE;

      if (button->focus_on_click && !GTK_WIDGET_HAS_FOCUS (widget))
	gtk_widget_grab_focus (widget);

      if (event->button == 1)
	gtk_button_pressed (button);
    }

  return TRUE;
}

static void
gtk_check_button_draw_indicator (GtkCheckButton *check_button,
				 GdkRectangle   *area)
{
  GtkCheckButtonClass *class;
  
  g_return_if_fail (GTK_IS_CHECK_BUTTON (check_button));
  
  class = GTK_CHECK_BUTTON_GET_CLASS (check_button);
  
  if (class->draw_indicator)
    (* class->draw_indicator) (check_button, area);
}

static void
gtk_real_check_button_draw_indicator (GtkCheckButton *check_button,
				      GdkRectangle   *area)
{
  GtkWidget *widget;
  GtkButton *button;
  GtkToggleButton *toggle_button;
  GtkStateType state_type;
  GtkShadowType shadow_type;
  
  GdkRectangle indicator = {0, 0, 0, 0};
  
  if (GTK_WIDGET_DRAWABLE (check_button))
    {
      widget = GTK_WIDGET (check_button);
      button = GTK_BUTTON (check_button);
      toggle_button = GTK_TOGGLE_BUTTON (check_button);
      gtk_check_button_calc_indicator_size( check_button, &indicator );
      
      /* move indicator to root coordinates */
      indicator.x += widget->allocation.x;
      indicator.y += widget->allocation.y;
      
      if (toggle_button->inconsistent)
	shadow_type = GTK_SHADOW_ETCHED_IN;
      else if (toggle_button->active)
	shadow_type = GTK_SHADOW_IN;
      else
	shadow_type = GTK_SHADOW_OUT;
      
      if (button->activate_timeout || (button->button_down && button->in_button))
	state_type = GTK_STATE_ACTIVE;
      else if (button->in_button)
	state_type = GTK_STATE_PRELIGHT;
      else if (!GTK_WIDGET_IS_SENSITIVE (widget))
	state_type = GTK_STATE_INSENSITIVE;
      else
	state_type = GTK_STATE_NORMAL;
      
      /* Hildon change. We want to draw active image always when we have
       * focus. */
      if (GTK_WIDGET_HAS_FOCUS (widget))
	state_type = GTK_STATE_ACTIVE;

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
	{
	  indicator.x = widget->allocation.x + widget->allocation.width 
	    - (indicator.width + indicator.x - widget->allocation.x);
	  
	}

      gtk_paint_check (widget->style, widget->window,
		       state_type, shadow_type,
		       area, widget, "checkbutton",
		       indicator.x, indicator.y, 
		       indicator.width, indicator.height);
    }
}


/* calculates the size and position of the indicator
 * relative to the origin of the check button.
 */
static void gtk_check_button_calc_indicator_size( GtkCheckButton *button, 
						  GdkRectangle *rect )
{
  gint indicator_size;
  gint indicator_spacing;
  gint focus_width;
  gint focus_pad;
  gint focus_x_pad;
  gboolean interior_focus;
  GtkWidget *child; 
  GtkWidget *widget = GTK_WIDGET(button);
  
    
  gtk_widget_style_get (widget, "interior_focus", &interior_focus,
			"focus-line-width", &focus_width, 
			"focus-padding", &focus_pad, 
			"focus-x-padding", &focus_x_pad, 
			"indicator-size", &indicator_size,
			"indicator-spacing", &indicator_spacing,
			NULL
			);
  

  /* HILDON: We want the indicator to be positioned according to the spec.
   *
   */
  rect->x = indicator_spacing + focus_x_pad;
  rect->y = ( widget->allocation.height - indicator_size ) / 2;
  
  /* Hildon: we always add space for the focus */
  rect->x += focus_width + focus_pad;      

  child = GTK_BIN (widget)->child;
  if (interior_focus && child && GTK_WIDGET_VISIBLE (child))
    {
      rect->y += HILDON_SPACING;      
    }
  
  rect->width = indicator_size;
  rect->height = indicator_size;
}

static void
gtk_check_button_clicked (GtkButton *button)
{
  GtkToggleButton *toggle_button = GTK_TOGGLE_BUTTON (button);

  toggle_button->active = !toggle_button->active;
  gtk_toggle_button_toggled (toggle_button);
  
  gtk_check_button_update_state (button);
  
  g_object_notify (G_OBJECT (toggle_button), "active");
}

static void
gtk_check_button_update_state (GtkButton *button)
{
  GtkToggleButton *toggle_button = GTK_TOGGLE_BUTTON (button);
  gboolean depressed;
  GtkStateType new_state;
  
  if (toggle_button->inconsistent)
    depressed = FALSE;
  else if (button->in_button && button->button_down)
    depressed = TRUE;
  else
    depressed = toggle_button->active;
      
  if (button->in_button && 
      (!button->button_down || toggle_button->draw_indicator))
    new_state = GTK_STATE_PRELIGHT;
  else
    new_state = depressed ? GTK_STATE_ACTIVE : GTK_STATE_NORMAL;
  
  _gtk_button_set_depressed (button, depressed); 
  gtk_widget_set_state (GTK_WIDGET (toggle_button), new_state);
}

#define __GTK_CHECK_BUTTON_C__
#include "gtkaliasdef.c"
