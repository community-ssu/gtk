/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
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

#include <config.h>
#include <string.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtkaccellabel.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkprivate.h>
#include "hildon-thumb-menu-item.h"

#define HILDON_THUMB_MENU_ITEM_GET_PRIVATE(obj) \
           (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
           HILDON_TYPE_THUMB_MENU_ITEM, HildonThumbMenuItemPrivate));

#define HILDON_MENU_WINDOW_NAME_NORMAL "hildon-menu-window-normal"
#define HILDON_MENU_WINDOW_NAME_THUMB "hildon-menu-window-thumb"

typedef struct _HildonThumbMenuItemPrivate HildonThumbMenuItemPrivate;

struct _HildonThumbMenuItemPrivate 
{
    GtkWidget *menu_image;
    GtkWidget *thumb_image;
    gchar *label;
    gchar *thumb_label;
    gchar *comment;
    gboolean thumb_mode;
    gboolean mnemonic;
};


static void
hildon_thumb_menu_item_class_init (HildonThumbMenuItemClass *klass);

static void
hildon_thumb_menu_item_init (HildonThumbMenuItem *thumb_menu_item);

static void
hildon_thumb_menu_item_forall (GtkContainer *container,
                               gboolean include_internals,
                               GtkCallback callback,
                               gpointer callback_data);

static void
hildon_thumb_menu_item_finalize (GObject *object);
    
static void
hildon_thumb_menu_item_set_property (GObject *object, guint prop_id,
                                     const GValue *value, GParamSpec *pspec);

static void
hildon_thumb_menu_item_get_property (GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec);

static void
hildon_thumb_menu_item_set_image (HildonThumbMenuItem *thumb_menu_item,
                                  GtkWidget *child);

static void
hildon_thumb_menu_item_set_thumb_image (HildonThumbMenuItem *thumb_menu_item,
                                        GtkWidget *child);

static void
hildon_thumb_menu_item_set_text (HildonThumbMenuItem *thumb_menu_item,
                                 const gchar *text);

static void
hildon_thumb_menu_item_set_comment (HildonThumbMenuItem *thumb_menu_item,
                                    const gchar *text);

static void
hildon_thumb_menu_item_set_mode (HildonThumbMenuItem *thumb_menu_item,
                                 gboolean thumb_mode);

static void
hildon_thumb_menu_item_set_text (HildonThumbMenuItem *thumb_menu_item,
                                 const gchar *text);

static void
hildon_thumb_menu_item_set_thumb_text (HildonThumbMenuItem *thumb_menu_item,
                                 const gchar *text);

static void
hildon_thumb_menu_item_set_comment (HildonThumbMenuItem *thumb_menu_item,
                                    const gchar *text);

static void
hildon_thumb_menu_item_set_label (HildonThumbMenuItem *thumb_menu_item,
                                  const gchar *text);

static void
hildon_thumb_menu_item_set_thumb_label (HildonThumbMenuItem *thumb_menu_item,
                                        HildonThumbMenuItemPrivate *priv);

enum ThumbMenuItemProperties 
{
    PROP_IMAGE = 1,
    PROP_THUMB_IMAGE,
    PROP_THUMB_MODE,
    PROP_LABEL,
    PROP_THUMB_LABEL,
    PROP_COMMENT
};

static GtkImageMenuItemClass *parent_class = NULL;

GType
hildon_thumb_menu_item_get_type (void)
{
    static GType thumb_menu_item_type = 0;

    if (!thumb_menu_item_type)
    {
        static const GTypeInfo thumb_menu_item_info =
        {
            sizeof (HildonThumbMenuItemClass),
            NULL,		/* base_init */
            NULL,		/* base_finalize */
            (GClassInitFunc) hildon_thumb_menu_item_class_init,
            NULL,		/* class_finalize */
            NULL,		/* class_data */
            sizeof (HildonThumbMenuItem),
            0,	                /* n_preallocs */
            (GInstanceInitFunc) hildon_thumb_menu_item_init,
        };

        thumb_menu_item_type =
            g_type_register_static (GTK_TYPE_IMAGE_MENU_ITEM,
                    "HildonThumbMenuItem",
                    &thumb_menu_item_info, 0);
    }

    return thumb_menu_item_type;
}

