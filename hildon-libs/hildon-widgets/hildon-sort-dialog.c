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

/** 
 * SECTION:hildon-sort-dialog
 * @short_description: A widget for defining the sorting order of items
 * 
 * HildonSortDialog is used to define an order (ascending/descending)
 * and a field by which items are sorted in a list. The combo boxes
 * display the current value when the dialog is opened.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <libintl.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkbox.h>
#include <hildon-widgets/hildon-caption.h>
#include "hildon-sort-dialog.h"


#define _(String) dgettext(PACKAGE, String)

static GtkDialogClass *parent_class;

#define HILDON_SORT_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE \
                ((obj), HILDON_TYPE_SORT_DIALOG, HildonSortDialogPrivate));

typedef struct _HildonSortDialogPrivate HildonSortDialogPrivate;

static void hildon_sort_dialog_class_init(HildonSortDialogClass * class);
static void hildon_sort_dialog_init(HildonSortDialog * widget);
static void hildon_sort_dialog_set_property(GObject * object,
                                     guint prop_id,
                                     const GValue * value,
                                     GParamSpec * pspec);
static void hildon_sort_dialog_get_property(GObject * object,
                                     guint prop_id,
                                     GValue * value, GParamSpec * pspec);

enum {
    PROP_0,
    PROP_SORT_KEY,
    PROP_SORT_ORDER
};

/* private data */
struct _HildonSortDialogPrivate {
    /* Sort category widgets */
    GtkWidget *combo_key;
    GtkWidget *caption_key;

    /* Sort order widgets */
    GtkWidget *combo_order;
    GtkWidget *caption_order;

    /* Index value counter */
    gint index_counter;
};

/* Private functions */

/*
 * Initialises the sort dialog class.
 */
static void hildon_sort_dialog_class_init(HildonSortDialogClass * class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(class);
    parent_class = g_type_class_peek_parent(class);
    g_type_class_add_private(class, sizeof(HildonSortDialogPrivate));
    
    gobject_class->set_property = hildon_sort_dialog_set_property;
    gobject_class->get_property = hildon_sort_dialog_get_property;
    
    g_object_class_install_property(gobject_class, PROP_SORT_KEY,
        g_param_spec_int("sort-key",
                         "Sort Key",
                         "The currently active sort key",
			 G_MININT,
			 G_MAXINT,
                         0, G_PARAM_READWRITE));
    
    g_object_class_install_property(gobject_class, PROP_SORT_ORDER,
        g_param_spec_enum("sort-order",
                         "Sort Order",
                         "The current sorting order",
			 GTK_TYPE_SORT_TYPE,
                         GTK_SORT_ASCENDING,
			 G_PARAM_READWRITE));
}

static void hildon_sort_dialog_init(HildonSortDialog * dialog)
{
    HildonSortDialogPrivate *priv;
    GtkSizeGroup *group;

    g_assert(HILDON_IS_SORT_DIALOG(dialog));

    priv = HILDON_SORT_DIALOG_GET_PRIVATE(dialog);

    priv->index_counter = 0;

    group = GTK_SIZE_GROUP(gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL));

    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_title(GTK_WINDOW(dialog), _("ckdg_ti_sort"));

    /* Tab one */
    priv->combo_key = gtk_combo_box_new_text();
    priv->caption_key = hildon_caption_new(group, _("ckdg_fi_sort_field"), priv->combo_key,
                                        NULL, HILDON_CAPTION_OPTIONAL);
    hildon_caption_set_separator(HILDON_CAPTION(priv->caption_key), "");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       priv->caption_key, FALSE, FALSE, 0);

    /* Tab two */
    priv->combo_order = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(priv->combo_order),
                              _("ckdg_va_sort_ascending"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(priv->combo_order),
                              _("ckdg_va_sort_descending"));

    priv->caption_order = hildon_caption_new(group, _("ckdg_fi_sort_order"),
                                        priv->combo_order,
                                        NULL, HILDON_CAPTION_OPTIONAL);
    hildon_caption_set_separator(HILDON_CAPTION(priv->caption_order), "");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       priv->caption_order, FALSE, FALSE, 0);

    gtk_combo_box_set_active(GTK_COMBO_BOX(priv->combo_key), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(priv->combo_order), 0);

    /* Create the OK/CANCEL buttons */
    (void) gtk_dialog_add_button(GTK_DIALOG(dialog),
                                           _("ckdg_bd_sort_dialog_ok"),
                                           GTK_RESPONSE_OK);
    (void) gtk_dialog_add_button(GTK_DIALOG(dialog),
                                               _("ckdg_bd_sort_dialog_cancel"),
                                               GTK_RESPONSE_CANCEL);
    /* FIXME: Hardcoded sizes are bad */
    gtk_window_resize(GTK_WINDOW(dialog), 370, 100);
    gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);

    g_object_unref(group); /* Captions now own their references to sizegroup */
}

/* Public functions */

/**
 * hildon_sort_dialog_get_type:
 *
 * Returns GType for HildonSortDialog as produced by 
 * g_type_register_static().
 *
 * Returns: HildonSortDialog type
 */
GType hildon_sort_dialog_get_type()
{
    static GType dialog_type = 0;

    if (!dialog_type) {
        static const GTypeInfo dialog_info = {
            sizeof(HildonSortDialogClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) hildon_sort_dialog_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof(HildonSortDialog),
            0,  /* n_preallocs */
            (GInstanceInitFunc) hildon_sort_dialog_init
        };

        dialog_type = g_type_register_static(GTK_TYPE_DIALOG,
                                             "HildonSortDialog",
                                             &dialog_info, 0);
    }
    return dialog_type;
}

