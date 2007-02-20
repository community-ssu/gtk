/*
 * This file is part of hildon-desktop
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

#include <gtk/gtk.h>
#include <libhildondesktop/libhildondesktop.h>
#include <libhildondesktop/hn-app-switcher.h>
#include <gdk/gdkwindow.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

int 
main (int argc, char **argv)
{
  GtkWidget *panel,
  	    *panel2,
  	    *button,
	    *button2,
	    *button3,
	    *button4,
	    *window,
	    *window2,
	    *window_t,
	    *socket,
	    *sbold;

  Display    *display;
  Window      win;
  Atom	      atoms[3];
       	
  gtk_init (&argc,&argv);

  socket  = g_object_new (STATUSBAR_TYPE_ITEM_SOCKET,NULL);
  window  = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  window2 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  window_t  = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  panel   = g_object_new (DESKTOP_TYPE_PANEL,NULL); 
  panel2  = g_object_new (DESKTOP_TYPE_PANEL,"orientation",GTK_ORIENTATION_VERTICAL,NULL);

  gtk_window_set_type_hint( GTK_WINDOW(window),GDK_WINDOW_TYPE_HINT_DOCK);
  gtk_window_set_type_hint( GTK_WINDOW(window2),GDK_WINDOW_TYPE_HINT_DOCK);
  gtk_window_set_type_hint( GTK_WINDOW(window_t),GDK_WINDOW_TYPE_HINT_DESKTOP);
  
  gtk_widget_realize (GTK_WIDGET (window_t)); 

  gtk_widget_set_size_request (GTK_WIDGET (window_t), gdk_screen_width (), gdk_screen_height ());

  gtk_window_set_decorated (GTK_WINDOW (window),FALSE);

  gtk_widget_realize (window);
  gtk_widget_realize (window2);
  
  gtk_widget_set_size_request (GTK_WIDGET (panel),300,40);
  gtk_widget_set_size_request (GTK_WIDGET (panel2),80,gdk_screen_height ()); 
   
  button  = gtk_button_new_with_label("Testing");
  button2 = GTK_WIDGET (statusbar_item_wrapper_new ("load","/usr/lib/hildon-status-bar/libload.so",FALSE));
  button4 = GTK_WIDGET (hn_app_switcher_new ());
  button3 = GTK_WIDGET (tasknavigator_item_wrapper_new ("contacts","/usr/lib/hildon-navigator/libosso-contact-plugin.so"));
  sbold   = gtk_button_new_with_label("Testing 4");
 
  gtk_box_pack_start (GTK_BOX (panel),button,  FALSE,FALSE,0);
  gtk_box_pack_start (GTK_BOX (panel),socket,  FALSE,FALSE,0);
  gtk_box_pack_start (GTK_BOX (panel),button2, FALSE,FALSE,0);
  gtk_box_pack_start (GTK_BOX (panel),sbold, FALSE,FALSE,0);
  gtk_box_pack_start (GTK_BOX (panel2),button3, FALSE,FALSE,0);
  gtk_box_pack_start (GTK_BOX (panel2),button4, FALSE,FALSE,0);

  gtk_container_add (GTK_CONTAINER (window),panel);
  gtk_container_add (GTK_CONTAINER (window2),panel2);

  gtk_widget_realize (socket);

  printf ("%d\n",hildon_desktop_item_socket_get_id (HILDON_DESKTOP_ITEM_SOCKET(socket)));

  gtk_widget_show_all (window_t); 
  gtk_widget_show_all (window);
  gtk_widget_show_all (window2);

  gdk_window_move (GDK_WINDOW (window->window), 400, 0);
   
  gtk_main();

  return 0;
}

