/*
 * This file is part of hildon-lgpl
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

/**
 * @file hildon-caption-control.c
 *
 * This file contains the implementation of Hildon Caption Control
 *
 */


#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkcomboboxentry.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkalignment.h>
#include <stdio.h>
#include <string.h>
#include <hildon-caption.h>
#include <hildon-lgpl/hildon-defines.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <libintl.h>
#define _(String) dgettext(PACKAGE, String)

#define HILDON_CAPTION_MANDATORY_ICON "qgn_list_gene_mandat_field"
#define HILDON_CAPTION_SPACING 6

#define HILDON_CAPTION_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
     HILDON_TYPE_CAPTION, HildonCaptionPrivate));

/*our parent class*/
static GtkEventBox *parent_class = NULL;

typedef struct _HildonCaptionPrivate HildonCaptionPrivate;

enum
{
  PROP_NONE,
  PROP_LABEL,
  PROP_ICON,
  PROP_STATUS,
  PROP_SEPARATOR,
  PROP_SIZE_GROUP
};

enum
{
  CHILD_PROP_NONE,
  CHILD_PROP_EXPAND
};

/*Init functions*/
static void hildon_caption_class_init( HildonCaptionClass *caption_class );
static void hildon_caption_init( HildonCaption *caption );
static void hildon_caption_size_request( GtkWidget *widget,
                                         GtkRequisition *requisition );
static void hildon_caption_size_allocate( GtkWidget *widget,
                                          GtkAllocation *allocation );
static void hildon_caption_forall( GtkContainer *container,
                                   gboolean include_internals,
                                   GtkCallback callback, gpointer data );
static void hildon_caption_hierarchy_changed( GtkWidget *widget,
                                              GtkWidget *previous_toplevel);
static void hildon_caption_set_focus( GtkWindow *window, GtkWidget *widget,
                                      GtkWidget *caption );
static void hildon_caption_activate( GtkWidget *widget );

static void hildon_caption_set_property( GObject *object, guint param_id,
			                                 const GValue *value, GParamSpec *pspec );
static void hildon_caption_get_property( GObject *object, guint param_id,
			                                   GValue *value, GParamSpec *pspec );
static gboolean hildon_caption_expose( GtkWidget *widget,
                                       GdkEventExpose *event );
static void hildon_caption_destroy( GtkObject *self );
static gboolean hildon_caption_button_press( GtkWidget *widget, GdkEventButton *event );
static void
hildon_caption_set_focus_child( GtkContainer *container, GtkWidget *child );

static void
hildon_caption_set_label_text( HildonCaptionPrivate *priv );

static void hildon_caption_set_child_property (GtkContainer    *container,
					       GtkWidget       *child,
					       guint            property_id,
					       const GValue    *value,
					       GParamSpec      *pspec);
static void hildon_caption_get_child_property (GtkContainer    *container,
					       GtkWidget       *child,
					       guint            property_id,
					       GValue          *value,
					       GParamSpec      *pspec);
static void get_first_focusable_child ( GtkWidget *widget, gpointer data );

struct _HildonCaptionPrivate
{
  GtkWidget *caption_area;
  GtkWidget *mandatory_icon;
  GtkWidget *label;
  GtkWidget *icon;
  GtkWidget *icon_align; /* Arbitrary icon widgets do not support alignment */
  GtkSizeGroup *group;
  gchar *text;
  gchar *separator;
  guint is_focused : 1;
  guint activate_block : 1;
  guint expand : 1;
  HildonCaptionStatus status;
};

G_CONST_RETURN GType
hildon_caption_status_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { HILDON_CAPTION_OPTIONAL, "HILDON_CAPTION_OPTIONAL", "optional" },
      { HILDON_CAPTION_MANDATORY, "HILDON_CAPTION_MANDATORY", "mandatory" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("HildonCaptionStatus", values);
  }
  return etype;
}

/**
 * hildon_caption_get_type:
 * @Returns : GType of #HildonCaption.
 *
 * Initialises, and returns the type of a hildon caption.
 */
G_CONST_RETURN GType hildon_caption_get_type( void )
{
  static GType caption_type = 0;

  if (!caption_type)
  {
    static const GTypeInfo caption_info = {
        sizeof(HildonCaptionClass),
               NULL,       /* base_init */
               NULL,       /* base_finalize */
               (GClassInitFunc) hildon_caption_class_init,
               NULL,       /* class_finalize */
               NULL,       /* class_data */
               sizeof(HildonCaption),
               0,  /* n_preallocs */
               (GInstanceInitFunc) hildon_caption_init,
    };
    caption_type = g_type_register_static( GTK_TYPE_EVENT_BOX,
                                           "HildonCaption", &caption_info, 0 );
  }
  return caption_type;
}

