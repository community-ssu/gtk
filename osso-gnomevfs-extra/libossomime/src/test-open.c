#include <config.h>
#include "osso-mime.h"

int
main (int argc, char** argv)
{
	DBusConnection *conn;
	gint            ret;

	if (argc < 2) {
		g_printerr ("Usage: %s <URI> [<mime-type>]\n", argv[0]);
		return 1;
	}
	
	conn = dbus_bus_get (DBUS_BUS_SESSION, NULL);
	g_assert (conn != NULL);

	g_print ("\nTesting osso_mime_open_file() with URI:'%s'\n", argv[1]);
	ret = osso_mime_open_file (conn, argv[1]);
	if (ret != 1) {
		g_print ("Error\n");
	} else {
		g_print ("Success\n");
	}
	
	g_print ("\nTesting osso_mime_open_file_list() with URI:'%s'\n", argv[1]);
	ret = osso_mime_open_file_list (conn, g_slist_append (NULL, argv[1]));
	if (ret != 1) {
		g_print ("Error\n");
	} else {
		g_print ("Success\n");
	}

	if (argc > 2 && argv[2]) {
		g_print ("\nTesting osso_mime_open_file_with_mime() with URI:'%s' and MIME:'%s'\n", 
                         argv[1], argv[2]);
		ret = osso_mime_open_file_with_mime_type (conn, argv[1], argv[2]);
		if (ret != 1) {
			g_print ("Error\n");
		} else {
			g_print ("Success\n");
		}
	}
	
	return 0;
}
