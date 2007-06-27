/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2007 Nokia Corporation.
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

/**
 * SECTION:haildesktoppopupwindow
 * @short_description: Implementation of the ATK interfaces for a #HailDesktopPopupMenu.
 * @see_also: #HildonDesktopPopupMenu, #HildonDesktopPopupWindow, #HailDesktopPopupWindow.
 *
 * #HailDesktopPopupMenu implements the required ATK interfaces of #HailDesktopPopupMenu.
 * In particular it exposes:
 * <itemizedlist>
 * <listitem>The embedded control. It gets a default name (<literal>control</literal>).</listitem>
 * <listitem>The #HildonHomeTitleBar widget as a child.</listitem>
 * </itemizedlist>
 * A valid name and description is given by the superclass #HailDesktopWindow.
 */

#include <libhildondesktop/hildon-desktop-popup-menu.h>
#include "haildesktoppopupmenu.h"
#include <gtk/gtkhbox.h>

static void       hail_desktop_popup_menu_class_init          (HailDesktopPopupMenuClass *klass);
static void       hail_desktop_popup_menu_object_init         (HailDesktopPopupMenu      *popup_menu);
static void       hail_desktop_popup_menu_finalize            (GObject                   *object);

static G_CONST_RETURN gchar *hail_desktop_popup_menu_get_name         (AtkObject *obj);
static G_CONST_RETURN gchar *hail_desktop_popup_menu_get_description  (AtkObject *obj);

static gint       hail_desktop_popup_menu_get_n_children      (AtkObject *obj);
static AtkObject *hail_desktop_popup_menu_ref_child           (AtkObject *obj,
							       gint       i);

/* AtkSelection */
static void       atk_selection_interface_init                (AtkSelectionIface *iface);
static gboolean   hail_desktop_popup_menu_add_selection       (AtkSelection *selection,
							       gint          i);
static gboolean   hail_desktop_popup_menu_clear_selection     (AtkSelection *selection);
static AtkObject *hail_desktop_popup_menu_ref_selection       (AtkSelection *selection,
							       gint          i);
static gint       hail_desktop_popup_menu_get_selection_count (AtkSelection *selection);
static gboolean   hail_desktop_popup_menu_is_child_selected   (AtkSelection *selection,
							       gint          i);
static gboolean   hail_desktop_popup_menu_remove_selection    (AtkSelection *selection,
							       gint          i);

static void       add_internal_widgets                        (GtkWidget *widget,
							       gpointer data);
static void       hail_desktop_popup_menu_real_initialize     (AtkObject *obj,
							       gpointer data);
static void       hail_desktop_popup_menu_update_widgets      (AtkObject *obj);

static void  	  hail_desktop_popup_menu_show_controls_cb    ();


#define HAIL_DESKTOP_POPUP_MENU_DEFAULT_NAME "Hildon Desktop Popup Menu"
#define HAIL_DESKTOP_POPUP_MENU_DEFAULT_DESCRIPTION "The menu which shows the popup menus"

typedef struct _HailDesktopPopupMenuPrivate HailDesktopPopupMenuPrivate;
struct _HailDesktopPopupMenuPrivate
{
  GList *children;   /* each one of the menu items */
  guint	 n_children; /* number of menu items */
  GtkWidget *box_buttons; /* scroll buttons */
};

#define HAIL_DESKTOP_POPUP_MENU_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_DESKTOP_POPUP_MENU, HailDesktopPopupMenuPrivate))

static GType parent_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_desktop_popup_menu_get_type:
 * 
 * Initialises, and returns the type of a #HailDesktopPopupMenu.
 * 
 * Return value: GType of #HailDesktopPopupMenu.
 **/
GType
hail_desktop_popup_menu_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailDesktopPopupMenuClass),
        (GBaseInitFunc) NULL,      /* base init */
        (GBaseFinalizeFunc) NULL,  /* base finalize */
        (GClassInitFunc) hail_desktop_popup_menu_class_init,     /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL,                      /* class data */
        (guint16) sizeof (HailDesktopPopupMenu),                 /* instance size */
        0,                         /* nb preallocs */
        (GInstanceInitFunc) hail_desktop_popup_menu_object_init, /* instance init */
        NULL                       /* value table */
      };

      static const GInterfaceInfo atk_selection_info =
	{
	  (GInterfaceInitFunc) atk_selection_interface_init,
	  (GInterfaceFinalizeFunc) NULL,
	  NULL
	};

      factory = atk_registry_get_factory (atk_get_default_registry (),
					  HILDON_DESKTOP_TYPE_POPUP_MENU);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailDesktopPopupMenu", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_SELECTION,
                                   &atk_selection_info);
    }

  return type;
}