/*
 * Initialises the caption class.
 */
static void hildon_caption_class_init( HildonCaptionClass *caption_class )
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(caption_class);
  GObjectClass *gobject_class = G_OBJECT_CLASS(caption_class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS(caption_class);
  
  parent_class = g_type_class_peek_parent( caption_class );

  g_type_class_add_private( caption_class, sizeof(HildonCaptionPrivate) );

  gobject_class->get_property = hildon_caption_get_property;
  gobject_class->set_property = hildon_caption_set_property;
  
  caption_class->activate = hildon_caption_activate;

  GTK_OBJECT_CLASS(caption_class)->destroy = hildon_caption_destroy;
  container_class->forall = hildon_caption_forall;
  container_class->set_focus_child = hildon_caption_set_focus_child;
  container_class->set_child_property = hildon_caption_set_child_property;
  container_class->get_child_property = hildon_caption_get_child_property;

  widget_class->expose_event = hildon_caption_expose;
  widget_class->hierarchy_changed = hildon_caption_hierarchy_changed;
  widget_class->size_request = hildon_caption_size_request;
  widget_class->size_allocate = hildon_caption_size_allocate;
  widget_class->button_press_event = hildon_caption_button_press;

  widget_class->activate_signal = g_signal_new( "activate",
                                                G_OBJECT_CLASS_TYPE(
                                                gobject_class),
                                                G_SIGNAL_RUN_FIRST |
                                                G_SIGNAL_ACTION,
                                                G_STRUCT_OFFSET( HildonCaptionClass,
                                                activate), NULL, NULL,
                                                gtk_marshal_VOID__VOID,
                                                G_TYPE_NONE, 0);

  /**
   * HildonCaption:label:
   *
   * Caption label.
   */
  g_object_class_install_property( gobject_class, PROP_LABEL,
                                   g_param_spec_string("label",
                                   "Current label", "Caption label",
                                   NULL, G_PARAM_READABLE | G_PARAM_WRITABLE) );

  /**
   * HildonCaption:icon:
   *
   * The icon shown on the caption area.
   */
  g_object_class_install_property( gobject_class, PROP_ICON,
                                   g_param_spec_object("icon",
                                   "Current icon",
                                   "The icon shown on the caption area",
                                   GTK_TYPE_WIDGET, G_PARAM_READABLE | 
                                   G_PARAM_WRITABLE) );
  /**
   * HildonCaption:status:
   *
   * Mandatory or optional status.
   */
  g_object_class_install_property( gobject_class, PROP_STATUS,
				   g_param_spec_enum("status",
				   "Current status",
				   "Mandatory or optional status",
				   HILDON_TYPE_CAPTION_STATUS,
				   HILDON_CAPTION_OPTIONAL,
				   G_PARAM_READABLE | G_PARAM_WRITABLE) );
  /**
   * HildonCaption:size_group:
   *
   * Current size group the caption is in.
   */
  g_object_class_install_property( gobject_class, PROP_SIZE_GROUP,
                                   g_param_spec_object("size_group",
                                   "Current size group",
                                   "Current size group the caption is in",
                                   GTK_TYPE_SIZE_GROUP, G_PARAM_READABLE | 
                                   G_PARAM_WRITABLE) );
  
  /**
   * HildonCaption:separator:
   *
   * The current separator.
   */
  g_object_class_install_property( gobject_class, PROP_SEPARATOR,
                                   g_param_spec_string("separator",
                                   "Current separator", "Current separator",
                                   _("Ecdg_ti_caption_separator"),
                                   G_PARAM_READABLE | G_PARAM_WRITABLE) );

  gtk_container_class_install_child_property (container_class,
				CHILD_PROP_EXPAND,
       				g_param_spec_boolean ("expand",
       				"Same as GtkBox expand.",
       				"Same as GtkBox expand. Wheter the child should be expanded or not.",
       				FALSE,
       				G_PARAM_READWRITE));
}

static void hildon_caption_destroy( GtkObject *self )
{
  HildonCaptionPrivate *priv = HILDON_CAPTION_GET_PRIVATE(self);

  if( priv->caption_area )
  {
    gtk_widget_unparent( priv->caption_area );
    priv->caption_area = NULL;
  }

  if( priv->text )
  {
    g_free( priv->text );
    priv->text = NULL;
  }

  if( priv->separator )
  {
    g_free( priv->separator );
    priv->separator = NULL;
  }

  if( GTK_OBJECT_CLASS(parent_class)->destroy )
    GTK_OBJECT_CLASS(parent_class)->destroy( self );
}

