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
 * Modified by the GTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

/* Modified by Nokia Corporation - 2005.
 * 
 */

#include <config.h>
#include <string.h>
#include "gtkalignment.h"
#include "gtkbutton.h"
#include "gtklabel.h"
#include "gtkmain.h"
#include "gtkmarshalers.h"
#include "gtkimage.h"
#include "gtkhbox.h"
#include "gtkstock.h"
#include "gtkiconfactory.h"
#include "gtkintl.h"
#include "gtkprivate.h"
#include "gtkalias.h"

/* Osso addition:
 * Here are the details for each attach 
 * bitmask combination. */
const gchar *osso_gtk_button_attach_details [OSSO_GTK_BUTTON_ATTACH_ENUM_END] =
  { "osso_button",
    "osso_button_n",
    "osso_button_e",
    "osso_button_ne",
    "osso_button_s",
    "osso_button_ns",
    "osso_button_es",
    "osso_button_nes",
    "osso_button_w",
    "osso_button_nw",
    "osso_button_ew",
    "osso_button_new",
    "osso_button_sw",
    "osso_button_nsw",
    "osso_button_esw",
    "osso_button_nesw",
  };

#define CHILD_SPACING     1

/* Take this away after font drawing is fixed */
#define OSSO_FONT_HACK TRUE

static const GtkBorder default_default_border = { 1, 1, 1, 1 };
static const GtkBorder default_default_outside_border = { 0, 0, 0, 0 };

/* Time out before giving up on getting a key release when animating
 * the close button.
 */
#define ACTIVATE_TIMEOUT 250

enum {
  PRESSED,
  RELEASED,
  CLICKED,
  ENTER,
  LEAVE,
  ACTIVATE,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_LABEL,
  PROP_IMAGE,
  PROP_RELIEF,
  PROP_USE_UNDERLINE,
  PROP_USE_STOCK,
  PROP_FOCUS_ON_CLICK,
  PROP_XALIGN,
  PROP_YALIGN,
  PROP_DETAIL,
  PROP_AUTOMATIC_DETAIL
};

#define GTK_BUTTON_GET_PRIVATE(o)       (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTK_TYPE_BUTTON, GtkButtonPrivate))
typedef struct _GtkButtonPrivate GtkButtonPrivate;

struct _GtkButtonPrivate
{
  gfloat       xalign;
  gfloat       yalign;
  GtkWidget   *image;
  guint        align_set : 1;
  guint        image_is_stock : 1;
  gchar       *detail;
  gboolean     automatic_detail;
};

static void gtk_button_class_init     (GtkButtonClass   *klass);
static void gtk_button_init           (GtkButton        *button);
static void gtk_button_destroy        (GtkObject        *object);
static void gtk_button_set_property   (GObject         *object,
                                       guint            prop_id,
                                       const GValue    *value,
                                       GParamSpec      *pspec);
static void gtk_button_get_property   (GObject         *object,
                                       guint            prop_id,
                                       GValue          *value,
                                       GParamSpec      *pspec);
static void gtk_button_screen_changed (GtkWidget        *widget,
				       GdkScreen        *previous_screen);
static void gtk_button_realize        (GtkWidget        *widget);
static void gtk_button_unrealize      (GtkWidget        *widget);
static void gtk_button_map            (GtkWidget        *widget);
static void gtk_button_unmap          (GtkWidget        *widget);
static void gtk_button_size_request   (GtkWidget        *widget,
				       GtkRequisition   *requisition);
static void gtk_button_size_allocate  (GtkWidget        *widget,
				       GtkAllocation    *allocation);
static gint gtk_button_expose         (GtkWidget        *widget,
				       GdkEventExpose   *event);
static gint gtk_button_button_press   (GtkWidget        *widget,
				       GdkEventButton   *event);
static gint gtk_button_button_release (GtkWidget        *widget,
				       GdkEventButton   *event);
static gint gtk_button_key_release    (GtkWidget        *widget,
				       GdkEventKey      *event);
static gint gtk_button_enter_notify   (GtkWidget        *widget,
				       GdkEventCrossing *event);
static gint gtk_button_leave_notify   (GtkWidget        *widget,
				       GdkEventCrossing *event);
static void gtk_real_button_pressed   (GtkButton        *button);
static void gtk_real_button_released  (GtkButton        *button);
static void gtk_real_button_activate  (GtkButton         *button);
static void gtk_button_update_state   (GtkButton        *button);
static void gtk_button_add            (GtkContainer   *container,
			               GtkWidget      *widget);
static GType gtk_button_child_type    (GtkContainer     *container);
static void gtk_button_finish_activate (GtkButton *button,
					gboolean   do_it);

static GObject*	gtk_button_constructor (GType                  type,
					guint                  n_construct_properties,
					GObjectConstructParam *construct_params);
static void gtk_button_construct_child (GtkButton             *button);
static void gtk_button_state_changed   (GtkWidget             *widget,
					GtkStateType           previous_state);
static void gtk_button_grab_notify     (GtkWidget             *widget,
					gboolean               was_grabbed);



static GtkBinClass *parent_class = NULL;
static guint button_signals[LAST_SIGNAL] = { 0 };


GType
gtk_button_get_type (void)
{
  static GType button_type = 0;

  if (!button_type)
    {
      static const GTypeInfo button_info =
      {
	sizeof (GtkButtonClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_button_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkButton),
	16,		/* n_preallocs */
	(GInstanceInitFunc) gtk_button_init,
      };

      button_type = g_type_register_static (GTK_TYPE_BIN, "GtkButton",
					    &button_info, 0);
    }

  return button_type;
}

