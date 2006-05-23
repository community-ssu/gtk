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

/* Modified by Nokia Corporation - 2005.
 * 
 */

/* Hildon : Button spacing according to the spec. */
#define HILDON_BUTTON_SPACING 5
/* Selecting hetefogenous mode for a childlayout */
#define GTK_BUTTONBOX_HETEROGENOUS 6

#include <config.h>
#include "gtkhbbox.h"
/* Hildon : We need this to deal with buttons graphics. */
#include "gtkbutton.h"
#include "gtkintl.h"
#include "gtkprivate.h"
#include "gtkalias.h"

static void gtk_hbutton_box_class_init    (GtkHButtonBoxClass   *klass);
static void gtk_hbutton_box_init          (GtkHButtonBox        *box);
static void gtk_hbutton_box_size_request  (GtkWidget      *widget,
					   GtkRequisition *requisition);
static void gtk_hbutton_box_size_allocate (GtkWidget      *widget,
					   GtkAllocation  *allocation);

static void osso_gtk_hbutton_child_showhide_handler (GtkWidget *widget,
						     gpointer user_data);
static void osso_gtk_hbutton_box_remove_child_signal_handlers (GtkHButtonBox *hbbox,
							       GtkWidget *removed_widget,
							       gpointer data);
static void osso_gtk_hbutton_box_find_button_detail (GtkHButtonBox *hbbox,
					      GtkWidget *addremovewidget,
					      gpointer data);

static gint default_spacing = 30;
static gint default_layout_style = GTK_BUTTONBOX_EDGE;

GType
gtk_hbutton_box_get_type (void)
{
  static GType hbutton_box_type = 0;

  if (!hbutton_box_type)
    {
      static const GTypeInfo hbutton_box_info =
      {
	sizeof (GtkHButtonBoxClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_hbutton_box_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkHButtonBox),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gtk_hbutton_box_init,
      };

      hbutton_box_type =
	g_type_register_static (GTK_TYPE_BUTTON_BOX, "GtkHButtonBox",
				&hbutton_box_info, 0);
    }

  return hbutton_box_type;
}

static void
gtk_hbutton_box_class_init (GtkHButtonBoxClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass*) class;

  widget_class->size_request = gtk_hbutton_box_size_request;
  widget_class->size_allocate = gtk_hbutton_box_size_allocate;

  /**
   * GtkHButtonBox:hildonlike:
   *
   * Currently this style property does not have any effect.
   * 
   * Since: maemo 1.0
   */
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean 
					   ( "hildonlike",
					     P_("hildonlike looks"),
					     P_("Name buttons, 1/0"),
					     FALSE,
					     GTK_PARAM_READABLE) );
}

static void
gtk_hbutton_box_init (GtkHButtonBox *hbutton_box)
{
  /* button_box_init has done everything allready */
}

GtkWidget*
gtk_hbutton_box_new (void)
{
  GtkHButtonBox *hbutton_box;

  hbutton_box = g_object_new (GTK_TYPE_HBUTTON_BOX, NULL);

  /* Attach signal handler for 'hildonizing' buttons.
   * gtk_hbutton_box_hildonize_buttons will check the name
   * and if it is something we're interested in i.e.
   *
   *   hildon_dialogbuttons
   *   hildon_viewbuttons
   *
   * it will do the hildonizing
   */
  g_signal_connect_after (G_OBJECT (hbutton_box), "remove",
		    G_CALLBACK (osso_gtk_hbutton_box_remove_child_signal_handlers),
		    NULL);
  g_signal_connect_after( G_OBJECT( hbutton_box ), "add",
		  G_CALLBACK( osso_gtk_hbutton_box_find_button_detail ),
		  NULL );
  g_signal_connect_after( G_OBJECT( hbutton_box ), "remove",
		  G_CALLBACK( osso_gtk_hbutton_box_find_button_detail ),
		  NULL );
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
		    layout <= GTK_BUTTONBOX_END);

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
  gint child_xpad=0;
  GtkBoxChild *child_req;
  GList *children_req;
  GtkRequisition treq;

  
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

  /* should GTK_BUTTONBOX_HETEROGENOUS add into the GtkButtonBoxStyle 
     enum struct 
  */ 
   if( !box->homogeneous )
     layout = GTK_BUTTONBOX_HETEROGENOUS; 

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
      requisition->width = nvis_children*child_width + ((nvis_children-1)*spacing);
      break;
    case GTK_BUTTONBOX_HETEROGENOUS:
      children_req = GTK_BOX (box)->children;
      child_req = children_req->data;
      requisition->width = 0; 

      while (children_req)
	{ 
	  child_req = children_req->data;
	  children_req = children_req->next;

	  if (GTK_WIDGET_VISIBLE (child_req->widget))
	    {
	      gtk_widget_get_child_requisition( child_req->widget, 
						&(treq) );   	      
	      requisition->width += treq.width;
	 
              gtk_widget_style_get(widget, 
				   "child-internal-pad-x", 
				   &(child_xpad), NULL);
	      requisition->width  += (child_xpad*2);	      
	    }
      	}
      requisition->width += ((nvis_children-1)*spacing);

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



