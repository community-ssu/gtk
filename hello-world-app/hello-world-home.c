#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <glib.h>
#include <gtk/gtk.h>

#include <hildon-home-plugin/hildon-home-plugin-interface.h>

#include "libhelloworld.h"

void *
hildon_home_applet_lib_initialize (void *state_data, 
				   int *state_size,
				   GtkWidget **applet_return)
{
  GtkWidget *button;

  fprintf (stderr, "hello-world initialize %p %d\n",
	   state_data, *state_size);

  button = hello_world_button_new (10);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (hello_world_dialog_show),
		    NULL);

  *applet_return = button;

  return NULL;
}

int
hildon_home_applet_lib_save_state (void *raw_data,
				   void **state_data, 
				   int *state_size)
{
  fprintf (stderr, "hello-world save_state\n");

  *state_data = NULL;
  *state_size = 0;
  return 0;
}

void
hildon_home_applet_lib_background (void *raw_data)
{
  fprintf (stderr, "hello-world background\n");
}

void
hildon_home_applet_lib_foreground (void *raw_data)
{
  fprintf (stderr, "hello-world foreground\n");
}

void
hildon_home_applet_lib_deinitialize (void *raw_data)
{
  fprintf (stderr, "hello-world deinitialize\n");
}

GtkWidget *
hildon_home_applet_lib_settings (void *applet_data,
				 GtkWindow *parent)
{
  fprintf (stderr, "hello-world settings\n");
  return NULL;
}