static void
hildon_thumb_menu_item_class_init (HildonThumbMenuItemClass *klass)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;
    GtkMenuItemClass *menu_item_class;
    GtkContainerClass *container_class;

    gobject_class = (GObjectClass*) klass;
    widget_class = (GtkWidgetClass*) klass;
    menu_item_class = (GtkMenuItemClass*) klass;
    container_class = (GtkContainerClass*) klass;

    parent_class = g_type_class_peek_parent (klass);
    
    /* Add the private struct */
    g_type_class_add_private(gobject_class, sizeof(HildonThumbMenuItemPrivate));

    container_class->forall = hildon_thumb_menu_item_forall;
    
    gobject_class->finalize     = hildon_thumb_menu_item_finalize;
    gobject_class->set_property = hildon_thumb_menu_item_set_property;
    gobject_class->get_property = hildon_thumb_menu_item_get_property;

    /**
     * HildonThumbMenuItem:image:
     *
     * The image to be shown when appearing as a regular menu item.
     */
    g_object_class_install_property (gobject_class, PROP_IMAGE,
                g_param_spec_object ("image", "Image widget",
                "Child widget to appear next to the menu text",
                GTK_TYPE_WIDGET, GTK_PARAM_READABLE | GTK_PARAM_WRITABLE));

    /**
     * HildonThumbMenuItem:thumb:
     *
     * The image to be shown when in thumb mode.
     */
    g_object_class_install_property (gobject_class, PROP_THUMB_IMAGE,
                g_param_spec_object ("thumb", "Thumb image widget",
                "Child widget to appear next to the menu text in thumb mode",
                GTK_TYPE_WIDGET, GTK_PARAM_READABLE | GTK_PARAM_WRITABLE));
    /** 
     * HildonThumbMenuItem:thumb_mode:
     *
     * Boolean whether the item is in thumb mode or not.
     */
    g_object_class_install_property (gobject_class, PROP_THUMB_MODE,
                g_param_spec_boolean("thumb_mode", "Thumb mode",
                "Controls whether the Thumb menu is in thumb mode",
                TRUE, G_PARAM_READABLE | G_PARAM_WRITABLE));

    /** 
     * HildonThumbMenuItem:label:
     *
     * The text shown in the label in all modes.
     */
    g_object_class_install_property (gobject_class, PROP_LABEL,
                g_param_spec_string("label", "Label Text",
                "The text on the menuitem that is visible in both modes",
                "", G_PARAM_READABLE | G_PARAM_WRITABLE));

    /** 
     * HildonThumbMenuItem:thumb-label:
     *
     * The text shown in the label in thumb mode.
     */
    g_object_class_install_property (gobject_class, PROP_LABEL,
                g_param_spec_string("thumb-label", "Thumb Label Text",
                "The text on the menuitem that is visible in thumb mode."
                "Overrides the \"label\" property if non-NULL",
                NULL, G_PARAM_READABLE | G_PARAM_WRITABLE));
    
    /** 
     * HildonThumbMenuItem:comment:
     *
     * The text shown in the label in thumb mode as a second line.
     */
    g_object_class_install_property (gobject_class, PROP_COMMENT,
                g_param_spec_string("comment", "Comment Text",
                "The comment text that is only visible in the thumb mode",
                "", G_PARAM_READABLE | G_PARAM_WRITABLE));
}

static void
hildon_thumb_menu_item_init (HildonThumbMenuItem *thumb_menu_item)
{
    HildonThumbMenuItemPrivate *priv;

    priv = HILDON_THUMB_MENU_ITEM_GET_PRIVATE (thumb_menu_item);

    /* Set the initial values for the private struct */
    priv->menu_image = NULL;
    priv->thumb_image = NULL;
    priv->label = NULL;
    priv->thumb_label = NULL;
    priv->comment = NULL;
    priv->thumb_mode = FALSE;
    priv->mnemonic = FALSE;
    GTK_IMAGE_MENU_ITEM(thumb_menu_item)->image = NULL;
}

