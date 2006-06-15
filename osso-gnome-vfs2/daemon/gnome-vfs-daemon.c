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
#include <locale.h>
#include <libbonobo.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-generic-factory.h>
#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf.h>
#include "gnome-vfs-daemon.h"
#include "gnome-vfs-async-daemon.h"
#include "gnome-vfs-private.h"
#include "gnome-vfs-volume-monitor.h"
#include "gnome-vfs-volume-monitor-private.h"
#include "gnome-vfs-volume-monitor-daemon.h"

#define QUIT_TIMEOUT (3*1000)

/* Global daemon */
static GnomeVFSDaemon *the_daemon = NULL;
static GnomeVFSAsyncDaemon *the_async_daemon = NULL;

typedef struct {
	GNOME_VFS_Client client;
	GList *outstanding_handles;
	GList *outstanding_dir_handles;
	GList *outstanding_contexts;
  /* DAEMON-TODO: outstanding_monitors */
} ClientInfo;

BONOBO_CLASS_BOILERPLATE_FULL(
	GnomeVFSDaemon,
	gnome_vfs_daemon,
	GNOME_VFS_Daemon,
	BonoboObject,
	BONOBO_TYPE_OBJECT);


static void disconnect_client_from_volume_monitor (const GNOME_VFS_Client client);

/* Protects the_daemon->clients and their contents */
G_LOCK_DEFINE_STATIC (client_list);


static gboolean
quit_timeout (gpointer data)
{
	if (the_daemon->clients == NULL) {
		g_print ("All clients dead, quitting ...\n");
		bonobo_main_quit ();
	}
	return FALSE;
}

static ClientInfo *
new_client_info (const GNOME_VFS_Client client)
{
	ClientInfo *client_info;
	
	client_info = g_new0 (ClientInfo, 1);
	client_info->client = CORBA_Object_duplicate (client, NULL);
	return client_info;
}


/* protected by client_lock */
static void
free_client_info (ClientInfo *client_info)
{
	GList *l;
	GnomeVFSDaemonHandle *handle;
	GnomeVFSDaemonDirHandle *dir_handle;
	GnomeVFSContext *context;
	GnomeVFSCancellation *cancellation;

	/* Cancel any outstanding operations for this client */
	for (l = client_info->outstanding_contexts; l != NULL; l = l->next) {
		context = l->data;
		cancellation = gnome_vfs_context_get_cancellation (context);
		if (cancellation) {
			gnome_vfs_cancellation_cancel (cancellation);
		}
	}

	/* Unref the handles outstanding for the client. If any
	 * operations methods are still running they are fine, because
	 * metods ref the object they correspond to while running.
	 */
	for (l = client_info->outstanding_handles; l != NULL; l = l->next) {
		handle = l->data;
		
		bonobo_object_unref (handle);
	}
	g_list_free (client_info->outstanding_handles);

	for (l = client_info->outstanding_dir_handles; l != NULL; l = l->next) {
		dir_handle = l->data;
		
		bonobo_object_unref (dir_handle);
	}
	g_list_free (client_info->outstanding_dir_handles);

	
	/* DAEMON-TODO: unref outstanding monitors (?) */
	
	CORBA_Object_release (client_info->client, NULL);
	g_free (client_info);
}

/* protected by client_list lock */
static ClientInfo *
lookup_client (GNOME_VFS_Client client)
{
	ClientInfo *client_info;
	GList *l;
	
	for (l = the_daemon->clients; l != NULL; l = l->next) {
		client_info = l->data;
		if (client_info->client == client)
			return client_info;
	}
	
	return NULL;
}


static void
remove_client (gpointer *cnx,
	       GNOME_VFS_Client client)
{
	ClientInfo *client_info;
	
	disconnect_client_from_volume_monitor (client);
	
	ORBit_small_unlisten_for_broken (client,
					 G_CALLBACK (remove_client));
	
	G_LOCK (client_list);
	client_info = lookup_client (client);
	if (client_info != NULL) {
		the_daemon->clients = g_list_remove (the_daemon->clients, client_info);
		free_client_info (client_info);
	}
	G_UNLOCK (client_list);

	if (the_daemon->clients == NULL) 
		g_timeout_add (QUIT_TIMEOUT, quit_timeout, NULL);
}

static void
de_register_client (PortableServer_Servant servant,
		    const GNOME_VFS_Client client,
		    CORBA_Environment     *ev)
{
	remove_client (NULL, client);
}

