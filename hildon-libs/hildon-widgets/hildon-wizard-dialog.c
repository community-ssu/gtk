/*
 * This file is part of hildon-libs
 *
 * Copyright (C) 2005, 2006 Nokia Corporation, all rights reserved.
 *
 * Contact: Michael Dominic Kostrzewa <michael.kostrzewa@nokia.com>
 *   Fixes: Michael Dominic Kostrzewa <michael.kostrzewa@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License or any later version.
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
 * SECTION:hildon-wizard-dialog
 * @short_description: A widget to create a guided installation
 * process wizard
 *
 * #HildonWizardDialog is a widget to create a guided installation
 * process. The dialog has four standard buttons, previous, next,
 * finish, cancel, and contains several pages with optional icons.
 * Response buttons are dimmed/undimmed automatically and the standard
 * icon is shown/hidden in response to page navigation. The notebook
 * widget provided by users contains the actual wizard pages.
 */

#include <gtk/gtkdialog.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>
#include <hildon-widgets/hildon-defines.h>

#include "hildon-wizard-dialog.h"

#include <libintl.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _(String) dgettext(PACKAGE, String)

static GtkDialogClass *parent_class;

static void class_init              (HildonWizardDialogClass   *wizard_dialog_class);

static void init                    (HildonWizardDialog        *wizard_dialog);

static void create_title            (HildonWizardDialog        *wizard_dialog);

static void set_property            (GObject                   *object,
                                     guint                     property_id,
                                     const GValue              *value,
                                     GParamSpec                *pspec);

static void get_property            (GObject                   *object,
                                     guint                     property_id,
                                     GValue                    *value,
                                     GParamSpec                *pspec);

static void finalize                (GObject                   *object);

static void response                (HildonWizardDialog        *wizard, 
                                     gint                      response_id,
                                     gpointer                  unused);

static void make_buttons_sensitive  (HildonWizardDialog *wizard_dialog,
                                     gboolean           previous,
                                     gboolean           finish,
                                     gboolean next);

enum {
    PROP_ZERO,
    PROP_WIZARD_NAME,
    PROP_WIZARD_NOTEBOOK
};

struct _HildonWizardDialogPrivate {
    gchar       *wizard_name;
    GtkNotebook *notebook;
    GtkBox      *box;
    GtkWidget   *image;
};


GType
hildon_wizard_dialog_get_type (void)
{
    static GType wizard_dialog_type = 0;

    if (!wizard_dialog_type) {

        static const GTypeInfo wizard_dialog_info = {
            sizeof (HildonWizardDialogClass),
            NULL,       /* base_init      */
            NULL,       /* base_finalize  */
            (GClassInitFunc) class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data     */
            sizeof (HildonWizardDialog),
            0,          /* n_preallocs    */
            (GInstanceInitFunc) init,
        };

        wizard_dialog_type = g_type_register_static (GTK_TYPE_DIALOG,
                                                     "HildonWizardDialog",
                                                     &wizard_dialog_info,
                                                     0);
    }

    return wizard_dialog_type;
}

static void
class_init (HildonWizardDialogClass *wizard_dialog_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (wizard_dialog_class);

    parent_class = g_type_class_peek_parent (wizard_dialog_class);

    g_type_class_add_private (wizard_dialog_class,
                              sizeof(HildonWizardDialogPrivate));

    /* Override virtual methods */
    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->finalize     = finalize;

    /**
     * HildonWizardDialog:wizard-name:
     *
     * The name of the wizard.
     */
    g_object_class_install_property (object_class, PROP_WIZARD_NAME,
            g_param_spec_string 
            ("wizard-name",
             "Wizard Name",
             "The name of the HildonWizardDialog",
             NULL,
             G_PARAM_READWRITE));

    /**
     * HildonWizardDialog:wizard-notebook:
     *
     * The notebook object, which is used by the HildonWizardDialog.
     */
    g_object_class_install_property(object_class, PROP_WIZARD_NOTEBOOK,
            g_param_spec_object 
            ("wizard-notebook",
             "Wizard Notebook",
             "GtkNotebook object to be used in the "
             "HildonWizardDialog",
             GTK_TYPE_NOTEBOOK, G_PARAM_READWRITE));
}

static void 
finalize (GObject *object)
{
    HildonWizardDialog *dialog = HILDON_WIZARD_DIALOG (object);
    g_return_if_fail (dialog != NULL);

    if (dialog->priv->wizard_name != NULL)
        g_free (HILDON_WIZARD_DIALOG (object)->priv->wizard_name);
    
    if (G_OBJECT_CLASS (parent_class)->finalize)
        G_OBJECT_CLASS (parent_class)->finalize(object);
}