static void
hildon_thumb_menu_item_set_property (GObject         *object,
        guint            prop_id,
        const GValue    *value,
        GParamSpec      *pspec)
{
    HildonThumbMenuItem *thumb_menu_item = HILDON_THUMB_MENU_ITEM (object);

    switch (prop_id)
    {
        case PROP_IMAGE:
            {
                GtkWidget *image;

                image = (GtkWidget*) g_value_get_object (value);

                hildon_thumb_menu_item_set_image (thumb_menu_item, image);
            }
            break;
            
        case PROP_THUMB_IMAGE:
            {
                GtkWidget *image;

                image = (GtkWidget*) g_value_get_object (value);

                hildon_thumb_menu_item_set_thumb_image (thumb_menu_item, image);
            }
            break;
            
        case PROP_THUMB_MODE:
            hildon_thumb_menu_item_set_mode (thumb_menu_item,
                                             g_value_get_boolean (value));
            break;
            
        case PROP_LABEL:
            hildon_thumb_menu_item_set_text (thumb_menu_item,
                                             g_value_get_string(value));
            break;
        case PROP_THUMB_LABEL:
            hildon_thumb_menu_item_set_thumb_text (thumb_menu_item,
                                             g_value_get_string(value));
            break;
            
        case PROP_COMMENT:
            hildon_thumb_menu_item_set_comment (thumb_menu_item,
                                                g_value_get_string(value));
            break;
            
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
hildon_thumb_menu_item_get_property (GObject         *object,
                                     guint            prop_id,
                                     GValue          *value,
                                     GParamSpec      *pspec)
{
    HildonThumbMenuItem *thumb_menu_item;
    HildonThumbMenuItemPrivate *priv;

    g_assert (HILDON_IS_THUMB_MENU_ITEM (object));
    
    thumb_menu_item = HILDON_THUMB_MENU_ITEM (object);
    priv = HILDON_THUMB_MENU_ITEM_GET_PRIVATE (thumb_menu_item);

    switch (prop_id)
    {
        case PROP_IMAGE:
            g_value_set_object (value, (GObject*) priv->menu_image);
            break;
            
        case PROP_THUMB_IMAGE:
            g_value_set_object (value, (GObject*) priv->thumb_image);
            break;
            
        case PROP_THUMB_MODE:
            g_value_set_boolean (value, priv->thumb_mode);
            break;
            
        case PROP_LABEL:
            g_value_set_string (value, priv->label);
            break;
            
        case PROP_THUMB_LABEL:
            g_value_set_string (value, priv->thumb_label);
            break;

        case PROP_COMMENT:
            g_value_set_string (value, priv->comment);
            break;
            
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
hildon_thumb_menu_item_forall (GtkContainer   *container,
                               gboolean include_internals,
                               GtkCallback callback,
                               gpointer callback_data)
{
    HildonThumbMenuItem *thumb_menu_item = HILDON_THUMB_MENU_ITEM (container);

    (* GTK_CONTAINER_CLASS (parent_class)->forall) (container,
                                                    include_internals,
                                                    callback,
                                                    callback_data);

    if (GTK_IMAGE_MENU_ITEM(thumb_menu_item)->image)
    {
        (* callback) (GTK_IMAGE_MENU_ITEM(thumb_menu_item)->image,
                      callback_data);
    }
}

static void
hildon_thumb_menu_item_finalize (GObject *object)
{
    HildonThumbMenuItem *thumb_menu_item;
    HildonThumbMenuItemPrivate *priv;

    thumb_menu_item = HILDON_THUMB_MENU_ITEM (object);
    priv = HILDON_THUMB_MENU_ITEM_GET_PRIVATE (thumb_menu_item);

    /* Remove the additional references from the internal images */
    if (priv->menu_image)
    {
        g_object_unref (G_OBJECT (priv->menu_image));
    }

    if (priv->thumb_image)
    {
        g_object_unref (G_OBJECT (priv->thumb_image));
    }
    
    /* Free the internal strings */
    g_free (priv->label);
    g_free (priv->thumb_label);
    g_free (priv->comment);

    if (G_OBJECT_CLASS (parent_class)->finalize)
        G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* Sets the image for normal mode operation.
 * Removes the old image and if the menu is in normal mode, the image is set
 * visible in the menu item.
 * This is also the image the menu item should default to in both modes, if
 * thumb mode image is not present.
 */
static void
hildon_thumb_menu_item_set_image (HildonThumbMenuItem *thumb_menu_item,
                                  GtkWidget *image)
{
    HildonThumbMenuItemPrivate *priv;

    priv = HILDON_THUMB_MENU_ITEM_GET_PRIVATE (thumb_menu_item);

    /* Not setting the same image again */
    if (image == priv->menu_image)
        return;

    /* Remove the reference to the old image */
    if (priv->menu_image)
        gtk_widget_unref (priv->menu_image);

    /* Set the new image */
    priv->menu_image = image;
    
    /* If there is an image, add a reference to it */
    if (image != NULL)
    {
        gtk_widget_ref (priv->menu_image);
        gtk_object_sink (GTK_OBJECT (priv->menu_image));
    }

    /* Should we show the new image right away? */
    if (!priv->thumb_mode)
    {
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (thumb_menu_item),
                                       image);
    }
}

/* Sets the image for thumb mode operation.  
 * Removes the old image and if the menu is in thumb mode, the image is set
 * visible in the menu item.
 * If the image is the same as in normal mode, image is set to NULL and normal
 * mode image is used as fallback.
 */
static void
hildon_thumb_menu_item_set_thumb_image (HildonThumbMenuItem *thumb_menu_item,
                                        GtkWidget *image)
{
    HildonThumbMenuItemPrivate *priv;

    priv = HILDON_THUMB_MENU_ITEM_GET_PRIVATE (thumb_menu_item);

    /* Not setting the same image again */
    if (image == priv->thumb_image)
        return;

    /* Remove the reference to the old image */
    if (priv->thumb_image)
        g_object_unref (priv->thumb_image);
    
    /* If someone is trying to set the same image that is used as normal mode
     * image, we'll just refuse to set it.
     */
    if (image == priv->menu_image)
    {
        priv->thumb_image = NULL;
        /* If in thumb mode, set the normal mode image */
        if (priv->thumb_mode)
        {
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(thumb_menu_item),
                                           image);
        }
    }
    else
    {
        priv->thumb_image = image;
    }

    /* If there is an image, add a reference to it */
    if (image != NULL)
    {
        gtk_widget_ref (priv->thumb_image);
        gtk_object_sink (GTK_OBJECT (priv->thumb_image));
    }
    
    /* Should we show the new image right away? */    
    if (priv->thumb_mode)
    {
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (thumb_menu_item),
                                       image);
    }
}

/* Sets the operation mode for the given thumb menu item */
static void
hildon_thumb_menu_item_set_mode (HildonThumbMenuItem *thumb_menu_item,
                                 gboolean thumb_mode)
{
    HildonThumbMenuItemPrivate *priv;
    GtkWidget *toplevel;

    priv = HILDON_THUMB_MENU_ITEM_GET_PRIVATE (thumb_menu_item);
    
    /* Setting the same mode again is a NOOP */
    if (priv->thumb_mode == thumb_mode)
        return;
    
    priv->thumb_mode = !priv->thumb_mode;

    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (thumb_menu_item));
    
    if (priv->thumb_mode)
    {
        hildon_thumb_menu_item_set_thumb_label (thumb_menu_item, priv);
        
        /* If we have a thumb image we use it. In other case we default to
         * using the normal mode image and that requires no operation
         */
        if (priv->thumb_image)
        {
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(thumb_menu_item),
                                           priv->thumb_image);
        }

        if (GTK_IS_WIDGET (toplevel))
          gtk_widget_set_name (toplevel, HILDON_MENU_WINDOW_NAME_THUMB);
    }
    else
    {
        /* Try to reset the attributes to avoid two coloured text in
         * normal mode
         */
        PangoAttrList *attrs;
        GtkLabel *label = GTK_LABEL(GTK_BIN(thumb_menu_item)->child);
        attrs = pango_attr_list_new();
        g_object_set(label, "attributes", attrs, NULL);
        
        hildon_thumb_menu_item_set_label (thumb_menu_item, priv->label);
        
        /* If we have a image or if the thumb_image exists set the image.
         * This is done to avoid overriding the image set with the
         * GtkImageMenuItem API when no other images have been set.
         */
        if (priv->menu_image || priv->thumb_image)
        {
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (
                                           thumb_menu_item),
                                           priv->menu_image);
        }
        
        /* Clean up */
        if (attrs)
            pango_attr_list_unref(attrs);
        
        if (GTK_IS_WIDGET (toplevel))
          gtk_widget_set_name (toplevel, HILDON_MENU_WINDOW_NAME_NORMAL);
    }
}

