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
#include <hildon-widgets/hildon-find-toolbar.h>
#include <gtk/gtk.h>

/* Application UI data struct */
typedef struct _AppData AppData;
struct _AppData {

    /* View items */
    HildonApp *app;
    HildonAppView *appview;

    /* Toolbar */
    GtkWidget *main_toolbar;

    /* Find toolbar */
    HildonFindToolbar *find_toolbar;

    /* Is Find toolbar visible or not */
    gboolean find_visible;
};

/* Callback for "Close" menu entry */
void item_close_cb()
{
    g_print("Closing application...\n");
    gtk_main_quit();
}

/* Callback for "Close" toolbar button */
void tb_close_cb(GtkToolButton * widget, AppData * view)
{
    g_print("Closing application...\n");
    gtk_main_quit();
}

/* Callback for "Find" toolbar button */
void tb_find_cb(GtkToolButton * widget, AppData * view)
{
    /* Show or hide find toolbar */
    if (view->find_visible) {
        gtk_widget_hide_all(GTK_WIDGET(view->find_toolbar));
        view->find_visible = FALSE;
    } else {
        gtk_widget_show_all(GTK_WIDGET(view->find_toolbar));
        view->find_visible = TRUE;
    }
}

/* Callback for "Close" find toolbar button */
void find_tb_close(GtkWidget * widget, AppData * view)
{
    gtk_widget_hide_all(GTK_WIDGET(view->find_toolbar));
    view->find_visible = FALSE;
}

/* Callback for "Search" find toolbar button */
void find_tb_search(GtkWidget * widget, AppData * view)
{
    gchar *find_text = NULL;
    g_object_get(G_OBJECT(widget), "prefix", &find_text, NULL);
    /* Implement the searching here... */
    g_print("Search for: %s\n", find_text);
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

/* Create the find toolbar */
void ui_create_find_toolbar(AppData * view)
{
    HildonFindToolbar *find_toolbar;
    find_toolbar = HILDON_FIND_TOOLBAR
        (hildon_find_toolbar_new("Find String: "));

    /* Add signal listers to "Search" and "Close" buttons */
    g_signal_connect(G_OBJECT(find_toolbar), "search",
                     G_CALLBACK(find_tb_search), view);
    g_signal_connect(G_OBJECT(find_toolbar), "close",
                     G_CALLBACK(find_tb_close), view);
    gtk_box_pack_start(GTK_BOX(view->appview->vbox),
                       GTK_WIDGET(find_toolbar), TRUE, TRUE, 0);

    /* Set variables to AppData */
    view->find_toolbar = find_toolbar;
    view->find_visible = FALSE;
}

/* Create the toolbar for the main view */
static void create_toolbar(AppData * appdata)
{
    /* Create needed variables */
    GtkWidget *main_toolbar;
    GtkToolItem *tb_new;
    GtkToolItem *tb_open;
    GtkToolItem *tb_save;
    GtkToolItem *tb_find;
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
    tb_find = gtk_tool_button_new_from_stock(GTK_STOCK_FIND);
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
    gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar), tb_find, -1);
    gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar), tb_close, -1);

    /* Add signal lister to "Close" button */
    g_signal_connect(G_OBJECT(tb_close), "clicked",
                     G_CALLBACK(tb_close_cb), NULL);

    /* Add signal lister to "Find" button */
    g_signal_connect(G_OBJECT(tb_find), "clicked",
                     G_CALLBACK(tb_find_cb), appdata);

    /* Add toolbar to 'vbox' of HildonAppView */
    gtk_box_pack_end(GTK_BOX(appdata->appview->vbox),
                     main_toolbar, TRUE, TRUE, 0);

    gtk_widget_show_all(main_toolbar);
    appdata->main_toolbar = main_toolbar;
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
    hildon_app_set_title(app, "HildonFindToolbar Example");

    /* Create HildonAppView and set it to HildonApp */
    appview = HILDON_APPVIEW(hildon_appview_new(NULL));
    hildon_app_set_appview(app, appview);

    /* Add example label to appview */
    gtk_container_add(GTK_CONTAINER(appview),
                      gtk_label_new("HildonFindToolbar Example"));

    /* Create AppData */
    AppData *appdata;
    appdata = g_new0(AppData, 1);
    appdata->app = app;
    appdata->appview = appview;

    /* Create toolbar for view */
    create_toolbar(appdata);

    /* Create find toolbar, but keep it hidden */
    ui_create_find_toolbar(appdata);

    /* Show all other widgets except the find toolbar */
    gtk_widget_show(GTK_WIDGET(appdata->app));
    gtk_widget_show(GTK_WIDGET(appdata->appview));
    gtk_widget_show(GTK_WIDGET(appdata->appview->vbox));

    /* Begin the main application */
    gtk_main();

    /* Exit */
    return 0;
}
