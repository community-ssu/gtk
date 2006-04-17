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
#include <hildon-widgets/hildon-appview.h>
#include <hildon-widgets/gtk-infoprint.h>
#include <gtk/gtk.h>

static gint infoprint_type = 1;

/* Callback to show infoprints */
void show_infoprint(GtkButton * widget, HildonApp * app)
{
    switch (infoprint_type) {
    case 1:
        /* Normal infoprint */
        gtk_infoprint(GTK_WINDOW(app), "Hi there!");
        break;

    case 2:
        /* Infoprint with custom icon */
        gtk_infoprint_with_icon_stock(GTK_WINDOW(app),
                                      "This is save icon", GTK_STOCK_SAVE);
        break;

    case 3:
        /* Infoprint with progressbar */
        gtk_banner_show_bar(GTK_WINDOW(app), "Info with progress bar");
        /* Set bar to be 20% full */
        gtk_banner_set_fraction(GTK_WINDOW(app), 0.2);
        break;

    case 4:
        /* Set bar to be 80% full */
        gtk_banner_set_fraction(GTK_WINDOW(app), 0.8);
        break;

    case 5:
        /* With fifth click, end the application */
        gtk_main_quit();
    }

    /* Increase the counter */
    infoprint_type++;
}


/* Main application */
int main(int argc, char *argv[])
{
    /* Create needed variables */
    HildonApp *app;
    HildonAppView *appview;
    GtkWidget *main_vbox;
    GtkWidget *button1;

    /* Initialize the GTK. */
    gtk_init(&argc, &argv);

    /* Create the hildon application and setup the title */
    app = HILDON_APP(hildon_app_new());
    hildon_app_set_title(app, "App Title");
    hildon_app_set_two_part_title(app, TRUE);

    /* Create HildonAppView and set it to HildonApp */
    appview = HILDON_APPVIEW(hildon_appview_new("AppView Title"));
    hildon_app_set_appview(app, appview);

    /* Add vbox to appview */
    main_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(appview), main_vbox);

    /* Add button to vbox */
    button1 = gtk_button_new_with_label("Show Info");
    gtk_box_pack_start(GTK_BOX(main_vbox), button1, FALSE, TRUE, 0);

    /* Add signal listener to button */
    g_signal_connect(G_OBJECT(button1), "clicked",
                     G_CALLBACK(show_infoprint), app);

    /* Begin the main application */
    gtk_widget_show_all(GTK_WIDGET(app));
    gtk_main();

    /* Exit */
    return 0;
}