/* Why we have this huge expose?
 * We do have own window.. Next good question is why we have that window?
 * In allocation, we can not ask parent to do it's allocation, why?
 *
 * Parent, eventbox will run allocate also for the child which may be
 * owning a window too. This window is then moved and resized and we do not
 * want to do that -> It causes flickering.
 * And just because we also want to
 */
static gboolean hildon_caption_expose( GtkWidget *widget,
                                       GdkEventExpose *event )
{
  HildonCaptionPrivate *priv = NULL;
  GtkRequisition req;
  GtkAllocation alloc;
  gfloat align;

  g_return_val_if_fail( HILDON_IS_CAPTION(widget), TRUE );
  priv = HILDON_CAPTION_GET_PRIVATE(widget);

  if( !GTK_WIDGET_DRAWABLE(widget) )
    return FALSE;

  GTK_WIDGET_CLASS(parent_class)->expose_event( widget, event );
  
  if ( priv->is_focused )
  {
    gtk_widget_get_child_requisition( priv->caption_area, &req );
    align = hildon_caption_get_label_alignment(HILDON_CAPTION(widget));

    alloc.width = priv->caption_area->allocation.width + HILDON_CAPTION_SPACING;
    alloc.height = MIN (req.height + (2 * widget->style->ythickness), priv->caption_area->allocation.height);
    alloc.x = priv->caption_area->allocation.x;
    alloc.y = priv->caption_area->allocation.y + 
              MAX(((priv->caption_area->allocation.height - alloc.height) * align), 0);

    gtk_paint_box( widget->style, widget->window, GTK_STATE_ACTIVE,
                   GTK_SHADOW_OUT, NULL, widget, "selection",
                   alloc.x, alloc.y, alloc.width, alloc.height );

    GTK_WIDGET_GET_CLASS(priv->caption_area)->expose_event(
         priv->caption_area, event);
  }
  
  return FALSE;
}

static void hildon_caption_set_property( GObject *object, guint param_id,
			                                   const GValue *value,
                                         GParamSpec *pspec )
{
  HildonCaptionPrivate *priv = HILDON_CAPTION_GET_PRIVATE(object);

  switch( param_id ) 
  {
    case PROP_LABEL:
      if( priv->text )
      {
        g_free( priv->text );
        priv->text = NULL;
      }

      if( value )
      {
        priv->text = g_strdup( g_value_get_string(value) );
        hildon_caption_set_label_text( priv );
      }
      gtk_widget_queue_resize( GTK_WIDGET(object) );
      break;
      
    case PROP_ICON:
      if( priv->icon )
        gtk_container_remove( GTK_CONTAINER(priv->icon_align), priv->icon );

      priv->icon = g_value_get_object( value );
      if( priv->icon )
      {
        gtk_container_add(GTK_CONTAINER(priv->icon_align), priv->icon);
        gtk_widget_show_all( priv->caption_area );
      }
      break;

    case PROP_STATUS:
      priv->status = g_value_get_enum( value );

      if( priv->status == HILDON_CAPTION_MANDATORY )
      {
        if( !priv->mandatory_icon )
        {
          gfloat align;
          priv->mandatory_icon = gtk_image_new_from_icon_name(
                                                HILDON_CAPTION_MANDATORY_ICON,
                                                HILDON_ICON_SIZE_NOTE );

          align = hildon_caption_get_label_alignment(HILDON_CAPTION(object));
          g_object_set(priv->mandatory_icon, "yalign", align, NULL);

          if( priv->mandatory_icon )
          {
            gtk_box_pack_end( GTK_BOX(priv->caption_area),
                                priv->mandatory_icon, FALSE, FALSE, 0 );
            gtk_widget_show_all( priv->caption_area );
          }
        }
      }
      else
      {
        if( priv->mandatory_icon )
        {
          gtk_container_remove( GTK_CONTAINER(priv->caption_area),
                                priv->mandatory_icon );
          priv->mandatory_icon = NULL;
          gtk_widget_queue_draw( GTK_WIDGET(object) );
        }
      }
      break;

    case PROP_SIZE_GROUP:
      if( priv->group )
        gtk_size_group_remove_widget( priv->group, priv->caption_area );
      
      priv->group = g_value_get_object( value );
      
      if( priv->group )
        gtk_size_group_add_widget( priv->group, priv->caption_area );
      gtk_widget_queue_draw( GTK_WIDGET(object) );
      break;
    
    case PROP_SEPARATOR:
      if( priv->separator )
      {
        g_free( priv->separator );
        priv->separator = NULL;
      }

      if( value )
      {
        priv->separator = g_strdup( g_value_get_string(value) );
        hildon_caption_set_label_text( priv );
      }
          
      gtk_widget_queue_resize( GTK_WIDGET(object) );
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
  }
}

