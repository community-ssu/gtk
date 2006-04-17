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
#include <gtk/gtk.h>

/* Callback for "Close" menu entry */
void item_close_cb()
{
    g_print("Closing application...\n");
    gtk_main_quit();
}

/* Callback for "Close" toolbar button */
void tb_close_cb(GtkToolButton * widget)
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

                                                  /* Create the toolbar needed for the main view *//* XXX: s/menu items/toolbar/ */
static void create_toolbar(HildonAppView * main_view)
{
    /* Create needed variables */
    GtkWidget *main_toolbar;
    GtkToolItem *tb_new;
    GtkToolItem *tb_open;
    GtkToolItem *tb_save;
    GtkToolItem *tb_close;
    GtkToolItem *tb_separator;
    GtkToolItem *tb_comboitem;
    GtkComboBox *tb_combo;

    /* Create toolbar */
    main_toolbar = gtk_toolbar_new();

    /* Create toolbar button items */
    tb_new = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
    tb_open = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
    tb_save = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
    tb_close = gtk_tool_button_new_from_stock(GTK_STOCK_CLOSE);

    /* Create toolbar combobox item */
    tb_comboitem = gtk_tool_item_new();
    tb_combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
    gtk_combo_box_append_text(tb_combo, "Entry 1");
    gtk_combo_box_append_text(tb_combo, "Entry 2");
    gtk_combo_box_append_text(tb_combo, "Entry 3");
    /* Select second item as default */
    gtk_combo_box_set_active(GTK_COMBO_BOX(tb_combo), 1);
    /* Make combobox to use all available toolbar space */
    gtk_tool_item_set_expand(tb_comboitem, TRUE);
    /* Add combobox inside toolitem */
    gtk_container_add(GTK_CONTAINER(tb_comboitem), GTK_WIDGET(tb_combo));

    /* Create separator */
    tb_separator = gtk_separator_tool_item_new();

    /* Add all items to toolbar */
    gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar), tb_new, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar), tb_separator, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar), tb_open, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar), tb_save, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar), tb_comboitem, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar), tb_close, -1);

    /* Add signal lister to "Close" button */
    g_signal_connect(G_OBJECT(tb_close), "clicked",
                     G_CALLBACK(tb_close_cb), NULL);

    /* Add toolbar to 'vbox' of HildonAppView */
    gtk_box_pack_end(GTK_BOX(main_view->vbox),
                     main_toolbar, TRUE, TRUE, 0);
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
    hildon_app_set_title(app, "HildonToolbar Example");

    /* Create HildonAppView and set it to HildonApp */
    appview = HILDON_APPVIEW(hildon_appview_new(NULL));
    hildon_app_set_appview(app, appview);

    /* Add example label to appview */
    gtk_container_add(GTK_CONTAINER(appview),
                      gtk_label_new("HildonToolbar Example"));

    /* Create menu for view */
    create_menu(appview);

    /* Create toolbar for view */
    create_toolbar(appview);

    /* Begin the main application */
    gtk_widget_show_all(GTK_WIDGET(app));
    gtk_main();

    /* Exit */
    return 0;
}