static void
gtk_button_class_init (GtkButtonClass *klass)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  gobject_class = G_OBJECT_CLASS (klass);
  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;
  container_class = (GtkContainerClass*) klass;
  
  parent_class = g_type_class_peek_parent (klass);

  gobject_class->constructor = gtk_button_constructor;
  gobject_class->set_property = gtk_button_set_property;
  gobject_class->get_property = gtk_button_get_property;

  object_class->destroy = gtk_button_destroy;

  widget_class->screen_changed = gtk_button_screen_changed;
  widget_class->realize = gtk_button_realize;
  widget_class->unrealize = gtk_button_unrealize;
  widget_class->map = gtk_button_map;
  widget_class->unmap = gtk_button_unmap;
  widget_class->size_request = gtk_button_size_request;
  widget_class->size_allocate = gtk_button_size_allocate;
  widget_class->expose_event = gtk_button_expose;
  widget_class->button_press_event = gtk_button_button_press;
  widget_class->button_release_event = gtk_button_button_release;
  widget_class->key_release_event = gtk_button_key_release;
  widget_class->enter_notify_event = gtk_button_enter_notify;
  widget_class->leave_notify_event = gtk_button_leave_notify;
  widget_class->state_changed = gtk_button_state_changed;
  widget_class->grab_notify = gtk_button_grab_notify;

  container_class->child_type = gtk_button_child_type;
  container_class->add = gtk_button_add;

  klass->pressed = gtk_real_button_pressed;
  klass->released = gtk_real_button_released;
  klass->clicked = NULL;
  klass->enter = gtk_button_update_state;
  klass->leave = gtk_button_update_state;
  klass->activate = gtk_real_button_activate;

  g_object_class_install_property (gobject_class,
                                   PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        P_("Label"),
                                                        P_("Text of the label widget inside the button, if the button contains a label widget"),
                                                        NULL,
                                                        GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  
  g_object_class_install_property (gobject_class,
                                   PROP_USE_UNDERLINE,
                                   g_param_spec_boolean ("use-underline",
							 P_("Use underline"),
							 P_("If set, an underline in the text indicates the next character should be used for the mnemonic accelerator key"),
                                                        FALSE,
                                                        GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  
  g_object_class_install_property (gobject_class,
                                   PROP_USE_STOCK,
                                   g_param_spec_boolean ("use-stock",
							 P_("Use stock"),
							 P_("If set, the label is used to pick a stock item instead of being displayed"),
                                                        FALSE,
                                                        GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  
  g_object_class_install_property (gobject_class,
                                   PROP_FOCUS_ON_CLICK,
                                   g_param_spec_boolean ("focus-on-click",
							 P_("Focus on click"),
							 P_("Whether the button grabs focus when it is clicked with the mouse"),
							 TRUE,
							 GTK_PARAM_READWRITE));
  
  g_object_class_install_property (gobject_class,
                                   PROP_RELIEF,
                                   g_param_spec_enum ("relief",
                                                      P_("Border relief"),
                                                      P_("The border relief style"),
                                                      GTK_TYPE_RELIEF_STYLE,
                                                      GTK_RELIEF_NORMAL,
                                                      GTK_PARAM_READABLE | GTK_PARAM_WRITABLE));
  
  /**
   * GtkButton:xalign:
   *
   * If the child of the button is a #GtkMisc or #GtkAlignment, this property 
   * can be used to control it's horizontal alignment. 0.0 is left aligned, 
   * 1.0 is right aligned.
   * 
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   PROP_XALIGN,
                                   g_param_spec_float("xalign",
                                                      P_("Horizontal alignment for child"),
                                                      P_("Horizontal position of child in available space. 0.0 is left aligned, 1.0 is right aligned"),
                                                      0.0,
                                                      1.0,
                                                      0.5,
                                                      GTK_PARAM_READABLE | GTK_PARAM_WRITABLE));

  /**
   * GtkButton:yalign:
   *
   * If the child of the button is a #GtkMisc or #GtkAlignment, this property 
   * can be used to control it's vertical alignment. 0.0 is top aligned, 
   * 1.0 is bottom aligned.
   * 
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   PROP_YALIGN,
                                   g_param_spec_float("yalign",
                                                      P_("Vertical alignment for child"),
                                                      P_("Vertical position of child in available space. 0.0 is top aligned, 1.0 is bottom aligned"),
                                                      0.0,
                                                      1.0,
                                                      0.5,
                                                      GTK_PARAM_READABLE | GTK_PARAM_WRITABLE));

  /**
   * GtkButton::image:
   * 
   * The child widget to appear next to the button text.
   * 
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
                                   PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        P_("Image widget"),
                                                        P_("Child widget to appear next to the button text"),
                                                        GTK_TYPE_WIDGET,
                                                        GTK_PARAM_READABLE | GTK_PARAM_WRITABLE));

  button_signals[PRESSED] =
    g_signal_new ("pressed",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GtkButtonClass, pressed),
		  NULL, NULL,
		  _gtk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  button_signals[RELEASED] =
    g_signal_new ("released",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GtkButtonClass, released),
		  NULL, NULL,
		  _gtk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  button_signals[CLICKED] =
    g_signal_new ("clicked",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GtkButtonClass, clicked),
		  NULL, NULL,
		  _gtk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  button_signals[ENTER] =
    g_signal_new ("enter",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GtkButtonClass, enter),
		  NULL, NULL,
		  _gtk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  button_signals[LEAVE] =
    g_signal_new ("leave",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GtkButtonClass, leave),
		  NULL, NULL,
		  _gtk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * GtkButton::activate:
   * @widget: the object which received the signal.
   *
   * The "activate" signal on GtkButton is an action signal and
   * emitting it causes the button to animate press then release. 
   * Applications should never connect to this signal, but use the
   * "clicked" signal.
   */
  button_signals[ACTIVATE] =
    g_signal_new ("activate",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GtkButtonClass, activate),
		  NULL, NULL,
		  _gtk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  widget_class->activate_signal = button_signals[ACTIVATE];

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("default-border",
							       P_("Default Spacing"),
							       P_("Extra space to add for CAN_DEFAULT buttons"),
							       GTK_TYPE_BORDER,
							       GTK_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("default-outside-border",
							       P_("Default Outside Spacing"),
							       P_("Extra space to add for CAN_DEFAULT buttons that is always drawn outside the border"),
							       GTK_TYPE_BORDER,
							       GTK_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("child-displacement-x",
							     P_("Child X Displacement"),
							     P_("How far in the x direction to move the child when the button is depressed"),
							     G_MININT,
							     G_MAXINT,
							     0,
							     GTK_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("child-displacement-y",
							     P_("Child Y Displacement"),
							     P_("How far in the y direction to move the child when the button is depressed"),
							     G_MININT,
							     G_MAXINT,
							     0,
							     GTK_PARAM_READABLE));

  /**
   * GtkButton:displace-focus:
   *
   * Whether the child_displacement_x/child_displacement_y properties should also 
   * affect the focus rectangle.
   *
   * Since: 2.6
   */
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("displace-focus",
								 P_("Displace focus"),
								 P_("Whether the child_displacement_x/_y properties should also affect the focus rectangle"),
						       FALSE,
						       GTK_PARAM_READABLE));

  gtk_settings_install_property (g_param_spec_boolean ("gtk-button-images",
						       P_("Show button images"),
						       P_("Whether stock icons should be shown in buttons"),
						       TRUE,
						       GTK_PARAM_READWRITE));

  /**
   * GtkButton:child-spacing:
   *
   * Spacing between the button edges and its child.
   * 
   * Since: maemo 1.0
   */
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("child-spacing",
							     P_("Child spacing"),
							     P_("Spacing between button edges and child."),
							     0,
							     G_MAXINT,
							     CHILD_SPACING,
							     GTK_PARAM_READABLE));
  /**
   * GtkButton:detail:
   *
   * The detail the button is drawn with.
   *
   * Since: maemo 1.0
   */
  g_object_class_install_property (gobject_class,
				   PROP_DETAIL,
				   g_param_spec_string ("detail",
							P_("Detail"),
							P_("The detail the button is drawn with."),
							"buttondefault",
							GTK_PARAM_READWRITE));

  /**
   * GtkButton:automatic-detail:
   *
   * Whether setting the drawing detail is automatic, based on 
   * GtkTable/GtkHButtonBox.
   * 
   * Since: maemo 1.0
   */
  g_object_class_install_property (gobject_class,
				   PROP_AUTOMATIC_DETAIL,
				   g_param_spec_boolean ("automatic-detail",
							 P_("Automatic Detail"),
							 P_("Whether setting detail is automatic based on GtkTable/GtkHButtonBox."),
							 TRUE,
							 GTK_PARAM_READWRITE));


  /**
   * GtkButton:child-offset-y:
   *
   * How many pixels to add/take away from GtkButton's child size allocation.
   * 
   * Since: maemo 1.0
   */
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("child-offset-y",
							     P_("Child Y Offset"),
							     P_("How many pixels to add/take away from GtkButton's child size allocation."),
							     G_MININT,
							     G_MAXINT,
							     0,
							     GTK_PARAM_READABLE));

  /**
   * GtkButton:listboxheader:
   *
   * Whether the button is a GtkTreeView column Listbox header. If TRUE, the 
   * button will be drawn differently.
   *
   * Since: maemo 1.0
   */
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("listboxheader",
								 P_( "Listbox header" ),
								 P_( "Listbox header ( FALSE / TRUE )" ),
								 FALSE,
								 GTK_PARAM_READABLE));

  /**
   * GtkButton::separator-height:
   *
   * Column Listbox header separator height
   *
   * Since: maemo 1.0
   */
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("separator-height",
							     P_( "Separator height" ),
							     P_( "Listbox header separator height ( FALSE / TRUE )" ),
							     0,
							     G_MAXINT,
							     0,
							     GTK_PARAM_READABLE));

  /**
   * GtkButton:padding:
   *
   * Paddings around the child of the button. gtkrc usage: GtkButton::padding  
   * = { left, right, top, bottom }
   * 
   * Since: maemo 1.0
   */
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("padding",
							       P_("Padding"),
							       P_("Paddings around the button child"),
							       GTK_TYPE_BORDER,
							       GTK_PARAM_READABLE));

  /**
   * GtkButton:minimum-width:
   *
   * The minimum width that the button will request.
   * 
   * Since: maemo 1.0
   */
  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("minimum-width",
							     P_("Minimum Width"),
							     P_("Minimum width of the button"),
							     0,
							     G_MAXINT,
							     0,
							     GTK_PARAM_READABLE));
  
  g_type_class_add_private (gobject_class, sizeof (GtkButtonPrivate));  
}