static void hildon_caption_get_property( GObject *object, guint param_id,
			                                   GValue *value, GParamSpec *pspec )
{
  HildonCaptionPrivate *priv = HILDON_CAPTION_GET_PRIVATE(object);

  switch (param_id) 
    {
    case PROP_LABEL:
      g_value_set_string( value, priv->text );
      break;
    case PROP_ICON:
      g_value_set_object( value, priv->icon );
      break;
    case PROP_STATUS:
      g_value_set_enum( value, priv->status );
      break;
    case PROP_SIZE_GROUP:
      g_value_set_object( value, priv->group );
      break;
    case PROP_SEPARATOR:
      g_value_set_string( value, priv->separator );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }
}

static void hildon_caption_set_child_property( GtkContainer    *container,
					      GtkWidget       *child,
					      guint            property_id,
					      const GValue    *value,
					      GParamSpec      *pspec )
{
  switch (property_id)
    {
    case CHILD_PROP_EXPAND:
      hildon_caption_set_child_expand( HILDON_CAPTION(container),
				      g_value_get_boolean(value) );
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(container, property_id, pspec);
      break;
    }
}

static void hildon_caption_get_child_property( GtkContainer    *container,
					      GtkWidget       *child,
					      guint            property_id,
					      GValue          *value,
					      GParamSpec      *pspec )
{
  switch (property_id)
    {
    case CHILD_PROP_EXPAND:
      g_value_set_boolean( value, hildon_caption_get_child_expand(
                           HILDON_CAPTION(container)) );
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(container, property_id, pspec);
      break;
    }
}

static gboolean hildon_caption_button_press( GtkWidget *widget, 
					     GdkEventButton *event )
{
  HildonCaptionPrivate *priv = HILDON_CAPTION_GET_PRIVATE(widget);
  /* When a label is the child widget, it calls parents mnemonic_activate.
   * We set is_focused to TRUE before we call mnemonic_activate to the label.
   * When the label calls parents mnemonic_activate, captions activate will be
   * run.
   * We set is_focused to FALSE there and that is the reason why this function
   * is not wrote in the following way.
   *
   * if( gtk_widget_mnemonic_activate( GTK_BIN(widget)->child, FALSE ) )
   *   priv->is_focused = TRUE;
   */
  if( GTK_WIDGET_CAN_FOCUS(GTK_BIN(widget)->child) )
  {
    priv->is_focused = TRUE;
    gtk_widget_grab_focus( GTK_BIN(widget)->child );
  }
  else if ( GTK_IS_CONTAINER(GTK_BIN(widget)->child) )
  {
    GtkWidget *cwid= NULL;
    /* go through the children of the container for the first focusable children */
    gtk_container_forall (GTK_CONTAINER(GTK_BIN(widget)->child), (GtkCallback) get_first_focusable_child, &cwid );
    if (cwid)
    {
       priv->is_focused = TRUE;
       gtk_widget_grab_focus( GTK_WIDGET(cwid) );
    }
  }    
  return FALSE;
}

static void get_first_focusable_child ( GtkWidget *widget, gpointer data )
{
  GtkWidget **child = (GtkWidget**)data;
  /* if a first child has already been found then do nothing */
  if (*child) return;

  if (GTK_WIDGET_CAN_FOCUS (widget)) *child = widget;  
  else if (GTK_IS_CONTAINER(widget)) /* if the child is a container itself then go through them also /including/ internals */  
  {
     gtk_container_forall (GTK_CONTAINER(widget), get_first_focusable_child, child);
  }
}

static void hildon_caption_init( HildonCaption *caption )
{
  HildonCaptionPrivate *priv = NULL;
  g_return_if_fail( HILDON_IS_CAPTION(caption) );
  priv = HILDON_CAPTION_GET_PRIVATE(caption);

  priv->status = HILDON_CAPTION_OPTIONAL;
  priv->icon = NULL;
  priv->group = NULL;
  priv->is_focused = FALSE;

  priv->separator = g_strdup(_("Ecdg_ti_caption_separator"));

  gtk_widget_push_composite_child();
  
  priv->caption_area = gtk_hbox_new( FALSE, HILDON_CAPTION_SPACING ); 
  priv->label = gtk_label_new( NULL );
  priv->icon_align = gtk_alignment_new(0.5f, 0.5f, 0.0f, 0.0f);

  gtk_event_box_set_above_child( GTK_EVENT_BOX(caption), FALSE );
  gtk_widget_add_events( GTK_WIDGET(caption), GDK_BUTTON_PRESS_MASK );

  gtk_box_pack_end( GTK_BOX(priv->caption_area), priv->icon_align, FALSE, FALSE, 0);
  gtk_box_pack_end( GTK_BOX(priv->caption_area), priv->label, FALSE, FALSE, 0 );
  gtk_widget_set_parent( priv->caption_area, GTK_WIDGET(caption) );

  gtk_widget_pop_composite_child();

  gtk_widget_show_all( priv->caption_area );
}