/* Disable or enable the Previous, Next and Finish buttons */
static void
make_buttons_sensitive (HildonWizardDialog *wizard_dialog,
                        gboolean previous,
                        gboolean finish,
                        gboolean next)
{
    gtk_dialog_set_response_sensitive (GTK_DIALOG (wizard_dialog),
                                       HILDON_WIZARD_DIALOG_PREVIOUS,
                                       previous);

    gtk_dialog_set_response_sensitive (GTK_DIALOG (wizard_dialog),
                                       HILDON_WIZARD_DIALOG_FINISH,
                                       finish);

    gtk_dialog_set_response_sensitive (GTK_DIALOG (wizard_dialog),
                                       HILDON_WIZARD_DIALOG_NEXT,
                                       next);
}

static void 
init (HildonWizardDialog *wizard_dialog)
{
    /* Initialize private structure for faster member access */
    HildonWizardDialogPrivate *priv =
        G_TYPE_INSTANCE_GET_PRIVATE (wizard_dialog,
                HILDON_TYPE_WIZARD_DIALOG,
                HildonWizardDialogPrivate);

    GtkDialog *dialog = GTK_DIALOG (wizard_dialog);

    /* Init internal widgets */
    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    gtk_dialog_set_has_separator (dialog, FALSE);
    wizard_dialog->priv = priv;
    priv->box = GTK_BOX (gtk_hbox_new (FALSE, 0));
    priv->image = gtk_image_new_from_icon_name ("qgn_widg_wizard",
            HILDON_ICON_SIZE_WIDG_WIZARD);

    /* Default values for user provided properties */
    priv->notebook = NULL;
    priv->wizard_name = NULL;

    /* Build wizard layout */
    gtk_box_pack_start_defaults (GTK_BOX (dialog->vbox), GTK_WIDGET (priv->box));
    gtk_box_pack_start_defaults (GTK_BOX (priv->box), GTK_WIDGET (vbox));
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (priv->image), FALSE, FALSE, 0);

    /* Add response buttons: finish, previous, next, cancel */
    gtk_dialog_add_button (dialog, _("ecdg_bd_wizard_finish"), HILDON_WIZARD_DIALOG_FINISH);
    gtk_dialog_add_button (dialog, _("ecdg_bd_wizard_previous"), HILDON_WIZARD_DIALOG_PREVIOUS);
    gtk_dialog_add_button (dialog, _("ecdg_bd_wizard_next"), HILDON_WIZARD_DIALOG_NEXT);
    gtk_dialog_add_button (dialog, _("ecdg_bd_wizard_cancel"), HILDON_WIZARD_DIALOG_CANCEL);

    /* Set initial button states: previous and finish buttons are disabled */
    make_buttons_sensitive (wizard_dialog, FALSE, FALSE, TRUE);

    /* connect to dialog's response signal */
    g_signal_connect (G_OBJECT (dialog), "response",
            G_CALLBACK (response), NULL);
}

