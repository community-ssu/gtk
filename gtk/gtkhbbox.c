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
#include "gtkhbbox.h"
#include "gtkintl.h"
#include "gtkalias.h"


static void gtk_hbutton_box_size_request  (GtkWidget      *widget,
					   GtkRequisition *requisition);
static void gtk_hbutton_box_size_allocate (GtkWidget      *widget,
					   GtkAllocation  *allocation);

static gint default_spacing = 30;
static gint default_layout_style = GTK_BUTTONBOX_EDGE;

G_DEFINE_TYPE (GtkHButtonBox, gtk_hbutton_box, GTK_TYPE_BUTTON_BOX)

static void
gtk_hbutton_box_class_init (GtkHButtonBoxClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass*) class;

  widget_class->size_request = gtk_hbutton_box_size_request;
  widget_class->size_allocate = gtk_hbutton_box_size_allocate;
}

static void
gtk_hbutton_box_init (GtkHButtonBox *hbutton_box)
{
	/* button_box_init has done everything already */
}

GtkWidget*
gtk_hbutton_box_new (void)
{
  GtkHButtonBox *hbutton_box;

  hbutton_box = g_object_new (GTK_TYPE_HBUTTON_BOX, NULL);

  return GTK_WIDGET (hbutton_box);
}


/* set default value for spacing */

void 
gtk_hbutton_box_set_spacing_default (gint spacing)
{
  default_spacing = spacing;
}


/* set default value for layout style */

void 
gtk_hbutton_box_set_layout_default (GtkButtonBoxStyle layout)
{
  g_return_if_fail (layout >= GTK_BUTTONBOX_DEFAULT_STYLE &&
		    layout <= GTK_BUTTONBOX_CENTER);

  default_layout_style = layout;
}

/* get default value for spacing */

gint 
gtk_hbutton_box_get_spacing_default (void)
{
  return default_spacing;
}


/* get default value for layout style */

GtkButtonBoxStyle
gtk_hbutton_box_get_layout_default (void)
{
  return default_layout_style;
}


  
static void
gtk_hbutton_box_size_request (GtkWidget      *widget,
			      GtkRequisition *requisition)
{
  GtkBox *box;
  GtkButtonBox *bbox;
  gint nvis_children;
  gint child_width;
  gint child_height;
  gint spacing;
  GtkButtonBoxStyle layout;
  
  box = GTK_BOX (widget);
  bbox = GTK_BUTTON_BOX (widget);

  spacing = box->spacing;
  layout = bbox->layout_style != GTK_BUTTONBOX_DEFAULT_STYLE
	  ? bbox->layout_style : default_layout_style;
  
  _gtk_button_box_child_requisition (widget,
                                     &nvis_children,
				     NULL,
                                     &child_width,
                                     &child_height);

  if (nvis_children == 0)
  {
    requisition->width = 0; 
    requisition->height = 0;
  }
  else
  {
    switch (layout)
    {
    case GTK_BUTTONBOX_SPREAD:
      requisition->width =
	      nvis_children*child_width + ((nvis_children+1)*spacing);
      break;
    case GTK_BUTTONBOX_EDGE:
    case GTK_BUTTONBOX_START:
    case GTK_BUTTONBOX_END:
    case GTK_BUTTONBOX_CENTER:
      requisition->width = nvis_children*child_width + ((nvis_children-1)*spacing);
      break;
    default:
      g_assert_not_reached();
      break;
    }
	  
    requisition->height = child_height;
  }
	  
  requisition->width += GTK_CONTAINER (box)->border_width * 2;
  requisition->height += GTK_CONTAINER (box)->border_width * 2;
}