/* Sets the internal string for the menu label and finally the actual menu text
 * If the text is NULL, we still use empty string internally.
 */
static void
hildon_thumb_menu_item_set_text (HildonThumbMenuItem *thumb_menu_item,
                                 const gchar *text)
{
    HildonThumbMenuItemPrivate *priv;

    priv = HILDON_THUMB_MENU_ITEM_GET_PRIVATE (thumb_menu_item);

    /* If we already have a label text, free it */
    if (priv->label)
    {
        g_free (priv->label);
        priv->label = NULL;
    }

    /* If the new text is valid, use it */
    if (text)
    {
        priv->label = g_strdup (text);
    }
    else /* else use an empty string */
    {
        priv->label = g_strdup ("");
    }

    /* If we're in normal mode, just set the text */
    if (!priv->thumb_mode)
    {
        hildon_thumb_menu_item_set_label (thumb_menu_item, priv->label);
    }
    else /* else set the thumb menu label */
    {
        hildon_thumb_menu_item_set_thumb_label (thumb_menu_item, priv);
    }
}

/* Sets the internal string for the menu label in thumb mode
 * If the text is NULL, the text is unset and will default to the normal label.
 */
static void
hildon_thumb_menu_item_set_thumb_text (HildonThumbMenuItem *thumb_menu_item,
                                 const gchar *text)
{
    HildonThumbMenuItemPrivate *priv;

    priv = HILDON_THUMB_MENU_ITEM_GET_PRIVATE (thumb_menu_item);

    /* If we already have a label text, free it */
    if (priv->thumb_label)
    {
        g_free (priv->thumb_label);
        priv->thumb_label = NULL;
    }

    /* If the new text is valid, use it */
    if (text)
    {
        priv->thumb_label = g_strdup (text);
    }
    else /* else set as NULL so that we will default to the normal label */
    {
        priv->thumb_label = NULL;
    }

    /* If we're in normal mode, we don't react. In thumb mode, set the label */
    if (priv->thumb_mode)
    {
        hildon_thumb_menu_item_set_thumb_label (thumb_menu_item, priv);
    }
}

