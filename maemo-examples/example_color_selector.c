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

#include <hildon-widgets/hildon-color-selector.h>
#include <hildon-widgets/hildon-color-button.h>
#include <hildon-widgets/hildon-app.h>
#include <gtk/gtk.h>

/* Application UI data struct */
typedef struct _AppData AppData;
struct _AppData {
    HildonApp *app;
    HildonAppView *appview;
};

void color_button_clicked(GtkWidget * widget, gpointer data)
{
    GdkColor *new_color = NULL;
    g_object_get(widget, "color", &new_color, NULL);
}

void ui_show_color_selector(GtkWidget * widget, AppData * appdata)
{
    GdkColor *color;
    GtkWidget *selector;
    gint result;

    /* Create new color selector (needs access to HildonApp!) */
    selector = hildon_color_selector_new(GTK_WINDOW(appdata->app));

    /* Set the current selected color to selector */
    hildon_color_selector_set_color(HILDON_COLOR_SELECTOR(selector),
                                    color);

    /* Show dialog */
    result = gtk_dialog_run(GTK_DIALOG(selector));

    /* Wait for user to select OK or Cancel */
    switch (result) {
    case GTK_RESPONSE_OK:

        /* Get the current selected color from selector */
        color = hildon_color_selector_get_color
            (HILDON_COLOR_SELECTOR(selector));

        /* Now the new color is in 'color' variable */
        /* Use it however suitable for the application */

        gtk_widget_destroy(selector);
        break;

    default:

        /* If dialog didn't return OK then it was canceled */
        gtk_widget_destroy(selector);
        break;
    }

}


int main(int argc, char *argv[])
{
    /* Create needed variables */
    HildonApp *app;
    HildonAppView *appview;
    HildonColorButton *color_button;
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
    color_button = HILDON_COLOR_BUTTON(hildon_color_button_new());
    g_signal_connect(G_OBJECT(color_button), "clicked",
                     G_CALLBACK(color_button_clicked), NULL);
    gtk_container_add(GTK_CONTAINER(vbox), GTK_WIDGET(color_button));

    button = gtk_button_new_with_label("Color Selector");
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(ui_show_color_selector), appdata);
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
