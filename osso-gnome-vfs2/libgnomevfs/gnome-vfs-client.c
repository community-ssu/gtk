#include <unistd.h>
#include <bonobo-activation/bonobo-activation.h>
#include <libbonobo.h>
#include "gnome-vfs-client.h"
#include "gnome-vfs-cancellation-private.h"
#include "gnome-vfs-volume-monitor-private.h"
#include "gnome-vfs-volume-monitor-client.h"

BONOBO_CLASS_BOILERPLATE_FULL(
	GnomeVFSClient,
	gnome_vfs_client,
	GNOME_VFS_Client,
	BonoboObject,
	BONOBO_TYPE_OBJECT);


struct _GnomeVFSClientPrivate {
	GNOME_VFS_Daemon daemon;
	GNOME_VFS_AsyncDaemon async_daemon;
};

static void activate_daemon (GnomeVFSClient *client);

static GnomeVFSClient *the_client = NULL;
G_LOCK_DEFINE_STATIC (the_client);

/* These are used for client calls to avoid reentrancy except for
 * client_poa objects, which are used for client_calls to allow
 * auth callbacks on the main loop.
 */
static ORBitPolicy *client_policy = NULL;
static PortableServer_POA client_poa;

static void
gnome_vfs_client_finalize (GObject *object)
{
	GnomeVFSClient *client = GNOME_VFS_CLIENT (object);

	if (client->priv->async_daemon != CORBA_OBJECT_NIL) {
		CORBA_Object_release (client->priv->async_daemon, NULL);
		client->priv->async_daemon = CORBA_OBJECT_NIL;
	}
	if (client->priv->daemon != CORBA_OBJECT_NIL) {
		CORBA_Object_release (client->priv->daemon, NULL);
		client->priv->daemon = CORBA_OBJECT_NIL;
	}
	
	g_free (client->priv);
	
	BONOBO_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
gnome_vfs_client_instance_init (GnomeVFSClient *client)
{
	client->priv = g_new0 (GnomeVFSClientPrivate, 1);
}

static void
gnome_vfs_client_monitor_callback (PortableServer_Servant _servant,
				   const GNOME_VFS_DaemonMonitor monitor,
				   const CORBA_char * monitor_uri,
				   const CORBA_char * info_uri,
				   const CORBA_long event_type,
				   CORBA_Environment * ev)
{
  /* DAEMON-TODO: monitor support */
}

static void
gnome_vfs_client_volume_mounted (PortableServer_Servant _servant,
				 const GNOME_VFS_Volume * corba_volume,
				 CORBA_Environment * ev)
{
	GnomeVFSVolume *volume;
	GnomeVFSVolumeMonitor *volume_monitor;

	volume_monitor = gnome_vfs_get_volume_monitor ();

	volume = _gnome_vfs_volume_from_corba (corba_volume, volume_monitor);
	_gnome_vfs_volume_monitor_mounted (volume_monitor, volume);
	gnome_vfs_volume_unref (volume);
}

static void
gnome_vfs_client_volume_unmounted (PortableServer_Servant _servant,
				   const CORBA_long id, CORBA_Environment * ev)
{
	GnomeVFSVolume *volume;
	GnomeVFSVolumeMonitor *volume_monitor;

	volume_monitor = gnome_vfs_get_volume_monitor ();

	volume = gnome_vfs_volume_monitor_get_volume_by_id (volume_monitor, id);
	if (volume != NULL) {
		_gnome_vfs_volume_monitor_unmounted (volume_monitor, volume);
		gnome_vfs_volume_unref (volume);
	}
}

static void
gnome_vfs_client_volume_pre_unmount (PortableServer_Servant _servant,
				     const CORBA_long id, CORBA_Environment * ev)
{
	GnomeVFSVolume *volume;
	GnomeVFSVolumeMonitor *volume_monitor;

	volume_monitor = gnome_vfs_get_volume_monitor ();

	volume = gnome_vfs_volume_monitor_get_volume_by_id (volume_monitor, id);
	if (volume != NULL) {
		gnome_vfs_volume_monitor_emit_pre_unmount (volume_monitor,
							   volume);
		gnome_vfs_volume_unref (volume);
	}
}


static void
gnome_vfs_client_drive_connected (PortableServer_Servant _servant,
				  const GNOME_VFS_Drive * corba_drive,
				  CORBA_Environment * ev)
{
	GnomeVFSDrive *drive;
	GnomeVFSVolumeMonitor *volume_monitor;

	volume_monitor = gnome_vfs_get_volume_monitor ();

	drive = _gnome_vfs_drive_from_corba (corba_drive, volume_monitor);
	_gnome_vfs_volume_monitor_connected (volume_monitor, drive);
	gnome_vfs_drive_unref (drive);
}

static void
gnome_vfs_client_drive_disconnected (PortableServer_Servant _servant,
				     const CORBA_long id,
				     CORBA_Environment * ev)
{
	GnomeVFSDrive *drive;
	GnomeVFSVolumeMonitor *volume_monitor;

	volume_monitor = gnome_vfs_get_volume_monitor ();

	drive = gnome_vfs_volume_monitor_get_drive_by_id (volume_monitor, id);
	if (drive != NULL) {
		_gnome_vfs_volume_monitor_disconnected (volume_monitor, drive);
		gnome_vfs_drive_unref (drive);
	}
}

static void
gnome_vfs_client_class_init (GnomeVFSClientClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_GNOME_VFS_Client__epv *epv = &klass->epv; 

	epv->MonitorCallback = gnome_vfs_client_monitor_callback;
	epv->VolumeMounted = gnome_vfs_client_volume_mounted;
	epv->VolumeUnmounted = gnome_vfs_client_volume_unmounted;
	epv->VolumePreUnmount = gnome_vfs_client_volume_pre_unmount;
	epv->DriveConnected = gnome_vfs_client_drive_connected;
	epv->DriveDisconnected = gnome_vfs_client_drive_disconnected;
	
	object_class->finalize = gnome_vfs_client_finalize;
}


static void
daemon_connection_broken (gpointer connection,
			  GnomeVFSClient *client)
{
	GnomeVFSVolumeMonitor *volume_monitor;

	/* This is run in an idle, so some code might run between the
	 * connection going bork and this code running.
	 */
	G_LOCK (the_client);
	CORBA_Object_release (client->priv->daemon, NULL);
	client->priv->daemon = CORBA_OBJECT_NIL;
	CORBA_Object_release (client->priv->async_daemon, NULL);
	client->priv->async_daemon = CORBA_OBJECT_NIL;

	/* DAEMON-TODO: Free all objects tied to the daemon:
	 * DaemonMonitor - free, mark for recreation on daemon reconnect
	 */

	/* Outstanding file and directory daemon handles will keep giving
	 * I/O errors and they will be freed on close of the
	 * corresponding handle close.
	 */

	G_UNLOCK (the_client);

	volume_monitor = _gnome_vfs_get_volume_monitor_internal (FALSE);
	if (volume_monitor != NULL) {
		/* We need to remove all the old volumes/drives since
		 * their ids are not the same for the next daemon.
		 */
		_gnome_vfs_volume_monitor_client_daemon_died (GNOME_VFS_VOLUME_MONITOR_CLIENT (volume_monitor));
	}

}


/* Run with the the_daemon lock held */
static void
activate_daemon (GnomeVFSClient *client)
{
	CORBA_Environment  ev;
	
	CORBA_exception_init (&ev);
	/* DAEMON-TODO: This call isn't really threadsafe */
	client->priv->daemon = bonobo_activation_activate_from_id ("OAFIID:GNOME_VFS_Daemon",
								   0, NULL, &ev);
	CORBA_exception_free (&ev);

	if (client->priv->daemon != CORBA_OBJECT_NIL) {
		/* Don't allow reentrancy on object */
		ORBit_object_set_policy  ((CORBA_Object) client->priv->daemon,
					  client_policy);

		CORBA_exception_init (&ev);
		
		/* Should not deadlock due to disabled reentrancy */
		GNOME_VFS_Daemon_registerClient (client->priv->daemon, BONOBO_OBJREF (client), &ev);
		
		/* If the registration fails for some reason we release the
		 * daemon object and return NIL. */
		if (BONOBO_EX (&ev)) {
			CORBA_exception_free (&ev);
			CORBA_Object_release (client->priv->daemon, NULL);
			client->priv->daemon = CORBA_OBJECT_NIL;
		}
	}
	
	if (client->priv->daemon != CORBA_OBJECT_NIL) {
		ORBit_small_listen_for_broken (client->priv->daemon, G_CALLBACK (daemon_connection_broken), client);
		
		/* DAEMON-TODO: Set up monitors that were up before a previous daemon disconnected */
	}
}

/**
 * gnome_vfs_client_get_daemon:
 * @client: The client object
 *
 * Returns a local duplicate of the daemon reference.
 * The client is guaranteed to be registred with the daemon.
 * May return CORBA_OBJECT_NIL. May return an object where the
 * connection has died. Safe to call from a thread. 
 */
GNOME_VFS_Daemon
_gnome_vfs_client_get_daemon (GnomeVFSClient *client)
{
	GNOME_VFS_Daemon daemon;

	G_LOCK (the_client);
	
	if (client->priv->daemon == CORBA_OBJECT_NIL)
		activate_daemon (client);

	if (client->priv->daemon != CORBA_OBJECT_NIL) 
		daemon = CORBA_Object_duplicate (client->priv->daemon, NULL);
	else
		daemon = CORBA_OBJECT_NIL;
	
	G_UNLOCK (the_client);
	
	return daemon;
}

/**
 * gnome_vfs_client_get_async_daemon:
 * @client: The client object
 *
 * Returns a local duplicate of the asyncdaemon reference.
 * The client is guaranteed to be registred with the daemon.
 * May return CORBA_OBJECT_NIL. May return an object where the
 * connection has died. Safe to call from a thread. 
 */
GNOME_VFS_AsyncDaemon
_gnome_vfs_client_get_async_daemon (GnomeVFSClient *client)
{
	GNOME_VFS_AsyncDaemon async_daemon;
	CORBA_Environment  ev;
	
	G_LOCK (the_client);

	async_daemon = CORBA_OBJECT_NIL;
	if (client->priv->async_daemon == CORBA_OBJECT_NIL) {
		if (client->priv->daemon == CORBA_OBJECT_NIL)
			activate_daemon (client);

		if (client->priv->daemon != CORBA_OBJECT_NIL) {
			CORBA_exception_init (&ev);
			client->priv->async_daemon = Bonobo_Unknown_queryInterface
				(client->priv->daemon, "IDL:GNOME/VFS/AsyncDaemon:1.0", &ev);
			if (client->priv->async_daemon == CORBA_OBJECT_NIL) {
				CORBA_exception_free (&ev);
				g_warning ("Failed to get async daemon interface");
			} else {
				/* Don't allow reentrancy on object */
				ORBit_object_set_policy  ((CORBA_Object) client->priv->async_daemon,
							  client_policy);
			}
		}
	}
	
	if (client->priv->async_daemon != CORBA_OBJECT_NIL) {
		async_daemon = CORBA_Object_duplicate (client->priv->async_daemon, NULL);
	}

	G_UNLOCK (the_client);
	
	return async_daemon;
}

ORBitPolicy *
_gnome_vfs_get_client_policy (void)
{
	return client_policy;
}

PortableServer_POA
_gnome_vfs_get_client_poa (void)
{
	return client_poa;
}

/* Returns local singleton object */
GnomeVFSClient *
_gnome_vfs_get_client (void)
{
	PortableServer_POA idle_poa;
	/* DAEMON-TODO: Policies and "allow" isn't actually implemented in ORBit2 yet */
	
	G_LOCK (the_client);
	if (the_client == NULL) {
		client_poa = bonobo_poa_get_threaded (ORBIT_THREAD_HINT_PER_OBJECT);
		if (client_poa == CORBA_OBJECT_NIL) {
			g_error ("Can't allocate gnome-vfs client POA");
			G_UNLOCK (the_client);
			return NULL;
		}
		
		client_policy = ORBit_policy_new (ORBIT_TYPE_POLICY_EX,
						  "allow", client_poa,
						  NULL);

		/* All Client callback happens in idle */
		idle_poa = bonobo_poa_get_threaded (ORBIT_THREAD_HINT_ALL_AT_IDLE);
		the_client = g_object_new (GNOME_TYPE_VFS_CLIENT,
					   "poa", idle_poa, NULL);
		CORBA_Object_release ((CORBA_Object) idle_poa, NULL);
	}
	G_UNLOCK (the_client);
	
	return the_client;
}

void
_gnome_vfs_client_shutdown (void)
{
	GnomeVFSClient *client;

	/* Free the main thread client_call */
	_gnome_vfs_client_call_destroy ();

	client = NULL;
	G_LOCK (the_client);
	if (the_client != NULL) {
		client = the_client;
		the_client = NULL;
	}
	G_UNLOCK (the_client);
	
	if (client != NULL) {
		bonobo_object_unref (client);
		
		ORBit_policy_unref (client_policy);
		client_policy = NULL;
		
		CORBA_Object_release ((CORBA_Object) client_poa, NULL);
		client_poa = CORBA_OBJECT_NIL;
	}
}