#if defined(MAEMO_CHANGES)
/* This function is pretty much an abomination against reason.
   More or less it's supposed to:
   - If an homogeneous layout is possible:
   + primary_width and secondary_width will be the sizes of the primary and
     secondary groups (basically the homogeneous width times the number of items).
   + children_widths will contain the homogeneous width for all items

   - If an homogeneous layout is NOT possible:
   + primary_width and secondary_width will contain the sum of the widths for each
     group
   + children_width will contain the heterogeneous width of each button, being this
     its requisition plus any possible extra width added

   All the other parameters come straight from _gtk_button_box_child_requisition
*/
static gint*
gtk_hbutton_box_get_children_sizes (GtkWidget *widget,
                                    gint *primary_width,
                                    gint *secondary_width,
                                    gint *child_height,
                                    gint *nvis_children,
                                    gint *n_secondaries)
{
  GtkAllocation *allocation = &widget->allocation;
  gint *children_widths;
  gint child_width;
  gint total_width;
  gint total_spacing;
  gint i;

  _gtk_button_box_child_requisition (widget,
                                     nvis_children,
                                     n_secondaries,
                                     &child_width,
                                     child_height);

  children_widths = g_slice_alloc (sizeof (gint) * (*nvis_children));

  total_width = *nvis_children * child_width;
  total_spacing = (*nvis_children - 1) * GTK_BOX (widget)->spacing;

  if (total_width + total_spacing > allocation->width)
    {
      /* homogeneous allocation too wide to fit container, shrink the buttons
       * to their size requisition */
      GList *children;
      gint extra_space;

      *primary_width = 0;
      *secondary_width = 0;
      children = GTK_BOX (widget)->children;
      i = 0;

      while (children)
        {
          GtkBoxChild *child = children->data;
          GtkWidget *child_widget = child->widget;
          children = children->next;

          if (GTK_WIDGET_VISIBLE (child_widget))
            {
              GtkRequisition req;

	      /* FIXME: take child-min-width and child-internal-pad-x style
	       * properties into account
	       */

              gtk_widget_size_request (child_widget, &req);

              if (! child->is_secondary)
                *primary_width += req.width;
              else
                *secondary_width += req.width;

              children_widths[i++] = req.width;
            }
        }

      total_width = *primary_width + *secondary_width;
      extra_space = allocation->width - (total_width + total_spacing);

      /* If extra space available, distribute it evenly to the buttons.
       * XXX: Smallest buttons should probably get the biggest share instead,
       *      to maximize button sizes and approximate homogeneous allocation.
       *
       * XXX: If extra_space is < 0 the layout will be broken (label truncation
       *      etc.) but we should still shrink the children
       */
      if (extra_space > 0)
        {
          gint extra;

          extra = extra_space / *nvis_children;
          children = GTK_BOX (widget)->children;
          i = 0;

          while (children)
            {
              GtkBoxChild *child = children->data;
              GtkWidget *child_widget = child->widget;
              children = children->next;

              if (GTK_WIDGET_VISIBLE (child_widget))
                {
                  children_widths[i++] += extra;

                  if (! child->is_secondary)
                    *primary_width += extra;
                  else
                    *secondary_width += extra;
                }
            }
        }
    }
  else
    {
      /* homogeneous allocation */
      *primary_width = child_width * (*nvis_children - *n_secondaries);
      *secondary_width = child_width * *n_secondaries;
      for (i = 0; i < *nvis_children; i++)
        children_widths[i] = child_width;
    }

  return children_widths;
}

static void
gtk_hbutton_box_size_allocate (GtkWidget     *widget,
			       GtkAllocation *allocation)
{
  gint primary_width, secondary_width, child_height;
  gint *children_widths;
  gint x, y;
  gint secondary_x;
  gint nvis_children, n_secondaries, childspacing;
  gint n_primaries, inner_width;
  GList *children;
  GtkBoxChild *child;
  gint i;
  GtkButtonBoxStyle layout;

  widget->allocation = *allocation;

  children_widths = gtk_hbutton_box_get_children_sizes (widget,
							&primary_width,
							&secondary_width,
							&child_height,
							&nvis_children,
							&n_secondaries);

  n_primaries = nvis_children - n_secondaries;
  inner_width = allocation->width - 2 * GTK_CONTAINER (widget)->border_width;
#define primary_spacing   (childspacing * (n_primaries - 1))
#define secondary_spacing (childspacing * (n_secondaries - 1))

  layout = GTK_BUTTON_BOX (widget)->layout_style != GTK_BUTTONBOX_DEFAULT_STYLE
    ? GTK_BUTTON_BOX (widget)->layout_style : default_layout_style;

  switch (layout)
    {
    case GTK_BUTTONBOX_SPREAD:
      childspacing = (inner_width
                      - (primary_width + secondary_width)) / (nvis_children + 1);
      x = allocation->x + GTK_CONTAINER (widget)->border_width + childspacing;
      secondary_x = x + primary_width + primary_spacing + childspacing;
      break;
    case GTK_BUTTONBOX_EDGE:
      if (nvis_children >= 2)
        {
          childspacing = (inner_width
                          - (primary_width + secondary_width)) / (nvis_children + 1);
          x = allocation->x + GTK_CONTAINER (widget)->border_width;
          secondary_x = x + primary_width + primary_spacing + childspacing;
        }
      else
        {
          /* one or zero children, just center */
          childspacing = inner_width;
          x = secondary_x = allocation->x + (allocation->width - primary_width) / 2;
        }
      break;
    case GTK_BUTTONBOX_START:
      childspacing = GTK_BOX (widget)->spacing;
      x = allocation->x + GTK_CONTAINER (widget)->border_width;
      secondary_x = allocation->x + allocation->width
        - secondary_width
        - secondary_spacing
        - GTK_CONTAINER (widget)->border_width;
      break;
    case GTK_BUTTONBOX_END:
      childspacing = GTK_BOX (widget)->spacing;
      x = allocation->x + allocation->width
        - primary_width
        - primary_spacing
        - GTK_CONTAINER (widget)->border_width;
      secondary_x = allocation->x + GTK_CONTAINER (widget)->border_width;
      break;
    case GTK_BUTTONBOX_CENTER:
      childspacing = GTK_BOX (widget)->spacing;
      x = allocation->x +
        (allocation->width
         - (primary_width + primary_spacing))/2
         + (secondary_width + secondary_spacing + childspacing)/2;
      secondary_x = allocation->x + GTK_CONTAINER (widget)->border_width;
      break;
    default:
      g_assert_not_reached();
      break;
    }
#undef primary_spacing
#undef secondary_spacing

  y = allocation->y + (allocation->height - child_height) / 2;

  children = GTK_BOX (widget)->children;
  i = 0;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
          GtkAllocation child_allocation;
          gint child_width = children_widths[i++];

	  child_allocation.width = child_width;
	  child_allocation.height = child_height;
	  child_allocation.y = y;

	  if (child->is_secondary)
	    {
	      child_allocation.x = secondary_x;
	      secondary_x += child_width + childspacing;
	    }
	  else
	    {
	      child_allocation.x = x;
	      x += child_width + childspacing;
	    }

	  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
	    child_allocation.x = (allocation->x + allocation->width) - (child_allocation.x + child_width - allocation->x);

	  gtk_widget_size_allocate (child->widget, &child_allocation);
	}
    }

  g_slice_free1 (sizeof (gint) * nvis_children, children_widths);
}