static void
gtk_hbutton_box_size_allocate (GtkWidget     *widget,
			       GtkAllocation *allocation)
{
  GtkBox *base_box;
  GtkButtonBox *box;
  GtkHButtonBox *hbox;
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
  hbox = GTK_HBUTTON_BOX (widget);
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

  if( !base_box->homogeneous )
     layout = GTK_BUTTONBOX_HETEROGENOUS;

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
  case GTK_BUTTONBOX_HETEROGENOUS:
    {
      gint sumwidth = 0;
      GtkBoxChild *child_h;
      GList *children_h = GTK_BOX (box)->children;
        /* heterogenous sized childs onto center */
      childspacing = spacing;

      while (children_h )
	{
	  child_h = children_h->data;
	  children_h = children_h->next;

	  if (GTK_WIDGET_VISIBLE (child_h->widget))
	    {
	      gint child_xpad = 0;
	      GtkRequisition treq;
	      gtk_widget_get_child_requisition( child_h->widget, &(treq) );  
	      sumwidth += treq.width;

	      gtk_widget_style_get(widget, 
				   "child-internal-pad-x", 
				   &(child_xpad), NULL); 
	      sumwidth += (child_xpad*2);
	    }
	}
      x = allocation->x + 
       ( (allocation->width - sumwidth - (spacing * (nvis_children - 1)))/2 );
      /* secondary property will be ignored below */
      break; 
    }
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
	  child_allocation.height = child_height;
	  child_allocation.y = y;
	  
	  if(layout != GTK_BUTTONBOX_HETEROGENOUS)
	    {
	      child_allocation.width = child_width; 
	    }
	  else /* if layout will be hetergenous */
	    {
	      gint child_hwidth = 0;
	      GtkRequisition treq;
	      gint child_xpad = 0;

	      gtk_widget_get_child_requisition( child->widget, &(treq) );
	      child_hwidth += treq.width;      

	      gtk_widget_style_get(widget, 
				   "child-internal-pad-x", 
				   &child_xpad, NULL);
	      child_hwidth += (child_xpad*2);
	         
	      child_allocation.width =  child_hwidth;
	      childspace = child_hwidth + childspacing;

	    }
	  
	  /* calculate the horizontal location */
	  if (child->is_secondary && layout != GTK_BUTTONBOX_HETEROGENOUS)
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

/* Function to wrap "hide" and "show" signals to call
 * osso_gtk_hbutton_box_find_button_detail -function.*/
static void osso_gtk_hbutton_child_showhide_handler (GtkWidget *widget,
						     gpointer user_data)
{
  osso_gtk_hbutton_box_find_button_detail (GTK_HBUTTON_BOX (widget), GTK_WIDGET (user_data), NULL);
}
  
/* Function to remove "show"&"hide" signal handlers
 * from a child when it's removed. */
static void osso_gtk_hbutton_box_remove_child_signal_handlers (GtkHButtonBox *hbbox,
							       GtkWidget *removed_widget,
							       gpointer data)
{
  g_signal_handlers_disconnect_by_func (G_OBJECT (removed_widget), osso_gtk_hbutton_box_find_button_detail, hbbox);
}

/* Signal handler called when we have to set
 * painting detail values for buttons in this 
 * gtk_horizontal_button_box.
 */  
static void osso_gtk_hbutton_box_find_button_detail (GtkHButtonBox *hbbox,
					      GtkWidget *addremovewidget,
					      gpointer data)
{
  GList *child;
  gint visible_buttons = 0;
  gint secondary_buttons = 0;
  GtkWidget *leftmost_button = NULL;
  GtkWidget *rightmost_button = NULL;

  for( child = GTK_BOX (hbbox)->children ; child ; child = child->next ) 
    {
      GtkBoxChild *box_child = child->data;
      GtkWidget *child_widget = box_child->widget;
      gulong signal_handler = g_signal_handler_find ( G_OBJECT( child_widget ),
						      G_SIGNAL_MATCH_FUNC,
						      0, 0, NULL,
						      G_CALLBACK (osso_gtk_hbutton_child_showhide_handler),
						      NULL);

      if (signal_handler == 0)
	{
	  g_signal_connect_object ( G_OBJECT( child_widget ), 
				    "hide", 
				    G_CALLBACK ( osso_gtk_hbutton_child_showhide_handler ), 
				    hbbox, G_CONNECT_SWAPPED);
	  g_signal_connect_object ( G_OBJECT( child_widget ), 
				    "show", 
				    G_CALLBACK ( osso_gtk_hbutton_child_showhide_handler ), 
				    hbbox, G_CONNECT_SWAPPED);
	}

      if ((GTK_WIDGET_VISIBLE (child_widget)) &&
         (GTK_IS_BUTTON (child_widget)))
	visible_buttons++;
      else
	continue;

      if (leftmost_button == NULL)
	leftmost_button = child_widget;

      if (secondary_buttons == 0)
	rightmost_button = child_widget;
      else 
	if (box_child->is_secondary)
	  {
	    rightmost_button = child_widget;
	    secondary_buttons++;
	  }

      if (box_child->is_secondary) 
        rightmost_button = child_widget;
    }

  if (visible_buttons == 0)
    return;

  for( child = GTK_BOX (hbbox)->children ; child ; child = child->next ) 
    {
      GtkBoxChild *box_child = child->data;
      GtkWidget *child_widget = box_child->widget;
      OssoGtkButtonAttachFlags attachflags = OSSO_GTK_BUTTON_ATTACH_NORTH | OSSO_GTK_BUTTON_ATTACH_SOUTH;
      gboolean automatic_detail;

      if (!((GTK_WIDGET_VISIBLE (child_widget)) &&
	  (GTK_IS_BUTTON (child_widget))))
	continue;

      if (child_widget == leftmost_button)
	attachflags |= OSSO_GTK_BUTTON_ATTACH_WEST;

      if (child_widget == rightmost_button)
	attachflags |= OSSO_GTK_BUTTON_ATTACH_EAST;

      g_object_get (G_OBJECT (child_widget), "automatic_detail", &automatic_detail, NULL);
      if (automatic_detail == TRUE)
        g_object_set (G_OBJECT (child_widget), "detail", osso_gtk_button_attach_details[attachflags], NULL);
    }
}

#define __GTK_HBUTTON_BOX_C__
#include "gtkaliasdef.c"
