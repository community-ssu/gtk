/*
 *  Copyright (C) 2003, 2004 Red Hat, Inc.
 *
 *  Nautilus is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Nautilus is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Alexander Larsson <alexl@redhat.com>
 *
 */
#include <config.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-generic-factory.h>
#include <libgnomevfs/gnome-vfs.h>
#include "gnome-vfs-async-daemon.h"
#include "gnome-vfs-cancellable-ops.h"
#include "gnome-vfs-daemon-handle.h"
#include "gnome-vfs-daemon-dir-handle.h"
#include "gnome-vfs-client-call.h"
#include "gnome-vfs-daemon.h"
#include "gnome-vfs-daemon-method.h"
#include <unistd.h>

BONOBO_CLASS_BOILERPLATE_FULL(
	GnomeVFSAsyncDaemon,
	gnome_vfs_async_daemon,
	GNOME_VFS_AsyncDaemon,
	BonoboObject,
	BONOBO_TYPE_OBJECT);


/* Protects the client_call_context hashtable, and the existance of
 *  the context object that has been looked up */
G_LOCK_DEFINE_STATIC (client_call_context);

static GnomeVFSAsyncDaemon *g_vfs_async_daemon = NULL;

static void
gnome_vfs_async_daemon_finalize (GObject *object)
{
	/* All client calls should have finished before we kill this object */
	g_assert (g_hash_table_size (g_vfs_async_daemon->client_call_context) == 0);
	g_hash_table_destroy (g_vfs_async_daemon->client_call_context);
	BONOBO_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
	g_vfs_async_daemon = NULL;
}

static void
gnome_vfs_async_daemon_instance_init (GnomeVFSAsyncDaemon *daemon)
{
	daemon->client_call_context = g_hash_table_new (NULL, NULL);
	g_vfs_async_daemon = daemon;
}

GnomeVFSContext *
gnome_vfs_async_daemon_get_context (const GNOME_VFS_ClientCall client_call,
				    const GNOME_VFS_Client client)
{
	GnomeVFSContext *context;

	if (g_vfs_async_daemon == NULL) {
		return NULL;
	}
	
	context = gnome_vfs_context_new ();
	G_LOCK (client_call_context);
	g_hash_table_insert (g_vfs_async_daemon->client_call_context, client_call, context);
	G_UNLOCK (client_call_context);

	gnome_vfs_daemon_add_context (client, context);
	gnome_vfs_daemon_set_current_daemon_client_call (client_call);

	return context;
}

