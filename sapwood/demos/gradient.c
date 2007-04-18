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

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *align;
  GtkWidget *button;

  gtk_rc_add_default_file ("gradient.gtkrc");
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  button = gtk_button_new_with_label ("OK");

  gtk_container_add (GTK_CONTAINER (window), align);
  gtk_container_add (GTK_CONTAINER (align), button);

  gtk_widget_set_size_request (button, 100, 100);

  gtk_widget_show_all (window);

  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_main ();

  return 0;
}
