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
#include <hildon-widgets/hildon-font-selection-dialog.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gtk/gtk.h>

#define GCONF_KEY "/apps/example_prefs/title"

/* Application UI data struct */
typedef struct _AppData AppData;
struct _AppData {
    HildonApp *app;
    HildonAppView *appview;
};

void commit_entry_data_callback(GtkWidget* entry)
{
	GConfClient* client;
    	HildonApp *app;
	gchar* str;

	str = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

	client = g_object_get_data(G_OBJECT (entry), "client");
	gconf_client_set_string(client, GCONF_KEY, str, NULL );

	app = g_object_get_data (G_OBJECT (entry), "app");
    	if (str != NULL && *str != '\0')
    		hildon_app_set_title(app, str);

	g_free(str);
	return ;
}

int main(int argc, char *argv[])
{
    /* Announce vars */
    HildonApp		*app;
    HildonAppView	*appview;
    GtkWidget		*entry, *label, *hbox;
    GConfClient		*client;

    /* Initialize GTK. */
    gtk_init(&argc, &argv);

    /* Get the default client */
    client = gconf_client_get_default();

    /*Add GConf node if absent*/
    gconf_client_add_dir (client, "/apps/example_prefs",
                    GCONF_CLIENT_PRELOAD_NONE, NULL);

    /* Read title */
    gchar *title = gconf_client_get_string(client, GCONF_KEY, NULL);

    /* Create the hildon application and setup the title */
    app = HILDON_APP(hildon_app_new());

    if (title != NULL) {
    	hildon_app_set_title(app, title);
    }

    appview = HILDON_APPVIEW(hildon_appview_new(NULL));

    /* Create buttons and add it to main view */
    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new ("Change window title: ");
    entry = gtk_entry_new();

    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (hbox), entry, FALSE, FALSE, 0);

    g_object_set_data (G_OBJECT (entry), "client", client);
    g_object_set_data (G_OBJECT (entry), "app", app);

    g_signal_connect(G_OBJECT(entry), "focus_out_event",
                     G_CALLBACK(commit_entry_data_callback), client);
    g_signal_connect(G_OBJECT(entry), "activate",
                     G_CALLBACK(commit_entry_data_callback), client);

    if (title) {
    	gtk_entry_set_text (GTK_ENTRY (entry), title);
    }
    g_free (title);

    gtk_widget_set_sensitive (entry,
		    gconf_client_key_is_writable (client,GCONF_KEY, NULL));

    gtk_container_add (GTK_CONTAINER (appview), hbox);

    /* Set application view */
    hildon_app_set_appview(app, appview);

    /* Begin the main application */
    gtk_widget_show_all(GTK_WIDGET(app));
    gtk_main();

    /* Exit */
    g_object_unref (G_OBJECT (client));
    return 0;
}