void
gnome_vfs_async_daemon_drop_context (const GNOME_VFS_ClientCall client_call,
				     const GNOME_VFS_Client client,
				     GnomeVFSContext *context)
{
	if (context != NULL) {
		gnome_vfs_daemon_set_current_daemon_client_call (NULL);
		gnome_vfs_daemon_remove_context (client, context);
		G_LOCK (client_call_context);
		if (g_vfs_async_daemon != NULL) {
			g_hash_table_remove (g_vfs_async_daemon->client_call_context, client_call);
		}
		gnome_vfs_context_free (context);
		G_UNLOCK (client_call_context);
	}
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_open (PortableServer_Servant _servant,
			     GNOME_VFS_DaemonHandle * handle_return,
			     const CORBA_char * uri_str,
			     const CORBA_long open_mode,
			     const GNOME_VFS_ClientCall client_call,
			     const GNOME_VFS_Client client,
			     CORBA_Environment * ev)
{
	GnomeVFSURI *uri;
	GnomeVFSHandle *vfs_handle;
	GnomeVFSResult res;
	GnomeVFSContext *context;
	GnomeVFSDaemonHandle *handle;
	
	*handle_return = NULL;

	uri = gnome_vfs_uri_new (uri_str);
	if (uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);

	res = gnome_vfs_open_uri_cancellable (&vfs_handle,
					      uri, open_mode,
					      context);

	if (res == GNOME_VFS_OK) {
		handle = gnome_vfs_daemon_handle_new (vfs_handle);
		*handle_return = CORBA_Object_duplicate (BONOBO_OBJREF (handle), NULL);
		gnome_vfs_daemon_add_client_handle (client, handle);
	}
	
	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_uri_unref (uri);
	
	return res;
}


static GNOME_VFS_Result
gnome_vfs_async_daemon_open_dir (PortableServer_Servant _servant,
				 GNOME_VFS_DaemonDirHandle *
				 handle_return,
				 const CORBA_char * uri_str,
				 const CORBA_long options,
				 const GNOME_VFS_ClientCall
				 client_call,
				 const GNOME_VFS_Client client,
				 CORBA_Environment * ev)
{
	GnomeVFSURI *uri;
	GnomeVFSDirectoryHandle *vfs_handle;
	GnomeVFSResult res;
	GnomeVFSContext *context;
	GnomeVFSDaemonDirHandle *handle;
	
	*handle_return = NULL;

	uri = gnome_vfs_uri_new (uri_str);
	if (uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);

	res = gnome_vfs_directory_open_from_uri_cancellable (&vfs_handle,
							     uri, options,
							     context);

	if (res == GNOME_VFS_OK) {
		handle = gnome_vfs_daemon_dir_handle_new (vfs_handle);
		*handle_return = CORBA_Object_duplicate (BONOBO_OBJREF (handle), NULL);
		gnome_vfs_daemon_add_client_dir_handle (client, handle);
	}
	
	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_uri_unref (uri);
	
	return res;
}


static gboolean
cancel_client_call_callback (gpointer data)
{
	GnomeVFSContext *context;
	GnomeVFSCancellation *cancellation;
	GNOME_VFS_ClientCall client_call;

	client_call = data;

	G_LOCK (client_call_context);
	context = g_hash_table_lookup (g_vfs_async_daemon->client_call_context, client_call);
	if (context != NULL) {
		cancellation = gnome_vfs_context_get_cancellation (context);
		if (cancellation) {
			/* context + cancellation is guaranteed to live until
			 * the client_call exists, and it hasn't, since we
			 * looked up the context and haven't dropped the lock
			 * yet.
			 */
			gnome_vfs_cancellation_cancel (cancellation);
		}
	}
	G_UNLOCK (client_call_context);


	CORBA_Object_release (client_call, NULL);
	return FALSE;
}

static void
gnome_vfs_async_daemon_cancel (PortableServer_Servant _servant,
			       const GNOME_VFS_ClientCall client_call,
			       CORBA_Environment *ev)
{
	/* Ref the client_call so it won't go away if the call finishes while
	 * waiting for the idle */
	CORBA_Object_duplicate (client_call, NULL);

	g_idle_add (cancel_client_call_callback, client_call);
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_create (PortableServer_Servant _servant,
			       GNOME_VFS_DaemonHandle *handle_return,
			       const CORBA_char *uri_str,
			       const CORBA_long open_mode,
			       const CORBA_boolean exclusive,
			       const CORBA_long perm,
			       const GNOME_VFS_ClientCall client_call,
			       const GNOME_VFS_Client client,
			       CORBA_Environment *ev)
{
	GnomeVFSURI *uri;
	GnomeVFSHandle *vfs_handle;
	GnomeVFSResult res;
	GnomeVFSContext *context;
	GnomeVFSDaemonHandle *handle;
	
	*handle_return = NULL;

	uri = gnome_vfs_uri_new (uri_str);
	if (uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);

	res = gnome_vfs_create_uri_cancellable (&vfs_handle,
						uri, open_mode,
						exclusive, perm,
						context);

	if (res == GNOME_VFS_OK) {
		handle = gnome_vfs_daemon_handle_new (vfs_handle);
		*handle_return = CORBA_Object_duplicate (BONOBO_OBJREF (handle), NULL);
		gnome_vfs_daemon_add_client_handle (client, handle);
	}
	
	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_uri_unref (uri);
	
	return res;
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_get_file_info (PortableServer_Servant _servant,
				      const CORBA_char *uri_str,
				      GNOME_VFS_FileInfo **corba_info,
				      const CORBA_long options,
				      const GNOME_VFS_ClientCall client_call,
				      const GNOME_VFS_Client client,
				      CORBA_Environment *ev)
{
	GnomeVFSURI *uri;
	GnomeVFSResult res;
	GnomeVFSContext *context;
	GnomeVFSFileInfo *file_info;

	*corba_info = GNOME_VFS_FileInfo__alloc ();
	
	file_info = gnome_vfs_file_info_new ();

	uri = gnome_vfs_uri_new (uri_str);
	if (uri == NULL) {
		gnome_vfs_daemon_convert_to_corba_file_info (file_info, *corba_info);
		gnome_vfs_file_info_unref (file_info);
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_get_file_info_uri_cancellable (uri, file_info,
						   options, context);

	gnome_vfs_daemon_convert_to_corba_file_info (file_info, *corba_info);
	
	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_file_info_unref (file_info);
	gnome_vfs_uri_unref (uri);
	
	return res;
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_truncate (PortableServer_Servant _servant,
				 const CORBA_char *uri_str,
				 const GNOME_VFS_FileSize length,
				 const GNOME_VFS_ClientCall client_call,
				 const GNOME_VFS_Client client,
				 CORBA_Environment *ev)
{
	GnomeVFSURI *uri;
	GnomeVFSResult res;
	GnomeVFSContext *context;

	uri = gnome_vfs_uri_new (uri_str);
	if (uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_truncate_uri_cancellable (uri, length, context);

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_uri_unref (uri);
	
	return res;
}

static CORBA_boolean
gnome_vfs_async_daemon_is_local (PortableServer_Servant _servant,
				 const CORBA_char *uri_str,
				 const GNOME_VFS_ClientCall client_call,
				 const GNOME_VFS_Client client,
				 CORBA_Environment *ev)
{
	GnomeVFSURI *uri;
	gboolean res;
	GnomeVFSContext *context;

	uri = gnome_vfs_uri_new (uri_str);
	if (uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_uri_is_local (uri);

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_uri_unref (uri);
	
	return res;
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_find_directory (PortableServer_Servant _servant,
				       const CORBA_char *find_near_uri_str,
				       const CORBA_long kind,
				       CORBA_string *result_uri_str,
				       const CORBA_boolean create_if_needed,
				       const CORBA_boolean find_if_needed,
				       const CORBA_unsigned_long perm,
				       const GNOME_VFS_ClientCall client_call,
				       const GNOME_VFS_Client client,
				       CORBA_Environment *ev)
{
	GnomeVFSURI *find_near_uri;
	GnomeVFSURI *result_uri;
	GnomeVFSResult res;
	GnomeVFSContext *context;
	char *str;

	find_near_uri = gnome_vfs_uri_new (find_near_uri_str);
	if (find_near_uri == NULL) {
		*result_uri_str = CORBA_string_dup ("");
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res =  gnome_vfs_find_directory_cancellable (find_near_uri,
						     kind,
						     &result_uri,
						     create_if_needed,
		  				     find_if_needed,
						     perm,
						     context);

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	if (res == GNOME_VFS_OK && result_uri != NULL) {
		str = gnome_vfs_uri_to_string (result_uri, GNOME_VFS_URI_HIDE_NONE);
		*result_uri_str = CORBA_string_dup (str);
		g_free (str);
		gnome_vfs_uri_unref (result_uri);
	} else {
		*result_uri_str = CORBA_string_dup ("");
	}
	
	gnome_vfs_uri_unref (find_near_uri);
	
	return res;
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_make_directory (PortableServer_Servant _servant,
				       const CORBA_char *uri_str,
				       const CORBA_unsigned_long perm,
				       const GNOME_VFS_ClientCall client_call,
				       const GNOME_VFS_Client client,
				       CORBA_Environment *ev)
{
	GnomeVFSURI *uri;
	GnomeVFSResult res;
	GnomeVFSContext *context;

	uri = gnome_vfs_uri_new (uri_str);
	if (uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_make_directory_for_uri_cancellable (uri, perm, context);

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_uri_unref (uri);
	
	return res;
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_remove_directory (PortableServer_Servant _servant,
					 const CORBA_char *uri_str,
					 const GNOME_VFS_ClientCall client_call,
					 const GNOME_VFS_Client client,
					 CORBA_Environment *ev)
{
	GnomeVFSURI *uri;
	GnomeVFSResult res;
	GnomeVFSContext *context;

	uri = gnome_vfs_uri_new (uri_str);
	if (uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_remove_directory_from_uri_cancellable (uri, context);

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_uri_unref (uri);
	
	return res;
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_move (PortableServer_Servant _servant,
			     const CORBA_char *old_uri_str,
			     const CORBA_char *new_uri_str,
			     const CORBA_boolean force_replace,
			     const GNOME_VFS_ClientCall client_call,
			     const GNOME_VFS_Client client,
			     CORBA_Environment *ev)
{
	GnomeVFSURI *old_uri, *new_uri;
	GnomeVFSResult res;
	GnomeVFSContext *context;

	old_uri = gnome_vfs_uri_new (old_uri_str);
	if (old_uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}
	
	new_uri = gnome_vfs_uri_new (new_uri_str);
	if (new_uri == NULL) {
		gnome_vfs_uri_unref (old_uri);
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_move_uri_cancellable (old_uri, new_uri, force_replace, context);

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_uri_unref (old_uri);
	gnome_vfs_uri_unref (new_uri);
	
	return res;
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_unlink (PortableServer_Servant _servant,
			       const CORBA_char *uri_str,
			       const GNOME_VFS_ClientCall client_call,
			       const GNOME_VFS_Client client,
			       CORBA_Environment *ev)
{
	GnomeVFSURI *uri;
	GnomeVFSResult res;
	GnomeVFSContext *context;

	uri = gnome_vfs_uri_new (uri_str);
	if (uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_unlink_from_uri_cancellable (uri, context);

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_uri_unref (uri);
	
	return res;
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_check_same_fs (PortableServer_Servant _servant,
				      const CORBA_char *uri_a_str,
				      const CORBA_char *uri_b_str,
				      CORBA_boolean *same_fs_ret,
				      const GNOME_VFS_ClientCall client_call,
				      const GNOME_VFS_Client client,
				      CORBA_Environment *ev)
{
	GnomeVFSURI *uri_a, *uri_b;
	GnomeVFSResult res;
	GnomeVFSContext *context;
	gboolean same_fs;

	uri_a = gnome_vfs_uri_new (uri_a_str);
	if (uri_a == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}
	
	uri_b = gnome_vfs_uri_new (uri_b_str);
	if (uri_b == NULL) {
		gnome_vfs_uri_unref (uri_b);
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_check_same_fs_uris_cancellable (uri_a, uri_b, &same_fs, context);
	*same_fs_ret = same_fs;

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_uri_unref (uri_a);
	gnome_vfs_uri_unref (uri_b);
	
	return res;
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_set_file_info (PortableServer_Servant _servant,
				      const CORBA_char *uri_str,
				      const GNOME_VFS_FileInfo *corba_info,
				      const CORBA_long mask,
				      const GNOME_VFS_ClientCall client_call,
				      const GNOME_VFS_Client client,
				      CORBA_Environment *ev)
{
	GnomeVFSURI *uri;
	GnomeVFSResult res;
	GnomeVFSContext *context;
	GnomeVFSFileInfo *file_info;

	uri = gnome_vfs_uri_new (uri_str);
	if (uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	file_info = gnome_vfs_file_info_new ();
	
	gnome_vfs_daemon_convert_from_corba_file_info (corba_info, file_info);
	res = gnome_vfs_set_file_info_cancellable (uri, file_info, mask,
						   context);

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_file_info_unref (file_info);
	gnome_vfs_uri_unref (uri);
	
	return res;
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_create_symbolic_link (PortableServer_Servant _servant,
					     const CORBA_char *uri_str,
					     const CORBA_char *target_reference,
					     const GNOME_VFS_ClientCall client_call,
					     const GNOME_VFS_Client client,
					     CORBA_Environment *ev)
{
	GnomeVFSURI *uri;
	GnomeVFSResult res;
	GnomeVFSContext *context;

	uri = gnome_vfs_uri_new (uri_str);
	if (uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_create_symbolic_link_cancellable (uri, target_reference, context);

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	gnome_vfs_uri_unref (uri);
	
	return res;
}

static GNOME_VFS_Result
gnome_vfs_async_daemon_monitor_add (PortableServer_Servant _servant,
				    const CORBA_char *uri_str,
				    const CORBA_long monitor_type,
				    GNOME_VFS_DaemonMonitor *monitor,
				    const GNOME_VFS_ClientCall client_call,
				    const GNOME_VFS_Client client,
				    CORBA_Environment *ev)
{
	/* DAEMON-TODO: monitor support */
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
}


static void
gnome_vfs_async_daemon_class_init (GnomeVFSAsyncDaemonClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_GNOME_VFS_AsyncDaemon__epv *epv = &klass->epv;

	epv->Open = gnome_vfs_async_daemon_open;
	epv->Cancel = gnome_vfs_async_daemon_cancel;
	epv->OpenDirectory = gnome_vfs_async_daemon_open_dir;
	epv->Create = gnome_vfs_async_daemon_create;
	epv->GetFileInfo = gnome_vfs_async_daemon_get_file_info;
	epv->Truncate = gnome_vfs_async_daemon_truncate;
	epv->IsLocal = gnome_vfs_async_daemon_is_local;
	epv->FindDirectory = gnome_vfs_async_daemon_find_directory;
	epv->MakeDirectory = gnome_vfs_async_daemon_make_directory;
	epv->RemoveDirectory = gnome_vfs_async_daemon_remove_directory;
	epv->Move = gnome_vfs_async_daemon_move;
	epv->Unlink = gnome_vfs_async_daemon_unlink;
	epv->CheckSameFS = gnome_vfs_async_daemon_check_same_fs;
	epv->SetFileInfo = gnome_vfs_async_daemon_set_file_info;
	epv->CreateSymbolicLink = gnome_vfs_async_daemon_create_symbolic_link;
	epv->MonitorAdd = gnome_vfs_async_daemon_monitor_add;
	
	object_class->finalize = gnome_vfs_async_daemon_finalize;
}
