#include <config.h>
#include <string.h>
#include "osso-mime.h"

int
main (int argc, char** argv)
{
	GnomeVFSFileInfo  *file_info;
	gchar            **names;
	gint               i;

	if (argc < 2) {
		g_printerr ("Usage: %s <mime-type> [dir]\n", argv[0]);
		return 1;
	}

	file_info = NULL;
	if (argc > 2) {
		if (strcmp (argv[2], "dir") == 0) {
			file_info = gnome_vfs_file_info_new ();
			file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_TYPE;
			file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
		}
	}

	names = osso_mime_get_icon_names (argv[1], file_info);

	i = 0;
	while (names[i]) {
		g_print ("Icon name: %s\n", names[i++]);
	}

	g_strfreev (names);

	if (file_info) {
		gnome_vfs_file_info_unref (file_info);
	}
	
	return 0;
}
