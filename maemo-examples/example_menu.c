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

/* Includes */
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <gtk/gtk.h>            /* XXX: Change gtkmain to gtk */

/* Callback for "Close" menu entry */
void item_close_cb()
{
    g_print("Closing application...\n");
    gtk_main_quit();
}

/* Create the menu items needed for the main view */
static void create_menu(HildonAppView * main_view)
{
    /* Create needed variables */
    GtkMenu *main_menu;
    GtkWidget *menu_others;
    GtkWidget *item_others;
    GtkWidget *item_radio1;
    GtkWidget *item_radio2;
    GtkWidget *item_check;
    GtkWidget *item_close;
    GtkWidget *item_separator;

    /* Get the menu from view */
    main_menu = hildon_appview_get_menu(main_view);

    /* Create new submenu for "Others" */
    menu_others = gtk_menu_new();

    /* Create menu items */
    item_others = gtk_menu_item_new_with_label("Others");
    item_radio1 = gtk_radio_menu_item_new_with_label(NULL, "Radio1");
    item_radio2 =
        gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM
                                                       (item_radio1),
                                                       "Radio2");
    item_check = gtk_check_menu_item_new_with_label("Check");
    item_close = gtk_menu_item_new_with_label("Close");
    item_separator = gtk_separator_menu_item_new();

    /* Add menu items to right menus */
    gtk_menu_append(main_menu, item_others);
    gtk_menu_append(menu_others, item_radio1);
    gtk_menu_append(menu_others, item_radio2);
    gtk_menu_append(menu_others, item_separator);
    gtk_menu_append(menu_others, item_check);
    gtk_menu_append(main_menu, item_close);

    /* Add others submenu to the "Others" item */
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_others), menu_others);

    /* Attach the callback functions to the activate signal */
    g_signal_connect(G_OBJECT(item_close), "activate",
                     GTK_SIGNAL_FUNC(item_close_cb), NULL);

    /* Make all menu widgets visible */
    gtk_widget_show_all(GTK_WIDGET(main_menu));
}

/* Main application */
int main(int argc, char *argv[])
{
    /* Create needed variables */
    HildonApp *app;
    HildonAppView *appview;

    /* Initialize the GTK. */
    gtk_init(&argc, &argv);

    /* Create the hildon application and setup the title */
    app = HILDON_APP(hildon_app_new());
    hildon_app_set_title(app, "Menu Example");

    /* Create HildonAppView and set it to HildonApp */
    appview = HILDON_APPVIEW(hildon_appview_new(NULL));
    hildon_app_set_appview(app, appview);

    /* Add example label to appview */
    gtk_container_add(GTK_CONTAINER(appview),
                      gtk_label_new("Menu Example"));

    /* Create menu for view */
    create_menu(appview);

    /* Begin the main application */
    gtk_widget_show_all(GTK_WIDGET(app));
    gtk_main();

    /* Exit */
    return 0;
}