static void
register_client (PortableServer_Servant servant,
		 const GNOME_VFS_Client client,
		 CORBA_Environment     *ev)
{
	ORBitConnectionStatus status;
	ClientInfo *client_info;
	

	status = ORBit_small_listen_for_broken (client, 
						G_CALLBACK (remove_client),
						client);
	if (status != ORBIT_CONNECTION_CONNECTED) {
		g_warning ("client died already !");
		return;
	}

	client_info = new_client_info (client);
	
	G_LOCK (client_list);
	the_daemon->clients = g_list_prepend (the_daemon->clients,
					  client_info);
	G_UNLOCK (client_list);
}

static void
volume_mounted (GnomeVFSVolumeMonitor *volume_monitor,
		GnomeVFSVolume	       *volume,
		GNOME_VFS_Client        client)
{
	CORBA_Environment ev;
	GNOME_VFS_Volume *corba_volume;

	corba_volume = GNOME_VFS_Volume__alloc ();

	gnome_vfs_volume_to_corba (volume, corba_volume);
	
	CORBA_exception_init (&ev);
	GNOME_VFS_Client_VolumeMounted (client,
					corba_volume,
					&ev);

	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
	}
	CORBA_free (corba_volume);
}

static void
volume_pre_unmount (GnomeVFSVolumeMonitor *volume_monitor,
		    GnomeVFSVolume        *volume,
		    GNOME_VFS_Client       client)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	GNOME_VFS_Client_VolumePreUnmount (client,
					   volume->priv->id,
					   &ev);

	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
	}
}


static void
volume_unmounted (GnomeVFSVolumeMonitor *volume_monitor,
		  GnomeVFSVolume        *volume,
		  GNOME_VFS_Client       client)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	GNOME_VFS_Client_VolumeUnmounted (client,
					  volume->priv->id,
					  &ev);

	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
	}
}

static void
drive_connected (GnomeVFSVolumeMonitor *volume_monitor,
		 GnomeVFSDrive	       *drive,
		GNOME_VFS_Client        client)
{
	CORBA_Environment ev;
	GNOME_VFS_Drive *corba_drive;

	corba_drive = GNOME_VFS_Drive__alloc ();

	gnome_vfs_drive_to_corba (drive, corba_drive);
	
	CORBA_exception_init (&ev);
	GNOME_VFS_Client_DriveConnected (client,
					 corba_drive,
					 &ev);

	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
	}
	CORBA_free (corba_drive);
}

static void
drive_disconnected (GnomeVFSVolumeMonitor *volume_monitor,
		    GnomeVFSDrive	  *drive,
		    GNOME_VFS_Client       client)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	GNOME_VFS_Client_DriveDisconnected (client,
					    drive->priv->id,
					    &ev);

	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
	}
}

static void
register_volume_monitor (PortableServer_Servant _servant,
			 const GNOME_VFS_Client client,
			 CORBA_Environment * ev)
{
	GnomeVFSVolumeMonitor *monitor;

	monitor = gnome_vfs_get_volume_monitor ();

	g_signal_connect (monitor, "volume_mounted",
			  G_CALLBACK (volume_mounted), client);
	g_signal_connect (monitor, "volume_pre_unmount",
			  G_CALLBACK (volume_pre_unmount), client);
	g_signal_connect (monitor, "volume_unmounted",
			  G_CALLBACK (volume_unmounted), client);
	g_signal_connect (monitor, "drive_connected",
			  G_CALLBACK (drive_connected), client);
	g_signal_connect (monitor, "drive_disconnected",
			  G_CALLBACK (drive_disconnected), client);
	
	
}

static void
disconnect_client_from_volume_monitor (const GNOME_VFS_Client client)
{
	GnomeVFSVolumeMonitor *monitor;

	monitor = gnome_vfs_get_volume_monitor ();

	g_signal_handlers_disconnect_by_func (monitor, G_CALLBACK (volume_mounted), client);
	g_signal_handlers_disconnect_by_func (monitor, G_CALLBACK (volume_pre_unmount), client);
	g_signal_handlers_disconnect_by_func (monitor, G_CALLBACK (volume_unmounted), client);
	g_signal_handlers_disconnect_by_func (monitor, G_CALLBACK (drive_connected), client);
	g_signal_handlers_disconnect_by_func (monitor, G_CALLBACK (drive_disconnected), client);
}

static void
deregister_volume_monitor (PortableServer_Servant _servant,
			   const GNOME_VFS_Client client,
			   CORBA_Environment * ev)
{
	disconnect_client_from_volume_monitor (client);
}

