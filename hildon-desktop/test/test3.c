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
#include <libhildondesktop/desktop-panel-window.h>

int 
main (int argc, char **argv)
{
  GtkWidget *window, *button, *a_button, *box;
  
  gtk_init (&argc,&argv);

  window = g_object_new (DESKTOP_TYPE_PANEL_WINDOW,
		  	 "width",300,
			 "height",100,
			 "x",0,"y",0,
			 "stretch",FALSE,
			 "move",TRUE,
			 "orientation",DPANEL_ORIENTATION_LEFT,
			  NULL);

  button   = gtk_button_new_with_label ("button 1");
  a_button = gtk_button_new_with_label ("button 2");

  gtk_box_pack_start (GTK_BOX (HILDON_DESKTOP_WINDOW (window)->container), button,TRUE,TRUE,0); 
  gtk_box_pack_start (GTK_BOX (HILDON_DESKTOP_WINDOW (window)->container), a_button,TRUE,TRUE,0); 
  
  gtk_widget_show_all (window);

  gtk_main ();  

  return 0;
}

