/* gnomevfs-ls.c - Test for open_dir(), read_dir() and close_dir() for gnome-vfs

   Copyright (C) 2003, Red Hat

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Bastien Nocera <hadess@hadess.net>
*/

#include <locale.h>
#include <libgnomevfs/gnome-vfs.h>

static char *directory = NULL;

static void show_data (gpointer item, gpointer no_item);
static void list (void);

static const gchar *
type_to_string (GnomeVFSFileType type)
{
	switch (type) {
	case GNOME_VFS_FILE_TYPE_UNKNOWN:
		return "Unknown";
	case GNOME_VFS_FILE_TYPE_REGULAR:
		return "Regular";
	case GNOME_VFS_FILE_TYPE_DIRECTORY:
		return "Directory";
	case GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK:
		return "Symbolic Link";
	case GNOME_VFS_FILE_TYPE_FIFO:
		return "FIFO";
	case GNOME_VFS_FILE_TYPE_SOCKET:
		return "Socket";
	case GNOME_VFS_FILE_TYPE_CHARACTER_DEVICE:
		return "Character device";
	case GNOME_VFS_FILE_TYPE_BLOCK_DEVICE:
		return "Block device";
	default:
		return "???";
	}
}

static void
show_data (gpointer item, gpointer no_item)
{
	GnomeVFSFileInfo *info;
	char *path;

	info = (GnomeVFSFileInfo *) item;

	path = g_strconcat (directory, "/", info->name, NULL);

	g_print ("%s\t%s%s%s\t(%s, %s)\tsize %ld\tmode %04o\n",
			info->name,
			GNOME_VFS_FILE_INFO_SYMLINK (info) ? " [link: " : "",
			GNOME_VFS_FILE_INFO_SYMLINK (info) ? info->symlink_name
			: "",
			GNOME_VFS_FILE_INFO_SYMLINK (info) ? " ]" : "",
			type_to_string (info->type),
			info->mime_type,
			(glong) info->size,
			info->permissions);

	g_free (path);
}

void
list (void)
{
	GnomeVFSResult result;
	GnomeVFSFileInfo *info;
	GnomeVFSDirectoryHandle *handle;

	result = gnome_vfs_directory_open (&handle, directory,
			GNOME_VFS_FILE_INFO_GET_MIME_TYPE
			| GNOME_VFS_FILE_INFO_FOLLOW_LINKS);

	if (result != GNOME_VFS_OK) {
		g_print("Error opening: %s\n", gnome_vfs_result_to_string
				(result));
		return;
	}

	info = gnome_vfs_file_info_new ();
	while ((result = gnome_vfs_directory_read_next (handle, info)) == GNOME_VFS_OK) {
		show_data ((gpointer) info, NULL);
	}

	gnome_vfs_file_info_unref (info);

	if ((result != GNOME_VFS_OK) && (result != GNOME_VFS_ERROR_EOF)) {
		g_print ("Error: %s\n", gnome_vfs_result_to_string (result));
		return;
	}

	result = gnome_vfs_directory_close (handle);

	if ((result != GNOME_VFS_OK) && (result != GNOME_VFS_ERROR_EOF)) {
		g_print ("Error: %s\n", gnome_vfs_result_to_string (result));
		return;
	}

}

int
main (int argc, char *argv[])
{
  
  	setlocale (LC_ALL, "");

	gnome_vfs_init ();

	if (argc > 1) {
		directory = gnome_vfs_make_uri_from_shell_arg (argv[1]);
	} else {
		char *tmp;

		tmp = g_get_current_dir ();
		directory = gnome_vfs_escape_path_string (tmp);
		g_free (tmp);
	}

	list ();
	
	g_free (directory);
	
	gnome_vfs_shutdown ();
	return 0;
}
