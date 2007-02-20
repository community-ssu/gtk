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

#include <libhildondesktop/hn-app-switcher.h>
#include <libhildonwm/hd-wm.h>

static void 
show_a_menu (GtkWidget *widget, gpointer data)
{
  hd_wm_activate (HD_TN_ACTIVATE_MAIN_MENU);
}

int 
main (int argc, char **argv)
{
  GtkWidget *win,*hbox,*button;
	
  gtk_init (&argc,&argv);

  HDWM *wm = hd_wm_get_singleton ();

  win    = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  hbox   = gtk_hbox_new (TRUE,0);
  button = gtk_button_new ();
  
  gtk_window_set_type_hint (GTK_WINDOW (win), GDK_WINDOW_TYPE_HINT_DESKTOP);
  
  gtk_container_add (GTK_CONTAINER (win), GTK_WIDGET (hbox));
  
  gtk_box_pack_start (GTK_BOX (hbox), g_object_new (HN_TYPE_APP_SWITCHER,"n_items",3,"orientation",GTK_ORIENTATION_HORIZONTAL,NULL),TRUE,TRUE,0);
  gtk_box_pack_start (GTK_BOX (hbox), g_object_new (HN_TYPE_APP_SWITCHER,"n_items",5,"orientation",GTK_ORIENTATION_VERTICAL,NULL),TRUE,TRUE,0);
  gtk_box_pack_start (GTK_BOX (hbox), g_object_new (HN_TYPE_APP_SWITCHER,"n_items",7,"orientation",GTK_ORIENTATION_VERTICAL,NULL),TRUE,TRUE,0);
  gtk_box_pack_start (GTK_BOX (hbox), g_object_new (HN_TYPE_APP_SWITCHER,"n_items",9,"orientation",GTK_ORIENTATION_VERTICAL,NULL),TRUE,TRUE,0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE,TRUE,0);

  g_signal_connect (button,"clicked",G_CALLBACK(show_a_menu),NULL);
  
  gtk_widget_show_all (win);
  
  gtk_main ();
	
  return 0;
}

