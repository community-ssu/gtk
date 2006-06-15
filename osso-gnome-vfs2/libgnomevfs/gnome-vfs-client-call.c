#include <config.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-generic-factory.h>
#include <libgnomevfs/gnome-vfs.h>
#include "gnome-vfs-client.h"
#include "gnome-vfs-client-call.h"
#include "gnome-vfs-cancellable-ops.h"
#include "gnome-vfs-cancellation-private.h"
#include "gnome-vfs-module-callback-private.h"

BONOBO_CLASS_BOILERPLATE_FULL(
	GnomeVFSClientCall,
	gnome_vfs_client_call,
	GNOME_VFS_ClientCall,
	BonoboObject,
	BONOBO_TYPE_OBJECT);


static GStaticPrivate daemon_client_call_private = G_STATIC_PRIVATE_INIT;


GNOME_VFS_ClientCall
_gnome_vfs_daemon_get_current_daemon_client_call (void)
{
	return (GNOME_VFS_ClientCall) g_static_private_get (&daemon_client_call_private);
}

void
gnome_vfs_daemon_set_current_daemon_client_call (GNOME_VFS_ClientCall client_call)
{
	g_static_private_set (&daemon_client_call_private, client_call, NULL);
}


static GStaticPrivate client_call_private = G_STATIC_PRIVATE_INIT;

static void
gnome_vfs_client_call_finalize (GObject *object)
{
	GnomeVFSClientCall *client_call;

	client_call = GNOME_VFS_CLIENT_CALL (object);
	g_mutex_free (client_call->delay_finish_mutex);
	g_cond_free (client_call->delay_finish_cond);
	
	BONOBO_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
gnome_vfs_client_call_instance_init (GnomeVFSClientCall *client_call)
{
	
	client_call->delay_finish_cond = g_cond_new ();
	client_call->delay_finish_mutex = g_mutex_new ();
	client_call->delay_finish = FALSE;
}

static CORBA_boolean
module_callback_invoke (PortableServer_Servant _servant,
			const CORBA_char * name,
			const CORBA_any * module_in,
			CORBA_any ** module_out,
			CORBA_Environment * ev)
{
	return _gnome_vfs_module_callback_demarshal_invoke (name, module_in, module_out);
}


static void
gnome_vfs_client_call_class_init (GnomeVFSClientCallClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_GNOME_VFS_ClientCall__epv *epv = &klass->epv;

	epv->ModuleCallbackInvoke = module_callback_invoke;
	
	object_class->finalize = gnome_vfs_client_call_finalize;
}

void
_gnome_vfs_client_call_delay_finish (GnomeVFSClientCall *client_call)
{
	g_mutex_lock (client_call->delay_finish_mutex);
	g_assert (!client_call->delay_finish);
	client_call->delay_finish = TRUE;
	g_mutex_unlock (client_call->delay_finish_mutex);
}

void
_gnome_vfs_client_call_delay_finish_done (GnomeVFSClientCall *client_call)
{
	g_mutex_lock (client_call->delay_finish_mutex);
	g_assert (client_call->delay_finish);
	client_call->delay_finish = FALSE;
	g_cond_signal (client_call->delay_finish_cond);
	g_mutex_unlock (client_call->delay_finish_mutex);
}

GnomeVFSClientCall *
_gnome_vfs_client_call_get (GnomeVFSContext *context)
{
	GnomeVFSClientCall *client_call;
	GnomeVFSCancellation *cancellation;

	client_call = g_static_private_get (&client_call_private);

	/* DAEMON-TODO: If this is called on non-vfs threads the client_call
	 * object won't be destroyed at gnome_vfs_shutdown() time, except for
	 * the main thread one (which is manually destroyed in
	 * _gnome_vfs_client_call_destroy
	 */
	if (client_call == NULL) {
		client_call = g_object_new (GNOME_TYPE_VFS_CLIENT_CALL,
					    "poa", _gnome_vfs_get_client_poa (),
					    NULL);
		ORBit_ObjectAdaptor_object_bind_to_current_thread (BONOBO_OBJREF (client_call));
		g_static_private_set (&client_call_private,
				      client_call, (GDestroyNotify)bonobo_object_unref);
	}

	if (context != NULL) {
		cancellation = gnome_vfs_context_get_cancellation (context);
		if (cancellation != NULL) {
			_gnome_vfs_cancellation_add_client_call (cancellation,
								 client_call);
		}
	}
	
	return client_call;
}


void
_gnome_vfs_client_call_destroy (void)
{
	GnomeVFSClientCall *client_call;
	client_call = g_static_private_get (&client_call_private);

	if (client_call != NULL) {
		g_static_private_set (&client_call_private,
				      NULL, NULL);
	}	
}


void
_gnome_vfs_client_call_finished (GnomeVFSClientCall *client_call,
				 GnomeVFSContext *context)
{
	GnomeVFSCancellation *cancellation;
	
	if (context != NULL) {
		cancellation = gnome_vfs_context_get_cancellation (context);
		if (cancellation != NULL) {
			_gnome_vfs_cancellation_remove_client_call (cancellation,
								    client_call);
		}
	}

	g_mutex_lock (client_call->delay_finish_mutex);
	if (client_call->delay_finish) {
		g_cond_wait (client_call->delay_finish_cond,
			     client_call->delay_finish_mutex);
	}
	g_assert (!client_call->delay_finish);
	g_mutex_unlock (client_call->delay_finish_mutex);
}
