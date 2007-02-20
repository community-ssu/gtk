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

int 
main (int argc, char **argv)
{
  GtkWidget *panel;
	
  gtk_init (&argc,&argv);

  HDWM *wm = hd_wm_get_singleton ();

  panel  = g_object_new (DESKTOP_TYPE_PANEL_WINDOW,
		  	 "width",
			 480,
			 "height",
			 100,
			 "orientation",
			 DPANEL_ORIENTATION_LEFT,
			 NULL);
  
  gtk_container_add (GTK_CONTAINER (HILDON_DESKTOP_WINDOW (panel)->container),
		     g_object_new (HN_TYPE_APP_SWITCHER,"n_items",3,NULL));
  
  
  gtk_widget_show_all (panel);
  
  gtk_main ();
	
  return 0;
}

