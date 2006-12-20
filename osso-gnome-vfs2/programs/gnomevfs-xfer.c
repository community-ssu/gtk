/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnomevfs-xfer.c - Test for GnomeVFS Xfer API

   Copyright (C) 2006 Free Software Foundation, Inc.

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

   Author: Christian Neumair <chris@gnome-de.org>
*/
#include <libgnomevfs/gnome-vfs.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

#include "authentication.c"

/* wrapper around GnomeVFSXferOptions */
static gboolean follow_links = 0;
static gboolean recursive = 0;
static gboolean same_fs = 0;
static gboolean delete_items = 0;
static gboolean empty_directories = 0;
static gboolean new_unique_directory = 0;
static gboolean remove_source = 0;
static gboolean use_unique_names = 0;
static gboolean link_items = 0;
static gboolean follow_links_recursive = 0;
static gboolean target_default_perms = 0;

static char **uris = NULL;

static GMainLoop *main_loop = NULL;

static GOptionEntry option_entries[] = 
{
  { "follow-links", 0, 0, G_OPTION_ARG_NONE, &follow_links, "Set GNOME_VFS_XFER_FOLLOW_LINKS", NULL },
  { "recursive", 0, 0, G_OPTION_ARG_NONE, &recursive, "Set GNOME_VFS_XFER_RECURSIVE", NULL },
  { "same-fs", 0, 0, G_OPTION_ARG_NONE, &same_fs, "Set GNOME_VFS_XFER_SAMEFS", NULL },
  { "delete-items", 0, 0, G_OPTION_ARG_NONE, &delete_items, "Set GNOME_VFS_XFER_DELETE_ITEMS", NULL },
  { "empty-directories", 0, 0, G_OPTION_ARG_NONE, &empty_directories, "Set GNOME_VFS_XFER_EMPTY_DIRECTORIES", NULL },
  { "new-unique-directory", 0, 0, G_OPTION_ARG_NONE, &new_unique_directory, "Set GNOME_VFS_XFER_NEW_UNIQUE_DIRECTORY", NULL },
  { "remove-source", 0, 0, G_OPTION_ARG_NONE, &remove_source, "Set GNOME_VFS_XFER_REMOVE_SOURCE", NULL },
  { "use-unique-names", 0, 0, G_OPTION_ARG_NONE, &use_unique_names, "Set GNOME_VFS_XFER_USE_UNIQUE_NAMES", NULL },
  { "link-items", 0, 0, G_OPTION_ARG_NONE, &link_items, "Set GNOME_VFS_XFER_LINK_ITEMS", NULL },
  { "follow-links-recursive", 0, 0, G_OPTION_ARG_NONE, &follow_links_recursive, "Set GNOME_VFS_XFER_FOLLOW_LINKS_RECURSIVE", NULL },
  { "target-default-perms", 0, 0, G_OPTION_ARG_NONE, &target_default_perms, "Set GNOME_VFS_XFER_TARGET_DEFAULT_PERMS", NULL },

  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &uris, "The xfer URIs", NULL },
  { NULL }
};

static void
show_result (GnomeVFSResult result, const gchar *what, const gchar *from, const gchar *to)
{
	if (result != GNOME_VFS_OK) {
		fprintf (stderr, "%s `%s' `%s': %s\n",
				what, from, to,
				gnome_vfs_result_to_string (result));
		exit (1);
	}
}

static char *
status_to_str (GnomeVFSXferProgressStatus status)
{
	switch (status) {
		case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
			return "OK";
		case GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR:
			return "VFS Error";
		case GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE:
			return "Overwrite";
		case GNOME_VFS_XFER_PROGRESS_STATUS_DUPLICATE:
			return "Duplicate";
		default:
			g_assert_not_reached ();
			return NULL;
	}
}

static char *
phase_to_str (GnomeVFSXferPhase phase)
{
	switch (phase) {
		case GNOME_VFS_XFER_PHASE_INITIAL:
			return "Initial";
		case GNOME_VFS_XFER_CHECKING_DESTINATION:
			return "Checking Destination";
		case GNOME_VFS_XFER_PHASE_COLLECTING:
			return "Collecting";
		case GNOME_VFS_XFER_PHASE_READYTOGO:
			return "Ready to Go";
		case GNOME_VFS_XFER_PHASE_OPENSOURCE:
			return "Open Source";
		case GNOME_VFS_XFER_PHASE_OPENTARGET:
			return "Open Target";
		case GNOME_VFS_XFER_PHASE_COPYING:
			return "Copying";
		case GNOME_VFS_XFER_PHASE_MOVING:
			return "Moving";
		case GNOME_VFS_XFER_PHASE_READSOURCE:
			return "Read Source";
		case GNOME_VFS_XFER_PHASE_WRITETARGET:
			return "Write Target";
		case GNOME_VFS_XFER_PHASE_CLOSESOURCE:
			return "Close Source";
		case GNOME_VFS_XFER_PHASE_CLOSETARGET:
			return "Close Target";
		case GNOME_VFS_XFER_PHASE_DELETESOURCE:
			return "Delete Source";
		case GNOME_VFS_XFER_PHASE_SETATTRIBUTES:
			return "Set Attributes";
		case GNOME_VFS_XFER_PHASE_FILECOMPLETED:
			return "File Completed";
		case GNOME_VFS_XFER_PHASE_CLEANUP:
			return "Cleanup";
		case GNOME_VFS_XFER_PHASE_COMPLETED:
			return "Completed";
		default:
			g_assert_not_reached ();
			return NULL;
	}
}