static GNOME_VFS_VolumeList *
get_volumes (PortableServer_Servant _servant,
	     const GNOME_VFS_Client client,
	     CORBA_Environment * ev)
{
	GList *volumes, *l;
	GnomeVFSVolumeMonitor *monitor;
	GNOME_VFS_VolumeList *list;
	int len, i;
	
	monitor = gnome_vfs_get_volume_monitor ();

	volumes = gnome_vfs_volume_monitor_get_mounted_volumes (monitor);

	len = g_list_length (volumes);
	list = CORBA_sequence_GNOME_VFS_Volume__alloc ();
	list->_buffer = CORBA_sequence_GNOME_VFS_Volume_allocbuf (len);
	list->_release = CORBA_TRUE;
	list->_length = len;
	list->_maximum = len;

	for (i = 0, l = volumes; l != NULL; l = l->next, i++) {
		gnome_vfs_volume_to_corba (l->data, &list->_buffer[i]);
		gnome_vfs_volume_unref (l->data);
	}
	g_list_free (volumes);

	return list;
}

static GNOME_VFS_DriveList *
get_drives (PortableServer_Servant _servant,
	    const GNOME_VFS_Client client,
	    CORBA_Environment * ev)
{
	GList *drives, *l;
	GnomeVFSVolumeMonitor *monitor;
	GNOME_VFS_DriveList *list;
	int len, i;
	
	monitor = gnome_vfs_get_volume_monitor ();

	drives = gnome_vfs_volume_monitor_get_connected_drives (monitor);

	len = g_list_length (drives);
	list = CORBA_sequence_GNOME_VFS_Drive__alloc ();
	list->_buffer = CORBA_sequence_GNOME_VFS_Drive_allocbuf (len);
	list->_release = CORBA_TRUE;
	list->_length = len;
	list->_maximum = len;

	for (i = 0, l = drives; l != NULL; l = l->next, i++) {
		gnome_vfs_drive_to_corba (l->data, &list->_buffer[i]);
		gnome_vfs_drive_unref (l->data);
	}
	g_list_free (drives);

	return list;
}

static void
emit_pre_unmount_volume (PortableServer_Servant _servant,
			 const GNOME_VFS_Client client,
			 const CORBA_long id,
			 CORBA_Environment * ev)
{
	GnomeVFSVolumeMonitor *monitor;
	GnomeVFSVolume *volume;
	
	monitor = gnome_vfs_get_volume_monitor ();

	volume = gnome_vfs_volume_monitor_get_volume_by_id (monitor, id);
	if (volume != NULL) {
		gnome_vfs_volume_monitor_emit_pre_unmount (monitor,
							   volume);
		gnome_vfs_volume_unref (volume);
	}
}

static void
force_probe (PortableServer_Servant _servant,
	     const GNOME_VFS_Client client,
	     CORBA_Environment * ev)
{
	GnomeVFSVolumeMonitor *monitor;
	
	monitor = gnome_vfs_get_volume_monitor ();

	gnome_vfs_volume_monitor_daemon_force_probe (monitor);
}


void
gnome_vfs_daemon_add_context (const GNOME_VFS_Client client,
			      GnomeVFSContext *context)
{
	
	ClientInfo *client_info;
	
	G_LOCK (client_list);
	client_info = lookup_client (client);
	if (client_info) {
		client_info->outstanding_contexts = g_list_prepend (client_info->outstanding_contexts,
								    context);
	}
	G_UNLOCK (client_list);
}

void
gnome_vfs_daemon_remove_context (const GNOME_VFS_Client client,
				 GnomeVFSContext *context)
{
	
	ClientInfo *client_info;
	
	G_LOCK (client_list);
	client_info = lookup_client (client);
	if (client_info) {
		client_info->outstanding_contexts = g_list_remove (client_info->outstanding_contexts,
								   context);
	}
	G_UNLOCK (client_list);
}


void
gnome_vfs_daemon_add_client_handle (const GNOME_VFS_Client client,
				    GnomeVFSDaemonHandle *handle)
{
	ClientInfo *client_info;
	
	G_LOCK (client_list);
	client_info = lookup_client (client);
	if (client_info) {
		client_info->outstanding_handles = g_list_prepend (client_info->outstanding_handles,
								   handle);
	}
	G_UNLOCK (client_list);
}

