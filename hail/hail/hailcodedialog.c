/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2006 Nokia Corporation.
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
 * SECTION:hailcodedialog
 * @short_description: Implementation of the ATK interfaces for #HildonCodeDialog.
 * @see_also: #HildonCodeDialog
 *
 * #HailCodeDialog implements the required ATK interfaces of #HildonCodeDialog and its
 * children (the buttons inside the widget). 
 */

#include <hildon-widgets/hildon-code-dialog.h>
#include "hailcodedialog.h"


#define HAIL_CODE_DIALOG_DEFAULT_NAME "Code dialog"
#define HAIL_CODE_DIALOG_ERROR_MESSAGE "Message"
#define HAIL_CODE_DIALOG_PASSWORD_ENTRY "Code"
#define HAIL_CODE_DIALOG_BACKSPACE_BUTTON "Backspace"

typedef struct _HailCodeDialogPrivate HailCodeDialogPrivate;

static void                  hail_code_dialog_class_init       (HailCodeDialogClass *klass);
static void                  hail_code_dialog_object_init      (HailCodeDialog      *code_dialog);

static G_CONST_RETURN gchar* hail_code_dialog_get_name         (AtkObject       *obj);
static void                  hail_code_dialog_real_initialize  (AtkObject       *obj,
								gpointer        data);

static GType parent_type;
static GtkAccessible * parent_class = NULL;

/**
 * hail_code_dialog_get_type:
 * 
 * Initialises, and returns the type of a hail code_dialog.
 * 
 * Return value: GType of #HailCodeDialog.
 **/
GType
hail_code_dialog_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailCodeDialogClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) hail_code_dialog_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        (guint16) sizeof (HailCodeDialog), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) hail_code_dialog_object_init, /* instance init */
        NULL /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_DIALOG);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailCodeDialog", &tinfo, 0);

    }

  return type;
}

static void
hail_code_dialog_class_init (HailCodeDialogClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods for AtkObject */
  class->get_name = hail_code_dialog_get_name;
  class->initialize = hail_code_dialog_real_initialize;

}

/* nullify or initialize attributes */
static void
hail_code_dialog_object_init (HailCodeDialog      *code_dialog)
{
}

/**
 * hail_code_dialog_new:
 * @widget: a #HildonCodeDialog casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonCodeDialog.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_code_dialog_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_CODE_DIALOG (widget), NULL);

  object = g_object_new (HAIL_TYPE_CODE_DIALOG, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_code_dialog_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_CODE_DIALOG (obj), NULL);

  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);
  if (name == NULL)
    {
      /*
       * Get the text on the label
       */
      GtkWidget *widget = NULL;

      widget = GTK_ACCESSIBLE (obj)->widget;
      if (widget == NULL)
        /*
         * State is defunct
         */
        return NULL;

      g_return_val_if_fail (HILDON_IS_CODE_DIALOG (widget), NULL);

      /* It gets the name from the code_dialog label */
      name = HAIL_CODE_DIALOG_DEFAULT_NAME;

    }
  return name;
}

/* Implementation of AtkObject method initialize() */
static void
hail_code_dialog_real_initialize (AtkObject *obj,
                             gpointer   data)
{
  AtkObject * main_filler = NULL;
  AtkObject * data_area_filler = NULL;
  AtkObject * tmp_container = NULL;
  AtkObject * tmp_accessible = NULL;

  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_PANEL;

  /* first obtain the filler containing the buttons, the message
   * code and the error message
   */
  main_filler = atk_object_ref_accessible_child(obj, 0);
  g_return_if_fail(ATK_IS_OBJECT(main_filler));
  data_area_filler = atk_object_ref_accessible_child(main_filler, 0);
  g_object_unref(main_filler);
  g_return_if_fail (ATK_IS_OBJECT(data_area_filler));

  /* obtain the error message label */
  tmp_container = atk_object_ref_accessible_child(data_area_filler, 0);
  g_return_if_fail(ATK_IS_OBJECT(tmp_container));
  tmp_accessible = atk_object_ref_accessible_child(tmp_container, 0);
  g_return_if_fail (ATK_IS_OBJECT(tmp_accessible));
  /* set the default atk name */
  atk_object_set_name(tmp_accessible, HAIL_CODE_DIALOG_ERROR_MESSAGE);
  g_object_unref(tmp_accessible);
  g_object_unref(tmp_container);

  /* obtain the password entry */
  tmp_accessible = atk_object_ref_accessible_child(data_area_filler, 1);
  g_return_if_fail (ATK_IS_OBJECT(tmp_accessible));
  /* set the default atk name */
  atk_object_set_name(tmp_accessible, HAIL_CODE_DIALOG_PASSWORD_ENTRY);
  g_object_unref(tmp_accessible);

  /* obtain the backspace button */
  tmp_container = atk_object_ref_accessible_child(data_area_filler, 2);
  g_return_if_fail(ATK_IS_OBJECT(tmp_container));
  tmp_accessible = atk_object_ref_accessible_child(tmp_container, 0);
  g_return_if_fail (ATK_IS_OBJECT(tmp_accessible));
  /* set the default atk name */
  atk_object_set_name(tmp_accessible, HAIL_CODE_DIALOG_BACKSPACE_BUTTON);
  g_object_unref(tmp_accessible);
  g_object_unref(tmp_container);

  g_object_unref(data_area_filler);

}

