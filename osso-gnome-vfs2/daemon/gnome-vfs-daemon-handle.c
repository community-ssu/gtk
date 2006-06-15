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
#include "gnome-vfs-daemon-handle.h"
#include "gnome-vfs-cancellable-ops.h"
#include "gnome-vfs-daemon.h"
#include "gnome-vfs-async-daemon.h"
#include "gnome-vfs-daemon-method.h"

BONOBO_CLASS_BOILERPLATE_FULL(
	GnomeVFSDaemonHandle,
	gnome_vfs_daemon_handle,
	GNOME_VFS_DaemonHandle,
	BonoboObject,
	BONOBO_TYPE_OBJECT);


static void
gnome_vfs_daemon_handle_finalize (GObject *object)
{
	GnomeVFSDaemonHandle *handle;

	handle = GNOME_VFS_DAEMON_HANDLE (object);
	
        if (handle->real_handle != NULL) {
		gnome_vfs_close_cancellable (handle->real_handle, NULL);
	}
	g_mutex_free (handle->mutex);
	BONOBO_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
gnome_vfs_daemon_handle_instance_init (GnomeVFSDaemonHandle *handle)
{
	handle->mutex = g_mutex_new ();
}

static GNOME_VFS_Result
gnome_vfs_daemon_handle_read (PortableServer_Servant _servant,
			      GNOME_VFS_buffer ** _buf,
			      const GNOME_VFS_FileSize num_bytes,
			      const GNOME_VFS_ClientCall client_call,
			      const GNOME_VFS_Client client,
			      CORBA_Environment * ev)
{
	GnomeVFSResult res;
	GnomeVFSDaemonHandle *handle;
	GnomeVFSFileSize bytes_written;
	GNOME_VFS_buffer * buf;
	GnomeVFSContext *context;

	buf = CORBA_sequence_CORBA_octet__alloc ();
	*_buf = buf;
	
	buf->_buffer = CORBA_sequence_CORBA_octet_allocbuf (num_bytes);
	buf->_release = CORBA_TRUE;
	buf->_length = 0;
	buf->_maximum = num_bytes;
	
	handle = GNOME_VFS_DAEMON_HANDLE (bonobo_object_from_servant (_servant));

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_read_cancellable (handle->real_handle,
					  buf->_buffer,
					  num_bytes,
					  &bytes_written,
					  context);
	
	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	buf->_length = bytes_written;

	return res;
}


static GNOME_VFS_Result
gnome_vfs_daemon_handle_close (PortableServer_Servant _servant,
			       const GNOME_VFS_ClientCall client_call,
			       const GNOME_VFS_Client client,
			       CORBA_Environment * ev)
{
	GnomeVFSDaemonHandle *handle;
	GnomeVFSResult res;
	GnomeVFSContext *context;

	handle = GNOME_VFS_DAEMON_HANDLE (bonobo_object_from_servant (_servant));

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_close_cancellable (handle->real_handle,
					   context);
	
	gnome_vfs_async_daemon_drop_context (client_call, client, context);
	
	if (res == GNOME_VFS_OK) {
		handle->real_handle = NULL;
		
		/* The client is now finished with the handle,
		   remove it from the list and free it */
		gnome_vfs_daemon_remove_client_handle (client,
						       handle);
		bonobo_object_unref (handle);
	}
	
	return res;
}


static GNOME_VFS_Result
gnome_vfs_daemon_handle_write (PortableServer_Servant _servant,
			       const GNOME_VFS_buffer *buf,
			       GNOME_VFS_FileSize * bytes_written_return,
			       const GNOME_VFS_ClientCall client_call,
			       const GNOME_VFS_Client client,
			       CORBA_Environment *ev)
{
	GnomeVFSResult res;
	GnomeVFSDaemonHandle *handle;
	GnomeVFSContext *context;
	GnomeVFSFileSize bytes_written;
	
	handle = GNOME_VFS_DAEMON_HANDLE (bonobo_object_from_servant (_servant));

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_write_cancellable (handle->real_handle,
					   buf->_buffer,
					   buf->_length,
					   &bytes_written,
					   context);
	*bytes_written_return = bytes_written;
	
	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	return res;
}

static GNOME_VFS_Result
gnome_vfs_daemon_handle_seek (PortableServer_Servant _servant,
			      const CORBA_long whence,
			      const GNOME_VFS_FileOffset offset,
			      const GNOME_VFS_ClientCall client_call,
			      const GNOME_VFS_Client client,
			      CORBA_Environment *ev)
{
	GnomeVFSResult res;
	GnomeVFSDaemonHandle *handle;
	GnomeVFSContext *context;
	
	handle = GNOME_VFS_DAEMON_HANDLE (bonobo_object_from_servant (_servant));

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_seek_cancellable (handle->real_handle,
					  whence, offset,
					  context);

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	return res;
}

static GNOME_VFS_Result
gnome_vfs_daemon_handle_tell (PortableServer_Servant _servant,
			      GNOME_VFS_FileOffset *offset_return,
			      const GNOME_VFS_ClientCall client_call,
			      const GNOME_VFS_Client client,
			      CORBA_Environment *ev)
{
	GnomeVFSResult res;
	GnomeVFSDaemonHandle *handle;
	GnomeVFSContext *context;
	GnomeVFSFileSize offset;
	
	handle = GNOME_VFS_DAEMON_HANDLE (bonobo_object_from_servant (_servant));

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_tell (handle->real_handle,
			      &offset);
	*offset_return = offset;

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	return res;
}

static GNOME_VFS_Result
gnome_vfs_daemon_handle_get_file_info (PortableServer_Servant _servant,
				       GNOME_VFS_FileInfo **corba_info,
				       const CORBA_long options,
				       const GNOME_VFS_ClientCall client_call,
				       const GNOME_VFS_Client client,
				       CORBA_Environment *ev)
{
	GnomeVFSResult res;
	GnomeVFSDaemonHandle *handle;
	GnomeVFSContext *context;
	GnomeVFSFileInfo *file_info;
	
	*corba_info = NULL;
	
	handle = GNOME_VFS_DAEMON_HANDLE (bonobo_object_from_servant (_servant));

	context = gnome_vfs_async_daemon_get_context (client_call, client);

	file_info = gnome_vfs_file_info_new ();
	res = gnome_vfs_get_file_info_from_handle_cancellable (handle->real_handle,
							       file_info, options,
							       context);

	
	if (res == GNOME_VFS_OK) {
		*corba_info = GNOME_VFS_FileInfo__alloc ();
		gnome_vfs_daemon_convert_to_corba_file_info (file_info, *corba_info);
	}
	
	gnome_vfs_async_daemon_drop_context (client_call, client, context);
	
	gnome_vfs_file_info_unref (file_info);

	return res;
}

static GNOME_VFS_Result
gnome_vfs_daemon_handle_truncate (PortableServer_Servant _servant,
				  const GNOME_VFS_FileSize length,
				  const GNOME_VFS_ClientCall client_call,
				  const GNOME_VFS_Client client,
				  CORBA_Environment *ev)
{
	GnomeVFSResult res;
	GnomeVFSDaemonHandle *handle;
	GnomeVFSContext *context;
	
	handle = GNOME_VFS_DAEMON_HANDLE (bonobo_object_from_servant (_servant));

	context = gnome_vfs_async_daemon_get_context (client_call, client);
	
	res = gnome_vfs_truncate_handle_cancellable (handle->real_handle, length,
						     context);

	gnome_vfs_async_daemon_drop_context (client_call, client, context);

	return res;
}


static void
gnome_vfs_daemon_handle_class_init (GnomeVFSDaemonHandleClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_GNOME_VFS_DaemonHandle__epv *epv = &klass->epv;

	epv->Read = gnome_vfs_daemon_handle_read;
	epv->Close = gnome_vfs_daemon_handle_close;
	epv->Write = gnome_vfs_daemon_handle_write;
	epv->Seek = gnome_vfs_daemon_handle_seek;
	epv->Tell = gnome_vfs_daemon_handle_tell;
	epv->GetFileInfo = gnome_vfs_daemon_handle_get_file_info;
	epv->Truncate = gnome_vfs_daemon_handle_truncate;
	
	object_class->finalize = gnome_vfs_daemon_handle_finalize;
}

GnomeVFSDaemonHandle *
gnome_vfs_daemon_handle_new (GnomeVFSHandle *real_handle)
{
	GnomeVFSDaemonHandle *daemon_handle;
        PortableServer_POA poa;

	poa = bonobo_poa_get_threaded (ORBIT_THREAD_HINT_PER_REQUEST);
	daemon_handle = g_object_new (GNOME_TYPE_VFS_DAEMON_HANDLE,
				      "poa", poa,
				      NULL);
	CORBA_Object_release ((CORBA_Object)poa, NULL);
	daemon_handle->real_handle = real_handle;

	return daemon_handle;
}
