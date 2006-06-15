#ifndef GNOME_VFS_PTHREAD_H
#define GNOME_VFS_PTHREAD_H

#include <glib/gtypes.h>
#include <pthread.h>

gboolean gnome_vfs_pthread_init                 (gboolean          init_deps);
int      gnome_vfs_pthread_recursive_mutex_init (pthread_mutex_t *);

#endif
