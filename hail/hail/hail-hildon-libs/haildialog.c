/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2005 Nokia Corporation.
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
 * SECTION:haildialog
 * @short_description: Implementation of the ATK interfaces for #GtkDialog, 
 * taking into account the possibility that the help hint is enabled
 * (see hildon_dialog_help_enable()).
 * @see_also: hildon_dialog_help_enable(), hildon-dialog
 *
 * #HailDialog implements the required ATK interfaces of #GtkDialog. It overrides #GailWindow
 * to implement the help hint exposed in hildon dialog help methods. It handles the window manager
 * trick used in the hildon stuff, through an action for all the dialogs.
 */

#include <gdk/gdkx.h>
#include <gtk/gtkdialog.h>
#include "haildialog.h"

static void                  hail_dialog_class_init       (HailDialogClass *klass);
static void                  hail_dialog_object_init      (HailDialog      *dialog);


/* AtkAction */
static void                  hail_dialog_atk_action_interface_init   (AtkActionIface  *iface);
static gint                  hail_dialog_action_get_n_actions    (AtkAction       *action);
static gboolean              hail_dialog_action_do_action        (AtkAction       *action,
								  gint            index);
static const gchar*          hail_dialog_action_get_name         (AtkAction       *action,
								  gint            index);
static const gchar*          hail_dialog_action_get_description  (AtkAction       *action,
								  gint            index);
static const gchar*          hail_dialog_action_get_keybinding   (AtkAction       *action,
								  gint            index);


static GType parent_type;
static GtkAccessibleClass* parent_class = NULL;

#define HAIL_DIALOG_HELP_ACTION_NAME "Help"
#define HAIL_DIALOG_HELP_ACTION_DESCRIPTION "Help about this dialog"

/**
 * hail_dialog_get_type:
 * 
 * Initialises, and returns the type of a hail dialog.
 * 
 * Return value: GType of #HailDialog.
 **/
GType
hail_dialog_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
	{
	  (guint16) sizeof (HailDialogClass),
	  (GBaseInitFunc) NULL, /* base init */
	  (GBaseFinalizeFunc) NULL, /* base finalize */
	  (GClassInitFunc) hail_dialog_class_init, /* class init */
	  (GClassFinalizeFunc) NULL, /* class finalize */
	  NULL, /* class data */
	  (guint16) sizeof (HailDialog), /* instance size */
	  0, /* nb preallocs */
	  (GInstanceInitFunc) hail_dialog_object_init, /* instance init */
	  NULL /* value table */
	};

      static const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) hail_dialog_atk_action_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_DIALOG);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;


      type = g_type_register_static (parent_type,
                                     "HailDialog", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);

    }

  return type;
}

static void
hail_dialog_class_init (HailDialogClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);


}

static void
hail_dialog_object_init (HailDialog      *dialog)
{
}


/**
 * hail_dialog_new:
 * @widget: a #HildonDialog casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonDialog.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_dialog_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (GTK_IS_DIALOG (widget), NULL);

  object = g_object_new (HAIL_TYPE_DIALOG, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Initializes the AtkAction interface, and binds the virtual methods */
static void
hail_dialog_atk_action_interface_init (AtkActionIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->do_action = hail_dialog_action_do_action;
  iface->get_n_actions = hail_dialog_action_get_n_actions;
  iface->get_name = hail_dialog_action_get_name;
  iface->get_description = hail_dialog_action_get_description;
  iface->get_keybinding = hail_dialog_action_get_keybinding;
}

/* Implementation of AtkAction method get_n_actions */
static gint
hail_dialog_action_get_n_actions    (AtkAction       *action)
{
  GtkWidget * dialog = NULL;
  g_return_val_if_fail (HAIL_IS_DIALOG (action), 0);

  dialog = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (GTK_IS_DIALOG(dialog), 0);

  return 1;
}

/* Implementation of AtkAction method do_action */
static gboolean
hail_dialog_action_do_action        (AtkAction       *action,
				     gint            index)
{
  GdkWindow *window;
  GdkDisplay *display;
  Atom *protocols;
  Atom *list;
  Atom helpatom;
  GtkWidget * dialog = NULL;
  gboolean help_enabled = FALSE;
  int n = 0;
  int i = 0;
  int amount = 0;

  /* In this method we emulate the behaviour of the Gtk dialog help hint system,
     used in Maemo with the window manager */

  g_return_val_if_fail (HAIL_IS_DIALOG (action), FALSE);
  g_return_val_if_fail ((index == 0), FALSE);

  dialog = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (GTK_IS_DIALOG(dialog), FALSE);

  gtk_widget_realize(GTK_WIDGET(dialog));
  window = GTK_WIDGET(dialog)->window;
  display = gdk_drawable_get_display (window);
  
  /* We get the protocols and obtain the _NET_WM_CONTEXT_HELP hint */
  XGetWMProtocols(GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (window),
		  &list, &amount);
    
  protocols = (Atom *) g_malloc0 ((amount+1) * sizeof (Atom));
  helpatom = gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_CONTEXT_HELP");

  for (i=0; i<amount; i++)
    {
      protocols[n++] = list[i];
      if (list[i] == helpatom)
	{
	  help_enabled = TRUE;
	}
    }
  XFree (list);

  /* If help is available, we emit the corresponding signal */
  if (help_enabled) {
    g_signal_emit_by_name(dialog, "help");
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Implementation of AtkAction method get_name */
static const gchar*
hail_dialog_action_get_name         (AtkAction       *action,
				       gint            index)
{
  GtkWidget * dialog = NULL;
  g_return_val_if_fail (HAIL_IS_DIALOG (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  dialog = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (GTK_IS_DIALOG(dialog), NULL);

  return HAIL_DIALOG_HELP_ACTION_NAME;
}

/* Implementation of AtkAction method get_description */
static const gchar*
hail_dialog_action_get_description   (AtkAction       *action,
					gint            index)
{
  GtkWidget * dialog = NULL;

  g_return_val_if_fail (HAIL_IS_DIALOG (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  dialog = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (GTK_IS_DIALOG(dialog), NULL);

  return HAIL_DIALOG_HELP_ACTION_DESCRIPTION;
}

/* Implementation of AtkAction method get_keybinding */
static const gchar*
hail_dialog_action_get_keybinding   (AtkAction       *action,
				       gint            index)
{
  GtkWidget * dialog = NULL;

  g_return_val_if_fail (HAIL_IS_DIALOG (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  dialog = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (GTK_IS_DIALOG(dialog), NULL);

  return NULL;
}
