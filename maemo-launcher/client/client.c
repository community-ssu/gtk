/*
 * $Id$
 *
 * Copyright (C) 2005 Nokia
 *
 * Author: Michael Natterer <mitch@imendio.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#include <gtk/gtk.h>
#include <hildon-widgets/hildon-app.h>

int
main(int argc, char *argv[])
{
  GTimer *timer;
  GtkWidget *view;
  GtkWidget *app;

  timer = g_timer_new();

  gtk_init(&argc, &argv);
  g_print("gtk_init() took %f seconds\n", g_timer_elapsed(timer, NULL));

  view = hildon_appview_new("maemo-client");
  app = hildon_app_new_with_appview(HILDON_APPVIEW(view));
  g_signal_connect(app, "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_print("creating widgets took %f seconds\n", g_timer_elapsed(timer, NULL));

  gtk_widget_show_all(app);
  g_print("showing widgets took %f seconds\n", g_timer_elapsed(timer, NULL));

  g_timer_destroy(timer);

  gtk_main();

  return 0;
}

