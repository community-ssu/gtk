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
 * SECTION:hailtogglebutton
 * @short_description: Implementation of the ATK interfaces for a #HailToggleButton.
 * @see_also: #GtkToggleButton, #HNAppButton
 *
 * #HailToggleButton implements the required ATK interfaces of #AtkObject.
 * In particular it exposes (for HNAppButton):
 * <itemizedlist>
 * <listitem>The embedded control. It gets the name from the #HDWMEntryInfo taken the name
 it represents,
 * taken it from the #HDWMEntryInfo. When no applications are running, a
 * default name is given.</listitem>
 * <listitem>Added behaviour to action of clicking the toggle button</listitem>
 * </itemizedlist>
 */

#include <gtk/gtktogglebutton.h>
#include <libhildonwm/hd-wm-entry-info.h>
#include <libhildonwm/hd-wm.h>
#include "hailtogglebutton.h"

static void                  hail_toggle_button_class_init       (HailToggleButtonClass *klass);
static void                  hail_toggle_button_object_init      (HailToggleButton      *toggle_button);
static void                  hail_toggle_button_finalize         (GObject               *object);

static G_CONST_RETURN gchar *hail_toggle_button_get_name         (AtkObject *obj);
static G_CONST_RETURN gchar *hail_toggle_button_get_description  (AtkObject *obj);

/* AtkAction.h */
static void                  hail_toggle_button_atk_action_interface_init
                                                                 (AtkActionIface *iface);
static gboolean              hail_toggle_button_action_do_action (AtkAction      *action,
                                                                  gint            index);

static void                  hail_toggle_button_real_initialize  (AtkObject *obj,
                                                                  gpointer data);

static void                  hail_toggle_button_app_button_entry_changed_cb
                                                                 (GObject *toggle_button,
                                                                  GParamSpec *spec,
                                                                  gpointer data);

#define HAIL_HN_APP_BUTTON_DEFAULT_NAME "HNAppButton"
#define HAIL_HN_APP_BUTTON_DEFAULT_DESCRIPTION "A single HNAppButton"
#define HAIL_TOGGLE_BUTTON_DEFAULT_NAME "Toggle Button"
#define HAIL_TOGGLE_BUTTON_DEFAULT_DESCRIPTION "A single toggle button"

typedef struct _HailToggleButtonPrivate HailToggleButtonPrivate;
struct _HailToggleButtonPrivate
{
  /* name to expose */
  /* not freed at finalize because it is constant */
  const gchar *name;
};

#define HAIL_TOGGLE_BUTTON_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_TOGGLE_BUTTON, HailToggleButtonPrivate))

static GType parent_type;
static GType hn_app_button_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_toggle_button_get_type:
 * 
 * Initialises, and returns the type of a #HailToggleButton.
 * 
 * Return value: GType of #HailToggleButton.
 **/
GType
hail_toggle_button_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailToggleButtonClass),
        (GBaseInitFunc) NULL,      /* base init */
        (GBaseFinalizeFunc) NULL,  /* base finalize */
        (GClassInitFunc) hail_toggle_button_class_init,     /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL,                      /* class data */
        (guint16) sizeof (HailToggleButton),                /* instance size */
        0,                         /* nb preallocs */
        (GInstanceInitFunc) hail_toggle_button_object_init, /* instance init */
        NULL                       /* value table */
      };

      static const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) hail_toggle_button_atk_action_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_TOGGLE_BUTTON);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailToggleButton", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);

    }

  return type;
}

static void
hail_toggle_button_class_init (HailToggleButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_toggle_button_finalize;

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailToggleButtonPrivate));
  
  /* bind virtual methods for AtkObject */
  class->get_name        = hail_toggle_button_get_name;
  class->get_description = hail_toggle_button_get_description;
  class->initialize      = hail_toggle_button_real_initialize;
}

static void
hail_toggle_button_object_init (HailToggleButton *toggle_button)
{
  HailToggleButtonPrivate *priv = NULL;
  
  g_return_if_fail (HAIL_IS_TOGGLE_BUTTON (toggle_button));
  priv = HAIL_TOGGLE_BUTTON_GET_PRIVATE(HAIL_TOGGLE_BUTTON(toggle_button));
  
  /* type */
  hn_app_button_type = g_type_from_name ("HNAppButton");

  /* default values */
  priv->name = NULL;
}

static void
hail_toggle_button_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * hail_toggle_button_new:
 * @widget: a #GtkToggleButton (HNAppMenuItem) casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #GtkToggleButton.
 * 
 * Return value: An #AtkObject
 **/
AtkObject *
hail_toggle_button_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (GTK_IS_TOGGLE_BUTTON(widget), NULL);

  object = g_object_new (HAIL_TYPE_TOGGLE_BUTTON, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * Implementation of AtkObject method get_name()
 */
static G_CONST_RETURN gchar *
hail_toggle_button_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar *name = NULL;
  GtkWidget *toggle_button = NULL;

  g_return_val_if_fail (HAIL_IS_TOGGLE_BUTTON (obj), NULL);
  toggle_button = GTK_ACCESSIBLE(obj)->widget;

  /* parent name */
  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);

  if (name == NULL)
    {
      /* concrete implementation for HNAppButton */
      if (g_type_is_a (G_TYPE_FROM_INSTANCE(toggle_button), hn_app_button_type))
	{
	  HailToggleButtonPrivate *priv = NULL;
	  priv = HAIL_TOGGLE_BUTTON_GET_PRIVATE(HAIL_TOGGLE_BUTTON(obj));
	  
	  /* updated by the callback (hail_toggle_button_app_button_entry_changed_cb) */
	  name = priv->name;
	  
        }

      /* for GtkToggleButton */
      else
        {
          name = HAIL_TOGGLE_BUTTON_DEFAULT_NAME;
        }
    }

  return name;
}