/**
 * hildon_sort_dialog_new:
 * @parent: widget to be transient for, or NULL if none
 *
 * HildonSortDialog contains two HildonCaptions with combo boxes. 
 *
 * Returns: pointer to a new @HildonSortDialog widget
 */
GtkWidget *hildon_sort_dialog_new(GtkWindow * parent)
{
    GtkWidget *sort_dialog = g_object_new(HILDON_TYPE_SORT_DIALOG, NULL);

    if (parent)
        gtk_window_set_transient_for(GTK_WINDOW(sort_dialog), parent);

    return sort_dialog;
}

/**
 * hildon_sort_dialog_get_sort_key:
 * @dialog: the #HildonSortDialog widget
 *
 * Gets index to currently active sort key.
 * 
 * Returns: an integer which is the index value of the "Sort by" 
 * field 
 */
gint hildon_sort_dialog_get_sort_key(HildonSortDialog * dialog)
{
    GtkWidget *combo_key;
    HildonSortDialogPrivate *priv;

    g_return_val_if_fail(HILDON_IS_SORT_DIALOG(dialog), -1);

    priv = HILDON_SORT_DIALOG_GET_PRIVATE(dialog);
    
    combo_key = gtk_bin_get_child(GTK_BIN(priv->caption_key));
        
    return gtk_combo_box_get_active(GTK_COMBO_BOX(combo_key));
}

/**
 * hildon_sort_dialog_get_sort_order:
 * @dialog: the #HildonSortDialog widget
 *
 * Gets current sorting order from "Sort order" field.
 *
 * Returns: current sorting order as #GtkSortType
 */
GtkSortType hildon_sort_dialog_get_sort_order(HildonSortDialog * dialog)
{
    GtkWidget *combo_order;
    HildonSortDialogPrivate *priv;

    g_return_val_if_fail(HILDON_IS_SORT_DIALOG(dialog), 0);

    priv = HILDON_SORT_DIALOG_GET_PRIVATE(dialog);
    combo_order = gtk_bin_get_child(GTK_BIN(priv->caption_order));
        
    return gtk_combo_box_get_active(GTK_COMBO_BOX(combo_order));
}

/**
 * hildon_sort_dialog_set_sort_key:
 * @dialog: the #HildonSortDialog widget
 * @key: combo box's index value
 *
 * Sets the index value of the #HildonSortDialog widget.
 */
void hildon_sort_dialog_set_sort_key(HildonSortDialog * dialog, gint key)
{
    GtkWidget *combo_key;
    HildonSortDialogPrivate *priv;

    g_return_if_fail(HILDON_IS_SORT_DIALOG(dialog));

    priv = HILDON_SORT_DIALOG_GET_PRIVATE(dialog);
    combo_key = gtk_bin_get_child(GTK_BIN(priv->caption_key));
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_key), key);

    g_object_notify (G_OBJECT (dialog), "sort-key");
}

/**
 * hildon_sort_dialog_set_sort_order:
 * @dialog: the #HildonSortDialog widget
 * @order: combo box's index value
 *
 * Sets the index value of the #HildonSortDialog widget.
 */
void
hildon_sort_dialog_set_sort_order(HildonSortDialog * dialog,
                                  GtkSortType order)
{
    GtkWidget *combo_order;
    HildonSortDialogPrivate *priv;

    g_return_if_fail(HILDON_IS_SORT_DIALOG(dialog));

    priv = HILDON_SORT_DIALOG_GET_PRIVATE(dialog);
    combo_order = gtk_bin_get_child(GTK_BIN(priv->caption_order));
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_order), order);

    g_object_notify (G_OBJECT (dialog), "sort-order");
}

/**
 * hildon_sort_dialog_add_sort_key:
 * @dialog: the #HildonSortDialog widget
 * @sort_key: combo box's index value
 *
 * Adds a new sort key and returns the respective index in
 * sort key combobox.
 *
 * Returns: an integer which is the index of the added combo box's
 * item
 */
gint
hildon_sort_dialog_add_sort_key(HildonSortDialog * dialog,
                                const gchar * sort_key)
{
    HildonSortDialogPrivate *priv;

    g_return_val_if_fail(HILDON_IS_SORT_DIALOG(dialog), -1);

    priv = HILDON_SORT_DIALOG_GET_PRIVATE(dialog);
    gtk_combo_box_append_text(GTK_COMBO_BOX(priv->combo_key), sort_key);

    return priv->index_counter++;
}

static void
hildon_sort_dialog_set_property(GObject * object,
                         guint prop_id,
                         const GValue * value, GParamSpec * pspec)
{
    HildonSortDialog *dialog;

    dialog = HILDON_SORT_DIALOG(object);

    switch (prop_id) {
    case PROP_SORT_KEY:
        hildon_sort_dialog_set_sort_key(dialog, g_value_get_int(value));
        break;
    case PROP_SORT_ORDER:
        hildon_sort_dialog_set_sort_order(dialog, g_value_get_enum(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
hildon_sort_dialog_get_property(GObject * object,
                         guint prop_id, GValue * value, GParamSpec * pspec)
{
    HildonSortDialog *dialog;

    dialog = HILDON_SORT_DIALOG(object);

    switch (prop_id) {
    case PROP_SORT_KEY:
        g_value_set_int(value, hildon_sort_dialog_get_sort_key(dialog));
        break;
    case PROP_SORT_ORDER:
        g_value_set_enum(value, hildon_sort_dialog_get_sort_order(dialog));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