void
gnome_vfs_daemon_remove_client_handle (const GNOME_VFS_Client client,
				       GnomeVFSDaemonHandle *handle)
{
	ClientInfo *client_info;
	
	G_LOCK (client_list);
	client_info = lookup_client (client);
	if (client_info) {
		client_info->outstanding_handles = g_list_remove (client_info->outstanding_handles,
								  handle);
	}
	G_UNLOCK (client_list);
}


void
gnome_vfs_daemon_add_client_dir_handle (const GNOME_VFS_Client client,
					GnomeVFSDaemonDirHandle *handle)
{
	ClientInfo *client_info;
	
	G_LOCK (client_list);
	client_info = lookup_client (client);
	if (client_info) {
		client_info->outstanding_dir_handles = g_list_prepend (client_info->outstanding_dir_handles,
								       handle);
	}
	G_UNLOCK (client_list);
}

void
gnome_vfs_daemon_remove_client_dir_handle (const GNOME_VFS_Client client,
					   GnomeVFSDaemonDirHandle *handle)
{
	ClientInfo *client_info;
	
	G_LOCK (client_list);
	client_info = lookup_client (client);
	if (client_info) {
		client_info->outstanding_dir_handles = g_list_remove (client_info->outstanding_dir_handles,
								      handle);
	}
	G_UNLOCK (client_list);
}


static void
gnome_vfs_daemon_finalize (GObject *object)
{
	BONOBO_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
gnome_vfs_daemon_instance_init (GnomeVFSDaemon *daemon_obj)
{
	bonobo_object_set_immortal (BONOBO_OBJECT (daemon_obj), TRUE);
}

static void
gnome_vfs_daemon_class_init (GnomeVFSDaemonClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_GNOME_VFS_Daemon__epv *epv = &klass->epv;

	object_class->finalize = gnome_vfs_daemon_finalize;
	
	epv->registerClient   = register_client;
	epv->deRegisterClient = de_register_client;

	epv->registerVolumeMonitor = register_volume_monitor;
	epv->deRegisterVolumeMonitor = deregister_volume_monitor;
	epv->getVolumes = get_volumes;
	epv->getDrives = get_drives;
	epv->emitPreUnmountVolume = emit_pre_unmount_volume;
	epv->forceProbe = force_probe;
	
	gnome_vfs_init ();
}

static BonoboObject *
gnome_vfs_daemon_factory (BonoboGenericFactory *factory,
			  const char           *component_id,
			  gpointer              closure)
{
        PortableServer_POA poa;
	
	if (!the_daemon) {
		the_daemon = g_object_new (GNOME_TYPE_VFS_DAEMON, NULL);

		poa = bonobo_poa_get_threaded (ORBIT_THREAD_HINT_PER_REQUEST);
		the_async_daemon = g_object_new (GNOME_TYPE_VFS_ASYNC_DAEMON,
					     "poa", poa,
					     NULL);
		CORBA_Object_release ((CORBA_Object)poa, NULL);
		bonobo_object_add_interface (BONOBO_OBJECT (the_daemon),
					     BONOBO_OBJECT (the_async_daemon));
	}
	return BONOBO_OBJECT (the_daemon);
}

int
main (int argc, char *argv [])
{
	BonoboGenericFactory *factory;
	
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, GNOMEVFS_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	if (!bonobo_init (&argc, argv)) {
		g_error (_("Could not initialize Bonobo"));
		return 1;
	}

	gnome_vfs_set_is_daemon (GNOME_VFS_TYPE_VOLUME_MONITOR_DAEMON,
				 gnome_vfs_volume_monitor_daemon_force_probe);
	if (!gnome_vfs_init ()) {
		g_error (_("Could not initialize gnome vfs"));
		return 1;
	}

	/* Call this here so the volume monitor exists when registerVolumeMonitor()
	 * calls arrive. Otherwise there is a deadlock situation wrt reentrancy
	 * since the constructor does corba calls (gconf).
	 */
	gnome_vfs_get_volume_monitor ();
	
	factory = bonobo_generic_factory_new ("OAFIID:GNOME_VFS_Daemon_Factory",
					      gnome_vfs_daemon_factory,
					      NULL);
	
	if (factory) {
		bonobo_main ();
		
		bonobo_object_unref (BONOBO_OBJECT (factory));

		if (the_daemon) {
			bonobo_object_set_immortal (BONOBO_OBJECT (the_daemon), FALSE);
			bonobo_object_unref (BONOBO_OBJECT (the_daemon));
		}

		gnome_vfs_shutdown();
		gconf_debug_shutdown();
		return bonobo_debug_shutdown ();
	} else {
		g_warning ("Failed to create factory\n");
		return 1;
	}
}
