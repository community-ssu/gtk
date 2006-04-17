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
#include <gtk/gtkmain.h>

static GtkWidget* popup_create(GtkWidget *window)
{
    GtkWidget *menu, *item_close;

    menu = gtk_menu_new ();

    /* Create a menu item */
    item_close = gtk_menu_item_new_with_label ("Close");
    gtk_menu_append (GTK_MENU(menu), item_close);
    gtk_widget_show (item_close);

    /* Menu item causes gtk_widget_destroy to window */
    g_signal_connect_swapped (G_OBJECT (item_close), "activate",
        G_CALLBACK (gtk_widget_destroy),
        G_OBJECT (window));

    return menu;
} 


int main(int argc, char *argv[])
{
    /* Create needed variables */
    HildonApp *app;
    GtkWidget *menu;

    /* Initialize the GTK. */
    gtk_init(&argc, &argv);

    /* Create the hildon application and setup the title */
    app = HILDON_APP(hildon_app_new());
    hildon_app_set_title(app, ("My Title"));
    
    /* Create a menu and set it as the context sensitive menu. */
    menu = popup_create(GTK_WIDGET(app));
    gtk_widget_tap_and_hold_setup(GTK_WIDGET(app), menu, NULL, 0);

    /* Begin the main application */
    gtk_widget_show_all(GTK_WIDGET(app));
    gtk_main();

    /* Exit */
    return 0;
}
