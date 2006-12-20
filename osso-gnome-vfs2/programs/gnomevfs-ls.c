/* gnomevfs-ls.c - Test for open_dir(), read_dir() and close_dir() for gnome-vfs

   Copyright (C) 2003, 2005, Red Hat

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <locale.h>
#include <libgnomevfs/gnome-vfs.h>

#include "authentication.c"

static gboolean timing = FALSE;
static gboolean quiet = FALSE;
static gboolean access_rights = FALSE;
static gboolean selinux_context = FALSE;

static GOptionEntry entries[] = 
{
	{ "time", 't', 0, G_OPTION_ARG_NONE, &timing, "Time the directory listening operation", NULL },
	{ "quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet, "Do not output the stat information (useful in conjunction with the --time)", NULL},
	{ "access-rights", 'a', 0, G_OPTION_ARG_NONE, &access_rights, "Get access rights", NULL},

#ifdef HAVE_SELINUX
	{ "selinux-context", 'Z', 0, G_OPTION_ARG_NONE, &selinux_context, "Get selinux context", NULL},
#endif

	{ NULL }
};

static void show_data (gpointer item, const char *directory);
static void list (const char *directory);

static const gchar *
type_to_string (GnomeVFSFileInfo *info)
{
	if (!(info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE)) {
		return "Unknown";
	}

	switch (info->type) {
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
show_data (gpointer item, const char *directory)
{
	GnomeVFSFileInfo *info;
	char *path;

	info = (GnomeVFSFileInfo *) item;

	path = g_strconcat (directory, "/", info->name, NULL);

	g_print ("%s\t%s%s%s\t(%s, %s)\tsize %" GNOME_VFS_SIZE_FORMAT_STR "",
			info->name,
			GNOME_VFS_FILE_INFO_SYMLINK (info) ? " [link: " : "",
			GNOME_VFS_FILE_INFO_SYMLINK (info) ? info->symlink_name
			: "",
			GNOME_VFS_FILE_INFO_SYMLINK (info) ? " ]" : "",
			type_to_string (info),
			info->mime_type,
			info->size);

	if (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS) {
			g_print ("\tmode %04o", info->permissions);
	}

	if (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_ACCESS) {
		g_print ("\t");
		
		if (info->permissions & GNOME_VFS_PERM_ACCESS_READABLE) {
			g_print ("r");
		}
		
		if (info->permissions & GNOME_VFS_PERM_ACCESS_WRITABLE) {
			g_print ("w");
		}
		
		if (info->permissions & GNOME_VFS_PERM_ACCESS_EXECUTABLE) {
			g_print ("x");
		}
	}

	if (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_SELINUX_CONTEXT)
		g_print ("\tcontext %s", info->selinux_context);

	g_print ("\n");
	
	g_free (path);
}

void
list (const char *directory)
{
	GnomeVFSResult result;
	GnomeVFSFileInfo *info;
	GnomeVFSDirectoryHandle *handle;
	GnomeVFSFileInfoOptions options;
	GTimer *timer;
	
	timer = NULL;
	if (timing) {
		timer = g_timer_new ();
		g_timer_start (timer);	
	}
	
	options = GNOME_VFS_FILE_INFO_GET_MIME_TYPE
			| GNOME_VFS_FILE_INFO_FOLLOW_LINKS;

	if (access_rights) {
		options |= GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS;
	}

	if (selinux_context)
		options |= GNOME_VFS_FILE_INFO_GET_SELINUX_CONTEXT;
	
	result = gnome_vfs_directory_open (&handle, directory, options);

	if (result != GNOME_VFS_OK) {
		g_print("Error opening: %s\n", gnome_vfs_result_to_string
				(result));
		return;
	}

	info = gnome_vfs_file_info_new ();
	while ((result = gnome_vfs_directory_read_next (handle, info)) == GNOME_VFS_OK) {
		if (!quiet) {
			show_data ((gpointer) info, directory);
		}
	}
	
	gnome_vfs_file_info_unref (info);

	if ((result != GNOME_VFS_OK) && (result != GNOME_VFS_ERROR_EOF)) {
		g_print ("Error: %s\n", gnome_vfs_result_to_string (result));
		return;
	}

	result = gnome_vfs_directory_close (handle);

	if ((result != GNOME_VFS_OK) && (result != GNOME_VFS_ERROR_EOF)) {
		g_print ("Error closing: %s\n", gnome_vfs_result_to_string (result));
		return;
	}

	if (timing) {
		gdouble duration;
		gulong  msecs;
		
		g_timer_stop (timer);
		
		duration = g_timer_elapsed (timer, &msecs);
		g_timer_destroy (timer);
		g_print ("Total time: %.16e\n", duration);
	}

	
}

int
main (int argc, char *argv[])
{
  	GError *error;
	GOptionContext *context;

  	setlocale (LC_ALL, "");

	gnome_vfs_init ();

	command_line_authentication_init ();

	error = NULL;
	context = g_option_context_new ("- list files at <uri>");
  	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  	g_option_context_parse (context, &argc, &argv, &error);

	if (argc > 1) {
		int i;

		for (i = 1; i < argc; i++) {
			char *directory;
			directory = gnome_vfs_make_uri_from_shell_arg (argv[i]);
			list (directory);
			g_free (directory);
		}
	} else {
		char *tmp, *directory;

		tmp = g_get_current_dir ();
		directory = gnome_vfs_get_uri_from_local_path (tmp);
		list (directory);
		g_free (tmp);
		g_free (directory);
	}

	gnome_vfs_shutdown ();
	return 0;
}
