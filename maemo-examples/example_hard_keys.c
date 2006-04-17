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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* Callback for hardware keys */
gboolean key_press_cb(GtkWidget * widget, GdkEventKey * event,
                      HildonApp * app)
{
    switch (event->keyval) {
    case GDK_Up:
        gtk_infoprint(GTK_WINDOW(app), "Navigation Key Up");
        return TRUE;

    case GDK_Down:
        gtk_infoprint(GTK_WINDOW(app), "Navigation Key Down");
        return TRUE;

    case GDK_Left:
        gtk_infoprint(GTK_WINDOW(app), "Navigation Key Left");
        return TRUE;

    case GDK_Right:
        gtk_infoprint(GTK_WINDOW(app), "Navigation Key Right");
        return TRUE;

    case GDK_Return:
        gtk_infoprint(GTK_WINDOW(app), "Navigation Key select");
        return TRUE;

    case GDK_F6:
        gtk_infoprint(GTK_WINDOW(app), "Full screen");
        return TRUE;

    case GDK_F7:
        gtk_infoprint(GTK_WINDOW(app), "Increase (zoom in)");
        return TRUE;

    case GDK_F8:
        gtk_infoprint(GTK_WINDOW(app), "Decrease (zoom out)");
        return TRUE;

    case GDK_Escape:
        gtk_infoprint(GTK_WINDOW(app), "Cancel/Close");
        return TRUE;
    }

    return FALSE;
}

/* Main application */
int main(int argc, char *argv[])
{
    /* Create needed variables */
    HildonApp *app;
    HildonAppView *appview;
    GtkWidget *main_vbox;
    GtkWidget *label;

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
    label = gtk_label_new("Press Hardware Keys!");
    gtk_box_pack_start(GTK_BOX(main_vbox), label, FALSE, TRUE, 0);

    /* Add hardware button listener to application */
    g_signal_connect(G_OBJECT(app),
                     "key_press_event", G_CALLBACK(key_press_cb), app);

    /* Begin the main application */
    gtk_widget_show_all(GTK_WIDGET(app));
    gtk_main();

    /* Exit */
    return 0;
}