/* Sets the internal string for the menu comment. The actual text is set only
 * when the menu item is in thumb mode.
 * If the text is NULL, we still use empty string internally.
 */
static void
hildon_thumb_menu_item_set_comment (HildonThumbMenuItem *thumb_menu_item,
                                    const gchar *text)
{
    HildonThumbMenuItemPrivate *priv;

    priv = HILDON_THUMB_MENU_ITEM_GET_PRIVATE (thumb_menu_item);

    /* If we already have a label text, free it */
    if (priv->comment)
    {
        g_free (priv->comment);
        priv->comment = NULL;
    }

    /* If the new text is valid, use it */
    if (text)
    {
        priv->comment = g_strdup (text);
    }
    else /* else use an empty string */
    {
        priv->comment = g_strdup ("");
    }

    /* In thumb mode, replace the text with a new one */
    if (priv->thumb_mode)
    {
        hildon_thumb_menu_item_set_thumb_label (thumb_menu_item, priv);
    }
}

/* Sets the actual menu label */
static void
hildon_thumb_menu_item_set_label (HildonThumbMenuItem *thumb_menu_item,
                                  const gchar *text)
{
    HildonThumbMenuItemPrivate *priv;
    GtkLabel *label = GTK_LABEL(GTK_BIN(thumb_menu_item)->child); 

    priv = HILDON_THUMB_MENU_ITEM_GET_PRIVATE (thumb_menu_item);

    if (priv->mnemonic)
    {
        gtk_label_set_text_with_mnemonic (label, text);
    }
    else
    {
        gtk_label_set_text (label , text);
    }

    g_object_notify (G_OBJECT(label), "label");
}

/* Sets the thumb mode label.
 * Concatenates the label and comment texts with a newline character to create
 * a two lined text. Fetches the colours from widget style and sets proper 
 * pango attributes to the label.
 */