static void
gtk_button_init (GtkButton *button)
{
  GtkButtonPrivate *priv = GTK_BUTTON_GET_PRIVATE (button);

  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_FOCUS | GTK_RECEIVES_DEFAULT);
  GTK_WIDGET_SET_FLAGS (button, GTK_NO_WINDOW);

  button->label_text = NULL;
  
  button->constructed = FALSE;
  button->in_button = FALSE;
  button->button_down = FALSE;
  button->relief = GTK_RELIEF_NORMAL;
  button->use_stock = FALSE;
  button->use_underline = FALSE;
  button->depressed = FALSE;
  button->depress_on_activate = TRUE;
  button->focus_on_click = TRUE;

  priv->xalign = 0.5;
  priv->yalign = 0.5;
  priv->align_set = 0;
  priv->detail = g_strdup ("buttondefault");
  priv->automatic_detail = TRUE;
  priv->image_is_stock = TRUE;

  g_object_set (G_OBJECT (button), "tap_and_hold_state",
                GTK_STATE_ACTIVE, NULL);
}

static void
gtk_button_destroy (GtkObject *object)
{
  GtkButton *button = GTK_BUTTON (object);
  GtkButtonPrivate *priv = GTK_BUTTON_GET_PRIVATE (button);
  
  if (button->label_text)
    {
      g_free (button->label_text);
      button->label_text = NULL;
    }

  if (priv->detail)
    {
      g_free (priv->detail);
      priv->detail = NULL;
    }
  
  (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static GObject*
gtk_button_constructor (GType                  type,
			guint                  n_construct_properties,
			GObjectConstructParam *construct_params)
{
  GObject *object;
  GtkButton *button;

  object = (* G_OBJECT_CLASS (parent_class)->constructor) (type,
							   n_construct_properties,
							   construct_params);

  button = GTK_BUTTON (object);
  button->constructed = TRUE;

  if (button->label_text != NULL)
    gtk_button_construct_child (button);
  
  return object;
}


static GType
gtk_button_child_type  (GtkContainer     *container)
{
  if (!GTK_BIN (container)->child)
    return GTK_TYPE_WIDGET;
  else
    return G_TYPE_NONE;
}

static void
maybe_set_alignment (GtkButton *button,
		     GtkWidget *widget)
{
  GtkButtonPrivate *priv = GTK_BUTTON_GET_PRIVATE (button);

  if (GTK_IS_MISC (widget))
    {
      GtkMisc *misc = GTK_MISC (widget);
      
      if (priv->align_set)
	gtk_misc_set_alignment (misc, priv->xalign, priv->yalign);
    }
  else if (GTK_IS_ALIGNMENT (widget))
    {
      GtkAlignment *alignment = GTK_ALIGNMENT (widget);

      if (priv->align_set)
	gtk_alignment_set (alignment, priv->xalign, priv->yalign, 
			   alignment->xscale, alignment->yscale);
    }
}

static void
gtk_button_add (GtkContainer *container,
		GtkWidget    *widget)
{
  maybe_set_alignment (GTK_BUTTON (container), widget);

  GTK_CONTAINER_CLASS (parent_class)->add (container, widget);
}

static void
gtk_button_set_property (GObject         *object,
                         guint            prop_id,
                         const GValue    *value,
                         GParamSpec      *pspec)
{
  GtkButton *button = GTK_BUTTON (object);
  GtkButtonPrivate *priv = GTK_BUTTON_GET_PRIVATE (button);

  switch (prop_id)
    {
    case PROP_LABEL:
      gtk_button_set_label (button, g_value_get_string (value));
      break;
    case PROP_IMAGE:
      gtk_button_set_image (button, (GtkWidget *) g_value_get_object (value));
      break;
    case PROP_RELIEF:
      gtk_button_set_relief (button, g_value_get_enum (value));
      break;
    case PROP_USE_UNDERLINE:
      gtk_button_set_use_underline (button, g_value_get_boolean (value));
      break;
    case PROP_USE_STOCK:
      gtk_button_set_use_stock (button, g_value_get_boolean (value));
      break;
    case PROP_FOCUS_ON_CLICK:
      gtk_button_set_focus_on_click (button, g_value_get_boolean (value));
      break;
    case PROP_XALIGN:
      gtk_button_set_alignment (button, g_value_get_float (value), priv->yalign);
      break;
    case PROP_YALIGN:
      gtk_button_set_alignment (button, priv->xalign, g_value_get_float (value));
      break;
    case PROP_DETAIL:
      if (priv->detail)
        g_free (priv->detail);
      priv->detail = g_strdup (g_value_get_string (value));
      gtk_widget_queue_draw (GTK_WIDGET (button));
      break;
    case PROP_AUTOMATIC_DETAIL:
      priv->automatic_detail = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_button_get_property (GObject         *object,
                         guint            prop_id,
                         GValue          *value,
                         GParamSpec      *pspec)
{
  GtkButton *button = GTK_BUTTON (object);
  GtkButtonPrivate *priv = GTK_BUTTON_GET_PRIVATE (button);

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, button->label_text);
      break;
    case PROP_IMAGE:
      g_value_set_object (value, (GObject *)priv->image);
      break;
    case PROP_RELIEF:
      g_value_set_enum (value, gtk_button_get_relief (button));
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, button->use_underline);
      break;
    case PROP_USE_STOCK:
      g_value_set_boolean (value, button->use_stock);
      break;
    case PROP_FOCUS_ON_CLICK:
      g_value_set_boolean (value, button->focus_on_click);
      break;
    case PROP_XALIGN:
      g_value_set_float (value, priv->xalign);
      break;
    case PROP_YALIGN:
      g_value_set_float (value, priv->yalign);
      break;
    case PROP_DETAIL:
      g_value_set_string (value, priv->detail);
      break;
    case PROP_AUTOMATIC_DETAIL:
      g_value_set_boolean (value, priv->automatic_detail);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

GtkWidget*
gtk_button_new (void)
{
  return g_object_new (GTK_TYPE_BUTTON, NULL);
}

static gboolean
show_image (GtkButton *button)
{
  gboolean show;
  
  if (button->label_text)
    {
      GtkSettings *settings;

      settings = gtk_widget_get_settings (GTK_WIDGET (button));        
      g_object_get (settings, "gtk-button-images", &show, NULL);
    }
  else
    show = TRUE;

  return show;
}

static void
gtk_button_construct_child (GtkButton *button)
{
  GtkButtonPrivate *priv = GTK_BUTTON_GET_PRIVATE (button);
  GtkStockItem item;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *align;
  GtkWidget *image = NULL;
  gchar *label_text = NULL;
  
  if (!button->constructed)
    return;
 
  if (!button->label_text && !priv->image)
    return;
  
  if (priv->image && !priv->image_is_stock)
    {
      image = g_object_ref (priv->image);
      if (image->parent)
	gtk_container_remove (GTK_CONTAINER (image->parent), image);
      
      priv->image = NULL;
    }
  
  if (GTK_BIN (button)->child)
    gtk_container_remove (GTK_CONTAINER (button),
			  GTK_BIN (button)->child);
  
  if (button->use_stock &&
      button->label_text &&
      gtk_stock_lookup (button->label_text, &item))
    {
      if (!image)
	image = g_object_ref (gtk_image_new_from_stock (button->label_text, GTK_ICON_SIZE_BUTTON));

      label_text = item.label;
    }
  else
    label_text = button->label_text;

  if (image)
    {
      priv->image = image;

      g_object_set (priv->image, 
		    "visible", show_image (button),
		    "no_show_all", TRUE,
		    NULL);
      hbox = gtk_hbox_new (FALSE, 2);

      if (priv->align_set)
	align = gtk_alignment_new (priv->xalign, priv->yalign, 0.0, 0.0);
      else
	align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	
      gtk_box_pack_start (GTK_BOX (hbox), priv->image, FALSE, FALSE, 0);

      if (label_text)
	{
	  label = gtk_label_new_with_mnemonic (label_text);
	  gtk_label_set_mnemonic_widget (GTK_LABEL (label), 
					 GTK_WIDGET (button));

	  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	}
      
      gtk_container_add (GTK_CONTAINER (button), align);
      gtk_container_add (GTK_CONTAINER (align), hbox);
      gtk_widget_show_all (align);

      g_object_unref (image);

      return;
    }
  
  if (button->use_underline)
    {
      label = gtk_label_new_with_mnemonic (button->label_text);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
    }
  else
    label = gtk_label_new (button->label_text);
  
  if (priv->align_set)
    gtk_misc_set_alignment (GTK_MISC (label), priv->xalign, priv->yalign);
  
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (button), label);
}


GtkWidget*
gtk_button_new_with_label (const gchar *label)
{
  return g_object_new (GTK_TYPE_BUTTON, "label", label, NULL);
}

/**
 * gtk_button_new_from_stock:
 * @stock_id: the name of the stock item 
 *
 * Creates a new #GtkButton containing the image and text from a stock item.
 * Some stock ids have preprocessor macros like #GTK_STOCK_OK and
 * #GTK_STOCK_APPLY.
 *
 * If @stock_id is unknown, then it will be treated as a mnemonic
 * label (as for gtk_button_new_with_mnemonic()).
 *
 * Returns: a new #GtkButton
 **/
GtkWidget*
gtk_button_new_from_stock (const gchar *stock_id)
{
  return g_object_new (GTK_TYPE_BUTTON,
                       "label", stock_id,
                       "use_stock", TRUE,
                       "use_underline", TRUE,
                       NULL);
}

/**
 * gtk_button_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *         mnemonic character
 * @returns: a new #GtkButton
 *
 * Creates a new #GtkButton containing a label.
 * If characters in @label are preceded by an underscore, they are underlined.
 * If you need a literal underscore character in a label, use '__' (two 
 * underscores). The first underlined character represents a keyboard 
 * accelerator called a mnemonic.
 * Pressing Alt and that key activates the button.
 **/
GtkWidget*
gtk_button_new_with_mnemonic (const gchar *label)
{
  return g_object_new (GTK_TYPE_BUTTON, "label", label, "use_underline", TRUE,  NULL);
}

void
gtk_button_pressed (GtkButton *button)
{
  g_return_if_fail (GTK_IS_BUTTON (button));

  
  g_signal_emit (button, button_signals[PRESSED], 0);
}

void
gtk_button_released (GtkButton *button)
{
  g_return_if_fail (GTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[RELEASED], 0);
}

void
gtk_button_clicked (GtkButton *button)
{
  g_return_if_fail (GTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[CLICKED], 0);
}

void
gtk_button_enter (GtkButton *button)
{
  g_return_if_fail (GTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[ENTER], 0);
}

void
gtk_button_leave (GtkButton *button)
{
  g_return_if_fail (GTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[LEAVE], 0);
}

void
gtk_button_set_relief (GtkButton *button,
		       GtkReliefStyle newrelief)
{
  g_return_if_fail (GTK_IS_BUTTON (button));

  if (newrelief != button->relief) 
    {
       button->relief = newrelief;
       g_object_notify (G_OBJECT (button), "relief");
       gtk_widget_queue_draw (GTK_WIDGET (button));
    }
}

GtkReliefStyle
gtk_button_get_relief (GtkButton *button)
{
  g_return_val_if_fail (GTK_IS_BUTTON (button), GTK_RELIEF_NORMAL);

  return button->relief;
}

static void
gtk_button_realize (GtkWidget *widget)
{
  GtkButton *button;
  GdkWindowAttr attributes;
  gint attributes_mask;
  gint border_width;

  button = GTK_BUTTON (widget);
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  border_width = GTK_CONTAINER (widget)->border_width;

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x + border_width;
  attributes.y = widget->allocation.y + border_width;
  attributes.width = widget->allocation.width - border_width * 2;
  attributes.height = widget->allocation.height - border_width * 2;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  widget->window = gtk_widget_get_parent_window (widget);
  g_object_ref (widget->window);
  
  button->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
					 &attributes, attributes_mask);
  gdk_window_set_user_data (button->event_window, button);

  widget->style = gtk_style_attach (widget->style, widget->window);
}

static void
gtk_button_unrealize (GtkWidget *widget)
{
  GtkButton *button = GTK_BUTTON (widget);

  if (button->activate_timeout)
    gtk_button_finish_activate (button, FALSE);

  if (button->event_window)
    {
      gdk_window_set_user_data (button->event_window, NULL);
      gdk_window_destroy (button->event_window);
      button->event_window = NULL;
    }
  
  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gtk_button_map (GtkWidget *widget)
{
  GtkButton *button = GTK_BUTTON (widget);
  
  GTK_WIDGET_CLASS (parent_class)->map (widget);

  if (button->event_window)
    gdk_window_show (button->event_window);
}

static void
gtk_button_unmap (GtkWidget *widget)
{
  GtkButton *button = GTK_BUTTON (widget);
    
  if (button->event_window)
    gdk_window_hide (button->event_window);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gtk_button_get_props (GtkButton *button,
		      GtkBorder *default_border,
		      GtkBorder *default_outside_border,
		      gboolean  *interior_focus)
{
  GtkWidget *widget =  GTK_WIDGET (button);
  GtkBorder *tmp_border;

  if (default_border)
    {
      gtk_widget_style_get (widget, "default_border", &tmp_border, NULL);

      if (tmp_border)
	{
	  *default_border = *tmp_border;
	  g_free (tmp_border);
	}
      else
	*default_border = default_default_border;
    }

  if (default_outside_border)
    {
      gtk_widget_style_get (widget, "default_outside_border", &tmp_border, NULL);

      if (tmp_border)
	{
	  *default_outside_border = *tmp_border;
	  g_free (tmp_border);
	}
      else
	*default_outside_border = default_default_outside_border;
    }

  if (interior_focus)
    gtk_widget_style_get (widget, "interior_focus", interior_focus, NULL);
}
	
static void
gtk_button_size_request (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkBorder default_border;
  gint focus_width;
  gint focus_pad;
  gint child_spacing;
  gint separator_height;

  gtk_button_get_props (button, &default_border, NULL, NULL);
  gtk_widget_style_get (GTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"child-spacing", &child_spacing,
			"separator-height", &separator_height,
			NULL);
 
  requisition->width = (GTK_CONTAINER (widget)->border_width + child_spacing +
			GTK_WIDGET (widget)->style->xthickness) * 2;
  requisition->height = (GTK_CONTAINER (widget)->border_width + child_spacing +
			 GTK_WIDGET (widget)->style->ythickness) * 2;

  if (GTK_WIDGET_CAN_DEFAULT (widget))
    {
      requisition->width += default_border.left + default_border.right;
      requisition->height += default_border.top + default_border.bottom;
    }

  if (GTK_BIN (button)->child && GTK_WIDGET_VISIBLE (GTK_BIN (button)->child))
    {
      GtkRequisition child_requisition;
      GtkBorder *padding;
      gint minimum_width;

      gtk_widget_size_request (GTK_BIN (button)->child, &child_requisition);

      gtk_widget_style_get (widget,
			    "padding",
			    &padding,
			    "minimum_width",
			    &minimum_width,
			    NULL);
		
      if ( padding )
	{
	  requisition->width += padding->left + padding->right;
	  requisition->height += padding->top + padding->bottom;

          g_free (padding);
	}

      requisition->width += child_requisition.width;
      requisition->height += child_requisition.height;

      if (requisition->width < minimum_width)
        requisition->width = minimum_width;
    }
  
  requisition->width += 2 * (focus_width + focus_pad);
  requisition->height += 2 * (focus_width + focus_pad);

  requisition->height += separator_height;
}

static void
gtk_button_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  GtkButton *button = GTK_BUTTON (widget);
  GtkAllocation child_allocation;

  gint border_width = GTK_CONTAINER (widget)->border_width;
  gint xthickness = GTK_WIDGET (widget)->style->xthickness;
  gint ythickness = GTK_WIDGET (widget)->style->ythickness;
  GtkBorder default_border;
  gint focus_width;
  gint focus_pad;
  gint child_spacing;
  gint separator_height;

  gtk_button_get_props (button, &default_border, NULL, NULL);
  gtk_widget_style_get (GTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"child-spacing", &child_spacing,
			"separator-height", &separator_height,
			NULL);

  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (button->event_window,
			    widget->allocation.x + border_width,
			    widget->allocation.y + border_width,
			    widget->allocation.width - border_width * 2,
			    widget->allocation.height - border_width * 2);

  if (GTK_BIN (button)->child && GTK_WIDGET_VISIBLE (GTK_BIN (button)->child))
    {
      child_allocation.x = widget->allocation.x + border_width + child_spacing + xthickness;
      child_allocation.y = widget->allocation.y + border_width + child_spacing + ythickness;
      
      child_allocation.width = MAX (1, widget->allocation.width - (child_spacing + xthickness) * 2 -
				    border_width * 2);
      child_allocation.height = MAX (1, widget->allocation.height - separator_height - (child_spacing + ythickness) * 2 -
				     border_width * 2);

#ifdef OSSO_FONT_HACK
      {
	gint child_offset_y;
	
	gtk_widget_style_get (widget, "child_offset_y", &child_offset_y, NULL);
	
	if( GTK_IS_LABEL(GTK_BIN (button)->child) )
	  {
	    child_allocation.y += child_offset_y;
	  }
      }
#endif
      
      if (GTK_WIDGET_CAN_DEFAULT (button))
	{
	  child_allocation.x += default_border.left;
	  child_allocation.y += default_border.top;
	  child_allocation.width =  MAX (1, child_allocation.width - default_border.left - default_border.right);
	  child_allocation.height = MAX (1, child_allocation.height - default_border.top - default_border.bottom);
	}

      if (GTK_WIDGET_CAN_FOCUS (button))
	{
	  child_allocation.x += focus_width + focus_pad;
	  child_allocation.y += focus_width + focus_pad;
	  child_allocation.width =  MAX (1, child_allocation.width - (focus_width + focus_pad) * 2);
	  child_allocation.height = MAX (1, child_allocation.height - (focus_width + focus_pad) * 2);
	}

      if (button->depressed)
	{
	  gint child_displacement_x;
	  gint child_displacement_y;
	  
	  gtk_widget_style_get (widget,
				"child_displacement_x", &child_displacement_x, 
				"child_displacement_y", &child_displacement_y,
				NULL);
	  child_allocation.x += child_displacement_x;
	  child_allocation.y += child_displacement_y;
	}

      gtk_widget_size_allocate (GTK_BIN (button)->child, &child_allocation);
    }
}

void
_gtk_button_paint (GtkButton    *button,
		   GdkRectangle *area,
		   GtkStateType  state_type,
		   GtkShadowType shadow_type,
		   const gchar  *main_detail,
		   const gchar  *default_detail)
{
  GtkWidget *widget;
  gint width, height;
  gint x, y;
  gint border_width;
  GtkBorder default_border;
  GtkBorder default_outside_border;
  gboolean interior_focus;
  gint focus_width;
  gint focus_pad;
  gint separator_height;
  gboolean listboxheader;
  GtkButtonPrivate *priv;

  g_return_if_fail (GTK_IS_BUTTON (button));
  
  priv = GTK_BUTTON_GET_PRIVATE (button);
  
  if (GTK_WIDGET_DRAWABLE (button))
    {
      widget = GTK_WIDGET (button);
      border_width = GTK_CONTAINER (widget)->border_width;

      gtk_button_get_props (button, &default_border, &default_outside_border, &interior_focus);
      gtk_widget_style_get (GTK_WIDGET (widget),
			    "focus-line-width", &focus_width,
			    "focus-padding", &focus_pad,
                            "listboxheader", &listboxheader,
                            "separator-height", &separator_height,
			    NULL); 
	
      x = widget->allocation.x + border_width;
      y = widget->allocation.y + border_width;
      width = widget->allocation.width - border_width * 2;
      height = widget->allocation.height - border_width * 2;

      if (listboxheader)
        {
          /* this changes everything! */
          PangoLayout *layout;
          int layout_height;

          /* construct layout - see get_layout in gtkcellrenderertext.c */
          layout = gtk_widget_create_pango_layout (widget, /* use parent treeview instead? */
                                                   button->label_text);
          pango_layout_set_width (layout, -1);
          pango_layout_get_pixel_size (layout, NULL, &layout_height);

          /* render text */
          gtk_paint_layout (widget->style,
                            widget->window,
                            GTK_STATE_NORMAL,
                            TRUE,
                            area,
                            widget,
                            "listboxheader",
                            x,
                            y,
                            layout);

          g_object_unref (layout);

          /* draw separator */
          gtk_paint_hline (widget->style,
                           widget->window,
                           GTK_STATE_NORMAL,
                           area,
                           widget,
                           "listboxseparator",
                           area->x - focus_width - focus_pad,
                           area->x + area->width + focus_width + focus_pad,
                           layout_height + separator_height * 2);
          return;
        }

      if (GTK_WIDGET_HAS_DEFAULT (widget) &&
	  GTK_BUTTON (widget)->relief == GTK_RELIEF_NORMAL)
	{
          /* This comment is here because it's part of the
           * normal GtkButton
           */
	  /*  gtk_paint_box (widget->style, widget->window,
			 GTK_STATE_NORMAL, GTK_SHADOW_IN,
			 area, widget, "buttondefault",
			 x, y, width, height); */

	  x += default_border.left;
	  y += default_border.top;
	  width -= default_border.left + default_border.right;
	  height -= default_border.top + default_border.bottom;
	}
      else if (GTK_WIDGET_CAN_DEFAULT (widget))
	{
	  x += default_outside_border.left;
	  y += default_outside_border.top;
	  width -= default_outside_border.left + default_outside_border.right;
	  height -= default_outside_border.top + default_outside_border.bottom;
	}
       
      if (!interior_focus && GTK_WIDGET_HAS_FOCUS (widget))
	{
	  x += focus_width + focus_pad;
	  y += focus_width + focus_pad;
	  width -= 2 * (focus_width + focus_pad);
	  height -= 2 * (focus_width + focus_pad);
	}

      if (button->relief != GTK_RELIEF_NONE || button->depressed ||
	  GTK_WIDGET_STATE(widget) == GTK_STATE_PRELIGHT)
	gtk_paint_box (widget->style, widget->window,
		       state_type,
		       shadow_type, area, widget, priv->detail,
		       x, y, width, height);
       
      if (GTK_WIDGET_HAS_FOCUS (widget))
	{
	  gint child_displacement_x;
	  gint child_displacement_y;
	  gboolean displace_focus;
	  
	  gtk_widget_style_get (GTK_WIDGET (widget),
				"child_displacement_y", &child_displacement_y,
				"child_displacement_x", &child_displacement_x,
				"displace_focus", &displace_focus,
				NULL);

	  if (interior_focus)
	    {
	      x += widget->style->xthickness + focus_pad;
	      y += widget->style->ythickness + focus_pad;
	      width -= 2 * (widget->style->xthickness + focus_pad);
	      height -=  2 * (widget->style->ythickness + focus_pad);
	    }
	  else
	    {
	      x -= focus_width + focus_pad;
	      y -= focus_width + focus_pad;
	      width += 2 * (focus_width + focus_pad);
	      height += 2 * (focus_width + focus_pad);
	    }

	  if (button->depressed && displace_focus)
	    {
	      x += child_displacement_x;
	      y += child_displacement_y;
	    }
          /* Comment exists, because it is part of normal GtkButton
	  gtk_paint_focus (widget->style, widget->window, GTK_WIDGET_STATE (widget),
			   area, widget, "button",
			   x, y, width, height);
          */
	  gtk_paint_focus (widget->style, widget->window, GTK_WIDGET_STATE (widget),
			   area, widget, priv->detail,
			   x, y, width, height);
	}
    }
}

static gboolean
gtk_button_expose (GtkWidget      *widget,
		   GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GtkButton *button = GTK_BUTTON (widget);
      
      _gtk_button_paint (button, &event->area,
			 GTK_WIDGET_STATE (widget),
			 button->depressed ? GTK_SHADOW_IN : GTK_SHADOW_OUT,
			 "button", "buttondefault");
      
      (* GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);
    }
  
  return FALSE;
}

static gboolean
gtk_button_button_press (GtkWidget      *widget,
			 GdkEventButton *event)
{
  GtkButton *button;

  if (event->type == GDK_BUTTON_PRESS)
    {
      button = GTK_BUTTON (widget);

      if (button->focus_on_click && !GTK_WIDGET_HAS_FOCUS (widget))
	gtk_widget_grab_focus (widget);

      if (event->button == 1)
	gtk_button_pressed (button);
    }

  return TRUE;
}

static gboolean
gtk_button_button_release (GtkWidget      *widget,
			   GdkEventButton *event)
{
  GtkButton *button;

  if (event->button == 1)
    {
      button = GTK_BUTTON (widget);
      gtk_button_released (button);
    }

  return TRUE;
}

static gboolean
gtk_button_key_release (GtkWidget   *widget,
			GdkEventKey *event)
{
  GtkButton *button = GTK_BUTTON (widget);

  if (button->activate_timeout)
    {
      gtk_button_finish_activate (button, TRUE);
      return TRUE;
    }
  else if (GTK_WIDGET_CLASS (parent_class)->key_release_event)
    return GTK_WIDGET_CLASS (parent_class)->key_release_event (widget, event);
  else
    return FALSE;
}

static gboolean
gtk_button_enter_notify (GtkWidget        *widget,
			 GdkEventCrossing *event)
{
  GtkButton *button;
  GtkWidget *event_widget;

  button = GTK_BUTTON (widget);
  event_widget = gtk_get_event_widget ((GdkEvent*) event);

  if ((event_widget == widget) &&
      (event->detail != GDK_NOTIFY_INFERIOR))
    {
      button->in_button = TRUE;
      gtk_button_enter (button);
    }

  return FALSE;
}

static gboolean
gtk_button_leave_notify (GtkWidget        *widget,
			 GdkEventCrossing *event)
{
  GtkButton *button;
  GtkWidget *event_widget;

  button = GTK_BUTTON (widget);
  event_widget = gtk_get_event_widget ((GdkEvent*) event);

  if ((event_widget == widget) &&
      (event->detail != GDK_NOTIFY_INFERIOR))
    {
      button->in_button = FALSE;
      gtk_button_leave (button);
    }

  return FALSE;
}

static void
gtk_real_button_pressed (GtkButton *button)
{
  if (button->activate_timeout)
    return;
  
  button->button_down = TRUE;
  gtk_button_update_state (button);
}

static void
gtk_real_button_released (GtkButton *button)
{
  if (button->button_down)
    {
      button->button_down = FALSE;

      if (button->activate_timeout)
	return;
      
      if (button->in_button)
	gtk_button_clicked (button);

      gtk_button_update_state (button);
    }
}

static gboolean
button_activate_timeout (gpointer data)
{
  GDK_THREADS_ENTER ();
  
  gtk_button_finish_activate (data, TRUE);

  GDK_THREADS_LEAVE ();

  return FALSE;
}

static void
gtk_real_button_activate (GtkButton *button)
{
  GtkWidget *widget = GTK_WIDGET (button);
  
  if (GTK_WIDGET_REALIZED (button) && !button->activate_timeout)
    {
      if (gdk_keyboard_grab (button->event_window, TRUE,
			     gtk_get_current_event_time ()) == 0)
	{
	  gtk_grab_add (widget);
	  
	  button->activate_timeout = g_timeout_add (ACTIVATE_TIMEOUT,
						    button_activate_timeout,
						    button);
	  button->button_down = TRUE;
	  gtk_button_update_state (button);
	  gtk_widget_queue_draw (GTK_WIDGET (button));
	}
    }
}

static void
gtk_button_finish_activate (GtkButton *button,
			    gboolean   do_it)
{
  GtkWidget *widget = GTK_WIDGET (button);
  
  g_source_remove (button->activate_timeout);
  button->activate_timeout = 0;

  gdk_display_keyboard_ungrab (gtk_widget_get_display (widget),
			       gtk_get_current_event_time ());
  gtk_grab_remove (widget);

  button->button_down = FALSE;

  gtk_button_update_state (button);
  gtk_widget_queue_draw (GTK_WIDGET (button));

  if (do_it)
    gtk_button_clicked (button);
}

/**
 * gtk_button_set_label:
 * @button: a #GtkButton
 * @label: a string
 *
 * Sets the text of the label of the button to @str. This text is
 * also used to select the stock item if gtk_button_set_use_stock()
 * is used.
 *
 * This will also clear any previously set labels.
 **/
void
gtk_button_set_label (GtkButton   *button,
		      const gchar *label)
{
  gchar *new_label;
  
  g_return_if_fail (GTK_IS_BUTTON (button));

  new_label = g_strdup (label);
  g_free (button->label_text);
  button->label_text = new_label;
  
  gtk_button_construct_child (button);
  
  g_object_notify (G_OBJECT (button), "label");
}

/**
 * gtk_button_get_label:
 * @button: a #GtkButton
 *
 * Fetches the text from the label of the button, as set by
 * gtk_button_set_label(). If the label text has not 
 * been set the return value will be %NULL. This will be the 
 * case if you create an empty button with gtk_button_new() to 
 * use as a container.
 *
 * Return value: The text of the label widget. This string is owned
 * by the widget and must not be modified or freed.
 **/
G_CONST_RETURN gchar *
gtk_button_get_label (GtkButton *button)
{
  g_return_val_if_fail (GTK_IS_BUTTON (button), NULL);
  
  return button->label_text;
}

/**
 * gtk_button_set_use_underline:
 * @button: a #GtkButton
 * @use_underline: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text of the button label indicates
 * the next character should be used for the mnemonic accelerator key.
 */
void
gtk_button_set_use_underline (GtkButton *button,
			      gboolean   use_underline)
{
  g_return_if_fail (GTK_IS_BUTTON (button));

  use_underline = use_underline != FALSE;

  if (use_underline != button->use_underline)
    {
      button->use_underline = use_underline;
  
      gtk_button_construct_child (button);
      
      g_object_notify (G_OBJECT (button), "use-underline");
    }
}

/**
 * gtk_button_get_use_underline:
 * @button: a #GtkButton
 *
 * Returns whether an embedded underline in the button label indicates a
 * mnemonic. See gtk_button_set_use_underline ().
 *
 * Return value: %TRUE if an embedded underline in the button label
 *               indicates the mnemonic accelerator keys.
 **/
gboolean
gtk_button_get_use_underline (GtkButton *button)
{
  g_return_val_if_fail (GTK_IS_BUTTON (button), FALSE);
  
  return button->use_underline;
}

/**
 * gtk_button_set_use_stock:
 * @button: a #GtkButton
 * @use_stock: %TRUE if the button should use a stock item
 *
 * If true, the label set on the button is used as a
 * stock id to select the stock item for the button.
 */
void
gtk_button_set_use_stock (GtkButton *button,
			  gboolean   use_stock)
{
  g_return_if_fail (GTK_IS_BUTTON (button));

  use_stock = use_stock != FALSE;

  if (use_stock != button->use_stock)
    {
      button->use_stock = use_stock;
  
      gtk_button_construct_child (button);
      
      g_object_notify (G_OBJECT (button), "use-stock");
    }
}

/**
 * gtk_button_get_use_stock:
 * @button: a #GtkButton
 *
 * Returns whether the button label is a stock item.
 *
 * Return value: %TRUE if the button label is used to
 *               select a stock item instead of being
 *               used directly as the label text.
 */
gboolean
gtk_button_get_use_stock (GtkButton *button)
{
  g_return_val_if_fail (GTK_IS_BUTTON (button), FALSE);
  
  return button->use_stock;
}

/**
 * gtk_button_set_focus_on_click:
 * @button: a #GtkButton
 * @focus_on_click: whether the button grabs focus when clicked with the mouse
 * 
 * Sets whether the button will grab focus when it is clicked with the mouse.
 * Making mouse clicks not grab focus is useful in places like toolbars where
 * you don't want the keyboard focus removed from the main area of the
 * application.
 *
 * Since: 2.4
 **/
void
gtk_button_set_focus_on_click (GtkButton *button,
			       gboolean   focus_on_click)
{
  g_return_if_fail (GTK_IS_BUTTON (button));
  
  focus_on_click = focus_on_click != FALSE;

  if (button->focus_on_click != focus_on_click)
    {
      button->focus_on_click = focus_on_click;
      
      g_object_notify (G_OBJECT (button), "focus-on-click");
    }
}

/**
 * gtk_button_get_focus_on_click:
 * @button: a #GtkButton
 * 
 * Returns whether the button grabs focus when it is clicked with the mouse.
 * See gtk_button_set_focus_on_click().
 *
 * Return value: %TRUE if the button grabs focus when it is clicked with
 *               the mouse.
 *
 * Since: 2.4
 **/
gboolean
gtk_button_get_focus_on_click (GtkButton *button)
{
  g_return_val_if_fail (GTK_IS_BUTTON (button), FALSE);
  
  return button->focus_on_click;
}

/**
 * gtk_button_set_alignment:
 * @button: a #GtkButton
 * @xalign: the horizontal position of the child, 0.0 is left aligned, 
 *   1.0 is right aligned
 * @yalign: the vertical position of the child, 0.0 is top aligned, 
 *   1.0 is bottom aligned
 *
 * Sets the alignment of the child. This property has no effect unless 
 * the child is a #GtkMisc or a #GtkAligment.
 *
 * Since: 2.4
 */
void
gtk_button_set_alignment (GtkButton *button,
			  gfloat     xalign,
			  gfloat     yalign)
{
  GtkButtonPrivate *priv;

  g_return_if_fail (GTK_IS_BUTTON (button));
  
  priv = GTK_BUTTON_GET_PRIVATE (button);

  priv->xalign = xalign;
  priv->yalign = yalign;
  priv->align_set = 1;

  maybe_set_alignment (button, GTK_BIN (button)->child);

  g_object_freeze_notify (G_OBJECT (button));
  g_object_notify (G_OBJECT (button), "xalign");
  g_object_notify (G_OBJECT (button), "yalign");
  g_object_thaw_notify (G_OBJECT (button));
}

/**
 * gtk_button_get_alignment:
 * @button: a #GtkButton
 * @xalign: return location for horizontal alignment
 * @yalign: return location for vertical alignment
 *
 * Gets the alignment of the child in the button.
 *
 * Since: 2.4
 */
void
gtk_button_get_alignment (GtkButton *button,
			  gfloat    *xalign,
			  gfloat    *yalign)
{
  GtkButtonPrivate *priv;

  g_return_if_fail (GTK_IS_BUTTON (button));
  
  priv = GTK_BUTTON_GET_PRIVATE (button);
 
  if (xalign) 
    *xalign = priv->xalign;

  if (yalign)
    *yalign = priv->yalign;
}

/**
 * _gtk_button_set_depressed:
 * @button: a #GtkButton
 * @depressed: %TRUE if the button should be drawn with a recessed shadow.
 * 
 * Sets whether the button is currently drawn as down or not. This is 
 * purely a visual setting, and is meant only for use by derived widgets
 * such as #GtkToggleButton.
 **/
void
_gtk_button_set_depressed (GtkButton *button,
			   gboolean   depressed)
{
  GtkWidget *widget = GTK_WIDGET (button);

  depressed = depressed != FALSE;

  if (depressed != button->depressed)
    {
      button->depressed = depressed;
      gtk_widget_queue_resize (widget);
    }
}

/**
 * hildon_gtk_button_set_depressed:
 * @button: a #GtkButton
 * @depressed: whether the button is depressed
 *
 * Exactly the same as private _gtk_button_set_depressed() but exported for
 * hildon. Basically a Hildon wrapper for _gtk_button_set_depressed.
 *
 * Sets whether the button is currently drawn as down or not. This is
 * purely a visual setting, and is meant only for use by derived widgets
 * such as #GtkToggleButton.
 *
 * Since: maemo 2.0
 */
void
hildon_gtk_button_set_depressed (GtkButton *button,
				 gboolean   depressed)
{
  _gtk_button_set_depressed (button, depressed);
}

static void
gtk_button_update_state (GtkButton *button)
{
  gboolean depressed;
  GtkStateType new_state;

  if (button->activate_timeout)
    depressed = button->depress_on_activate;
  else
    depressed = button->in_button && button->button_down;

  if (button->in_button && (!button->button_down || !depressed))
    new_state = GTK_STATE_PRELIGHT;
  else
    new_state = depressed ? GTK_STATE_ACTIVE : GTK_STATE_NORMAL;

  _gtk_button_set_depressed (button, depressed); 
  gtk_widget_set_state (GTK_WIDGET (button), new_state);
}

static void 
show_image_change_notify (GtkButton *button)
{
  GtkButtonPrivate *priv = GTK_BUTTON_GET_PRIVATE (button);

  if (priv->image) 
    {
      if (show_image (button))
	gtk_widget_show (priv->image);
      else
	gtk_widget_hide (priv->image);
    }
}

static void
traverse_container (GtkWidget *widget,
		    gpointer   data)
{
  if (GTK_IS_BUTTON (widget))
    show_image_change_notify (GTK_BUTTON (widget));
  else if (GTK_IS_CONTAINER (widget))
    gtk_container_forall (GTK_CONTAINER (widget), traverse_container, NULL);
}

static void
gtk_button_setting_changed (GtkSettings *settings)
{
  GList *list, *l;

  list = gtk_window_list_toplevels ();

  for (l = list; l; l = l->next)
    gtk_container_forall (GTK_CONTAINER (l->data), 
			  traverse_container, NULL);

  g_list_free (list);
}


static void
gtk_button_screen_changed (GtkWidget *widget,
			   GdkScreen *previous_screen)
{
  GtkSettings *settings;
  guint show_image_connection;

  if (!gtk_widget_has_screen (widget))
    return;

  settings = gtk_widget_get_settings (widget);

  show_image_connection = 
    GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (settings), 
					 "gtk-button-connection"));
  
  if (show_image_connection)
    return;

  show_image_connection =
    g_signal_connect (settings, "notify::gtk-button-images",
		      G_CALLBACK (gtk_button_setting_changed), 0);
  g_object_set_data (G_OBJECT (settings), 
		     "gtk-button-connection",
		     GUINT_TO_POINTER (show_image_connection));

  show_image_change_notify (GTK_BUTTON (widget));
}