static void
set_property (GObject      *object, 
              guint        property_id,
              const GValue *value, 
              GParamSpec   *pspec)
{
    HildonWizardDialogPrivate *priv = HILDON_WIZARD_DIALOG(object)->priv;

    switch (property_id) {

        case PROP_WIZARD_NAME: 

            /* Set new wizard name. This name will appear in titlebar */
            if (priv->wizard_name)
                g_free (priv->wizard_name);

            gchar *str = (gchar *) g_value_get_string (value);
            g_return_if_fail (str != NULL);

            priv->wizard_name = g_strdup (str);

            /* We need notebook in order to create title, since page information
               is used in title generation */
            
            if (priv->notebook)
                create_title (HILDON_WIZARD_DIALOG (object));
    
            break;

        case PROP_WIZARD_NOTEBOOK: {

            GtkNotebook *book = GTK_NOTEBOOK (g_value_get_object (value));
            g_return_if_fail (book != NULL);

            priv->notebook = book;

            /* Set the default properties for the notebook (disable tabs,
             * and remove borders) to make it look like a nice wizard widget */
            gtk_notebook_set_show_tabs (priv->notebook, FALSE);
            gtk_notebook_set_show_border (priv->notebook, FALSE);
            gtk_box_pack_start_defaults (GTK_BOX( priv->box), GTK_WIDGET (priv->notebook));

            /* Update dialog title to reflect current page stats etc */        
            if (priv->wizard_name)
                create_title (HILDON_WIZARD_DIALOG (object));
            
            } break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
get_property (GObject      *object,
              guint        property_id,
              GValue       *value,
              GParamSpec   *pspec)
{
    HildonWizardDialogPrivate *priv = HILDON_WIZARD_DIALOG (object)->priv;

    switch (property_id) {

        case PROP_WIZARD_NAME:
            g_value_set_string (value, priv->wizard_name);
            break;

        case PROP_WIZARD_NOTEBOOK:
            g_value_set_object (value, priv->notebook);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

/*
 * Creates the title of the dialog taking into account the current 
 * page of the notebook.
 */
static void
create_title (HildonWizardDialog *wizard_dialog)
{
    gint pages, current;
    gchar *str = NULL;
    HildonWizardDialogPrivate *priv = wizard_dialog->priv;
    GtkNotebook *notebook = priv->notebook;

    if (!notebook)
        return;

    /* Get page information, we'll need that when creating title */
    pages = gtk_notebook_get_n_pages (notebook);
    current = gtk_notebook_get_current_page (priv->notebook);
    if (current < 0)
        current = 0;

    /* the welcome title on the initial page */
    if (current == 0) {
        str = g_strdup_printf (_("ecdg_ti_wizard_welcome"), 
                priv->wizard_name, pages);
    } else {
        const gchar *steps = gtk_notebook_get_tab_label_text (notebook,
                gtk_notebook_get_nth_page (notebook, current));

        str = g_strdup_printf (_("ecdg_ti_wizard_step"), 
                priv->wizard_name, current + 1, pages, steps);
    }

    /* Update the dialog to display the generated title */
    gtk_window_set_title (GTK_WINDOW (wizard_dialog), str);
    g_free (str);
}

/*
 * Response signal handler. This function is needed because GtkDialog's 
 * handler for this signal closes the dialog and we don't want that, we 
 * want to change pages and, dimm certain response buttons. Overriding the 
 * virtual function would not work because that would be called after the 
 * signal handler implemented by GtkDialog.
 * FIXME: There is a much saner way to do that [MDK]
 */
static void 
response (HildonWizardDialog   *wizard_dialog,
          gint                 response_id,
          gpointer             unused)
{
    HildonWizardDialogPrivate *priv = wizard_dialog->priv;
    GtkNotebook *notebook = priv->notebook;
    gint current = 0;
    gint last = gtk_notebook_get_n_pages (notebook) - 1;
    gboolean is_first, is_last;
    
    switch (response_id) {
        
        case HILDON_WIZARD_DIALOG_PREVIOUS:
            gtk_notebook_prev_page (notebook); /* go to previous page */
            break;

        case HILDON_WIZARD_DIALOG_NEXT:
            gtk_notebook_next_page (notebook); /* go to next page */
            break;

        case HILDON_WIZARD_DIALOG_CANCEL:      
        case HILDON_WIZARD_DIALOG_FINISH:      
            return;

    }

    current = gtk_notebook_get_current_page (notebook);
    is_last = current == last;
    is_first = current == 0;
    
    /* If first page, previous and finish are disabled, 
       if last page, next is disabled */
    make_buttons_sensitive (wizard_dialog,
            !is_first, !is_first, !is_last);
    
    /* Don't let the dialog close */
    g_signal_stop_emission_by_name (wizard_dialog, "response");

    /* We show the default image on first and last pages */
    if (current == last || current == 0)
        gtk_widget_show (GTK_WIDGET(priv->image));
    else
        gtk_widget_hide (GTK_WIDGET(priv->image));

    /* New page number may appear in the title, update it */
    create_title (wizard_dialog);
}

/**
 * hildon_wizard_dialog_new:
 * @parent: a #GtkWindow
 * @wizard_name: the name of dialog
 * @notebook: the notebook to be shown on the dialog
 *
 * Creates a new #HildonWizardDialog.
 *
 * Returns: a new #HildonWizardDialog
 */
GtkWidget*
hildon_wizard_dialog_new (GtkWindow   *parent,
                          const char  *wizard_name,
                          GtkNotebook *notebook)
{
    GtkWidget *widget;

    g_return_val_if_fail (GTK_IS_NOTEBOOK (notebook), NULL);

    widget = GTK_WIDGET (g_object_new
            (HILDON_TYPE_WIZARD_DIALOG,
             "wizard-name", wizard_name,
             "wizard-notebook", notebook, NULL));

    if (parent)
        gtk_window_set_transient_for (GTK_WINDOW (widget), parent);

    return widget;
}
