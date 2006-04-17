/*
 * This file is part of Maemo Examples
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-file-details-dialog.h>
#include <gtk/gtk.h>

/* Application UI data struct */
typedef struct _AppData AppData;
struct _AppData {
    HildonApp *app;
    HildonAppView *appview;
};

void callback_file_details(GtkWidget * widget, AppData * data)
{
    HildonFileDetailsDialog *dialog;
    GtkWidget *label = NULL;
    gint result;

    /* Create dialog */
    dialog = HILDON_FILE_DETAILS_DIALOG
        (hildon_file_details_dialog_new(GTK_WINDOW(data->app), TEXT_FILE));

    /* Set the dialog to show tabs */
    g_object_set(dialog, "show-tabs", TRUE, NULL);

    /* Create some content to additional tab */
    label =
        gtk_label_new(g_strdup_printf
                      ("Name of this \nfile really is:\n%s", TEXT_FILE)
        );

    /* Add content to additional tab label */
    g_object_set(dialog, "additional-tab", label, NULL);

    /* Change tab label */
    g_object_set(dialog, "additional-tab-label", "More Info", NULL);

    /* Show the dialog */
    gtk_widget_show_all(GTK_WIDGET(dialog));

    /* Wait for user to select OK or CANCEL */
    result = gtk_dialog_run(GTK_DIALOG(dialog));

    /* Close the dialog */
    gtk_widget_destroy(GTK_WIDGET(dialog));

}

int main(int argc, char *argv[])
{
    /* Create needed variables */
    HildonApp *app;
    HildonAppView *appview;
    GtkWidget *button;
    GtkWidget *vbox;

    /* Initialize the GTK. */
    gtk_init(&argc, &argv);

    /* Create the hildon application and setup the title */
    app = HILDON_APP(hildon_app_new());
    hildon_app_set_title(app, ("Hello maemo!"));
    appview = HILDON_APPVIEW(hildon_appview_new(NULL));

    /* Create AppData */
    AppData *appdata;
    appdata = g_new0(AppData, 1);
    appdata->app = app;
    appdata->appview = appview;

    /* Create buttons and add it to main view */
    vbox = gtk_vbox_new(TRUE, 5);

    /* Test */
    button = gtk_button_new_with_label("File Details");
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(callback_file_details), appdata);
    gtk_container_add(GTK_CONTAINER(vbox), button);

    /* Add VBox to AppView */
    gtk_container_add(GTK_CONTAINER(appview), vbox);

    /* Set application view */
    hildon_app_set_appview(app, appview);

    /* Begin the main application */
    gtk_widget_show_all(GTK_WIDGET(app));
    gtk_main();

    /* Exit */
    return 0;
}
