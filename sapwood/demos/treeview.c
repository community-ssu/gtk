/*
 * This file is part of sapwood
 *
 * Copyright (C) 2007 Nokia Corporation. 
 *
 * Contact: Tommi Komulainen <tommi.komulainen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <gtk/gtk.h>

static GtkWidget *
create_treeview (int n_columns)
{
  GtkWidget *treeview;
  GtkListStore *store;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  int i;

  store = gtk_list_store_new (1, G_TYPE_STRING);
  for (i = 0; i < 10; i++)
    {
      char buf[128];
      g_snprintf (buf, sizeof(buf), "Row %d", i+1);
      gtk_list_store_insert_with_values (store, &iter, -1, 0, buf, -1);
    }

  treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  while (n_columns-- > 0)
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
						 -1, "",
						 gtk_cell_renderer_text_new (),
						 "text", 0,
						 NULL);

  gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (treeview), TRUE);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

  return treeview;
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *treeview;
  GtkWidget *box;

  gtk_rc_add_default_file ("treeview.gtkrc");
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  box = gtk_vbox_new (TRUE, 6);

  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_container_add (GTK_CONTAINER (box), treeview=create_treeview (1));
  gtk_widget_set_name (treeview, "treeview-without-row-endings");
  gtk_container_add (GTK_CONTAINER (box), treeview=create_treeview (1));
  gtk_widget_set_name (treeview, "treeview-with-row-endings");
  gtk_container_add (GTK_CONTAINER (box), treeview=create_treeview (3));
  gtk_widget_set_name (treeview, "treeview-with-row-endings");

  gtk_widget_show_all (window);

  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_main ();

  return 0;
}