static void hildon_caption_set_focus( GtkWindow *window, GtkWidget *widget,
                                      GtkWidget *caption )
{
  GtkWidget *win = GTK_WIDGET(window);
  GtkWidget *tmp = widget;
  HildonCaptionPrivate *priv = HILDON_CAPTION_GET_PRIVATE(caption);

  while( GTK_IS_WIDGET(tmp) && win != tmp )
  {  
    if( tmp == caption )
    {
      priv->is_focused = TRUE;
      gtk_widget_queue_draw( caption );
      return;
    }
    tmp = gtk_widget_get_parent( tmp );
  }
  if( priv->is_focused == TRUE )
  {
    priv->is_focused = FALSE;
    gtk_widget_queue_draw( caption );
  }
}

static void
hildon_caption_set_focus_child( GtkContainer *container, GtkWidget *child )
{
  GtkWidget *parent = gtk_widget_get_parent( GTK_WIDGET(container) );
  
  GTK_CONTAINER_CLASS(parent_class)->set_focus_child( container, child );

  if( parent && child != container->focus_child )
    gtk_container_set_focus_child( GTK_CONTAINER(parent),
                                   GTK_WIDGET(container) );
}

static void hildon_caption_hierarchy_changed( GtkWidget *widget,
                                              GtkWidget *previous_toplevel)
{
  GtkWidget *current_ancestor;
  HildonCaptionPrivate *priv;

  g_return_if_fail( HILDON_IS_CAPTION(widget) );
  priv = HILDON_CAPTION_GET_PRIVATE(widget);

  if( GTK_WIDGET_CLASS(parent_class)->hierarchy_changed )
    GTK_WIDGET_CLASS(parent_class)->hierarchy_changed( widget,
                                                       previous_toplevel );
  if (previous_toplevel) {
  /* This is a compilation workaround for gcc > 3.3 since glib is buggy */
  /* see http://bugzilla.gnome.org/show_bug.cgi?id=310175 */
#ifdef __GNUC__
  __extension__
#endif
    g_signal_handlers_disconnect_by_func
      (previous_toplevel, (gpointer) hildon_caption_set_focus, widget);
  }
  current_ancestor = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);

  if (current_ancestor)
    g_signal_connect( current_ancestor, "set-focus", 
      G_CALLBACK(hildon_caption_set_focus), widget );
}

static void hildon_caption_size_request( GtkWidget *widget,
                                         GtkRequisition *requisition )
{
  GtkRequisition req;
  HildonCaptionPrivate *priv = NULL;
  g_return_if_fail( HILDON_IS_CAPTION(widget) );
  priv = HILDON_CAPTION_GET_PRIVATE(widget);

  gtk_widget_size_request( priv->caption_area, &req );

  if( GTK_WIDGET_CLASS(parent_class)->size_request )
    GTK_WIDGET_CLASS(parent_class)->size_request( widget, requisition );

  requisition->width += req.width + HILDON_CAPTION_SPACING * 3;

  if( (req.height + (2 * widget->style->ythickness)) > requisition->height )
   requisition->height = req.height + (2 * widget->style->ythickness);
}

/* We use HILDON_CAPTION_SPACING to make it look a bit nicer */
static void hildon_caption_size_allocate( GtkWidget *widget,
                                          GtkAllocation *allocation )
{
  GtkAllocation allocA;
  GtkAllocation allocB;
  GtkRequisition req;
  GtkWidget *child = NULL;
  HildonCaptionPrivate *priv = NULL;
  g_return_if_fail( HILDON_IS_CAPTION(widget) );
  priv = HILDON_CAPTION_GET_PRIVATE(widget);

  if( GTK_WIDGET_REALIZED(widget) )
    gdk_window_move_resize (widget->window,
				allocation->x + GTK_CONTAINER (widget)->border_width,
				allocation->y + GTK_CONTAINER (widget)->border_width,
				MAX (allocation->width - GTK_CONTAINER (widget)->border_width * 2, 0),
				MAX (allocation->height - GTK_CONTAINER (widget)->border_width * 2, 0));

  child = GTK_BIN(widget)->child;

  widget->allocation = *allocation;  
  gtk_widget_get_child_requisition( priv->caption_area, &req );

  allocA.height = allocB.height = allocation->height;
  allocA.width = allocB.width = allocation->width;
  allocA.x = allocB.x = allocB.y = allocA.y = 0;
  
  if( allocA.width > req.width + HILDON_CAPTION_SPACING )
  {
    allocA.x += req.width + HILDON_CAPTION_SPACING * 2;
    allocB.width = req.width;
  }

  allocA.width -= req.width + HILDON_CAPTION_SPACING * 2;

  if( !priv->expand && child && GTK_WIDGET_VISIBLE(child) )
  {
    GtkRequisition child_req;
    gtk_widget_get_child_requisition( child, &child_req );
    allocA.width = MIN( allocA.width, child_req.width );
    allocA.height = MIN( allocA.height, child_req.height );
  }

  if( allocA.width < 0 )
  {
    allocB.width = req.width + allocA.width;
    allocA.width = 0;
    allocB.width = MAX (allocB.width, 0);
  }

  allocA.height = MAX (allocA.height, 0);
  allocB.height = MAX (allocB.height, 0);

  if (child && GTK_WIDGET_VISIBLE(child) )
    gtk_widget_size_allocate( child, &allocA );
  gtk_widget_size_allocate( priv->caption_area, &allocB );
}