static void
gtk_button_state_changed (GtkWidget    *widget,
                          GtkStateType  previous_state)
{
  GtkButton *button = GTK_BUTTON (widget);

  if (!GTK_WIDGET_IS_SENSITIVE (widget))
    {
      button->in_button = FALSE;
      gtk_real_button_released (button);
    }
}

static void
gtk_button_grab_notify (GtkWidget *widget,
			gboolean   was_grabbed)
{
  GtkButton *button = GTK_BUTTON (widget);

  if (!was_grabbed)
    {
      button->in_button = FALSE;
      gtk_real_button_released (button);
    }
}

/**
 * gtk_button_set_image:
 * @button: a #GtkButton
 * @image: a widget to set as the image for the button
 *
 * Set the image of @button to the given widget. Note that
 * it depends on the gtk-button-images setting whether the
 * image will be displayed or not.
 *
 * Since: 2.6
 */ 
void
gtk_button_set_image (GtkButton *button,
		      GtkWidget *image)
{
  GtkButtonPrivate *priv = GTK_BUTTON_GET_PRIVATE (button);

  priv->image = image;
  priv->image_is_stock = (image == NULL);

  gtk_button_construct_child (button);

  g_object_notify (G_OBJECT (button), "image");
}

/**
 * gtk_button_get_image:
 * @button: a #GtkButton
 *
 * Gets the widget that is currenty set as the image of @button.
 * This may have been explicitly set by gtk_button_set_image()
 * or constructed by gtk_button_new_from_stock().
 *
 * Return value: a #GtkWidget or %NULL in case there is no image
 *
 * Since: 2.6
 */
GtkWidget *
gtk_button_get_image (GtkButton *button)
{
  GtkButtonPrivate *priv;

  g_return_val_if_fail (GTK_IS_BUTTON (button), NULL);

  priv = GTK_BUTTON_GET_PRIVATE (button);
  
  return priv->image;
}

/**
 * osso_gtk_button_set_detail_from_attach_flags:
 * @button: a #GtkButton
 * @flags: #OssoGtkButtonAttachFlags
 *
 * This function sets "automatic-detail" to %FALSE and uses the %flags to set 
 * the detail.
 *
 * Since: maemo 1.0
 */
void osso_gtk_button_set_detail_from_attach_flags (GtkButton *button,OssoGtkButtonAttachFlags flags)
{
  g_return_if_fail (GTK_IS_BUTTON (button));
  g_object_set (G_OBJECT (button),
                "automatic_detail",
		FALSE,
		"detail",
		osso_gtk_button_attach_details[flags],
		NULL);
} 

#define __GTK_BUTTON_C__
#include "gtkaliasdef.c"
