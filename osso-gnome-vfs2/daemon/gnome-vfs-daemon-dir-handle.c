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
#include "gnome-vfs-daemon-dir-handle.h"
#include "gnome-vfs-cancellable-ops.h"
#include "gnome-vfs-daemon.h"
#include "gnome-vfs-daemon-method.h"
#include "gnome-vfs-async-daemon.h"

#define READDIR_CHUNK_SIZE 50

BONOBO_CLASS_BOILERPLATE_FULL(
	GnomeVFSDaemonDirHandle,
	gnome_vfs_daemon_dir_handle,
	GNOME_VFS_DaemonDirHandle,
	BonoboObject,
	BONOBO_TYPE_OBJECT);


static void
gnome_vfs_daemon_dir_handle_finalize (GObject *object)
{
	GnomeVFSDaemonDirHandle *handle;

	handle = GNOME_VFS_DAEMON_DIR_HANDLE (object);
	
        if (handle->real_handle != NULL) {
		gnome_vfs_directory_close (handle->real_handle);
	}
	g_mutex_free (handle->mutex);
	BONOBO_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
gnome_vfs_daemon_dir_handle_instance_init (GnomeVFSDaemonDirHandle *handle)
{
	handle->mutex = g_mutex_new ();
}

static GNOME_VFS_Result
gnome_vfs_daemon_dir_handle_read (PortableServer_Servant _servant,
				  GNOME_VFS_FileInfoList ** file_info_list,
				  const GNOME_VFS_ClientCall client_call,
				  const GNOME_VFS_Client client,
				  CORBA_Environment * ev)
{
	GnomeVFSDaemonDirHandle *handle;
	CORBA_sequence_GNOME_VFS_FileInfo *list;
	GnomeVFSFileInfo *file_info;
	GnomeVFSResult res;

	handle = GNOME_VFS_DAEMON_DIR_HANDLE (bonobo_object_from_servant (_servant));
	
	list = CORBA_sequence_GNOME_VFS_FileInfo__alloc ();
	list->_buffer = CORBA_sequence_GNOME_VFS_FileInfo_allocbuf (READDIR_CHUNK_SIZE);
	list->_release = CORBA_TRUE;
	list->_length = 0;
	list->_maximum = READDIR_CHUNK_SIZE;
        CORBA_sequence_set_release (list, CORBA_TRUE);

	file_info = gnome_vfs_file_info_new ();

	res = 0;
	while (list->_length < READDIR_CHUNK_SIZE &&
	       (res = gnome_vfs_directory_read_next (handle->real_handle, file_info)) == GNOME_VFS_OK) {
		gnome_vfs_daemon_convert_to_corba_file_info (file_info, &list->_buffer[list->_length]);
		list->_length++;
		gnome_vfs_file_info_clear (file_info);
	}

	gnome_vfs_file_info_unref (file_info);
	
	*file_info_list = list;
	
	if (list->_length > 0) {
		return GNOME_VFS_OK;
	} else {
		return res;
	}
	
}

static GNOME_VFS_Result
gnome_vfs_daemon_dir_handle_close (PortableServer_Servant _servant,
				   const GNOME_VFS_ClientCall client_call,
				   const GNOME_VFS_Client client,
				   CORBA_Environment * ev)
{
	GnomeVFSDaemonDirHandle *handle;
	GnomeVFSResult res;

	handle = GNOME_VFS_DAEMON_DIR_HANDLE (bonobo_object_from_servant (_servant));

	res = gnome_vfs_directory_close (handle->real_handle);
	
	if (res == GNOME_VFS_OK) {
		handle->real_handle = NULL;
		/* The client is now finished with the handle,
		   remove it from the list and free it */
		gnome_vfs_daemon_remove_client_dir_handle (client,
							   handle);
		bonobo_object_unref (handle);
	}
	
	return res;
}

static void
gnome_vfs_daemon_dir_handle_class_init (GnomeVFSDaemonDirHandleClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_GNOME_VFS_DaemonDirHandle__epv *epv = &klass->epv;

	epv->Read = gnome_vfs_daemon_dir_handle_read;
	epv->Close = gnome_vfs_daemon_dir_handle_close;
	
	object_class->finalize = gnome_vfs_daemon_dir_handle_finalize;
}

GnomeVFSDaemonDirHandle *
gnome_vfs_daemon_dir_handle_new (GnomeVFSDirectoryHandle *real_handle)
{
	GnomeVFSDaemonDirHandle *daemon_dir_handle;
        PortableServer_POA poa;

	poa = bonobo_poa_get_threaded (ORBIT_THREAD_HINT_PER_REQUEST);
	daemon_dir_handle = g_object_new (GNOME_TYPE_VFS_DAEMON_DIR_HANDLE,
				      "poa", poa,
				      NULL);
	CORBA_Object_release ((CORBA_Object)poa, NULL);
	daemon_dir_handle->real_handle = real_handle;

	return daemon_dir_handle;
}