static void
debug_info (GnomeVFSXferProgressInfo *info)
{
	fprintf (stdout, "status: %s\n", status_to_str (info->status));
	fprintf (stdout, "VFS status: %s\n", gnome_vfs_result_to_string (info->vfs_status));
	fprintf (stdout, "phase: %s\n", phase_to_str (info->phase));
	fprintf (stdout, "source: %s\n", info->source_name);
	fprintf (stdout, "target: %s\n", info->target_name);
	fprintf (stdout, "index: %ld/%ld\n", info->file_index, info->files_total);
	fprintf (stdout, "copied (file): %u/%u\n", (guint) info->bytes_copied, (guint) info->file_size);
	fprintf (stdout, "copied (total): %u/%u\n", (guint) info->total_bytes_copied, (guint) info->bytes_total);
	fprintf (stdout, "duplicate name/count: %s/%u\n", info->duplicate_name, info->duplicate_count);
}

static int
progress_update_callback (GnomeVFSAsyncHandle *handle,
			  GnomeVFSXferProgressInfo *info,
			  gpointer user_data)
{
	int ret;
	fprintf (stdout, "+++ progress_update_callback enter\n");

	debug_info (info);

	if (info->phase == GNOME_VFS_XFER_PHASE_COMPLETED) {
		g_main_loop_quit (main_loop);
	}

	switch (info->status) {
		case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
			ret = TRUE;
			break;

		default:
			fprintf (stdout, "aborting due to %s status\n",
				 status_to_str (info->status));
			ret = FALSE;
	}

	fprintf (stdout, "--- progress_update_callback leave\n");

	return ret;
}

int
main (int argc,
      char **argv)
{
	GnomeVFSAsyncHandle *handle;
	GnomeVFSResult    result;
	char  	         *from, *to;
	GnomeVFSURI      *from_uri, *to_uri;
	GList *from_list, *to_list;
	GOptionContext *ctx;
	GError *error = NULL;
	GnomeVFSXferOptions options;

	ctx = g_option_context_new ("source_uri target_uri");

	g_option_context_add_main_entries (ctx, option_entries, NULL);

	if (!g_option_context_parse (ctx, &argc, &argv, &error)) {
		g_error_free (error);
		return 1;
	}

	if (!gnome_vfs_init ()) {
		fprintf (stderr, "Cannot initialize gnome-vfs.\n");
		return 1;
	}

	command_line_authentication_init ();

	if (uris == NULL || g_strv_length (uris) != 2) {
		fprintf (stderr, "You didn't pass two URIs.\n");
		return 1;
	}

	from = gnome_vfs_make_uri_from_shell_arg (uris[0]);

	if (from == NULL || (from_uri = gnome_vfs_uri_new (from)) == NULL) {
		g_free (from);
		fprintf (stderr, "Could not guess URI from %s\n", uris[0]);
		return 1;
	}

	to = gnome_vfs_make_uri_from_shell_arg (uris[1]);

	if (to == NULL || (to_uri = gnome_vfs_uri_new (to)) == NULL) {
		g_free (from);
		g_free (to);
		gnome_vfs_uri_unref (from_uri);
		fprintf (stderr, "Could not guess URI from %s\n", uris[1]);
		return 1;
	}

	from_list = g_list_append (NULL, from_uri);
	to_list = g_list_append (NULL, to_uri);

	options = GNOME_VFS_XFER_DEFAULT;

	if (follow_links) {
		options |= GNOME_VFS_XFER_FOLLOW_LINKS;
	}

	if (recursive) {
		options |= GNOME_VFS_XFER_RECURSIVE;
	}

	if (same_fs) {
		options |= GNOME_VFS_XFER_SAMEFS;
	}

	if (delete_items) {
		options |= GNOME_VFS_XFER_DELETE_ITEMS;
	}

	if (empty_directories) {
		options |= GNOME_VFS_XFER_EMPTY_DIRECTORIES;
	}

	if (new_unique_directory) {
		options |= GNOME_VFS_XFER_NEW_UNIQUE_DIRECTORY;
	}

	if (remove_source) {
		options |= GNOME_VFS_XFER_REMOVESOURCE;
	}

	if (use_unique_names) {
		options |= GNOME_VFS_XFER_USE_UNIQUE_NAMES;
	}

	if (link_items) {
		options |= GNOME_VFS_XFER_LINK_ITEMS;
	}

	if (follow_links_recursive) {
		options |= GNOME_VFS_XFER_FOLLOW_LINKS_RECURSIVE;
	}

	if (target_default_perms) {
		options |= GNOME_VFS_XFER_TARGET_DEFAULT_PERMS;
	}

	result = gnome_vfs_async_xfer (&handle, from_list, to_list, options,
				       GNOME_VFS_XFER_ERROR_MODE_QUERY,
				       GNOME_VFS_XFER_OVERWRITE_MODE_QUERY,
				       GNOME_VFS_PRIORITY_DEFAULT,
				       progress_update_callback, NULL,
				       NULL, NULL);

	show_result (result, "async xfer (initiated)", from, to);

	main_loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (main_loop);

	g_free (from);
	g_free (to);
	gnome_vfs_uri_unref (from_uri);
	gnome_vfs_uri_unref (to_uri);

	return 0;
}