static void
hail_desktop_popup_menu_class_init (HailDesktopPopupMenuClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_desktop_popup_menu_finalize;

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailDesktopPopupMenuPrivate));

  /* bind virtual methods */
  class->get_name        = hail_desktop_popup_menu_get_name;
  class->get_description = hail_desktop_popup_menu_get_description;
  class->get_n_children  = hail_desktop_popup_menu_get_n_children;
  class->ref_child       = hail_desktop_popup_menu_ref_child;
  class->initialize      = hail_desktop_popup_menu_real_initialize;
}

static void
hail_desktop_popup_menu_object_init (HailDesktopPopupMenu *popup_menu)
{
  HailDesktopPopupMenuPrivate *priv = NULL;

  g_return_if_fail (HAIL_IS_DESKTOP_POPUP_MENU (popup_menu));
  priv = HAIL_DESKTOP_POPUP_MENU_GET_PRIVATE (popup_menu);

  priv->children = NULL;
  priv->n_children = 0;
  priv->box_buttons = NULL;
}

static void
hail_desktop_popup_menu_finalize (GObject *object)
{
  HailDesktopPopupMenu *popup_menu = NULL;
  HailDesktopPopupMenuPrivate *priv  = NULL;

  g_return_if_fail (HAIL_IS_DESKTOP_POPUP_MENU (object));
  popup_menu = HAIL_DESKTOP_POPUP_MENU (object);
  priv = HAIL_DESKTOP_POPUP_MENU_GET_PRIVATE (popup_menu);

  if (priv->children)
    {
      g_list_free (priv->children);
      priv->children = NULL;
    }

  priv->box_buttons = NULL;
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * hail_desktop_popup_menu_new:
 * @widget: a #HildonDesktopPopupMenu casted as a #GtkWidget.
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonDesktopPopupMenu.
 * 
 * Return value: An #AtkObject.
 **/
AtkObject *
hail_desktop_popup_menu_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_DESKTOP_IS_POPUP_MENU(widget), NULL);

  object = g_object_new (HAIL_TYPE_DESKTOP_POPUP_MENU, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * Implementation of AtkObject method get_name()
 */
static G_CONST_RETURN gchar *
hail_desktop_popup_menu_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar *name = NULL;

  g_return_val_if_fail (HAIL_IS_DESKTOP_POPUP_MENU (obj), NULL);

  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);
  if (name == NULL)
    {
      name = "Hildon Desktop Popup Menu";
    }

  return name;
}

/*
 * Implementation of AtkObject method get_description()
 */
static G_CONST_RETURN gchar *
hail_desktop_popup_menu_get_description (AtkObject *obj)
{
  G_CONST_RETURN gchar *description = NULL;

  g_return_val_if_fail (HAIL_IS_DESKTOP_POPUP_MENU (obj), NULL);

  description = ATK_OBJECT_CLASS (parent_class)->get_description (obj);
  if (description == NULL)
    {
      /*
       * Default description
       */
      description = HAIL_DESKTOP_POPUP_MENU_DEFAULT_DESCRIPTION;
    }

  return description;
}

/*
 * Implementation of AtkObject method get_n_children()
 * At least the widget has the titlebar widget (#HildonHomeTitleBar).
 */
static gint
hail_desktop_popup_menu_get_n_children (AtkObject *obj)
{
  HailDesktopPopupMenuPrivate *priv  = NULL;
  gint n_children = 0;

  g_return_val_if_fail (HAIL_IS_DESKTOP_POPUP_MENU (obj), 0);

  priv = HAIL_DESKTOP_POPUP_MENU_GET_PRIVATE (HAIL_DESKTOP_POPUP_MENU (obj));

  /* take care of changes in the items and the scroll of the widget */
  hail_desktop_popup_menu_update_widgets (obj);

  if (priv->children)
    {
      n_children += priv->n_children;
    }

  if (priv->box_buttons != NULL)
    {
      n_children++;
    }

  return n_children;
}

/*
 * Implementation of AtkObject method ref_accessible_child()
 */
