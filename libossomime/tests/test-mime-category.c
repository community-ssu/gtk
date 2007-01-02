#include <config.h>
#include <libgnomevfs/gnome-vfs.h>
#include <osso-mime.h>

int
main (int argc, char **argv)
{
	gchar            *name;
	OssoMimeCategory  category;

	gnome_vfs_init ();

	if (argc < 2) {
		g_printerr ("Usage: %s <mime type>\n", argv[0]);
		return 1;
	}

	name = argv[1];
	category = osso_mime_get_category_for_mime_type (name);

	g_print ("category is: %s (%d)\n",
		 osso_mime_get_category_name (category), category);

	return 0;
}

