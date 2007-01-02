#include <config.h>
#include "osso-mime.h"

int
main (int argc, char** argv)
{
	GList *list, *l;

	if (argc < 2) {
		g_printerr ("Usage: %s <app-id>\n", argv[0]);
		return 1;
	}

	list = osso_mime_application_get_mime_types (argv[1]);
	if (list) {
		g_print ("Supported mime types:\n");
		for (l = list; l; l = l->next) {
			g_print (" %s\n", (gchar *) l->data);
		}
		osso_mime_application_mime_types_list_free (list);
	} else {
		g_print ("No supported mime types\n");
	}
	
	return 0;
}