static AtkObject *
hail_desktop_popup_menu_ref_child (AtkObject *obj,
				   gint       i)
{
  HailDesktopPopupMenuPrivate *priv  = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HAIL_IS_DESKTOP_POPUP_MENU (obj), NULL);
  g_return_val_if_fail ((i >= 0), NULL);

  priv = HAIL_DESKTOP_POPUP_MENU_GET_PRIVATE (HAIL_DESKTOP_POPUP_MENU (obj));

  /* take care of changes in the items and the scroll of the widget */
  hail_desktop_popup_menu_update_widgets (obj);

  g_return_val_if_fail ((i < priv->n_children + ((priv->box_buttons != NULL) ? 1 : 0)), NULL);

  /* items */
  if ((priv->children != NULL) && (i < priv->n_children))
    {
      accessible = gtk_widget_get_accessible (GTK_WIDGET(g_list_nth_data (priv->children, i)));
      g_object_ref (accessible);
    }

  /* box_buttons */
  if ((i == priv->n_children) && (priv->box_buttons != NULL))
    {
      accessible = gtk_widget_get_accessible (priv->box_buttons);
      g_object_ref (accessible);
    }
    
  return accessible;
}

/*
 * Initializes the AtkSelection interface, and binds the virtual methods
 */
static void
atk_selection_interface_init (AtkSelectionIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->add_selection       = hail_desktop_popup_menu_add_selection;
  iface->clear_selection     = hail_desktop_popup_menu_clear_selection;
  iface->ref_selection       = hail_desktop_popup_menu_ref_selection;
  iface->get_selection_count = hail_desktop_popup_menu_get_selection_count;
  iface->is_child_selected   = hail_desktop_popup_menu_is_child_selected;
  iface->remove_selection    = hail_desktop_popup_menu_remove_selection;
  /* select_all_selection does not make sense for the popup menu */
}

static gboolean
hail_desktop_popup_menu_add_selection (AtkSelection *selection,
				       gint          i)
{
  HildonDesktopPopupMenu *popup_menu = NULL;
  GtkWidget *widget = NULL;
  GtkMenuItem *item = NULL;
  HailDesktopPopupMenuPrivate *priv  = NULL;

  g_return_val_if_fail (HAIL_IS_DESKTOP_POPUP_MENU (selection), FALSE);
  priv = HAIL_DESKTOP_POPUP_MENU_GET_PRIVATE (selection);

  if ((i < 0) || (i > priv->n_children))
    {
      return FALSE;
    }

  widget = GTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    {
      /*
       * State is defunct
       */
      return FALSE;
    }

  popup_menu = HILDON_DESKTOP_POPUP_MENU (widget);
  item = GTK_MENU_ITEM (g_list_nth_data (priv->children, i));
  g_return_val_if_fail (item != NULL, FALSE);

  hildon_desktop_popup_menu_select_item (popup_menu, item);

  return TRUE;
}

static gboolean 
hail_desktop_popup_menu_clear_selection (AtkSelection *selection)
{
  HildonDesktopPopupMenu *popup_menu = NULL;
  GtkWidget *widget = NULL;
  GtkMenuItem *item = NULL;

  widget = GTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    {
      /*
       * State is defunct
       */
      return FALSE;
    }

  popup_menu = HILDON_DESKTOP_POPUP_MENU (widget);
  item = hildon_desktop_popup_menu_get_selected_item (popup_menu);

  if (item != NULL)
    {
      /* deselect selected item */
      hildon_desktop_popup_menu_deselect_item (popup_menu, item);
    }

  return TRUE;
}

static AtkObject *
hail_desktop_popup_menu_ref_selection (AtkSelection *selection,
				       gint          i)
{
  HildonDesktopPopupMenu *popup_menu = NULL;
  GtkMenuItem *item = NULL;
  AtkObject *obj = NULL;
  GtkWidget *widget = NULL;
  HailDesktopPopupMenuPrivate *priv  = NULL;

  g_return_val_if_fail (HAIL_IS_DESKTOP_POPUP_MENU (selection), NULL);
  priv = HAIL_DESKTOP_POPUP_MENU_GET_PRIVATE (selection);

  if ((i < 0) || (i > priv->n_children))
    {
      return FALSE;
    }

  widget = GTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    {
      /*
       * State is defunct
       */
      return NULL;
    }

  popup_menu = HILDON_DESKTOP_POPUP_MENU (widget);
  item = hildon_desktop_popup_menu_get_selected_item (popup_menu);

  if (item == NULL)
    {
      return NULL;
    }

  obj = gtk_widget_get_accessible (GTK_WIDGET (item));
  g_object_ref (obj);

  return obj;
}

