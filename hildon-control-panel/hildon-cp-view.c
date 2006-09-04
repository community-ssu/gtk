/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */


#include "hildon-cp-view.h"
#include "hildon-cp-applist.h"
#include "hildon-cp-item.h"
#include "hildon-control-panel-main.h"

#include <libintl.h>

#define _(a) gettext(a)

static GtkWidget *first_grid = NULL;


static GtkWidget*
hcp_view_create_grid (void)
{
    GtkWidget *grid;

    grid = cp_grid_new();
    gtk_widget_set_name (grid, "hildon-control-panel-grid");

    return grid;
}

static GtkWidget*
hcp_view_create_separator (const gchar *label)
{
    GtkWidget *hbox         = gtk_hbox_new (FALSE, 5);
    GtkWidget *separator_1  = gtk_hseparator_new ();
    GtkWidget *separator_2  = gtk_hseparator_new ();
    GtkWidget *label_1      = gtk_label_new (label);

    gtk_widget_set_name (separator_1, "hildon-control-panel-separator");
    gtk_widget_set_name (separator_2, "hildon-control-panel-separator");
    gtk_box_pack_start (GTK_BOX(hbox), separator_1, TRUE, TRUE, 5);
    gtk_box_pack_start (GTK_BOX(hbox), label_1, FALSE, FALSE, 5);
    gtk_box_pack_start (GTK_BOX(hbox), separator_2, TRUE, TRUE, 5);

    return hbox;
}

static void
hcp_view_launch_item (GtkWidget *item_widget, HCPItem *item)
{
    hcp_item_launch (item, TRUE);
}

static gboolean
hcp_view_focus (GtkWidget *item_widget, GdkEventFocus *event, HCPItem *item)
{
    hcp_item_focus (item);
    return FALSE;
}


static void
hcp_view_add_item (HCPItem *item, GtkWidget *grid)
{
    GtkWidget *grid_item;

    grid_item = cp_grid_item_new_with_label (
            item->icon,
            _(item->name));
    
    g_signal_connect (grid_item, "focus-in-event",
            G_CALLBACK (hcp_view_focus),
            item);
    
    g_signal_connect (grid_item, "activate",
            G_CALLBACK (hcp_view_launch_item),
            item);
    
    gtk_container_add (GTK_CONTAINER(grid), grid_item);

    item->grid_item = grid_item;
}

static void
hcp_view_add_category (HCPCategory *category, GtkWidget *view)
{
    GList *focus_chain = NULL;

    /* If a group has items */
    if (category->applets)
    {
        GtkWidget *grid, *separator;
        
        grid = hcp_view_create_grid ();

        /* if we are creating a group with a defined name, we use
         * it in the separator */
        separator = hcp_view_create_separator (_(category->name));

        /* Pack the separator and the corresponding grid to the vbox */
        gtk_box_pack_start (GTK_BOX (view), separator, FALSE, FALSE, 5);
        gtk_box_pack_start (GTK_BOX (view), grid, TRUE, TRUE, 5);

        /* Why do we explicitely need to do this, shouldn't GTK take
         * care of it? -- Jobi
         */
        gtk_container_get_focus_chain (GTK_CONTAINER (view), &focus_chain);
        focus_chain = g_list_append(focus_chain, grid);
        gtk_container_set_focus_chain (GTK_CONTAINER (view), focus_chain);
        g_list_free (focus_chain);

        g_slist_foreach (category->applets,
                         (GFunc)hcp_view_add_item,
                         grid);

        if (!first_grid)
            first_grid = grid;
    }
}
    





GtkWidget *
hcp_view_new ()
{
    GtkWidget *vbox;

    vbox = gtk_vbox_new (FALSE, 6);

    return vbox;
}


void
hcp_view_populate (GtkWidget *view, HCPAppList *al)
{
    gtk_container_foreach (GTK_CONTAINER (view),
                           (GtkCallback)gtk_widget_destroy,
                           NULL);

    first_grid = NULL;
        
    gtk_container_set_focus_chain (GTK_CONTAINER (view), NULL);

    g_slist_foreach (al->categories,
                     (GFunc)hcp_view_add_category,
                     view);

    /* Put focus on the first item of the first grid */
    if (first_grid)
        gtk_widget_grab_focus (first_grid);

    
}