static void
hildon_thumb_menu_item_set_thumb_label (HildonThumbMenuItem *thumb_menu_item,
                                        HildonThumbMenuItemPrivate *priv)
{
    gchar *tmp_label;
    int line1_length, line2_length;
    PangoAttrList *attrs = NULL;
    PangoAttribute *attr;
    GdkColor color1, color2;
    gboolean color1_found, color2_found;
    GtkLabel *label = GTK_LABEL(GTK_BIN(thumb_menu_item)->child);

    g_assert (HILDON_IS_THUMB_MENU_ITEM (thumb_menu_item));
    
    /* Get the text lengths */
    if (priv->thumb_label)
    {
        line1_length = strlen (priv->thumb_label);
        tmp_label = g_strconcat (priv->thumb_label, "\n", priv->comment, NULL);
    }
    else
    {
        line1_length = strlen (priv->label);
        tmp_label = g_strconcat (priv->label, "\n", priv->comment, NULL);
    }

    line2_length = strlen (priv->comment);

    /* Create and set the actual label */
    hildon_thumb_menu_item_set_label (thumb_menu_item, tmp_label);
    
    /* Make sure the widget has a style attached to it */
    gtk_widget_ensure_style (GTK_WIDGET (thumb_menu_item));

    /* Fetch the colours to be used */
    color1_found = gtk_style_lookup_logical_color(GTK_WIDGET(
                thumb_menu_item)->style, "DefaultTextColor", &color1);
    color2_found = gtk_style_lookup_logical_color(GTK_WIDGET(
                thumb_menu_item)->style, "SecondaryTextColor", &color2);

    attrs = pango_attr_list_new();

    /* If colours are found, apply them to proper ranges */
    if (color1_found)
    {
        attr = pango_attr_foreground_new(color1.red, color1.green, color1.blue);
        attr->start_index = 0;
        attr->end_index = line1_length;
        pango_attr_list_insert(attrs, attr);
    }
    if (color2_found)
    {
        attr = pango_attr_foreground_new(color2.red, color2.green,color2.blue);
        attr->start_index = line1_length + 1;
        attr->end_index = line1_length + line2_length + 1;
        pango_attr_list_insert(attrs, attr);
    }

    /* Set the attributes */
    g_object_set(label, "attributes", attrs, NULL);

    gtk_label_set_ellipsize (label,PANGO_ELLIPSIZE_NONE);

    /* Clean up */
    if (attrs)
        pango_attr_list_unref(attrs);

    g_free (tmp_label);
}

/**
 * hildon_thumb_menu_item_new:
 * @returns: a new #HildonThumbMenuItem.
 *
 * Creates a new #HildonThumbMenuItem with an empty label.
 **/
GtkWidget*
hildon_thumb_menu_item_new (void)
{
    return g_object_new (HILDON_TYPE_THUMB_MENU_ITEM, NULL);
}

/**
 * hildon_thumb_menu_item_new_with_labels:
 * @label: the text of the menu item.
 * @comment: the comment line in the thumb mode.
 * @returns: a new #HildonThumbMenuItem.
 *
 * Creates a new #HildonThumbMenuItem containing a label. 
 **/
GtkWidget*
hildon_thumb_menu_item_new_with_labels (const gchar *label,
                                        const gchar *thumb_label,
                                        const gchar *comment)
{
    HildonThumbMenuItem *thumb_menu_item;
    GtkWidget *accel_label;
    HildonThumbMenuItemPrivate *priv;

    thumb_menu_item = g_object_new (HILDON_TYPE_THUMB_MENU_ITEM, NULL);

    priv = HILDON_THUMB_MENU_ITEM_GET_PRIVATE (thumb_menu_item);    
    
    /* Set the initial label with an empty string to avoid problems
     * with NULL values
     */
    accel_label = gtk_accel_label_new ("");
    gtk_misc_set_alignment (GTK_MISC (accel_label), 0.0, 0.5);

    gtk_container_add (GTK_CONTAINER (thumb_menu_item), accel_label);
    gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (accel_label),
            GTK_WIDGET (thumb_menu_item));
    
    /* Set the proper strings */
    hildon_thumb_menu_item_set_text (thumb_menu_item, label);
    hildon_thumb_menu_item_set_thumb_text (thumb_menu_item, thumb_label);
    hildon_thumb_menu_item_set_comment (thumb_menu_item, comment);
    
    gtk_widget_show (accel_label);

    return GTK_WIDGET(thumb_menu_item);
}


