#ifndef _GNOME_VFS_CLIENT_H_
#define _GNOME_VFS_CLIENT_H_

#include <bonobo/bonobo-object.h>
#include <libgnomevfs/gnome-vfs-context.h>
#include <GNOME_VFS_Daemon.h>

G_BEGIN_DECLS

typedef struct _GnomeVFSClient GnomeVFSClient;

#define GNOME_TYPE_VFS_CLIENT        (gnome_vfs_client_get_type ())
#define GNOME_VFS_CLIENT(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_TYPE_VFS_CLIENT, GnomeVFSClient))
#define GNOME_VFS_CLIENT_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), GNOME_TYPE_VFS_CLIENT, GnomeVFSClientClass))
#define GNOME_IS_VFS_CLIENT(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_TYPE_VFS_CLIENT))
#define GNOME_IS_VFS_CLIENT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_TYPE_VFS_CLIENT))

typedef struct _GnomeVFSClientPrivate GnomeVFSClientPrivate;

struct _GnomeVFSClient {
	BonoboObject parent;

        GnomeVFSClientPrivate *priv;
};

typedef struct {
	BonoboObjectClass parent_class;

	POA_GNOME_VFS_Client__epv epv;
} GnomeVFSClientClass;

GType gnome_vfs_client_get_type (void) G_GNUC_CONST;
GNOME_VFS_Daemon      _gnome_vfs_client_get_daemon             (GnomeVFSClient       *client);
GNOME_VFS_AsyncDaemon _gnome_vfs_client_get_async_daemon       (GnomeVFSClient       *client);
GnomeVFSClient *      _gnome_vfs_get_client                    (void);
void                  _gnome_vfs_client_shutdown               (void);
ORBitPolicy *         _gnome_vfs_get_client_policy             (void);
PortableServer_POA    _gnome_vfs_get_client_poa                (void);


G_END_DECLS

#endif /* _GNOME_VFS_CLIENT_H_ */