/*
 * Implementation of AtkObject method get_description()
 */
static G_CONST_RETURN gchar *
hail_toggle_button_get_description (AtkObject *obj)
{
  G_CONST_RETURN gchar *description = NULL;
  GtkWidget *toggle_button = NULL;

  g_return_val_if_fail (HAIL_IS_TOGGLE_BUTTON (obj), NULL);
  toggle_button = GTK_ACCESSIBLE(obj)->widget;

  /* parent description */
  description = ATK_OBJECT_CLASS (parent_class)->get_description (obj);

  if (description == NULL)
    {
      /* concrete implementation for HNAppButton */
      if (g_type_is_a (G_TYPE_FROM_INSTANCE(toggle_button), hn_app_button_type))
	{
	  description = HAIL_HN_APP_BUTTON_DEFAULT_DESCRIPTION;
	}

      /* for GtkToggleButton */
      else
	{
	  description = HAIL_TOGGLE_BUTTON_DEFAULT_DESCRIPTION;
	}
    }

  return description;
}

/*
 * Initializes the AtkAction interface, and binds the virtual methods
 */
static void
hail_toggle_button_atk_action_interface_init (AtkActionIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->do_action = hail_toggle_button_action_do_action;
}

/*
 * Implementation of AtkAction method do_action()
 */
static gboolean
hail_toggle_button_action_do_action (AtkAction *action,
				     gint       index)
{
  GtkWidget *toggle_button = NULL;

  g_return_val_if_fail (HAIL_IS_TOGGLE_BUTTON (action), FALSE);

  toggle_button = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (GTK_IS_TOGGLE_BUTTON (toggle_button), FALSE);

  /* apply parent behaviour */
  AtkActionIface *parent_action_iface =
    g_type_interface_peek_parent (ATK_ACTION_GET_IFACE (action));
  
  if (parent_action_iface->do_action(action, index) == FALSE)
    {
      return FALSE;
    }

  /* implementation for HNAppButton */
  if (g_type_is_a (G_TYPE_FROM_INSTANCE(toggle_button), hn_app_button_type))
    {
      /* we want to add behaviour for "click" action */
      if (index == 0)
	{
	  /* top the selected app:
	   * see hn-app-button:hn_app_button_pop_menu for details
	   */
          HDWMEntryInfo *info = NULL;

          /* entry-info is a pointer property,
           * so we mustn't free it with hd_entry_info_free
           */
          g_object_get (G_OBJECT(toggle_button),
                        "entry-info", &info,
                        NULL);

          if (info != NULL)
            {   
	      hd_wm_top_item (info);
	    }
	}
    }

  return TRUE;
}

/*
 * Implementation of AtkObject method initialize()
 */
static void
hail_toggle_button_real_initialize (AtkObject *obj, gpointer data)
{
  HailToggleButtonPrivate *priv = NULL;
  GtkWidget *toggle_button = NULL;

  g_return_if_fail (HAIL_IS_TOGGLE_BUTTON (obj));
  priv = HAIL_TOGGLE_BUTTON_GET_PRIVATE(HAIL_TOGGLE_BUTTON(obj));

  /* parent initialize */
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  toggle_button = GTK_ACCESSIBLE(obj)->widget;

  /* accessibility HNAppButton initialize */
  if (g_type_is_a (G_TYPE_FROM_INSTANCE(toggle_button), hn_app_button_type))
    {
      HDWMEntryInfo *info = NULL;

      /* entry-info is a pointer property,
       * so we mustn't free it with hd_entry_info_free
       */
      g_object_get (G_OBJECT(toggle_button),
                    "entry-info", &info,
                    NULL);

      if (info != NULL)
        {
          /* name of the application */
          priv->name = hd_wm_entry_info_peek_app_name (info);
        }
      else
        {
          /* default name */
          priv->name = HAIL_HN_APP_BUTTON_DEFAULT_NAME;
        }

      /* connect for changes in the HDWMEntryInfo */
      g_signal_connect (G_OBJECT(toggle_button), "notify",
                        G_CALLBACK(hail_toggle_button_app_button_entry_changed_cb), obj); 
    }
}

static void
hail_toggle_button_app_button_entry_changed_cb (GObject *toggle_button,
                                                GParamSpec *spec,
                                                gpointer data)
{
  HailToggleButtonPrivate *priv = NULL;
  AtkObject *obj = ATK_OBJECT(data);

  priv = HAIL_TOGGLE_BUTTON_GET_PRIVATE(HAIL_TOGGLE_BUTTON(obj));

  /* implementation for HNAppButton */
  if (g_type_is_a (G_TYPE_FROM_INSTANCE(toggle_button), hn_app_button_type))
    {
      /* check for HDWMEntryInfo property changes */
      if (g_ascii_strcasecmp (spec->name, "entry-info"))
        {
          HDWMEntryInfo *info = NULL;

          /* entry-info is a pointer property,
           * so we mustn't free it with hd_entry_info_free
           */
          g_object_get (G_OBJECT(toggle_button),
                        "entry-info", &info,
                        NULL);
          
          if (info != NULL)
            {
              /* name of the application */
              priv->name = hd_wm_entry_info_peek_app_name (info);
            }
	  else
	    {
              priv->name = HAIL_HN_APP_BUTTON_DEFAULT_NAME;
	    }
        }
    }
}