/**
 * hildon_thumb_menu_item_new_with_mnemonic:
 * @label: the text of the menu item, with an underscore in front of the
 *         mnemonic character
 * @comment: the comment line in the thumb mode.
 * @returns: a new #HildonThumbMenuItem
 *
 * Creates a new #HildonThumbMenuItem containing a label. The label
 * will be created using gtk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the menu item.
 **/
GtkWidget*
hildon_thumb_menu_item_new_with_mnemonic (const gchar *label,
                                          const gchar *thumb_label,
                                          const gchar *comment)
{
    HildonThumbMenuItem *thumb_menu_item;
    GtkWidget *accel_label;
    HildonThumbMenuItemPrivate *priv;
        
    thumb_menu_item = g_object_new (HILDON_TYPE_THUMB_MENU_ITEM, NULL);

    priv = HILDON_THUMB_MENU_ITEM_GET_PRIVATE (thumb_menu_item);

    priv->mnemonic = TRUE;

    /* Set the initial label with an empty string to avoid problems
     * with NULL values
     */
    accel_label = g_object_new (GTK_TYPE_ACCEL_LABEL, NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (accel_label), label);
    gtk_label_set_ellipsize (GTK_LABEL (accel_label),PANGO_ELLIPSIZE_NONE);
    gtk_misc_set_alignment (GTK_MISC (accel_label), 0.0, 0.5);

    gtk_container_add (GTK_CONTAINER (thumb_menu_item), accel_label);
    gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (accel_label),
            GTK_WIDGET (thumb_menu_item));

    /* Set the proper strings */
    hildon_thumb_menu_item_set_text (thumb_menu_item, label);
    hildon_thumb_menu_item_set_comment (thumb_menu_item, comment);
        
    gtk_widget_show (accel_label);

    return GTK_WIDGET(thumb_menu_item);
}

/** 
 * hildon_thumb_menu_item_set_images:
 * @thumb_menu_item: a #HildonThumbMenuItem.
 * @image: a widget to set as the image for the menu item.
 * @thumb_image: a widget to set as the image for the menu item in thumb mode.
 * 
 * Sets the image of @thumb_menu_item to the given widget. 
 * 
 * If the thumb mode image is not provided, the thumb mode will default to 
 * normal mode image. If user want's to use the same image for both modes, only
 * the normal mode image should be provided.
 * 
 * Note that it depends on the show-menu-images setting whether
 * the image will be displayed or not.
 **/ 
void
hildon_thumb_menu_item_set_images (HildonThumbMenuItem *thumb_menu_item,
                                   GtkWidget *image, GtkWidget *thumb_image)
{
    g_return_if_fail (GTK_IS_IMAGE_MENU_ITEM (thumb_menu_item));

    hildon_thumb_menu_item_set_image (thumb_menu_item, image);
    if (image != thumb_image)
    {
        hildon_thumb_menu_item_set_thumb_image (thumb_menu_item, thumb_image);
    }
}

/**
 * hildon_menu_set_thumb_mode:
 * @menu: a GtkMenu which mode to switch
 * @thumb_mode: a boolean indicating whether the menu should be switched to
 * normal mode (FALSE) or thumb mode (TRUE)
 *
 * Goes through all items in the menu and also recursively all submenus and
 * switches all the found HildonThumbMenuItems to the requested mode.
 */
void
hildon_menu_set_thumb_mode (GtkMenu *menu, gboolean thumb_mode)
{
    GList *item_list = 0;
    GtkWidget *submenu = NULL;    
    
    g_assert (GTK_IS_MENU(menu));

    /* not very polite, but seems to be the only way */
    item_list = GTK_MENU_SHELL (menu)->children;

    for (; item_list != NULL; item_list = item_list->next)
    {
        /* If a HildonThumbMenuItem is found, set the mode as requested */
        if (HILDON_IS_THUMB_MENU_ITEM (item_list->data))
        {
            hildon_thumb_menu_item_set_mode (
                    HILDON_THUMB_MENU_ITEM (item_list->data), thumb_mode);
            submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (
                                                    item_list->data));
        }
        else if (GTK_IS_MENU_ITEM (item_list->data))
        {
            submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (
                                                    item_list->data));
        }
        else
        {
            submenu = NULL;
        }

        /* If a submenu is found, call hildon_menu_set_thumb_mode */
        if (submenu)
        {
            hildon_menu_set_thumb_mode (GTK_MENU (submenu), thumb_mode);
        }
    }
}