static void hildon_caption_forall( GtkContainer *container,
                                   gboolean include_internals,
                                   GtkCallback callback, gpointer data )
{
  HildonCaptionPrivate *priv = NULL;
  g_return_if_fail( HILDON_IS_CAPTION(container) );
  priv = HILDON_CAPTION_GET_PRIVATE(container);

  if( GTK_CONTAINER_CLASS(parent_class)->forall )
    GTK_CONTAINER_CLASS(parent_class)->forall( container, include_internals,
                                               callback, data );

  if( include_internals )
    (*callback)( priv->caption_area, data );
}


/**
 * hildon_caption_set_sizegroup:
 * @caption : A #HildonCaption
 * @new_group : A #GtkSizeGroup
 *
 * Sets a #GtkSizeGroup of a given captioned control.
 *
 * Deprecated: Use g_object_set, property "size_group". 
 */
#ifndef HILDON_DISABLE_DEPRECATED
void hildon_caption_set_sizegroup( const HildonCaption *self,
                                   GtkSizeGroup *group )
{
  g_object_set( G_OBJECT(self), "size_group", group, NULL );
}
#endif
/**
 * hildon_caption_get_sizegroup:
 * @caption : A #HildonCaption
 * @Returns : A #GtkSizeGroup
 *
 * Query given captioned control for the #GtkSizeGroup assigned to it.
 *
 * Deprecated: Use g_object_get, property "size_group". 
 */
#ifndef HILDON_DISABLE_DEPRECATED
GtkSizeGroup *hildon_caption_get_sizegroup( const HildonCaption *self )
{
  HildonCaptionPrivate *priv;
  g_return_val_if_fail( HILDON_IS_CAPTION (self), NULL );
  priv = HILDON_CAPTION_GET_PRIVATE(self);
  return priv->group;
}
#endif
/**
 * hildon_caption_new:
 * @group : a #GtkSizeGroup for controlling the size of related captions.
 *          Can be NULL.
 * @value : the caption text to accompany the text entry.  The widget makes
 *          a copy of this text.
 * @control : the control that is to be captioned
 * @icon : an icon to accompany the label - can be NULL in which case no
 *         icon is displayed
 * @flag : indicates whether this captioned control is mandatory or
 *         optional.
 * @returns : A #GtkWidget pointer of Caption
 * 
 * Creates a new instance of hildon_caption widget, with a specific
 * control and image.
 * Note: Clicking on a focused caption will trigger the activate signal.
 * The default behaviour for the caption's activate signal is to call	 
 * gtk_widget_activate on it's control.
 */
GtkWidget *hildon_caption_new( GtkSizeGroup *group, const gchar *value,
                               GtkWidget *control, GtkWidget *icon,
                               HildonCaptionStatus flag)
{
  GtkWidget *widget;
  g_return_val_if_fail( GTK_IS_WIDGET(control), NULL );
  
  widget = g_object_new( HILDON_TYPE_CAPTION, "label", value,
			"size_group", group, "icon", icon, "status", flag,
                        NULL );

  hildon_caption_set_child_expand( HILDON_CAPTION(widget), TRUE );
  gtk_container_add( GTK_CONTAINER(widget), control );


  return widget;
}

/**
 * hildon_caption_is_mandatory:
 * @caption : A #HildonCaption
 * @returns : is this captioned control a mandatory one?
 * 
 * Query #HildonCaption whether this captioned control is a mandatory one.
 *
 */

gboolean hildon_caption_is_mandatory( const HildonCaption *caption )
{
  HildonCaptionPrivate *priv;
  g_return_val_if_fail( HILDON_IS_CAPTION(caption), FALSE );
  priv = HILDON_CAPTION_GET_PRIVATE(caption);

  return priv->status == HILDON_CAPTION_MANDATORY;
}