#else
static void
gtk_hbutton_box_size_allocate (GtkWidget     *widget,
			       GtkAllocation *allocation)
{
  GtkBox *base_box;
  GtkButtonBox *box;
  GtkBoxChild *child;
  GList *children;
  GtkAllocation child_allocation;
  gint nvis_children;
  gint n_secondaries;
  gint child_width;
  gint child_height;
  gint x = 0;
  gint secondary_x = 0;
  gint y = 0;
  gint width;
  gint childspace;
  gint childspacing = 0;
  GtkButtonBoxStyle layout;
  gint spacing;
  
  base_box = GTK_BOX (widget);
  box = GTK_BUTTON_BOX (widget);
  spacing = base_box->spacing;
  layout = box->layout_style != GTK_BUTTONBOX_DEFAULT_STYLE
	  ? box->layout_style : default_layout_style;
  _gtk_button_box_child_requisition (widget,
                                     &nvis_children,
				     &n_secondaries,
                                     &child_width,
                                     &child_height);
  widget->allocation = *allocation;
  width = allocation->width - GTK_CONTAINER (box)->border_width*2;
  switch (layout)
  {
  case GTK_BUTTONBOX_SPREAD:
    childspacing = (width - (nvis_children * child_width)) / (nvis_children + 1);
    x = allocation->x + GTK_CONTAINER (box)->border_width + childspacing;
    secondary_x = x + ((nvis_children - n_secondaries) * (child_width + childspacing));
    break;
  case GTK_BUTTONBOX_EDGE:
    if (nvis_children >= 2)
      {
	childspacing = (width - (nvis_children * child_width)) / (nvis_children - 1);
	x = allocation->x + GTK_CONTAINER (box)->border_width;
	secondary_x = x + ((nvis_children - n_secondaries) * (child_width + childspacing));
      }
    else
      {
	/* one or zero children, just center */
        childspacing = width;
	x = secondary_x = allocation->x + (allocation->width - child_width) / 2;
      }
    break;
  case GTK_BUTTONBOX_START:
    childspacing = spacing;
    x = allocation->x + GTK_CONTAINER (box)->border_width;
    secondary_x = allocation->x + allocation->width 
      - child_width * n_secondaries
      - spacing * (n_secondaries - 1)
      - GTK_CONTAINER (box)->border_width;
    break;
  case GTK_BUTTONBOX_END:
    childspacing = spacing;
    x = allocation->x + allocation->width 
      - child_width * (nvis_children - n_secondaries)
      - spacing * (nvis_children - n_secondaries - 1)
      - GTK_CONTAINER (box)->border_width;
    secondary_x = allocation->x + GTK_CONTAINER (box)->border_width;
    break;
  case GTK_BUTTONBOX_CENTER:
    childspacing = spacing;
    x = allocation->x + 
      (allocation->width
       - (child_width * (nvis_children - n_secondaries)
	  + spacing * (nvis_children - n_secondaries - 1)))/2
      + (n_secondaries * child_width + n_secondaries * spacing)/2;
    secondary_x = allocation->x + GTK_CONTAINER (box)->border_width;
    break;
  default:
    g_assert_not_reached();
    break;
  }

		  
  y = allocation->y + (allocation->height - child_height) / 2;
  childspace = child_width + childspacing;

  children = GTK_BOX (box)->children;
	  
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  child_allocation.width = child_width;
	  child_allocation.height = child_height;
	  child_allocation.y = y;
	  
	  if (child->is_secondary)
	    {
	      child_allocation.x = secondary_x;
	      secondary_x += childspace;
	    }
	  else
	    {
	      child_allocation.x = x;
	      x += childspace;
	    }

	  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
	    child_allocation.x = (allocation->x + allocation->width) - (child_allocation.x + child_width - allocation->x);
	  
	  gtk_widget_size_allocate (child->widget, &child_allocation);
	}
    }
}
#endif
  
#define __GTK_HBUTTON_BOX_C__
#include "gtkaliasdef.c"
