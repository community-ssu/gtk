#ifndef GNOME_VFS_BACKEND_H
#define GNOME_VFS_BACKEND_H

#include <libgnomevfs/gnome-vfs-context.h>
#include <libgnomevfs/gnome-vfs-module-callback.h>

G_BEGIN_DECLS

void	    _gnome_vfs_get_current_context       (/* OUT */ GnomeVFSContext **context);
void        _gnome_vfs_dispatch_module_callback  (GnomeVFSAsyncModuleCallback    callback,
						 gconstpointer                  in,
						 gsize                          in_size,
						 gpointer                       out, 
						 gsize                          out_size,
						 gpointer                       user_data,
						 GnomeVFSModuleCallbackResponse response,
						 gpointer                       response_data);


G_END_DECLS

#endif