/**
 * hildon_caption_set_status:
 * @caption : A #HildonCaption
 * @flag : one of the values from #HildonCaptionStatus
 *
 * Sets #HildonCaption status.
 *

 */

void hildon_caption_set_status( HildonCaption *caption,
                                HildonCaptionStatus flag )
{
  g_object_set( G_OBJECT(caption), "status", flag, NULL );
}

/**
 * hildon_caption_get_status:
 * @caption : A #HildonCaption
 * @returns : one of the values from #HildonCaptionStatus
 *
 * Gets #HildonCaption status.
 *
 */

HildonCaptionStatus hildon_caption_get_status( const HildonCaption *caption )
{
  HildonCaptionPrivate *priv;
  g_return_val_if_fail( HILDON_IS_CAPTION(caption), HILDON_CAPTION_OPTIONAL );
  priv = HILDON_CAPTION_GET_PRIVATE(caption);

  return priv->status;
}

/**
 * hildon_caption_set_icon_image:
 * @caption : A #HildonCaption
 * @icon : the #GtkImage to use as the icon. 
 *         calls gtk_widget_show on the icon if !GTK_WIDGET_VISIBLE(icon)
 *
 * Sets the icon to be used by this hildon_caption widget.
 *
 */

void hildon_caption_set_icon_image( HildonCaption *caption, GtkWidget *icon )
{
  g_object_set( G_OBJECT(caption), "icon", icon, NULL );
}

/**
 * hildon_caption_get_icon_image:
 * @caption : A #HildonCaption
 * @returns : the #GtkImage that is being used as the icon by the
 *            hildon_caption, or NULL if no icon is in use.
 *
 * Gets icon of #HildonCaption
 *
 */

GtkWidget *hildon_caption_get_icon_image( const HildonCaption *caption )
{
  HildonCaptionPrivate *priv;
  g_return_val_if_fail( HILDON_IS_CAPTION(caption), NULL );
  priv = HILDON_CAPTION_GET_PRIVATE(caption);

  return priv->icon;
}

/**
 * hildon_caption_set_label:
 * @caption : A #HildonCaption
 * @label : the text to use
 *
 * Sets the label text that appears before the control.  
 * Separator character is added to the end of the label string. By default
 * the separator is ":".
 *
 */

void hildon_caption_set_label( HildonCaption *caption, const gchar *label )
{
  g_object_set( G_OBJECT(caption), "label", label, NULL );
}

/**
 * hildon_caption_get_label:
 * @caption : A #HildonCaption
 * @returns : the text currently being used as the label of the caption
 * control. The string is owned by the label and the caller should never free or
 * modify this value.
 *
 * Gets label of #HildonCaption
 *
 */

gchar *hildon_caption_get_label( const HildonCaption *caption )
{
  HildonCaptionPrivate *priv;
  g_return_val_if_fail(HILDON_IS_CAPTION(caption), "");
  priv = HILDON_CAPTION_GET_PRIVATE(caption);

  return (gchar*)gtk_label_get_text(GTK_LABEL(GTK_LABEL(priv->label)));
}

/**
 * hildon_caption_set_separator:
 * @caption : A #HildonCaption
 * @separator : the separator to use
 *
 * Sets the separator character that appears after the label.  
 * The default seaparator character is ":"
 * separately.
 *

 */

void hildon_caption_set_separator( HildonCaption *caption, 
                                   const gchar *separator )
{
  g_object_set( G_OBJECT(caption), "separator", separator, NULL );
}

/**
 * hildon_caption_get_separator:
 * @caption : A #HildonCaption
 * @returns : the text currently being used as the separator of the caption
 * control. The string is owned by the caption control and the caller should
 * never free or modify this value.
 *
 * Gets separator string of #HildonCaption
 *
 */

gchar *hildon_caption_get_separator( const HildonCaption *caption )
{
  HildonCaptionPrivate *priv;
  g_return_val_if_fail(HILDON_IS_CAPTION(caption), "");
  priv = HILDON_CAPTION_GET_PRIVATE(caption);

  return priv->separator;
}


/**
 * hildon_caption_get_control:
 * @caption : A #HildonCaption
 * @returns : A #GtkWidget
 *
 * Gets caption's control.
 *
 * Deprecated: use gtk_bin_get_child instead
 */
#ifndef HILDON_DISABLE_DEPRECATED
GtkWidget *hildon_caption_get_control( const HildonCaption *caption )
{
  g_return_val_if_fail( HILDON_IS_CAPTION(caption), NULL );
  return GTK_BIN(caption)->child;
}
#endif
/*activates the child control
 *We have this signal so that if needed an application can 
 *know when we've been activated (useful for captions with
 *multiple children
 */