static gint
hail_desktop_popup_menu_get_selection_count (AtkSelection *selection)
{
  HildonDesktopPopupMenu *popup_menu = NULL;
  GtkWidget *widget = NULL;
  GtkMenuItem *item = NULL;

  widget = GTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    {
      /*
       * State is defunct
       */
      return 0;
    }

  popup_menu = HILDON_DESKTOP_POPUP_MENU (widget);

  item = hildon_desktop_popup_menu_get_selected_item (popup_menu);

  return (item != NULL) ? 1 : 0;
}

static gboolean
hail_desktop_popup_menu_is_child_selected (AtkSelection *selection,
					   gint          i)
{
  HildonDesktopPopupMenu *popup_menu = NULL;
  GtkMenuItem *child_i = NULL;
  GtkMenuItem *child_s = NULL;
  GtkWidget *widget = NULL;
  HailDesktopPopupMenuPrivate *priv = NULL;

  g_return_val_if_fail (HAIL_IS_DESKTOP_POPUP_MENU (selection), FALSE);
  priv = HAIL_DESKTOP_POPUP_MENU_GET_PRIVATE (selection);

  if ((i < 0) || (i > priv->n_children))
    {
      return FALSE;
    }

  widget = GTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    {
      /*
       * State is defunct
       */
      return FALSE;
    }

  popup_menu = HILDON_DESKTOP_POPUP_MENU (widget);

  /* selected item */
  child_s = hildon_desktop_popup_menu_get_selected_item (popup_menu);

  /* the item being asked */
  child_i = GTK_MENU_ITEM (g_list_nth_data (priv->children, i));

  return (child_s == child_i);
}

static gboolean
hail_desktop_popup_menu_remove_selection (AtkSelection *selection,
					  gint          i)
{
  if (atk_selection_is_child_selected (selection, i))
    {
      atk_selection_clear_selection (selection);
    }

  return TRUE;
}

/*
 * callback for gtk_container_forall.
 * It's used from atk object initialize to get the references
 * to the AtkObjects of the internal widgets.
 */
static void
add_internal_widgets (GtkWidget *widget, gpointer data)
{
  HailDesktopPopupMenu *popup_menu  = NULL;
  HailDesktopPopupMenuPrivate *priv = NULL;

  popup_menu = HAIL_DESKTOP_POPUP_MENU (data);
  priv = HAIL_DESKTOP_POPUP_MENU_GET_PRIVATE (popup_menu);

  /* box_buttons */
  if (GTK_IS_HBOX (widget))
    {
      priv->box_buttons = widget;
    }
}

/*
 * Implementation of AtkObject method initialize().
 */
static void
hail_desktop_popup_menu_real_initialize (AtkObject *obj, gpointer data)
{
  GtkWidget *widget = NULL;

  g_return_if_fail (HAIL_IS_DESKTOP_POPUP_MENU (obj));

  /* parent initialize */
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  /* box_items */
  hail_desktop_popup_menu_update_widgets (obj);

  /* box_butoms */
  widget = GTK_ACCESSIBLE(obj)->widget;
  gtk_container_forall (GTK_CONTAINER (widget),
			add_internal_widgets,
			obj);
}

/*
 * refactorized code.
 * called from get_n_children, ref_child and real_initialize.
 */
static void
hail_desktop_popup_menu_update_widgets (AtkObject *obj)
{
  HailDesktopPopupMenu *popup_menu = NULL;
  HailDesktopPopupMenuPrivate *priv = NULL;
  GtkWidget *widget = NULL;
  GList *tmp = NULL;

  popup_menu = HAIL_DESKTOP_POPUP_MENU (obj);
  priv = HAIL_DESKTOP_POPUP_MENU_GET_PRIVATE (popup_menu);
  widget = GTK_ACCESSIBLE(obj)->widget;

  /* box_items */
  if (priv->children)
    {
      g_list_free (priv->children);
      priv->children = NULL;
    }

  if (HILDON_DESKTOP_IS_POPUP_MENU (widget))
    {
      priv->children   =
	hildon_desktop_popup_menu_get_children (HILDON_DESKTOP_POPUP_MENU (widget));
      priv->n_children = g_list_length (priv->children);
    }

  for (tmp = priv->children; tmp; tmp = g_list_next (tmp))
    {
      atk_object_set_parent (gtk_widget_get_accessible (GTK_WIDGET (tmp->data)),
			     obj);
    }

  /* box_buttons */
  priv->box_buttons = NULL;
  gtk_container_forall (GTK_CONTAINER (widget),
			add_internal_widgets,
			obj);
}
