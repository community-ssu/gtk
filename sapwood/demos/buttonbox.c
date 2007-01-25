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

#define PACK_START FALSE
#define PACK_END   TRUE
#define PRIMARY    FALSE
#define SECONDARY  TRUE

typedef struct {
  const char   *label;
  gboolean      is_secondary;
  gboolean      pack_end;
} button_t;

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *bbox;
  button_t buttons_1[] = {
    {"One",   PRIMARY,   PACK_START},
    {"Two",   SECONDARY, PACK_START},
    {NULL}
  };
  button_t buttons_2[] = {
    {"One",   PRIMARY,   PACK_START},
    {"Two",   PRIMARY,   PACK_START},
    {"Three", SECONDARY, PACK_START},
    {"Four",  SECONDARY, PACK_START},
    {NULL}
  };
  button_t buttons_3[] = {
    {"One",   PRIMARY,   PACK_START},
    {"Two",   PRIMARY,   PACK_START},
    {"Three", PRIMARY,   PACK_START},
    {"Four",  SECONDARY, PACK_START},
    {"Five",  SECONDARY, PACK_START},
    {"Six",   SECONDARY, PACK_START},
    {NULL}
  };
  button_t buttons_4[] = {
    {"One",   PRIMARY,   PACK_START},
    {"Two",   PRIMARY,   PACK_START},
    {"Three", PRIMARY,   PACK_START},
    {"Four",  PRIMARY,   PACK_START},
    {"Five",  SECONDARY, PACK_START},
    {"Six",   SECONDARY, PACK_START},
    {"Seven", SECONDARY, PACK_START},
    {"Eight", SECONDARY, PACK_START},
    {NULL}
  };
  button_t *boxes[] = {
    buttons_1,
    buttons_2,
    buttons_3,
    buttons_4,
  };
  int row;

  gtk_rc_add_default_file ("buttonbox.gtkrc");
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  vbox = gtk_vbox_new (FALSE, 0);

  for (row = 0; row < G_N_ELEMENTS(boxes); row++)
    {
      button_t *buttons = boxes[row];

      bbox = gtk_hbutton_box_new ();
      gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);

      for (;;)
	{
	  GtkWidget *widget;

	  button_t *button = buttons++;
	  if (!button->label)
	    break;

	  widget = gtk_button_new_with_label (button->label);
	  if (button->pack_end)
	    gtk_box_pack_end_defaults (GTK_BOX (bbox), widget);
	  else
	    gtk_box_pack_start_defaults (GTK_BOX (bbox), widget);
	  gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (bbox), widget,
					      button->is_secondary);
	}

      gtk_container_add (GTK_CONTAINER (vbox), bbox);
    }
  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_widget_show_all (window);

  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_main ();

  return 0;
}
