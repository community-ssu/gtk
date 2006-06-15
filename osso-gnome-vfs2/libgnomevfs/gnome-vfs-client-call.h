#ifndef _GNOME_VFS_CLIENT_CALL_H_
#define _GNOME_VFS_CLIENT_CALL_H_

#include <bonobo/bonobo-object.h>
#include <GNOME_VFS_Daemon.h>
#include "gnome-vfs-context.h"

G_BEGIN_DECLS

typedef struct _GnomeVFSClientCall GnomeVFSClientCall;

#define GNOME_TYPE_VFS_CLIENT_CALL        (gnome_vfs_client_call_get_type ())
#define GNOME_VFS_CLIENT_CALL(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_TYPE_VFS_CLIENT_CALL, GnomeVFSClientCall))
#define GNOME_VFS_CLIENT_CALL_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), GNOME_TYPE_VFS_CLIENT_CALL, GnomeVFSClientCallClass))
#define GNOME_IS_VFS_CLIENT_CALL(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_TYPE_VFS_CLIENT_CALL))
#define GNOME_IS_VFS_CLIENT_CALL_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_TYPE_VFS_CLIENT_CALL))

struct _GnomeVFSClientCall {
	BonoboObject parent;

	GMutex *delay_finish_mutex;
	GCond *delay_finish_cond;
	gboolean delay_finish;
};

typedef struct {
	BonoboObjectClass parent_class;

	POA_GNOME_VFS_ClientCall__epv epv;
} GnomeVFSClientCallClass;

GType gnome_vfs_client_call_get_type (void) G_GNUC_CONST;

GnomeVFSClientCall *_gnome_vfs_client_call_get               (GnomeVFSContext    *context);
void                _gnome_vfs_client_call_destroy           (void);
void                _gnome_vfs_client_call_finished          (GnomeVFSClientCall *client_call,
							      GnomeVFSContext    *context);
void                _gnome_vfs_client_call_delay_finish      (GnomeVFSClientCall *client_call);
void                _gnome_vfs_client_call_delay_finish_done (GnomeVFSClientCall *client_call);

GNOME_VFS_ClientCall _gnome_vfs_daemon_get_current_daemon_client_call (void);
void                 gnome_vfs_daemon_set_current_daemon_client_call (GNOME_VFS_ClientCall client_call);

G_END_DECLS

#endif /* _GNOME_VFS_CLIENT_CALL_H_ */
