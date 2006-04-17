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
#include <gtk/gtkmain.h>

int main(int argc, char *argv[])
{
    /* Create needed variables */
    HildonApp *app;
    HildonAppView *appview;

    /* Initialize the GTK. */
    gtk_init(&argc, &argv);

    /* Create the hildon application and setup the title */
    app = HILDON_APP(hildon_app_new());
    hildon_app_set_title(app, "App Title");
    hildon_app_set_two_part_title(app, TRUE);

    /* Create HildonAppView and set it to HildonApp */
    appview = HILDON_APPVIEW(hildon_appview_new("AppView Title"));
    hildon_app_set_appview(app, appview);

    /* Add example label to appview */
    gtk_container_add(GTK_CONTAINER(appview),
                      GTK_WIDGET(gtk_label_new("HildonAppView Example")));

    /* Begin the main application */
    gtk_widget_show_all(GTK_WIDGET(app));

    gtk_main();

    /* Exit */
    return 0;
}
