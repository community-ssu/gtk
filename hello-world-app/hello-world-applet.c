#include <libosso.h>
#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>
#include <gtk/gtk.h>

#include "libhelloworld.h"

osso_return_t
execute (osso_context_t *osso, gpointer data,
	 gboolean user_activated)
{
  hello_world_dialog_show ();
  return OSSO_OK;    
}
