#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <libosso.h>

#include "libhelloworld.h"

gint
dbus_callback (const gchar *interface, const gchar *method,
	       GArray *arguments, gpointer data,
	       osso_rpc_t *retval)
{
  printf ("hello-world dbus: %s, %s\n", interface, method);

  if (!strcmp (method, "top_application"))
      gtk_window_present (GTK_WINDOW (data));

  retval->type = DBUS_TYPE_INVALID;
  return OSSO_OK;
}

int
main (int argc, char *argv[])
{
  osso_context_t *ctxt;
  osso_return_t ret;
  GtkWindow *window;

  printf ("hello-world: starting up\n");

  ctxt = osso_initialize ("hello_world_app", PACKAGE_VERSION, TRUE, NULL);
  if (ctxt == NULL)
    {
      fprintf (stderr, "osso_initialize failed.\n");
      exit (1);
    }

  gtk_init (&argc, &argv);

  window = hello_world_new ();

  ret = osso_rpc_set_default_cb_f (ctxt, dbus_callback, window);
  if (ret != OSSO_OK)
    {
      fprintf (stderr, "osso_rpc_set_default_cb_f failed: %d.\n", ret);
      exit (1);
    }
  
  gtk_main ();
  
  return 0;
}