static void hildon_caption_activate( GtkWidget *widget )
{
  HildonCaptionPrivate *priv;
  GtkWidget *child = GTK_BIN(widget)->child;
  priv = HILDON_CAPTION_GET_PRIVATE(widget);

  if( priv->activate_block )
  {
    priv->is_focused = FALSE;
    return;
  }

  if( child )
  {
    priv->activate_block = TRUE;
    gtk_widget_grab_focus( child );
  }
}

/**
 * hildon_caption_set_child_expand:
 * @caption : A #HildonCaption
 * @expand : gboolean to determine is the child expandable
 *
 * Sets child expandability.
 */
void hildon_caption_set_child_expand( HildonCaption *caption, gboolean expand )
{
  HildonCaptionPrivate *priv = NULL;
  GtkWidget *child = NULL;
  g_return_if_fail( HILDON_IS_CAPTION(caption) );

  priv = HILDON_CAPTION_GET_PRIVATE(caption);

  if( priv->expand == expand )
    return;

  priv->expand = expand;
  child = GTK_BIN(caption)->child;

  if( !GTK_IS_WIDGET(child) )
    return;

  if( GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_VISIBLE (caption) )
    gtk_widget_queue_resize( child );

  gtk_widget_child_notify( child, "expand" );
}

/**
 * hildon_caption_get_child_expand:
 * @caption : A #HildonCaption
 * @returns : Wheter the child is expandable or not.
 *
 * Gets childs expandability.
 */
gboolean hildon_caption_get_child_expand( HildonCaption *caption )
{
  HildonCaptionPrivate *priv = NULL;
  g_return_val_if_fail( HILDON_IS_CAPTION(caption), FALSE );
  priv = HILDON_CAPTION_GET_PRIVATE(caption);
  return priv->expand;
}

/**
 * hildon_caption_set_control: 
 * @caption : A #HildonCaption
 * @control : The control to use. Control should not be NULL.
 *
 * Sets the control of the caption. 
 * The old control will be destroyed, unless the caller has added a
 * reference to it.
 * Function unparents the old control (if there is one) and adds the new
 * control.
 *
 * Deprecated: Use gtk_container_add. 
 */
#ifndef HILDON_DISABLE_DEPRECATED
void hildon_caption_set_control( HildonCaption *caption, GtkWidget *control )
{
  GtkWidget *child = NULL;
  g_return_if_fail( HILDON_IS_CAPTION(caption) );
  child = GTK_BIN(caption)->child;

  if( child )
    gtk_container_remove( GTK_CONTAINER(caption), child );
    
  if( control )
  {
    gtk_container_add( GTK_CONTAINER(caption), control );
    child = control;
  }
  else
    child = NULL;
}
#endif

static void
hildon_caption_set_label_text( HildonCaptionPrivate *priv )
{
  gchar *tmp = NULL;
  g_return_if_fail ( priv != NULL );

  if ( priv->text )
  {
    if( priv->separator )
    {
      if( !strcmp( priv->separator, ":" ) &&
          priv->text [strlen ( priv->text ) - 1] == ':')
      {
        gtk_label_set_text( GTK_LABEL( priv->label ), priv->text );
      }
      else
      {
        tmp = g_strconcat( priv->text, priv->separator, NULL );
        gtk_label_set_text( GTK_LABEL( priv->label ), tmp );
        g_free( tmp );
      }
    }
    else
    {
      gtk_label_set_text( GTK_LABEL( priv->label ), priv->text );
    }
  }
  else
  {
    gtk_label_set_text( GTK_LABEL( priv->label ), "" );
  }

}
    
void hildon_caption_set_label_alignment(HildonCaption *caption, 
                                        gfloat alignment)
{
  HildonCaptionPrivate *priv;

  g_return_if_fail(HILDON_IS_CAPTION(caption));

  priv = HILDON_CAPTION_GET_PRIVATE(caption);
  g_object_set(priv->label, "yalign", alignment, NULL);
  g_object_set(priv->icon_align, "yalign", alignment, NULL);

  if (priv->mandatory_icon)
    g_object_set(priv->mandatory_icon, "yalign", alignment, NULL);
}

gfloat hildon_caption_get_label_alignment(HildonCaption *caption)
{
  HildonCaptionPrivate *priv;
  gfloat result;

  g_return_val_if_fail( HILDON_IS_CAPTION(caption), 0);
  priv = HILDON_CAPTION_GET_PRIVATE(caption);
  g_object_get(priv->label, "yalign", &result, NULL);

  return result;
}
